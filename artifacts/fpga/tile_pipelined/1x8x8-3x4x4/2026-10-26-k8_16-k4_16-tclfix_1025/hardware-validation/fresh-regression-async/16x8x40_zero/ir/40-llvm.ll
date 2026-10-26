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
  %58 = call ptr @fpga_gemm_begin(i32 16, i32 8, i32 40, i32 6)
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
  %75 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %74, i64 40, 4, 0
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
  %113 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %112, i64 8, 4, 0
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
  %125 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %115, 3, 0
  %126 = mul i64 1, %125
  %127 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %115, 3, 1
  %128 = mul i64 %126, %127
  %129 = mul i64 %128, 4
  %130 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %115, 1
  %131 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %115, 2
  %132 = getelementptr float, ptr %130, i64 %131
  %133 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %124, 1
  %134 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %124, 2
  %135 = getelementptr float, ptr %133, i64 %134
  call void @llvm.memcpy.p0.p0.i64(ptr %135, ptr %132, i64 %129, i1 false)
  %136 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %137 = ptrtoint ptr %136 to i64
  %138 = inttoptr i64 %137 to ptr
  %139 = call i32 @fpga_tile_submit(ptr %58, i32 0, i32 0, i32 3, i32 8, i32 16, i32 8, ptr %96, ptr %138, ptr %57, i32 8, i32 0, i32 0, i32 125)
  %140 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %141 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %142 = insertvalue { ptr, ptr, i64 } poison, ptr %140, 0
  %143 = insertvalue { ptr, ptr, i64 } %142, ptr %141, 1
  %144 = insertvalue { ptr, ptr, i64 } %143, i64 0, 2
  %145 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %146 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %147 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %148 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %149 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %150 = extractvalue { ptr, ptr, i64 } %144, 0
  %151 = extractvalue { ptr, ptr, i64 } %144, 1
  %152 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %150, 0
  %153 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %152, ptr %151, 1
  %154 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %153, i64 16, 2
  %155 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %154, i64 8, 3, 0
  %156 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %155, i64 40, 4, 0
  %157 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %156, i64 16, 3, 1
  %158 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %157, i64 1, 4, 1
  %159 = call ptr @llvm.stacksave.p0()
  %160 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %158, ptr %160, align 8
  %161 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %160, 1
  %162 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %86, ptr %162, align 8
  %163 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %162, 1
  %164 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %161, ptr %164, align 8
  %165 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %163, ptr %165, align 8
  call void @memrefCopy(i64 4, ptr %164, ptr %165)
  call void @llvm.stackrestore.p0(ptr %159)
  %166 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %167 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %168 = insertvalue { ptr, ptr, i64 } poison, ptr %166, 0
  %169 = insertvalue { ptr, ptr, i64 } %168, ptr %167, 1
  %170 = insertvalue { ptr, ptr, i64 } %169, i64 0, 2
  %171 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %172 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %173 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %174 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %175 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %176 = extractvalue { ptr, ptr, i64 } %170, 0
  %177 = extractvalue { ptr, ptr, i64 } %170, 1
  %178 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %176, 0
  %179 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %178, ptr %177, 1
  %180 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %179, i64 128, 2
  %181 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %180, i64 16, 3, 0
  %182 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %181, i64 8, 4, 0
  %183 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %182, i64 8, 3, 1
  %184 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %183, i64 1, 4, 1
  %185 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %184, 3, 0
  %186 = mul i64 1, %185
  %187 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %184, 3, 1
  %188 = mul i64 %186, %187
  %189 = mul i64 %188, 4
  %190 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %184, 1
  %191 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %184, 2
  %192 = getelementptr float, ptr %190, i64 %191
  %193 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %124, 1
  %194 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %124, 2
  %195 = getelementptr float, ptr %193, i64 %194
  call void @llvm.memcpy.p0.p0.i64(ptr %195, ptr %192, i64 %189, i1 false)
  %196 = call i32 @fpga_tile_submit(ptr %58, i32 0, i32 1, i32 3, i32 8, i32 16, i32 8, ptr %96, ptr %138, ptr %57, i32 8, i32 0, i32 42, i32 125)
  %197 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %198 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %199 = insertvalue { ptr, ptr, i64 } poison, ptr %197, 0
  %200 = insertvalue { ptr, ptr, i64 } %199, ptr %198, 1
  %201 = insertvalue { ptr, ptr, i64 } %200, i64 0, 2
  %202 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %203 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %204 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %205 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %206 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %207 = extractvalue { ptr, ptr, i64 } %201, 0
  %208 = extractvalue { ptr, ptr, i64 } %201, 1
  %209 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %207, 0
  %210 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %209, ptr %208, 1
  %211 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %210, i64 32, 2
  %212 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %211, i64 8, 3, 0
  %213 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %212, i64 40, 4, 0
  %214 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %213, i64 8, 3, 1
  %215 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %214, i64 1, 4, 1
  %216 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 0
  %217 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %218 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %216, 0
  %219 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %218, ptr %217, 1
  %220 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %219, i64 0, 2
  %221 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %220, i64 8, 3, 0
  %222 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %221, i64 8, 4, 0
  %223 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %222, i64 8, 3, 1
  %224 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %223, i64 1, 4, 1
  %225 = call ptr @llvm.stacksave.p0()
  %226 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %215, ptr %226, align 8
  %227 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %226, 1
  %228 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %224, ptr %228, align 8
  %229 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %228, 1
  %230 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %227, ptr %230, align 8
  %231 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %229, ptr %231, align 8
  call void @memrefCopy(i64 4, ptr %230, ptr %231)
  call void @llvm.stackrestore.p0(ptr %225)
  %232 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %233 = ptrtoint ptr %232 to i64
  %234 = inttoptr i64 %233 to ptr
  %235 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %236 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %237 = insertvalue { ptr, ptr, i64 } poison, ptr %235, 0
  %238 = insertvalue { ptr, ptr, i64 } %237, ptr %236, 1
  %239 = insertvalue { ptr, ptr, i64 } %238, i64 0, 2
  %240 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %241 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %242 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %243 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %244 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %245 = extractvalue { ptr, ptr, i64 } %239, 0
  %246 = extractvalue { ptr, ptr, i64 } %239, 1
  %247 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %245, 0
  %248 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %247, ptr %246, 1
  %249 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %248, i64 256, 2
  %250 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %249, i64 8, 3, 0
  %251 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %250, i64 8, 4, 0
  %252 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %251, i64 8, 3, 1
  %253 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %252, i64 1, 4, 1
  %254 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 0
  %255 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %256 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %254, 0
  %257 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %256, ptr %255, 1
  %258 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %257, i64 0, 2
  %259 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %258, i64 8, 3, 0
  %260 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %259, i64 8, 4, 0
  %261 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %260, i64 8, 3, 1
  %262 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %261, i64 1, 4, 1
  %263 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %253, 3, 0
  %264 = mul i64 1, %263
  %265 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %253, 3, 1
  %266 = mul i64 %264, %265
  %267 = mul i64 %266, 4
  %268 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %253, 1
  %269 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %253, 2
  %270 = getelementptr float, ptr %268, i64 %269
  %271 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %262, 1
  %272 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %262, 2
  %273 = getelementptr float, ptr %271, i64 %272
  call void @llvm.memcpy.p0.p0.i64(ptr %273, ptr %270, i64 %267, i1 false)
  %274 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %275 = ptrtoint ptr %274 to i64
  %276 = inttoptr i64 %275 to ptr
  %277 = call i32 @fpga_tile_submit(ptr %58, i32 0, i32 2, i32 3, i32 8, i32 8, i32 8, ptr %234, ptr %276, ptr %57, i32 8, i32 0, i32 128, i32 117)
  %278 = add i64 %56, 256
  %279 = inttoptr i64 %278 to ptr
  %280 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %281 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %282 = insertvalue { ptr, ptr, i64 } poison, ptr %280, 0
  %283 = insertvalue { ptr, ptr, i64 } %282, ptr %281, 1
  %284 = insertvalue { ptr, ptr, i64 } %283, i64 0, 2
  %285 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %286 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %287 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %288 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %289 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %290 = extractvalue { ptr, ptr, i64 } %284, 0
  %291 = extractvalue { ptr, ptr, i64 } %284, 1
  %292 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %290, 0
  %293 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %292, ptr %291, 1
  %294 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %293, i64 320, 2
  %295 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %294, i64 8, 3, 0
  %296 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %295, i64 40, 4, 0
  %297 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %296, i64 16, 3, 1
  %298 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %297, i64 1, 4, 1
  %299 = call ptr @llvm.stacksave.p0()
  %300 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %298, ptr %300, align 8
  %301 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %300, 1
  %302 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %86, ptr %302, align 8
  %303 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %302, 1
  %304 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %301, ptr %304, align 8
  %305 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %303, ptr %305, align 8
  call void @memrefCopy(i64 4, ptr %304, ptr %305)
  call void @llvm.stackrestore.p0(ptr %299)
  %306 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %115, 3, 0
  %307 = mul i64 1, %306
  %308 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %115, 3, 1
  %309 = mul i64 %307, %308
  %310 = mul i64 %309, 4
  %311 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %115, 1
  %312 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %115, 2
  %313 = getelementptr float, ptr %311, i64 %312
  %314 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %124, 1
  %315 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %124, 2
  %316 = getelementptr float, ptr %314, i64 %315
  call void @llvm.memcpy.p0.p0.i64(ptr %316, ptr %313, i64 %310, i1 false)
  %317 = call i32 @fpga_tile_submit(ptr %58, i32 1, i32 0, i32 3, i32 8, i32 16, i32 8, ptr %96, ptr %138, ptr %279, i32 8, i32 0, i32 197, i32 125)
  %318 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %319 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %320 = insertvalue { ptr, ptr, i64 } poison, ptr %318, 0
  %321 = insertvalue { ptr, ptr, i64 } %320, ptr %319, 1
  %322 = insertvalue { ptr, ptr, i64 } %321, i64 0, 2
  %323 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %324 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %325 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %326 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %327 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %328 = extractvalue { ptr, ptr, i64 } %322, 0
  %329 = extractvalue { ptr, ptr, i64 } %322, 1
  %330 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %328, 0
  %331 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %330, ptr %329, 1
  %332 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %331, i64 336, 2
  %333 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %332, i64 8, 3, 0
  %334 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %333, i64 40, 4, 0
  %335 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %334, i64 16, 3, 1
  %336 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %335, i64 1, 4, 1
  %337 = call ptr @llvm.stacksave.p0()
  %338 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %336, ptr %338, align 8
  %339 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %338, 1
  %340 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %86, ptr %340, align 8
  %341 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %340, 1
  %342 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %339, ptr %342, align 8
  %343 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %341, ptr %343, align 8
  call void @memrefCopy(i64 4, ptr %342, ptr %343)
  call void @llvm.stackrestore.p0(ptr %337)
  %344 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %184, 3, 0
  %345 = mul i64 1, %344
  %346 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %184, 3, 1
  %347 = mul i64 %345, %346
  %348 = mul i64 %347, 4
  %349 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %184, 1
  %350 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %184, 2
  %351 = getelementptr float, ptr %349, i64 %350
  %352 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %124, 1
  %353 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %124, 2
  %354 = getelementptr float, ptr %352, i64 %353
  call void @llvm.memcpy.p0.p0.i64(ptr %354, ptr %351, i64 %348, i1 false)
  %355 = call i32 @fpga_tile_submit(ptr %58, i32 1, i32 1, i32 3, i32 8, i32 16, i32 8, ptr %96, ptr %138, ptr %279, i32 8, i32 0, i32 266, i32 125)
  %356 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %357 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %358 = insertvalue { ptr, ptr, i64 } poison, ptr %356, 0
  %359 = insertvalue { ptr, ptr, i64 } %358, ptr %357, 1
  %360 = insertvalue { ptr, ptr, i64 } %359, i64 0, 2
  %361 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %362 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %363 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %364 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %365 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %366 = extractvalue { ptr, ptr, i64 } %360, 0
  %367 = extractvalue { ptr, ptr, i64 } %360, 1
  %368 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %366, 0
  %369 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %368, ptr %367, 1
  %370 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %369, i64 352, 2
  %371 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %370, i64 8, 3, 0
  %372 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %371, i64 40, 4, 0
  %373 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %372, i64 8, 3, 1
  %374 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %373, i64 1, 4, 1
  %375 = call ptr @llvm.stacksave.p0()
  %376 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %374, ptr %376, align 8
  %377 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %376, 1
  %378 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %224, ptr %378, align 8
  %379 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %378, 1
  %380 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %377, ptr %380, align 8
  %381 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %379, ptr %381, align 8
  call void @memrefCopy(i64 4, ptr %380, ptr %381)
  call void @llvm.stackrestore.p0(ptr %375)
  %382 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %253, 3, 0
  %383 = mul i64 1, %382
  %384 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %253, 3, 1
  %385 = mul i64 %383, %384
  %386 = mul i64 %385, 4
  %387 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %253, 1
  %388 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %253, 2
  %389 = getelementptr float, ptr %387, i64 %388
  %390 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %262, 1
  %391 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %262, 2
  %392 = getelementptr float, ptr %390, i64 %391
  call void @llvm.memcpy.p0.p0.i64(ptr %392, ptr %389, i64 %386, i1 false)
  %393 = call i32 @fpga_tile_submit(ptr %58, i32 1, i32 2, i32 3, i32 8, i32 8, i32 8, ptr %234, ptr %276, ptr %279, i32 8, i32 0, i32 335, i32 117)
  %394 = call i32 @fpga_gemm_wait(ptr %58)
  %395 = call ptr @malloc(i64 512)
  %396 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %395, 0
  %397 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %396, ptr %395, 1
  %398 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %397, i64 0, 2
  %399 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %398, i64 16, 3, 0
  %400 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %399, i64 8, 3, 1
  %401 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %400, i64 8, 4, 0
  %402 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %401, i64 1, 4, 1
  %403 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %404 = mul i64 1, %403
  %405 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %406 = mul i64 %404, %405
  %407 = mul i64 %406, 4
  %408 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %409 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %410 = getelementptr float, ptr %408, i64 %409
  %411 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %402, 1
  %412 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %402, 2
  %413 = getelementptr float, ptr %411, i64 %412
  call void @llvm.memcpy.p0.p0.i64(ptr %413, ptr %410, i64 %407, i1 false)
  ret { ptr, ptr, i64, [2 x i64], [2 x i64] } %402
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
