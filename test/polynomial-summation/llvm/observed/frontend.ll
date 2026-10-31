; ModuleID = '/home/steven-studio/work/systolic-mlir-polynomial-summation/test/polynomial-summation/llvm/recurrences.c'
source_filename = "/home/steven-studio/work/systolic-mlir-polynomial-summation/test/polynomial-summation/llvm/recurrences.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: noinline nounwind uwtable
define dso_local i64 @sum_p1(i64 noundef %n) #0 {
entry:
  %n.addr = alloca i64, align 8
  %s = alloca i64, align 8
  %t = alloca i64, align 8
  %i = alloca i64, align 8
  store i64 %n, ptr %n.addr, align 8
  store i64 0, ptr %s, align 8
  store i64 0, ptr %t, align 8
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %0 = load i64, ptr %t, align 8
  %1 = load i64, ptr %n.addr, align 8
  %cmp = icmp ult i64 %0, %1
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %2 = load i64, ptr %t, align 8
  %add = add i64 %2, 1
  store i64 %add, ptr %i, align 8
  %3 = load i64, ptr %i, align 8
  %4 = load i64, ptr %s, align 8
  %add1 = add i64 %4, %3
  store i64 %add1, ptr %s, align 8
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %5 = load i64, ptr %t, align 8
  %inc = add i64 %5, 1
  store i64 %inc, ptr %t, align 8
  br label %for.cond, !llvm.loop !5

for.end:                                          ; preds = %for.cond
  %6 = load i64, ptr %s, align 8
  ret i64 %6
}

; Function Attrs: noinline nounwind uwtable
define dso_local i64 @sum_p2(i64 noundef %n) #0 {
entry:
  %n.addr = alloca i64, align 8
  %s = alloca i64, align 8
  %t = alloca i64, align 8
  %i = alloca i64, align 8
  store i64 %n, ptr %n.addr, align 8
  store i64 0, ptr %s, align 8
  store i64 0, ptr %t, align 8
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %0 = load i64, ptr %t, align 8
  %1 = load i64, ptr %n.addr, align 8
  %cmp = icmp ult i64 %0, %1
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %2 = load i64, ptr %t, align 8
  %add = add i64 %2, 1
  store i64 %add, ptr %i, align 8
  %3 = load i64, ptr %i, align 8
  %4 = load i64, ptr %i, align 8
  %mul = mul i64 %3, %4
  %5 = load i64, ptr %s, align 8
  %add1 = add i64 %5, %mul
  store i64 %add1, ptr %s, align 8
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %6 = load i64, ptr %t, align 8
  %inc = add i64 %6, 1
  store i64 %inc, ptr %t, align 8
  br label %for.cond, !llvm.loop !7

for.end:                                          ; preds = %for.cond
  %7 = load i64, ptr %s, align 8
  ret i64 %7
}

; Function Attrs: noinline nounwind uwtable
define dso_local i64 @sum_p3(i64 noundef %n) #0 {
entry:
  %n.addr = alloca i64, align 8
  %s = alloca i64, align 8
  %t = alloca i64, align 8
  %i = alloca i64, align 8
  store i64 %n, ptr %n.addr, align 8
  store i64 0, ptr %s, align 8
  store i64 0, ptr %t, align 8
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %0 = load i64, ptr %t, align 8
  %1 = load i64, ptr %n.addr, align 8
  %cmp = icmp ult i64 %0, %1
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %2 = load i64, ptr %t, align 8
  %add = add i64 %2, 1
  store i64 %add, ptr %i, align 8
  %3 = load i64, ptr %i, align 8
  %4 = load i64, ptr %i, align 8
  %mul = mul i64 %3, %4
  %5 = load i64, ptr %i, align 8
  %mul1 = mul i64 %mul, %5
  %6 = load i64, ptr %s, align 8
  %add2 = add i64 %6, %mul1
  store i64 %add2, ptr %s, align 8
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %7 = load i64, ptr %t, align 8
  %inc = add i64 %7, 1
  store i64 %inc, ptr %t, align 8
  br label %for.cond, !llvm.loop !8

for.end:                                          ; preds = %for.cond
  %8 = load i64, ptr %s, align 8
  ret i64 %8
}

; Function Attrs: noinline nounwind uwtable
define dso_local i64 @sum_p4(i64 noundef %n) #0 {
entry:
  %n.addr = alloca i64, align 8
  %s = alloca i64, align 8
  %t = alloca i64, align 8
  %i = alloca i64, align 8
  store i64 %n, ptr %n.addr, align 8
  store i64 0, ptr %s, align 8
  store i64 0, ptr %t, align 8
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %0 = load i64, ptr %t, align 8
  %1 = load i64, ptr %n.addr, align 8
  %cmp = icmp ult i64 %0, %1
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %2 = load i64, ptr %t, align 8
  %add = add i64 %2, 1
  store i64 %add, ptr %i, align 8
  %3 = load i64, ptr %i, align 8
  %4 = load i64, ptr %i, align 8
  %mul = mul i64 %3, %4
  %5 = load i64, ptr %i, align 8
  %mul1 = mul i64 %mul, %5
  %6 = load i64, ptr %i, align 8
  %mul2 = mul i64 %mul1, %6
  %7 = load i64, ptr %s, align 8
  %add3 = add i64 %7, %mul2
  store i64 %add3, ptr %s, align 8
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %8 = load i64, ptr %t, align 8
  %inc = add i64 %8, 1
  store i64 %inc, ptr %t, align 8
  br label %for.cond, !llvm.loop !9

for.end:                                          ; preds = %for.cond
  %9 = load i64, ptr %s, align 8
  ret i64 %9
}

; Function Attrs: noinline nounwind uwtable
define dso_local i64 @sum_p5(i64 noundef %n) #0 {
entry:
  %n.addr = alloca i64, align 8
  %s = alloca i64, align 8
  %t = alloca i64, align 8
  %i = alloca i64, align 8
  store i64 %n, ptr %n.addr, align 8
  store i64 0, ptr %s, align 8
  store i64 0, ptr %t, align 8
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %0 = load i64, ptr %t, align 8
  %1 = load i64, ptr %n.addr, align 8
  %cmp = icmp ult i64 %0, %1
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %2 = load i64, ptr %t, align 8
  %add = add i64 %2, 1
  store i64 %add, ptr %i, align 8
  %3 = load i64, ptr %i, align 8
  %4 = load i64, ptr %i, align 8
  %mul = mul i64 %3, %4
  %5 = load i64, ptr %i, align 8
  %mul1 = mul i64 %mul, %5
  %6 = load i64, ptr %i, align 8
  %mul2 = mul i64 %mul1, %6
  %7 = load i64, ptr %i, align 8
  %mul3 = mul i64 %mul2, %7
  %8 = load i64, ptr %s, align 8
  %add4 = add i64 %8, %mul3
  store i64 %add4, ptr %s, align 8
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %9 = load i64, ptr %t, align 8
  %inc = add i64 %9, 1
  store i64 %inc, ptr %t, align 8
  br label %for.cond, !llvm.loop !10

for.end:                                          ; preds = %for.cond
  %10 = load i64, ptr %s, align 8
  ret i64 %10
}

; Function Attrs: noinline nounwind uwtable
define dso_local i64 @sum_p6(i64 noundef %n) #0 {
entry:
  %n.addr = alloca i64, align 8
  %s = alloca i64, align 8
  %t = alloca i64, align 8
  %i = alloca i64, align 8
  store i64 %n, ptr %n.addr, align 8
  store i64 0, ptr %s, align 8
  store i64 0, ptr %t, align 8
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %0 = load i64, ptr %t, align 8
  %1 = load i64, ptr %n.addr, align 8
  %cmp = icmp ult i64 %0, %1
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %2 = load i64, ptr %t, align 8
  %add = add i64 %2, 1
  store i64 %add, ptr %i, align 8
  %3 = load i64, ptr %i, align 8
  %4 = load i64, ptr %i, align 8
  %mul = mul i64 %3, %4
  %5 = load i64, ptr %i, align 8
  %mul1 = mul i64 %mul, %5
  %6 = load i64, ptr %i, align 8
  %mul2 = mul i64 %mul1, %6
  %7 = load i64, ptr %i, align 8
  %mul3 = mul i64 %mul2, %7
  %8 = load i64, ptr %i, align 8
  %mul4 = mul i64 %mul3, %8
  %9 = load i64, ptr %s, align 8
  %add5 = add i64 %9, %mul4
  store i64 %add5, ptr %s, align 8
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %10 = load i64, ptr %t, align 8
  %inc = add i64 %10, 1
  store i64 %inc, ptr %t, align 8
  br label %for.cond, !llvm.loop !11

for.end:                                          ; preds = %for.cond
  %11 = load i64, ptr %s, align 8
  ret i64 %11
}

; Function Attrs: noinline nounwind uwtable
define dso_local i64 @sum_p7(i64 noundef %n) #0 {
entry:
  %n.addr = alloca i64, align 8
  %s = alloca i64, align 8
  %t = alloca i64, align 8
  %i = alloca i64, align 8
  store i64 %n, ptr %n.addr, align 8
  store i64 0, ptr %s, align 8
  store i64 0, ptr %t, align 8
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %0 = load i64, ptr %t, align 8
  %1 = load i64, ptr %n.addr, align 8
  %cmp = icmp ult i64 %0, %1
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %2 = load i64, ptr %t, align 8
  %add = add i64 %2, 1
  store i64 %add, ptr %i, align 8
  %3 = load i64, ptr %i, align 8
  %4 = load i64, ptr %i, align 8
  %mul = mul i64 %3, %4
  %5 = load i64, ptr %i, align 8
  %mul1 = mul i64 %mul, %5
  %6 = load i64, ptr %i, align 8
  %mul2 = mul i64 %mul1, %6
  %7 = load i64, ptr %i, align 8
  %mul3 = mul i64 %mul2, %7
  %8 = load i64, ptr %i, align 8
  %mul4 = mul i64 %mul3, %8
  %9 = load i64, ptr %i, align 8
  %mul5 = mul i64 %mul4, %9
  %10 = load i64, ptr %s, align 8
  %add6 = add i64 %10, %mul5
  store i64 %add6, ptr %s, align 8
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %11 = load i64, ptr %t, align 8
  %inc = add i64 %11, 1
  store i64 %inc, ptr %t, align 8
  br label %for.cond, !llvm.loop !12

for.end:                                          ; preds = %for.cond
  %12 = load i64, ptr %s, align 8
  ret i64 %12
}

; Function Attrs: noinline nounwind uwtable
define dso_local i64 @sum_p8(i64 noundef %n) #0 {
entry:
  %n.addr = alloca i64, align 8
  %s = alloca i64, align 8
  %t = alloca i64, align 8
  %i = alloca i64, align 8
  store i64 %n, ptr %n.addr, align 8
  store i64 0, ptr %s, align 8
  store i64 0, ptr %t, align 8
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %0 = load i64, ptr %t, align 8
  %1 = load i64, ptr %n.addr, align 8
  %cmp = icmp ult i64 %0, %1
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %2 = load i64, ptr %t, align 8
  %add = add i64 %2, 1
  store i64 %add, ptr %i, align 8
  %3 = load i64, ptr %i, align 8
  %4 = load i64, ptr %i, align 8
  %mul = mul i64 %3, %4
  %5 = load i64, ptr %i, align 8
  %mul1 = mul i64 %mul, %5
  %6 = load i64, ptr %i, align 8
  %mul2 = mul i64 %mul1, %6
  %7 = load i64, ptr %i, align 8
  %mul3 = mul i64 %mul2, %7
  %8 = load i64, ptr %i, align 8
  %mul4 = mul i64 %mul3, %8
  %9 = load i64, ptr %i, align 8
  %mul5 = mul i64 %mul4, %9
  %10 = load i64, ptr %i, align 8
  %mul6 = mul i64 %mul5, %10
  %11 = load i64, ptr %s, align 8
  %add7 = add i64 %11, %mul6
  store i64 %add7, ptr %s, align 8
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %12 = load i64, ptr %t, align 8
  %inc = add i64 %12, 1
  store i64 %inc, ptr %t, align 8
  br label %for.cond, !llvm.loop !13

for.end:                                          ; preds = %for.cond
  %13 = load i64, ptr %s, align 8
  ret i64 %13
}

; Function Attrs: noinline nounwind uwtable
define dso_local i64 @general_cubic(i64 noundef %n) #0 {
entry:
  %n.addr = alloca i64, align 8
  %s = alloca i64, align 8
  %t = alloca i64, align 8
  %i = alloca i64, align 8
  store i64 %n, ptr %n.addr, align 8
  store i64 0, ptr %s, align 8
  store i64 0, ptr %t, align 8
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %0 = load i64, ptr %t, align 8
  %1 = load i64, ptr %n.addr, align 8
  %cmp = icmp ult i64 %0, %1
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %2 = load i64, ptr %t, align 8
  %add = add i64 %2, 1
  store i64 %add, ptr %i, align 8
  %3 = load i64, ptr %i, align 8
  %mul = mul i64 7, %3
  %4 = load i64, ptr %i, align 8
  %mul1 = mul i64 %mul, %4
  %5 = load i64, ptr %i, align 8
  %mul2 = mul i64 %mul1, %5
  %6 = load i64, ptr %i, align 8
  %mul3 = mul i64 3, %6
  %7 = load i64, ptr %i, align 8
  %mul4 = mul i64 %mul3, %7
  %sub = sub i64 %mul2, %mul4
  %8 = load i64, ptr %i, align 8
  %mul5 = mul i64 11, %8
  %add6 = add i64 %sub, %mul5
  %sub7 = sub i64 %add6, 17
  %9 = load i64, ptr %s, align 8
  %add8 = add i64 %9, %sub7
  store i64 %add8, ptr %s, align 8
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %10 = load i64, ptr %t, align 8
  %inc = add i64 %10, 1
  store i64 %inc, ptr %t, align 8
  br label %for.cond, !llvm.loop !14

for.end:                                          ; preds = %for.cond
  %11 = load i64, ptr %s, align 8
  ret i64 %11
}

; Function Attrs: noinline nounwind uwtable
define dso_local i64 @shifted_square(i64 noundef %n, i64 noundef %start, i64 noundef %step, i64 noundef %initial) #0 {
entry:
  %n.addr = alloca i64, align 8
  %start.addr = alloca i64, align 8
  %step.addr = alloca i64, align 8
  %initial.addr = alloca i64, align 8
  %i = alloca i64, align 8
  %s = alloca i64, align 8
  %t = alloca i64, align 8
  store i64 %n, ptr %n.addr, align 8
  store i64 %start, ptr %start.addr, align 8
  store i64 %step, ptr %step.addr, align 8
  store i64 %initial, ptr %initial.addr, align 8
  %0 = load i64, ptr %start.addr, align 8
  store i64 %0, ptr %i, align 8
  %1 = load i64, ptr %initial.addr, align 8
  store i64 %1, ptr %s, align 8
  store i64 0, ptr %t, align 8
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %2 = load i64, ptr %t, align 8
  %3 = load i64, ptr %n.addr, align 8
  %cmp = icmp ult i64 %2, %3
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %4 = load i64, ptr %i, align 8
  %5 = load i64, ptr %i, align 8
  %mul = mul i64 %4, %5
  %6 = load i64, ptr %s, align 8
  %add = add i64 %6, %mul
  store i64 %add, ptr %s, align 8
  %7 = load i64, ptr %step.addr, align 8
  %8 = load i64, ptr %i, align 8
  %add1 = add i64 %8, %7
  store i64 %add1, ptr %i, align 8
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %9 = load i64, ptr %t, align 8
  %inc = add i64 %9, 1
  store i64 %inc, ptr %t, align 8
  br label %for.cond, !llvm.loop !15

for.end:                                          ; preds = %for.cond
  %10 = load i64, ptr %s, align 8
  ret i64 %10
}

; Function Attrs: noinline nounwind uwtable
define dso_local i64 @triangular(i64 noundef %n) #0 {
entry:
  %n.addr = alloca i64, align 8
  %x = alloca i64, align 8
  %y = alloca i64, align 8
  %z = alloca i64, align 8
  %t = alloca i64, align 8
  store i64 %n, ptr %n.addr, align 8
  store i64 0, ptr %x, align 8
  store i64 0, ptr %y, align 8
  store i64 0, ptr %z, align 8
  store i64 0, ptr %t, align 8
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %0 = load i64, ptr %t, align 8
  %1 = load i64, ptr %n.addr, align 8
  %cmp = icmp ult i64 %0, %1
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %2 = load i64, ptr %y, align 8
  %3 = load i64, ptr %z, align 8
  %add = add i64 %3, %2
  store i64 %add, ptr %z, align 8
  %4 = load i64, ptr %x, align 8
  %5 = load i64, ptr %x, align 8
  %mul = mul i64 %4, %5
  %6 = load i64, ptr %y, align 8
  %add1 = add i64 %6, %mul
  store i64 %add1, ptr %y, align 8
  %7 = load i64, ptr %x, align 8
  %add2 = add i64 %7, 1
  store i64 %add2, ptr %x, align 8
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %8 = load i64, ptr %t, align 8
  %inc = add i64 %8, 1
  store i64 %inc, ptr %t, align 8
  br label %for.cond, !llvm.loop !16

for.end:                                          ; preds = %for.cond
  %9 = load i64, ptr %z, align 8
  ret i64 %9
}

; Function Attrs: noinline nounwind uwtable
define dso_local i64 @nonlinear_self(i64 noundef %n) #0 {
entry:
  %n.addr = alloca i64, align 8
  %x = alloca i64, align 8
  %t = alloca i64, align 8
  store i64 %n, ptr %n.addr, align 8
  store i64 2, ptr %x, align 8
  store i64 0, ptr %t, align 8
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %0 = load i64, ptr %t, align 8
  %1 = load i64, ptr %n.addr, align 8
  %cmp = icmp ult i64 %0, %1
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %2 = load i64, ptr %x, align 8
  %3 = load i64, ptr %x, align 8
  %mul = mul i64 %2, %3
  %add = add i64 %mul, 1
  store i64 %add, ptr %x, align 8
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %4 = load i64, ptr %t, align 8
  %inc = add i64 %4, 1
  store i64 %inc, ptr %t, align 8
  br label %for.cond, !llvm.loop !17

for.end:                                          ; preds = %for.cond
  %5 = load i64, ptr %x, align 8
  ret i64 %5
}

; Function Attrs: noinline nounwind uwtable
define dso_local i64 @exponential(i64 noundef %n) #0 {
entry:
  %n.addr = alloca i64, align 8
  %x = alloca i64, align 8
  %t = alloca i64, align 8
  store i64 %n, ptr %n.addr, align 8
  store i64 1, ptr %x, align 8
  store i64 0, ptr %t, align 8
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %0 = load i64, ptr %t, align 8
  %1 = load i64, ptr %n.addr, align 8
  %cmp = icmp ult i64 %0, %1
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %2 = load i64, ptr %x, align 8
  %mul = mul i64 %2, 2
  store i64 %mul, ptr %x, align 8
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %3 = load i64, ptr %t, align 8
  %inc = add i64 %3, 1
  store i64 %inc, ptr %t, align 8
  br label %for.cond, !llvm.loop !18

for.end:                                          ; preds = %for.cond
  %4 = load i64, ptr %x, align 8
  ret i64 %4
}

; Function Attrs: noinline nounwind uwtable
define dso_local i64 @unknown_exit(i64 noundef %n) #0 {
entry:
  %n.addr = alloca i64, align 8
  %x = alloca i64, align 8
  store i64 %n, ptr %n.addr, align 8
  store i64 2, ptr %x, align 8
  br label %while.cond

while.cond:                                       ; preds = %while.body, %entry
  %0 = load i64, ptr %x, align 8
  %1 = load i64, ptr %n.addr, align 8
  %cmp = icmp ult i64 %0, %1
  br i1 %cmp, label %while.body, label %while.end

while.body:                                       ; preds = %while.cond
  %2 = load i64, ptr %x, align 8
  %3 = load i64, ptr %x, align 8
  %mul = mul i64 %2, %3
  %add = add i64 %mul, 1
  store i64 %add, ptr %x, align 8
  br label %while.cond, !llvm.loop !19

while.end:                                        ; preds = %while.cond
  %4 = load i64, ptr %x, align 8
  ret i64 %4
}

; Function Attrs: noinline nounwind uwtable
define dso_local i64 @fixed_square() #0 {
entry:
  %s = alloca i64, align 8
  %i = alloca i64, align 8
  store i64 0, ptr %s, align 8
  store i64 1, ptr %i, align 8
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %0 = load i64, ptr %i, align 8
  %cmp = icmp ule i64 %0, 10
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %1 = load i64, ptr %i, align 8
  %2 = load i64, ptr %i, align 8
  %mul = mul i64 %1, %2
  %3 = load i64, ptr %s, align 8
  %add = add i64 %3, %mul
  store i64 %add, ptr %s, align 8
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %4 = load i64, ptr %i, align 8
  %inc = add i64 %4, 1
  store i64 %inc, ptr %i, align 8
  br label %for.cond, !llvm.loop !20

for.end:                                          ; preds = %for.cond
  %5 = load i64, ptr %s, align 8
  ret i64 %5
}

; Function Attrs: noinline nounwind uwtable
define dso_local i64 @observable_square(i64 noundef %n, ptr noundef %out) #0 {
entry:
  %n.addr = alloca i64, align 8
  %out.addr = alloca ptr, align 8
  %s = alloca i64, align 8
  %t = alloca i64, align 8
  %i = alloca i64, align 8
  store i64 %n, ptr %n.addr, align 8
  store ptr %out, ptr %out.addr, align 8
  store i64 0, ptr %s, align 8
  store i64 0, ptr %t, align 8
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %0 = load i64, ptr %t, align 8
  %1 = load i64, ptr %n.addr, align 8
  %cmp = icmp ult i64 %0, %1
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %2 = load i64, ptr %t, align 8
  %add = add i64 %2, 1
  store i64 %add, ptr %i, align 8
  %3 = load i64, ptr %i, align 8
  %4 = load i64, ptr %i, align 8
  %mul = mul i64 %3, %4
  %5 = load i64, ptr %s, align 8
  %add1 = add i64 %5, %mul
  store i64 %add1, ptr %s, align 8
  %6 = load i64, ptr %s, align 8
  %7 = load ptr, ptr %out.addr, align 8
  store volatile i64 %6, ptr %7, align 8
  br label %for.inc

for.inc:                                          ; preds = %for.body
  %8 = load i64, ptr %t, align 8
  %inc = add i64 %8, 1
  store i64 %inc, ptr %t, align 8
  br label %for.cond, !llvm.loop !21

for.end:                                          ; preds = %for.cond
  %9 = load i64, ptr %s, align 8
  ret i64 %9
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
