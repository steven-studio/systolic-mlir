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
  %58 = call ptr @fpga_gemm_begin(i32 12, i32 12, i32 35, i32 18)
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
  %75 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %74, i64 35, 4, 0
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
  %113 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %112, i64 12, 4, 0
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
  %135 = call i32 @fpga_tile_submit(ptr %58, i32 0, i32 0, i32 3, i32 8, i32 16, i32 8, ptr %96, ptr %134, ptr %57, i32 12, i32 0, i32 0, i32 125)
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
  %152 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %151, i64 35, 4, 0
  %153 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %152, i64 16, 3, 1
  %154 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %153, i64 1, 4, 1
  %155 = call ptr @llvm.stacksave.p0()
  %156 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %154, ptr %156, align 8
  %157 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %156, 1
  %158 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %86, ptr %158, align 8
  %159 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %158, 1
  %160 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %157, ptr %160, align 8
  %161 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %159, ptr %161, align 8
  call void @memrefCopy(i64 4, ptr %160, ptr %161)
  call void @llvm.stackrestore.p0(ptr %155)
  %162 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %163 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %164 = insertvalue { ptr, ptr, i64 } poison, ptr %162, 0
  %165 = insertvalue { ptr, ptr, i64 } %164, ptr %163, 1
  %166 = insertvalue { ptr, ptr, i64 } %165, i64 0, 2
  %167 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %168 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %169 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %170 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %171 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %172 = extractvalue { ptr, ptr, i64 } %166, 0
  %173 = extractvalue { ptr, ptr, i64 } %166, 1
  %174 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %172, 0
  %175 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %174, ptr %173, 1
  %176 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %175, i64 192, 2
  %177 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %176, i64 16, 3, 0
  %178 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %177, i64 12, 4, 0
  %179 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %178, i64 8, 3, 1
  %180 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %179, i64 1, 4, 1
  %181 = call ptr @llvm.stacksave.p0()
  %182 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %180, ptr %182, align 8
  %183 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %182, 1
  %184 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %124, ptr %184, align 8
  %185 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %184, 1
  %186 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %183, ptr %186, align 8
  %187 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %185, ptr %187, align 8
  call void @memrefCopy(i64 4, ptr %186, ptr %187)
  call void @llvm.stackrestore.p0(ptr %181)
  %188 = call i32 @fpga_tile_submit(ptr %58, i32 0, i32 1, i32 3, i32 8, i32 16, i32 8, ptr %96, ptr %134, ptr %57, i32 12, i32 0, i32 42, i32 125)
  %189 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %190 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %191 = insertvalue { ptr, ptr, i64 } poison, ptr %189, 0
  %192 = insertvalue { ptr, ptr, i64 } %191, ptr %190, 1
  %193 = insertvalue { ptr, ptr, i64 } %192, i64 0, 2
  %194 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %195 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %196 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %197 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %198 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %199 = extractvalue { ptr, ptr, i64 } %193, 0
  %200 = extractvalue { ptr, ptr, i64 } %193, 1
  %201 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %199, 0
  %202 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %201, ptr %200, 1
  %203 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %202, i64 32, 2
  %204 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %203, i64 8, 3, 0
  %205 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %204, i64 35, 4, 0
  %206 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %205, i64 3, 3, 1
  %207 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %206, i64 1, 4, 1
  %208 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 0
  %209 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %210 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %208, 0
  %211 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %210, ptr %209, 1
  %212 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %211, i64 0, 2
  %213 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %212, i64 8, 3, 0
  %214 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %213, i64 3, 4, 0
  %215 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %214, i64 3, 3, 1
  %216 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %215, i64 1, 4, 1
  %217 = call ptr @llvm.stacksave.p0()
  %218 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %207, ptr %218, align 8
  %219 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %218, 1
  %220 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %216, ptr %220, align 8
  %221 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %220, 1
  %222 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %219, ptr %222, align 8
  %223 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %221, ptr %223, align 8
  call void @memrefCopy(i64 4, ptr %222, ptr %223)
  call void @llvm.stackrestore.p0(ptr %217)
  %224 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %225 = ptrtoint ptr %224 to i64
  %226 = inttoptr i64 %225 to ptr
  %227 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %228 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %229 = insertvalue { ptr, ptr, i64 } poison, ptr %227, 0
  %230 = insertvalue { ptr, ptr, i64 } %229, ptr %228, 1
  %231 = insertvalue { ptr, ptr, i64 } %230, i64 0, 2
  %232 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %233 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %234 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %235 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %236 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %237 = extractvalue { ptr, ptr, i64 } %231, 0
  %238 = extractvalue { ptr, ptr, i64 } %231, 1
  %239 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %237, 0
  %240 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %239, ptr %238, 1
  %241 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %240, i64 384, 2
  %242 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %241, i64 3, 3, 0
  %243 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %242, i64 12, 4, 0
  %244 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %243, i64 8, 3, 1
  %245 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %244, i64 1, 4, 1
  %246 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 0
  %247 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %248 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %246, 0
  %249 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %248, ptr %247, 1
  %250 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %249, i64 0, 2
  %251 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %250, i64 3, 3, 0
  %252 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %251, i64 8, 4, 0
  %253 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %252, i64 8, 3, 1
  %254 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %253, i64 1, 4, 1
  %255 = call ptr @llvm.stacksave.p0()
  %256 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %245, ptr %256, align 8
  %257 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %256, 1
  %258 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %254, ptr %258, align 8
  %259 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %258, 1
  %260 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %257, ptr %260, align 8
  %261 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %259, ptr %261, align 8
  call void @memrefCopy(i64 4, ptr %260, ptr %261)
  call void @llvm.stackrestore.p0(ptr %255)
  %262 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %263 = ptrtoint ptr %262 to i64
  %264 = inttoptr i64 %263 to ptr
  %265 = call i32 @fpga_tile_submit(ptr %58, i32 0, i32 2, i32 3, i32 8, i32 3, i32 8, ptr %226, ptr %264, ptr %57, i32 12, i32 0, i32 128, i32 112)
  %266 = add i64 %56, 32
  %267 = inttoptr i64 %266 to ptr
  %268 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %269 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %270 = insertvalue { ptr, ptr, i64 } poison, ptr %268, 0
  %271 = insertvalue { ptr, ptr, i64 } %270, ptr %269, 1
  %272 = insertvalue { ptr, ptr, i64 } %271, i64 0, 2
  %273 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %274 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %275 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %276 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %277 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %278 = extractvalue { ptr, ptr, i64 } %272, 0
  %279 = extractvalue { ptr, ptr, i64 } %272, 1
  %280 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %278, 0
  %281 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %280, ptr %279, 1
  %282 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %281, i64 0, 2
  %283 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %282, i64 4, 3, 0
  %284 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %283, i64 35, 4, 0
  %285 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %284, i64 16, 3, 1
  %286 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %285, i64 1, 4, 1
  %287 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 0
  %288 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %289 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %287, 0
  %290 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %289, ptr %288, 1
  %291 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %290, i64 0, 2
  %292 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %291, i64 4, 3, 0
  %293 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %292, i64 16, 4, 0
  %294 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %293, i64 16, 3, 1
  %295 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %294, i64 1, 4, 1
  %296 = call ptr @llvm.stacksave.p0()
  %297 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %286, ptr %297, align 8
  %298 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %297, 1
  %299 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %295, ptr %299, align 8
  %300 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %299, 1
  %301 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %298, ptr %301, align 8
  %302 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %300, ptr %302, align 8
  call void @memrefCopy(i64 4, ptr %301, ptr %302)
  call void @llvm.stackrestore.p0(ptr %296)
  %303 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %304 = ptrtoint ptr %303 to i64
  %305 = inttoptr i64 %304 to ptr
  %306 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %307 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %308 = insertvalue { ptr, ptr, i64 } poison, ptr %306, 0
  %309 = insertvalue { ptr, ptr, i64 } %308, ptr %307, 1
  %310 = insertvalue { ptr, ptr, i64 } %309, i64 0, 2
  %311 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %312 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %313 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %314 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %315 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %316 = extractvalue { ptr, ptr, i64 } %310, 0
  %317 = extractvalue { ptr, ptr, i64 } %310, 1
  %318 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %316, 0
  %319 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %318, ptr %317, 1
  %320 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %319, i64 8, 2
  %321 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %320, i64 16, 3, 0
  %322 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %321, i64 12, 4, 0
  %323 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %322, i64 4, 3, 1
  %324 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %323, i64 1, 4, 1
  %325 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 0
  %326 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %327 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %325, 0
  %328 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %327, ptr %326, 1
  %329 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %328, i64 0, 2
  %330 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %329, i64 16, 3, 0
  %331 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %330, i64 4, 4, 0
  %332 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %331, i64 4, 3, 1
  %333 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %332, i64 1, 4, 1
  %334 = call ptr @llvm.stacksave.p0()
  %335 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %324, ptr %335, align 8
  %336 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %335, 1
  %337 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %333, ptr %337, align 8
  %338 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %337, 1
  %339 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %336, ptr %339, align 8
  %340 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %338, ptr %340, align 8
  call void @memrefCopy(i64 4, ptr %339, ptr %340)
  call void @llvm.stackrestore.p0(ptr %334)
  %341 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %342 = ptrtoint ptr %341 to i64
  %343 = inttoptr i64 %342 to ptr
  %344 = call i32 @fpga_tile_submit(ptr %58, i32 1, i32 0, i32 3, i32 4, i32 16, i32 4, ptr %305, ptr %343, ptr %267, i32 12, i32 1, i32 0, i32 117)
  %345 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %346 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %347 = insertvalue { ptr, ptr, i64 } poison, ptr %345, 0
  %348 = insertvalue { ptr, ptr, i64 } %347, ptr %346, 1
  %349 = insertvalue { ptr, ptr, i64 } %348, i64 0, 2
  %350 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %351 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %352 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %353 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %354 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %355 = extractvalue { ptr, ptr, i64 } %349, 0
  %356 = extractvalue { ptr, ptr, i64 } %349, 1
  %357 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %355, 0
  %358 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %357, ptr %356, 1
  %359 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %358, i64 16, 2
  %360 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %359, i64 4, 3, 0
  %361 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %360, i64 35, 4, 0
  %362 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %361, i64 16, 3, 1
  %363 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %362, i64 1, 4, 1
  %364 = call ptr @llvm.stacksave.p0()
  %365 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %363, ptr %365, align 8
  %366 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %365, 1
  %367 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %295, ptr %367, align 8
  %368 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %367, 1
  %369 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %366, ptr %369, align 8
  %370 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %368, ptr %370, align 8
  call void @memrefCopy(i64 4, ptr %369, ptr %370)
  call void @llvm.stackrestore.p0(ptr %364)
  %371 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %372 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %373 = insertvalue { ptr, ptr, i64 } poison, ptr %371, 0
  %374 = insertvalue { ptr, ptr, i64 } %373, ptr %372, 1
  %375 = insertvalue { ptr, ptr, i64 } %374, i64 0, 2
  %376 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %377 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %378 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %379 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %380 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %381 = extractvalue { ptr, ptr, i64 } %375, 0
  %382 = extractvalue { ptr, ptr, i64 } %375, 1
  %383 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %381, 0
  %384 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %383, ptr %382, 1
  %385 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %384, i64 200, 2
  %386 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %385, i64 16, 3, 0
  %387 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %386, i64 12, 4, 0
  %388 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %387, i64 4, 3, 1
  %389 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %388, i64 1, 4, 1
  %390 = call ptr @llvm.stacksave.p0()
  %391 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %389, ptr %391, align 8
  %392 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %391, 1
  %393 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %333, ptr %393, align 8
  %394 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %393, 1
  %395 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %392, ptr %395, align 8
  %396 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %394, ptr %396, align 8
  call void @memrefCopy(i64 4, ptr %395, ptr %396)
  call void @llvm.stackrestore.p0(ptr %390)
  %397 = call i32 @fpga_tile_submit(ptr %58, i32 1, i32 1, i32 3, i32 4, i32 16, i32 4, ptr %305, ptr %343, ptr %267, i32 12, i32 1, i32 42, i32 117)
  %398 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %399 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %400 = insertvalue { ptr, ptr, i64 } poison, ptr %398, 0
  %401 = insertvalue { ptr, ptr, i64 } %400, ptr %399, 1
  %402 = insertvalue { ptr, ptr, i64 } %401, i64 0, 2
  %403 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %404 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %405 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %406 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %407 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %408 = extractvalue { ptr, ptr, i64 } %402, 0
  %409 = extractvalue { ptr, ptr, i64 } %402, 1
  %410 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %408, 0
  %411 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %410, ptr %409, 1
  %412 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %411, i64 32, 2
  %413 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %412, i64 4, 3, 0
  %414 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %413, i64 35, 4, 0
  %415 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %414, i64 3, 3, 1
  %416 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %415, i64 1, 4, 1
  %417 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 0
  %418 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %419 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %417, 0
  %420 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %419, ptr %418, 1
  %421 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %420, i64 0, 2
  %422 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %421, i64 4, 3, 0
  %423 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %422, i64 3, 4, 0
  %424 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %423, i64 3, 3, 1
  %425 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %424, i64 1, 4, 1
  %426 = call ptr @llvm.stacksave.p0()
  %427 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %416, ptr %427, align 8
  %428 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %427, 1
  %429 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %425, ptr %429, align 8
  %430 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %429, 1
  %431 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %428, ptr %431, align 8
  %432 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %430, ptr %432, align 8
  call void @memrefCopy(i64 4, ptr %431, ptr %432)
  call void @llvm.stackrestore.p0(ptr %426)
  %433 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %434 = ptrtoint ptr %433 to i64
  %435 = inttoptr i64 %434 to ptr
  %436 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %437 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %438 = insertvalue { ptr, ptr, i64 } poison, ptr %436, 0
  %439 = insertvalue { ptr, ptr, i64 } %438, ptr %437, 1
  %440 = insertvalue { ptr, ptr, i64 } %439, i64 0, 2
  %441 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %442 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %443 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %444 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %445 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %446 = extractvalue { ptr, ptr, i64 } %440, 0
  %447 = extractvalue { ptr, ptr, i64 } %440, 1
  %448 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %446, 0
  %449 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %448, ptr %447, 1
  %450 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %449, i64 392, 2
  %451 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %450, i64 3, 3, 0
  %452 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %451, i64 12, 4, 0
  %453 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %452, i64 4, 3, 1
  %454 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %453, i64 1, 4, 1
  %455 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 0
  %456 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %457 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %455, 0
  %458 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %457, ptr %456, 1
  %459 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %458, i64 0, 2
  %460 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %459, i64 3, 3, 0
  %461 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %460, i64 4, 4, 0
  %462 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %461, i64 4, 3, 1
  %463 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %462, i64 1, 4, 1
  %464 = call ptr @llvm.stacksave.p0()
  %465 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %454, ptr %465, align 8
  %466 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %465, 1
  %467 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %463, ptr %467, align 8
  %468 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %467, 1
  %469 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %466, ptr %469, align 8
  %470 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %468, ptr %470, align 8
  call void @memrefCopy(i64 4, ptr %469, ptr %470)
  call void @llvm.stackrestore.p0(ptr %464)
  %471 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %472 = ptrtoint ptr %471 to i64
  %473 = inttoptr i64 %472 to ptr
  %474 = call i32 @fpga_tile_submit(ptr %58, i32 1, i32 2, i32 3, i32 4, i32 3, i32 4, ptr %435, ptr %473, ptr %267, i32 12, i32 1, i32 120, i32 104)
  %475 = add i64 %56, 224
  %476 = inttoptr i64 %475 to ptr
  %477 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %478 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %479 = insertvalue { ptr, ptr, i64 } poison, ptr %477, 0
  %480 = insertvalue { ptr, ptr, i64 } %479, ptr %478, 1
  %481 = insertvalue { ptr, ptr, i64 } %480, i64 0, 2
  %482 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %483 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %484 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %485 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %486 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %487 = extractvalue { ptr, ptr, i64 } %481, 0
  %488 = extractvalue { ptr, ptr, i64 } %481, 1
  %489 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %487, 0
  %490 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %489, ptr %488, 1
  %491 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %490, i64 140, 2
  %492 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %491, i64 4, 3, 0
  %493 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %492, i64 35, 4, 0
  %494 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %493, i64 16, 3, 1
  %495 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %494, i64 1, 4, 1
  %496 = call ptr @llvm.stacksave.p0()
  %497 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %495, ptr %497, align 8
  %498 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %497, 1
  %499 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %295, ptr %499, align 8
  %500 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %499, 1
  %501 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %498, ptr %501, align 8
  %502 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %500, ptr %502, align 8
  call void @memrefCopy(i64 4, ptr %501, ptr %502)
  call void @llvm.stackrestore.p0(ptr %496)
  %503 = call ptr @llvm.stacksave.p0()
  %504 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %324, ptr %504, align 8
  %505 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %504, 1
  %506 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %333, ptr %506, align 8
  %507 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %506, 1
  %508 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %505, ptr %508, align 8
  %509 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %507, ptr %509, align 8
  call void @memrefCopy(i64 4, ptr %508, ptr %509)
  call void @llvm.stackrestore.p0(ptr %503)
  %510 = call i32 @fpga_tile_submit(ptr %58, i32 2, i32 0, i32 3, i32 4, i32 16, i32 4, ptr %305, ptr %343, ptr %476, i32 12, i32 2, i32 0, i32 117)
  %511 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %512 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %513 = insertvalue { ptr, ptr, i64 } poison, ptr %511, 0
  %514 = insertvalue { ptr, ptr, i64 } %513, ptr %512, 1
  %515 = insertvalue { ptr, ptr, i64 } %514, i64 0, 2
  %516 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %517 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %518 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %519 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %520 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %521 = extractvalue { ptr, ptr, i64 } %515, 0
  %522 = extractvalue { ptr, ptr, i64 } %515, 1
  %523 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %521, 0
  %524 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %523, ptr %522, 1
  %525 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %524, i64 156, 2
  %526 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %525, i64 4, 3, 0
  %527 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %526, i64 35, 4, 0
  %528 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %527, i64 16, 3, 1
  %529 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %528, i64 1, 4, 1
  %530 = call ptr @llvm.stacksave.p0()
  %531 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %529, ptr %531, align 8
  %532 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %531, 1
  %533 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %295, ptr %533, align 8
  %534 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %533, 1
  %535 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %532, ptr %535, align 8
  %536 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %534, ptr %536, align 8
  call void @memrefCopy(i64 4, ptr %535, ptr %536)
  call void @llvm.stackrestore.p0(ptr %530)
  %537 = call ptr @llvm.stacksave.p0()
  %538 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %389, ptr %538, align 8
  %539 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %538, 1
  %540 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %333, ptr %540, align 8
  %541 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %540, 1
  %542 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %539, ptr %542, align 8
  %543 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %541, ptr %543, align 8
  call void @memrefCopy(i64 4, ptr %542, ptr %543)
  call void @llvm.stackrestore.p0(ptr %537)
  %544 = call i32 @fpga_tile_submit(ptr %58, i32 2, i32 1, i32 3, i32 4, i32 16, i32 4, ptr %305, ptr %343, ptr %476, i32 12, i32 2, i32 42, i32 117)
  %545 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %546 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %547 = insertvalue { ptr, ptr, i64 } poison, ptr %545, 0
  %548 = insertvalue { ptr, ptr, i64 } %547, ptr %546, 1
  %549 = insertvalue { ptr, ptr, i64 } %548, i64 0, 2
  %550 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %551 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %552 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %553 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %554 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %555 = extractvalue { ptr, ptr, i64 } %549, 0
  %556 = extractvalue { ptr, ptr, i64 } %549, 1
  %557 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %555, 0
  %558 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %557, ptr %556, 1
  %559 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %558, i64 172, 2
  %560 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %559, i64 4, 3, 0
  %561 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %560, i64 35, 4, 0
  %562 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %561, i64 3, 3, 1
  %563 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %562, i64 1, 4, 1
  %564 = call ptr @llvm.stacksave.p0()
  %565 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %563, ptr %565, align 8
  %566 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %565, 1
  %567 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %425, ptr %567, align 8
  %568 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %567, 1
  %569 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %566, ptr %569, align 8
  %570 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %568, ptr %570, align 8
  call void @memrefCopy(i64 4, ptr %569, ptr %570)
  call void @llvm.stackrestore.p0(ptr %564)
  %571 = call ptr @llvm.stacksave.p0()
  %572 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %454, ptr %572, align 8
  %573 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %572, 1
  %574 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %463, ptr %574, align 8
  %575 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %574, 1
  %576 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %573, ptr %576, align 8
  %577 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %575, ptr %577, align 8
  call void @memrefCopy(i64 4, ptr %576, ptr %577)
  call void @llvm.stackrestore.p0(ptr %571)
  %578 = call i32 @fpga_tile_submit(ptr %58, i32 2, i32 2, i32 3, i32 4, i32 3, i32 4, ptr %435, ptr %473, ptr %476, i32 12, i32 2, i32 120, i32 104)
  %579 = add i64 %56, 384
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
  %595 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %594, i64 280, 2
  %596 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %595, i64 4, 3, 0
  %597 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %596, i64 35, 4, 0
  %598 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %597, i64 16, 3, 1
  %599 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %598, i64 1, 4, 1
  %600 = call ptr @llvm.stacksave.p0()
  %601 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %599, ptr %601, align 8
  %602 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %601, 1
  %603 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %295, ptr %603, align 8
  %604 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %603, 1
  %605 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %602, ptr %605, align 8
  %606 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %604, ptr %606, align 8
  call void @memrefCopy(i64 4, ptr %605, ptr %606)
  call void @llvm.stackrestore.p0(ptr %600)
  %607 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %608 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %609 = insertvalue { ptr, ptr, i64 } poison, ptr %607, 0
  %610 = insertvalue { ptr, ptr, i64 } %609, ptr %608, 1
  %611 = insertvalue { ptr, ptr, i64 } %610, i64 0, 2
  %612 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %613 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %614 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %615 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %616 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %617 = extractvalue { ptr, ptr, i64 } %611, 0
  %618 = extractvalue { ptr, ptr, i64 } %611, 1
  %619 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %617, 0
  %620 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %619, ptr %618, 1
  %621 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %620, i64 0, 2
  %622 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %621, i64 16, 3, 0
  %623 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %622, i64 12, 4, 0
  %624 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %623, i64 4, 3, 1
  %625 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %624, i64 1, 4, 1
  %626 = call ptr @llvm.stacksave.p0()
  %627 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %625, ptr %627, align 8
  %628 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %627, 1
  %629 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %333, ptr %629, align 8
  %630 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %629, 1
  %631 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %628, ptr %631, align 8
  %632 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %630, ptr %632, align 8
  call void @memrefCopy(i64 4, ptr %631, ptr %632)
  call void @llvm.stackrestore.p0(ptr %626)
  %633 = call i32 @fpga_tile_submit(ptr %58, i32 3, i32 0, i32 3, i32 4, i32 16, i32 4, ptr %305, ptr %343, ptr %580, i32 12, i32 3, i32 0, i32 117)
  %634 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %635 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %636 = insertvalue { ptr, ptr, i64 } poison, ptr %634, 0
  %637 = insertvalue { ptr, ptr, i64 } %636, ptr %635, 1
  %638 = insertvalue { ptr, ptr, i64 } %637, i64 0, 2
  %639 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %640 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %641 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %642 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %643 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %644 = extractvalue { ptr, ptr, i64 } %638, 0
  %645 = extractvalue { ptr, ptr, i64 } %638, 1
  %646 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %644, 0
  %647 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %646, ptr %645, 1
  %648 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %647, i64 296, 2
  %649 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %648, i64 4, 3, 0
  %650 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %649, i64 35, 4, 0
  %651 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %650, i64 16, 3, 1
  %652 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %651, i64 1, 4, 1
  %653 = call ptr @llvm.stacksave.p0()
  %654 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %652, ptr %654, align 8
  %655 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %654, 1
  %656 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %295, ptr %656, align 8
  %657 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %656, 1
  %658 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %655, ptr %658, align 8
  %659 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %657, ptr %659, align 8
  call void @memrefCopy(i64 4, ptr %658, ptr %659)
  call void @llvm.stackrestore.p0(ptr %653)
  %660 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %661 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %662 = insertvalue { ptr, ptr, i64 } poison, ptr %660, 0
  %663 = insertvalue { ptr, ptr, i64 } %662, ptr %661, 1
  %664 = insertvalue { ptr, ptr, i64 } %663, i64 0, 2
  %665 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %666 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %667 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %668 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %669 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %670 = extractvalue { ptr, ptr, i64 } %664, 0
  %671 = extractvalue { ptr, ptr, i64 } %664, 1
  %672 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %670, 0
  %673 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %672, ptr %671, 1
  %674 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %673, i64 192, 2
  %675 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %674, i64 16, 3, 0
  %676 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %675, i64 12, 4, 0
  %677 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %676, i64 4, 3, 1
  %678 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %677, i64 1, 4, 1
  %679 = call ptr @llvm.stacksave.p0()
  %680 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %678, ptr %680, align 8
  %681 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %680, 1
  %682 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %333, ptr %682, align 8
  %683 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %682, 1
  %684 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %681, ptr %684, align 8
  %685 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %683, ptr %685, align 8
  call void @memrefCopy(i64 4, ptr %684, ptr %685)
  call void @llvm.stackrestore.p0(ptr %679)
  %686 = call i32 @fpga_tile_submit(ptr %58, i32 3, i32 1, i32 3, i32 4, i32 16, i32 4, ptr %305, ptr %343, ptr %580, i32 12, i32 3, i32 42, i32 117)
  %687 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %688 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %689 = insertvalue { ptr, ptr, i64 } poison, ptr %687, 0
  %690 = insertvalue { ptr, ptr, i64 } %689, ptr %688, 1
  %691 = insertvalue { ptr, ptr, i64 } %690, i64 0, 2
  %692 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %693 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %694 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %695 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %696 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %697 = extractvalue { ptr, ptr, i64 } %691, 0
  %698 = extractvalue { ptr, ptr, i64 } %691, 1
  %699 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %697, 0
  %700 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %699, ptr %698, 1
  %701 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %700, i64 312, 2
  %702 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %701, i64 4, 3, 0
  %703 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %702, i64 35, 4, 0
  %704 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %703, i64 3, 3, 1
  %705 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %704, i64 1, 4, 1
  %706 = call ptr @llvm.stacksave.p0()
  %707 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %705, ptr %707, align 8
  %708 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %707, 1
  %709 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %425, ptr %709, align 8
  %710 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %709, 1
  %711 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %708, ptr %711, align 8
  %712 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %710, ptr %712, align 8
  call void @memrefCopy(i64 4, ptr %711, ptr %712)
  call void @llvm.stackrestore.p0(ptr %706)
  %713 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %714 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %715 = insertvalue { ptr, ptr, i64 } poison, ptr %713, 0
  %716 = insertvalue { ptr, ptr, i64 } %715, ptr %714, 1
  %717 = insertvalue { ptr, ptr, i64 } %716, i64 0, 2
  %718 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %719 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %720 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %721 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %722 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %723 = extractvalue { ptr, ptr, i64 } %717, 0
  %724 = extractvalue { ptr, ptr, i64 } %717, 1
  %725 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %723, 0
  %726 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %725, ptr %724, 1
  %727 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %726, i64 384, 2
  %728 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %727, i64 3, 3, 0
  %729 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %728, i64 12, 4, 0
  %730 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %729, i64 4, 3, 1
  %731 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %730, i64 1, 4, 1
  %732 = call ptr @llvm.stacksave.p0()
  %733 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %731, ptr %733, align 8
  %734 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %733, 1
  %735 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %463, ptr %735, align 8
  %736 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %735, 1
  %737 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %734, ptr %737, align 8
  %738 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %736, ptr %738, align 8
  call void @memrefCopy(i64 4, ptr %737, ptr %738)
  call void @llvm.stackrestore.p0(ptr %732)
  %739 = call i32 @fpga_tile_submit(ptr %58, i32 3, i32 2, i32 3, i32 4, i32 3, i32 4, ptr %435, ptr %473, ptr %580, i32 12, i32 3, i32 120, i32 104)
  %740 = add i64 %56, 400
  %741 = inttoptr i64 %740 to ptr
  %742 = call ptr @llvm.stacksave.p0()
  %743 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %599, ptr %743, align 8
  %744 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %743, 1
  %745 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %295, ptr %745, align 8
  %746 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %745, 1
  %747 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %744, ptr %747, align 8
  %748 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %746, ptr %748, align 8
  call void @memrefCopy(i64 4, ptr %747, ptr %748)
  call void @llvm.stackrestore.p0(ptr %742)
  %749 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %750 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %751 = insertvalue { ptr, ptr, i64 } poison, ptr %749, 0
  %752 = insertvalue { ptr, ptr, i64 } %751, ptr %750, 1
  %753 = insertvalue { ptr, ptr, i64 } %752, i64 0, 2
  %754 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %755 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %756 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %757 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %758 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %759 = extractvalue { ptr, ptr, i64 } %753, 0
  %760 = extractvalue { ptr, ptr, i64 } %753, 1
  %761 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %759, 0
  %762 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %761, ptr %760, 1
  %763 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %762, i64 4, 2
  %764 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %763, i64 16, 3, 0
  %765 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %764, i64 12, 4, 0
  %766 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %765, i64 4, 3, 1
  %767 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %766, i64 1, 4, 1
  %768 = call ptr @llvm.stacksave.p0()
  %769 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %767, ptr %769, align 8
  %770 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %769, 1
  %771 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %333, ptr %771, align 8
  %772 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %771, 1
  %773 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %770, ptr %773, align 8
  %774 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %772, ptr %774, align 8
  call void @memrefCopy(i64 4, ptr %773, ptr %774)
  call void @llvm.stackrestore.p0(ptr %768)
  %775 = call i32 @fpga_tile_submit(ptr %58, i32 4, i32 0, i32 3, i32 4, i32 16, i32 4, ptr %305, ptr %343, ptr %741, i32 12, i32 1, i32 189, i32 117)
  %776 = call ptr @llvm.stacksave.p0()
  %777 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %652, ptr %777, align 8
  %778 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %777, 1
  %779 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %295, ptr %779, align 8
  %780 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %779, 1
  %781 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %778, ptr %781, align 8
  %782 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %780, ptr %782, align 8
  call void @memrefCopy(i64 4, ptr %781, ptr %782)
  call void @llvm.stackrestore.p0(ptr %776)
  %783 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %784 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %785 = insertvalue { ptr, ptr, i64 } poison, ptr %783, 0
  %786 = insertvalue { ptr, ptr, i64 } %785, ptr %784, 1
  %787 = insertvalue { ptr, ptr, i64 } %786, i64 0, 2
  %788 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %789 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %790 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %791 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %792 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %793 = extractvalue { ptr, ptr, i64 } %787, 0
  %794 = extractvalue { ptr, ptr, i64 } %787, 1
  %795 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %793, 0
  %796 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %795, ptr %794, 1
  %797 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %796, i64 196, 2
  %798 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %797, i64 16, 3, 0
  %799 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %798, i64 12, 4, 0
  %800 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %799, i64 4, 3, 1
  %801 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %800, i64 1, 4, 1
  %802 = call ptr @llvm.stacksave.p0()
  %803 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %801, ptr %803, align 8
  %804 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %803, 1
  %805 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %333, ptr %805, align 8
  %806 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %805, 1
  %807 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %804, ptr %807, align 8
  %808 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %806, ptr %808, align 8
  call void @memrefCopy(i64 4, ptr %807, ptr %808)
  call void @llvm.stackrestore.p0(ptr %802)
  %809 = call i32 @fpga_tile_submit(ptr %58, i32 4, i32 1, i32 3, i32 4, i32 16, i32 4, ptr %305, ptr %343, ptr %741, i32 12, i32 1, i32 258, i32 117)
  %810 = call ptr @llvm.stacksave.p0()
  %811 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %705, ptr %811, align 8
  %812 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %811, 1
  %813 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %425, ptr %813, align 8
  %814 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %813, 1
  %815 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %812, ptr %815, align 8
  %816 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %814, ptr %816, align 8
  call void @memrefCopy(i64 4, ptr %815, ptr %816)
  call void @llvm.stackrestore.p0(ptr %810)
  %817 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %818 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %819 = insertvalue { ptr, ptr, i64 } poison, ptr %817, 0
  %820 = insertvalue { ptr, ptr, i64 } %819, ptr %818, 1
  %821 = insertvalue { ptr, ptr, i64 } %820, i64 0, 2
  %822 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %823 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %824 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %825 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %826 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %827 = extractvalue { ptr, ptr, i64 } %821, 0
  %828 = extractvalue { ptr, ptr, i64 } %821, 1
  %829 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %827, 0
  %830 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %829, ptr %828, 1
  %831 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %830, i64 388, 2
  %832 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %831, i64 3, 3, 0
  %833 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %832, i64 12, 4, 0
  %834 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %833, i64 4, 3, 1
  %835 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %834, i64 1, 4, 1
  %836 = call ptr @llvm.stacksave.p0()
  %837 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %835, ptr %837, align 8
  %838 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %837, 1
  %839 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %463, ptr %839, align 8
  %840 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %839, 1
  %841 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %838, ptr %841, align 8
  %842 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %840, ptr %842, align 8
  call void @memrefCopy(i64 4, ptr %841, ptr %842)
  call void @llvm.stackrestore.p0(ptr %836)
  %843 = call i32 @fpga_tile_submit(ptr %58, i32 4, i32 2, i32 3, i32 4, i32 3, i32 4, ptr %435, ptr %473, ptr %741, i32 12, i32 1, i32 327, i32 104)
  %844 = add i64 %56, 416
  %845 = inttoptr i64 %844 to ptr
  %846 = call ptr @llvm.stacksave.p0()
  %847 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %599, ptr %847, align 8
  %848 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %847, 1
  %849 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %295, ptr %849, align 8
  %850 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %849, 1
  %851 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %848, ptr %851, align 8
  %852 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %850, ptr %852, align 8
  call void @memrefCopy(i64 4, ptr %851, ptr %852)
  call void @llvm.stackrestore.p0(ptr %846)
  %853 = call ptr @llvm.stacksave.p0()
  %854 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %324, ptr %854, align 8
  %855 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %854, 1
  %856 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %333, ptr %856, align 8
  %857 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %856, 1
  %858 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %855, ptr %858, align 8
  %859 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %857, ptr %859, align 8
  call void @memrefCopy(i64 4, ptr %858, ptr %859)
  call void @llvm.stackrestore.p0(ptr %853)
  %860 = call i32 @fpga_tile_submit(ptr %58, i32 5, i32 0, i32 3, i32 4, i32 16, i32 4, ptr %305, ptr %343, ptr %845, i32 12, i32 2, i32 189, i32 117)
  %861 = call ptr @llvm.stacksave.p0()
  %862 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %652, ptr %862, align 8
  %863 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %862, 1
  %864 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %295, ptr %864, align 8
  %865 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %864, 1
  %866 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %863, ptr %866, align 8
  %867 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %865, ptr %867, align 8
  call void @memrefCopy(i64 4, ptr %866, ptr %867)
  call void @llvm.stackrestore.p0(ptr %861)
  %868 = call ptr @llvm.stacksave.p0()
  %869 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %389, ptr %869, align 8
  %870 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %869, 1
  %871 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %333, ptr %871, align 8
  %872 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %871, 1
  %873 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %870, ptr %873, align 8
  %874 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %872, ptr %874, align 8
  call void @memrefCopy(i64 4, ptr %873, ptr %874)
  call void @llvm.stackrestore.p0(ptr %868)
  %875 = call i32 @fpga_tile_submit(ptr %58, i32 5, i32 1, i32 3, i32 4, i32 16, i32 4, ptr %305, ptr %343, ptr %845, i32 12, i32 2, i32 258, i32 117)
  %876 = call ptr @llvm.stacksave.p0()
  %877 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %705, ptr %877, align 8
  %878 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %877, 1
  %879 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %425, ptr %879, align 8
  %880 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %879, 1
  %881 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %878, ptr %881, align 8
  %882 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %880, ptr %882, align 8
  call void @memrefCopy(i64 4, ptr %881, ptr %882)
  call void @llvm.stackrestore.p0(ptr %876)
  %883 = call ptr @llvm.stacksave.p0()
  %884 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %454, ptr %884, align 8
  %885 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %884, 1
  %886 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %463, ptr %886, align 8
  %887 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %886, 1
  %888 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %885, ptr %888, align 8
  %889 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %887, ptr %889, align 8
  call void @memrefCopy(i64 4, ptr %888, ptr %889)
  call void @llvm.stackrestore.p0(ptr %883)
  %890 = call i32 @fpga_tile_submit(ptr %58, i32 5, i32 2, i32 3, i32 4, i32 3, i32 4, ptr %435, ptr %473, ptr %845, i32 12, i32 2, i32 327, i32 104)
  %891 = call i32 @fpga_gemm_wait(ptr %58)
  %892 = call ptr @malloc(i64 576)
  %893 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %892, 0
  %894 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %893, ptr %892, 1
  %895 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %894, i64 0, 2
  %896 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %895, i64 12, 3, 0
  %897 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %896, i64 12, 3, 1
  %898 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %897, i64 12, 4, 0
  %899 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %898, i64 1, 4, 1
  %900 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %901 = mul i64 1, %900
  %902 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %903 = mul i64 %901, %902
  %904 = mul i64 %903, 4
  %905 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %906 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %907 = getelementptr float, ptr %905, i64 %906
  %908 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %899, 1
  %909 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %899, 2
  %910 = getelementptr float, ptr %908, i64 %909
  call void @llvm.memcpy.p0.p0.i64(ptr %910, ptr %907, i64 %904, i1 false)
  ret { ptr, ptr, i64, [2 x i64], [2 x i64] } %899
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
