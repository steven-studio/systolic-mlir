; ModuleID = '/home/steven-studio/work/systolic-mlir-polynomial-summation/test/polynomial-summation/llvm/observed/canonical.ll'
source_filename = "/home/steven-studio/work/systolic-mlir-polynomial-summation/test/polynomial-summation/llvm/recurrences.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: mustprogress nofree noinline norecurse nosync nounwind willreturn memory(none) uwtable
define dso_local i64 @sum_p1(i64 noundef %n) local_unnamed_addr #0 {
entry:
  %cmp1.not = icmp eq i64 %n, 0
  br i1 %cmp1.not, label %for.end, label %for.body.preheader

for.body.preheader:                               ; preds = %entry
  %0 = shl i64 %n, 1
  %1 = add i64 %n, -1
  %2 = zext i64 %1 to i65
  %3 = add i64 %n, -2
  %4 = zext i64 %3 to i65
  %5 = mul i65 %2, %4
  %6 = lshr i65 %5, 1
  %7 = trunc nuw i65 %6 to i64
  %8 = add i64 %0, %7
  %9 = add i64 %8, -1
  br label %for.end

for.end:                                          ; preds = %for.body.preheader, %entry
  %s.0.lcssa = phi i64 [ 0, %entry ], [ %9, %for.body.preheader ]
  ret i64 %s.0.lcssa
}

; Function Attrs: mustprogress nofree noinline norecurse nosync nounwind willreturn memory(none) uwtable
define dso_local i64 @sum_p2(i64 noundef %n) local_unnamed_addr #0 {
entry:
  %cmp1.not = icmp eq i64 %n, 0
  br i1 %cmp1.not, label %for.end, label %for.body.preheader

for.body.preheader:                               ; preds = %entry
  %0 = add i64 %n, -1
  %1 = zext i64 %0 to i65
  %2 = add i64 %n, -2
  %3 = zext i64 %2 to i65
  %4 = mul i65 %1, %3
  %5 = add i64 %n, -3
  %6 = zext i64 %5 to i65
  %7 = mul i65 %4, %6
  %8 = lshr i65 %7, 1
  %9 = trunc nuw i65 %8 to i64
  %10 = mul i64 %9, 6148914691236517206
  %11 = lshr i65 %4, 1
  %12 = trunc nuw i65 %11 to i64
  %13 = mul i64 %12, 5
  %14 = add i64 %10, %13
  %15 = shl i64 %n, 2
  %16 = add i64 %14, %15
  %17 = add i64 %16, -3
  br label %for.end

for.end:                                          ; preds = %for.body.preheader, %entry
  %s.0.lcssa = phi i64 [ 0, %entry ], [ %17, %for.body.preheader ]
  ret i64 %s.0.lcssa
}

; Function Attrs: mustprogress nofree noinline norecurse nosync nounwind willreturn memory(none) uwtable
define dso_local i64 @sum_p3(i64 noundef %n) local_unnamed_addr #0 {
entry:
  %cmp1.not = icmp eq i64 %n, 0
  br i1 %cmp1.not, label %for.end, label %for.body.preheader

for.body.preheader:                               ; preds = %entry
  %0 = add i64 %n, -1
  %1 = zext i64 %0 to i65
  %2 = add i64 %n, -2
  %3 = zext i64 %2 to i65
  %4 = mul i65 %1, %3
  %5 = lshr i65 %4, 1
  %6 = trunc nuw i65 %5 to i64
  %7 = mul i64 %6, 19
  %8 = shl i64 %n, 3
  %9 = add i64 %7, %8
  %10 = add i64 %n, -3
  %11 = zext i64 %10 to i65
  %12 = mul i65 %4, %11
  %13 = lshr i65 %12, 1
  %14 = trunc nuw i65 %13 to i64
  %15 = mul i64 %14, 6
  %16 = add i64 %9, %15
  %17 = zext i64 %0 to i67
  %18 = zext i64 %2 to i67
  %19 = mul i67 %17, %18
  %20 = zext i64 %10 to i67
  %21 = mul i67 %19, %20
  %22 = add i64 %n, -4
  %23 = zext i64 %22 to i67
  %24 = mul i67 %21, %23
  %sh.diff = lshr i67 %24, 2
  %tr.sh.diff = trunc i67 %sh.diff to i64
  %25 = and i64 %tr.sh.diff, -2
  %26 = add i64 %16, %25
  %27 = add i64 %26, -7
  br label %for.end

for.end:                                          ; preds = %for.body.preheader, %entry
  %s.0.lcssa = phi i64 [ 0, %entry ], [ %27, %for.body.preheader ]
  ret i64 %s.0.lcssa
}

; Function Attrs: mustprogress nofree noinline norecurse nosync nounwind willreturn memory(none) uwtable
define dso_local i64 @sum_p4(i64 noundef %n) local_unnamed_addr #0 {
entry:
  %cmp1.not = icmp eq i64 %n, 0
  br i1 %cmp1.not, label %for.end, label %for.body.preheader

for.body.preheader:                               ; preds = %entry
  %0 = add i64 %n, -1
  %1 = zext i64 %0 to i67
  %2 = add i64 %n, -2
  %3 = zext i64 %2 to i67
  %4 = mul i67 %1, %3
  %5 = add i64 %n, -3
  %6 = zext i64 %5 to i67
  %7 = mul i67 %4, %6
  %8 = add i64 %n, -4
  %9 = zext i64 %8 to i67
  %10 = mul i67 %7, %9
  %11 = add i64 %n, -5
  %12 = zext i64 %11 to i67
  %13 = mul i67 %10, %12
  %14 = lshr i67 %13, 3
  %15 = trunc nuw i67 %14 to i64
  %16 = mul i64 %15, 7378697629483820648
  %17 = zext i64 %0 to i65
  %18 = zext i64 %2 to i65
  %19 = mul i65 %17, %18
  %20 = zext i64 %5 to i65
  %21 = mul i65 %19, %20
  %22 = lshr i65 %21, 1
  %23 = trunc nuw i65 %22 to i64
  %24 = mul i64 %23, 6148914691236517242
  %25 = add i64 %16, %24
  %26 = lshr i65 %19, 1
  %27 = trunc nuw i65 %26 to i64
  %28 = mul i64 %27, 65
  %29 = add i64 %25, %28
  %30 = lshr i67 %10, 3
  %31 = trunc nuw i67 %30 to i64
  %32 = mul i64 %31, 28
  %33 = add i64 %29, %32
  %34 = shl i64 %n, 4
  %35 = add i64 %33, %34
  %36 = add i64 %35, -15
  br label %for.end

for.end:                                          ; preds = %for.body.preheader, %entry
  %s.0.lcssa = phi i64 [ 0, %entry ], [ %36, %for.body.preheader ]
  ret i64 %s.0.lcssa
}

; Function Attrs: mustprogress nofree noinline norecurse nosync nounwind willreturn memory(none) uwtable
define dso_local i64 @sum_p5(i64 noundef %n) local_unnamed_addr #0 {
entry:
  %cmp1.not = icmp eq i64 %n, 0
  br i1 %cmp1.not, label %for.end, label %for.body.preheader

for.body.preheader:                               ; preds = %entry
  %0 = add i64 %n, -1
  %1 = zext i64 %0 to i68
  %2 = add i64 %n, -2
  %3 = zext i64 %2 to i68
  %4 = mul i68 %1, %3
  %5 = add i64 %n, -3
  %6 = zext i64 %5 to i68
  %7 = mul i68 %4, %6
  %8 = add i64 %n, -4
  %9 = zext i64 %8 to i68
  %10 = mul i68 %7, %9
  %11 = add i64 %n, -5
  %12 = zext i64 %11 to i68
  %13 = mul i68 %10, %12
  %14 = add i64 %n, -6
  %15 = zext i64 %14 to i68
  %16 = mul i68 %13, %15
  %17 = lshr i68 %16, 4
  %18 = trunc nuw i68 %17 to i64
  %19 = mul i64 %18, 6148914691236517208
  %20 = zext i64 %0 to i67
  %21 = zext i64 %2 to i67
  %22 = mul i67 %20, %21
  %23 = zext i64 %5 to i67
  %24 = mul i67 %22, %23
  %25 = zext i64 %8 to i67
  %26 = mul i67 %24, %25
  %27 = lshr i67 %26, 3
  %28 = trunc nuw i67 %27 to i64
  %29 = mul i64 %28, 250
  %30 = add i64 %19, %29
  %31 = zext i64 %0 to i65
  %32 = zext i64 %2 to i65
  %33 = mul i65 %31, %32
  %34 = lshr i65 %33, 1
  %35 = trunc nuw i65 %34 to i64
  %36 = mul i64 %35, 211
  %37 = add i64 %30, %36
  %38 = zext i64 %5 to i65
  %39 = mul i65 %33, %38
  %40 = lshr i65 %39, 1
  %41 = trunc nuw i65 %40 to i64
  %42 = mul i64 %41, 190
  %43 = add i64 %37, %42
  %44 = shl i64 %n, 5
  %45 = add i64 %43, %44
  %46 = trunc i67 %26 to i64
  %47 = mul i64 %11, %46
  %48 = shl i64 %47, 2
  %49 = and i64 %48, -32
  %50 = add i64 %45, %49
  %51 = add i64 %50, -31
  br label %for.end

for.end:                                          ; preds = %for.body.preheader, %entry
  %s.0.lcssa = phi i64 [ 0, %entry ], [ %51, %for.body.preheader ]
  ret i64 %s.0.lcssa
}

; Function Attrs: mustprogress nofree noinline norecurse nosync nounwind willreturn memory(none) uwtable
define dso_local i64 @sum_p6(i64 noundef %n) local_unnamed_addr #0 {
entry:
  %cmp1.not = icmp eq i64 %n, 0
  br i1 %cmp1.not, label %for.end, label %for.body.preheader

for.body.preheader:                               ; preds = %entry
  %0 = add i64 %n, -1
  %1 = zext i64 %0 to i65
  %2 = add i64 %n, -2
  %3 = zext i64 %2 to i65
  %4 = mul i65 %1, %3
  %5 = add i64 %n, -3
  %6 = zext i64 %5 to i65
  %7 = mul i65 %4, %6
  %8 = lshr i65 %7, 1
  %9 = trunc nuw i65 %8 to i64
  %10 = mul i64 %9, 6148914691236518106
  %11 = zext i64 %0 to i67
  %12 = zext i64 %2 to i67
  %13 = mul i67 %11, %12
  %14 = zext i64 %5 to i67
  %15 = mul i67 %13, %14
  %16 = add i64 %n, -4
  %17 = zext i64 %16 to i67
  %18 = mul i67 %15, %17
  %19 = lshr i67 %18, 3
  %20 = trunc nuw i67 %19 to i64
  %21 = mul i64 %20, 1820
  %22 = add i64 %10, %21
  %23 = lshr i65 %4, 1
  %24 = trunc nuw i65 %23 to i64
  %25 = mul i64 %24, 665
  %26 = add i64 %22, %25
  %27 = add i64 %n, -5
  %28 = zext i64 %27 to i67
  %29 = mul i67 %18, %28
  %30 = lshr i67 %29, 3
  %31 = trunc nuw i67 %30 to i64
  %32 = mul i64 %31, 392
  %33 = add i64 %26, %32
  %34 = zext i64 %0 to i68
  %35 = zext i64 %2 to i68
  %36 = mul i68 %34, %35
  %37 = zext i64 %5 to i68
  %38 = mul i68 %36, %37
  %39 = zext i64 %16 to i68
  %40 = mul i68 %38, %39
  %41 = zext i64 %27 to i68
  %42 = mul i68 %40, %41
  %43 = add i64 %n, -6
  %44 = zext i64 %43 to i68
  %45 = mul i68 %42, %44
  %46 = lshr i68 %45, 4
  %47 = trunc nuw i68 %46 to i64
  %48 = mul i64 %47, 72
  %49 = add i64 %33, %48
  %50 = shl i64 %n, 6
  %51 = add i64 %49, %50
  %52 = add i64 %51, -63
  %53 = add i64 %n, -7
  %54 = zext i64 %53 to i68
  %55 = mul i68 %45, %54
  %56 = lshr i68 %55, 4
  %57 = trunc nuw i68 %56 to i64
  %.neg = mul i64 %57, -2635249153387078800
  %58 = add i64 %.neg, %52
  br label %for.end

for.end:                                          ; preds = %for.body.preheader, %entry
  %s.0.lcssa = phi i64 [ 0, %entry ], [ %58, %for.body.preheader ]
  ret i64 %s.0.lcssa
}

; Function Attrs: mustprogress nofree noinline norecurse nosync nounwind willreturn memory(none) uwtable
define dso_local i64 @sum_p7(i64 noundef %n) local_unnamed_addr #0 {
entry:
  %cmp1.not = icmp eq i64 %n, 0
  br i1 %cmp1.not, label %for.end, label %for.body.preheader

for.body.preheader:                               ; preds = %entry
  %0 = add i64 %n, -1
  %1 = zext i64 %0 to i67
  %2 = add i64 %n, -2
  %3 = zext i64 %2 to i67
  %4 = mul i67 %1, %3
  %5 = add i64 %n, -3
  %6 = zext i64 %5 to i67
  %7 = mul i67 %4, %6
  %8 = add i64 %n, -4
  %9 = zext i64 %8 to i67
  %10 = mul i67 %7, %9
  %11 = lshr i67 %10, 3
  %12 = trunc nuw i67 %11 to i64
  %13 = mul i64 %12, 11802
  %14 = zext i64 %0 to i65
  %15 = zext i64 %2 to i65
  %16 = mul i65 %14, %15
  %17 = zext i64 %5 to i65
  %18 = mul i65 %16, %17
  %19 = lshr i65 %18, 1
  %20 = trunc nuw i65 %19 to i64
  %21 = mul i64 %20, 4046
  %22 = add i64 %13, %21
  %23 = add i64 %n, -5
  %24 = zext i64 %23 to i67
  %25 = mul i67 %10, %24
  %26 = lshr i67 %25, 3
  %27 = trunc nuw i67 %26 to i64
  %28 = mul i64 %27, 3808
  %29 = add i64 %22, %28
  %30 = lshr i65 %16, 1
  %31 = trunc nuw i65 %30 to i64
  %32 = mul i64 %31, 2059
  %33 = add i64 %29, %32
  %34 = shl i64 %n, 7
  %35 = add i64 %33, %34
  %36 = zext i64 %0 to i68
  %37 = zext i64 %2 to i68
  %38 = mul i68 %36, %37
  %39 = zext i64 %5 to i68
  %40 = mul i68 %38, %39
  %41 = zext i64 %8 to i68
  %42 = mul i68 %40, %41
  %43 = zext i64 %23 to i68
  %44 = mul i68 %42, %43
  %45 = add i64 %n, -6
  %46 = zext i64 %45 to i68
  %47 = mul i68 %44, %46
  %48 = add i64 %n, -7
  %49 = zext i64 %48 to i68
  %50 = mul i68 %47, %49
  %51 = lshr i68 %50, 4
  %52 = trunc nuw i68 %51 to i64
  %53 = mul i64 %52, 80
  %54 = add i64 %35, %53
  %55 = zext i64 %0 to i71
  %56 = zext i64 %2 to i71
  %57 = mul i71 %55, %56
  %58 = zext i64 %5 to i71
  %59 = mul i71 %57, %58
  %60 = zext i64 %8 to i71
  %61 = mul i71 %59, %60
  %62 = zext i64 %23 to i71
  %63 = mul i71 %61, %62
  %64 = zext i64 %45 to i71
  %65 = mul i71 %63, %64
  %66 = zext i64 %48 to i71
  %67 = mul i71 %65, %66
  %68 = add i64 %n, -8
  %69 = zext i64 %68 to i71
  %70 = mul i71 %67, %69
  %sh.diff = lshr i71 %70, 3
  %tr.sh.diff = trunc i71 %sh.diff to i64
  %71 = and i64 %tr.sh.diff, -16
  %72 = add i64 %54, %71
  %73 = add i64 %72, -127
  %74 = lshr i68 %47, 4
  %75 = trunc nuw i68 %74 to i64
  %.neg = mul i64 %75, -6148914691236516048
  %76 = add i64 %.neg, %73
  br label %for.end

for.end:                                          ; preds = %for.body.preheader, %entry
  %s.0.lcssa = phi i64 [ 0, %entry ], [ %76, %for.body.preheader ]
  ret i64 %s.0.lcssa
}

; Function Attrs: nofree noinline norecurse nosync nounwind memory(none) uwtable
define dso_local i64 @sum_p8(i64 noundef %n) local_unnamed_addr #1 {
entry:
  %cmp1.not = icmp eq i64 %n, 0
  br i1 %cmp1.not, label %for.end, label %for.body.preheader

for.body.preheader:                               ; preds = %entry
  %xtraiter = and i64 %n, 3
  %0 = icmp ult i64 %n, 4
  br i1 %0, label %for.body.epil.preheader, label %for.body.preheader.new

for.body.preheader.new:                           ; preds = %for.body.preheader
  %unroll_iter = and i64 %n, -4
  br label %for.body

for.body:                                         ; preds = %for.body, %for.body.preheader.new
  %t.03 = phi i64 [ 0, %for.body.preheader.new ], [ %add.3, %for.body ]
  %s.02 = phi i64 [ 0, %for.body.preheader.new ], [ %add7.3, %for.body ]
  %niter = phi i64 [ 0, %for.body.preheader.new ], [ %niter.next.3, %for.body ]
  %add = or disjoint i64 %t.03, 1
  %1 = mul i64 %add, %add
  %2 = mul i64 %1, %1
  %3 = mul i64 %2, %2
  %add7 = add i64 %3, %s.02
  %add.1 = or disjoint i64 %t.03, 2
  %4 = mul i64 %add.1, %add.1
  %5 = mul i64 %4, %4
  %6 = mul i64 %5, %5
  %add7.1 = add i64 %6, %add7
  %add.2 = or disjoint i64 %t.03, 3
  %7 = mul i64 %add.2, %add.2
  %8 = mul i64 %7, %7
  %9 = mul i64 %8, %8
  %add7.2 = add i64 %9, %add7.1
  %add.3 = add nuw i64 %t.03, 4
  %10 = mul i64 %add.3, %add.3
  %11 = mul i64 %10, %10
  %12 = mul i64 %11, %11
  %add7.3 = add i64 %12, %add7.2
  %niter.next.3 = add nuw i64 %niter, 4
  %niter.ncmp.3 = icmp eq i64 %niter.next.3, %unroll_iter
  br i1 %niter.ncmp.3, label %for.end.loopexit.unr-lcssa, label %for.body, !llvm.loop !5

for.end.loopexit.unr-lcssa:                       ; preds = %for.body
  %lcmp.mod.not = icmp eq i64 %xtraiter, 0
  br i1 %lcmp.mod.not, label %for.end, label %for.body.epil.preheader

for.body.epil.preheader:                          ; preds = %for.end.loopexit.unr-lcssa, %for.body.preheader
  %t.03.epil.init = phi i64 [ 0, %for.body.preheader ], [ %add.3, %for.end.loopexit.unr-lcssa ]
  %s.02.epil.init = phi i64 [ 0, %for.body.preheader ], [ %add7.3, %for.end.loopexit.unr-lcssa ]
  %lcmp.mod5 = icmp ne i64 %xtraiter, 0
  tail call void @llvm.assume(i1 %lcmp.mod5)
  br label %for.body.epil

for.body.epil:                                    ; preds = %for.body.epil, %for.body.epil.preheader
  %t.03.epil = phi i64 [ %add.epil, %for.body.epil ], [ %t.03.epil.init, %for.body.epil.preheader ]
  %s.02.epil = phi i64 [ %add7.epil, %for.body.epil ], [ %s.02.epil.init, %for.body.epil.preheader ]
  %epil.iter = phi i64 [ %epil.iter.next, %for.body.epil ], [ 0, %for.body.epil.preheader ]
  %add.epil = add nuw i64 %t.03.epil, 1
  %13 = mul i64 %add.epil, %add.epil
  %14 = mul i64 %13, %13
  %15 = mul i64 %14, %14
  %add7.epil = add i64 %15, %s.02.epil
  %epil.iter.next = add i64 %epil.iter, 1
  %epil.iter.cmp.not = icmp eq i64 %epil.iter.next, %xtraiter
  br i1 %epil.iter.cmp.not, label %for.end, label %for.body.epil, !llvm.loop !7

for.end:                                          ; preds = %for.end.loopexit.unr-lcssa, %for.body.epil, %entry
  %s.0.lcssa = phi i64 [ 0, %entry ], [ %add7.3, %for.end.loopexit.unr-lcssa ], [ %add7.epil, %for.body.epil ]
  ret i64 %s.0.lcssa
}

; Function Attrs: mustprogress nofree noinline norecurse nosync nounwind willreturn memory(none) uwtable
define dso_local i64 @general_cubic(i64 noundef %n) local_unnamed_addr #0 {
entry:
  %cmp4.not = icmp eq i64 %n, 0
  br i1 %cmp4.not, label %for.end, label %for.body.preheader

for.body.preheader:                               ; preds = %entry
  %0 = add i64 %n, -1
  %1 = zext i64 %0 to i65
  %2 = add i64 %n, -2
  %3 = zext i64 %2 to i65
  %4 = mul i65 %1, %3
  %5 = lshr i65 %4, 1
  %6 = trunc nuw i65 %5 to i64
  %7 = mul i64 %6, 129
  %8 = mul i64 %n, 49
  %9 = add i64 %7, %8
  %10 = add i64 %n, -3
  %11 = zext i64 %10 to i65
  %12 = mul i65 %4, %11
  %13 = lshr i65 %12, 1
  %14 = trunc nuw i65 %13 to i64
  %15 = mul i64 %14, 40
  %16 = add i64 %9, %15
  %17 = zext i64 %0 to i67
  %18 = zext i64 %2 to i67
  %19 = mul i67 %17, %18
  %20 = zext i64 %10 to i67
  %21 = mul i67 %19, %20
  %22 = add i64 %n, -4
  %23 = zext i64 %22 to i67
  %24 = mul i67 %21, %23
  %25 = lshr i67 %24, 3
  %26 = trunc nuw i67 %25 to i64
  %27 = mul i64 %26, 14
  %28 = add i64 %16, %27
  %29 = add i64 %28, -51
  br label %for.end

for.end:                                          ; preds = %for.body.preheader, %entry
  %s.0.lcssa = phi i64 [ 0, %entry ], [ %29, %for.body.preheader ]
  ret i64 %s.0.lcssa
}

; Function Attrs: mustprogress nofree noinline norecurse nosync nounwind willreturn memory(none) uwtable
define dso_local i64 @shifted_square(i64 noundef %n, i64 noundef %start, i64 noundef %step, i64 noundef %initial) local_unnamed_addr #0 {
entry:
  %cmp1.not = icmp eq i64 %n, 0
  br i1 %cmp1.not, label %for.end, label %for.body.preheader

for.body.preheader:                               ; preds = %entry
  %0 = mul i64 %start, %start
  %1 = add i64 %initial, %0
  %2 = add i64 %n, -1
  %3 = zext i64 %2 to i65
  %4 = add i64 %n, -2
  %5 = zext i64 %4 to i65
  %6 = mul i65 %3, %5
  %7 = add i64 %n, -3
  %8 = zext i64 %7 to i65
  %9 = mul i65 %6, %8
  %10 = lshr i65 %9, 1
  %11 = trunc nuw i65 %10 to i64
  %12 = mul i64 %step, %11
  %13 = mul i64 %12, 6148914691236517206
  %14 = mul i64 %step, 3
  %15 = shl i64 %start, 1
  %16 = add i64 %14, %15
  %17 = lshr i65 %6, 1
  %18 = trunc nuw i65 %17 to i64
  %19 = mul i64 %16, %18
  %20 = add i64 %13, %19
  %21 = mul i64 %step, %20
  %22 = add i64 %1, %21
  %23 = add i64 %step, %15
  %24 = mul i64 %step, %23
  %25 = add i64 %0, %24
  %26 = mul i64 %25, %2
  %27 = add i64 %22, %26
  br label %for.end

for.end:                                          ; preds = %for.body.preheader, %entry
  %s.0.lcssa = phi i64 [ %initial, %entry ], [ %27, %for.body.preheader ]
  ret i64 %s.0.lcssa
}

; Function Attrs: mustprogress nofree noinline norecurse nosync nounwind willreturn memory(none) uwtable
define dso_local i64 @triangular(i64 noundef %n) local_unnamed_addr #0 {
entry:
  %cmp1.not = icmp eq i64 %n, 0
  br i1 %cmp1.not, label %for.end, label %for.body.preheader

for.body.preheader:                               ; preds = %entry
  %0 = add i64 %n, -1
  %1 = zext i64 %0 to i67
  %2 = add i64 %n, -2
  %3 = zext i64 %2 to i67
  %4 = mul i67 %1, %3
  %5 = add i64 %n, -3
  %6 = zext i64 %5 to i67
  %7 = mul i67 %4, %6
  %8 = add i64 %n, -4
  %9 = zext i64 %8 to i67
  %10 = mul i67 %7, %9
  %11 = lshr i67 %10, 3
  %12 = trunc nuw i67 %11 to i64
  %13 = mul i64 %12, 6148914691236517206
  %14 = zext i64 %0 to i65
  %15 = zext i64 %2 to i65
  %16 = mul i65 %14, %15
  %17 = zext i64 %5 to i65
  %18 = mul i65 %16, %17
  %19 = lshr i65 %18, 1
  %20 = trunc nuw i65 %19 to i64
  %21 = add i64 %13, %20
  %22 = lshr i65 %16, 1
  %23 = trunc nuw i65 %22 to i64
  %24 = add i64 %21, %23
  br label %for.end

for.end:                                          ; preds = %for.body.preheader, %entry
  %z.0.lcssa = phi i64 [ 0, %entry ], [ %24, %for.body.preheader ]
  ret i64 %z.0.lcssa
}

; Function Attrs: nofree noinline norecurse nosync nounwind memory(none) uwtable
define dso_local i64 @nonlinear_self(i64 noundef %n) local_unnamed_addr #1 {
entry:
  %cmp1.not = icmp eq i64 %n, 0
  br i1 %cmp1.not, label %for.end, label %for.body.preheader

for.body.preheader:                               ; preds = %entry
  %xtraiter = and i64 %n, 7
  %0 = icmp ult i64 %n, 8
  br i1 %0, label %for.body.epil.preheader, label %for.body.preheader.new

for.body.preheader.new:                           ; preds = %for.body.preheader
  %unroll_iter = and i64 %n, -8
  br label %for.body

for.body:                                         ; preds = %for.body, %for.body.preheader.new
  %x.02 = phi i64 [ 2, %for.body.preheader.new ], [ %add.7, %for.body ]
  %niter = phi i64 [ 0, %for.body.preheader.new ], [ %niter.next.7, %for.body ]
  %mul = mul i64 %x.02, %x.02
  %add = add i64 %mul, 1
  %mul.1 = mul i64 %add, %add
  %add.1 = add i64 %mul.1, 1
  %mul.2 = mul i64 %add.1, %add.1
  %add.2 = add i64 %mul.2, 1
  %mul.3 = mul i64 %add.2, %add.2
  %add.3 = add i64 %mul.3, 1
  %mul.4 = mul i64 %add.3, %add.3
  %add.4 = add i64 %mul.4, 1
  %mul.5 = mul i64 %add.4, %add.4
  %add.5 = add i64 %mul.5, 1
  %mul.6 = mul i64 %add.5, %add.5
  %add.6 = add i64 %mul.6, 1
  %mul.7 = mul i64 %add.6, %add.6
  %add.7 = add i64 %mul.7, 1
  %niter.next.7 = add nuw i64 %niter, 8
  %niter.ncmp.7 = icmp eq i64 %niter.next.7, %unroll_iter
  br i1 %niter.ncmp.7, label %for.end.loopexit.unr-lcssa, label %for.body, !llvm.loop !9

for.end.loopexit.unr-lcssa:                       ; preds = %for.body
  %lcmp.mod.not = icmp eq i64 %xtraiter, 0
  br i1 %lcmp.mod.not, label %for.end, label %for.body.epil.preheader

for.body.epil.preheader:                          ; preds = %for.end.loopexit.unr-lcssa, %for.body.preheader
  %x.02.epil.init = phi i64 [ 2, %for.body.preheader ], [ %add.7, %for.end.loopexit.unr-lcssa ]
  %lcmp.mod5 = icmp ne i64 %xtraiter, 0
  tail call void @llvm.assume(i1 %lcmp.mod5)
  br label %for.body.epil

for.body.epil:                                    ; preds = %for.body.epil, %for.body.epil.preheader
  %x.02.epil = phi i64 [ %add.epil, %for.body.epil ], [ %x.02.epil.init, %for.body.epil.preheader ]
  %epil.iter = phi i64 [ %epil.iter.next, %for.body.epil ], [ 0, %for.body.epil.preheader ]
  %mul.epil = mul i64 %x.02.epil, %x.02.epil
  %add.epil = add i64 %mul.epil, 1
  %epil.iter.next = add i64 %epil.iter, 1
  %epil.iter.cmp.not = icmp eq i64 %epil.iter.next, %xtraiter
  br i1 %epil.iter.cmp.not, label %for.end, label %for.body.epil, !llvm.loop !10

for.end:                                          ; preds = %for.end.loopexit.unr-lcssa, %for.body.epil, %entry
  %x.0.lcssa = phi i64 [ 2, %entry ], [ %add.7, %for.end.loopexit.unr-lcssa ], [ %add.epil, %for.body.epil ]
  ret i64 %x.0.lcssa
}

; Function Attrs: nofree noinline norecurse nosync nounwind memory(none) uwtable
define dso_local range(i64 0, -1) i64 @exponential(i64 noundef %n) local_unnamed_addr #1 {
entry:
  %cmp1.not = icmp eq i64 %n, 0
  br i1 %cmp1.not, label %for.end, label %for.body.preheader

for.body.preheader:                               ; preds = %entry
  %xtraiter = and i64 %n, 7
  %0 = icmp ult i64 %n, 8
  br i1 %0, label %for.body.epil.preheader, label %for.body.preheader.new

for.body.preheader.new:                           ; preds = %for.body.preheader
  %unroll_iter = and i64 %n, -8
  br label %for.body

for.body:                                         ; preds = %for.body, %for.body.preheader.new
  %x.02 = phi i64 [ 1, %for.body.preheader.new ], [ %mul, %for.body ]
  %niter = phi i64 [ 0, %for.body.preheader.new ], [ %niter.next.7, %for.body ]
  %mul = shl i64 %x.02, 8
  %niter.next.7 = add nuw i64 %niter, 8
  %niter.ncmp.7 = icmp eq i64 %niter.next.7, %unroll_iter
  br i1 %niter.ncmp.7, label %for.end.loopexit.unr-lcssa, label %for.body, !llvm.loop !11

for.end.loopexit.unr-lcssa:                       ; preds = %for.body
  %lcmp.mod.not = icmp eq i64 %xtraiter, 0
  br i1 %lcmp.mod.not, label %for.end, label %for.body.epil.preheader

for.body.epil.preheader:                          ; preds = %for.end.loopexit.unr-lcssa, %for.body.preheader
  %x.02.epil.init = phi i64 [ 1, %for.body.preheader ], [ %mul, %for.end.loopexit.unr-lcssa ]
  %lcmp.mod5 = icmp ne i64 %xtraiter, 0
  tail call void @llvm.assume(i1 %lcmp.mod5)
  br label %for.body.epil

for.body.epil:                                    ; preds = %for.body.epil, %for.body.epil.preheader
  %x.02.epil = phi i64 [ %mul.epil, %for.body.epil ], [ %x.02.epil.init, %for.body.epil.preheader ]
  %epil.iter = phi i64 [ %epil.iter.next, %for.body.epil ], [ 0, %for.body.epil.preheader ]
  %mul.epil = shl i64 %x.02.epil, 1
  %epil.iter.next = add i64 %epil.iter, 1
  %epil.iter.cmp.not = icmp eq i64 %epil.iter.next, %xtraiter
  br i1 %epil.iter.cmp.not, label %for.end, label %for.body.epil, !llvm.loop !12

for.end:                                          ; preds = %for.end.loopexit.unr-lcssa, %for.body.epil, %entry
  %x.0.lcssa = phi i64 [ 1, %entry ], [ %mul, %for.end.loopexit.unr-lcssa ], [ %mul.epil, %for.body.epil ]
  ret i64 %x.0.lcssa
}

; Function Attrs: nofree noinline norecurse nosync nounwind memory(none) uwtable
define dso_local range(i64 2, 0) i64 @unknown_exit(i64 noundef %n) local_unnamed_addr #1 {
entry:
  %cmp1 = icmp ugt i64 %n, 2
  br i1 %cmp1, label %while.body, label %while.end

while.body:                                       ; preds = %entry, %while.body
  %x.02 = phi i64 [ %add, %while.body ], [ 2, %entry ]
  %mul = mul i64 %x.02, %x.02
  %add = add i64 %mul, 1
  %cmp = icmp ult i64 %add, %n
  br i1 %cmp, label %while.body, label %while.end, !llvm.loop !13

while.end:                                        ; preds = %while.body, %entry
  %x.0.lcssa = phi i64 [ 2, %entry ], [ %add, %while.body ]
  ret i64 %x.0.lcssa
}

; Function Attrs: mustprogress nofree noinline norecurse nosync nounwind willreturn memory(none) uwtable
define dso_local noundef i64 @fixed_square() local_unnamed_addr #0 {
entry:
  ret i64 385
}

; Function Attrs: nofree noinline norecurse nosync nounwind memory(argmem: readwrite, inaccessiblemem: readwrite) uwtable
define dso_local i64 @observable_square(i64 noundef %n, ptr nofree noundef captures(address) %out) local_unnamed_addr #2 {
entry:
  %cmp1.not = icmp eq i64 %n, 0
  br i1 %cmp1.not, label %for.end, label %for.body.preheader

for.body.preheader:                               ; preds = %entry
  %xtraiter = and i64 %n, 3
  %0 = icmp ult i64 %n, 4
  br i1 %0, label %for.body.epil.preheader, label %for.body.preheader.new

for.body.preheader.new:                           ; preds = %for.body.preheader
  %unroll_iter = and i64 %n, -4
  br label %for.body

for.body:                                         ; preds = %for.body, %for.body.preheader.new
  %t.03 = phi i64 [ 0, %for.body.preheader.new ], [ %add.3, %for.body ]
  %s.02 = phi i64 [ 0, %for.body.preheader.new ], [ %add1.3, %for.body ]
  %niter = phi i64 [ 0, %for.body.preheader.new ], [ %niter.next.3, %for.body ]
  %add = or disjoint i64 %t.03, 1
  %mul = mul i64 %add, %add
  %add1 = add i64 %mul, %s.02
  store volatile i64 %add1, ptr %out, align 8
  %add.1 = or disjoint i64 %t.03, 2
  %mul.1 = mul i64 %add.1, %add.1
  %add1.1 = add i64 %mul.1, %add1
  store volatile i64 %add1.1, ptr %out, align 8
  %add.2 = or disjoint i64 %t.03, 3
  %mul.2 = mul i64 %add.2, %add.2
  %add1.2 = add i64 %mul.2, %add1.1
  store volatile i64 %add1.2, ptr %out, align 8
  %add.3 = add nuw i64 %t.03, 4
  %mul.3 = mul i64 %add.3, %add.3
  %add1.3 = add i64 %mul.3, %add1.2
  store volatile i64 %add1.3, ptr %out, align 8
  %niter.next.3 = add nuw i64 %niter, 4
  %niter.ncmp.3 = icmp eq i64 %niter.next.3, %unroll_iter
  br i1 %niter.ncmp.3, label %for.end.loopexit.unr-lcssa, label %for.body, !llvm.loop !14

for.end.loopexit.unr-lcssa:                       ; preds = %for.body
  %lcmp.mod.not = icmp eq i64 %xtraiter, 0
  br i1 %lcmp.mod.not, label %for.end, label %for.body.epil.preheader

for.body.epil.preheader:                          ; preds = %for.end.loopexit.unr-lcssa, %for.body.preheader
  %t.03.epil.init = phi i64 [ 0, %for.body.preheader ], [ %add.3, %for.end.loopexit.unr-lcssa ]
  %s.02.epil.init = phi i64 [ 0, %for.body.preheader ], [ %add1.3, %for.end.loopexit.unr-lcssa ]
  %lcmp.mod5 = icmp ne i64 %xtraiter, 0
  tail call void @llvm.assume(i1 %lcmp.mod5)
  br label %for.body.epil

for.body.epil:                                    ; preds = %for.body.epil, %for.body.epil.preheader
  %t.03.epil = phi i64 [ %add.epil, %for.body.epil ], [ %t.03.epil.init, %for.body.epil.preheader ]
  %s.02.epil = phi i64 [ %add1.epil, %for.body.epil ], [ %s.02.epil.init, %for.body.epil.preheader ]
  %epil.iter = phi i64 [ %epil.iter.next, %for.body.epil ], [ 0, %for.body.epil.preheader ]
  %add.epil = add nuw i64 %t.03.epil, 1
  %mul.epil = mul i64 %add.epil, %add.epil
  %add1.epil = add i64 %mul.epil, %s.02.epil
  store volatile i64 %add1.epil, ptr %out, align 8
  %epil.iter.next = add i64 %epil.iter, 1
  %epil.iter.cmp.not = icmp eq i64 %epil.iter.next, %xtraiter
  br i1 %epil.iter.cmp.not, label %for.end, label %for.body.epil, !llvm.loop !15

for.end:                                          ; preds = %for.end.loopexit.unr-lcssa, %for.body.epil, %entry
  %s.0.lcssa = phi i64 [ 0, %entry ], [ %add1.3, %for.end.loopexit.unr-lcssa ], [ %add1.epil, %for.body.epil ]
  ret i64 %s.0.lcssa
}

; Function Attrs: nocallback nofree nosync nounwind willreturn memory(inaccessiblemem: write)
declare void @llvm.assume(i1 noundef) #3

attributes #0 = { mustprogress nofree noinline norecurse nosync nounwind willreturn memory(none) uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { nofree noinline norecurse nosync nounwind memory(none) uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #2 = { nofree noinline norecurse nosync nounwind memory(argmem: readwrite, inaccessiblemem: readwrite) uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #3 = { nocallback nofree nosync nounwind willreturn memory(inaccessiblemem: write) }

!llvm.module.flags = !{!0, !1, !2, !3}
!llvm.ident = !{!4}

!0 = !{i32 8, !"PIC Level", i32 2}
!1 = !{i32 7, !"PIE Level", i32 2}
!2 = !{i32 7, !"uwtable", i32 2}
!3 = !{i32 7, !"frame-pointer", i32 2}
!4 = !{!"clang version 24.0.0git (https://github.com/llvm/llvm-project.git 7615da10b75c57286a4032eac8bb43c1b8d2ba46)"}
!5 = distinct !{!5, !6}
!6 = !{!"llvm.loop.mustprogress"}
!7 = distinct !{!7, !8}
!8 = !{!"llvm.loop.unroll.disable"}
!9 = distinct !{!9, !6}
!10 = distinct !{!10, !8}
!11 = distinct !{!11, !6}
!12 = distinct !{!12, !8}
!13 = distinct !{!13, !6}
!14 = distinct !{!14, !6}
!15 = distinct !{!15, !8}
