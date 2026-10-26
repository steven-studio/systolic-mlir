; ModuleID = 'LLVMDialectModule'
source_filename = "LLVMDialectModule"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

declare ptr @malloc(i64)

declare i32 @fpga_gemm_wait(ptr)

declare i32 @fpga_tile_submit(ptr, i32, i32, i32, i32, i32, i32, ptr, ptr, ptr, i32, i32, i32, i32)

declare ptr @fpga_gemm_begin(i32, i32, i32, i32)

define { ptr, ptr, i64, [2 x i64], [2 x i64] } @systolic_matmul(ptr %0, ptr %1, i64 %2, i64 %3, i64 %4, i64 %5, i64 %6, ptr %7, ptr %8, i64 %9, i64 %10, i64 %11, i64 %12, i64 %13, ptr %14, ptr %15, i64 %16, i64 %17, i64 %18, i64 %19, i64 %20) {
  %22 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %14, 0
  %23 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %22, ptr %15, 1
  %24 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %23, i64 %16, 2
  %25 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %24, i64 %17, 3, 0
  %26 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %25, i64 %19, 4, 0
  %27 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %26, i64 %18, 3, 1
  %28 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %27, i64 %20, 4, 1
  %29 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %7, 0
  %30 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %29, ptr %8, 1
  %31 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %30, i64 %9, 2
  %32 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %31, i64 %10, 3, 0
  %33 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %32, i64 %12, 4, 0
  %34 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %33, i64 %11, 3, 1
  %35 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %34, i64 %13, 4, 1
  %36 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %0, 0
  %37 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %36, ptr %1, 1
  %38 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %37, i64 %2, 2
  %39 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %38, i64 %3, 3, 0
  %40 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %39, i64 %5, 4, 0
  %41 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %40, i64 %4, 3, 1
  %42 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %41, i64 %6, 4, 1
  %43 = alloca float, i64 128, align 4
  %44 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } poison, ptr %43, 0
  %45 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %44, ptr %43, 1
  %46 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %45, i64 0, 2
  %47 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %46, i64 128, 3, 0
  %48 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %47, i64 1, 4, 0
  %49 = alloca float, i64 128, align 4
  %50 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } poison, ptr %49, 0
  %51 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %50, ptr %49, 1
  %52 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %51, i64 0, 2
  %53 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %52, i64 128, 3, 0
  %54 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %53, i64 1, 4, 0
  %55 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %56 = ptrtoint ptr %55 to i64
  %57 = inttoptr i64 %56 to ptr
  %58 = call ptr @fpga_gemm_begin(i32 8, i32 4, i32 16, i32 2)
  %59 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %60 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %61 = insertvalue { ptr, ptr, i64 } poison, ptr %59, 0
  %62 = insertvalue { ptr, ptr, i64 } %61, ptr %60, 1
  %63 = insertvalue { ptr, ptr, i64 } %62, i64 0, 2
  %64 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %65 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %66 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %67 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %68 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %69 = extractvalue { ptr, ptr, i64 } %63, 0
  %70 = extractvalue { ptr, ptr, i64 } %63, 1
  %71 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %69, 0
  %72 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %71, ptr %70, 1
  %73 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %72, i64 0, 2
  %74 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %73, i64 4, 3, 0
  %75 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %74, i64 16, 4, 0
  %76 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %75, i64 16, 3, 1
  %77 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %76, i64 1, 4, 1
  %78 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 0
  %79 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %80 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %78, 0
  %81 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %80, ptr %79, 1
  %82 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %81, i64 0, 2
  %83 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %82, i64 4, 3, 0
  %84 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %83, i64 16, 4, 0
  %85 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %84, i64 16, 3, 1
  %86 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %85, i64 1, 4, 1
  %87 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %77, 3, 0
  %88 = mul i64 1, %87
  %89 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %77, 3, 1
  %90 = mul i64 %88, %89
  %91 = mul i64 %90, 4
  %92 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %77, 1
  %93 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %77, 2
  %94 = getelementptr float, ptr %92, i64 %93
  %95 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %86, 1
  %96 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %86, 2
  %97 = getelementptr float, ptr %95, i64 %96
  call void @llvm.memcpy.p0.p0.i64(ptr %97, ptr %94, i64 %91, i1 false)
  %98 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %99 = ptrtoint ptr %98 to i64
  %100 = inttoptr i64 %99 to ptr
  %101 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 0
  %102 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %103 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %101, 0
  %104 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %103, ptr %102, 1
  %105 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %104, i64 0, 2
  %106 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %105, i64 16, 3, 0
  %107 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %106, i64 4, 4, 0
  %108 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %107, i64 4, 3, 1
  %109 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %108, i64 1, 4, 1
  %110 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %111 = mul i64 1, %110
  %112 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %113 = mul i64 %111, %112
  %114 = mul i64 %113, 4
  %115 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %116 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %117 = getelementptr float, ptr %115, i64 %116
  %118 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %109, 1
  %119 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %109, 2
  %120 = getelementptr float, ptr %118, i64 %119
  call void @llvm.memcpy.p0.p0.i64(ptr %120, ptr %117, i64 %114, i1 false)
  %121 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %122 = ptrtoint ptr %121 to i64
  %123 = inttoptr i64 %122 to ptr
  %124 = call i32 @fpga_tile_submit(ptr %58, i32 0, i32 0, i32 1, i32 4, i32 16, i32 4, ptr %100, ptr %123, ptr %57, i32 4, i32 1, i32 0, i32 117)
  %125 = add i64 %56, 64
  %126 = inttoptr i64 %125 to ptr
  %127 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %128 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %129 = insertvalue { ptr, ptr, i64 } poison, ptr %127, 0
  %130 = insertvalue { ptr, ptr, i64 } %129, ptr %128, 1
  %131 = insertvalue { ptr, ptr, i64 } %130, i64 0, 2
  %132 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %133 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %134 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %135 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %136 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %137 = extractvalue { ptr, ptr, i64 } %131, 0
  %138 = extractvalue { ptr, ptr, i64 } %131, 1
  %139 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %137, 0
  %140 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %139, ptr %138, 1
  %141 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %140, i64 64, 2
  %142 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %141, i64 4, 3, 0
  %143 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %142, i64 16, 4, 0
  %144 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %143, i64 16, 3, 1
  %145 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %144, i64 1, 4, 1
  %146 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %145, 3, 0
  %147 = mul i64 1, %146
  %148 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %145, 3, 1
  %149 = mul i64 %147, %148
  %150 = mul i64 %149, 4
  %151 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %145, 1
  %152 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %145, 2
  %153 = getelementptr float, ptr %151, i64 %152
  %154 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %86, 1
  %155 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %86, 2
  %156 = getelementptr float, ptr %154, i64 %155
  call void @llvm.memcpy.p0.p0.i64(ptr %156, ptr %153, i64 %150, i1 false)
  %157 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %158 = mul i64 1, %157
  %159 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %160 = mul i64 %158, %159
  %161 = mul i64 %160, 4
  %162 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %163 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %164 = getelementptr float, ptr %162, i64 %163
  %165 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %109, 1
  %166 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %109, 2
  %167 = getelementptr float, ptr %165, i64 %166
  call void @llvm.memcpy.p0.p0.i64(ptr %167, ptr %164, i64 %161, i1 false)
  %168 = call i32 @fpga_tile_submit(ptr %58, i32 1, i32 0, i32 1, i32 4, i32 16, i32 4, ptr %100, ptr %123, ptr %126, i32 4, i32 2, i32 0, i32 117)
  %169 = call i32 @fpga_gemm_wait(ptr %58)
  %170 = call ptr @malloc(i64 128)
  %171 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %170, 0
  %172 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %171, ptr %170, 1
  %173 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %172, i64 0, 2
  %174 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %173, i64 8, 3, 0
  %175 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %174, i64 4, 3, 1
  %176 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %175, i64 4, 4, 0
  %177 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %176, i64 1, 4, 1
  %178 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %179 = mul i64 1, %178
  %180 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %181 = mul i64 %179, %180
  %182 = mul i64 %181, 4
  %183 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %184 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %185 = getelementptr float, ptr %183, i64 %184
  %186 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %177, 1
  %187 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %177, 2
  %188 = getelementptr float, ptr %186, i64 %187
  call void @llvm.memcpy.p0.p0.i64(ptr %188, ptr %185, i64 %182, i1 false)
  ret { ptr, ptr, i64, [2 x i64], [2 x i64] } %177
}

define void @_mlir_ciface_systolic_matmul(ptr %0, ptr %1, ptr %2, ptr %3) {
  %5 = load { ptr, ptr, i64, [2 x i64], [2 x i64] }, ptr %1, align 8
  %6 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5, 0
  %7 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5, 1
  %8 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5, 2
  %9 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5, 3, 0
  %10 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5, 3, 1
  %11 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5, 4, 0
  %12 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5, 4, 1
  %13 = load { ptr, ptr, i64, [2 x i64], [2 x i64] }, ptr %2, align 8
  %14 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %13, 0
  %15 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %13, 1
  %16 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %13, 2
  %17 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %13, 3, 0
  %18 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %13, 3, 1
  %19 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %13, 4, 0
  %20 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %13, 4, 1
  %21 = load { ptr, ptr, i64, [2 x i64], [2 x i64] }, ptr %3, align 8
  %22 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %21, 0
  %23 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %21, 1
  %24 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %21, 2
  %25 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %21, 3, 0
  %26 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %21, 3, 1
  %27 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %21, 4, 0
  %28 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %21, 4, 1
  %29 = call { ptr, ptr, i64, [2 x i64], [2 x i64] } @systolic_matmul(ptr %6, ptr %7, i64 %8, i64 %9, i64 %10, i64 %11, i64 %12, ptr %14, ptr %15, i64 %16, i64 %17, i64 %18, i64 %19, i64 %20, ptr %22, ptr %23, i64 %24, i64 %25, i64 %26, i64 %27, i64 %28)
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %29, ptr %0, align 8
  ret void
}

; Function Attrs: nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
declare void @llvm.memcpy.p0.p0.i64(ptr noalias writeonly captures(none), ptr noalias readonly captures(none), i64, i1 immarg) #0

define void @_mlir_systolic_matmul(ptr %0) {
  %2 = getelementptr ptr, ptr %0, i64 0
  %3 = load ptr, ptr %2, align 8
  %4 = load ptr, ptr %3, align 8
  %5 = getelementptr ptr, ptr %0, i64 1
  %6 = load ptr, ptr %5, align 8
  %7 = load ptr, ptr %6, align 8
  %8 = getelementptr ptr, ptr %0, i64 2
  %9 = load ptr, ptr %8, align 8
  %10 = load i64, ptr %9, align 8
  %11 = getelementptr ptr, ptr %0, i64 3
  %12 = load ptr, ptr %11, align 8
  %13 = load i64, ptr %12, align 8
  %14 = getelementptr ptr, ptr %0, i64 4
  %15 = load ptr, ptr %14, align 8
  %16 = load i64, ptr %15, align 8
  %17 = getelementptr ptr, ptr %0, i64 5
  %18 = load ptr, ptr %17, align 8
  %19 = load i64, ptr %18, align 8
  %20 = getelementptr ptr, ptr %0, i64 6
  %21 = load ptr, ptr %20, align 8
  %22 = load i64, ptr %21, align 8
  %23 = getelementptr ptr, ptr %0, i64 7
  %24 = load ptr, ptr %23, align 8
  %25 = load ptr, ptr %24, align 8
  %26 = getelementptr ptr, ptr %0, i64 8
  %27 = load ptr, ptr %26, align 8
  %28 = load ptr, ptr %27, align 8
  %29 = getelementptr ptr, ptr %0, i64 9
  %30 = load ptr, ptr %29, align 8
  %31 = load i64, ptr %30, align 8
  %32 = getelementptr ptr, ptr %0, i64 10
  %33 = load ptr, ptr %32, align 8
  %34 = load i64, ptr %33, align 8
  %35 = getelementptr ptr, ptr %0, i64 11
  %36 = load ptr, ptr %35, align 8
  %37 = load i64, ptr %36, align 8
  %38 = getelementptr ptr, ptr %0, i64 12
  %39 = load ptr, ptr %38, align 8
  %40 = load i64, ptr %39, align 8
  %41 = getelementptr ptr, ptr %0, i64 13
  %42 = load ptr, ptr %41, align 8
  %43 = load i64, ptr %42, align 8
  %44 = getelementptr ptr, ptr %0, i64 14
  %45 = load ptr, ptr %44, align 8
  %46 = load ptr, ptr %45, align 8
  %47 = getelementptr ptr, ptr %0, i64 15
  %48 = load ptr, ptr %47, align 8
  %49 = load ptr, ptr %48, align 8
  %50 = getelementptr ptr, ptr %0, i64 16
  %51 = load ptr, ptr %50, align 8
  %52 = load i64, ptr %51, align 8
  %53 = getelementptr ptr, ptr %0, i64 17
  %54 = load ptr, ptr %53, align 8
  %55 = load i64, ptr %54, align 8
  %56 = getelementptr ptr, ptr %0, i64 18
  %57 = load ptr, ptr %56, align 8
  %58 = load i64, ptr %57, align 8
  %59 = getelementptr ptr, ptr %0, i64 19
  %60 = load ptr, ptr %59, align 8
  %61 = load i64, ptr %60, align 8
  %62 = getelementptr ptr, ptr %0, i64 20
  %63 = load ptr, ptr %62, align 8
  %64 = load i64, ptr %63, align 8
  %65 = call { ptr, ptr, i64, [2 x i64], [2 x i64] } @systolic_matmul(ptr %4, ptr %7, i64 %10, i64 %13, i64 %16, i64 %19, i64 %22, ptr %25, ptr %28, i64 %31, i64 %34, i64 %37, i64 %40, i64 %43, ptr %46, ptr %49, i64 %52, i64 %55, i64 %58, i64 %61, i64 %64)
  %66 = getelementptr ptr, ptr %0, i64 21
  %67 = load ptr, ptr %66, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %65, ptr %67, align 8
  ret void
}

define void @_mlir__mlir_ciface_systolic_matmul(ptr %0) {
  %2 = getelementptr ptr, ptr %0, i64 0
  %3 = load ptr, ptr %2, align 8
  %4 = load ptr, ptr %3, align 8
  %5 = getelementptr ptr, ptr %0, i64 1
  %6 = load ptr, ptr %5, align 8
  %7 = load ptr, ptr %6, align 8
  %8 = getelementptr ptr, ptr %0, i64 2
  %9 = load ptr, ptr %8, align 8
  %10 = load ptr, ptr %9, align 8
  %11 = getelementptr ptr, ptr %0, i64 3
  %12 = load ptr, ptr %11, align 8
  %13 = load ptr, ptr %12, align 8
  call void @_mlir_ciface_systolic_matmul(ptr %4, ptr %7, ptr %10, ptr %13)
  ret void
}

attributes #0 = { nocallback nofree nosync nounwind willreturn memory(argmem: readwrite) }

!llvm.module.flags = !{!0}

!0 = !{i32 2, !"Debug Info Version", i32 3}
