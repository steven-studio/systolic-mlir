; ModuleID = '/home/steven-studio/work/systolic-mlir-polynomial-summation/test/polynomial-summation/llvm/observed/frontend.ll'
source_filename = "/home/steven-studio/work/systolic-mlir-polynomial-summation/test/polynomial-summation/llvm/recurrences.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: noinline nounwind uwtable
define dso_local i64 @sum_p1(i64 noundef %n) #0 {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %s.0 = phi i64 [ 0, %entry ], [ %add1, %for.inc ]
  %t.0 = phi i64 [ 0, %entry ], [ %inc, %for.inc ]
  %cmp = icmp ult i64 %t.0, %n
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %add = add i64 %t.0, 1
  %add1 = add i64 %s.0, %add
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %inc = add i64 %t.0, 1
  br label %for.cond, !llvm.loop !5

for.end:                                          ; preds = %for.cond
  %s.0.lcssa = phi i64 [ %s.0, %for.cond ]
  ret i64 %s.0.lcssa
}

; Function Attrs: noinline nounwind uwtable
define dso_local i64 @sum_p2(i64 noundef %n) #0 {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %s.0 = phi i64 [ 0, %entry ], [ %add1, %for.inc ]
  %t.0 = phi i64 [ 0, %entry ], [ %inc, %for.inc ]
  %cmp = icmp ult i64 %t.0, %n
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %add = add i64 %t.0, 1
  %mul = mul i64 %add, %add
  %add1 = add i64 %s.0, %mul
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %inc = add i64 %t.0, 1
  br label %for.cond, !llvm.loop !7

for.end:                                          ; preds = %for.cond
  %s.0.lcssa = phi i64 [ %s.0, %for.cond ]
  ret i64 %s.0.lcssa
}

; Function Attrs: noinline nounwind uwtable
define dso_local i64 @sum_p3(i64 noundef %n) #0 {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %s.0 = phi i64 [ 0, %entry ], [ %add2, %for.inc ]
  %t.0 = phi i64 [ 0, %entry ], [ %inc, %for.inc ]
  %cmp = icmp ult i64 %t.0, %n
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %add = add i64 %t.0, 1
  %mul = mul i64 %add, %add
  %mul1 = mul i64 %mul, %add
  %add2 = add i64 %s.0, %mul1
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %inc = add i64 %t.0, 1
  br label %for.cond, !llvm.loop !8

for.end:                                          ; preds = %for.cond
  %s.0.lcssa = phi i64 [ %s.0, %for.cond ]
  ret i64 %s.0.lcssa
}

; Function Attrs: noinline nounwind uwtable
define dso_local i64 @sum_p4(i64 noundef %n) #0 {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %s.0 = phi i64 [ 0, %entry ], [ %add3, %for.inc ]
  %t.0 = phi i64 [ 0, %entry ], [ %inc, %for.inc ]
  %cmp = icmp ult i64 %t.0, %n
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %add = add i64 %t.0, 1
  %mul = mul i64 %add, %add
  %mul1 = mul i64 %mul, %add
  %mul2 = mul i64 %mul1, %add
  %add3 = add i64 %s.0, %mul2
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %inc = add i64 %t.0, 1
  br label %for.cond, !llvm.loop !9

for.end:                                          ; preds = %for.cond
  %s.0.lcssa = phi i64 [ %s.0, %for.cond ]
  ret i64 %s.0.lcssa
}

; Function Attrs: noinline nounwind uwtable
define dso_local i64 @sum_p5(i64 noundef %n) #0 {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %s.0 = phi i64 [ 0, %entry ], [ %add4, %for.inc ]
  %t.0 = phi i64 [ 0, %entry ], [ %inc, %for.inc ]
  %cmp = icmp ult i64 %t.0, %n
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %add = add i64 %t.0, 1
  %mul = mul i64 %add, %add
  %mul1 = mul i64 %mul, %add
  %mul2 = mul i64 %mul1, %add
  %mul3 = mul i64 %mul2, %add
  %add4 = add i64 %s.0, %mul3
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %inc = add i64 %t.0, 1
  br label %for.cond, !llvm.loop !10

for.end:                                          ; preds = %for.cond
  %s.0.lcssa = phi i64 [ %s.0, %for.cond ]
  ret i64 %s.0.lcssa
}

; Function Attrs: noinline nounwind uwtable
define dso_local i64 @sum_p6(i64 noundef %n) #0 {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %s.0 = phi i64 [ 0, %entry ], [ %add5, %for.inc ]
  %t.0 = phi i64 [ 0, %entry ], [ %inc, %for.inc ]
  %cmp = icmp ult i64 %t.0, %n
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %add = add i64 %t.0, 1
  %mul = mul i64 %add, %add
  %mul1 = mul i64 %mul, %add
  %mul2 = mul i64 %mul1, %add
  %mul3 = mul i64 %mul2, %add
  %mul4 = mul i64 %mul3, %add
  %add5 = add i64 %s.0, %mul4
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %inc = add i64 %t.0, 1
  br label %for.cond, !llvm.loop !11

for.end:                                          ; preds = %for.cond
  %s.0.lcssa = phi i64 [ %s.0, %for.cond ]
  ret i64 %s.0.lcssa
}

; Function Attrs: noinline nounwind uwtable
define dso_local i64 @sum_p7(i64 noundef %n) #0 {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %s.0 = phi i64 [ 0, %entry ], [ %add6, %for.inc ]
  %t.0 = phi i64 [ 0, %entry ], [ %inc, %for.inc ]
  %cmp = icmp ult i64 %t.0, %n
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %add = add i64 %t.0, 1
  %mul = mul i64 %add, %add
  %mul1 = mul i64 %mul, %add
  %mul2 = mul i64 %mul1, %add
  %mul3 = mul i64 %mul2, %add
  %mul4 = mul i64 %mul3, %add
  %mul5 = mul i64 %mul4, %add
  %add6 = add i64 %s.0, %mul5
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %inc = add i64 %t.0, 1
  br label %for.cond, !llvm.loop !12

for.end:                                          ; preds = %for.cond
  %s.0.lcssa = phi i64 [ %s.0, %for.cond ]
  ret i64 %s.0.lcssa
}

; Function Attrs: noinline nounwind uwtable
define dso_local i64 @sum_p8(i64 noundef %n) #0 {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %s.0 = phi i64 [ 0, %entry ], [ %add7, %for.inc ]
  %t.0 = phi i64 [ 0, %entry ], [ %inc, %for.inc ]
  %cmp = icmp ult i64 %t.0, %n
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %add = add i64 %t.0, 1
  %mul = mul i64 %add, %add
  %mul1 = mul i64 %mul, %add
  %mul2 = mul i64 %mul1, %add
  %mul3 = mul i64 %mul2, %add
  %mul4 = mul i64 %mul3, %add
  %mul5 = mul i64 %mul4, %add
  %mul6 = mul i64 %mul5, %add
  %add7 = add i64 %s.0, %mul6
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %inc = add i64 %t.0, 1
  br label %for.cond, !llvm.loop !13

for.end:                                          ; preds = %for.cond
  %s.0.lcssa = phi i64 [ %s.0, %for.cond ]
  ret i64 %s.0.lcssa
}

; Function Attrs: noinline nounwind uwtable
define dso_local i64 @general_cubic(i64 noundef %n) #0 {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %s.0 = phi i64 [ 0, %entry ], [ %add8, %for.inc ]
  %t.0 = phi i64 [ 0, %entry ], [ %inc, %for.inc ]
  %cmp = icmp ult i64 %t.0, %n
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %add = add i64 %t.0, 1
  %mul = mul i64 7, %add
  %mul1 = mul i64 %mul, %add
  %mul2 = mul i64 %mul1, %add
  %mul3 = mul i64 3, %add
  %mul4 = mul i64 %mul3, %add
  %sub = sub i64 %mul2, %mul4
  %mul5 = mul i64 11, %add
  %add6 = add i64 %sub, %mul5
  %sub7 = sub i64 %add6, 17
  %add8 = add i64 %s.0, %sub7
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %inc = add i64 %t.0, 1
  br label %for.cond, !llvm.loop !14

for.end:                                          ; preds = %for.cond
  %s.0.lcssa = phi i64 [ %s.0, %for.cond ]
  ret i64 %s.0.lcssa
}

; Function Attrs: noinline nounwind uwtable
define dso_local i64 @shifted_square(i64 noundef %n, i64 noundef %start, i64 noundef %step, i64 noundef %initial) #0 {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %i.0 = phi i64 [ %start, %entry ], [ %add1, %for.inc ]
  %s.0 = phi i64 [ %initial, %entry ], [ %add, %for.inc ]
  %t.0 = phi i64 [ 0, %entry ], [ %inc, %for.inc ]
  %cmp = icmp ult i64 %t.0, %n
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %mul = mul i64 %i.0, %i.0
  %add = add i64 %s.0, %mul
  %add1 = add i64 %i.0, %step
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %inc = add i64 %t.0, 1
  br label %for.cond, !llvm.loop !15

for.end:                                          ; preds = %for.cond
  %s.0.lcssa = phi i64 [ %s.0, %for.cond ]
  ret i64 %s.0.lcssa
}

; Function Attrs: noinline nounwind uwtable
define dso_local i64 @triangular(i64 noundef %n) #0 {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %z.0 = phi i64 [ 0, %entry ], [ %add, %for.inc ]
  %y.0 = phi i64 [ 0, %entry ], [ %add1, %for.inc ]
  %x.0 = phi i64 [ 0, %entry ], [ %add2, %for.inc ]
  %t.0 = phi i64 [ 0, %entry ], [ %inc, %for.inc ]
  %cmp = icmp ult i64 %t.0, %n
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %add = add i64 %z.0, %y.0
  %mul = mul i64 %x.0, %x.0
  %add1 = add i64 %y.0, %mul
  %add2 = add i64 %x.0, 1
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %inc = add i64 %t.0, 1
  br label %for.cond, !llvm.loop !16

for.end:                                          ; preds = %for.cond
  %z.0.lcssa = phi i64 [ %z.0, %for.cond ]
  ret i64 %z.0.lcssa
}

; Function Attrs: noinline nounwind uwtable
define dso_local i64 @nonlinear_self(i64 noundef %n) #0 {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %x.0 = phi i64 [ 2, %entry ], [ %add, %for.inc ]
  %t.0 = phi i64 [ 0, %entry ], [ %inc, %for.inc ]
  %cmp = icmp ult i64 %t.0, %n
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %mul = mul i64 %x.0, %x.0
  %add = add i64 %mul, 1
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %inc = add i64 %t.0, 1
  br label %for.cond, !llvm.loop !17

for.end:                                          ; preds = %for.cond
  %x.0.lcssa = phi i64 [ %x.0, %for.cond ]
  ret i64 %x.0.lcssa
}

; Function Attrs: noinline nounwind uwtable
define dso_local i64 @exponential(i64 noundef %n) #0 {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %x.0 = phi i64 [ 1, %entry ], [ %mul, %for.inc ]
  %t.0 = phi i64 [ 0, %entry ], [ %inc, %for.inc ]
  %cmp = icmp ult i64 %t.0, %n
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %mul = mul i64 %x.0, 2
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %inc = add i64 %t.0, 1
  br label %for.cond, !llvm.loop !18

for.end:                                          ; preds = %for.cond
  %x.0.lcssa = phi i64 [ %x.0, %for.cond ]
  ret i64 %x.0.lcssa
}

; Function Attrs: noinline nounwind uwtable
define dso_local i64 @unknown_exit(i64 noundef %n) #0 {
entry:
  br label %while.cond

while.cond:                                       ; preds = %while.body, %entry
  %x.0 = phi i64 [ 2, %entry ], [ %add, %while.body ]
  %cmp = icmp ult i64 %x.0, %n
  br i1 %cmp, label %while.body, label %while.end

while.body:                                       ; preds = %while.cond
  %mul = mul i64 %x.0, %x.0
  %add = add i64 %mul, 1
  br label %while.cond, !llvm.loop !19

while.end:                                        ; preds = %while.cond
  %x.0.lcssa = phi i64 [ %x.0, %while.cond ]
  ret i64 %x.0.lcssa
}

; Function Attrs: noinline nounwind uwtable
define dso_local i64 @fixed_square() #0 {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %i.0 = phi i64 [ 1, %entry ], [ %inc, %for.inc ]
  %s.0 = phi i64 [ 0, %entry ], [ %add, %for.inc ]
  %cmp = icmp ule i64 %i.0, 10
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %mul = mul i64 %i.0, %i.0
  %add = add i64 %s.0, %mul
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %inc = add i64 %i.0, 1
  br label %for.cond, !llvm.loop !20

for.end:                                          ; preds = %for.cond
  %s.0.lcssa = phi i64 [ %s.0, %for.cond ]
  ret i64 %s.0.lcssa
}

; Function Attrs: noinline nounwind uwtable
define dso_local i64 @observable_square(i64 noundef %n, ptr noundef %out) #0 {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %s.0 = phi i64 [ 0, %entry ], [ %add1, %for.inc ]
  %t.0 = phi i64 [ 0, %entry ], [ %inc, %for.inc ]
  %cmp = icmp ult i64 %t.0, %n
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %add = add i64 %t.0, 1
  %mul = mul i64 %add, %add
  %add1 = add i64 %s.0, %mul
  store volatile i64 %add1, ptr %out, align 8
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %inc = add i64 %t.0, 1
  br label %for.cond, !llvm.loop !21

for.end:                                          ; preds = %for.cond
  %s.0.lcssa = phi i64 [ %s.0, %for.cond ]
  ret i64 %s.0.lcssa
}

attributes #0 = { noinline nounwind uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }

!llvm.module.flags = !{!0, !1, !2, !3}
!llvm.ident = !{!4}

!0 = !{i32 8, !"PIC Level", i32 2}
!1 = !{i32 7, !"PIE Level", i32 2}
!2 = !{i32 7, !"uwtable", i32 2}
!3 = !{i32 7, !"frame-pointer", i32 2}
!4 = !{!"clang version 24.0.0git (https://github.com/llvm/llvm-project.git 7615da10b75c57286a4032eac8bb43c1b8d2ba46)"}
!5 = distinct !{!5, !6}
!6 = !{!"llvm.loop.mustprogress"}
!7 = distinct !{!7, !6}
!8 = distinct !{!8, !6}
!9 = distinct !{!9, !6}
!10 = distinct !{!10, !6}
!11 = distinct !{!11, !6}
!12 = distinct !{!12, !6}
!13 = distinct !{!13, !6}
!14 = distinct !{!14, !6}
!15 = distinct !{!15, !6}
!16 = distinct !{!16, !6}
!17 = distinct !{!17, !6}
!18 = distinct !{!18, !6}
!19 = distinct !{!19, !6}
!20 = distinct !{!20, !6}
!21 = distinct !{!21, !6}
