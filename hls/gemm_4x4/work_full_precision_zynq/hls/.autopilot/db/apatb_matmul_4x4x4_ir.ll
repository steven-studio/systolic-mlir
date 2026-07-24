; ModuleID = '/home/steven-studio/systolic-mlir/hls/gemm_4x4/work_full_precision_zynq/hls/.autopilot/db/a.g.ld.5.gdce.bc'
source_filename = "llvm-link"
target datalayout = "e-m:e-i64:64-i128:128-i256:256-i512:512-i1024:1024-i2048:2048-i4096:4096-n8:16:32:64-S128-v16:16-v24:32-v32:32-v48:64-v96:128-v192:256-v256:256-v512:512-v1024:1024"
target triple = "fpga64-xilinx-none"

; Function Attrs: inaccessiblemem_or_argmemonly noinline willreturn
define void @apatb_matmul_4x4x4_ir([4 x float]* noalias nocapture nonnull readonly "fpga.decayed.dim.hint"="4" "partition" %arg0, [4 x float]* noalias nocapture nonnull readonly "fpga.decayed.dim.hint"="4" "partition" %arg1, [4 x float]* noalias nocapture nonnull "fpga.decayed.dim.hint"="4" "partition" %arg2) local_unnamed_addr #0 {
entry:
  %0 = bitcast [4 x float]* %arg0 to [4 x [4 x float]]*
  %arg0_copy_0_0 = alloca float, align 512
  %arg0_copy_0_1 = alloca float, align 512
  %arg0_copy_0_2 = alloca float, align 512
  %arg0_copy_0_3 = alloca float, align 512
  %arg0_copy_1_0 = alloca float, align 512
  %arg0_copy_1_1 = alloca float, align 512
  %arg0_copy_1_2 = alloca float, align 512
  %arg0_copy_1_3 = alloca float, align 512
  %arg0_copy_2_0 = alloca float, align 512
  %arg0_copy_2_1 = alloca float, align 512
  %arg0_copy_2_2 = alloca float, align 512
  %arg0_copy_2_3 = alloca float, align 512
  %arg0_copy_3_0 = alloca float, align 512
  %arg0_copy_3_1 = alloca float, align 512
  %arg0_copy_3_2 = alloca float, align 512
  %arg0_copy_3_3 = alloca float, align 512
  %1 = bitcast [4 x float]* %arg1 to [4 x [4 x float]]*
  %arg1_copy_0_0 = alloca float, align 512
  %arg1_copy_0_1 = alloca float, align 512
  %arg1_copy_0_2 = alloca float, align 512
  %arg1_copy_0_3 = alloca float, align 512
  %arg1_copy_1_0 = alloca float, align 512
  %arg1_copy_1_1 = alloca float, align 512
  %arg1_copy_1_2 = alloca float, align 512
  %arg1_copy_1_3 = alloca float, align 512
  %arg1_copy_2_0 = alloca float, align 512
  %arg1_copy_2_1 = alloca float, align 512
  %arg1_copy_2_2 = alloca float, align 512
  %arg1_copy_2_3 = alloca float, align 512
  %arg1_copy_3_0 = alloca float, align 512
  %arg1_copy_3_1 = alloca float, align 512
  %arg1_copy_3_2 = alloca float, align 512
  %arg1_copy_3_3 = alloca float, align 512
  %2 = bitcast [4 x float]* %arg2 to [4 x [4 x float]]*
  %arg2_copy_0_0 = alloca float, align 512
  %arg2_copy_0_1 = alloca float, align 512
  %arg2_copy_0_2 = alloca float, align 512
  %arg2_copy_0_3 = alloca float, align 512
  %arg2_copy_1_0 = alloca float, align 512
  %arg2_copy_1_1 = alloca float, align 512
  %arg2_copy_1_2 = alloca float, align 512
  %arg2_copy_1_3 = alloca float, align 512
  %arg2_copy_2_0 = alloca float, align 512
  %arg2_copy_2_1 = alloca float, align 512
  %arg2_copy_2_2 = alloca float, align 512
  %arg2_copy_2_3 = alloca float, align 512
  %arg2_copy_3_0 = alloca float, align 512
  %arg2_copy_3_1 = alloca float, align 512
  %arg2_copy_3_2 = alloca float, align 512
  %arg2_copy_3_3 = alloca float, align 512
  call void @copy_in([4 x [4 x float]]* nonnull %0, float* nonnull align 512 %arg0_copy_0_0, float* nonnull align 512 %arg0_copy_0_1, float* nonnull align 512 %arg0_copy_0_2, float* nonnull align 512 %arg0_copy_0_3, float* nonnull align 512 %arg0_copy_1_0, float* nonnull align 512 %arg0_copy_1_1, float* nonnull align 512 %arg0_copy_1_2, float* nonnull align 512 %arg0_copy_1_3, float* nonnull align 512 %arg0_copy_2_0, float* nonnull align 512 %arg0_copy_2_1, float* nonnull align 512 %arg0_copy_2_2, float* nonnull align 512 %arg0_copy_2_3, float* nonnull align 512 %arg0_copy_3_0, float* nonnull align 512 %arg0_copy_3_1, float* nonnull align 512 %arg0_copy_3_2, float* nonnull align 512 %arg0_copy_3_3, [4 x [4 x float]]* nonnull %1, float* nonnull align 512 %arg1_copy_0_0, float* nonnull align 512 %arg1_copy_0_1, float* nonnull align 512 %arg1_copy_0_2, float* nonnull align 512 %arg1_copy_0_3, float* nonnull align 512 %arg1_copy_1_0, float* nonnull align 512 %arg1_copy_1_1, float* nonnull align 512 %arg1_copy_1_2, float* nonnull align 512 %arg1_copy_1_3, float* nonnull align 512 %arg1_copy_2_0, float* nonnull align 512 %arg1_copy_2_1, float* nonnull align 512 %arg1_copy_2_2, float* nonnull align 512 %arg1_copy_2_3, float* nonnull align 512 %arg1_copy_3_0, float* nonnull align 512 %arg1_copy_3_1, float* nonnull align 512 %arg1_copy_3_2, float* nonnull align 512 %arg1_copy_3_3, [4 x [4 x float]]* nonnull %2, float* nonnull align 512 %arg2_copy_0_0, float* nonnull align 512 %arg2_copy_0_1, float* nonnull align 512 %arg2_copy_0_2, float* nonnull align 512 %arg2_copy_0_3, float* nonnull align 512 %arg2_copy_1_0, float* nonnull align 512 %arg2_copy_1_1, float* nonnull align 512 %arg2_copy_1_2, float* nonnull align 512 %arg2_copy_1_3, float* nonnull align 512 %arg2_copy_2_0, float* nonnull align 512 %arg2_copy_2_1, float* nonnull align 512 %arg2_copy_2_2, float* nonnull align 512 %arg2_copy_2_3, float* nonnull align 512 %arg2_copy_3_0, float* nonnull align 512 %arg2_copy_3_1, float* nonnull align 512 %arg2_copy_3_2, float* nonnull align 512 %arg2_copy_3_3)
  call void @apatb_matmul_4x4x4_hw(float* %arg0_copy_0_0, float* %arg0_copy_0_1, float* %arg0_copy_0_2, float* %arg0_copy_0_3, float* %arg0_copy_1_0, float* %arg0_copy_1_1, float* %arg0_copy_1_2, float* %arg0_copy_1_3, float* %arg0_copy_2_0, float* %arg0_copy_2_1, float* %arg0_copy_2_2, float* %arg0_copy_2_3, float* %arg0_copy_3_0, float* %arg0_copy_3_1, float* %arg0_copy_3_2, float* %arg0_copy_3_3, float* %arg1_copy_0_0, float* %arg1_copy_0_1, float* %arg1_copy_0_2, float* %arg1_copy_0_3, float* %arg1_copy_1_0, float* %arg1_copy_1_1, float* %arg1_copy_1_2, float* %arg1_copy_1_3, float* %arg1_copy_2_0, float* %arg1_copy_2_1, float* %arg1_copy_2_2, float* %arg1_copy_2_3, float* %arg1_copy_3_0, float* %arg1_copy_3_1, float* %arg1_copy_3_2, float* %arg1_copy_3_3, float* %arg2_copy_0_0, float* %arg2_copy_0_1, float* %arg2_copy_0_2, float* %arg2_copy_0_3, float* %arg2_copy_1_0, float* %arg2_copy_1_1, float* %arg2_copy_1_2, float* %arg2_copy_1_3, float* %arg2_copy_2_0, float* %arg2_copy_2_1, float* %arg2_copy_2_2, float* %arg2_copy_2_3, float* %arg2_copy_3_0, float* %arg2_copy_3_1, float* %arg2_copy_3_2, float* %arg2_copy_3_3)
  call void @copy_back([4 x [4 x float]]* %0, float* %arg0_copy_0_0, float* %arg0_copy_0_1, float* %arg0_copy_0_2, float* %arg0_copy_0_3, float* %arg0_copy_1_0, float* %arg0_copy_1_1, float* %arg0_copy_1_2, float* %arg0_copy_1_3, float* %arg0_copy_2_0, float* %arg0_copy_2_1, float* %arg0_copy_2_2, float* %arg0_copy_2_3, float* %arg0_copy_3_0, float* %arg0_copy_3_1, float* %arg0_copy_3_2, float* %arg0_copy_3_3, [4 x [4 x float]]* %1, float* %arg1_copy_0_0, float* %arg1_copy_0_1, float* %arg1_copy_0_2, float* %arg1_copy_0_3, float* %arg1_copy_1_0, float* %arg1_copy_1_1, float* %arg1_copy_1_2, float* %arg1_copy_1_3, float* %arg1_copy_2_0, float* %arg1_copy_2_1, float* %arg1_copy_2_2, float* %arg1_copy_2_3, float* %arg1_copy_3_0, float* %arg1_copy_3_1, float* %arg1_copy_3_2, float* %arg1_copy_3_3, [4 x [4 x float]]* %2, float* %arg2_copy_0_0, float* %arg2_copy_0_1, float* %arg2_copy_0_2, float* %arg2_copy_0_3, float* %arg2_copy_1_0, float* %arg2_copy_1_1, float* %arg2_copy_1_2, float* %arg2_copy_1_3, float* %arg2_copy_2_0, float* %arg2_copy_2_1, float* %arg2_copy_2_2, float* %arg2_copy_2_3, float* %arg2_copy_3_0, float* %arg2_copy_3_1, float* %arg2_copy_3_2, float* %arg2_copy_3_3)
  ret void
}

; Function Attrs: argmemonly noinline norecurse willreturn
define void @arraycpy_hls.p0a4a4f32([4 x [4 x float]]* "orig.arg.no"="0" %dst, [4 x [4 x float]]* readonly "orig.arg.no"="1" %src, i64 "orig.arg.no"="2" %num) local_unnamed_addr #1 {
entry:
  %0 = icmp eq [4 x [4 x float]]* %src, null
  %1 = icmp eq [4 x [4 x float]]* %dst, null
  %2 = or i1 %1, %0
  br i1 %2, label %ret, label %copy

copy:                                             ; preds = %entry
  %for.loop.cond1 = icmp sgt i64 %num, 0
  br i1 %for.loop.cond1, label %for.loop.lr.ph, label %copy.split

for.loop.lr.ph:                                   ; preds = %copy
  br label %for.loop

for.loop:                                         ; preds = %for.loop, %for.loop.lr.ph
  %for.loop.idx2 = phi i64 [ 0, %for.loop.lr.ph ], [ %for.loop.idx.next, %for.loop ]
  %dst.addr = getelementptr [4 x [4 x float]], [4 x [4 x float]]* %dst, i64 0, i64 %for.loop.idx2
  %src.addr = getelementptr [4 x [4 x float]], [4 x [4 x float]]* %src, i64 0, i64 %for.loop.idx2
  call void @arraycpy_hls.p0a4f32([4 x float]* %dst.addr, [4 x float]* %src.addr, i64 4)
  %for.loop.idx.next = add nuw nsw i64 %for.loop.idx2, 1
  %exitcond = icmp ne i64 %for.loop.idx.next, %num
  br i1 %exitcond, label %for.loop, label %copy.split

copy.split:                                       ; preds = %for.loop, %copy
  br label %ret

ret:                                              ; preds = %copy.split, %entry
  ret void
}

; Function Attrs: argmemonly noinline norecurse willreturn
define void @arraycpy_hls.p0a4f32([4 x float]* "orig.arg.no"="0" %dst, [4 x float]* readonly "orig.arg.no"="1" %src, i64 "orig.arg.no"="2" %num) local_unnamed_addr #1 {
entry:
  %0 = icmp eq [4 x float]* %src, null
  %1 = icmp eq [4 x float]* %dst, null
  %2 = or i1 %1, %0
  br i1 %2, label %ret, label %copy

copy:                                             ; preds = %entry
  %for.loop.cond1 = icmp sgt i64 %num, 0
  br i1 %for.loop.cond1, label %for.loop.lr.ph, label %copy.split

for.loop.lr.ph:                                   ; preds = %copy
  br label %for.loop

for.loop:                                         ; preds = %for.loop, %for.loop.lr.ph
  %for.loop.idx2 = phi i64 [ 0, %for.loop.lr.ph ], [ %for.loop.idx.next, %for.loop ]
  %dst.addr = getelementptr [4 x float], [4 x float]* %dst, i64 0, i64 %for.loop.idx2
  %src.addr = getelementptr [4 x float], [4 x float]* %src, i64 0, i64 %for.loop.idx2
  %3 = load float, float* %src.addr, align 4
  store float %3, float* %dst.addr, align 4
  %for.loop.idx.next = add nuw nsw i64 %for.loop.idx2, 1
  %exitcond = icmp ne i64 %for.loop.idx.next, %num
  br i1 %exitcond, label %for.loop, label %copy.split

copy.split:                                       ; preds = %for.loop, %copy
  br label %ret

ret:                                              ; preds = %copy.split, %entry
  ret void
}

; Function Attrs: argmemonly noinline norecurse willreturn
define void @arraycpy_hls.p0a4f32.5.6(float* "orig.arg.no"="0" "unpacked"="0.0" %dst_0, float* "orig.arg.no"="0" "unpacked"="0.1" %dst_1, float* "orig.arg.no"="0" "unpacked"="0.2" %dst_2, float* "orig.arg.no"="0" "unpacked"="0.3" %dst_3, [4 x float]* readonly "orig.arg.no"="1" %src, i64 "orig.arg.no"="2" %num) #1 {
entry:
  %0 = icmp eq [4 x float]* %src, null
  %1 = icmp eq float* %dst_0, null
  %2 = or i1 %1, %0
  br i1 %2, label %ret, label %copy

copy:                                             ; preds = %entry
  %for.loop.cond1 = icmp sgt i64 %num, 0
  br i1 %for.loop.cond1, label %for.loop.lr.ph, label %copy.split

for.loop.lr.ph:                                   ; preds = %copy
  br label %for.loop

for.loop:                                         ; preds = %dst.addr.exit, %for.loop.lr.ph
  %for.loop.idx2 = phi i64 [ 0, %for.loop.lr.ph ], [ %for.loop.idx.next, %dst.addr.exit ]
  %src.addr = getelementptr [4 x float], [4 x float]* %src, i64 0, i64 %for.loop.idx2
  %3 = load float, float* %src.addr, align 4
  switch i64 %for.loop.idx2, label %dst.addr.exit [
    i64 0, label %dst.addr.case.0
    i64 1, label %dst.addr.case.1
    i64 2, label %dst.addr.case.2
    i64 3, label %dst.addr.case.3
  ]

dst.addr.case.0:                                  ; preds = %for.loop
  store float %3, float* %dst_0, align 4
  br label %dst.addr.exit

dst.addr.case.1:                                  ; preds = %for.loop
  store float %3, float* %dst_1, align 4
  br label %dst.addr.exit

dst.addr.case.2:                                  ; preds = %for.loop
  store float %3, float* %dst_2, align 4
  br label %dst.addr.exit

dst.addr.case.3:                                  ; preds = %for.loop
  store float %3, float* %dst_3, align 4
  br label %dst.addr.exit

dst.addr.exit:                                    ; preds = %dst.addr.case.3, %dst.addr.case.2, %dst.addr.case.1, %dst.addr.case.0, %for.loop
  %for.loop.idx.next = add nuw nsw i64 %for.loop.idx2, 1
  %exitcond = icmp ne i64 %for.loop.idx.next, %num
  br i1 %exitcond, label %for.loop, label %copy.split

copy.split:                                       ; preds = %dst.addr.exit, %copy
  br label %ret

ret:                                              ; preds = %copy.split, %entry
  ret void
}

; Function Attrs: argmemonly noinline norecurse willreturn
define void @arraycpy_hls.p0a4a4f32.4.7(float* "orig.arg.no"="0" "unpacked"="0.0.0" %dst_0_0, float* "orig.arg.no"="0" "unpacked"="0.0.1" %dst_0_1, float* "orig.arg.no"="0" "unpacked"="0.0.2" %dst_0_2, float* "orig.arg.no"="0" "unpacked"="0.0.3" %dst_0_3, float* "orig.arg.no"="0" "unpacked"="0.1.0" %dst_1_0, float* "orig.arg.no"="0" "unpacked"="0.1.1" %dst_1_1, float* "orig.arg.no"="0" "unpacked"="0.1.2" %dst_1_2, float* "orig.arg.no"="0" "unpacked"="0.1.3" %dst_1_3, float* "orig.arg.no"="0" "unpacked"="0.2.0" %dst_2_0, float* "orig.arg.no"="0" "unpacked"="0.2.1" %dst_2_1, float* "orig.arg.no"="0" "unpacked"="0.2.2" %dst_2_2, float* "orig.arg.no"="0" "unpacked"="0.2.3" %dst_2_3, float* "orig.arg.no"="0" "unpacked"="0.3.0" %dst_3_0, float* "orig.arg.no"="0" "unpacked"="0.3.1" %dst_3_1, float* "orig.arg.no"="0" "unpacked"="0.3.2" %dst_3_2, float* "orig.arg.no"="0" "unpacked"="0.3.3" %dst_3_3, [4 x [4 x float]]* readonly "orig.arg.no"="1" %src, i64 "orig.arg.no"="2" %num) #1 {
entry:
  %0 = icmp eq [4 x [4 x float]]* %src, null
  %1 = icmp eq float* %dst_0_0, null
  %2 = or i1 %1, %0
  br i1 %2, label %ret, label %copy

copy:                                             ; preds = %entry
  %for.loop.cond1 = icmp sgt i64 %num, 0
  br i1 %for.loop.cond1, label %for.loop.lr.ph, label %copy.split

for.loop.lr.ph:                                   ; preds = %copy
  br label %for.loop

for.loop:                                         ; preds = %dst.addr.exit, %for.loop.lr.ph
  %for.loop.idx2 = phi i64 [ 0, %for.loop.lr.ph ], [ %for.loop.idx.next, %dst.addr.exit ]
  %src.addr = getelementptr [4 x [4 x float]], [4 x [4 x float]]* %src, i64 0, i64 %for.loop.idx2
  switch i64 %for.loop.idx2, label %dst.addr.default [
    i64 0, label %dst.addr.case.0
    i64 1, label %dst.addr.case.1
    i64 2, label %dst.addr.case.2
    i64 3, label %dst.addr.case.3
  ]

dst.addr.default:                                 ; preds = %for.loop
  call void @arraycpy_hls.p0a4f32.5.6(float* %dst_0_0, float* %dst_0_0, float* %dst_0_0, float* %dst_0_0, [4 x float]* %src.addr, i64 4)
  br label %dst.addr.exit

dst.addr.case.0:                                  ; preds = %for.loop
  call void @arraycpy_hls.p0a4f32.5.6(float* %dst_0_0, float* %dst_0_1, float* %dst_0_2, float* %dst_0_3, [4 x float]* %src.addr, i64 4)
  br label %dst.addr.exit

dst.addr.case.1:                                  ; preds = %for.loop
  call void @arraycpy_hls.p0a4f32.5.6(float* %dst_1_0, float* %dst_1_1, float* %dst_1_2, float* %dst_1_3, [4 x float]* %src.addr, i64 4)
  br label %dst.addr.exit

dst.addr.case.2:                                  ; preds = %for.loop
  call void @arraycpy_hls.p0a4f32.5.6(float* %dst_2_0, float* %dst_2_1, float* %dst_2_2, float* %dst_2_3, [4 x float]* %src.addr, i64 4)
  br label %dst.addr.exit

dst.addr.case.3:                                  ; preds = %for.loop
  call void @arraycpy_hls.p0a4f32.5.6(float* %dst_3_0, float* %dst_3_1, float* %dst_3_2, float* %dst_3_3, [4 x float]* %src.addr, i64 4)
  br label %dst.addr.exit

dst.addr.exit:                                    ; preds = %dst.addr.case.3, %dst.addr.case.2, %dst.addr.case.1, %dst.addr.case.0, %dst.addr.default
  %for.loop.idx.next = add nuw nsw i64 %for.loop.idx2, 1
  %exitcond = icmp ne i64 %for.loop.idx.next, %num
  br i1 %exitcond, label %for.loop, label %copy.split

copy.split:                                       ; preds = %dst.addr.exit, %copy
  br label %ret

ret:                                              ; preds = %copy.split, %entry
  ret void
}

; Function Attrs: argmemonly noinline norecurse willreturn
define internal void @onebyonecpy_hls.p0a4a4f32.3.8(float* noalias align 512 "orig.arg.no"="0" "unpacked"="0.0.0" %dst_0_0, float* noalias align 512 "orig.arg.no"="0" "unpacked"="0.0.1" %dst_0_1, float* noalias align 512 "orig.arg.no"="0" "unpacked"="0.0.2" %dst_0_2, float* noalias align 512 "orig.arg.no"="0" "unpacked"="0.0.3" %dst_0_3, float* noalias align 512 "orig.arg.no"="0" "unpacked"="0.1.0" %dst_1_0, float* noalias align 512 "orig.arg.no"="0" "unpacked"="0.1.1" %dst_1_1, float* noalias align 512 "orig.arg.no"="0" "unpacked"="0.1.2" %dst_1_2, float* noalias align 512 "orig.arg.no"="0" "unpacked"="0.1.3" %dst_1_3, float* noalias align 512 "orig.arg.no"="0" "unpacked"="0.2.0" %dst_2_0, float* noalias align 512 "orig.arg.no"="0" "unpacked"="0.2.1" %dst_2_1, float* noalias align 512 "orig.arg.no"="0" "unpacked"="0.2.2" %dst_2_2, float* noalias align 512 "orig.arg.no"="0" "unpacked"="0.2.3" %dst_2_3, float* noalias align 512 "orig.arg.no"="0" "unpacked"="0.3.0" %dst_3_0, float* noalias align 512 "orig.arg.no"="0" "unpacked"="0.3.1" %dst_3_1, float* noalias align 512 "orig.arg.no"="0" "unpacked"="0.3.2" %dst_3_2, float* noalias align 512 "orig.arg.no"="0" "unpacked"="0.3.3" %dst_3_3, [4 x [4 x float]]* noalias readonly "orig.arg.no"="1" %src) #2 {
entry:
  %0 = icmp eq float* %dst_0_0, null
  %1 = icmp eq [4 x [4 x float]]* %src, null
  %2 = or i1 %0, %1
  br i1 %2, label %ret, label %copy

copy:                                             ; preds = %entry
  call void @arraycpy_hls.p0a4a4f32.4.7(float* nonnull %dst_0_0, float* %dst_0_1, float* %dst_0_2, float* %dst_0_3, float* %dst_1_0, float* %dst_1_1, float* %dst_1_2, float* %dst_1_3, float* %dst_2_0, float* %dst_2_1, float* %dst_2_2, float* %dst_2_3, float* %dst_3_0, float* %dst_3_1, float* %dst_3_2, float* %dst_3_3, [4 x [4 x float]]* nonnull %src, i64 4)
  br label %ret

ret:                                              ; preds = %copy, %entry
  ret void
}

; Function Attrs: argmemonly noinline norecurse willreturn
define internal void @copy_in([4 x [4 x float]]* noalias readonly "orig.arg.no"="0", float* noalias align 512 "orig.arg.no"="1" "unpacked"="1.0.0" %_0_0, float* noalias align 512 "orig.arg.no"="1" "unpacked"="1.0.1" %_0_1, float* noalias align 512 "orig.arg.no"="1" "unpacked"="1.0.2" %_0_2, float* noalias align 512 "orig.arg.no"="1" "unpacked"="1.0.3" %_0_3, float* noalias align 512 "orig.arg.no"="1" "unpacked"="1.1.0" %_1_0, float* noalias align 512 "orig.arg.no"="1" "unpacked"="1.1.1" %_1_1, float* noalias align 512 "orig.arg.no"="1" "unpacked"="1.1.2" %_1_2, float* noalias align 512 "orig.arg.no"="1" "unpacked"="1.1.3" %_1_3, float* noalias align 512 "orig.arg.no"="1" "unpacked"="1.2.0" %_2_0, float* noalias align 512 "orig.arg.no"="1" "unpacked"="1.2.1" %_2_1, float* noalias align 512 "orig.arg.no"="1" "unpacked"="1.2.2" %_2_2, float* noalias align 512 "orig.arg.no"="1" "unpacked"="1.2.3" %_2_3, float* noalias align 512 "orig.arg.no"="1" "unpacked"="1.3.0" %_3_0, float* noalias align 512 "orig.arg.no"="1" "unpacked"="1.3.1" %_3_1, float* noalias align 512 "orig.arg.no"="1" "unpacked"="1.3.2" %_3_2, float* noalias align 512 "orig.arg.no"="1" "unpacked"="1.3.3" %_3_3, [4 x [4 x float]]* noalias readonly "orig.arg.no"="2", float* noalias align 512 "orig.arg.no"="3" "unpacked"="3.0.0" %_0_01, float* noalias align 512 "orig.arg.no"="3" "unpacked"="3.0.1" %_0_12, float* noalias align 512 "orig.arg.no"="3" "unpacked"="3.0.2" %_0_23, float* noalias align 512 "orig.arg.no"="3" "unpacked"="3.0.3" %_0_34, float* noalias align 512 "orig.arg.no"="3" "unpacked"="3.1.0" %_1_05, float* noalias align 512 "orig.arg.no"="3" "unpacked"="3.1.1" %_1_16, float* noalias align 512 "orig.arg.no"="3" "unpacked"="3.1.2" %_1_27, float* noalias align 512 "orig.arg.no"="3" "unpacked"="3.1.3" %_1_38, float* noalias align 512 "orig.arg.no"="3" "unpacked"="3.2.0" %_2_09, float* noalias align 512 "orig.arg.no"="3" "unpacked"="3.2.1" %_2_110, float* noalias align 512 "orig.arg.no"="3" "unpacked"="3.2.2" %_2_211, float* noalias align 512 "orig.arg.no"="3" "unpacked"="3.2.3" %_2_312, float* noalias align 512 "orig.arg.no"="3" "unpacked"="3.3.0" %_3_013, float* noalias align 512 "orig.arg.no"="3" "unpacked"="3.3.1" %_3_114, float* noalias align 512 "orig.arg.no"="3" "unpacked"="3.3.2" %_3_215, float* noalias align 512 "orig.arg.no"="3" "unpacked"="3.3.3" %_3_316, [4 x [4 x float]]* noalias readonly "orig.arg.no"="4", float* noalias align 512 "orig.arg.no"="5" "unpacked"="5.0.0" %_0_017, float* noalias align 512 "orig.arg.no"="5" "unpacked"="5.0.1" %_0_118, float* noalias align 512 "orig.arg.no"="5" "unpacked"="5.0.2" %_0_219, float* noalias align 512 "orig.arg.no"="5" "unpacked"="5.0.3" %_0_320, float* noalias align 512 "orig.arg.no"="5" "unpacked"="5.1.0" %_1_021, float* noalias align 512 "orig.arg.no"="5" "unpacked"="5.1.1" %_1_122, float* noalias align 512 "orig.arg.no"="5" "unpacked"="5.1.2" %_1_223, float* noalias align 512 "orig.arg.no"="5" "unpacked"="5.1.3" %_1_324, float* noalias align 512 "orig.arg.no"="5" "unpacked"="5.2.0" %_2_025, float* noalias align 512 "orig.arg.no"="5" "unpacked"="5.2.1" %_2_126, float* noalias align 512 "orig.arg.no"="5" "unpacked"="5.2.2" %_2_227, float* noalias align 512 "orig.arg.no"="5" "unpacked"="5.2.3" %_2_328, float* noalias align 512 "orig.arg.no"="5" "unpacked"="5.3.0" %_3_029, float* noalias align 512 "orig.arg.no"="5" "unpacked"="5.3.1" %_3_130, float* noalias align 512 "orig.arg.no"="5" "unpacked"="5.3.2" %_3_231, float* noalias align 512 "orig.arg.no"="5" "unpacked"="5.3.3" %_3_332) #3 {
entry:
  call void @onebyonecpy_hls.p0a4a4f32.3.8(float* align 512 %_0_0, float* align 512 %_0_1, float* align 512 %_0_2, float* align 512 %_0_3, float* align 512 %_1_0, float* align 512 %_1_1, float* align 512 %_1_2, float* align 512 %_1_3, float* align 512 %_2_0, float* align 512 %_2_1, float* align 512 %_2_2, float* align 512 %_2_3, float* align 512 %_3_0, float* align 512 %_3_1, float* align 512 %_3_2, float* align 512 %_3_3, [4 x [4 x float]]* %0)
  call void @onebyonecpy_hls.p0a4a4f32.3.8(float* align 512 %_0_01, float* align 512 %_0_12, float* align 512 %_0_23, float* align 512 %_0_34, float* align 512 %_1_05, float* align 512 %_1_16, float* align 512 %_1_27, float* align 512 %_1_38, float* align 512 %_2_09, float* align 512 %_2_110, float* align 512 %_2_211, float* align 512 %_2_312, float* align 512 %_3_013, float* align 512 %_3_114, float* align 512 %_3_215, float* align 512 %_3_316, [4 x [4 x float]]* %1)
  call void @onebyonecpy_hls.p0a4a4f32.3.8(float* align 512 %_0_017, float* align 512 %_0_118, float* align 512 %_0_219, float* align 512 %_0_320, float* align 512 %_1_021, float* align 512 %_1_122, float* align 512 %_1_223, float* align 512 %_1_324, float* align 512 %_2_025, float* align 512 %_2_126, float* align 512 %_2_227, float* align 512 %_2_328, float* align 512 %_3_029, float* align 512 %_3_130, float* align 512 %_3_231, float* align 512 %_3_332, [4 x [4 x float]]* %2)
  ret void
}

; Function Attrs: argmemonly noinline norecurse willreturn
define void @arraycpy_hls.p0a4f32.15.16([4 x float]* "orig.arg.no"="0" %dst, float* readonly "orig.arg.no"="1" "unpacked"="1.0" %src_0, float* readonly "orig.arg.no"="1" "unpacked"="1.1" %src_1, float* readonly "orig.arg.no"="1" "unpacked"="1.2" %src_2, float* readonly "orig.arg.no"="1" "unpacked"="1.3" %src_3, i64 "orig.arg.no"="2" %num) #1 {
entry:
  %0 = icmp eq float* %src_0, null
  %1 = icmp eq [4 x float]* %dst, null
  %2 = or i1 %1, %0
  br i1 %2, label %ret, label %copy

copy:                                             ; preds = %entry
  %for.loop.cond1 = icmp sgt i64 %num, 0
  br i1 %for.loop.cond1, label %for.loop.lr.ph, label %copy.split

for.loop.lr.ph:                                   ; preds = %copy
  br label %for.loop

for.loop:                                         ; preds = %src.addr.exit, %for.loop.lr.ph
  %for.loop.idx2 = phi i64 [ 0, %for.loop.lr.ph ], [ %for.loop.idx.next, %src.addr.exit ]
  %dst.addr = getelementptr [4 x float], [4 x float]* %dst, i64 0, i64 %for.loop.idx2
  switch i64 %for.loop.idx2, label %src.addr.exit [
    i64 0, label %src.addr.case.0
    i64 1, label %src.addr.case.1
    i64 2, label %src.addr.case.2
    i64 3, label %src.addr.case.3
  ]

src.addr.case.0:                                  ; preds = %for.loop
  %_0 = load float, float* %src_0, align 4
  br label %src.addr.exit

src.addr.case.1:                                  ; preds = %for.loop
  %_1 = load float, float* %src_1, align 4
  br label %src.addr.exit

src.addr.case.2:                                  ; preds = %for.loop
  %_2 = load float, float* %src_2, align 4
  br label %src.addr.exit

src.addr.case.3:                                  ; preds = %for.loop
  %_3 = load float, float* %src_3, align 4
  br label %src.addr.exit

src.addr.exit:                                    ; preds = %src.addr.case.3, %src.addr.case.2, %src.addr.case.1, %src.addr.case.0, %for.loop
  %3 = phi float [ %_0, %src.addr.case.0 ], [ %_1, %src.addr.case.1 ], [ %_2, %src.addr.case.2 ], [ %_3, %src.addr.case.3 ], [ undef, %for.loop ]
  store float %3, float* %dst.addr, align 4
  %for.loop.idx.next = add nuw nsw i64 %for.loop.idx2, 1
  %exitcond = icmp ne i64 %for.loop.idx.next, %num
  br i1 %exitcond, label %for.loop, label %copy.split

copy.split:                                       ; preds = %src.addr.exit, %copy
  br label %ret

ret:                                              ; preds = %copy.split, %entry
  ret void
}

; Function Attrs: argmemonly noinline norecurse willreturn
define void @arraycpy_hls.p0a4a4f32.14.17([4 x [4 x float]]* "orig.arg.no"="0" %dst, float* readonly "orig.arg.no"="1" "unpacked"="1.0.0" %src_0_0, float* readonly "orig.arg.no"="1" "unpacked"="1.0.1" %src_0_1, float* readonly "orig.arg.no"="1" "unpacked"="1.0.2" %src_0_2, float* readonly "orig.arg.no"="1" "unpacked"="1.0.3" %src_0_3, float* readonly "orig.arg.no"="1" "unpacked"="1.1.0" %src_1_0, float* readonly "orig.arg.no"="1" "unpacked"="1.1.1" %src_1_1, float* readonly "orig.arg.no"="1" "unpacked"="1.1.2" %src_1_2, float* readonly "orig.arg.no"="1" "unpacked"="1.1.3" %src_1_3, float* readonly "orig.arg.no"="1" "unpacked"="1.2.0" %src_2_0, float* readonly "orig.arg.no"="1" "unpacked"="1.2.1" %src_2_1, float* readonly "orig.arg.no"="1" "unpacked"="1.2.2" %src_2_2, float* readonly "orig.arg.no"="1" "unpacked"="1.2.3" %src_2_3, float* readonly "orig.arg.no"="1" "unpacked"="1.3.0" %src_3_0, float* readonly "orig.arg.no"="1" "unpacked"="1.3.1" %src_3_1, float* readonly "orig.arg.no"="1" "unpacked"="1.3.2" %src_3_2, float* readonly "orig.arg.no"="1" "unpacked"="1.3.3" %src_3_3, i64 "orig.arg.no"="2" %num) #1 {
entry:
  %0 = icmp eq float* %src_0_0, null
  %1 = icmp eq [4 x [4 x float]]* %dst, null
  %2 = or i1 %1, %0
  br i1 %2, label %ret, label %copy

copy:                                             ; preds = %entry
  %for.loop.cond1 = icmp sgt i64 %num, 0
  br i1 %for.loop.cond1, label %for.loop.lr.ph, label %copy.split

for.loop.lr.ph:                                   ; preds = %copy
  br label %for.loop

for.loop:                                         ; preds = %src.addr.exit, %for.loop.lr.ph
  %for.loop.idx2 = phi i64 [ 0, %for.loop.lr.ph ], [ %for.loop.idx.next, %src.addr.exit ]
  %dst.addr = getelementptr [4 x [4 x float]], [4 x [4 x float]]* %dst, i64 0, i64 %for.loop.idx2
  switch i64 %for.loop.idx2, label %src.addr.default [
    i64 0, label %src.addr.case.0
    i64 1, label %src.addr.case.1
    i64 2, label %src.addr.case.2
    i64 3, label %src.addr.case.3
  ]

src.addr.default:                                 ; preds = %for.loop
  call void @arraycpy_hls.p0a4f32.15.16([4 x float]* %dst.addr, float* %src_0_0, float* %src_0_0, float* %src_0_0, float* %src_0_0, i64 4)
  br label %src.addr.exit

src.addr.case.0:                                  ; preds = %for.loop
  call void @arraycpy_hls.p0a4f32.15.16([4 x float]* %dst.addr, float* %src_0_0, float* %src_0_1, float* %src_0_2, float* %src_0_3, i64 4)
  br label %src.addr.exit

src.addr.case.1:                                  ; preds = %for.loop
  call void @arraycpy_hls.p0a4f32.15.16([4 x float]* %dst.addr, float* %src_1_0, float* %src_1_1, float* %src_1_2, float* %src_1_3, i64 4)
  br label %src.addr.exit

src.addr.case.2:                                  ; preds = %for.loop
  call void @arraycpy_hls.p0a4f32.15.16([4 x float]* %dst.addr, float* %src_2_0, float* %src_2_1, float* %src_2_2, float* %src_2_3, i64 4)
  br label %src.addr.exit

src.addr.case.3:                                  ; preds = %for.loop
  call void @arraycpy_hls.p0a4f32.15.16([4 x float]* %dst.addr, float* %src_3_0, float* %src_3_1, float* %src_3_2, float* %src_3_3, i64 4)
  br label %src.addr.exit

src.addr.exit:                                    ; preds = %src.addr.case.3, %src.addr.case.2, %src.addr.case.1, %src.addr.case.0, %src.addr.default
  %for.loop.idx.next = add nuw nsw i64 %for.loop.idx2, 1
  %exitcond = icmp ne i64 %for.loop.idx.next, %num
  br i1 %exitcond, label %for.loop, label %copy.split

copy.split:                                       ; preds = %src.addr.exit, %copy
  br label %ret

ret:                                              ; preds = %copy.split, %entry
  ret void
}

; Function Attrs: argmemonly noinline norecurse willreturn
define internal void @onebyonecpy_hls.p0a4a4f32.13.18([4 x [4 x float]]* noalias "orig.arg.no"="0" %dst, float* noalias readonly align 512 "orig.arg.no"="1" "unpacked"="1.0.0" %src_0_0, float* noalias readonly align 512 "orig.arg.no"="1" "unpacked"="1.0.1" %src_0_1, float* noalias readonly align 512 "orig.arg.no"="1" "unpacked"="1.0.2" %src_0_2, float* noalias readonly align 512 "orig.arg.no"="1" "unpacked"="1.0.3" %src_0_3, float* noalias readonly align 512 "orig.arg.no"="1" "unpacked"="1.1.0" %src_1_0, float* noalias readonly align 512 "orig.arg.no"="1" "unpacked"="1.1.1" %src_1_1, float* noalias readonly align 512 "orig.arg.no"="1" "unpacked"="1.1.2" %src_1_2, float* noalias readonly align 512 "orig.arg.no"="1" "unpacked"="1.1.3" %src_1_3, float* noalias readonly align 512 "orig.arg.no"="1" "unpacked"="1.2.0" %src_2_0, float* noalias readonly align 512 "orig.arg.no"="1" "unpacked"="1.2.1" %src_2_1, float* noalias readonly align 512 "orig.arg.no"="1" "unpacked"="1.2.2" %src_2_2, float* noalias readonly align 512 "orig.arg.no"="1" "unpacked"="1.2.3" %src_2_3, float* noalias readonly align 512 "orig.arg.no"="1" "unpacked"="1.3.0" %src_3_0, float* noalias readonly align 512 "orig.arg.no"="1" "unpacked"="1.3.1" %src_3_1, float* noalias readonly align 512 "orig.arg.no"="1" "unpacked"="1.3.2" %src_3_2, float* noalias readonly align 512 "orig.arg.no"="1" "unpacked"="1.3.3" %src_3_3) #2 {
entry:
  %0 = icmp eq [4 x [4 x float]]* %dst, null
  %1 = icmp eq float* %src_0_0, null
  %2 = or i1 %0, %1
  br i1 %2, label %ret, label %copy

copy:                                             ; preds = %entry
  call void @arraycpy_hls.p0a4a4f32.14.17([4 x [4 x float]]* nonnull %dst, float* nonnull %src_0_0, float* %src_0_1, float* %src_0_2, float* %src_0_3, float* %src_1_0, float* %src_1_1, float* %src_1_2, float* %src_1_3, float* %src_2_0, float* %src_2_1, float* %src_2_2, float* %src_2_3, float* %src_3_0, float* %src_3_1, float* %src_3_2, float* %src_3_3, i64 4)
  br label %ret

ret:                                              ; preds = %copy, %entry
  ret void
}

; Function Attrs: argmemonly noinline norecurse willreturn
define internal void @copy_out([4 x [4 x float]]* noalias "orig.arg.no"="0", float* noalias readonly align 512 "orig.arg.no"="1" "unpacked"="1.0.0" %_0_0, float* noalias readonly align 512 "orig.arg.no"="1" "unpacked"="1.0.1" %_0_1, float* noalias readonly align 512 "orig.arg.no"="1" "unpacked"="1.0.2" %_0_2, float* noalias readonly align 512 "orig.arg.no"="1" "unpacked"="1.0.3" %_0_3, float* noalias readonly align 512 "orig.arg.no"="1" "unpacked"="1.1.0" %_1_0, float* noalias readonly align 512 "orig.arg.no"="1" "unpacked"="1.1.1" %_1_1, float* noalias readonly align 512 "orig.arg.no"="1" "unpacked"="1.1.2" %_1_2, float* noalias readonly align 512 "orig.arg.no"="1" "unpacked"="1.1.3" %_1_3, float* noalias readonly align 512 "orig.arg.no"="1" "unpacked"="1.2.0" %_2_0, float* noalias readonly align 512 "orig.arg.no"="1" "unpacked"="1.2.1" %_2_1, float* noalias readonly align 512 "orig.arg.no"="1" "unpacked"="1.2.2" %_2_2, float* noalias readonly align 512 "orig.arg.no"="1" "unpacked"="1.2.3" %_2_3, float* noalias readonly align 512 "orig.arg.no"="1" "unpacked"="1.3.0" %_3_0, float* noalias readonly align 512 "orig.arg.no"="1" "unpacked"="1.3.1" %_3_1, float* noalias readonly align 512 "orig.arg.no"="1" "unpacked"="1.3.2" %_3_2, float* noalias readonly align 512 "orig.arg.no"="1" "unpacked"="1.3.3" %_3_3, [4 x [4 x float]]* noalias "orig.arg.no"="2", float* noalias readonly align 512 "orig.arg.no"="3" "unpacked"="3.0.0" %_0_01, float* noalias readonly align 512 "orig.arg.no"="3" "unpacked"="3.0.1" %_0_12, float* noalias readonly align 512 "orig.arg.no"="3" "unpacked"="3.0.2" %_0_23, float* noalias readonly align 512 "orig.arg.no"="3" "unpacked"="3.0.3" %_0_34, float* noalias readonly align 512 "orig.arg.no"="3" "unpacked"="3.1.0" %_1_05, float* noalias readonly align 512 "orig.arg.no"="3" "unpacked"="3.1.1" %_1_16, float* noalias readonly align 512 "orig.arg.no"="3" "unpacked"="3.1.2" %_1_27, float* noalias readonly align 512 "orig.arg.no"="3" "unpacked"="3.1.3" %_1_38, float* noalias readonly align 512 "orig.arg.no"="3" "unpacked"="3.2.0" %_2_09, float* noalias readonly align 512 "orig.arg.no"="3" "unpacked"="3.2.1" %_2_110, float* noalias readonly align 512 "orig.arg.no"="3" "unpacked"="3.2.2" %_2_211, float* noalias readonly align 512 "orig.arg.no"="3" "unpacked"="3.2.3" %_2_312, float* noalias readonly align 512 "orig.arg.no"="3" "unpacked"="3.3.0" %_3_013, float* noalias readonly align 512 "orig.arg.no"="3" "unpacked"="3.3.1" %_3_114, float* noalias readonly align 512 "orig.arg.no"="3" "unpacked"="3.3.2" %_3_215, float* noalias readonly align 512 "orig.arg.no"="3" "unpacked"="3.3.3" %_3_316, [4 x [4 x float]]* noalias "orig.arg.no"="4", float* noalias readonly align 512 "orig.arg.no"="5" "unpacked"="5.0.0" %_0_017, float* noalias readonly align 512 "orig.arg.no"="5" "unpacked"="5.0.1" %_0_118, float* noalias readonly align 512 "orig.arg.no"="5" "unpacked"="5.0.2" %_0_219, float* noalias readonly align 512 "orig.arg.no"="5" "unpacked"="5.0.3" %_0_320, float* noalias readonly align 512 "orig.arg.no"="5" "unpacked"="5.1.0" %_1_021, float* noalias readonly align 512 "orig.arg.no"="5" "unpacked"="5.1.1" %_1_122, float* noalias readonly align 512 "orig.arg.no"="5" "unpacked"="5.1.2" %_1_223, float* noalias readonly align 512 "orig.arg.no"="5" "unpacked"="5.1.3" %_1_324, float* noalias readonly align 512 "orig.arg.no"="5" "unpacked"="5.2.0" %_2_025, float* noalias readonly align 512 "orig.arg.no"="5" "unpacked"="5.2.1" %_2_126, float* noalias readonly align 512 "orig.arg.no"="5" "unpacked"="5.2.2" %_2_227, float* noalias readonly align 512 "orig.arg.no"="5" "unpacked"="5.2.3" %_2_328, float* noalias readonly align 512 "orig.arg.no"="5" "unpacked"="5.3.0" %_3_029, float* noalias readonly align 512 "orig.arg.no"="5" "unpacked"="5.3.1" %_3_130, float* noalias readonly align 512 "orig.arg.no"="5" "unpacked"="5.3.2" %_3_231, float* noalias readonly align 512 "orig.arg.no"="5" "unpacked"="5.3.3" %_3_332) #4 {
entry:
  call void @onebyonecpy_hls.p0a4a4f32.13.18([4 x [4 x float]]* %0, float* align 512 %_0_0, float* align 512 %_0_1, float* align 512 %_0_2, float* align 512 %_0_3, float* align 512 %_1_0, float* align 512 %_1_1, float* align 512 %_1_2, float* align 512 %_1_3, float* align 512 %_2_0, float* align 512 %_2_1, float* align 512 %_2_2, float* align 512 %_2_3, float* align 512 %_3_0, float* align 512 %_3_1, float* align 512 %_3_2, float* align 512 %_3_3)
  call void @onebyonecpy_hls.p0a4a4f32.13.18([4 x [4 x float]]* %1, float* align 512 %_0_01, float* align 512 %_0_12, float* align 512 %_0_23, float* align 512 %_0_34, float* align 512 %_1_05, float* align 512 %_1_16, float* align 512 %_1_27, float* align 512 %_1_38, float* align 512 %_2_09, float* align 512 %_2_110, float* align 512 %_2_211, float* align 512 %_2_312, float* align 512 %_3_013, float* align 512 %_3_114, float* align 512 %_3_215, float* align 512 %_3_316)
  call void @onebyonecpy_hls.p0a4a4f32.13.18([4 x [4 x float]]* %2, float* align 512 %_0_017, float* align 512 %_0_118, float* align 512 %_0_219, float* align 512 %_0_320, float* align 512 %_1_021, float* align 512 %_1_122, float* align 512 %_1_223, float* align 512 %_1_324, float* align 512 %_2_025, float* align 512 %_2_126, float* align 512 %_2_227, float* align 512 %_2_328, float* align 512 %_3_029, float* align 512 %_3_130, float* align 512 %_3_231, float* align 512 %_3_332)
  ret void
}

declare i8* @malloc(i64)

declare void @free(i8*)

declare void @apatb_matmul_4x4x4_hw(float* %arg0_0_0, float* %arg0_0_1, float* %arg0_0_2, float* %arg0_0_3, float* %arg0_1_0, float* %arg0_1_1, float* %arg0_1_2, float* %arg0_1_3, float* %arg0_2_0, float* %arg0_2_1, float* %arg0_2_2, float* %arg0_2_3, float* %arg0_3_0, float* %arg0_3_1, float* %arg0_3_2, float* %arg0_3_3, float* %arg1_0_0, float* %arg1_0_1, float* %arg1_0_2, float* %arg1_0_3, float* %arg1_1_0, float* %arg1_1_1, float* %arg1_1_2, float* %arg1_1_3, float* %arg1_2_0, float* %arg1_2_1, float* %arg1_2_2, float* %arg1_2_3, float* %arg1_3_0, float* %arg1_3_1, float* %arg1_3_2, float* %arg1_3_3, float* %arg2_0_0, float* %arg2_0_1, float* %arg2_0_2, float* %arg2_0_3, float* %arg2_1_0, float* %arg2_1_1, float* %arg2_1_2, float* %arg2_1_3, float* %arg2_2_0, float* %arg2_2_1, float* %arg2_2_2, float* %arg2_2_3, float* %arg2_3_0, float* %arg2_3_1, float* %arg2_3_2, float* %arg2_3_3)

; Function Attrs: argmemonly noinline norecurse willreturn
define internal void @copy_back([4 x [4 x float]]* noalias "orig.arg.no"="0", float* noalias readonly align 512 "orig.arg.no"="1" "unpacked"="1.0.0" %_0_0, float* noalias readonly align 512 "orig.arg.no"="1" "unpacked"="1.0.1" %_0_1, float* noalias readonly align 512 "orig.arg.no"="1" "unpacked"="1.0.2" %_0_2, float* noalias readonly align 512 "orig.arg.no"="1" "unpacked"="1.0.3" %_0_3, float* noalias readonly align 512 "orig.arg.no"="1" "unpacked"="1.1.0" %_1_0, float* noalias readonly align 512 "orig.arg.no"="1" "unpacked"="1.1.1" %_1_1, float* noalias readonly align 512 "orig.arg.no"="1" "unpacked"="1.1.2" %_1_2, float* noalias readonly align 512 "orig.arg.no"="1" "unpacked"="1.1.3" %_1_3, float* noalias readonly align 512 "orig.arg.no"="1" "unpacked"="1.2.0" %_2_0, float* noalias readonly align 512 "orig.arg.no"="1" "unpacked"="1.2.1" %_2_1, float* noalias readonly align 512 "orig.arg.no"="1" "unpacked"="1.2.2" %_2_2, float* noalias readonly align 512 "orig.arg.no"="1" "unpacked"="1.2.3" %_2_3, float* noalias readonly align 512 "orig.arg.no"="1" "unpacked"="1.3.0" %_3_0, float* noalias readonly align 512 "orig.arg.no"="1" "unpacked"="1.3.1" %_3_1, float* noalias readonly align 512 "orig.arg.no"="1" "unpacked"="1.3.2" %_3_2, float* noalias readonly align 512 "orig.arg.no"="1" "unpacked"="1.3.3" %_3_3, [4 x [4 x float]]* noalias "orig.arg.no"="2", float* noalias readonly align 512 "orig.arg.no"="3" "unpacked"="3.0.0" %_0_01, float* noalias readonly align 512 "orig.arg.no"="3" "unpacked"="3.0.1" %_0_12, float* noalias readonly align 512 "orig.arg.no"="3" "unpacked"="3.0.2" %_0_23, float* noalias readonly align 512 "orig.arg.no"="3" "unpacked"="3.0.3" %_0_34, float* noalias readonly align 512 "orig.arg.no"="3" "unpacked"="3.1.0" %_1_05, float* noalias readonly align 512 "orig.arg.no"="3" "unpacked"="3.1.1" %_1_16, float* noalias readonly align 512 "orig.arg.no"="3" "unpacked"="3.1.2" %_1_27, float* noalias readonly align 512 "orig.arg.no"="3" "unpacked"="3.1.3" %_1_38, float* noalias readonly align 512 "orig.arg.no"="3" "unpacked"="3.2.0" %_2_09, float* noalias readonly align 512 "orig.arg.no"="3" "unpacked"="3.2.1" %_2_110, float* noalias readonly align 512 "orig.arg.no"="3" "unpacked"="3.2.2" %_2_211, float* noalias readonly align 512 "orig.arg.no"="3" "unpacked"="3.2.3" %_2_312, float* noalias readonly align 512 "orig.arg.no"="3" "unpacked"="3.3.0" %_3_013, float* noalias readonly align 512 "orig.arg.no"="3" "unpacked"="3.3.1" %_3_114, float* noalias readonly align 512 "orig.arg.no"="3" "unpacked"="3.3.2" %_3_215, float* noalias readonly align 512 "orig.arg.no"="3" "unpacked"="3.3.3" %_3_316, [4 x [4 x float]]* noalias "orig.arg.no"="4", float* noalias readonly align 512 "orig.arg.no"="5" "unpacked"="5.0.0" %_0_017, float* noalias readonly align 512 "orig.arg.no"="5" "unpacked"="5.0.1" %_0_118, float* noalias readonly align 512 "orig.arg.no"="5" "unpacked"="5.0.2" %_0_219, float* noalias readonly align 512 "orig.arg.no"="5" "unpacked"="5.0.3" %_0_320, float* noalias readonly align 512 "orig.arg.no"="5" "unpacked"="5.1.0" %_1_021, float* noalias readonly align 512 "orig.arg.no"="5" "unpacked"="5.1.1" %_1_122, float* noalias readonly align 512 "orig.arg.no"="5" "unpacked"="5.1.2" %_1_223, float* noalias readonly align 512 "orig.arg.no"="5" "unpacked"="5.1.3" %_1_324, float* noalias readonly align 512 "orig.arg.no"="5" "unpacked"="5.2.0" %_2_025, float* noalias readonly align 512 "orig.arg.no"="5" "unpacked"="5.2.1" %_2_126, float* noalias readonly align 512 "orig.arg.no"="5" "unpacked"="5.2.2" %_2_227, float* noalias readonly align 512 "orig.arg.no"="5" "unpacked"="5.2.3" %_2_328, float* noalias readonly align 512 "orig.arg.no"="5" "unpacked"="5.3.0" %_3_029, float* noalias readonly align 512 "orig.arg.no"="5" "unpacked"="5.3.1" %_3_130, float* noalias readonly align 512 "orig.arg.no"="5" "unpacked"="5.3.2" %_3_231, float* noalias readonly align 512 "orig.arg.no"="5" "unpacked"="5.3.3" %_3_332) #4 {
entry:
  call void @onebyonecpy_hls.p0a4a4f32.13.18([4 x [4 x float]]* %2, float* align 512 %_0_017, float* align 512 %_0_118, float* align 512 %_0_219, float* align 512 %_0_320, float* align 512 %_1_021, float* align 512 %_1_122, float* align 512 %_1_223, float* align 512 %_1_324, float* align 512 %_2_025, float* align 512 %_2_126, float* align 512 %_2_227, float* align 512 %_2_328, float* align 512 %_3_029, float* align 512 %_3_130, float* align 512 %_3_231, float* align 512 %_3_332)
  ret void
}

declare void @matmul_4x4x4_hw_stub([4 x float]* noalias nocapture nonnull readonly, [4 x float]* noalias nocapture nonnull readonly, [4 x float]* noalias nocapture nonnull)

define void @matmul_4x4x4_hw_stub_wrapper(float* %arg0_0_0, float* %arg0_0_1, float* %arg0_0_2, float* %arg0_0_3, float* %arg0_1_0, float* %arg0_1_1, float* %arg0_1_2, float* %arg0_1_3, float* %arg0_2_0, float* %arg0_2_1, float* %arg0_2_2, float* %arg0_2_3, float* %arg0_3_0, float* %arg0_3_1, float* %arg0_3_2, float* %arg0_3_3, float* %arg1_0_0, float* %arg1_0_1, float* %arg1_0_2, float* %arg1_0_3, float* %arg1_1_0, float* %arg1_1_1, float* %arg1_1_2, float* %arg1_1_3, float* %arg1_2_0, float* %arg1_2_1, float* %arg1_2_2, float* %arg1_2_3, float* %arg1_3_0, float* %arg1_3_1, float* %arg1_3_2, float* %arg1_3_3, float* %arg2_0_0, float* %arg2_0_1, float* %arg2_0_2, float* %arg2_0_3, float* %arg2_1_0, float* %arg2_1_1, float* %arg2_1_2, float* %arg2_1_3, float* %arg2_2_0, float* %arg2_2_1, float* %arg2_2_2, float* %arg2_2_3, float* %arg2_3_0, float* %arg2_3_1, float* %arg2_3_2, float* %arg2_3_3) #5 {
entry:
  %0 = call i8* @malloc(i64 64)
  %1 = bitcast i8* %0 to [4 x [4 x float]]*
  %2 = call i8* @malloc(i64 64)
  %3 = bitcast i8* %2 to [4 x [4 x float]]*
  %4 = call i8* @malloc(i64 64)
  %5 = bitcast i8* %4 to [4 x [4 x float]]*
  call void @copy_out([4 x [4 x float]]* %1, float* %arg0_0_0, float* %arg0_0_1, float* %arg0_0_2, float* %arg0_0_3, float* %arg0_1_0, float* %arg0_1_1, float* %arg0_1_2, float* %arg0_1_3, float* %arg0_2_0, float* %arg0_2_1, float* %arg0_2_2, float* %arg0_2_3, float* %arg0_3_0, float* %arg0_3_1, float* %arg0_3_2, float* %arg0_3_3, [4 x [4 x float]]* %3, float* %arg1_0_0, float* %arg1_0_1, float* %arg1_0_2, float* %arg1_0_3, float* %arg1_1_0, float* %arg1_1_1, float* %arg1_1_2, float* %arg1_1_3, float* %arg1_2_0, float* %arg1_2_1, float* %arg1_2_2, float* %arg1_2_3, float* %arg1_3_0, float* %arg1_3_1, float* %arg1_3_2, float* %arg1_3_3, [4 x [4 x float]]* %5, float* %arg2_0_0, float* %arg2_0_1, float* %arg2_0_2, float* %arg2_0_3, float* %arg2_1_0, float* %arg2_1_1, float* %arg2_1_2, float* %arg2_1_3, float* %arg2_2_0, float* %arg2_2_1, float* %arg2_2_2, float* %arg2_2_3, float* %arg2_3_0, float* %arg2_3_1, float* %arg2_3_2, float* %arg2_3_3)
  %6 = bitcast [4 x [4 x float]]* %1 to [4 x float]*
  %7 = bitcast [4 x [4 x float]]* %3 to [4 x float]*
  %8 = bitcast [4 x [4 x float]]* %5 to [4 x float]*
  call void @matmul_4x4x4_hw_stub([4 x float]* %6, [4 x float]* %7, [4 x float]* %8)
  call void @copy_in([4 x [4 x float]]* %1, float* %arg0_0_0, float* %arg0_0_1, float* %arg0_0_2, float* %arg0_0_3, float* %arg0_1_0, float* %arg0_1_1, float* %arg0_1_2, float* %arg0_1_3, float* %arg0_2_0, float* %arg0_2_1, float* %arg0_2_2, float* %arg0_2_3, float* %arg0_3_0, float* %arg0_3_1, float* %arg0_3_2, float* %arg0_3_3, [4 x [4 x float]]* %3, float* %arg1_0_0, float* %arg1_0_1, float* %arg1_0_2, float* %arg1_0_3, float* %arg1_1_0, float* %arg1_1_1, float* %arg1_1_2, float* %arg1_1_3, float* %arg1_2_0, float* %arg1_2_1, float* %arg1_2_2, float* %arg1_2_3, float* %arg1_3_0, float* %arg1_3_1, float* %arg1_3_2, float* %arg1_3_3, [4 x [4 x float]]* %5, float* %arg2_0_0, float* %arg2_0_1, float* %arg2_0_2, float* %arg2_0_3, float* %arg2_1_0, float* %arg2_1_1, float* %arg2_1_2, float* %arg2_1_3, float* %arg2_2_0, float* %arg2_2_1, float* %arg2_2_2, float* %arg2_2_3, float* %arg2_3_0, float* %arg2_3_1, float* %arg2_3_2, float* %arg2_3_3)
  call void @free(i8* %0)
  call void @free(i8* %2)
  call void @free(i8* %4)
  ret void
}

attributes #0 = { inaccessiblemem_or_argmemonly noinline willreturn "fpga.wrapper.func"="wrapper" }
attributes #1 = { argmemonly noinline norecurse willreturn "fpga.wrapper.func"="arraycpy_hls" }
attributes #2 = { argmemonly noinline norecurse willreturn "fpga.wrapper.func"="onebyonecpy_hls" }
attributes #3 = { argmemonly noinline norecurse willreturn "fpga.wrapper.func"="copyin" }
attributes #4 = { argmemonly noinline norecurse willreturn "fpga.wrapper.func"="copyout" }
attributes #5 = { "fpga.wrapper.func"="stub" }

!llvm.dbg.cu = !{}
!llvm.ident = !{!0, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1, !1}
!llvm.module.flags = !{!2, !3, !4}
!blackbox_cfg = !{!5}
!datalayout.transforms.on.top = !{!6, !29, !49}

!0 = !{!"AMD/Xilinx clang version 16.0.6"}
!1 = !{!"clang version 7.0.0 "}
!2 = !{i32 2, !"Dwarf Version", i32 4}
!3 = !{i32 2, !"Debug Info Version", i32 3}
!4 = !{i32 1, !"wchar_size", i32 4}
!5 = !{}
!6 = !{!7, !9, !12}
!7 = !{!8}
!8 = !{!"0", [4 x [4 x float]]* null}
!9 = !{!10, !11}
!10 = !{!"array_partition", !"type=Complete", !"dim=1"}
!11 = !{!"array_partition", !"type=Complete", !"dim=2"}
!12 = !{!13, !14, !15, !16, !17, !18, !19, !20, !21, !22, !23, !24, !25, !26, !27, !28}
!13 = !{!"0.0.0", float* null}
!14 = !{!"0.0.1", float* null}
!15 = !{!"0.0.2", float* null}
!16 = !{!"0.0.3", float* null}
!17 = !{!"0.1.0", float* null}
!18 = !{!"0.1.1", float* null}
!19 = !{!"0.1.2", float* null}
!20 = !{!"0.1.3", float* null}
!21 = !{!"0.2.0", float* null}
!22 = !{!"0.2.1", float* null}
!23 = !{!"0.2.2", float* null}
!24 = !{!"0.2.3", float* null}
!25 = !{!"0.3.0", float* null}
!26 = !{!"0.3.1", float* null}
!27 = !{!"0.3.2", float* null}
!28 = !{!"0.3.3", float* null}
!29 = !{!30, !9, !32}
!30 = !{!31}
!31 = !{!"1", [4 x [4 x float]]* null}
!32 = !{!33, !34, !35, !36, !37, !38, !39, !40, !41, !42, !43, !44, !45, !46, !47, !48}
!33 = !{!"1.0.0", float* null}
!34 = !{!"1.0.1", float* null}
!35 = !{!"1.0.2", float* null}
!36 = !{!"1.0.3", float* null}
!37 = !{!"1.1.0", float* null}
!38 = !{!"1.1.1", float* null}
!39 = !{!"1.1.2", float* null}
!40 = !{!"1.1.3", float* null}
!41 = !{!"1.2.0", float* null}
!42 = !{!"1.2.1", float* null}
!43 = !{!"1.2.2", float* null}
!44 = !{!"1.2.3", float* null}
!45 = !{!"1.3.0", float* null}
!46 = !{!"1.3.1", float* null}
!47 = !{!"1.3.2", float* null}
!48 = !{!"1.3.3", float* null}
!49 = !{!50, !9, !52}
!50 = !{!51}
!51 = !{!"2", [4 x [4 x float]]* null}
!52 = !{!53, !54, !55, !56, !57, !58, !59, !60, !61, !62, !63, !64, !65, !66, !67, !68}
!53 = !{!"2.0.0", float* null}
!54 = !{!"2.0.1", float* null}
!55 = !{!"2.0.2", float* null}
!56 = !{!"2.0.3", float* null}
!57 = !{!"2.1.0", float* null}
!58 = !{!"2.1.1", float* null}
!59 = !{!"2.1.2", float* null}
!60 = !{!"2.1.3", float* null}
!61 = !{!"2.2.0", float* null}
!62 = !{!"2.2.1", float* null}
!63 = !{!"2.2.2", float* null}
!64 = !{!"2.2.3", float* null}
!65 = !{!"2.3.0", float* null}
!66 = !{!"2.3.1", float* null}
!67 = !{!"2.3.2", float* null}
!68 = !{!"2.3.3", float* null}
