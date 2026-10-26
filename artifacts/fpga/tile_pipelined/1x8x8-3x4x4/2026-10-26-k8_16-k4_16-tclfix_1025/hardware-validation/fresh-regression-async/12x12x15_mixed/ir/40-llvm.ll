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
  %58 = call ptr @fpga_gemm_begin(i32 12, i32 12, i32 15, i32 6)
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
  %75 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %74, i64 15, 4, 0
  %76 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %75, i64 15, 3, 1
  %77 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %76, i64 1, 4, 1
  %78 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 0
  %79 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %80 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %78, 0
  %81 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %80, ptr %79, 1
  %82 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %81, i64 0, 2
  %83 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %82, i64 8, 3, 0
  %84 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %83, i64 15, 4, 0
  %85 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %84, i64 15, 3, 1
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
  %116 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %115, i64 15, 3, 0
  %117 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %116, i64 12, 4, 0
  %118 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %117, i64 8, 3, 1
  %119 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %118, i64 1, 4, 1
  %120 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 0
  %121 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %122 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %120, 0
  %123 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %122, ptr %121, 1
  %124 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %123, i64 0, 2
  %125 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %124, i64 15, 3, 0
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
  %139 = call i32 @fpga_tile_submit(ptr %58, i32 0, i32 0, i32 1, i32 8, i32 15, i32 8, ptr %100, ptr %138, ptr %57, i32 12, i32 0, i32 0, i32 124)
  %140 = add i64 %56, 32
  %141 = inttoptr i64 %140 to ptr
  %142 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %143 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %144 = insertvalue { ptr, ptr, i64 } poison, ptr %142, 0
  %145 = insertvalue { ptr, ptr, i64 } %144, ptr %143, 1
  %146 = insertvalue { ptr, ptr, i64 } %145, i64 0, 2
  %147 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %148 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %149 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %150 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %151 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %152 = extractvalue { ptr, ptr, i64 } %146, 0
  %153 = extractvalue { ptr, ptr, i64 } %146, 1
  %154 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %152, 0
  %155 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %154, ptr %153, 1
  %156 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %155, i64 0, 2
  %157 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %156, i64 4, 3, 0
  %158 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %157, i64 15, 4, 0
  %159 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %158, i64 15, 3, 1
  %160 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %159, i64 1, 4, 1
  %161 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 0
  %162 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %163 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %161, 0
  %164 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %163, ptr %162, 1
  %165 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, i64 0, 2
  %166 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %165, i64 4, 3, 0
  %167 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %166, i64 15, 4, 0
  %168 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %167, i64 15, 3, 1
  %169 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %168, i64 1, 4, 1
  %170 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %160, 3, 0
  %171 = mul i64 1, %170
  %172 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %160, 3, 1
  %173 = mul i64 %171, %172
  %174 = mul i64 %173, 4
  %175 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %160, 1
  %176 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %160, 2
  %177 = getelementptr float, ptr %175, i64 %176
  %178 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %169, 1
  %179 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %169, 2
  %180 = getelementptr float, ptr %178, i64 %179
  call void @llvm.memcpy.p0.p0.i64(ptr %180, ptr %177, i64 %174, i1 false)
  %181 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %182 = ptrtoint ptr %181 to i64
  %183 = inttoptr i64 %182 to ptr
  %184 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %185 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %186 = insertvalue { ptr, ptr, i64 } poison, ptr %184, 0
  %187 = insertvalue { ptr, ptr, i64 } %186, ptr %185, 1
  %188 = insertvalue { ptr, ptr, i64 } %187, i64 0, 2
  %189 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %190 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %191 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %192 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %193 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %194 = extractvalue { ptr, ptr, i64 } %188, 0
  %195 = extractvalue { ptr, ptr, i64 } %188, 1
  %196 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %194, 0
  %197 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %196, ptr %195, 1
  %198 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %197, i64 8, 2
  %199 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %198, i64 15, 3, 0
  %200 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %199, i64 12, 4, 0
  %201 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %200, i64 4, 3, 1
  %202 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %201, i64 1, 4, 1
  %203 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 0
  %204 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %205 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %203, 0
  %206 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %205, ptr %204, 1
  %207 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %206, i64 0, 2
  %208 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %207, i64 15, 3, 0
  %209 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %208, i64 4, 4, 0
  %210 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %209, i64 4, 3, 1
  %211 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %210, i64 1, 4, 1
  %212 = call ptr @llvm.stacksave.p0()
  %213 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %202, ptr %213, align 8
  %214 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %213, 1
  %215 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %211, ptr %215, align 8
  %216 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %215, 1
  %217 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %214, ptr %217, align 8
  %218 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %216, ptr %218, align 8
  call void @memrefCopy(i64 4, ptr %217, ptr %218)
  call void @llvm.stackrestore.p0(ptr %212)
  %219 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %220 = ptrtoint ptr %219 to i64
  %221 = inttoptr i64 %220 to ptr
  %222 = call i32 @fpga_tile_submit(ptr %58, i32 1, i32 0, i32 1, i32 4, i32 15, i32 4, ptr %183, ptr %221, ptr %141, i32 12, i32 1, i32 0, i32 116)
  %223 = add i64 %56, 224
  %224 = inttoptr i64 %223 to ptr
  %225 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %226 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %227 = insertvalue { ptr, ptr, i64 } poison, ptr %225, 0
  %228 = insertvalue { ptr, ptr, i64 } %227, ptr %226, 1
  %229 = insertvalue { ptr, ptr, i64 } %228, i64 0, 2
  %230 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %231 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %232 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %233 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %234 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %235 = extractvalue { ptr, ptr, i64 } %229, 0
  %236 = extractvalue { ptr, ptr, i64 } %229, 1
  %237 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %235, 0
  %238 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %237, ptr %236, 1
  %239 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %238, i64 60, 2
  %240 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %239, i64 4, 3, 0
  %241 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %240, i64 15, 4, 0
  %242 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %241, i64 15, 3, 1
  %243 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %242, i64 1, 4, 1
  %244 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %243, 3, 0
  %245 = mul i64 1, %244
  %246 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %243, 3, 1
  %247 = mul i64 %245, %246
  %248 = mul i64 %247, 4
  %249 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %243, 1
  %250 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %243, 2
  %251 = getelementptr float, ptr %249, i64 %250
  %252 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %169, 1
  %253 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %169, 2
  %254 = getelementptr float, ptr %252, i64 %253
  call void @llvm.memcpy.p0.p0.i64(ptr %254, ptr %251, i64 %248, i1 false)
  %255 = call ptr @llvm.stacksave.p0()
  %256 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %202, ptr %256, align 8
  %257 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %256, 1
  %258 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %211, ptr %258, align 8
  %259 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %258, 1
  %260 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %257, ptr %260, align 8
  %261 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %259, ptr %261, align 8
  call void @memrefCopy(i64 4, ptr %260, ptr %261)
  call void @llvm.stackrestore.p0(ptr %255)
  %262 = call i32 @fpga_tile_submit(ptr %58, i32 2, i32 0, i32 1, i32 4, i32 15, i32 4, ptr %183, ptr %221, ptr %224, i32 12, i32 2, i32 0, i32 116)
  %263 = add i64 %56, 384
  %264 = inttoptr i64 %263 to ptr
  %265 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %266 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %267 = insertvalue { ptr, ptr, i64 } poison, ptr %265, 0
  %268 = insertvalue { ptr, ptr, i64 } %267, ptr %266, 1
  %269 = insertvalue { ptr, ptr, i64 } %268, i64 0, 2
  %270 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %271 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %272 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %273 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %274 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %275 = extractvalue { ptr, ptr, i64 } %269, 0
  %276 = extractvalue { ptr, ptr, i64 } %269, 1
  %277 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %275, 0
  %278 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %277, ptr %276, 1
  %279 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %278, i64 120, 2
  %280 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %279, i64 4, 3, 0
  %281 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %280, i64 15, 4, 0
  %282 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %281, i64 15, 3, 1
  %283 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %282, i64 1, 4, 1
  %284 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %283, 3, 0
  %285 = mul i64 1, %284
  %286 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %283, 3, 1
  %287 = mul i64 %285, %286
  %288 = mul i64 %287, 4
  %289 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %283, 1
  %290 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %283, 2
  %291 = getelementptr float, ptr %289, i64 %290
  %292 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %169, 1
  %293 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %169, 2
  %294 = getelementptr float, ptr %292, i64 %293
  call void @llvm.memcpy.p0.p0.i64(ptr %294, ptr %291, i64 %288, i1 false)
  %295 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %296 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %297 = insertvalue { ptr, ptr, i64 } poison, ptr %295, 0
  %298 = insertvalue { ptr, ptr, i64 } %297, ptr %296, 1
  %299 = insertvalue { ptr, ptr, i64 } %298, i64 0, 2
  %300 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %301 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %302 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %303 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %304 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %305 = extractvalue { ptr, ptr, i64 } %299, 0
  %306 = extractvalue { ptr, ptr, i64 } %299, 1
  %307 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %305, 0
  %308 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %307, ptr %306, 1
  %309 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %308, i64 0, 2
  %310 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %309, i64 15, 3, 0
  %311 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %310, i64 12, 4, 0
  %312 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %311, i64 4, 3, 1
  %313 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %312, i64 1, 4, 1
  %314 = call ptr @llvm.stacksave.p0()
  %315 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %313, ptr %315, align 8
  %316 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %315, 1
  %317 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %211, ptr %317, align 8
  %318 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %317, 1
  %319 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %316, ptr %319, align 8
  %320 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %318, ptr %320, align 8
  call void @memrefCopy(i64 4, ptr %319, ptr %320)
  call void @llvm.stackrestore.p0(ptr %314)
  %321 = call i32 @fpga_tile_submit(ptr %58, i32 3, i32 0, i32 1, i32 4, i32 15, i32 4, ptr %183, ptr %221, ptr %264, i32 12, i32 3, i32 0, i32 116)
  %322 = add i64 %56, 400
  %323 = inttoptr i64 %322 to ptr
  %324 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %283, 3, 0
  %325 = mul i64 1, %324
  %326 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %283, 3, 1
  %327 = mul i64 %325, %326
  %328 = mul i64 %327, 4
  %329 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %283, 1
  %330 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %283, 2
  %331 = getelementptr float, ptr %329, i64 %330
  %332 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %169, 1
  %333 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %169, 2
  %334 = getelementptr float, ptr %332, i64 %333
  call void @llvm.memcpy.p0.p0.i64(ptr %334, ptr %331, i64 %328, i1 false)
  %335 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %336 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %337 = insertvalue { ptr, ptr, i64 } poison, ptr %335, 0
  %338 = insertvalue { ptr, ptr, i64 } %337, ptr %336, 1
  %339 = insertvalue { ptr, ptr, i64 } %338, i64 0, 2
  %340 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %341 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %342 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %343 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %344 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %345 = extractvalue { ptr, ptr, i64 } %339, 0
  %346 = extractvalue { ptr, ptr, i64 } %339, 1
  %347 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %345, 0
  %348 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %347, ptr %346, 1
  %349 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %348, i64 4, 2
  %350 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %349, i64 15, 3, 0
  %351 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %350, i64 12, 4, 0
  %352 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %351, i64 4, 3, 1
  %353 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %352, i64 1, 4, 1
  %354 = call ptr @llvm.stacksave.p0()
  %355 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %353, ptr %355, align 8
  %356 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %355, 1
  %357 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %211, ptr %357, align 8
  %358 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %357, 1
  %359 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %356, ptr %359, align 8
  %360 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %358, ptr %360, align 8
  call void @memrefCopy(i64 4, ptr %359, ptr %360)
  call void @llvm.stackrestore.p0(ptr %354)
  %361 = call i32 @fpga_tile_submit(ptr %58, i32 4, i32 0, i32 1, i32 4, i32 15, i32 4, ptr %183, ptr %221, ptr %323, i32 12, i32 1, i32 41, i32 116)
  %362 = add i64 %56, 416
  %363 = inttoptr i64 %362 to ptr
  %364 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %283, 3, 0
  %365 = mul i64 1, %364
  %366 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %283, 3, 1
  %367 = mul i64 %365, %366
  %368 = mul i64 %367, 4
  %369 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %283, 1
  %370 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %283, 2
  %371 = getelementptr float, ptr %369, i64 %370
  %372 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %169, 1
  %373 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %169, 2
  %374 = getelementptr float, ptr %372, i64 %373
  call void @llvm.memcpy.p0.p0.i64(ptr %374, ptr %371, i64 %368, i1 false)
  %375 = call ptr @llvm.stacksave.p0()
  %376 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %202, ptr %376, align 8
  %377 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %376, 1
  %378 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %211, ptr %378, align 8
  %379 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %378, 1
  %380 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %377, ptr %380, align 8
  %381 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %379, ptr %381, align 8
  call void @memrefCopy(i64 4, ptr %380, ptr %381)
  call void @llvm.stackrestore.p0(ptr %375)
  %382 = call i32 @fpga_tile_submit(ptr %58, i32 5, i32 0, i32 1, i32 4, i32 15, i32 4, ptr %183, ptr %221, ptr %363, i32 12, i32 2, i32 41, i32 116)
  %383 = call i32 @fpga_gemm_wait(ptr %58)
  %384 = call ptr @malloc(i64 576)
  %385 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %384, 0
  %386 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %385, ptr %384, 1
  %387 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %386, i64 0, 2
  %388 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %387, i64 12, 3, 0
  %389 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %388, i64 12, 3, 1
  %390 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %389, i64 12, 4, 0
  %391 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %390, i64 1, 4, 1
  %392 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %393 = mul i64 1, %392
  %394 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %395 = mul i64 %393, %394
  %396 = mul i64 %395, 4
  %397 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %398 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %399 = getelementptr float, ptr %397, i64 %398
  %400 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %391, 1
  %401 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %391, 2
  %402 = getelementptr float, ptr %400, i64 %401
  call void @llvm.memcpy.p0.p0.i64(ptr %402, ptr %399, i64 %396, i1 false)
  ret { ptr, ptr, i64, [2 x i64], [2 x i64] } %391
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
