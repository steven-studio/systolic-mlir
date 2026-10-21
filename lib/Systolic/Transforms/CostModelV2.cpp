#include "Systolic/CostModelV2.h"

#include <algorithm>
#include <limits>
#include <map>

namespace systolic {
namespace v2 {

namespace {

constexpr int64_t kNone = -1;

int64_t ceilDiv(int64_t a, int64_t b) { return a / b + (a % b != 0 ? 1 : 0); }

int64_t slabBytesOf(const FleetHardware &hw) {
  if (hw.slabBytes > 0)
    return hw.slabBytes;
  int64_t windows = 0;
  for (const ContextConfig &c : hw.contexts)
    windows = std::max(windows, ceilDiv(c.kMax, hw.kWindow));
  return windows * hw.kWindow * hw.wireLanes * 2 * hw.elementBytes;
}

// ---------------------------------------------------------------------------
// The readback output path: dpti_async_fifo (two-flop Gray pointer
// synchronizers both ways) -> dpti_beat_to_byte -> dpti_sync245_phy TX, the
// host always taking (TXE# low) and no RX traffic. The writer side moves on
// array-clock edges, the rest on DPTI edges; the logic is the RTL's,
// register for register (dpti_async_fifo.sv, dpti_beat_to_byte.sv,
// dpti_sync245_phy.sv M_IDLE / M_TX).
// ---------------------------------------------------------------------------
class OutputPath {
public:
  explicit OutputPath(const FleetHardware &hw)
      : depth(hw.rbFifoDepth), txDepth(hw.txFifoDepth), turn(hw.turnCycles),
        beatBytes(hw.beatBytes) {}

  bool full() const { return mod(wr - rdW2) == depth; }

  bool quiescent() const {
    return wr == rd && rdW1 == rd && rdW2 == rd && wrR1 == wr && wrR2 == wr &&
           !bBusy && txCount == 0 && mode == Mode::Idle && turnCnt >= turn &&
           wrN && !wrLowPrev;
  }

  // Array-clock edge: the read pointer is synchronized (its value now), then
  // `writes` beats are committed.
  void uiEdge(int writes) {
    rdW2 = rdW1;
    rdW1 = rd;
    wr = mod(wr + writes);
  }

  // DPTI edge at time `now` (ps).
  void dptiEdge(int64_t now) {
    const bool rdValid = rd != wrR2;
    const bool txAcc = wrLowPrev;                 // TXE# low: every strobe lands
    const bool txLeftGe1 = txAcc ? txCount >= 2 : txCount >= 1;
    const bool txReady = txCount < txDepth;
    const bool pushByte = bBusy && txReady;
    const bool popBeat = !bBusy && rdValid;

    wrR2 = wrR1;
    wrR1 = wr;
    const bool nextWrLowPrev = !wrN;
    if (txAcc) {
      ++bytesOut;
      lastBytePs = now;
    }
    switch (mode) {
    case Mode::Idle: {
      wrN = true;
      const bool turnDone = turnCnt >= turn;
      if (!turnDone)
        ++turnCnt;
      if (turnDone && txLeftGe1) {
        mode = Mode::Tx;
        wrN = false;
      }
      break;
    }
    case Mode::Tx:
      if (!wrN)
        wrN = true;                               // a strobe: follow with a gap
      else if (!txLeftGe1) {
        mode = Mode::Idle;
        turnCnt = 0;
      } else
        wrN = false;
      break;
    }
    wrLowPrev = nextWrLowPrev;
    txCount += (pushByte ? 1 : 0) - (txAcc ? 1 : 0);
    if (pushByte) {
      if (bIdx == beatBytes - 1) {
        bBusy = false;
        bIdx = 0;
      } else {
        ++bIdx;
      }
    } else if (popBeat) {
      bBusy = true;
      bIdx = 0;
      rd = mod(rd + 1);
    }
  }

  int64_t bytesOut = 0;
  int64_t lastBytePs = 0;

private:
  enum class Mode { Idle, Tx };
  int64_t mod(int64_t v) const { return ((v % (2 * depth)) + 2 * depth) % (2 * depth); }

  int64_t depth, txDepth, turn, beatBytes;
  int64_t wr = 0, rd = 0, rdW1 = 0, rdW2 = 0, wrR1 = 0, wrR2 = 0;
  bool bBusy = false;
  int64_t bIdx = 0;
  int64_t txCount = 0;
  Mode mode = Mode::Idle;
  int64_t turnCnt = 2;   // reset value is 0; the path has long been idle at T0
  bool wrN = true, wrLowPrev = false;
};

struct Job {
  FleetJob in;
  int64_t nth = 0;       // its index among its context's jobs
  FleetJobEvents e;
  int64_t wbStart = kNone;
};

enum class RbState { Idle, IdleNext, IdleNext2, HdrNext, Hdr, DataNext, Data };

struct Burst {
  int slot;
  int64_t beats;
};

} // namespace

FleetHardware fpgaFleet(int64_t k8, int64_t k4, bool beatWide, bool pipelined) {
  FleetHardware hw;
  ContextConfig c8;
  c8.rows = 8;
  c8.kMax = k8;
  ContextConfig c4;
  c4.rows = 4;
  c4.kMax = k4;
  if (!pipelined) {
    c8.pipelineDepth = c4.pipelineDepth = 1;
    c8.reduceCycles = c4.reduceCycles = 0;
    c8.feedGap = c4.feedGap = 0;
  }
  hw.contexts = {c8, c4, c4, c4};
  if (beatWide) {
    hw.writerCyclesPerBeat = 1;
    hw.landTail = 2;
    hw.sharedBurstLen = 16;
  }
  return hw;
}

bool isValidFleetHardware(const FleetHardware &hw, std::string *why) {
  auto fail = [&](const char *m) {
    if (why)
      *why = m;
    return false;
  };
  if (hw.contexts.empty())
    return fail("no context");
  if (hw.contexts.size() > 64)
    return fail("more than 64 contexts");
  for (const ContextConfig &c : hw.contexts) {
    if (c.rows < 1 || c.rows > hw.wireLanes)
      return fail("a context's rows are outside 1..wire lanes");
    if (c.kMax < 1)
      return fail("a context without k_max");
    if (c.pipelineDepth < 1 || c.tileOverhead < 0 || c.reduceCycles < 0 ||
        c.feedGap < 0)
      return fail("a context constant is out of range");
    if ((c.rows * c.rows * hw.elementBytes) % hw.beatBytes != 0)
      return fail("a result tile is not whole beats");
  }
  if (hw.writerCyclesPerBeat < 1 || hw.landTail < 0 || hw.release < 0 ||
      hw.descLatency < 0 || hw.kWindow < 1 || hw.elementBytes < 1 ||
      hw.beatBytes < 1 || hw.burstLen < 1 || hw.sharedBurstLen < 1 ||
      hw.sharedBurstLen > hw.burstLen || hw.maxOutstanding < 1 ||
      hw.pageBytes < hw.beatBytes || hw.rbDepth < 1 || hw.rbFifoDepth < 2 ||
      hw.uiPeriodPs < 1 || hw.dptiPeriodPs < 1 || hw.txFifoDepth < 2 ||
      hw.turnCycles < 0 || hw.operandSlots < 1 || hw.resultSlots < 1)
    return fail("a fleet constant is out of range");
  return true;
}

int64_t operandBeats(int64_t k, const FleetHardware &hw) {
  if (k < 1)
    return 0;
  return ceilDiv(k, hw.kWindow) * hw.kWindow * hw.wireLanes * 2 *
         hw.elementBytes / hw.beatBytes;
}

int64_t aloneReadCycles(int64_t k, const FleetHardware &hw) {
  return hw.writerCyclesPerBeat * (operandBeats(k, hw) - 1) + 3 + hw.landTail;
}

FleetResult simulateFleet(const FleetHardware &hw,
                          const std::vector<FleetJob> &input,
                          const FleetOptions &options) {
  FleetResult result;
  std::string why;
  if (!isValidFleetHardware(hw, &why)) {
    result.error = why;
    return result;
  }
  const int NA = static_cast<int>(hw.contexts.size());
  const int RB = NA;              // the readback slot
  const int NS = NA + 1;
  const int64_t nj = static_cast<int64_t>(input.size());
  const int64_t slab = slabBytesOf(hw);
  const int64_t beatsPerPage = hw.pageBytes / hw.beatBytes;

  std::vector<Job> J(input.size());
  std::vector<std::vector<int64_t>> perDev(NA);
  for (int64_t i = 0; i < nj; ++i) {
    const FleetJob &in = input[i];
    if (in.context < 0 || in.context >= NA) {
      result.error = "job " + std::to_string(i) + " names no context";
      return result;
    }
    const ContextConfig &c = hw.contexts[in.context];
    if (in.k < 1 || in.k > c.kMax) {
      result.error = "job " + std::to_string(i) + " has k outside 1..k_max";
      return result;
    }
    if (in.dep >= i || in.present < 0) {
      result.error = "job " + std::to_string(i) + " has a bad dependence";
      return result;
    }
    J[i].in = in;
    J[i].nth = static_cast<int64_t>(perDev[in.context].size());
    perDev[in.context].push_back(i);
  }

  // ---- the read engine (dma_engine_multi.sv) --------------------------------
  std::vector<bool> act(NS, false);
  std::vector<int64_t> issueLeft(NS, 0), retLeft(NS, 0), addrBeat(NS, 0),
      owner(NS, kNone);
  bool arqValid = false;
  Burst arq{0, 0};
  int64_t inFlight = 0;           // bursts committed and not fully returned
  int rr = 0;
  std::vector<Burst> mig;         // bursts the memory has accepted, in order
  size_t migHead = 0;
  bool rvValid = false;
  Burst rv{0, 0};                 // the burst whose beat is on R
  std::vector<bool> wBusy(NS, false);
  std::vector<int64_t> wCnt(NS, 0);

  // ---- write-back engine (systolic_dma_top.sv, dma_writeback_engine.sv) ------
  bool wbInflight = false;
  int64_t wbFreeAt = 0;

  // ---- readback (systolic_dma_top.sv RB_IDLE / RB_HDR / RB_DATA) -------------
  RbState rbState = RbState::Idle;
  int64_t rbJob = kNone;
  int rbLast = NA - 1;            // context 0 first after reset
  std::vector<int64_t> nTaken(NA, 0);
  OutputPath out(hw);

  // ---- port and per-context progress ----------------------------------------
  int64_t nxt = 0;                // InOrder: the next job to offer
  int64_t firePendingJob = kNone, firePendingAt = kNone;
  int64_t lastFire = kNone;
  std::vector<int64_t> nFire(NA, 0), nDone(NA, 0), nRbDone(NA, 0), lo(NA, 0);
  std::vector<int64_t> nWbAcc(NA, 0);           // jobs granted the write-back engine
  std::vector<int64_t> nextUnfired(NA, 0);      // Credit: per-context cursor
  int64_t fired = 0;
  std::multimap<int64_t, std::pair<int, int64_t>> descAt;   // cycle -> (slot, job)

  FleetStats &st = result.stats;
  st.burstLengths.assign(hw.burstLen + 1, 0);

  auto known = [](int64_t v) { return v != kNone; };
  int64_t m = 0;                  // DPTI edges done
  int64_t t = 0;

  auto offerOf = [&](int64_t i) -> int64_t {
    const Job &j = J[i];
    int64_t off = std::max<int64_t>(j.in.present, 1);
    if (known(lastFire))
      off = std::max(off, lastFire + 1);
    if (j.in.dep >= 0) {
      const int64_t d = J[j.in.dep].e.done;
      if (!known(d))
        return kNone;
      off = std::max({off, d + 1, d + j.in.gap});
    }
    return off;
  };
  auto room = [&](int dv) {
    const ContextConfig &c = hw.contexts[dv];
    if (nFire[dv] - nDone[dv] >= c.pipelineDepth)
      return false;
    if (hw.readback && nFire[dv] - nRbDone[dv] >= hw.rbDepth)
      return false;
    return true;
  };

  // Progress: anything that changes the machine's state. A stretch of
  // kDeadlockCycles without any while something is in flight is a deadlock.
  int64_t progress = 0, lastProgressCount = -1, lastProgressCycle = 0;
  int64_t bytesOutSeen = 0;

  while (true) {
    const int64_t E = t * hw.uiPeriodPs;
    while (hw.dptiPhasePs + m * hw.dptiPeriodPs < E) {
      out.dptiEdge(hw.dptiPhasePs + m * hw.dptiPeriodPs);
      ++m;
    }

    // ---- events strictly before this cycle ----------------------------------
    for (int dv = 0; dv < NA; ++dv) {
      const auto &lst = perDev[dv];
      while (nDone[dv] < (int64_t)lst.size() && known(J[lst[nDone[dv]]].e.done) &&
             J[lst[nDone[dv]]].e.done < t)
        ++nDone[dv];
      while (nRbDone[dv] < (int64_t)lst.size() &&
             known(J[lst[nRbDone[dv]]].e.rbDone) && J[lst[nRbDone[dv]]].e.rbDone < t)
        ++nRbDone[dv];
    }

    // ---- the job port: a registered acceptance (systolic_dma_top.sv accept_q):
    // the job offered at cycle s with its context ready fires at s + 1, and
    // the cycle after a fire accepts nothing ------------------------------------
    if (known(firePendingJob) && firePendingAt == t) {
      Job &j = J[firePendingJob];
      j.e.fire = t;
      lastFire = t;
      ++progress;
      ++nFire[j.in.context];
      ++fired;
      if (options.port == PortPolicy::InOrder)
        nxt = firePendingJob + 1;
      else
        ++nextUnfired[j.in.context];
      firePendingJob = kNone;
    }
    if (!known(firePendingJob)) {
      if (options.port == PortPolicy::InOrder) {
        if (nxt < nj) {
          const int64_t off = offerOf(nxt);
          J[nxt].e.offer = off;
          if (known(off) && t >= off && room(J[nxt].in.context)) {
            firePendingJob = nxt;
            firePendingAt = t + 1;
          }
        }
      } else {
        for (int dv = 0; dv < NA; ++dv) {
          if (nextUnfired[dv] >= (int64_t)perDev[dv].size())
            continue;
          const int64_t i = perDev[dv][nextUnfired[dv]];
          const int64_t off = offerOf(i);
          if (known(off) && t >= off && room(dv)) {
            J[i].e.offer = std::max(off, t);
            firePendingJob = i;
            firePendingAt = t + 1;
            break;
          }
        }
      }
    }

    // ---- the contexts: V1's recurrences, the fill's own length from the
    // engine (systolic_accel_context.sv: fill_ok, the release handshake, the
    // two accumulator sets, the result banks, the write-back stage) -----------
    for (int dv = 0; dv < NA; ++dv) {
      const ContextConfig &c = hw.contexts[dv];
      const auto &lst = perDev[dv];
      while (lo[dv] < (int64_t)lst.size() && known(J[lst[lo[dv]]].e.wbReq))
        ++lo[dv];
      for (int64_t n = lo[dv]; n < (int64_t)lst.size(); ++n) {
        Job &j = J[lst[n]];
        if (!known(j.e.fire))
          break;
        Job *prv = n >= 1 ? &J[lst[n - 1]] : nullptr;
        Job *pp = n >= 2 ? &J[lst[n - 2]] : nullptr;
        if (!known(j.e.fillStart) &&
            (!prv || (known(prv->e.land) && known(prv->e.fold)))) {
          int64_t f = j.e.fire;
          if (prv)
            f = std::max({f, prv->e.land + hw.release, prv->e.fold});
          j.e.fillStart = f;
          j.e.desc = f + hw.descLatency;
          descAt.emplace(j.e.desc, std::make_pair(dv, lst[n]));
        }
        if (!known(j.e.fold) && known(j.e.land)) {
          bool ok = true;
          int64_t f = j.e.land + hw.release;
          if (prv) {
            if (!known(prv->e.fold))
              ok = false;
            else
              f = std::max(f, prv->e.fold + prv->in.k + c.feedGap);
          }
          if (pp) {
            if (!known(pp->e.cv) || !known(pp->e.done))
              ok = false;
            else
              f = std::max({f, pp->e.cv + 3, pp->e.done + 3});
          }
          if (ok)
            j.e.fold = f;
        }
        if (!known(j.e.cv) && known(j.e.fold)) {
          int64_t cv = j.e.fold + j.in.k + 2 * c.rows - 2 + c.tileOverhead;
          if (prv) {
            if (!known(prv->e.cv))
              continue;
            cv = std::max(cv, prv->e.cv + c.reduceCycles);
          }
          j.e.cv = cv;
        }
        if (!known(j.e.wbReq) && known(j.e.cv) && (!prv || known(prv->e.done))) {
          j.wbStart = prv ? std::max(j.e.cv, prv->e.done) : j.e.cv;
          j.e.wbReq = j.wbStart + c.rows * c.rows + hw.scanExtra;
        }
      }
    }

    // ---- write-back engine: lowest-numbered request, one tile at a time ------
    if (!wbInflight || t >= wbFreeAt) {
      wbInflight = false;
      for (int dv = 0; dv < NA && !wbInflight; ++dv) {
        // a context's write-backs are granted in its job order: the next
        // one to grant is job nWbAcc[dv]
        if (nWbAcc[dv] < (int64_t)perDev[dv].size()) {
          Job &j = J[perDev[dv][nWbAcc[dv]]];
          if (known(j.e.wbReq) && j.e.wbReq <= t) {
            ++nWbAcc[dv];
            const int64_t rows = hw.contexts[dv].rows;
            j.e.wbAcc = t;
            ++progress;
            // a request that waited has its first result beat staged
            j.e.done = t + rows * rows * hw.elementBytes / hw.beatBytes +
                       hw.wbEngineExtra - (t > j.e.wbReq ? 1 : 0);
            wbInflight = true;
            wbFreeAt = j.e.done + 1;
          }
        }
      }
    }

    const bool srcReady = !out.full();

    // ---- readback FSM -----------------------------------------------------------
    int fifoWrites = 0;
    if (hw.readback) {
      if (rbState == RbState::Idle) {
        for (int n = 1; n <= NA; ++n) {
          const int dv = (rbLast + n) % NA;
          if (nTaken[dv] >= (int64_t)perDev[dv].size())
            continue;
          Job &j = J[perDev[dv][nTaken[dv]]];
          if (known(j.e.done) && j.e.done < t) {
            j.e.rbTake = t;
            ++progress;
            rbJob = perDev[dv][nTaken[dv]];
            ++nTaken[dv];
            rbLast = dv;
            rbState = RbState::HdrNext;
            break;
          }
        }
      } else if (rbState == RbState::Hdr && srcReady) {
        ++fifoWrites;                              // the completion record
        rbState = RbState::DataNext;
      }
    }

    // ---- read engine, combinational ------------------------------------------
    int nact = 0;
    for (int s = 0; s < NS; ++s)
      nact += act[s] ? 1 : 0;
    const bool shared = nact > 1;
    auto dstFull = [&](int s) {
      if (s == RB)
        return !srcReady;
      return wBusy[s] && wCnt[s] != hw.writerCyclesPerBeat - 1;
    };
    int g = -1;
    for (int n = 0; n < NS; ++n) {
      const int s = (rr + n) % NS;
      bool e = act[s] && issueLeft[s] > 0;
      if (s == RB) {
        if (!srcReady || retLeft[s] != issueLeft[s])
          e = false;
      } else if (!shared && dstFull(s)) {
        e = false;
      }
      if (e) {
        g = s;
        break;
      }
    }
    const bool arIssue = g >= 0 && inFlight < hw.maxOutstanding;
    const bool rFire = rvValid && !dstFull(rv.slot);
    if (shared)
      ++st.sharedCycles;
    if (rvValid && !rFire)
      ++st.rStallCycles;

    // ---- updates at the edge ------------------------------------------------------
    bool newArq = false;
    Burst nb{0, 0};
    if (arIssue) {
      const int64_t lim = shared ? hw.sharedBurstLen : hw.burstLen;
      const int64_t toPage = beatsPerPage - (addrBeat[g] % beatsPerPage);
      const int64_t len = std::min({lim, issueLeft[g], toPage});
      issueLeft[g] -= len;
      addrBeat[g] += len;
      ++inFlight;
      ++progress;
      rr = (g + 1) % NS;
      newArq = true;
      nb = Burst{g, len};
      ++st.arBursts;
      if (g != RB)
        ++st.operandArBursts;
      ++st.burstLengths[len];
      st.maxOutstanding = std::max(st.maxOutstanding, inFlight);
    }
    if (arqValid)
      mig.push_back(arq);                          // MIG takes AR at once (arready = 1)
    arqValid = newArq;
    arq = nb;
    int acc = -1;
    if (rFire) {
      const int s = rv.slot;
      acc = s;
      ++progress;
      --retLeft[s];
      --rv.beats;
      if (s == RB)
        ++st.readbackBeats;
      else
        ++st.operandBeats;
      if (rv.beats == 0)
        --inFlight;
      if (retLeft[s] == 0) {
        act[s] = false;
        Job &j = J[owner[s]];
        if (s == RB) {
          j.e.rbDone = t + 1;                      // done_valid is registered
          rbState = RbState::IdleNext2;
        } else {
          j.e.lastBeat = t;
          j.e.land = t + hw.landTail;
        }
      }
    }
    if (!rvValid || rFire) {
      if (rvValid && rv.beats > 0) {
        // the burst continues
      } else if (migHead < mig.size()) {
        rv = mig[migHead++];
        rvValid = true;
        if (migHead > 1024) {
          mig.erase(mig.begin(), mig.begin() + migHead);
          migHead = 0;
        }
      } else {
        rvValid = false;
      }
    }
    for (int s = 0; s < NA; ++s) {
      if (acc == s) {
        wBusy[s] = true;
        wCnt[s] = 0;
      } else if (wBusy[s]) {
        if (wCnt[s] == hw.writerCyclesPerBeat - 1)
          wBusy[s] = false;
        else
          ++wCnt[s];
      }
    }
    if (acc == RB)
      ++fifoWrites;
    out.uiEdge(fifoWrites);

    for (auto it = descAt.lower_bound(t); it != descAt.end() && it->first == t;
         it = descAt.erase(it)) {
      const int s = it->second.first;
      const int64_t i = it->second.second;
      if (act[s]) {
        result.error = "internal: a descriptor for a busy slot";
        return result;
      }
      act[s] = true;
      owner[s] = i;
      ++progress;
      if (s == RB) {
        const int dv = J[i].in.context;
        const int64_t rows = hw.contexts[dv].rows;
        issueLeft[s] = retLeft[s] = rows * rows * hw.elementBytes / hw.beatBytes;
        addrBeat[s] = (hw.resultBase + (dv * hw.resultSlots + J[i].nth % hw.resultSlots) *
                                           hw.resultSlotBytes) /
                      hw.beatBytes;
      } else {
        issueLeft[s] = retLeft[s] = operandBeats(J[i].in.k, hw);
        addrBeat[s] = (hw.operandBase + (s * hw.operandSlots + J[i].nth % hw.operandSlots) *
                                            slab) /
                      hw.beatBytes;
      }
    }
    switch (rbState) {
    case RbState::DataNext:
      rbState = RbState::Data;
      J[rbJob].e.rbDesc = t + 1;
      descAt.emplace(t + 1, std::make_pair(RB, rbJob));
      break;
    case RbState::HdrNext:
      rbState = RbState::Hdr;
      break;
    case RbState::IdleNext:
      rbState = RbState::Idle;
      break;
    case RbState::IdleNext2:
      rbState = RbState::IdleNext;
      break;
    default:
      break;
    }

    ++t;

    // ---- finished? -------------------------------------------------------------
    if (fired == nj && !known(firePendingJob)) {
      // every job's completion (readback, or result in DDR) before the new
      // t: the per-context cursors, advanced to t (completions are in job
      // order per context)
      bool all = true;
      for (int dv = 0; dv < NA && all; ++dv) {
        const auto &lst = perDev[dv];
        int64_t &c = hw.readback ? nRbDone[dv] : nDone[dv];
        while (c < (int64_t)lst.size()) {
          const int64_t v = hw.readback ? J[lst[c]].e.rbDone : J[lst[c]].e.done;
          if (!known(v) || v >= t)
            break;
          ++c;
        }
        all = c == (int64_t)lst.size();
      }
      if (all)
        break;
    }

    // ---- progress, deadlock, and skipping cycles where nothing can happen -------
    progress += fifoWrites + (out.bytesOut - bytesOutSeen);
    bytesOutSeen = out.bytesOut;
    bool idle = !arqValid && !rvValid && migHead >= mig.size() && !known(firePendingJob) &&
                out.quiescent() && (rbState == RbState::Idle || !hw.readback) &&
                !wbInflight;
    for (int s = 0; s < NS && idle; ++s)
      if (act[s] || wBusy[s])
        idle = false;
    if (progress != lastProgressCount) {
      lastProgressCount = progress;
      lastProgressCycle = t;
    } else if (!idle && t - lastProgressCycle > kDeadlockCycles) {
      result.error = "deadlock: nothing moved for " +
                     std::to_string(kDeadlockCycles) + " cycles at cycle " +
                     std::to_string(t) + " with work in flight";
      return result;
    }
    if (idle) {
      int64_t next = std::numeric_limits<int64_t>::max();
      if (!descAt.empty())
        next = std::min(next, descAt.begin()->first);
      for (int dv = 0; dv < NA; ++dv)
        for (int64_t n = std::max<int64_t>(0, lo[dv] - 2); n < (int64_t)perDev[dv].size(); ++n) {
          const Job &j = J[perDev[dv][n]];
          for (int64_t v : {j.e.wbReq, j.e.land, j.e.fold, j.e.cv})
            if (known(v) && v >= t)
              next = std::min(next, v);
          // a completion is acted on the cycle after it (readback take,
          // the port's room)
          for (int64_t v : {j.e.done, j.e.rbDone})
            if (known(v) && v + 1 >= t)
              next = std::min(next, v + 1);
          if (!known(j.e.fire))
            break;
        }
      // the port: the next offer (it may depend on a done already counted)
      if (options.port == PortPolicy::InOrder) {
        if (nxt < nj) {
          const int64_t off = offerOf(nxt);
          if (known(off))
            next = std::min(next, off);
        }
      } else {
        for (int dv = 0; dv < NA; ++dv)
          if (nextUnfired[dv] < (int64_t)perDev[dv].size()) {
            const int64_t off = offerOf(perDev[dv][nextUnfired[dv]]);
            if (known(off))
              next = std::min(next, off);
          }
      }
      if (next == std::numeric_limits<int64_t>::max() &&
          (progress == lastProgressCount && t - lastProgressCycle > 2)) {
        // nothing is in flight, nothing is scheduled, jobs remain
        result.error = "deadlock: at cycle " + std::to_string(t) +
                       " no event can ever happen and " +
                       std::to_string(nj - fired) + " job(s) were never accepted";
        return result;
      }
      if (options.skipIdle && next != std::numeric_limits<int64_t>::max() &&
          next > t + 1) {
        // done / rbDone are counted the cycle after they happen
        t = next - 1 > t ? next - 1 : t;
      }
    }
  }

  st.simulatedCycles = t;
  st.operandReadBytes = st.operandBeats * hw.beatBytes;
  st.readbackBytes = st.readbackBeats * hw.beatBytes + 0;
  result.jobs.reserve(J.size());
  int64_t first = std::numeric_limits<int64_t>::max(), last = 0, rbEnd = 0;
  result.contextMakespan.assign(NA, 0);
  std::vector<int64_t> cFirst(NA, std::numeric_limits<int64_t>::max()), cLast(NA, 0);
  int64_t records = 0;
  for (const Job &j : J) {
    result.jobs.push_back(j.e);
    first = std::min(first, j.e.fire);
    last = std::max(last, j.e.done);
    rbEnd = std::max(rbEnd, j.e.rbDone);
    const int dv = j.in.context;
    cFirst[dv] = std::min(cFirst[dv], j.e.fire);
    cLast[dv] = std::max(cLast[dv], j.e.done);
    st.contentionCycles += j.e.readCycles() - aloneReadCycles(j.in.k, hw);
    ++records;
  }
  if (hw.readback)
    st.readbackBytes += records * hw.beatBytes;
  for (int dv = 0; dv < NA; ++dv)
    if (!perDev[dv].empty())
      result.contextMakespan[dv] = cLast[dv] - cFirst[dv];
  if (nj > 0) {
    result.deviceMakespan = last - first;
    result.readbackEnd = hw.readback ? rbEnd - first : 0;
    result.lastByteToHost =
        hw.readback ? (out.lastBytePs + hw.uiPeriodPs - 1) / hw.uiPeriodPs - first : 0;
  }
  result.ok = true;
  return result;
}

//===----------------------------------------------------------------------===//
// Saturated streams
//===----------------------------------------------------------------------===//

SaturatedStreams saturatedStreams(const FleetHardware &hw, int64_t reduction,
                                  const std::vector<int64_t> &rows,
                                  const std::vector<int64_t> &tilesPerGeometry) {
  SaturatedStreams s;
  std::string why;
  if (!isValidFleetHardware(hw, &why)) {
    s.error = why;
    return s;
  }
  if (reduction < 1 || rows.size() != tilesPerGeometry.size()) {
    s.error = "bad saturated-stream request";
    return s;
  }
  const int NA = static_cast<int>(hw.contexts.size());
  s.geometries = rows;
  s.tileStart.assign(rows.size(), {});
  s.tileEnd.assign(rows.size(), {});

  // Every context runs tiles of its own geometry: the invocations of a tile
  // are its K folds, depth min(kMax, K - i * kMax). wantTiles[dv]: the tiles
  // of context dv the streams need measured.
  std::vector<int64_t> invPerTile(NA), wantTiles(NA, 0);
  for (int dv = 0; dv < NA; ++dv) {
    const ContextConfig &c = hw.contexts[dv];
    invPerTile[dv] = ceilDiv(reduction, c.kMax);
    bool found = false;
    for (size_t g = 0; g < rows.size(); ++g)
      if (rows[g] == c.rows) {
        wantTiles[dv] = std::max(wantTiles[dv], tilesPerGeometry[g]);
        found = true;
      }
    if (!found) {
      s.error = "a context whose geometry has no stream";
      return s;
    }
    if (wantTiles[dv] < 1) {
      s.error = "a stream of no tiles";
      return s;
    }
  }

  // Saturated: while any measured tile is still running, every context has
  // work. Run with each context's measured tiles plus one, and lengthen
  // (double) the stream of every context that ran dry before the last
  // measured tile ended, until none does. More work on one context can only
  // delay the others, so the measured horizon only grows and the loop ends
  // once every stream outlasts it.
  std::vector<int64_t> runTiles(NA);
  for (int dv = 0; dv < NA; ++dv)
    runTiles[dv] = wantTiles[dv] + 1;
  FleetResult r;
  std::vector<std::vector<int64_t>> firstJobOfTile, lastJobOfTile;
  while (true) {
    ++s.simulationRuns;
    std::vector<FleetJob> jobs;
    firstJobOfTile.assign(NA, {});
    lastJobOfTile.assign(NA, {});
    int64_t maxRun = 0;
    for (int dv = 0; dv < NA; ++dv)
      maxRun = std::max(maxRun, runTiles[dv]);
    // offer order: tile by tile, the contexts interleaved (the runtime
    // submits tiles in schedule order); Credit takes them as contexts free up
    for (int64_t tile = 0; tile < maxRun; ++tile)
      for (int dv = 0; dv < NA; ++dv) {
        if (tile >= runTiles[dv])
          continue;
        const ContextConfig &c = hw.contexts[dv];
        firstJobOfTile[dv].push_back(static_cast<int64_t>(jobs.size()));
        for (int64_t i = 0; i < invPerTile[dv]; ++i) {
          FleetJob j;
          j.context = dv;
          j.k = std::min(c.kMax, reduction - i * c.kMax);
          j.present = 0;
          jobs.push_back(j);
        }
        lastJobOfTile[dv].push_back(static_cast<int64_t>(jobs.size()) - 1);
      }
    FleetOptions opt;
    opt.port = PortPolicy::Credit;
    r = simulateFleet(hw, jobs, opt);
    if (!r.ok) {
      s.error = r.error;
      return s;
    }
    int64_t horizon = 0;
    for (int dv = 0; dv < NA; ++dv)
      horizon = std::max(horizon,
                         r.jobs[lastJobOfTile[dv][wantTiles[dv] - 1]].done);
    bool dry = false;
    for (int dv = 0; dv < NA; ++dv)
      if (r.jobs[lastJobOfTile[dv].back()].done < horizon) {
        dry = true;
        if (runTiles[dv] > std::numeric_limits<int64_t>::max() / 4) {
          s.error = "saturated stream too long";
          return s;
        }
        runTiles[dv] *= 2;
      }
    if (!dry)
      break;
  }

  // Per context: tile starts (first invocation's fill start) and ends (last
  // invocation's done), from the first acceptance.
  int64_t origin = std::numeric_limits<int64_t>::max();
  for (const FleetJobEvents &e : r.jobs)
    origin = std::min(origin, e.fire);
  for (size_t g = 0; g < rows.size(); ++g) {
    const int64_t want = tilesPerGeometry[g];
    std::vector<int64_t> &ts = s.tileStart[g];
    std::vector<int64_t> &te = s.tileEnd[g];
    ts.assign(static_cast<size_t>(want), 0);
    te.assign(static_cast<size_t>(want), 0);
    for (int dv = 0; dv < NA; ++dv) {
      if (hw.contexts[dv].rows != rows[g])
        continue;
      for (int64_t tile = 0; tile < want; ++tile) {
        ts[tile] = std::max(ts[tile], r.jobs[firstJobOfTile[dv][tile]].fillStart - origin);
        te[tile] = std::max(te[tile], r.jobs[lastJobOfTile[dv][tile]].done - origin);
      }
    }
    // Every context's tiles end strictly later one after the other and start
    // no earlier, so the pointwise maximum does too; verified, not assumed.
    for (int64_t tile = 1; tile < want; ++tile)
      if (te[tile] <= te[tile - 1] || ts[tile] < ts[tile - 1]) {
        s.error = "internal: a saturated stream is not increasing";
        return s;
      }
  }
  s.ok = true;
  return s;
}

} // namespace v2
} // namespace systolic
