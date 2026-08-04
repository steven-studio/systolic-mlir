set moduleName matmul_4x4x4_Pipeline_VITIS_LOOP_10_38
set isTopModule 0
set isCombinational 0
set isDatapathOnly 0
set isPipelined 1
set isPipelined_legacy 1
set pipeline_type loop_auto_rewind
set FunctionProtocol ap_ctrl_hs
set restart_counter_num 0
set isOneStateSeq 0
set ProfileFlag 0
set StallSigGenFlag 0
set isEnableWaveformDebug 1
set hasInterrupt 0
set DLRegFirstOffset 0
set DLRegItemOffset 0
set svuvm_can_support 1
set cdfgNum 19
set C_modelName {matmul_4x4x4_Pipeline_VITIS_LOOP_10_38}
set C_modelType { void 0 }
set ap_memory_interface_dict [dict create]
set C_modelArgList {
	{ acc_22 float 32 regular  }
	{ bitcast_ln12_24 float 32 regular  }
	{ bitcast_ln12_25 float 32 regular  }
	{ bitcast_ln12_26 float 32 regular  }
	{ bitcast_ln12_27 float 32 regular  }
	{ bitcast_ln12_4 float 32 regular  }
	{ bitcast_ln12_5 float 32 regular  }
	{ bitcast_ln12_6 float 32 regular  }
	{ bitcast_ln12_7 float 32 regular  }
	{ acc_26_out float 32 regular {pointer 1}  }
}
set hasAXIMCache 0
set l_AXIML2Cache [list]
set AXIMCacheInstDict [dict create]
set C_modelArgMapList {[ 
	{ "Name" : "acc_22", "interface" : "wire", "bitwidth" : 32, "direction" : "READONLY"} , 
 	{ "Name" : "bitcast_ln12_24", "interface" : "wire", "bitwidth" : 32, "direction" : "READONLY"} , 
 	{ "Name" : "bitcast_ln12_25", "interface" : "wire", "bitwidth" : 32, "direction" : "READONLY"} , 
 	{ "Name" : "bitcast_ln12_26", "interface" : "wire", "bitwidth" : 32, "direction" : "READONLY"} , 
 	{ "Name" : "bitcast_ln12_27", "interface" : "wire", "bitwidth" : 32, "direction" : "READONLY"} , 
 	{ "Name" : "bitcast_ln12_4", "interface" : "wire", "bitwidth" : 32, "direction" : "READONLY"} , 
 	{ "Name" : "bitcast_ln12_5", "interface" : "wire", "bitwidth" : 32, "direction" : "READONLY"} , 
 	{ "Name" : "bitcast_ln12_6", "interface" : "wire", "bitwidth" : 32, "direction" : "READONLY"} , 
 	{ "Name" : "bitcast_ln12_7", "interface" : "wire", "bitwidth" : 32, "direction" : "READONLY"} , 
 	{ "Name" : "acc_26_out", "interface" : "wire", "bitwidth" : 32, "direction" : "WRITEONLY"} ]}
# RTL Port declarations: 
set portNum 17
set portList { 
	{ ap_clk sc_in sc_logic 1 clock -1 } 
	{ ap_rst sc_in sc_logic 1 reset -1 active_high_sync } 
	{ ap_start sc_in sc_logic 1 start -1 } 
	{ ap_done sc_out sc_logic 1 predone -1 } 
	{ ap_idle sc_out sc_logic 1 done -1 } 
	{ ap_ready sc_out sc_logic 1 ready -1 } 
	{ acc_22 sc_in sc_lv 32 signal 0 } 
	{ bitcast_ln12_24 sc_in sc_lv 32 signal 1 } 
	{ bitcast_ln12_25 sc_in sc_lv 32 signal 2 } 
	{ bitcast_ln12_26 sc_in sc_lv 32 signal 3 } 
	{ bitcast_ln12_27 sc_in sc_lv 32 signal 4 } 
	{ bitcast_ln12_4 sc_in sc_lv 32 signal 5 } 
	{ bitcast_ln12_5 sc_in sc_lv 32 signal 6 } 
	{ bitcast_ln12_6 sc_in sc_lv 32 signal 7 } 
	{ bitcast_ln12_7 sc_in sc_lv 32 signal 8 } 
	{ acc_26_out sc_out sc_lv 32 signal 9 } 
	{ acc_26_out_ap_vld sc_out sc_logic 1 outvld 9 } 
}
set NewPortList {[ 
	{ "name": "ap_clk", "direction": "in", "datatype": "sc_logic", "bitwidth":1, "type": "clock", "bundle":{"name": "ap_clk", "role": "default" }} , 
 	{ "name": "ap_rst", "direction": "in", "datatype": "sc_logic", "bitwidth":1, "type": "reset", "bundle":{"name": "ap_rst", "role": "default" }} , 
 	{ "name": "ap_start", "direction": "in", "datatype": "sc_logic", "bitwidth":1, "type": "start", "bundle":{"name": "ap_start", "role": "default" }} , 
 	{ "name": "ap_done", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "predone", "bundle":{"name": "ap_done", "role": "default" }} , 
 	{ "name": "ap_idle", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "done", "bundle":{"name": "ap_idle", "role": "default" }} , 
 	{ "name": "ap_ready", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "ready", "bundle":{"name": "ap_ready", "role": "default" }} , 
 	{ "name": "acc_22", "direction": "in", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "acc_22", "role": "default" }} , 
 	{ "name": "bitcast_ln12_24", "direction": "in", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "bitcast_ln12_24", "role": "default" }} , 
 	{ "name": "bitcast_ln12_25", "direction": "in", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "bitcast_ln12_25", "role": "default" }} , 
 	{ "name": "bitcast_ln12_26", "direction": "in", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "bitcast_ln12_26", "role": "default" }} , 
 	{ "name": "bitcast_ln12_27", "direction": "in", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "bitcast_ln12_27", "role": "default" }} , 
 	{ "name": "bitcast_ln12_4", "direction": "in", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "bitcast_ln12_4", "role": "default" }} , 
 	{ "name": "bitcast_ln12_5", "direction": "in", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "bitcast_ln12_5", "role": "default" }} , 
 	{ "name": "bitcast_ln12_6", "direction": "in", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "bitcast_ln12_6", "role": "default" }} , 
 	{ "name": "bitcast_ln12_7", "direction": "in", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "bitcast_ln12_7", "role": "default" }} , 
 	{ "name": "acc_26_out", "direction": "out", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "acc_26_out", "role": "default" }} , 
 	{ "name": "acc_26_out_ap_vld", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "outvld", "bundle":{"name": "acc_26_out", "role": "ap_vld" }}  ]}

set ArgLastReadFirstWriteLatency {
	matmul_4x4x4_Pipeline_VITIS_LOOP_10_38 {
		acc_22 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_24 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_25 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_26 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_27 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_4 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_5 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_6 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_7 {Type I LastRead 0 FirstWrite -1}
		acc_26_out {Type O LastRead -1 FirstWrite 4}}}

set hasDtUnsupportedChannel 0

set PerformanceInfo {[
	{"Name" : "Latency", "Min" : "10", "Max" : "10"}
	, {"Name" : "Interval", "Min" : "5", "Max" : "5"}
]}

set PipelineEnableSignalInfo {[
	{"Pipeline" : "0", "EnableSignal" : "ap_enable_pp0"}
]}

set Spec2ImplPortList { 
	acc_22 { ap_none {  { acc_22 in_data 0 32 } } }
	bitcast_ln12_24 { ap_none {  { bitcast_ln12_24 in_data 0 32 } } }
	bitcast_ln12_25 { ap_none {  { bitcast_ln12_25 in_data 0 32 } } }
	bitcast_ln12_26 { ap_none {  { bitcast_ln12_26 in_data 0 32 } } }
	bitcast_ln12_27 { ap_none {  { bitcast_ln12_27 in_data 0 32 } } }
	bitcast_ln12_4 { ap_none {  { bitcast_ln12_4 in_data 0 32 } } }
	bitcast_ln12_5 { ap_none {  { bitcast_ln12_5 in_data 0 32 } } }
	bitcast_ln12_6 { ap_none {  { bitcast_ln12_6 in_data 0 32 } } }
	bitcast_ln12_7 { ap_none {  { bitcast_ln12_7 in_data 0 32 } } }
	acc_26_out { ap_vld {  { acc_26_out out_data 1 32 }  { acc_26_out_ap_vld out_vld 1 1 } } }
}
