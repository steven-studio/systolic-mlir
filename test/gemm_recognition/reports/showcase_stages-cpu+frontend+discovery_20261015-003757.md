# GEMM recognition showcase: actual results

- **partial run**: only the stages cpu, frontend, discovery were executed; 'partial_pass' means those stages behaved as predicted, nothing is claimed about the others
- mode: **stages-cpu+frontend+discovery** (plain C compiler, no systolic-cc)
- timestamp: 20261015-003757, host: vm (Linux-6.18.44-fc-v114-x86_64-with-glibc2.39), label: container stop-gap: recognizer of 6e25528 built against LLVM 18 (discovery-opt), cfront with clang 18
- compiler: commit 6e25528, clang: Ubuntu clang version 18.1.3 (1ubuntu1), build: /tmp/claude-0/-home-claude/543a1223-6aa4-5ccc-919f-04c239e419e6/scratchpad/stopbin, LLVM: /usr/lib/llvm-18
- cases: 114; verdicts: known_false_negative=3, partial_pass=111

| id | set | spec | impl. predicted | actual driver | recognition | dense | lowering | planned inv. | runtime inv. | program | verdict | notes |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| case_001 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_002 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_003 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_004 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_005 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_006 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_007 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_008 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_009 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_010 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_011 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_012 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_013 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_014 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_015 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_016 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_017 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_018 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_019 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_020 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_021 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_022 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_023 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_024 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_025 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_026 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_027 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_028 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_029 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_030 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_031 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_032 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_033 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_034 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_035 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_036 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_037 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_038 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_039 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_040 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_041 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_042 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_043 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_044 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_045 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_046 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_047 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_048 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_049 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_050 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_051 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_052 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_053 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_054 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_055 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_056 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_057 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_058 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_059 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_060 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_061 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_062 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_063 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_064 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_065 | positive | ACCEPT | stays_on_host | None | None | None | None |  |  | None | **known_false_negative** | specification: ACCEPT; discovery: reject (ISSUE-001) |
| case_066 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_067 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_068 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_069 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_070 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_071 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_072 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_073 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_074 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_075 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_076 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_077 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_078 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_079 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_080 | positive | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| case_081 | negative | REJECT | stays_on_host | None | None | None | None |  |  | None | **partial_pass** |  |
| case_082 | negative | REJECT | stays_on_host | None | None | None | None |  |  | None | **partial_pass** |  |
| case_083 | negative | OUT_OF_SCOPE | stays_on_host | None | None | None | None |  |  | None | **partial_pass** |  |
| case_084 | negative | OUT_OF_SCOPE | stays_on_host | None | None | None | None |  |  | None | **partial_pass** |  |
| case_085 | negative | OUT_OF_SCOPE | stays_on_host | None | None | None | None |  |  | None | **partial_pass** |  |
| case_086 | negative | OUT_OF_SCOPE | stays_on_host | None | None | None | None |  |  | None | **partial_pass** |  |
| case_087 | negative | REJECT | stays_on_host | None | None | None | None |  |  | None | **partial_pass** |  |
| case_088 | negative | REJECT | stays_on_host | None | None | None | None |  |  | None | **partial_pass** |  |
| case_089 | negative | REJECT | stays_on_host | None | None | None | None |  |  | None | **partial_pass** |  |
| case_090 | negative | REJECT | stays_on_host | None | None | None | None |  |  | None | **partial_pass** |  |
| case_091 | negative | OUT_OF_SCOPE | stays_on_host | None | None | None | None |  |  | None | **partial_pass** |  |
| case_092 | negative | OUT_OF_SCOPE | stays_on_host | None | None | None | None |  |  | None | **partial_pass** |  |
| case_093 | negative | OUT_OF_SCOPE | stays_on_host | None | None | None | None |  |  | None | **partial_pass** |  |
| case_094 | negative | ACCEPT | stays_on_host | None | None | None | None |  |  | None | **known_false_negative** | specification: ACCEPT; discovery: reject (ISSUE-001) |
| case_095 | negative | REJECT | stays_on_host | None | None | None | None |  |  | None | **partial_pass** |  |
| case_096 | negative | OUT_OF_SCOPE | stays_on_host | None | None | None | None |  |  | None | **partial_pass** |  |
| case_097 | negative | REJECT | stays_on_host | None | None | None | None |  |  | None | **partial_pass** |  |
| case_098 | negative | OUT_OF_SCOPE | stays_on_host | None | None | None | None |  |  | None | **partial_pass** |  |
| case_099 | negative | OUT_OF_SCOPE | stays_on_host | None | None | None | None |  |  | None | **partial_pass** |  |
| case_100 | negative | OUT_OF_SCOPE | stays_on_host | None | None | None | None |  |  | None | **partial_pass** |  |
| ext_001 | extended | OUT_OF_SCOPE | not_translated | None | None | None | None |  |  | None | **partial_pass** |  |
| ext_002 | extended | OUT_OF_SCOPE | stays_on_host | None | None | None | None |  |  | None | **partial_pass** |  |
| ext_003 | extended | OUT_OF_SCOPE | stays_on_host | None | None | None | None |  |  | None | **partial_pass** |  |
| ext_004 | extended | OUT_OF_SCOPE | stays_on_host | None | None | None | None |  |  | None | **partial_pass** |  |
| ext_005 | extended | REJECT | stays_on_host | None | None | None | None |  |  | None | **partial_pass** |  |
| ext_006 | extended | REJECT | stays_on_host | None | None | None | None |  |  | None | **partial_pass** |  |
| ext_007 | extended | OUT_OF_SCOPE | stays_on_host | None | None | None | None |  |  | None | **partial_pass** |  |
| ext_008 | extended | ACCEPT | stays_on_host | None | None | None | None |  |  | None | **known_false_negative** | specification: ACCEPT; discovery: reject (ISSUE-001) |
| ext_009 | extended | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| ext_010 | extended | OUT_OF_SCOPE | stays_on_host | None | None | None | None |  |  | None | **partial_pass** |  |
| ext_011 | extended | OUT_OF_SCOPE | stays_on_host | None | None | None | None |  |  | None | **partial_pass** |  |
| ext_012 | extended | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| ext_013 | extended | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |
| ext_014 | extended | ACCEPT | offloaded | None | None | None | None |  |  | None | **partial_pass** |  |

## Known false negatives reproduced (open issues)

- case_065 (subblock_c_larger): specification: ACCEPT; discovery: reject (ISSUE-001)
- case_094 (operand_specific_stride): specification: ACCEPT; discovery: reject (ISSUE-001)
- ext_008 (operand_specific_offset): specification: ACCEPT; discovery: reject (ISSUE-001)

