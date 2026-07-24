set moduleName matmul_4x4x4
set isTopModule 1
set isCombinational 0
set isDatapathOnly 0
set isPipelined 0
set isPipelined_legacy 0
set pipeline_type none
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
set C_modelName {matmul_4x4x4}
set C_modelType { void 0 }
set ap_memory_interface_dict [dict create]
set C_modelArgList {
	{ arg0_0_0 int 32 regular {pointer 0}  }
	{ arg0_0_1 int 32 regular {pointer 0}  }
	{ arg0_0_2 int 32 regular {pointer 0}  }
	{ arg0_0_3 int 32 regular {pointer 0}  }
	{ arg0_1_0 int 32 regular {pointer 0}  }
	{ arg0_1_1 int 32 regular {pointer 0}  }
	{ arg0_1_2 int 32 regular {pointer 0}  }
	{ arg0_1_3 int 32 regular {pointer 0}  }
	{ arg0_2_0 int 32 regular {pointer 0}  }
	{ arg0_2_1 int 32 regular {pointer 0}  }
	{ arg0_2_2 int 32 regular {pointer 0}  }
	{ arg0_2_3 int 32 regular {pointer 0}  }
	{ arg0_3_0 int 32 regular {pointer 0}  }
	{ arg0_3_1 int 32 regular {pointer 0}  }
	{ arg0_3_2 int 32 regular {pointer 0}  }
	{ arg0_3_3 int 32 regular {pointer 0}  }
	{ arg1_0_0 int 32 regular {pointer 0}  }
	{ arg1_0_1 int 32 regular {pointer 0}  }
	{ arg1_0_2 int 32 regular {pointer 0}  }
	{ arg1_0_3 int 32 regular {pointer 0}  }
	{ arg1_1_0 int 32 regular {pointer 0}  }
	{ arg1_1_1 int 32 regular {pointer 0}  }
	{ arg1_1_2 int 32 regular {pointer 0}  }
	{ arg1_1_3 int 32 regular {pointer 0}  }
	{ arg1_2_0 int 32 regular {pointer 0}  }
	{ arg1_2_1 int 32 regular {pointer 0}  }
	{ arg1_2_2 int 32 regular {pointer 0}  }
	{ arg1_2_3 int 32 regular {pointer 0}  }
	{ arg1_3_0 int 32 regular {pointer 0}  }
	{ arg1_3_1 int 32 regular {pointer 0}  }
	{ arg1_3_2 int 32 regular {pointer 0}  }
	{ arg1_3_3 int 32 regular {pointer 0}  }
	{ arg2_0_0 int 32 regular {pointer 2}  }
	{ arg2_0_1 int 32 regular {pointer 2}  }
	{ arg2_0_2 int 32 regular {pointer 2}  }
	{ arg2_0_3 int 32 regular {pointer 2}  }
	{ arg2_1_0 int 32 regular {pointer 2}  }
	{ arg2_1_1 int 32 regular {pointer 2}  }
	{ arg2_1_2 int 32 regular {pointer 2}  }
	{ arg2_1_3 int 32 regular {pointer 2}  }
	{ arg2_2_0 int 32 regular {pointer 2}  }
	{ arg2_2_1 int 32 regular {pointer 2}  }
	{ arg2_2_2 int 32 regular {pointer 2}  }
	{ arg2_2_3 int 32 regular {pointer 2}  }
	{ arg2_3_0 int 32 regular {pointer 2}  }
	{ arg2_3_1 int 32 regular {pointer 2}  }
	{ arg2_3_2 int 32 regular {pointer 2}  }
	{ arg2_3_3 int 32 regular {pointer 2}  }
}
set hasAXIMCache 0
set l_AXIML2Cache [list]
set AXIMCacheInstDict [dict create]
set C_modelArgMapList {[ 
	{ "Name" : "arg0_0_0", "interface" : "wire", "bitwidth" : 32, "direction" : "READONLY"} , 
 	{ "Name" : "arg0_0_1", "interface" : "wire", "bitwidth" : 32, "direction" : "READONLY"} , 
 	{ "Name" : "arg0_0_2", "interface" : "wire", "bitwidth" : 32, "direction" : "READONLY"} , 
 	{ "Name" : "arg0_0_3", "interface" : "wire", "bitwidth" : 32, "direction" : "READONLY"} , 
 	{ "Name" : "arg0_1_0", "interface" : "wire", "bitwidth" : 32, "direction" : "READONLY"} , 
 	{ "Name" : "arg0_1_1", "interface" : "wire", "bitwidth" : 32, "direction" : "READONLY"} , 
 	{ "Name" : "arg0_1_2", "interface" : "wire", "bitwidth" : 32, "direction" : "READONLY"} , 
 	{ "Name" : "arg0_1_3", "interface" : "wire", "bitwidth" : 32, "direction" : "READONLY"} , 
 	{ "Name" : "arg0_2_0", "interface" : "wire", "bitwidth" : 32, "direction" : "READONLY"} , 
 	{ "Name" : "arg0_2_1", "interface" : "wire", "bitwidth" : 32, "direction" : "READONLY"} , 
 	{ "Name" : "arg0_2_2", "interface" : "wire", "bitwidth" : 32, "direction" : "READONLY"} , 
 	{ "Name" : "arg0_2_3", "interface" : "wire", "bitwidth" : 32, "direction" : "READONLY"} , 
 	{ "Name" : "arg0_3_0", "interface" : "wire", "bitwidth" : 32, "direction" : "READONLY"} , 
 	{ "Name" : "arg0_3_1", "interface" : "wire", "bitwidth" : 32, "direction" : "READONLY"} , 
 	{ "Name" : "arg0_3_2", "interface" : "wire", "bitwidth" : 32, "direction" : "READONLY"} , 
 	{ "Name" : "arg0_3_3", "interface" : "wire", "bitwidth" : 32, "direction" : "READONLY"} , 
 	{ "Name" : "arg1_0_0", "interface" : "wire", "bitwidth" : 32, "direction" : "READONLY"} , 
 	{ "Name" : "arg1_0_1", "interface" : "wire", "bitwidth" : 32, "direction" : "READONLY"} , 
 	{ "Name" : "arg1_0_2", "interface" : "wire", "bitwidth" : 32, "direction" : "READONLY"} , 
 	{ "Name" : "arg1_0_3", "interface" : "wire", "bitwidth" : 32, "direction" : "READONLY"} , 
 	{ "Name" : "arg1_1_0", "interface" : "wire", "bitwidth" : 32, "direction" : "READONLY"} , 
 	{ "Name" : "arg1_1_1", "interface" : "wire", "bitwidth" : 32, "direction" : "READONLY"} , 
 	{ "Name" : "arg1_1_2", "interface" : "wire", "bitwidth" : 32, "direction" : "READONLY"} , 
 	{ "Name" : "arg1_1_3", "interface" : "wire", "bitwidth" : 32, "direction" : "READONLY"} , 
 	{ "Name" : "arg1_2_0", "interface" : "wire", "bitwidth" : 32, "direction" : "READONLY"} , 
 	{ "Name" : "arg1_2_1", "interface" : "wire", "bitwidth" : 32, "direction" : "READONLY"} , 
 	{ "Name" : "arg1_2_2", "interface" : "wire", "bitwidth" : 32, "direction" : "READONLY"} , 
 	{ "Name" : "arg1_2_3", "interface" : "wire", "bitwidth" : 32, "direction" : "READONLY"} , 
 	{ "Name" : "arg1_3_0", "interface" : "wire", "bitwidth" : 32, "direction" : "READONLY"} , 
 	{ "Name" : "arg1_3_1", "interface" : "wire", "bitwidth" : 32, "direction" : "READONLY"} , 
 	{ "Name" : "arg1_3_2", "interface" : "wire", "bitwidth" : 32, "direction" : "READONLY"} , 
 	{ "Name" : "arg1_3_3", "interface" : "wire", "bitwidth" : 32, "direction" : "READONLY"} , 
 	{ "Name" : "arg2_0_0", "interface" : "wire", "bitwidth" : 32, "direction" : "READWRITE"} , 
 	{ "Name" : "arg2_0_1", "interface" : "wire", "bitwidth" : 32, "direction" : "READWRITE"} , 
 	{ "Name" : "arg2_0_2", "interface" : "wire", "bitwidth" : 32, "direction" : "READWRITE"} , 
 	{ "Name" : "arg2_0_3", "interface" : "wire", "bitwidth" : 32, "direction" : "READWRITE"} , 
 	{ "Name" : "arg2_1_0", "interface" : "wire", "bitwidth" : 32, "direction" : "READWRITE"} , 
 	{ "Name" : "arg2_1_1", "interface" : "wire", "bitwidth" : 32, "direction" : "READWRITE"} , 
 	{ "Name" : "arg2_1_2", "interface" : "wire", "bitwidth" : 32, "direction" : "READWRITE"} , 
 	{ "Name" : "arg2_1_3", "interface" : "wire", "bitwidth" : 32, "direction" : "READWRITE"} , 
 	{ "Name" : "arg2_2_0", "interface" : "wire", "bitwidth" : 32, "direction" : "READWRITE"} , 
 	{ "Name" : "arg2_2_1", "interface" : "wire", "bitwidth" : 32, "direction" : "READWRITE"} , 
 	{ "Name" : "arg2_2_2", "interface" : "wire", "bitwidth" : 32, "direction" : "READWRITE"} , 
 	{ "Name" : "arg2_2_3", "interface" : "wire", "bitwidth" : 32, "direction" : "READWRITE"} , 
 	{ "Name" : "arg2_3_0", "interface" : "wire", "bitwidth" : 32, "direction" : "READWRITE"} , 
 	{ "Name" : "arg2_3_1", "interface" : "wire", "bitwidth" : 32, "direction" : "READWRITE"} , 
 	{ "Name" : "arg2_3_2", "interface" : "wire", "bitwidth" : 32, "direction" : "READWRITE"} , 
 	{ "Name" : "arg2_3_3", "interface" : "wire", "bitwidth" : 32, "direction" : "READWRITE"} ]}
# RTL Port declarations: 
set portNum 86
set portList { 
	{ ap_clk sc_in sc_logic 1 clock -1 } 
	{ ap_rst sc_in sc_logic 1 reset -1 active_high_sync } 
	{ ap_start sc_in sc_logic 1 start -1 } 
	{ ap_done sc_out sc_logic 1 predone -1 } 
	{ ap_idle sc_out sc_logic 1 done -1 } 
	{ ap_ready sc_out sc_logic 1 ready -1 } 
	{ arg0_0_0 sc_in sc_lv 32 signal 0 } 
	{ arg0_0_1 sc_in sc_lv 32 signal 1 } 
	{ arg0_0_2 sc_in sc_lv 32 signal 2 } 
	{ arg0_0_3 sc_in sc_lv 32 signal 3 } 
	{ arg0_1_0 sc_in sc_lv 32 signal 4 } 
	{ arg0_1_1 sc_in sc_lv 32 signal 5 } 
	{ arg0_1_2 sc_in sc_lv 32 signal 6 } 
	{ arg0_1_3 sc_in sc_lv 32 signal 7 } 
	{ arg0_2_0 sc_in sc_lv 32 signal 8 } 
	{ arg0_2_1 sc_in sc_lv 32 signal 9 } 
	{ arg0_2_2 sc_in sc_lv 32 signal 10 } 
	{ arg0_2_3 sc_in sc_lv 32 signal 11 } 
	{ arg0_3_0 sc_in sc_lv 32 signal 12 } 
	{ arg0_3_1 sc_in sc_lv 32 signal 13 } 
	{ arg0_3_2 sc_in sc_lv 32 signal 14 } 
	{ arg0_3_3 sc_in sc_lv 32 signal 15 } 
	{ arg1_0_0 sc_in sc_lv 32 signal 16 } 
	{ arg1_0_1 sc_in sc_lv 32 signal 17 } 
	{ arg1_0_2 sc_in sc_lv 32 signal 18 } 
	{ arg1_0_3 sc_in sc_lv 32 signal 19 } 
	{ arg1_1_0 sc_in sc_lv 32 signal 20 } 
	{ arg1_1_1 sc_in sc_lv 32 signal 21 } 
	{ arg1_1_2 sc_in sc_lv 32 signal 22 } 
	{ arg1_1_3 sc_in sc_lv 32 signal 23 } 
	{ arg1_2_0 sc_in sc_lv 32 signal 24 } 
	{ arg1_2_1 sc_in sc_lv 32 signal 25 } 
	{ arg1_2_2 sc_in sc_lv 32 signal 26 } 
	{ arg1_2_3 sc_in sc_lv 32 signal 27 } 
	{ arg1_3_0 sc_in sc_lv 32 signal 28 } 
	{ arg1_3_1 sc_in sc_lv 32 signal 29 } 
	{ arg1_3_2 sc_in sc_lv 32 signal 30 } 
	{ arg1_3_3 sc_in sc_lv 32 signal 31 } 
	{ arg2_0_0_i sc_in sc_lv 32 signal 32 } 
	{ arg2_0_0_o sc_out sc_lv 32 signal 32 } 
	{ arg2_0_0_o_ap_vld sc_out sc_logic 1 outvld 32 } 
	{ arg2_0_1_i sc_in sc_lv 32 signal 33 } 
	{ arg2_0_1_o sc_out sc_lv 32 signal 33 } 
	{ arg2_0_1_o_ap_vld sc_out sc_logic 1 outvld 33 } 
	{ arg2_0_2_i sc_in sc_lv 32 signal 34 } 
	{ arg2_0_2_o sc_out sc_lv 32 signal 34 } 
	{ arg2_0_2_o_ap_vld sc_out sc_logic 1 outvld 34 } 
	{ arg2_0_3_i sc_in sc_lv 32 signal 35 } 
	{ arg2_0_3_o sc_out sc_lv 32 signal 35 } 
	{ arg2_0_3_o_ap_vld sc_out sc_logic 1 outvld 35 } 
	{ arg2_1_0_i sc_in sc_lv 32 signal 36 } 
	{ arg2_1_0_o sc_out sc_lv 32 signal 36 } 
	{ arg2_1_0_o_ap_vld sc_out sc_logic 1 outvld 36 } 
	{ arg2_1_1_i sc_in sc_lv 32 signal 37 } 
	{ arg2_1_1_o sc_out sc_lv 32 signal 37 } 
	{ arg2_1_1_o_ap_vld sc_out sc_logic 1 outvld 37 } 
	{ arg2_1_2_i sc_in sc_lv 32 signal 38 } 
	{ arg2_1_2_o sc_out sc_lv 32 signal 38 } 
	{ arg2_1_2_o_ap_vld sc_out sc_logic 1 outvld 38 } 
	{ arg2_1_3_i sc_in sc_lv 32 signal 39 } 
	{ arg2_1_3_o sc_out sc_lv 32 signal 39 } 
	{ arg2_1_3_o_ap_vld sc_out sc_logic 1 outvld 39 } 
	{ arg2_2_0_i sc_in sc_lv 32 signal 40 } 
	{ arg2_2_0_o sc_out sc_lv 32 signal 40 } 
	{ arg2_2_0_o_ap_vld sc_out sc_logic 1 outvld 40 } 
	{ arg2_2_1_i sc_in sc_lv 32 signal 41 } 
	{ arg2_2_1_o sc_out sc_lv 32 signal 41 } 
	{ arg2_2_1_o_ap_vld sc_out sc_logic 1 outvld 41 } 
	{ arg2_2_2_i sc_in sc_lv 32 signal 42 } 
	{ arg2_2_2_o sc_out sc_lv 32 signal 42 } 
	{ arg2_2_2_o_ap_vld sc_out sc_logic 1 outvld 42 } 
	{ arg2_2_3_i sc_in sc_lv 32 signal 43 } 
	{ arg2_2_3_o sc_out sc_lv 32 signal 43 } 
	{ arg2_2_3_o_ap_vld sc_out sc_logic 1 outvld 43 } 
	{ arg2_3_0_i sc_in sc_lv 32 signal 44 } 
	{ arg2_3_0_o sc_out sc_lv 32 signal 44 } 
	{ arg2_3_0_o_ap_vld sc_out sc_logic 1 outvld 44 } 
	{ arg2_3_1_i sc_in sc_lv 32 signal 45 } 
	{ arg2_3_1_o sc_out sc_lv 32 signal 45 } 
	{ arg2_3_1_o_ap_vld sc_out sc_logic 1 outvld 45 } 
	{ arg2_3_2_i sc_in sc_lv 32 signal 46 } 
	{ arg2_3_2_o sc_out sc_lv 32 signal 46 } 
	{ arg2_3_2_o_ap_vld sc_out sc_logic 1 outvld 46 } 
	{ arg2_3_3_i sc_in sc_lv 32 signal 47 } 
	{ arg2_3_3_o sc_out sc_lv 32 signal 47 } 
	{ arg2_3_3_o_ap_vld sc_out sc_logic 1 outvld 47 } 
}
set NewPortList {[ 
	{ "name": "ap_clk", "direction": "in", "datatype": "sc_logic", "bitwidth":1, "type": "clock", "bundle":{"name": "ap_clk", "role": "default" }} , 
 	{ "name": "ap_rst", "direction": "in", "datatype": "sc_logic", "bitwidth":1, "type": "reset", "bundle":{"name": "ap_rst", "role": "default" }} , 
 	{ "name": "ap_start", "direction": "in", "datatype": "sc_logic", "bitwidth":1, "type": "start", "bundle":{"name": "ap_start", "role": "default" }} , 
 	{ "name": "ap_done", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "predone", "bundle":{"name": "ap_done", "role": "default" }} , 
 	{ "name": "ap_idle", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "done", "bundle":{"name": "ap_idle", "role": "default" }} , 
 	{ "name": "ap_ready", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "ready", "bundle":{"name": "ap_ready", "role": "default" }} , 
 	{ "name": "arg0_0_0", "direction": "in", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "arg0_0_0", "role": "default" }} , 
 	{ "name": "arg0_0_1", "direction": "in", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "arg0_0_1", "role": "default" }} , 
 	{ "name": "arg0_0_2", "direction": "in", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "arg0_0_2", "role": "default" }} , 
 	{ "name": "arg0_0_3", "direction": "in", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "arg0_0_3", "role": "default" }} , 
 	{ "name": "arg0_1_0", "direction": "in", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "arg0_1_0", "role": "default" }} , 
 	{ "name": "arg0_1_1", "direction": "in", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "arg0_1_1", "role": "default" }} , 
 	{ "name": "arg0_1_2", "direction": "in", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "arg0_1_2", "role": "default" }} , 
 	{ "name": "arg0_1_3", "direction": "in", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "arg0_1_3", "role": "default" }} , 
 	{ "name": "arg0_2_0", "direction": "in", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "arg0_2_0", "role": "default" }} , 
 	{ "name": "arg0_2_1", "direction": "in", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "arg0_2_1", "role": "default" }} , 
 	{ "name": "arg0_2_2", "direction": "in", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "arg0_2_2", "role": "default" }} , 
 	{ "name": "arg0_2_3", "direction": "in", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "arg0_2_3", "role": "default" }} , 
 	{ "name": "arg0_3_0", "direction": "in", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "arg0_3_0", "role": "default" }} , 
 	{ "name": "arg0_3_1", "direction": "in", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "arg0_3_1", "role": "default" }} , 
 	{ "name": "arg0_3_2", "direction": "in", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "arg0_3_2", "role": "default" }} , 
 	{ "name": "arg0_3_3", "direction": "in", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "arg0_3_3", "role": "default" }} , 
 	{ "name": "arg1_0_0", "direction": "in", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "arg1_0_0", "role": "default" }} , 
 	{ "name": "arg1_0_1", "direction": "in", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "arg1_0_1", "role": "default" }} , 
 	{ "name": "arg1_0_2", "direction": "in", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "arg1_0_2", "role": "default" }} , 
 	{ "name": "arg1_0_3", "direction": "in", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "arg1_0_3", "role": "default" }} , 
 	{ "name": "arg1_1_0", "direction": "in", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "arg1_1_0", "role": "default" }} , 
 	{ "name": "arg1_1_1", "direction": "in", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "arg1_1_1", "role": "default" }} , 
 	{ "name": "arg1_1_2", "direction": "in", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "arg1_1_2", "role": "default" }} , 
 	{ "name": "arg1_1_3", "direction": "in", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "arg1_1_3", "role": "default" }} , 
 	{ "name": "arg1_2_0", "direction": "in", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "arg1_2_0", "role": "default" }} , 
 	{ "name": "arg1_2_1", "direction": "in", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "arg1_2_1", "role": "default" }} , 
 	{ "name": "arg1_2_2", "direction": "in", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "arg1_2_2", "role": "default" }} , 
 	{ "name": "arg1_2_3", "direction": "in", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "arg1_2_3", "role": "default" }} , 
 	{ "name": "arg1_3_0", "direction": "in", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "arg1_3_0", "role": "default" }} , 
 	{ "name": "arg1_3_1", "direction": "in", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "arg1_3_1", "role": "default" }} , 
 	{ "name": "arg1_3_2", "direction": "in", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "arg1_3_2", "role": "default" }} , 
 	{ "name": "arg1_3_3", "direction": "in", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "arg1_3_3", "role": "default" }} , 
 	{ "name": "arg2_0_0_i", "direction": "in", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "arg2_0_0", "role": "i" }} , 
 	{ "name": "arg2_0_0_o", "direction": "out", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "arg2_0_0", "role": "o" }} , 
 	{ "name": "arg2_0_0_o_ap_vld", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "outvld", "bundle":{"name": "arg2_0_0", "role": "o_ap_vld" }} , 
 	{ "name": "arg2_0_1_i", "direction": "in", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "arg2_0_1", "role": "i" }} , 
 	{ "name": "arg2_0_1_o", "direction": "out", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "arg2_0_1", "role": "o" }} , 
 	{ "name": "arg2_0_1_o_ap_vld", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "outvld", "bundle":{"name": "arg2_0_1", "role": "o_ap_vld" }} , 
 	{ "name": "arg2_0_2_i", "direction": "in", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "arg2_0_2", "role": "i" }} , 
 	{ "name": "arg2_0_2_o", "direction": "out", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "arg2_0_2", "role": "o" }} , 
 	{ "name": "arg2_0_2_o_ap_vld", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "outvld", "bundle":{"name": "arg2_0_2", "role": "o_ap_vld" }} , 
 	{ "name": "arg2_0_3_i", "direction": "in", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "arg2_0_3", "role": "i" }} , 
 	{ "name": "arg2_0_3_o", "direction": "out", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "arg2_0_3", "role": "o" }} , 
 	{ "name": "arg2_0_3_o_ap_vld", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "outvld", "bundle":{"name": "arg2_0_3", "role": "o_ap_vld" }} , 
 	{ "name": "arg2_1_0_i", "direction": "in", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "arg2_1_0", "role": "i" }} , 
 	{ "name": "arg2_1_0_o", "direction": "out", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "arg2_1_0", "role": "o" }} , 
 	{ "name": "arg2_1_0_o_ap_vld", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "outvld", "bundle":{"name": "arg2_1_0", "role": "o_ap_vld" }} , 
 	{ "name": "arg2_1_1_i", "direction": "in", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "arg2_1_1", "role": "i" }} , 
 	{ "name": "arg2_1_1_o", "direction": "out", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "arg2_1_1", "role": "o" }} , 
 	{ "name": "arg2_1_1_o_ap_vld", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "outvld", "bundle":{"name": "arg2_1_1", "role": "o_ap_vld" }} , 
 	{ "name": "arg2_1_2_i", "direction": "in", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "arg2_1_2", "role": "i" }} , 
 	{ "name": "arg2_1_2_o", "direction": "out", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "arg2_1_2", "role": "o" }} , 
 	{ "name": "arg2_1_2_o_ap_vld", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "outvld", "bundle":{"name": "arg2_1_2", "role": "o_ap_vld" }} , 
 	{ "name": "arg2_1_3_i", "direction": "in", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "arg2_1_3", "role": "i" }} , 
 	{ "name": "arg2_1_3_o", "direction": "out", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "arg2_1_3", "role": "o" }} , 
 	{ "name": "arg2_1_3_o_ap_vld", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "outvld", "bundle":{"name": "arg2_1_3", "role": "o_ap_vld" }} , 
 	{ "name": "arg2_2_0_i", "direction": "in", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "arg2_2_0", "role": "i" }} , 
 	{ "name": "arg2_2_0_o", "direction": "out", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "arg2_2_0", "role": "o" }} , 
 	{ "name": "arg2_2_0_o_ap_vld", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "outvld", "bundle":{"name": "arg2_2_0", "role": "o_ap_vld" }} , 
 	{ "name": "arg2_2_1_i", "direction": "in", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "arg2_2_1", "role": "i" }} , 
 	{ "name": "arg2_2_1_o", "direction": "out", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "arg2_2_1", "role": "o" }} , 
 	{ "name": "arg2_2_1_o_ap_vld", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "outvld", "bundle":{"name": "arg2_2_1", "role": "o_ap_vld" }} , 
 	{ "name": "arg2_2_2_i", "direction": "in", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "arg2_2_2", "role": "i" }} , 
 	{ "name": "arg2_2_2_o", "direction": "out", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "arg2_2_2", "role": "o" }} , 
 	{ "name": "arg2_2_2_o_ap_vld", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "outvld", "bundle":{"name": "arg2_2_2", "role": "o_ap_vld" }} , 
 	{ "name": "arg2_2_3_i", "direction": "in", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "arg2_2_3", "role": "i" }} , 
 	{ "name": "arg2_2_3_o", "direction": "out", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "arg2_2_3", "role": "o" }} , 
 	{ "name": "arg2_2_3_o_ap_vld", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "outvld", "bundle":{"name": "arg2_2_3", "role": "o_ap_vld" }} , 
 	{ "name": "arg2_3_0_i", "direction": "in", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "arg2_3_0", "role": "i" }} , 
 	{ "name": "arg2_3_0_o", "direction": "out", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "arg2_3_0", "role": "o" }} , 
 	{ "name": "arg2_3_0_o_ap_vld", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "outvld", "bundle":{"name": "arg2_3_0", "role": "o_ap_vld" }} , 
 	{ "name": "arg2_3_1_i", "direction": "in", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "arg2_3_1", "role": "i" }} , 
 	{ "name": "arg2_3_1_o", "direction": "out", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "arg2_3_1", "role": "o" }} , 
 	{ "name": "arg2_3_1_o_ap_vld", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "outvld", "bundle":{"name": "arg2_3_1", "role": "o_ap_vld" }} , 
 	{ "name": "arg2_3_2_i", "direction": "in", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "arg2_3_2", "role": "i" }} , 
 	{ "name": "arg2_3_2_o", "direction": "out", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "arg2_3_2", "role": "o" }} , 
 	{ "name": "arg2_3_2_o_ap_vld", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "outvld", "bundle":{"name": "arg2_3_2", "role": "o_ap_vld" }} , 
 	{ "name": "arg2_3_3_i", "direction": "in", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "arg2_3_3", "role": "i" }} , 
 	{ "name": "arg2_3_3_o", "direction": "out", "datatype": "sc_lv", "bitwidth":32, "type": "signal", "bundle":{"name": "arg2_3_3", "role": "o" }} , 
 	{ "name": "arg2_3_3_o_ap_vld", "direction": "out", "datatype": "sc_logic", "bitwidth":1, "type": "outvld", "bundle":{"name": "arg2_3_3", "role": "o_ap_vld" }}  ]}

set ArgLastReadFirstWriteLatency {
	matmul_4x4x4 {
		arg0_0_0 {Type I LastRead 0 FirstWrite -1}
		arg0_0_1 {Type I LastRead 0 FirstWrite -1}
		arg0_0_2 {Type I LastRead 0 FirstWrite -1}
		arg0_0_3 {Type I LastRead 0 FirstWrite -1}
		arg0_1_0 {Type I LastRead 0 FirstWrite -1}
		arg0_1_1 {Type I LastRead 0 FirstWrite -1}
		arg0_1_2 {Type I LastRead 0 FirstWrite -1}
		arg0_1_3 {Type I LastRead 0 FirstWrite -1}
		arg0_2_0 {Type I LastRead 0 FirstWrite -1}
		arg0_2_1 {Type I LastRead 0 FirstWrite -1}
		arg0_2_2 {Type I LastRead 0 FirstWrite -1}
		arg0_2_3 {Type I LastRead 0 FirstWrite -1}
		arg0_3_0 {Type I LastRead 0 FirstWrite -1}
		arg0_3_1 {Type I LastRead 0 FirstWrite -1}
		arg0_3_2 {Type I LastRead 0 FirstWrite -1}
		arg0_3_3 {Type I LastRead 0 FirstWrite -1}
		arg1_0_0 {Type I LastRead 0 FirstWrite -1}
		arg1_0_1 {Type I LastRead 0 FirstWrite -1}
		arg1_0_2 {Type I LastRead 0 FirstWrite -1}
		arg1_0_3 {Type I LastRead 0 FirstWrite -1}
		arg1_1_0 {Type I LastRead 0 FirstWrite -1}
		arg1_1_1 {Type I LastRead 0 FirstWrite -1}
		arg1_1_2 {Type I LastRead 0 FirstWrite -1}
		arg1_1_3 {Type I LastRead 0 FirstWrite -1}
		arg1_2_0 {Type I LastRead 0 FirstWrite -1}
		arg1_2_1 {Type I LastRead 0 FirstWrite -1}
		arg1_2_2 {Type I LastRead 0 FirstWrite -1}
		arg1_2_3 {Type I LastRead 0 FirstWrite -1}
		arg1_3_0 {Type I LastRead 0 FirstWrite -1}
		arg1_3_1 {Type I LastRead 0 FirstWrite -1}
		arg1_3_2 {Type I LastRead 0 FirstWrite -1}
		arg1_3_3 {Type I LastRead 0 FirstWrite -1}
		arg2_0_0 {Type IO LastRead 0 FirstWrite 2}
		arg2_0_1 {Type IO LastRead 0 FirstWrite 2}
		arg2_0_2 {Type IO LastRead 0 FirstWrite 2}
		arg2_0_3 {Type IO LastRead 0 FirstWrite 2}
		arg2_1_0 {Type IO LastRead 0 FirstWrite 2}
		arg2_1_1 {Type IO LastRead 0 FirstWrite 2}
		arg2_1_2 {Type IO LastRead 0 FirstWrite 2}
		arg2_1_3 {Type IO LastRead 0 FirstWrite 2}
		arg2_2_0 {Type IO LastRead 0 FirstWrite 2}
		arg2_2_1 {Type IO LastRead 0 FirstWrite 2}
		arg2_2_2 {Type IO LastRead 0 FirstWrite 2}
		arg2_2_3 {Type IO LastRead 0 FirstWrite 2}
		arg2_3_0 {Type IO LastRead 0 FirstWrite 2}
		arg2_3_1 {Type IO LastRead 0 FirstWrite 2}
		arg2_3_2 {Type IO LastRead 0 FirstWrite 2}
		arg2_3_3 {Type IO LastRead 0 FirstWrite 2}}
	matmul_4x4x4_Pipeline_VITIS_LOOP_10_3 {
		acc {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_1 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_2 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_3 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_4 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_5 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_6 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_7 {Type I LastRead 0 FirstWrite -1}
		acc_2_out {Type O LastRead -1 FirstWrite 4}}
	matmul_4x4x4_Pipeline_VITIS_LOOP_10_31 {
		acc_1 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_1 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_2 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_3 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_8 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_9 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_10 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_11 {Type I LastRead 0 FirstWrite -1}
		acc_5_out {Type O LastRead -1 FirstWrite 4}}
	matmul_4x4x4_Pipeline_VITIS_LOOP_10_32 {
		acc_4 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_1 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_2 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_3 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_12 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_13 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_14 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_15 {Type I LastRead 0 FirstWrite -1}
		acc_8_out {Type O LastRead -1 FirstWrite 4}}
	matmul_4x4x4_Pipeline_VITIS_LOOP_10_33 {
		acc_7 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_1 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_2 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_3 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_16 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_17 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_18 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_19 {Type I LastRead 0 FirstWrite -1}
		acc_11_out {Type O LastRead -1 FirstWrite 4}}
	matmul_4x4x4_Pipeline_VITIS_LOOP_10_34 {
		acc_10 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_20 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_21 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_22 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_23 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_4 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_5 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_6 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_7 {Type I LastRead 0 FirstWrite -1}
		acc_14_out {Type O LastRead -1 FirstWrite 4}}
	matmul_4x4x4_Pipeline_VITIS_LOOP_10_35 {
		acc_13 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_20 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_21 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_22 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_23 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_8 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_9 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_10 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_11 {Type I LastRead 0 FirstWrite -1}
		acc_17_out {Type O LastRead -1 FirstWrite 4}}
	matmul_4x4x4_Pipeline_VITIS_LOOP_10_36 {
		acc_16 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_20 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_21 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_22 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_23 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_12 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_13 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_14 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_15 {Type I LastRead 0 FirstWrite -1}
		acc_20_out {Type O LastRead -1 FirstWrite 4}}
	matmul_4x4x4_Pipeline_VITIS_LOOP_10_37 {
		acc_19 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_20 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_21 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_22 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_23 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_16 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_17 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_18 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_19 {Type I LastRead 0 FirstWrite -1}
		acc_23_out {Type O LastRead -1 FirstWrite 4}}
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
		acc_26_out {Type O LastRead -1 FirstWrite 4}}
	matmul_4x4x4_Pipeline_VITIS_LOOP_10_39 {
		acc_25 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_24 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_25 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_26 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_27 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_8 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_9 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_10 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_11 {Type I LastRead 0 FirstWrite -1}
		acc_29_out {Type O LastRead -1 FirstWrite 4}}
	matmul_4x4x4_Pipeline_VITIS_LOOP_10_310 {
		acc_28 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_24 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_25 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_26 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_27 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_12 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_13 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_14 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_15 {Type I LastRead 0 FirstWrite -1}
		acc_32_out {Type O LastRead -1 FirstWrite 4}}
	matmul_4x4x4_Pipeline_VITIS_LOOP_10_311 {
		acc_31 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_24 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_25 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_26 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_27 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_16 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_17 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_18 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_19 {Type I LastRead 0 FirstWrite -1}
		acc_35_out {Type O LastRead -1 FirstWrite 4}}
	matmul_4x4x4_Pipeline_VITIS_LOOP_10_312 {
		acc_34 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_28 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_29 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_30 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_31 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_4 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_5 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_6 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_7 {Type I LastRead 0 FirstWrite -1}
		acc_38_out {Type O LastRead -1 FirstWrite 4}}
	matmul_4x4x4_Pipeline_VITIS_LOOP_10_313 {
		acc_37 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_28 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_29 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_30 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_31 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_8 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_9 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_10 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_11 {Type I LastRead 0 FirstWrite -1}
		acc_41_out {Type O LastRead -1 FirstWrite 4}}
	matmul_4x4x4_Pipeline_VITIS_LOOP_10_314 {
		acc_40 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_28 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_29 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_30 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_31 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_12 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_13 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_14 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_15 {Type I LastRead 0 FirstWrite -1}
		acc_44_out {Type O LastRead -1 FirstWrite 4}}
	matmul_4x4x4_Pipeline_VITIS_LOOP_10_315 {
		acc_43 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_28 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_29 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_30 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_31 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_16 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_17 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_18 {Type I LastRead 0 FirstWrite -1}
		bitcast_ln12_19 {Type I LastRead 0 FirstWrite -1}
		acc_46_out {Type O LastRead -1 FirstWrite 4}}}

set hasDtUnsupportedChannel 0

set PerformanceInfo {[
	{"Name" : "Latency", "Min" : "12", "Max" : "12"}
	, {"Name" : "Interval", "Min" : "13", "Max" : "13"}
]}

set PipelineEnableSignalInfo {[
]}

set Spec2ImplPortList { 
	arg0_0_0 { ap_none {  { arg0_0_0 in_data 0 32 } } }
	arg0_0_1 { ap_none {  { arg0_0_1 in_data 0 32 } } }
	arg0_0_2 { ap_none {  { arg0_0_2 in_data 0 32 } } }
	arg0_0_3 { ap_none {  { arg0_0_3 in_data 0 32 } } }
	arg0_1_0 { ap_none {  { arg0_1_0 in_data 0 32 } } }
	arg0_1_1 { ap_none {  { arg0_1_1 in_data 0 32 } } }
	arg0_1_2 { ap_none {  { arg0_1_2 in_data 0 32 } } }
	arg0_1_3 { ap_none {  { arg0_1_3 in_data 0 32 } } }
	arg0_2_0 { ap_none {  { arg0_2_0 in_data 0 32 } } }
	arg0_2_1 { ap_none {  { arg0_2_1 in_data 0 32 } } }
	arg0_2_2 { ap_none {  { arg0_2_2 in_data 0 32 } } }
	arg0_2_3 { ap_none {  { arg0_2_3 in_data 0 32 } } }
	arg0_3_0 { ap_none {  { arg0_3_0 in_data 0 32 } } }
	arg0_3_1 { ap_none {  { arg0_3_1 in_data 0 32 } } }
	arg0_3_2 { ap_none {  { arg0_3_2 in_data 0 32 } } }
	arg0_3_3 { ap_none {  { arg0_3_3 in_data 0 32 } } }
	arg1_0_0 { ap_none {  { arg1_0_0 in_data 0 32 } } }
	arg1_0_1 { ap_none {  { arg1_0_1 in_data 0 32 } } }
	arg1_0_2 { ap_none {  { arg1_0_2 in_data 0 32 } } }
	arg1_0_3 { ap_none {  { arg1_0_3 in_data 0 32 } } }
	arg1_1_0 { ap_none {  { arg1_1_0 in_data 0 32 } } }
	arg1_1_1 { ap_none {  { arg1_1_1 in_data 0 32 } } }
	arg1_1_2 { ap_none {  { arg1_1_2 in_data 0 32 } } }
	arg1_1_3 { ap_none {  { arg1_1_3 in_data 0 32 } } }
	arg1_2_0 { ap_none {  { arg1_2_0 in_data 0 32 } } }
	arg1_2_1 { ap_none {  { arg1_2_1 in_data 0 32 } } }
	arg1_2_2 { ap_none {  { arg1_2_2 in_data 0 32 } } }
	arg1_2_3 { ap_none {  { arg1_2_3 in_data 0 32 } } }
	arg1_3_0 { ap_none {  { arg1_3_0 in_data 0 32 } } }
	arg1_3_1 { ap_none {  { arg1_3_1 in_data 0 32 } } }
	arg1_3_2 { ap_none {  { arg1_3_2 in_data 0 32 } } }
	arg1_3_3 { ap_none {  { arg1_3_3 in_data 0 32 } } }
	arg2_0_0 { ap_ovld {  { arg2_0_0_i in_data 0 32 }  { arg2_0_0_o out_data 1 32 }  { arg2_0_0_o_ap_vld out_vld 1 1 } } }
	arg2_0_1 { ap_ovld {  { arg2_0_1_i in_data 0 32 }  { arg2_0_1_o out_data 1 32 }  { arg2_0_1_o_ap_vld out_vld 1 1 } } }
	arg2_0_2 { ap_ovld {  { arg2_0_2_i in_data 0 32 }  { arg2_0_2_o out_data 1 32 }  { arg2_0_2_o_ap_vld out_vld 1 1 } } }
	arg2_0_3 { ap_ovld {  { arg2_0_3_i in_data 0 32 }  { arg2_0_3_o out_data 1 32 }  { arg2_0_3_o_ap_vld out_vld 1 1 } } }
	arg2_1_0 { ap_ovld {  { arg2_1_0_i in_data 0 32 }  { arg2_1_0_o out_data 1 32 }  { arg2_1_0_o_ap_vld out_vld 1 1 } } }
	arg2_1_1 { ap_ovld {  { arg2_1_1_i in_data 0 32 }  { arg2_1_1_o out_data 1 32 }  { arg2_1_1_o_ap_vld out_vld 1 1 } } }
	arg2_1_2 { ap_ovld {  { arg2_1_2_i in_data 0 32 }  { arg2_1_2_o out_data 1 32 }  { arg2_1_2_o_ap_vld out_vld 1 1 } } }
	arg2_1_3 { ap_ovld {  { arg2_1_3_i in_data 0 32 }  { arg2_1_3_o out_data 1 32 }  { arg2_1_3_o_ap_vld out_vld 1 1 } } }
	arg2_2_0 { ap_ovld {  { arg2_2_0_i in_data 0 32 }  { arg2_2_0_o out_data 1 32 }  { arg2_2_0_o_ap_vld out_vld 1 1 } } }
	arg2_2_1 { ap_ovld {  { arg2_2_1_i in_data 0 32 }  { arg2_2_1_o out_data 1 32 }  { arg2_2_1_o_ap_vld out_vld 1 1 } } }
	arg2_2_2 { ap_ovld {  { arg2_2_2_i in_data 0 32 }  { arg2_2_2_o out_data 1 32 }  { arg2_2_2_o_ap_vld out_vld 1 1 } } }
	arg2_2_3 { ap_ovld {  { arg2_2_3_i in_data 0 32 }  { arg2_2_3_o out_data 1 32 }  { arg2_2_3_o_ap_vld out_vld 1 1 } } }
	arg2_3_0 { ap_ovld {  { arg2_3_0_i in_data 0 32 }  { arg2_3_0_o out_data 1 32 }  { arg2_3_0_o_ap_vld out_vld 1 1 } } }
	arg2_3_1 { ap_ovld {  { arg2_3_1_i in_data 0 32 }  { arg2_3_1_o out_data 1 32 }  { arg2_3_1_o_ap_vld out_vld 1 1 } } }
	arg2_3_2 { ap_ovld {  { arg2_3_2_i in_data 0 32 }  { arg2_3_2_o out_data 1 32 }  { arg2_3_2_o_ap_vld out_vld 1 1 } } }
	arg2_3_3 { ap_ovld {  { arg2_3_3_i in_data 0 32 }  { arg2_3_3_o out_data 1 32 }  { arg2_3_3_o_ap_vld out_vld 1 1 } } }
}

set maxi_interface_dict [dict create]

# RTL port scheduling information:
set fifoSchedulingInfoList { 
}

# RTL bus port read request latency information:
set busReadReqLatencyList { 
}

# RTL bus port write response latency information:
set busWriteResLatencyList { 
}

# RTL array port load latency information:
set memoryLoadLatencyList { 
}
