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
  %58 = call ptr @fpga_gemm_begin(i32 15, i32 15, i32 16, i32 7)
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
  %75 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %74, i64 16, 4, 0
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
  %101 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %102 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %103 = insertvalue { ptr, ptr, i64 } poison, ptr %101, 0
  %104 = insertvalue { ptr, ptr, i64 } %103, ptr %102, 1
  %105 = insertvalue { ptr, ptr, i64 } %104, i64 0, 2
  %106 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %107 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %108 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %109 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %110 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %111 = extractvalue { ptr, ptr, i64 } %105, 0
  %112 = extractvalue { ptr, ptr, i64 } %105, 1
  %113 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %111, 0
  %114 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %113, ptr %112, 1
  %115 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %114, i64 0, 2
  %116 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %115, i64 16, 3, 0
  %117 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %116, i64 15, 4, 0
  %118 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %117, i64 8, 3, 1
  %119 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %118, i64 1, 4, 1
  %120 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 0
  %121 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %122 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %120, 0
  %123 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %122, ptr %121, 1
  %124 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %123, i64 0, 2
  %125 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %124, i64 16, 3, 0
  %126 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %125, i64 8, 4, 0
  %127 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, i64 8, 3, 1
  %128 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %127, i64 1, 4, 1
  %129 = call ptr @llvm.stacksave.p0()
  %130 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %119, ptr %130, align 8
  %131 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %130, 1
  %132 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %128, ptr %132, align 8
  %133 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %132, 1
  %134 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %131, ptr %134, align 8
  %135 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %133, ptr %135, align 8
  call void @memrefCopy(i64 4, ptr %134, ptr %135)
  call void @llvm.stackrestore.p0(ptr %129)
  %136 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %137 = ptrtoint ptr %136 to i64
  %138 = inttoptr i64 %137 to ptr
  %139 = call i32 @fpga_tile_submit(ptr %58, i32 0, i32 0, i32 1, i32 8, i32 16, i32 8, ptr %100, ptr %138, ptr %57, i32 15, i32 0, i32 0, i32 125)
  %140 = add i64 %56, 32
  %141 = inttoptr i64 %140 to ptr
  %142 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %77, 3, 0
  %143 = mul i64 1, %142
  %144 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %77, 3, 1
  %145 = mul i64 %143, %144
  %146 = mul i64 %145, 4
  %147 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %77, 1
  %148 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %77, 2
  %149 = getelementptr float, ptr %147, i64 %148
  %150 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %86, 1
  %151 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %86, 2
  %152 = getelementptr float, ptr %150, i64 %151
  call void @llvm.memcpy.p0.p0.i64(ptr %152, ptr %149, i64 %146, i1 false)
  %153 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %154 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %155 = insertvalue { ptr, ptr, i64 } poison, ptr %153, 0
  %156 = insertvalue { ptr, ptr, i64 } %155, ptr %154, 1
  %157 = insertvalue { ptr, ptr, i64 } %156, i64 0, 2
  %158 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %159 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %160 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %161 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %162 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %163 = extractvalue { ptr, ptr, i64 } %157, 0
  %164 = extractvalue { ptr, ptr, i64 } %157, 1
  %165 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %163, 0
  %166 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %165, ptr %164, 1
  %167 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %166, i64 8, 2
  %168 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %167, i64 16, 3, 0
  %169 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %168, i64 15, 4, 0
  %170 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %169, i64 7, 3, 1
  %171 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %170, i64 1, 4, 1
  %172 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 0
  %173 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %174 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %172, 0
  %175 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %174, ptr %173, 1
  %176 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %175, i64 0, 2
  %177 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %176, i64 16, 3, 0
  %178 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %177, i64 7, 4, 0
  %179 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %178, i64 7, 3, 1
  %180 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %179, i64 1, 4, 1
  %181 = call ptr @llvm.stacksave.p0()
  %182 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %171, ptr %182, align 8
  %183 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %182, 1
  %184 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %180, ptr %184, align 8
  %185 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %184, 1
  %186 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %183, ptr %186, align 8
  %187 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %185, ptr %187, align 8
  call void @memrefCopy(i64 4, ptr %186, ptr %187)
  call void @llvm.stackrestore.p0(ptr %181)
  %188 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %189 = ptrtoint ptr %188 to i64
  %190 = inttoptr i64 %189 to ptr
  %191 = call i32 @fpga_tile_submit(ptr %58, i32 1, i32 0, i32 1, i32 8, i32 16, i32 7, ptr %100, ptr %190, ptr %141, i32 15, i32 0, i32 42, i32 125)
  %192 = add i64 %56, 480
  %193 = inttoptr i64 %192 to ptr
  %194 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %195 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %196 = insertvalue { ptr, ptr, i64 } poison, ptr %194, 0
  %197 = insertvalue { ptr, ptr, i64 } %196, ptr %195, 1
  %198 = insertvalue { ptr, ptr, i64 } %197, i64 0, 2
  %199 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %200 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %201 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %202 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %203 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %204 = extractvalue { ptr, ptr, i64 } %198, 0
  %205 = extractvalue { ptr, ptr, i64 } %198, 1
  %206 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %204, 0
  %207 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %206, ptr %205, 1
  %208 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %207, i64 128, 2
  %209 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %208, i64 7, 3, 0
  %210 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %209, i64 16, 4, 0
  %211 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %210, i64 16, 3, 1
  %212 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %211, i64 1, 4, 1
  %213 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 0
  %214 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %215 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %213, 0
  %216 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %215, ptr %214, 1
  %217 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %216, i64 0, 2
  %218 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %217, i64 7, 3, 0
  %219 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %218, i64 16, 4, 0
  %220 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %219, i64 16, 3, 1
  %221 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %220, i64 1, 4, 1
  %222 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %212, 3, 0
  %223 = mul i64 1, %222
  %224 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %212, 3, 1
  %225 = mul i64 %223, %224
  %226 = mul i64 %225, 4
  %227 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %212, 1
  %228 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %212, 2
  %229 = getelementptr float, ptr %227, i64 %228
  %230 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %221, 1
  %231 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %221, 2
  %232 = getelementptr float, ptr %230, i64 %231
  call void @llvm.memcpy.p0.p0.i64(ptr %232, ptr %229, i64 %226, i1 false)
  %233 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %234 = ptrtoint ptr %233 to i64
  %235 = inttoptr i64 %234 to ptr
  %236 = call ptr @llvm.stacksave.p0()
  %237 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %119, ptr %237, align 8
  %238 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %237, 1
  %239 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %128, ptr %239, align 8
  %240 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %239, 1
  %241 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %238, ptr %241, align 8
  %242 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %240, ptr %242, align 8
  call void @memrefCopy(i64 4, ptr %241, ptr %242)
  call void @llvm.stackrestore.p0(ptr %236)
  %243 = call i32 @fpga_tile_submit(ptr %58, i32 2, i32 0, i32 1, i32 7, i32 16, i32 8, ptr %235, ptr %138, ptr %193, i32 15, i32 0, i32 128, i32 125)
  %244 = add i64 %56, 512
  %245 = inttoptr i64 %244 to ptr
  %246 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %247 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %248 = insertvalue { ptr, ptr, i64 } poison, ptr %246, 0
  %249 = insertvalue { ptr, ptr, i64 } %248, ptr %247, 1
  %250 = insertvalue { ptr, ptr, i64 } %249, i64 0, 2
  %251 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %252 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %253 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %254 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %255 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %256 = extractvalue { ptr, ptr, i64 } %250, 0
  %257 = extractvalue { ptr, ptr, i64 } %250, 1
  %258 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %256, 0
  %259 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %258, ptr %257, 1
  %260 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %259, i64 128, 2
  %261 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %260, i64 4, 3, 0
  %262 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %261, i64 16, 4, 0
  %263 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %262, i64 16, 3, 1
  %264 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %263, i64 1, 4, 1
  %265 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 0
  %266 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %267 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %265, 0
  %268 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %267, ptr %266, 1
  %269 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %268, i64 0, 2
  %270 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %269, i64 4, 3, 0
  %271 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %270, i64 16, 4, 0
  %272 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %271, i64 16, 3, 1
  %273 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %272, i64 1, 4, 1
  %274 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %264, 3, 0
  %275 = mul i64 1, %274
  %276 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %264, 3, 1
  %277 = mul i64 %275, %276
  %278 = mul i64 %277, 4
  %279 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %264, 1
  %280 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %264, 2
  %281 = getelementptr float, ptr %279, i64 %280
  %282 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %273, 1
  %283 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %273, 2
  %284 = getelementptr float, ptr %282, i64 %283
  call void @llvm.memcpy.p0.p0.i64(ptr %284, ptr %281, i64 %278, i1 false)
  %285 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %286 = ptrtoint ptr %285 to i64
  %287 = inttoptr i64 %286 to ptr
  %288 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %289 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %290 = insertvalue { ptr, ptr, i64 } poison, ptr %288, 0
  %291 = insertvalue { ptr, ptr, i64 } %290, ptr %289, 1
  %292 = insertvalue { ptr, ptr, i64 } %291, i64 0, 2
  %293 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %294 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %295 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %296 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %297 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %298 = extractvalue { ptr, ptr, i64 } %292, 0
  %299 = extractvalue { ptr, ptr, i64 } %292, 1
  %300 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %298, 0
  %301 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %300, ptr %299, 1
  %302 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %301, i64 8, 2
  %303 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %302, i64 16, 3, 0
  %304 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %303, i64 15, 4, 0
  %305 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %304, i64 4, 3, 1
  %306 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %305, i64 1, 4, 1
  %307 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 0
  %308 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %309 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %307, 0
  %310 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %309, ptr %308, 1
  %311 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %310, i64 0, 2
  %312 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %311, i64 16, 3, 0
  %313 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %312, i64 4, 4, 0
  %314 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %313, i64 4, 3, 1
  %315 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %314, i64 1, 4, 1
  %316 = call ptr @llvm.stacksave.p0()
  %317 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %306, ptr %317, align 8
  %318 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %317, 1
  %319 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %315, ptr %319, align 8
  %320 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %319, 1
  %321 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %318, ptr %321, align 8
  %322 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %320, ptr %322, align 8
  call void @memrefCopy(i64 4, ptr %321, ptr %322)
  call void @llvm.stackrestore.p0(ptr %316)
  %323 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %324 = ptrtoint ptr %323 to i64
  %325 = inttoptr i64 %324 to ptr
  %326 = call i32 @fpga_tile_submit(ptr %58, i32 3, i32 0, i32 1, i32 4, i32 16, i32 4, ptr %287, ptr %325, ptr %245, i32 15, i32 1, i32 0, i32 117)
  %327 = add i64 %56, 528
  %328 = inttoptr i64 %327 to ptr
  %329 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %264, 3, 0
  %330 = mul i64 1, %329
  %331 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %264, 3, 1
  %332 = mul i64 %330, %331
  %333 = mul i64 %332, 4
  %334 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %264, 1
  %335 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %264, 2
  %336 = getelementptr float, ptr %334, i64 %335
  %337 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %273, 1
  %338 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %273, 2
  %339 = getelementptr float, ptr %337, i64 %338
  call void @llvm.memcpy.p0.p0.i64(ptr %339, ptr %336, i64 %333, i1 false)
  %340 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %341 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %342 = insertvalue { ptr, ptr, i64 } poison, ptr %340, 0
  %343 = insertvalue { ptr, ptr, i64 } %342, ptr %341, 1
  %344 = insertvalue { ptr, ptr, i64 } %343, i64 0, 2
  %345 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %346 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %347 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %348 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %349 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %350 = extractvalue { ptr, ptr, i64 } %344, 0
  %351 = extractvalue { ptr, ptr, i64 } %344, 1
  %352 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %350, 0
  %353 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %352, ptr %351, 1
  %354 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %353, i64 12, 2
  %355 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %354, i64 16, 3, 0
  %356 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %355, i64 15, 4, 0
  %357 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %356, i64 3, 3, 1
  %358 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %357, i64 1, 4, 1
  %359 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 0
  %360 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %361 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %359, 0
  %362 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %361, ptr %360, 1
  %363 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %362, i64 0, 2
  %364 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %363, i64 16, 3, 0
  %365 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %364, i64 3, 4, 0
  %366 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %365, i64 3, 3, 1
  %367 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %366, i64 1, 4, 1
  %368 = call ptr @llvm.stacksave.p0()
  %369 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %358, ptr %369, align 8
  %370 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %369, 1
  %371 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %367, ptr %371, align 8
  %372 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %371, 1
  %373 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %370, ptr %373, align 8
  %374 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %372, ptr %374, align 8
  call void @memrefCopy(i64 4, ptr %373, ptr %374)
  call void @llvm.stackrestore.p0(ptr %368)
  %375 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %376 = ptrtoint ptr %375 to i64
  %377 = inttoptr i64 %376 to ptr
  %378 = call i32 @fpga_tile_submit(ptr %58, i32 4, i32 0, i32 1, i32 4, i32 16, i32 3, ptr %287, ptr %377, ptr %328, i32 15, i32 2, i32 0, i32 117)
  %379 = add i64 %56, 752
  %380 = inttoptr i64 %379 to ptr
  %381 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %382 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %383 = insertvalue { ptr, ptr, i64 } poison, ptr %381, 0
  %384 = insertvalue { ptr, ptr, i64 } %383, ptr %382, 1
  %385 = insertvalue { ptr, ptr, i64 } %384, i64 0, 2
  %386 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %387 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %388 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %389 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %390 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %391 = extractvalue { ptr, ptr, i64 } %385, 0
  %392 = extractvalue { ptr, ptr, i64 } %385, 1
  %393 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %391, 0
  %394 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %393, ptr %392, 1
  %395 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %394, i64 192, 2
  %396 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %395, i64 3, 3, 0
  %397 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %396, i64 16, 4, 0
  %398 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %397, i64 16, 3, 1
  %399 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %398, i64 1, 4, 1
  %400 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 0
  %401 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %402 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %400, 0
  %403 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %402, ptr %401, 1
  %404 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %403, i64 0, 2
  %405 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %404, i64 3, 3, 0
  %406 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %405, i64 16, 4, 0
  %407 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %406, i64 16, 3, 1
  %408 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %407, i64 1, 4, 1
  %409 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %399, 3, 0
  %410 = mul i64 1, %409
  %411 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %399, 3, 1
  %412 = mul i64 %410, %411
  %413 = mul i64 %412, 4
  %414 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %399, 1
  %415 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %399, 2
  %416 = getelementptr float, ptr %414, i64 %415
  %417 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %408, 1
  %418 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %408, 2
  %419 = getelementptr float, ptr %417, i64 %418
  call void @llvm.memcpy.p0.p0.i64(ptr %419, ptr %416, i64 %413, i1 false)
  %420 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %421 = ptrtoint ptr %420 to i64
  %422 = inttoptr i64 %421 to ptr
  %423 = call ptr @llvm.stacksave.p0()
  %424 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %306, ptr %424, align 8
  %425 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %424, 1
  %426 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %315, ptr %426, align 8
  %427 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %426, 1
  %428 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %425, ptr %428, align 8
  %429 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %427, ptr %429, align 8
  call void @memrefCopy(i64 4, ptr %428, ptr %429)
  call void @llvm.stackrestore.p0(ptr %423)
  %430 = call i32 @fpga_tile_submit(ptr %58, i32 5, i32 0, i32 1, i32 3, i32 16, i32 4, ptr %422, ptr %325, ptr %380, i32 15, i32 3, i32 0, i32 117)
  %431 = add i64 %56, 768
  %432 = inttoptr i64 %431 to ptr
  %433 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %399, 3, 0
  %434 = mul i64 1, %433
  %435 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %399, 3, 1
  %436 = mul i64 %434, %435
  %437 = mul i64 %436, 4
  %438 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %399, 1
  %439 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %399, 2
  %440 = getelementptr float, ptr %438, i64 %439
  %441 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %408, 1
  %442 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %408, 2
  %443 = getelementptr float, ptr %441, i64 %442
  call void @llvm.memcpy.p0.p0.i64(ptr %443, ptr %440, i64 %437, i1 false)
  %444 = call ptr @llvm.stacksave.p0()
  %445 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %358, ptr %445, align 8
  %446 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %445, 1
  %447 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %367, ptr %447, align 8
  %448 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %447, 1
  %449 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %446, ptr %449, align 8
  %450 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %448, ptr %450, align 8
  call void @memrefCopy(i64 4, ptr %449, ptr %450)
  call void @llvm.stackrestore.p0(ptr %444)
  %451 = call i32 @fpga_tile_submit(ptr %58, i32 6, i32 0, i32 1, i32 3, i32 16, i32 3, ptr %422, ptr %377, ptr %432, i32 15, i32 1, i32 42, i32 117)
  %452 = call i32 @fpga_gemm_wait(ptr %58)
  %453 = call ptr @malloc(i64 900)
  %454 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %453, 0
  %455 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %454, ptr %453, 1
  %456 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %455, i64 0, 2
  %457 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %456, i64 15, 3, 0
  %458 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %457, i64 15, 3, 1
  %459 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %458, i64 15, 4, 0
  %460 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %459, i64 1, 4, 1
  %461 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %462 = mul i64 1, %461
  %463 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %464 = mul i64 %462, %463
  %465 = mul i64 %464, 4
  %466 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %467 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %468 = getelementptr float, ptr %466, i64 %467
  %469 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %460, 1
  %470 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %460, 2
  %471 = getelementptr float, ptr %469, i64 %470
  call void @llvm.memcpy.p0.p0.i64(ptr %471, ptr %468, i64 %465, i1 false)
  ret { ptr, ptr, i64, [2 x i64], [2 x i64] } %460
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

; Function Attrs: nocallback nofree nosync nounwind willreturn
declare ptr @llvm.stacksave.p0() #1

; Function Attrs: nocallback nofree nosync nounwind willreturn
declare void @llvm.stackrestore.p0(ptr) #1

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
attributes #1 = { nocallback nofree nosync nounwind willreturn }

!llvm.module.flags = !{!0}

!0 = !{i32 2, !"Debug Info Version", i32 3}
