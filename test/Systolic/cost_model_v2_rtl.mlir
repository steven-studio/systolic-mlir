// Cost Model V2 fleet simulator against the RTL (docs/COSTMODEL_V2.md).
//
// Every number checked below was MEASURED in RTL simulation by the bench
// eval/cost_model_v2/tb_fleet_jobs.sv (Verilator, the bitstream RTL of
// rtl/multi_200t/tile_pipelined) on the same job file, not computed by the
// model: job acceptance, read descriptor, last operand beat, fill landed,
// fold, result published, write-back grant, result in DDR, readback take and
// readback done, and the number of AXI read requests. --dpti-phase-ps is the
// bench run's measured clock phase between the array clock and the DPTI
// clock, --dpti-period-ps the bench's DPTI period (2 x 8.333 ns).
// Regenerate: eval/cost_model_v2/README.md.

// 16/16 K_MAX, v1 operand path: combo_3x4x4_K16_gap0
// RUN: systolic-cost fleet-sim --jobs %S/Inputs/cost_model_v2/case1_3x4x4_k16.jobs --k8 16 --k4 16 --dpti-phase-ps 3245 --dpti-period-ps 16666 | FileCheck %s --check-prefix=CASE1_3X4X4_K16
// CASE1_3X4X4_K16: job 0 dev=1 k=16 offer={{[0-9-]+}} fire=2 fill={{[0-9]+}} desc=5 last=260 land=265 fold=269 cv=386 wbreq={{[0-9]+}} wbacc=407 done=417 take=418 rbdesc={{[0-9]+}} rbdone=427
// CASE1_3X4X4_K16: job 1 dev=2 k=16 offer={{[0-9-]+}} fire=4 fill={{[0-9]+}} desc=7 last=385 land=390 fold=394 cv=511 wbreq={{[0-9]+}} wbacc=532 done=542 take=543 rbdesc={{[0-9]+}} rbdone=552
// CASE1_3X4X4_K16: job 2 dev=3 k=16 offer={{[0-9-]+}} fire=6 fill={{[0-9]+}} desc=9 last=390 land=395 fold=399 cv=516 wbreq={{[0-9]+}} wbacc=543 done=552 take=553 rbdesc={{[0-9]+}} rbdone=740
// CASE1_3X4X4_K16: fleet device_makespan=550 {{.*}} ar=165 

// 16/16 K_MAX, v1 operand path: combo_8x8+3x4x4_K17_gap3
// RUN: systolic-cost fleet-sim --jobs %S/Inputs/cost_model_v2/fleet_k17_gap3.jobs --k8 16 --k4 16 --dpti-phase-ps 3245 --dpti-period-ps 16666 | FileCheck %s --check-prefix=FLEET_K17_GAP3
// FLEET_K17_GAP3: job 0 dev=0 k=16 offer={{[0-9-]+}} fire=2 fill={{[0-9]+}} desc=5 last=260 land=265 fold=269 cv=394 wbreq={{[0-9]+}} wbacc=463 done=485 take=486 rbdesc={{[0-9]+}} rbdone=915
// FLEET_K17_GAP3: job 1 dev=1 k=16 offer={{[0-9-]+}} fire=4 fill={{[0-9]+}} desc=7 last=385 land=390 fold=394 cv=511 wbreq={{[0-9]+}} wbacc=532 done=542 take=916 rbdesc={{[0-9]+}} rbdone=1182
// FLEET_K17_GAP3: job 2 dev=2 k=16 offer={{[0-9-]+}} fire=7 fill={{[0-9]+}} desc=10 last=390 land=395 fold=399 cv=516 wbreq={{[0-9]+}} wbacc=543 done=552 take=1183 rbdesc={{[0-9]+}} rbdone=1449
// FLEET_K17_GAP3: job 3 dev=3 k=16 offer={{[0-9-]+}} fire=10 fill={{[0-9]+}} desc=13 last=395 land=400 fold=404 cv=521 wbreq={{[0-9]+}} wbacc=553 done=562 take=1450 rbdesc={{[0-9]+}} rbdone=1715
// FLEET_K17_GAP3: job 4 dev=0 k=1 offer={{[0-9-]+}} fire=13 fill={{[0-9]+}} desc=272 last=408 land=413 fold=417 cv=527 wbreq={{[0-9]+}} wbacc=596 done=618 take=1716 rbdesc={{[0-9]+}} rbdone=2622
// FLEET_K17_GAP3: job 5 dev=1 k=1 offer={{[0-9-]+}} fire=16 fill={{[0-9]+}} desc=397 last=533 land=538 fold=542 cv=644 wbreq={{[0-9]+}} wbacc=665 done=675 take=2623 rbdesc={{[0-9]+}} rbdone=2889
// FLEET_K17_GAP3: job 6 dev=2 k=1 offer={{[0-9-]+}} fire=19 fill={{[0-9]+}} desc=402 last=550 land=555 fold=559 cv=661 wbreq={{[0-9]+}} wbacc=682 done=692 take=2890 rbdesc={{[0-9]+}} rbdone=3155
// FLEET_K17_GAP3: job 7 dev=3 k=1 offer={{[0-9-]+}} fire=22 fill={{[0-9]+}} desc=407 last=555 land=560 fold=564 cv=666 wbreq={{[0-9]+}} wbacc=693 done=702 take=3156 rbdesc={{[0-9]+}} rbdone=3422
// FLEET_K17_GAP3: fleet device_makespan=700 {{.*}} ar=368 

// 32/32 K_MAX, v1 operand path: combo_8x8+3x4x4_K65_gap0
// RUN: systolic-cost fleet-sim --jobs %S/Inputs/cost_model_v2/fleet_k65_kmax32.jobs --k8 32 --k4 32 --dpti-phase-ps 3245 --dpti-period-ps 16666 | FileCheck %s --check-prefix=FLEET_K65_KMAX32
// FLEET_K65_KMAX32: job 0 dev=0 k=32 offer={{[0-9-]+}} fire=2 fill={{[0-9]+}} desc=5 last=516 land=521 fold=525 cv=666 wbreq={{[0-9]+}} wbacc=735 done=757 take=758 rbdesc={{[0-9]+}} rbdone=1196
// FLEET_K65_KMAX32: job 1 dev=1 k=32 offer={{[0-9-]+}} fire=4 fill={{[0-9]+}} desc=7 last=641 land=646 fold=650 cv=783 wbreq={{[0-9]+}} wbacc=804 done=814 take=1197 rbdesc={{[0-9]+}} rbdone=1464
// FLEET_K65_KMAX32: job 2 dev=2 k=32 offer={{[0-9-]+}} fire=6 fill={{[0-9]+}} desc=9 last=646 land=651 fold=655 cv=788 wbreq={{[0-9]+}} wbacc=815 done=824 take=1465 rbdesc={{[0-9]+}} rbdone=1720
// FLEET_K65_KMAX32: job 3 dev=3 k=32 offer={{[0-9-]+}} fire=8 fill={{[0-9]+}} desc=11 last=647 land=652 fold=656 cv=789 wbreq={{[0-9]+}} wbacc=825 done=834 take=1721 rbdesc={{[0-9]+}} rbdone=1987
// FLEET_K65_KMAX32: job 4 dev=0 k=32 offer={{[0-9-]+}} fire=10 fill={{[0-9]+}} desc=528 last=1061 land=1066 fold=1070 cv=1211 wbreq={{[0-9]+}} wbacc=1280 done=1302 take=1988 rbdesc={{[0-9]+}} rbdone=2894
// FLEET_K65_KMAX32: job 5 dev=1 k=32 offer={{[0-9-]+}} fire=12 fill={{[0-9]+}} desc=653 last=1325 land=1330 fold=1334 cv=1467 wbreq={{[0-9]+}} wbacc=1488 done=1498 take=2895 rbdesc={{[0-9]+}} rbdone=3160
// FLEET_K65_KMAX32: job 6 dev=2 k=32 offer={{[0-9-]+}} fire=14 fill={{[0-9]+}} desc=658 last=1330 land=1335 fold=1339 cv=1472 wbreq={{[0-9]+}} wbacc=1499 done=1508 take=3161 rbdesc={{[0-9]+}} rbdone=3427
// FLEET_K65_KMAX32: job 7 dev=3 k=32 offer={{[0-9-]+}} fire=16 fill={{[0-9]+}} desc=659 last=1331 land=1336 fold=1340 cv=1473 wbreq={{[0-9]+}} wbacc=1509 done=1518 take=3428 rbdesc={{[0-9]+}} rbdone=3694
// FLEET_K65_KMAX32: job 8 dev=0 k=1 offer={{[0-9-]+}} fire=18 fill={{[0-9]+}} desc=1073 last=1212 land=1217 fold=1221 cv=1331 wbreq={{[0-9]+}} wbacc=1400 done=1422 take=3695 rbdesc={{[0-9]+}} rbdone=4600
// FLEET_K65_KMAX32: job 9 dev=1 k=1 offer={{[0-9-]+}} fire=20 fill={{[0-9]+}} desc=1337 last=1464 land=1469 fold=1473 cv=1575 wbreq={{[0-9]+}} wbacc=1596 done=1606 take=4601 rbdesc={{[0-9]+}} rbdone=4867
// FLEET_K65_KMAX32: job 10 dev=2 k=1 offer={{[0-9-]+}} fire=22 fill={{[0-9]+}} desc=1342 last=1481 land=1486 fold=1490 cv=1592 wbreq={{[0-9]+}} wbacc=1613 done=1623 take=4868 rbdesc={{[0-9]+}} rbdone=5134
// FLEET_K65_KMAX32: job 11 dev=3 k=1 offer={{[0-9-]+}} fire=24 fill={{[0-9]+}} desc=1343 last=1482 land=1487 fold=1491 cv=1593 wbreq={{[0-9]+}} wbacc=1624 done=1633 take=5135 rbdesc={{[0-9]+}} rbdone=5400
// FLEET_K65_KMAX32: fleet device_makespan=1631 {{.*}} ar=1122 

// 32/32 K_MAX, v1 operand path: serial_8x8+3x4x4_K17_gap40
// RUN: systolic-cost fleet-sim --jobs %S/Inputs/cost_model_v2/serial_k17_gap40.jobs --k8 32 --k4 32 --dpti-phase-ps 3245 --dpti-period-ps 16666 | FileCheck %s --check-prefix=SERIAL_K17_GAP40
// SERIAL_K17_GAP40: job 0 dev=0 k=17 offer={{[0-9-]+}} fire=2 fill={{[0-9]+}} desc=5 last=388 land=393 fold=397 cv=523 wbreq={{[0-9]+}} wbacc=592 done=614 take=615 rbdesc={{[0-9]+}} rbdone=1044
// SERIAL_K17_GAP40: job 1 dev=1 k=17 offer={{[0-9-]+}} fire=4 fill={{[0-9]+}} desc=7 last=513 land=518 fold=522 cv=640 wbreq={{[0-9]+}} wbacc=661 done=671 take=1045 rbdesc={{[0-9]+}} rbdone=1320
// SERIAL_K17_GAP40: job 2 dev=2 k=17 offer={{[0-9-]+}} fire=6 fill={{[0-9]+}} desc=9 last=518 land=523 fold=527 cv=645 wbreq={{[0-9]+}} wbacc=672 done=681 take=1321 rbdesc={{[0-9]+}} rbdone=1577
// SERIAL_K17_GAP40: job 3 dev=3 k=17 offer={{[0-9-]+}} fire=8 fill={{[0-9]+}} desc=11 last=519 land=524 fold=528 cv=646 wbreq={{[0-9]+}} wbacc=682 done=691 take=1578 rbdesc={{[0-9]+}} rbdone=1844
// SERIAL_K17_GAP40: job 4 dev=0 k=17 offer={{[0-9-]+}} fire=655 fill={{[0-9]+}} desc=658 last=1429 land=1434 fold=1438 cv=1564 wbreq={{[0-9]+}} wbacc=1633 done=1655 take=1845 rbdesc={{[0-9]+}} rbdone=2750
// SERIAL_K17_GAP40: job 5 dev=1 k=17 offer={{[0-9-]+}} fire=712 fill={{[0-9]+}} desc=715 last=1454 land=1459 fold=1463 cv=1581 wbreq={{[0-9]+}} wbacc=1602 done=1612 take=2751 rbdesc={{[0-9]+}} rbdone=3017
// SERIAL_K17_GAP40: job 6 dev=2 k=17 offer={{[0-9-]+}} fire=722 fill={{[0-9]+}} desc=725 last=1455 land=1460 fold=1464 cv=1582 wbreq={{[0-9]+}} wbacc=1613 done=1622 take=3018 rbdesc={{[0-9]+}} rbdone=3284
// SERIAL_K17_GAP40: job 7 dev=3 k=17 offer={{[0-9-]+}} fire=732 fill={{[0-9]+}} desc=735 last=1456 land=1461 fold=1465 cv=1583 wbreq={{[0-9]+}} wbacc=1623 done=1632 take=3285 rbdesc={{[0-9]+}} rbdone=3550
// SERIAL_K17_GAP40: job 8 dev=0 k=17 offer={{[0-9-]+}} fire=1696 fill={{[0-9]+}} desc=1699 last=2230 land=2235 fold=2239 cv=2365 wbreq={{[0-9]+}} wbacc=2434 done=2456 take=3551 rbdesc={{[0-9]+}} rbdone=4457
// SERIAL_K17_GAP40: job 9 dev=1 k=17 offer={{[0-9-]+}} fire=1698 fill={{[0-9]+}} desc=1701 last=2235 land=2240 fold=2244 cv=2362 wbreq={{[0-9]+}} wbacc=2383 done=2393 take=4458 rbdesc={{[0-9]+}} rbdone=4724
// SERIAL_K17_GAP40: job 10 dev=2 k=17 offer={{[0-9-]+}} fire=1700 fill={{[0-9]+}} desc=1703 last=2240 land=2245 fold=2249 cv=2367 wbreq={{[0-9]+}} wbacc=2394 done=2403 take=4725 rbdesc={{[0-9]+}} rbdone=4990
// SERIAL_K17_GAP40: job 11 dev=3 k=17 offer={{[0-9-]+}} fire=1702 fill={{[0-9]+}} desc=1705 last=2241 land=2246 fold=2250 cv=2368 wbreq={{[0-9]+}} wbacc=2404 done=2413 take=4991 rbdesc={{[0-9]+}} rbdone=5257
// SERIAL_K17_GAP40: fleet device_makespan=2454 {{.*}} ar=1144 

// 16/16 K_MAX, v2 operand path: combo_8x8+3x4x4_K17_gap3
// RUN: systolic-cost fleet-sim --jobs %S/Inputs/cost_model_v2/fleet_k17_gap3.jobs --k8 16 --k4 16 --v2-operand-path --dpti-phase-ps 3245 --dpti-period-ps 16666 | FileCheck %s --check-prefix=FLEET_K17_GAP3_V2PATH
// FLEET_K17_GAP3_V2PATH: job 0 dev=0 k=16 offer={{[0-9-]+}} fire=2 fill={{[0-9]+}} desc=5 last=119 land=121 fold=125 cv=250 wbreq={{[0-9]+}} wbacc=319 done=341 take=342 rbdesc={{[0-9]+}} rbdone=770
// FLEET_K17_GAP3_V2PATH: job 1 dev=1 k=16 offer={{[0-9-]+}} fire=4 fill={{[0-9]+}} desc=7 last=183 land=185 fold=189 cv=306 wbreq={{[0-9]+}} wbacc=342 done=351 take=771 rbdesc={{[0-9]+}} rbdone=1037
// FLEET_K17_GAP3_V2PATH: job 2 dev=2 k=16 offer={{[0-9-]+}} fire=7 fill={{[0-9]+}} desc=10 last=231 land=233 fold=237 cv=354 wbreq={{[0-9]+}} wbacc=375 done=385 take=1038 rbdesc={{[0-9]+}} rbdone=1304
// FLEET_K17_GAP3_V2PATH: job 3 dev=3 k=16 offer={{[0-9-]+}} fire=10 fill={{[0-9]+}} desc=13 last=279 land=281 fold=285 cv=402 wbreq={{[0-9]+}} wbacc=423 done=433 take=1305 rbdesc={{[0-9]+}} rbdone=1570
// FLEET_K17_GAP3_V2PATH: job 4 dev=0 k=1 offer={{[0-9-]+}} fire=13 fill={{[0-9]+}} desc=128 last=295 land=297 fold=301 cv=411 wbreq={{[0-9]+}} wbacc=480 done=502 take=1571 rbdesc={{[0-9]+}} rbdone=2477
// FLEET_K17_GAP3_V2PATH: job 5 dev=1 k=1 offer={{[0-9-]+}} fire=16 fill={{[0-9]+}} desc=192 last=327 land=329 fold=333 cv=435 wbreq={{[0-9]+}} wbacc=456 done=466 take=2478 rbdesc={{[0-9]+}} rbdone=2744
// FLEET_K17_GAP3_V2PATH: job 6 dev=2 k=1 offer={{[0-9-]+}} fire=19 fill={{[0-9]+}} desc=240 last=359 land=361 fold=365 cv=467 wbreq={{[0-9]+}} wbacc=503 done=512 take=2745 rbdesc={{[0-9]+}} rbdone=3010
// FLEET_K17_GAP3_V2PATH: job 7 dev=3 k=1 offer={{[0-9-]+}} fire=22 fill={{[0-9]+}} desc=288 last=391 land=393 fold=397 cv=499 wbreq={{[0-9]+}} wbacc=520 done=530 take=3011 rbdesc={{[0-9]+}} rbdone=3277
// FLEET_K17_GAP3_V2PATH: fleet device_makespan=528 {{.*}} ar=32 
