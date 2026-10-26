; ModuleID = 'LLVMDialectModule'
source_filename = "LLVMDialectModule"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

declare ptr @malloc(i64)

declare void @memrefCopy(i64, ptr, ptr)

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
  %58 = call ptr @fpga_gemm_begin(i32 16, i32 16, i32 31, i32 14)
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
  %74 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %73, i64 8, 3, 0
  %75 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %74, i64 31, 4, 0
  %76 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %75, i64 16, 3, 1
  %77 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %76, i64 1, 4, 1
  %78 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 0
  %79 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %80 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %78, 0
  %81 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %80, ptr %79, 1
  %82 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %81, i64 0, 2
  %83 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %82, i64 8, 3, 0
  %84 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %83, i64 16, 4, 0
  %85 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %84, i64 16, 3, 1
  %86 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %85, i64 1, 4, 1
  %87 = call ptr @llvm.stacksave.p0()
  %88 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %77, ptr %88, align 8
  %89 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %88, 1
  %90 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %86, ptr %90, align 8
  %91 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %90, 1
  %92 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %89, ptr %92, align 8
  %93 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %91, ptr %93, align 8
  call void @memrefCopy(i64 4, ptr %92, ptr %93)
  call void @llvm.stackrestore.p0(ptr %87)
  %94 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %95 = ptrtoint ptr %94 to i64
  %96 = inttoptr i64 %95 to ptr
  %97 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %98 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %99 = insertvalue { ptr, ptr, i64 } poison, ptr %97, 0
  %100 = insertvalue { ptr, ptr, i64 } %99, ptr %98, 1
  %101 = insertvalue { ptr, ptr, i64 } %100, i64 0, 2
  %102 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %103 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %104 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %105 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %106 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %107 = extractvalue { ptr, ptr, i64 } %101, 0
  %108 = extractvalue { ptr, ptr, i64 } %101, 1
  %109 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %107, 0
  %110 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %109, ptr %108, 1
  %111 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %110, i64 0, 2
  %112 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %111, i64 16, 3, 0
  %113 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %112, i64 16, 4, 0
  %114 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %113, i64 8, 3, 1
  %115 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %114, i64 1, 4, 1
  %116 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 0
  %117 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %118 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %116, 0
  %119 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %118, ptr %117, 1
  %120 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %119, i64 0, 2
  %121 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %120, i64 16, 3, 0
  %122 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %121, i64 8, 4, 0
  %123 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %122, i64 8, 3, 1
  %124 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %123, i64 1, 4, 1
  %125 = call ptr @llvm.stacksave.p0()
  %126 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %115, ptr %126, align 8
  %127 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %126, 1
  %128 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %124, ptr %128, align 8
  %129 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %128, 1
  %130 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %127, ptr %130, align 8
  %131 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %129, ptr %131, align 8
  call void @memrefCopy(i64 4, ptr %130, ptr %131)
  call void @llvm.stackrestore.p0(ptr %125)
  %132 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %133 = ptrtoint ptr %132 to i64
  %134 = inttoptr i64 %133 to ptr
  %135 = call i32 @fpga_tile_submit(ptr %58, i32 0, i32 0, i32 2, i32 8, i32 16, i32 8, ptr %96, ptr %134, ptr %57, i32 16, i32 0, i32 0, i32 125)
  %136 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %137 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %138 = insertvalue { ptr, ptr, i64 } poison, ptr %136, 0
  %139 = insertvalue { ptr, ptr, i64 } %138, ptr %137, 1
  %140 = insertvalue { ptr, ptr, i64 } %139, i64 0, 2
  %141 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %142 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %143 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %144 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %145 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %146 = extractvalue { ptr, ptr, i64 } %140, 0
  %147 = extractvalue { ptr, ptr, i64 } %140, 1
  %148 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %146, 0
  %149 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %148, ptr %147, 1
  %150 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %149, i64 16, 2
  %151 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %150, i64 8, 3, 0
  %152 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %151, i64 31, 4, 0
  %153 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %152, i64 15, 3, 1
  %154 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %153, i64 1, 4, 1
  %155 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 0
  %156 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %157 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %155, 0
  %158 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %157, ptr %156, 1
  %159 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %158, i64 0, 2
  %160 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %159, i64 8, 3, 0
  %161 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %160, i64 15, 4, 0
  %162 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %161, i64 15, 3, 1
  %163 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %162, i64 1, 4, 1
  %164 = call ptr @llvm.stacksave.p0()
  %165 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %154, ptr %165, align 8
  %166 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %165, 1
  %167 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %163, ptr %167, align 8
  %168 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %167, 1
  %169 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %166, ptr %169, align 8
  %170 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %168, ptr %170, align 8
  call void @memrefCopy(i64 4, ptr %169, ptr %170)
  call void @llvm.stackrestore.p0(ptr %164)
  %171 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %172 = ptrtoint ptr %171 to i64
  %173 = inttoptr i64 %172 to ptr
  %174 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %175 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %176 = insertvalue { ptr, ptr, i64 } poison, ptr %174, 0
  %177 = insertvalue { ptr, ptr, i64 } %176, ptr %175, 1
  %178 = insertvalue { ptr, ptr, i64 } %177, i64 0, 2
  %179 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %180 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %181 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %182 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %183 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %184 = extractvalue { ptr, ptr, i64 } %178, 0
  %185 = extractvalue { ptr, ptr, i64 } %178, 1
  %186 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %184, 0
  %187 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %186, ptr %185, 1
  %188 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %187, i64 256, 2
  %189 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %188, i64 15, 3, 0
  %190 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %189, i64 16, 4, 0
  %191 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %190, i64 8, 3, 1
  %192 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %191, i64 1, 4, 1
  %193 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 0
  %194 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %195 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %193, 0
  %196 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %195, ptr %194, 1
  %197 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %196, i64 0, 2
  %198 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %197, i64 15, 3, 0
  %199 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %198, i64 8, 4, 0
  %200 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %199, i64 8, 3, 1
  %201 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %200, i64 1, 4, 1
  %202 = call ptr @llvm.stacksave.p0()
  %203 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %192, ptr %203, align 8
  %204 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %203, 1
  %205 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %201, ptr %205, align 8
  %206 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %205, 1
  %207 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %204, ptr %207, align 8
  %208 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %206, ptr %208, align 8
  call void @memrefCopy(i64 4, ptr %207, ptr %208)
  call void @llvm.stackrestore.p0(ptr %202)
  %209 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %210 = ptrtoint ptr %209 to i64
  %211 = inttoptr i64 %210 to ptr
  %212 = call i32 @fpga_tile_submit(ptr %58, i32 0, i32 1, i32 2, i32 8, i32 15, i32 8, ptr %173, ptr %211, ptr %57, i32 16, i32 0, i32 42, i32 124)
  %213 = add i64 %56, 32
  %214 = inttoptr i64 %213 to ptr
  %215 = call ptr @llvm.stacksave.p0()
  %216 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %77, ptr %216, align 8
  %217 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %216, 1
  %218 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %86, ptr %218, align 8
  %219 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %218, 1
  %220 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %217, ptr %220, align 8
  %221 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %219, ptr %221, align 8
  call void @memrefCopy(i64 4, ptr %220, ptr %221)
  call void @llvm.stackrestore.p0(ptr %215)
  %222 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %223 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %224 = insertvalue { ptr, ptr, i64 } poison, ptr %222, 0
  %225 = insertvalue { ptr, ptr, i64 } %224, ptr %223, 1
  %226 = insertvalue { ptr, ptr, i64 } %225, i64 0, 2
  %227 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %228 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %229 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %230 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %231 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %232 = extractvalue { ptr, ptr, i64 } %226, 0
  %233 = extractvalue { ptr, ptr, i64 } %226, 1
  %234 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %232, 0
  %235 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %234, ptr %233, 1
  %236 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %235, i64 8, 2
  %237 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %236, i64 16, 3, 0
  %238 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %237, i64 16, 4, 0
  %239 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %238, i64 8, 3, 1
  %240 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %239, i64 1, 4, 1
  %241 = call ptr @llvm.stacksave.p0()
  %242 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %240, ptr %242, align 8
  %243 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %242, 1
  %244 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %124, ptr %244, align 8
  %245 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %244, 1
  %246 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %243, ptr %246, align 8
  %247 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %245, ptr %247, align 8
  call void @memrefCopy(i64 4, ptr %246, ptr %247)
  call void @llvm.stackrestore.p0(ptr %241)
  %248 = call i32 @fpga_tile_submit(ptr %58, i32 1, i32 0, i32 2, i32 8, i32 16, i32 8, ptr %96, ptr %134, ptr %214, i32 16, i32 0, i32 128, i32 125)
  %249 = call ptr @llvm.stacksave.p0()
  %250 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %154, ptr %250, align 8
  %251 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %250, 1
  %252 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %163, ptr %252, align 8
  %253 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %252, 1
  %254 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %251, ptr %254, align 8
  %255 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %253, ptr %255, align 8
  call void @memrefCopy(i64 4, ptr %254, ptr %255)
  call void @llvm.stackrestore.p0(ptr %249)
  %256 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %257 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %258 = insertvalue { ptr, ptr, i64 } poison, ptr %256, 0
  %259 = insertvalue { ptr, ptr, i64 } %258, ptr %257, 1
  %260 = insertvalue { ptr, ptr, i64 } %259, i64 0, 2
  %261 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %262 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %263 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %264 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %265 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %266 = extractvalue { ptr, ptr, i64 } %260, 0
  %267 = extractvalue { ptr, ptr, i64 } %260, 1
  %268 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %266, 0
  %269 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %268, ptr %267, 1
  %270 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %269, i64 264, 2
  %271 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %270, i64 15, 3, 0
  %272 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %271, i64 16, 4, 0
  %273 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %272, i64 8, 3, 1
  %274 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %273, i64 1, 4, 1
  %275 = call ptr @llvm.stacksave.p0()
  %276 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %274, ptr %276, align 8
  %277 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %276, 1
  %278 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %201, ptr %278, align 8
  %279 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %278, 1
  %280 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %277, ptr %280, align 8
  %281 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %279, ptr %281, align 8
  call void @memrefCopy(i64 4, ptr %280, ptr %281)
  call void @llvm.stackrestore.p0(ptr %275)
  %282 = call i32 @fpga_tile_submit(ptr %58, i32 1, i32 1, i32 2, i32 8, i32 15, i32 8, ptr %173, ptr %211, ptr %214, i32 16, i32 0, i32 197, i32 124)
  %283 = add i64 %56, 512
  %284 = inttoptr i64 %283 to ptr
  %285 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %286 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %287 = insertvalue { ptr, ptr, i64 } poison, ptr %285, 0
  %288 = insertvalue { ptr, ptr, i64 } %287, ptr %286, 1
  %289 = insertvalue { ptr, ptr, i64 } %288, i64 0, 2
  %290 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %291 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %292 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %293 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %294 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %295 = extractvalue { ptr, ptr, i64 } %289, 0
  %296 = extractvalue { ptr, ptr, i64 } %289, 1
  %297 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %295, 0
  %298 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %297, ptr %296, 1
  %299 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %298, i64 248, 2
  %300 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %299, i64 8, 3, 0
  %301 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %300, i64 31, 4, 0
  %302 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %301, i64 16, 3, 1
  %303 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %302, i64 1, 4, 1
  %304 = call ptr @llvm.stacksave.p0()
  %305 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %303, ptr %305, align 8
  %306 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %305, 1
  %307 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %86, ptr %307, align 8
  %308 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %307, 1
  %309 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %306, ptr %309, align 8
  %310 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %308, ptr %310, align 8
  call void @memrefCopy(i64 4, ptr %309, ptr %310)
  call void @llvm.stackrestore.p0(ptr %304)
  %311 = call ptr @llvm.stacksave.p0()
  %312 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %115, ptr %312, align 8
  %313 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %312, 1
  %314 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %124, ptr %314, align 8
  %315 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %314, 1
  %316 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %313, ptr %316, align 8
  %317 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %315, ptr %317, align 8
  call void @memrefCopy(i64 4, ptr %316, ptr %317)
  call void @llvm.stackrestore.p0(ptr %311)
  %318 = call i32 @fpga_tile_submit(ptr %58, i32 2, i32 0, i32 2, i32 8, i32 16, i32 8, ptr %96, ptr %134, ptr %284, i32 16, i32 0, i32 266, i32 125)
  %319 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %320 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %321 = insertvalue { ptr, ptr, i64 } poison, ptr %319, 0
  %322 = insertvalue { ptr, ptr, i64 } %321, ptr %320, 1
  %323 = insertvalue { ptr, ptr, i64 } %322, i64 0, 2
  %324 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %325 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %326 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %327 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %328 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %329 = extractvalue { ptr, ptr, i64 } %323, 0
  %330 = extractvalue { ptr, ptr, i64 } %323, 1
  %331 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %329, 0
  %332 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %331, ptr %330, 1
  %333 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %332, i64 264, 2
  %334 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %333, i64 8, 3, 0
  %335 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %334, i64 31, 4, 0
  %336 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %335, i64 15, 3, 1
  %337 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %336, i64 1, 4, 1
  %338 = call ptr @llvm.stacksave.p0()
  %339 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %337, ptr %339, align 8
  %340 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %339, 1
  %341 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %163, ptr %341, align 8
  %342 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %341, 1
  %343 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %340, ptr %343, align 8
  %344 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %342, ptr %344, align 8
  call void @memrefCopy(i64 4, ptr %343, ptr %344)
  call void @llvm.stackrestore.p0(ptr %338)
  %345 = call ptr @llvm.stacksave.p0()
  %346 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %192, ptr %346, align 8
  %347 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %346, 1
  %348 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %201, ptr %348, align 8
  %349 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %348, 1
  %350 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %347, ptr %350, align 8
  %351 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %349, ptr %351, align 8
  call void @memrefCopy(i64 4, ptr %350, ptr %351)
  call void @llvm.stackrestore.p0(ptr %345)
  %352 = call i32 @fpga_tile_submit(ptr %58, i32 2, i32 1, i32 2, i32 8, i32 15, i32 8, ptr %173, ptr %211, ptr %284, i32 16, i32 0, i32 335, i32 124)
  %353 = add i64 %56, 544
  %354 = inttoptr i64 %353 to ptr
  %355 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %356 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %357 = insertvalue { ptr, ptr, i64 } poison, ptr %355, 0
  %358 = insertvalue { ptr, ptr, i64 } %357, ptr %356, 1
  %359 = insertvalue { ptr, ptr, i64 } %358, i64 0, 2
  %360 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %361 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %362 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %363 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %364 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %365 = extractvalue { ptr, ptr, i64 } %359, 0
  %366 = extractvalue { ptr, ptr, i64 } %359, 1
  %367 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %365, 0
  %368 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %367, ptr %366, 1
  %369 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %368, i64 248, 2
  %370 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %369, i64 4, 3, 0
  %371 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %370, i64 31, 4, 0
  %372 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %371, i64 16, 3, 1
  %373 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %372, i64 1, 4, 1
  %374 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 0
  %375 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %376 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %374, 0
  %377 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %376, ptr %375, 1
  %378 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %377, i64 0, 2
  %379 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %378, i64 4, 3, 0
  %380 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %379, i64 16, 4, 0
  %381 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %380, i64 16, 3, 1
  %382 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %381, i64 1, 4, 1
  %383 = call ptr @llvm.stacksave.p0()
  %384 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %373, ptr %384, align 8
  %385 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %384, 1
  %386 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %382, ptr %386, align 8
  %387 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %386, 1
  %388 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %385, ptr %388, align 8
  %389 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %387, ptr %389, align 8
  call void @memrefCopy(i64 4, ptr %388, ptr %389)
  call void @llvm.stackrestore.p0(ptr %383)
  %390 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %391 = ptrtoint ptr %390 to i64
  %392 = inttoptr i64 %391 to ptr
  %393 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %394 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %395 = insertvalue { ptr, ptr, i64 } poison, ptr %393, 0
  %396 = insertvalue { ptr, ptr, i64 } %395, ptr %394, 1
  %397 = insertvalue { ptr, ptr, i64 } %396, i64 0, 2
  %398 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %399 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %400 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %401 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %402 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %403 = extractvalue { ptr, ptr, i64 } %397, 0
  %404 = extractvalue { ptr, ptr, i64 } %397, 1
  %405 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %403, 0
  %406 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %405, ptr %404, 1
  %407 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %406, i64 8, 2
  %408 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %407, i64 16, 3, 0
  %409 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %408, i64 16, 4, 0
  %410 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %409, i64 4, 3, 1
  %411 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %410, i64 1, 4, 1
  %412 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 0
  %413 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %414 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %412, 0
  %415 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %414, ptr %413, 1
  %416 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %415, i64 0, 2
  %417 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %416, i64 16, 3, 0
  %418 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %417, i64 4, 4, 0
  %419 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %418, i64 4, 3, 1
  %420 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %419, i64 1, 4, 1
  %421 = call ptr @llvm.stacksave.p0()
  %422 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %411, ptr %422, align 8
  %423 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %422, 1
  %424 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %420, ptr %424, align 8
  %425 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %424, 1
  %426 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %423, ptr %426, align 8
  %427 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %425, ptr %427, align 8
  call void @memrefCopy(i64 4, ptr %426, ptr %427)
  call void @llvm.stackrestore.p0(ptr %421)
  %428 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %429 = ptrtoint ptr %428 to i64
  %430 = inttoptr i64 %429 to ptr
  %431 = call i32 @fpga_tile_submit(ptr %58, i32 3, i32 0, i32 2, i32 4, i32 16, i32 4, ptr %392, ptr %430, ptr %354, i32 16, i32 1, i32 0, i32 117)
  %432 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %433 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %434 = insertvalue { ptr, ptr, i64 } poison, ptr %432, 0
  %435 = insertvalue { ptr, ptr, i64 } %434, ptr %433, 1
  %436 = insertvalue { ptr, ptr, i64 } %435, i64 0, 2
  %437 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %438 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %439 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %440 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %441 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %442 = extractvalue { ptr, ptr, i64 } %436, 0
  %443 = extractvalue { ptr, ptr, i64 } %436, 1
  %444 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %442, 0
  %445 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %444, ptr %443, 1
  %446 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %445, i64 264, 2
  %447 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %446, i64 4, 3, 0
  %448 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %447, i64 31, 4, 0
  %449 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %448, i64 15, 3, 1
  %450 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %449, i64 1, 4, 1
  %451 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 0
  %452 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %453 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %451, 0
  %454 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %453, ptr %452, 1
  %455 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %454, i64 0, 2
  %456 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %455, i64 4, 3, 0
  %457 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %456, i64 15, 4, 0
  %458 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %457, i64 15, 3, 1
  %459 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %458, i64 1, 4, 1
  %460 = call ptr @llvm.stacksave.p0()
  %461 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %450, ptr %461, align 8
  %462 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %461, 1
  %463 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %459, ptr %463, align 8
  %464 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %463, 1
  %465 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %462, ptr %465, align 8
  %466 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %464, ptr %466, align 8
  call void @memrefCopy(i64 4, ptr %465, ptr %466)
  call void @llvm.stackrestore.p0(ptr %460)
  %467 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %468 = ptrtoint ptr %467 to i64
  %469 = inttoptr i64 %468 to ptr
  %470 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %471 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %472 = insertvalue { ptr, ptr, i64 } poison, ptr %470, 0
  %473 = insertvalue { ptr, ptr, i64 } %472, ptr %471, 1
  %474 = insertvalue { ptr, ptr, i64 } %473, i64 0, 2
  %475 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %476 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %477 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %478 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %479 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %480 = extractvalue { ptr, ptr, i64 } %474, 0
  %481 = extractvalue { ptr, ptr, i64 } %474, 1
  %482 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %480, 0
  %483 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %482, ptr %481, 1
  %484 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %483, i64 264, 2
  %485 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %484, i64 15, 3, 0
  %486 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %485, i64 16, 4, 0
  %487 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %486, i64 4, 3, 1
  %488 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %487, i64 1, 4, 1
  %489 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 0
  %490 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %491 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %489, 0
  %492 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %491, ptr %490, 1
  %493 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %492, i64 0, 2
  %494 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %493, i64 15, 3, 0
  %495 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %494, i64 4, 4, 0
  %496 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %495, i64 4, 3, 1
  %497 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %496, i64 1, 4, 1
  %498 = call ptr @llvm.stacksave.p0()
  %499 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %488, ptr %499, align 8
  %500 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %499, 1
  %501 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %497, ptr %501, align 8
  %502 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %501, 1
  %503 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %500, ptr %503, align 8
  %504 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %502, ptr %504, align 8
  call void @memrefCopy(i64 4, ptr %503, ptr %504)
  call void @llvm.stackrestore.p0(ptr %498)
  %505 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %506 = ptrtoint ptr %505 to i64
  %507 = inttoptr i64 %506 to ptr
  %508 = call i32 @fpga_tile_submit(ptr %58, i32 3, i32 1, i32 2, i32 4, i32 15, i32 4, ptr %469, ptr %507, ptr %354, i32 16, i32 1, i32 42, i32 116)
  %509 = add i64 %56, 560
  %510 = inttoptr i64 %509 to ptr
  %511 = call ptr @llvm.stacksave.p0()
  %512 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %373, ptr %512, align 8
  %513 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %512, 1
  %514 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %382, ptr %514, align 8
  %515 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %514, 1
  %516 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %513, ptr %516, align 8
  %517 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %515, ptr %517, align 8
  call void @memrefCopy(i64 4, ptr %516, ptr %517)
  call void @llvm.stackrestore.p0(ptr %511)
  %518 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %519 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %520 = insertvalue { ptr, ptr, i64 } poison, ptr %518, 0
  %521 = insertvalue { ptr, ptr, i64 } %520, ptr %519, 1
  %522 = insertvalue { ptr, ptr, i64 } %521, i64 0, 2
  %523 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %524 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %525 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %526 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %527 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %528 = extractvalue { ptr, ptr, i64 } %522, 0
  %529 = extractvalue { ptr, ptr, i64 } %522, 1
  %530 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %528, 0
  %531 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %530, ptr %529, 1
  %532 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %531, i64 12, 2
  %533 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %532, i64 16, 3, 0
  %534 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %533, i64 16, 4, 0
  %535 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %534, i64 4, 3, 1
  %536 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %535, i64 1, 4, 1
  %537 = call ptr @llvm.stacksave.p0()
  %538 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %536, ptr %538, align 8
  %539 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %538, 1
  %540 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %420, ptr %540, align 8
  %541 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %540, 1
  %542 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %539, ptr %542, align 8
  %543 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %541, ptr %543, align 8
  call void @memrefCopy(i64 4, ptr %542, ptr %543)
  call void @llvm.stackrestore.p0(ptr %537)
  %544 = call i32 @fpga_tile_submit(ptr %58, i32 4, i32 0, i32 2, i32 4, i32 16, i32 4, ptr %392, ptr %430, ptr %510, i32 16, i32 2, i32 0, i32 117)
  %545 = call ptr @llvm.stacksave.p0()
  %546 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %450, ptr %546, align 8
  %547 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %546, 1
  %548 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %459, ptr %548, align 8
  %549 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %548, 1
  %550 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %547, ptr %550, align 8
  %551 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %549, ptr %551, align 8
  call void @memrefCopy(i64 4, ptr %550, ptr %551)
  call void @llvm.stackrestore.p0(ptr %545)
  %552 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %553 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %554 = insertvalue { ptr, ptr, i64 } poison, ptr %552, 0
  %555 = insertvalue { ptr, ptr, i64 } %554, ptr %553, 1
  %556 = insertvalue { ptr, ptr, i64 } %555, i64 0, 2
  %557 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %558 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %559 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %560 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %561 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %562 = extractvalue { ptr, ptr, i64 } %556, 0
  %563 = extractvalue { ptr, ptr, i64 } %556, 1
  %564 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %562, 0
  %565 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %564, ptr %563, 1
  %566 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %565, i64 268, 2
  %567 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %566, i64 15, 3, 0
  %568 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %567, i64 16, 4, 0
  %569 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %568, i64 4, 3, 1
  %570 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %569, i64 1, 4, 1
  %571 = call ptr @llvm.stacksave.p0()
  %572 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %570, ptr %572, align 8
  %573 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %572, 1
  %574 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %497, ptr %574, align 8
  %575 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %574, 1
  %576 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %573, ptr %576, align 8
  %577 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %575, ptr %577, align 8
  call void @memrefCopy(i64 4, ptr %576, ptr %577)
  call void @llvm.stackrestore.p0(ptr %571)
  %578 = call i32 @fpga_tile_submit(ptr %58, i32 4, i32 1, i32 2, i32 4, i32 15, i32 4, ptr %469, ptr %507, ptr %510, i32 16, i32 2, i32 42, i32 116)
  %579 = add i64 %56, 800
  %580 = inttoptr i64 %579 to ptr
  %581 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %582 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %583 = insertvalue { ptr, ptr, i64 } poison, ptr %581, 0
  %584 = insertvalue { ptr, ptr, i64 } %583, ptr %582, 1
  %585 = insertvalue { ptr, ptr, i64 } %584, i64 0, 2
  %586 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %587 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %588 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %589 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %590 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %591 = extractvalue { ptr, ptr, i64 } %585, 0
  %592 = extractvalue { ptr, ptr, i64 } %585, 1
  %593 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %591, 0
  %594 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %593, ptr %592, 1
  %595 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %594, i64 372, 2
  %596 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %595, i64 4, 3, 0
  %597 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %596, i64 31, 4, 0
  %598 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %597, i64 16, 3, 1
  %599 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %598, i64 1, 4, 1
  %600 = call ptr @llvm.stacksave.p0()
  %601 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %599, ptr %601, align 8
  %602 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %601, 1
  %603 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %382, ptr %603, align 8
  %604 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %603, 1
  %605 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %602, ptr %605, align 8
  %606 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %604, ptr %606, align 8
  call void @memrefCopy(i64 4, ptr %605, ptr %606)
  call void @llvm.stackrestore.p0(ptr %600)
  %607 = call ptr @llvm.stacksave.p0()
  %608 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %411, ptr %608, align 8
  %609 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %608, 1
  %610 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %420, ptr %610, align 8
  %611 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %610, 1
  %612 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %609, ptr %612, align 8
  %613 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %611, ptr %613, align 8
  call void @memrefCopy(i64 4, ptr %612, ptr %613)
  call void @llvm.stackrestore.p0(ptr %607)
  %614 = call i32 @fpga_tile_submit(ptr %58, i32 5, i32 0, i32 2, i32 4, i32 16, i32 4, ptr %392, ptr %430, ptr %580, i32 16, i32 3, i32 0, i32 117)
  %615 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %616 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %617 = insertvalue { ptr, ptr, i64 } poison, ptr %615, 0
  %618 = insertvalue { ptr, ptr, i64 } %617, ptr %616, 1
  %619 = insertvalue { ptr, ptr, i64 } %618, i64 0, 2
  %620 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %621 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %622 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %623 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %624 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %625 = extractvalue { ptr, ptr, i64 } %619, 0
  %626 = extractvalue { ptr, ptr, i64 } %619, 1
  %627 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %625, 0
  %628 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %627, ptr %626, 1
  %629 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %628, i64 388, 2
  %630 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %629, i64 4, 3, 0
  %631 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %630, i64 31, 4, 0
  %632 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %631, i64 15, 3, 1
  %633 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %632, i64 1, 4, 1
  %634 = call ptr @llvm.stacksave.p0()
  %635 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %633, ptr %635, align 8
  %636 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %635, 1
  %637 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %459, ptr %637, align 8
  %638 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %637, 1
  %639 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %636, ptr %639, align 8
  %640 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %638, ptr %640, align 8
  call void @memrefCopy(i64 4, ptr %639, ptr %640)
  call void @llvm.stackrestore.p0(ptr %634)
  %641 = call ptr @llvm.stacksave.p0()
  %642 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %488, ptr %642, align 8
  %643 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %642, 1
  %644 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %497, ptr %644, align 8
  %645 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %644, 1
  %646 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %643, ptr %646, align 8
  %647 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %645, ptr %647, align 8
  call void @memrefCopy(i64 4, ptr %646, ptr %647)
  call void @llvm.stackrestore.p0(ptr %641)
  %648 = call i32 @fpga_tile_submit(ptr %58, i32 5, i32 1, i32 2, i32 4, i32 15, i32 4, ptr %469, ptr %507, ptr %580, i32 16, i32 3, i32 42, i32 116)
  %649 = add i64 %56, 816
  %650 = inttoptr i64 %649 to ptr
  %651 = call ptr @llvm.stacksave.p0()
  %652 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %599, ptr %652, align 8
  %653 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %652, 1
  %654 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %382, ptr %654, align 8
  %655 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %654, 1
  %656 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %653, ptr %656, align 8
  %657 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %655, ptr %657, align 8
  call void @memrefCopy(i64 4, ptr %656, ptr %657)
  call void @llvm.stackrestore.p0(ptr %651)
  %658 = call ptr @llvm.stacksave.p0()
  %659 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %536, ptr %659, align 8
  %660 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %659, 1
  %661 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %420, ptr %661, align 8
  %662 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %661, 1
  %663 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %660, ptr %663, align 8
  %664 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %662, ptr %664, align 8
  call void @memrefCopy(i64 4, ptr %663, ptr %664)
  call void @llvm.stackrestore.p0(ptr %658)
  %665 = call i32 @fpga_tile_submit(ptr %58, i32 6, i32 0, i32 2, i32 4, i32 16, i32 4, ptr %392, ptr %430, ptr %650, i32 16, i32 1, i32 120, i32 117)
  %666 = call ptr @llvm.stacksave.p0()
  %667 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %633, ptr %667, align 8
  %668 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %667, 1
  %669 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %459, ptr %669, align 8
  %670 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %669, 1
  %671 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %668, ptr %671, align 8
  %672 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %670, ptr %672, align 8
  call void @memrefCopy(i64 4, ptr %671, ptr %672)
  call void @llvm.stackrestore.p0(ptr %666)
  %673 = call ptr @llvm.stacksave.p0()
  %674 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %570, ptr %674, align 8
  %675 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %674, 1
  %676 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %497, ptr %676, align 8
  %677 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %676, 1
  %678 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %675, ptr %678, align 8
  %679 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %677, ptr %679, align 8
  call void @memrefCopy(i64 4, ptr %678, ptr %679)
  call void @llvm.stackrestore.p0(ptr %673)
  %680 = call i32 @fpga_tile_submit(ptr %58, i32 6, i32 1, i32 2, i32 4, i32 15, i32 4, ptr %469, ptr %507, ptr %650, i32 16, i32 1, i32 189, i32 116)
  %681 = call i32 @fpga_gemm_wait(ptr %58)
  %682 = call ptr @malloc(i64 1024)
  %683 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %682, 0
  %684 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %683, ptr %682, 1
  %685 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %684, i64 0, 2
  %686 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %685, i64 16, 3, 0
  %687 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %686, i64 16, 3, 1
  %688 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %687, i64 16, 4, 0
  %689 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %688, i64 1, 4, 1
  %690 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %691 = mul i64 1, %690
  %692 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %693 = mul i64 %691, %692
  %694 = mul i64 %693, 4
  %695 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %696 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %697 = getelementptr float, ptr %695, i64 %696
  %698 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %689, 1
  %699 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %689, 2
  %700 = getelementptr float, ptr %698, i64 %699
  call void @llvm.memcpy.p0.p0.i64(ptr %700, ptr %697, i64 %694, i1 false)
  ret { ptr, ptr, i64, [2 x i64], [2 x i64] } %689
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

; Function Attrs: nocallback nofree nosync nounwind willreturn
declare ptr @llvm.stacksave.p0() #0

; Function Attrs: nocallback nofree nosync nounwind willreturn
declare void @llvm.stackrestore.p0(ptr) #0

; Function Attrs: nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
declare void @llvm.memcpy.p0.p0.i64(ptr noalias writeonly captures(none), ptr noalias readonly captures(none), i64, i1 immarg) #1

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

attributes #0 = { nocallback nofree nosync nounwind willreturn }
attributes #1 = { nocallback nofree nosync nounwind willreturn memory(argmem: readwrite) }

!llvm.module.flags = !{!0}

!0 = !{i32 2, !"Debug Info Version", i32 3}
