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
  %58 = call ptr @fpga_gemm_begin(i32 12, i32 12, i32 17, i32 12)
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
  %75 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %74, i64 17, 4, 0
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
  %135 = call i32 @fpga_tile_submit(ptr %58, i32 0, i32 0, i32 2, i32 8, i32 16, i32 8, ptr %96, ptr %134, ptr %57, i32 12, i32 0, i32 0, i32 125)
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
  %152 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %151, i64 17, 4, 0
  %153 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %152, i64 1, 3, 1
  %154 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %153, i64 1, 4, 1
  %155 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 0
  %156 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %157 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %155, 0
  %158 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %157, ptr %156, 1
  %159 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %158, i64 0, 2
  %160 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %159, i64 8, 3, 0
  %161 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %160, i64 1, 4, 0
  %162 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %161, i64 1, 3, 1
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
  %188 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %187, i64 192, 2
  %189 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %188, i64 1, 3, 0
  %190 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %189, i64 12, 4, 0
  %191 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %190, i64 8, 3, 1
  %192 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %191, i64 1, 4, 1
  %193 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 0
  %194 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %195 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %193, 0
  %196 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %195, ptr %194, 1
  %197 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %196, i64 0, 2
  %198 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %197, i64 1, 3, 0
  %199 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %198, i64 8, 4, 0
  %200 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %199, i64 8, 3, 1
  %201 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %200, i64 1, 4, 1
  %202 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %192, 3, 0
  %203 = mul i64 1, %202
  %204 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %192, 3, 1
  %205 = mul i64 %203, %204
  %206 = mul i64 %205, 4
  %207 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %192, 1
  %208 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %192, 2
  %209 = getelementptr float, ptr %207, i64 %208
  %210 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %201, 1
  %211 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %201, 2
  %212 = getelementptr float, ptr %210, i64 %211
  call void @llvm.memcpy.p0.p0.i64(ptr %212, ptr %209, i64 %206, i1 false)
  %213 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %214 = ptrtoint ptr %213 to i64
  %215 = inttoptr i64 %214 to ptr
  %216 = call i32 @fpga_tile_submit(ptr %58, i32 0, i32 1, i32 2, i32 8, i32 1, i32 8, ptr %173, ptr %215, ptr %57, i32 12, i32 0, i32 42, i32 110)
  %217 = add i64 %56, 32
  %218 = inttoptr i64 %217 to ptr
  %219 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %220 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %221 = insertvalue { ptr, ptr, i64 } poison, ptr %219, 0
  %222 = insertvalue { ptr, ptr, i64 } %221, ptr %220, 1
  %223 = insertvalue { ptr, ptr, i64 } %222, i64 0, 2
  %224 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %225 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %226 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %227 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %228 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %229 = extractvalue { ptr, ptr, i64 } %223, 0
  %230 = extractvalue { ptr, ptr, i64 } %223, 1
  %231 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %229, 0
  %232 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %231, ptr %230, 1
  %233 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %232, i64 0, 2
  %234 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %233, i64 4, 3, 0
  %235 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %234, i64 17, 4, 0
  %236 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %235, i64 16, 3, 1
  %237 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %236, i64 1, 4, 1
  %238 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 0
  %239 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %240 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %238, 0
  %241 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %240, ptr %239, 1
  %242 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %241, i64 0, 2
  %243 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %242, i64 4, 3, 0
  %244 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %243, i64 16, 4, 0
  %245 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %244, i64 16, 3, 1
  %246 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %245, i64 1, 4, 1
  %247 = call ptr @llvm.stacksave.p0()
  %248 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %237, ptr %248, align 8
  %249 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %248, 1
  %250 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %246, ptr %250, align 8
  %251 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %250, 1
  %252 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %249, ptr %252, align 8
  %253 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %251, ptr %253, align 8
  call void @memrefCopy(i64 4, ptr %252, ptr %253)
  call void @llvm.stackrestore.p0(ptr %247)
  %254 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %255 = ptrtoint ptr %254 to i64
  %256 = inttoptr i64 %255 to ptr
  %257 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %258 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %259 = insertvalue { ptr, ptr, i64 } poison, ptr %257, 0
  %260 = insertvalue { ptr, ptr, i64 } %259, ptr %258, 1
  %261 = insertvalue { ptr, ptr, i64 } %260, i64 0, 2
  %262 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %263 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %264 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %265 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %266 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %267 = extractvalue { ptr, ptr, i64 } %261, 0
  %268 = extractvalue { ptr, ptr, i64 } %261, 1
  %269 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %267, 0
  %270 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %269, ptr %268, 1
  %271 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %270, i64 8, 2
  %272 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %271, i64 16, 3, 0
  %273 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %272, i64 12, 4, 0
  %274 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %273, i64 4, 3, 1
  %275 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %274, i64 1, 4, 1
  %276 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 0
  %277 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %278 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %276, 0
  %279 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %278, ptr %277, 1
  %280 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %279, i64 0, 2
  %281 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %280, i64 16, 3, 0
  %282 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %281, i64 4, 4, 0
  %283 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %282, i64 4, 3, 1
  %284 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %283, i64 1, 4, 1
  %285 = call ptr @llvm.stacksave.p0()
  %286 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %275, ptr %286, align 8
  %287 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %286, 1
  %288 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %284, ptr %288, align 8
  %289 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %288, 1
  %290 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %287, ptr %290, align 8
  %291 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %289, ptr %291, align 8
  call void @memrefCopy(i64 4, ptr %290, ptr %291)
  call void @llvm.stackrestore.p0(ptr %285)
  %292 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %293 = ptrtoint ptr %292 to i64
  %294 = inttoptr i64 %293 to ptr
  %295 = call i32 @fpga_tile_submit(ptr %58, i32 1, i32 0, i32 2, i32 4, i32 16, i32 4, ptr %256, ptr %294, ptr %218, i32 12, i32 1, i32 0, i32 117)
  %296 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %297 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %298 = insertvalue { ptr, ptr, i64 } poison, ptr %296, 0
  %299 = insertvalue { ptr, ptr, i64 } %298, ptr %297, 1
  %300 = insertvalue { ptr, ptr, i64 } %299, i64 0, 2
  %301 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %302 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %303 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %304 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %305 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %306 = extractvalue { ptr, ptr, i64 } %300, 0
  %307 = extractvalue { ptr, ptr, i64 } %300, 1
  %308 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %306, 0
  %309 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %308, ptr %307, 1
  %310 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %309, i64 16, 2
  %311 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %310, i64 4, 3, 0
  %312 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %311, i64 17, 4, 0
  %313 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %312, i64 1, 3, 1
  %314 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %313, i64 1, 4, 1
  %315 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 0
  %316 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %317 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %315, 0
  %318 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %317, ptr %316, 1
  %319 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %318, i64 0, 2
  %320 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %319, i64 4, 3, 0
  %321 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %320, i64 1, 4, 0
  %322 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %321, i64 1, 3, 1
  %323 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %322, i64 1, 4, 1
  %324 = call ptr @llvm.stacksave.p0()
  %325 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %314, ptr %325, align 8
  %326 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %325, 1
  %327 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %323, ptr %327, align 8
  %328 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %327, 1
  %329 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %326, ptr %329, align 8
  %330 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %328, ptr %330, align 8
  call void @memrefCopy(i64 4, ptr %329, ptr %330)
  call void @llvm.stackrestore.p0(ptr %324)
  %331 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %332 = ptrtoint ptr %331 to i64
  %333 = inttoptr i64 %332 to ptr
  %334 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %335 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %336 = insertvalue { ptr, ptr, i64 } poison, ptr %334, 0
  %337 = insertvalue { ptr, ptr, i64 } %336, ptr %335, 1
  %338 = insertvalue { ptr, ptr, i64 } %337, i64 0, 2
  %339 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %340 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %341 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %342 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %343 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %344 = extractvalue { ptr, ptr, i64 } %338, 0
  %345 = extractvalue { ptr, ptr, i64 } %338, 1
  %346 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %344, 0
  %347 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %346, ptr %345, 1
  %348 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %347, i64 200, 2
  %349 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %348, i64 1, 3, 0
  %350 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %349, i64 12, 4, 0
  %351 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %350, i64 4, 3, 1
  %352 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %351, i64 1, 4, 1
  %353 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 0
  %354 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %355 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %353, 0
  %356 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %355, ptr %354, 1
  %357 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %356, i64 0, 2
  %358 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %357, i64 1, 3, 0
  %359 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %358, i64 4, 4, 0
  %360 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %359, i64 4, 3, 1
  %361 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %360, i64 1, 4, 1
  %362 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %352, 3, 0
  %363 = mul i64 1, %362
  %364 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %352, 3, 1
  %365 = mul i64 %363, %364
  %366 = mul i64 %365, 4
  %367 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %352, 1
  %368 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %352, 2
  %369 = getelementptr float, ptr %367, i64 %368
  %370 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %361, 1
  %371 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %361, 2
  %372 = getelementptr float, ptr %370, i64 %371
  call void @llvm.memcpy.p0.p0.i64(ptr %372, ptr %369, i64 %366, i1 false)
  %373 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %374 = ptrtoint ptr %373 to i64
  %375 = inttoptr i64 %374 to ptr
  %376 = call i32 @fpga_tile_submit(ptr %58, i32 1, i32 1, i32 2, i32 4, i32 1, i32 4, ptr %333, ptr %375, ptr %218, i32 12, i32 1, i32 42, i32 102)
  %377 = add i64 %56, 224
  %378 = inttoptr i64 %377 to ptr
  %379 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %380 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %381 = insertvalue { ptr, ptr, i64 } poison, ptr %379, 0
  %382 = insertvalue { ptr, ptr, i64 } %381, ptr %380, 1
  %383 = insertvalue { ptr, ptr, i64 } %382, i64 0, 2
  %384 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %385 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %386 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %387 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %388 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %389 = extractvalue { ptr, ptr, i64 } %383, 0
  %390 = extractvalue { ptr, ptr, i64 } %383, 1
  %391 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %389, 0
  %392 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %391, ptr %390, 1
  %393 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %392, i64 68, 2
  %394 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %393, i64 4, 3, 0
  %395 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %394, i64 17, 4, 0
  %396 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %395, i64 16, 3, 1
  %397 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %396, i64 1, 4, 1
  %398 = call ptr @llvm.stacksave.p0()
  %399 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %397, ptr %399, align 8
  %400 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %399, 1
  %401 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %246, ptr %401, align 8
  %402 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %401, 1
  %403 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %400, ptr %403, align 8
  %404 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %402, ptr %404, align 8
  call void @memrefCopy(i64 4, ptr %403, ptr %404)
  call void @llvm.stackrestore.p0(ptr %398)
  %405 = call ptr @llvm.stacksave.p0()
  %406 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %275, ptr %406, align 8
  %407 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %406, 1
  %408 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %284, ptr %408, align 8
  %409 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %408, 1
  %410 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %407, ptr %410, align 8
  %411 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %409, ptr %411, align 8
  call void @memrefCopy(i64 4, ptr %410, ptr %411)
  call void @llvm.stackrestore.p0(ptr %405)
  %412 = call i32 @fpga_tile_submit(ptr %58, i32 2, i32 0, i32 2, i32 4, i32 16, i32 4, ptr %256, ptr %294, ptr %378, i32 12, i32 2, i32 0, i32 117)
  %413 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %414 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %415 = insertvalue { ptr, ptr, i64 } poison, ptr %413, 0
  %416 = insertvalue { ptr, ptr, i64 } %415, ptr %414, 1
  %417 = insertvalue { ptr, ptr, i64 } %416, i64 0, 2
  %418 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %419 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %420 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %421 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %422 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %423 = extractvalue { ptr, ptr, i64 } %417, 0
  %424 = extractvalue { ptr, ptr, i64 } %417, 1
  %425 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %423, 0
  %426 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %425, ptr %424, 1
  %427 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %426, i64 84, 2
  %428 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %427, i64 4, 3, 0
  %429 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %428, i64 17, 4, 0
  %430 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %429, i64 1, 3, 1
  %431 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %430, i64 1, 4, 1
  %432 = call ptr @llvm.stacksave.p0()
  %433 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %431, ptr %433, align 8
  %434 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %433, 1
  %435 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %323, ptr %435, align 8
  %436 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %435, 1
  %437 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %434, ptr %437, align 8
  %438 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %436, ptr %438, align 8
  call void @memrefCopy(i64 4, ptr %437, ptr %438)
  call void @llvm.stackrestore.p0(ptr %432)
  %439 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %352, 3, 0
  %440 = mul i64 1, %439
  %441 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %352, 3, 1
  %442 = mul i64 %440, %441
  %443 = mul i64 %442, 4
  %444 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %352, 1
  %445 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %352, 2
  %446 = getelementptr float, ptr %444, i64 %445
  %447 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %361, 1
  %448 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %361, 2
  %449 = getelementptr float, ptr %447, i64 %448
  call void @llvm.memcpy.p0.p0.i64(ptr %449, ptr %446, i64 %443, i1 false)
  %450 = call i32 @fpga_tile_submit(ptr %58, i32 2, i32 1, i32 2, i32 4, i32 1, i32 4, ptr %333, ptr %375, ptr %378, i32 12, i32 2, i32 42, i32 102)
  %451 = add i64 %56, 384
  %452 = inttoptr i64 %451 to ptr
  %453 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %454 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %455 = insertvalue { ptr, ptr, i64 } poison, ptr %453, 0
  %456 = insertvalue { ptr, ptr, i64 } %455, ptr %454, 1
  %457 = insertvalue { ptr, ptr, i64 } %456, i64 0, 2
  %458 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %459 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %460 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %461 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %462 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %463 = extractvalue { ptr, ptr, i64 } %457, 0
  %464 = extractvalue { ptr, ptr, i64 } %457, 1
  %465 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %463, 0
  %466 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %465, ptr %464, 1
  %467 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %466, i64 136, 2
  %468 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %467, i64 4, 3, 0
  %469 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %468, i64 17, 4, 0
  %470 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %469, i64 16, 3, 1
  %471 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %470, i64 1, 4, 1
  %472 = call ptr @llvm.stacksave.p0()
  %473 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %471, ptr %473, align 8
  %474 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %473, 1
  %475 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %246, ptr %475, align 8
  %476 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %475, 1
  %477 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %474, ptr %477, align 8
  %478 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %476, ptr %478, align 8
  call void @memrefCopy(i64 4, ptr %477, ptr %478)
  call void @llvm.stackrestore.p0(ptr %472)
  %479 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %480 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %481 = insertvalue { ptr, ptr, i64 } poison, ptr %479, 0
  %482 = insertvalue { ptr, ptr, i64 } %481, ptr %480, 1
  %483 = insertvalue { ptr, ptr, i64 } %482, i64 0, 2
  %484 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %485 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %486 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %487 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %488 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %489 = extractvalue { ptr, ptr, i64 } %483, 0
  %490 = extractvalue { ptr, ptr, i64 } %483, 1
  %491 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %489, 0
  %492 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %491, ptr %490, 1
  %493 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %492, i64 0, 2
  %494 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %493, i64 16, 3, 0
  %495 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %494, i64 12, 4, 0
  %496 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %495, i64 4, 3, 1
  %497 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %496, i64 1, 4, 1
  %498 = call ptr @llvm.stacksave.p0()
  %499 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %497, ptr %499, align 8
  %500 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %499, 1
  %501 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %284, ptr %501, align 8
  %502 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %501, 1
  %503 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %500, ptr %503, align 8
  %504 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %502, ptr %504, align 8
  call void @memrefCopy(i64 4, ptr %503, ptr %504)
  call void @llvm.stackrestore.p0(ptr %498)
  %505 = call i32 @fpga_tile_submit(ptr %58, i32 3, i32 0, i32 2, i32 4, i32 16, i32 4, ptr %256, ptr %294, ptr %452, i32 12, i32 3, i32 0, i32 117)
  %506 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %507 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %508 = insertvalue { ptr, ptr, i64 } poison, ptr %506, 0
  %509 = insertvalue { ptr, ptr, i64 } %508, ptr %507, 1
  %510 = insertvalue { ptr, ptr, i64 } %509, i64 0, 2
  %511 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %512 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %513 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %514 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %515 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %516 = extractvalue { ptr, ptr, i64 } %510, 0
  %517 = extractvalue { ptr, ptr, i64 } %510, 1
  %518 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %516, 0
  %519 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %518, ptr %517, 1
  %520 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %519, i64 152, 2
  %521 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %520, i64 4, 3, 0
  %522 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %521, i64 17, 4, 0
  %523 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %522, i64 1, 3, 1
  %524 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %523, i64 1, 4, 1
  %525 = call ptr @llvm.stacksave.p0()
  %526 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %524, ptr %526, align 8
  %527 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %526, 1
  %528 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %323, ptr %528, align 8
  %529 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %528, 1
  %530 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %527, ptr %530, align 8
  %531 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %529, ptr %531, align 8
  call void @memrefCopy(i64 4, ptr %530, ptr %531)
  call void @llvm.stackrestore.p0(ptr %525)
  %532 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %533 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %534 = insertvalue { ptr, ptr, i64 } poison, ptr %532, 0
  %535 = insertvalue { ptr, ptr, i64 } %534, ptr %533, 1
  %536 = insertvalue { ptr, ptr, i64 } %535, i64 0, 2
  %537 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %538 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %539 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %540 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %541 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %542 = extractvalue { ptr, ptr, i64 } %536, 0
  %543 = extractvalue { ptr, ptr, i64 } %536, 1
  %544 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %542, 0
  %545 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %544, ptr %543, 1
  %546 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %545, i64 192, 2
  %547 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %546, i64 1, 3, 0
  %548 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %547, i64 12, 4, 0
  %549 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %548, i64 4, 3, 1
  %550 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %549, i64 1, 4, 1
  %551 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %550, 3, 0
  %552 = mul i64 1, %551
  %553 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %550, 3, 1
  %554 = mul i64 %552, %553
  %555 = mul i64 %554, 4
  %556 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %550, 1
  %557 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %550, 2
  %558 = getelementptr float, ptr %556, i64 %557
  %559 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %361, 1
  %560 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %361, 2
  %561 = getelementptr float, ptr %559, i64 %560
  call void @llvm.memcpy.p0.p0.i64(ptr %561, ptr %558, i64 %555, i1 false)
  %562 = call i32 @fpga_tile_submit(ptr %58, i32 3, i32 1, i32 2, i32 4, i32 1, i32 4, ptr %333, ptr %375, ptr %452, i32 12, i32 3, i32 42, i32 102)
  %563 = add i64 %56, 400
  %564 = inttoptr i64 %563 to ptr
  %565 = call ptr @llvm.stacksave.p0()
  %566 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %471, ptr %566, align 8
  %567 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %566, 1
  %568 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %246, ptr %568, align 8
  %569 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %568, 1
  %570 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %567, ptr %570, align 8
  %571 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %569, ptr %571, align 8
  call void @memrefCopy(i64 4, ptr %570, ptr %571)
  call void @llvm.stackrestore.p0(ptr %565)
  %572 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %573 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %574 = insertvalue { ptr, ptr, i64 } poison, ptr %572, 0
  %575 = insertvalue { ptr, ptr, i64 } %574, ptr %573, 1
  %576 = insertvalue { ptr, ptr, i64 } %575, i64 0, 2
  %577 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %578 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %579 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %580 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %581 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %582 = extractvalue { ptr, ptr, i64 } %576, 0
  %583 = extractvalue { ptr, ptr, i64 } %576, 1
  %584 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %582, 0
  %585 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %584, ptr %583, 1
  %586 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %585, i64 4, 2
  %587 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %586, i64 16, 3, 0
  %588 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %587, i64 12, 4, 0
  %589 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %588, i64 4, 3, 1
  %590 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %589, i64 1, 4, 1
  %591 = call ptr @llvm.stacksave.p0()
  %592 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %590, ptr %592, align 8
  %593 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %592, 1
  %594 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %284, ptr %594, align 8
  %595 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %594, 1
  %596 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %593, ptr %596, align 8
  %597 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %595, ptr %597, align 8
  call void @memrefCopy(i64 4, ptr %596, ptr %597)
  call void @llvm.stackrestore.p0(ptr %591)
  %598 = call i32 @fpga_tile_submit(ptr %58, i32 4, i32 0, i32 2, i32 4, i32 16, i32 4, ptr %256, ptr %294, ptr %564, i32 12, i32 1, i32 120, i32 117)
  %599 = call ptr @llvm.stacksave.p0()
  %600 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %524, ptr %600, align 8
  %601 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %600, 1
  %602 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %323, ptr %602, align 8
  %603 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %602, 1
  %604 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %601, ptr %604, align 8
  %605 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %603, ptr %605, align 8
  call void @memrefCopy(i64 4, ptr %604, ptr %605)
  call void @llvm.stackrestore.p0(ptr %599)
  %606 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %607 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %608 = insertvalue { ptr, ptr, i64 } poison, ptr %606, 0
  %609 = insertvalue { ptr, ptr, i64 } %608, ptr %607, 1
  %610 = insertvalue { ptr, ptr, i64 } %609, i64 0, 2
  %611 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %612 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %613 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %614 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %615 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %616 = extractvalue { ptr, ptr, i64 } %610, 0
  %617 = extractvalue { ptr, ptr, i64 } %610, 1
  %618 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %616, 0
  %619 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %618, ptr %617, 1
  %620 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %619, i64 196, 2
  %621 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %620, i64 1, 3, 0
  %622 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %621, i64 12, 4, 0
  %623 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %622, i64 4, 3, 1
  %624 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %623, i64 1, 4, 1
  %625 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %624, 3, 0
  %626 = mul i64 1, %625
  %627 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %624, 3, 1
  %628 = mul i64 %626, %627
  %629 = mul i64 %628, 4
  %630 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %624, 1
  %631 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %624, 2
  %632 = getelementptr float, ptr %630, i64 %631
  %633 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %361, 1
  %634 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %361, 2
  %635 = getelementptr float, ptr %633, i64 %634
  call void @llvm.memcpy.p0.p0.i64(ptr %635, ptr %632, i64 %629, i1 false)
  %636 = call i32 @fpga_tile_submit(ptr %58, i32 4, i32 1, i32 2, i32 4, i32 1, i32 4, ptr %333, ptr %375, ptr %564, i32 12, i32 1, i32 189, i32 102)
  %637 = add i64 %56, 416
  %638 = inttoptr i64 %637 to ptr
  %639 = call ptr @llvm.stacksave.p0()
  %640 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %471, ptr %640, align 8
  %641 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %640, 1
  %642 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %246, ptr %642, align 8
  %643 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %642, 1
  %644 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %641, ptr %644, align 8
  %645 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %643, ptr %645, align 8
  call void @memrefCopy(i64 4, ptr %644, ptr %645)
  call void @llvm.stackrestore.p0(ptr %639)
  %646 = call ptr @llvm.stacksave.p0()
  %647 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %275, ptr %647, align 8
  %648 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %647, 1
  %649 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %284, ptr %649, align 8
  %650 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %649, 1
  %651 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %648, ptr %651, align 8
  %652 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %650, ptr %652, align 8
  call void @memrefCopy(i64 4, ptr %651, ptr %652)
  call void @llvm.stackrestore.p0(ptr %646)
  %653 = call i32 @fpga_tile_submit(ptr %58, i32 5, i32 0, i32 2, i32 4, i32 16, i32 4, ptr %256, ptr %294, ptr %638, i32 12, i32 2, i32 120, i32 117)
  %654 = call ptr @llvm.stacksave.p0()
  %655 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %524, ptr %655, align 8
  %656 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %655, 1
  %657 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %323, ptr %657, align 8
  %658 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %657, 1
  %659 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %656, ptr %659, align 8
  %660 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %658, ptr %660, align 8
  call void @memrefCopy(i64 4, ptr %659, ptr %660)
  call void @llvm.stackrestore.p0(ptr %654)
  %661 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %352, 3, 0
  %662 = mul i64 1, %661
  %663 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %352, 3, 1
  %664 = mul i64 %662, %663
  %665 = mul i64 %664, 4
  %666 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %352, 1
  %667 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %352, 2
  %668 = getelementptr float, ptr %666, i64 %667
  %669 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %361, 1
  %670 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %361, 2
  %671 = getelementptr float, ptr %669, i64 %670
  call void @llvm.memcpy.p0.p0.i64(ptr %671, ptr %668, i64 %665, i1 false)
  %672 = call i32 @fpga_tile_submit(ptr %58, i32 5, i32 1, i32 2, i32 4, i32 1, i32 4, ptr %333, ptr %375, ptr %638, i32 12, i32 2, i32 189, i32 102)
  %673 = call i32 @fpga_gemm_wait(ptr %58)
  %674 = call ptr @malloc(i64 576)
  %675 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %674, 0
  %676 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %675, ptr %674, 1
  %677 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %676, i64 0, 2
  %678 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %677, i64 12, 3, 0
  %679 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %678, i64 12, 3, 1
  %680 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %679, i64 12, 4, 0
  %681 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %680, i64 1, 4, 1
  %682 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %683 = mul i64 1, %682
  %684 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %685 = mul i64 %683, %684
  %686 = mul i64 %685, 4
  %687 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %688 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %689 = getelementptr float, ptr %687, i64 %688
  %690 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %681, 1
  %691 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %681, 2
  %692 = getelementptr float, ptr %690, i64 %691
  call void @llvm.memcpy.p0.p0.i64(ptr %692, ptr %689, i64 %686, i1 false)
  ret { ptr, ptr, i64, [2 x i64], [2 x i64] } %681
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
