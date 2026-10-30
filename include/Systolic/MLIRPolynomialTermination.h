#pragma once

#include "Systolic/MLIRPolynomialRecurrence.h"

namespace systolic::polynomial::termination {

// Independent of polynomial::Status. Claims quantify over ALL defined inputs
// in the reported nonempty domain. No trip count, exit value, or requested
// header-visit reachability is established by this API.
enum class Status { ProvenTerminating, ProvenNonterminating, Unknown };
enum class CertificateKind { LinearRanking, ZeroIterations, RecurrentGuard };

struct Limits {
  unsigned maxBlocks = 64, maxOperations = 512, maxStates = 16;
  unsigned maxParameters = 16, maxNodes = 4096, maxDepth = 128;
  unsigned maxCandidates = 4, maxQueries = 16;
  unsigned timeoutMilliseconds = 2000, solverResourceLimit = 5000000;
  unsigned maxSMTBytes = 1048576;
};
struct Options {
  // Signed representatives of source bit patterns, just as in P2a. Guard
  // signedness is independent. Defaults include every bit pattern.
  std::map<unsigned, std::pair<Integer, Integer>> parameterRanges;
  Limits limits;
};
struct Parameter {
  unsigned argument = 0, bitWidth = 0;
  Integer lower = 0, upper = 0;
  bool operator==(const Parameter &p) const {
    return argument == p.argument && bitWidth == p.bitWidth &&
           lower == p.lower && upper == p.upper;
  }
};
struct StateInterval {
  unsigned state = 0;
  Integer lower = 0, upper = 0; // signed source-width representatives
};
struct Certificate {
  CertificateKind kind = CertificateKind::LinearRanking;
  std::string sourceFingerprint, guard;
  std::vector<Parameter> domain;
  // Generation uses the full typed state domain. Supplied interval invariants
  // are never assumed: verification checks initiation and preservation.
  std::vector<StateInterval> invariant;
  unsigned counter = 0;
  bool ascending = true, signedValues = true;
  int offset = 0; // restricted to -1, 0, 1; rank uses width+2 signed arithmetic
};
struct Obligation {
  std::string name, smtlib, expected, outcome, detail;
};
struct Result {
  Status status = Status::Unknown;
  std::string reason, sourceFingerprint, solverVersion;
  Options options;
  std::vector<Parameter> domain;
  std::optional<Certificate> certificate; // absent on every UNKNOWN
  std::vector<Obligation> obligations; // attempts, not certificates by themselves
  // Stable descriptions of E, G, T. I and rho are in the certificate.
  unsigned bitWidth = 0;
  std::vector<std::string> initial, transition;
  std::string guard;
  bool definedTotalPathVerified = false;
};

// Read-only, restricted P2a CFGs; actual unflagged source bitvectors only.
// No P2a assumption or finite horizon is imported as a termination premise.
std::vector<Certificate> generateCandidates(mlir::func::FuncOp function,
                                          const Options &options = {});
// Re-extracts the input independently of generation. Never trusts a status,
// guard, parameter domain, or invariant supplied in a certificate.
Result verifyCertificate(mlir::func::FuncOp function, const Certificate &certificate,
                         const Options &options = {});
Result analyze(mlir::func::FuncOp function, const Options &options = {});
std::string formatResult(const Result &result);

} // namespace systolic::polynomial::termination
