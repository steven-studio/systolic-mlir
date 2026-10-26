; ModuleID = 'LLVMDialectModule'
source_filename = "LLVMDialectModule"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

declare ptr @malloc(i64)

declare void @memrefCopy(i64, ptr, ptr)

declare i32 @fpga_matmul_tiled_auto_scheduled(i32, i32, i32, ptr, ptr, ptr, i32, i32, i32)

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
  %55 = alloca float, i64 64, align 4
  %56 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } poison, ptr %55, 0
  %57 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %56, ptr %55, 1
  %58 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %57, i64 0, 2
  %59 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %58, i64 64, 3, 0
  %60 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %59, i64 1, 4, 0
  %61 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %62 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %63 = insertvalue { ptr, ptr, i64 } poison, ptr %61, 0
  %64 = insertvalue { ptr, ptr, i64 } %63, ptr %62, 1
  %65 = insertvalue { ptr, ptr, i64 } %64, i64 0, 2
  %66 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %67 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %68 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %69 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %70 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %71 = extractvalue { ptr, ptr, i64 } %65, 0
  %72 = extractvalue { ptr, ptr, i64 } %65, 1
  %73 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %71, 0
  %74 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %73, ptr %72, 1
  %75 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %74, i64 0, 2
  %76 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %75, i64 8, 3, 0
  %77 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %76, i64 33, 4, 0
  %78 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %77, i64 8, 3, 1
  %79 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %78, i64 1, 4, 1
  %80 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 0
  %81 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 1
  %82 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %80, 0
  %83 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %82, ptr %81, 1
  %84 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %83, i64 0, 2
  %85 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %84, i64 8, 3, 0
  %86 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %85, i64 8, 4, 0
  %87 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %86, i64 8, 3, 1
  %88 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %87, i64 1, 4, 1
  %89 = call ptr @llvm.stacksave.p0()
  %90 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %79, ptr %90, align 8
  %91 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %90, 1
  %92 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %92, align 8
  %93 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %92, 1
  %94 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %91, ptr %94, align 8
  %95 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %93, ptr %95, align 8
  call void @memrefCopy(i64 4, ptr %94, ptr %95)
  call void @llvm.stackrestore.p0(ptr %89)
  %96 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 1
  %97 = ptrtoint ptr %96 to i64
  %98 = inttoptr i64 %97 to ptr
  %99 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %100 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %101 = insertvalue { ptr, ptr, i64 } poison, ptr %99, 0
  %102 = insertvalue { ptr, ptr, i64 } %101, ptr %100, 1
  %103 = insertvalue { ptr, ptr, i64 } %102, i64 0, 2
  %104 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %105 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %106 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %107 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %108 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %109 = extractvalue { ptr, ptr, i64 } %103, 0
  %110 = extractvalue { ptr, ptr, i64 } %103, 1
  %111 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %109, 0
  %112 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %111, ptr %110, 1
  %113 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %112, i64 0, 2
  %114 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %113, i64 8, 3, 0
  %115 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %114, i64 16, 4, 0
  %116 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %115, i64 16, 3, 1
  %117 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %116, i64 1, 4, 1
  %118 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 0
  %119 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %120 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %118, 0
  %121 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %120, ptr %119, 1
  %122 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %121, i64 0, 2
  %123 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %122, i64 8, 3, 0
  %124 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %123, i64 16, 4, 0
  %125 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %124, i64 16, 3, 1
  %126 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %125, i64 1, 4, 1
  %127 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %117, 3, 0
  %128 = mul i64 1, %127
  %129 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %117, 3, 1
  %130 = mul i64 %128, %129
  %131 = mul i64 %130, 4
  %132 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %117, 1
  %133 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %117, 2
  %134 = getelementptr float, ptr %132, i64 %133
  %135 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, 1
  %136 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, 2
  %137 = getelementptr float, ptr %135, i64 %136
  call void @llvm.memcpy.p0.p0.i64(ptr %137, ptr %134, i64 %131, i1 false)
  %138 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %139 = ptrtoint ptr %138 to i64
  %140 = inttoptr i64 %139 to ptr
  %141 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %142 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %143 = insertvalue { ptr, ptr, i64 } poison, ptr %141, 0
  %144 = insertvalue { ptr, ptr, i64 } %143, ptr %142, 1
  %145 = insertvalue { ptr, ptr, i64 } %144, i64 0, 2
  %146 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %147 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %148 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %149 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %150 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %151 = extractvalue { ptr, ptr, i64 } %145, 0
  %152 = extractvalue { ptr, ptr, i64 } %145, 1
  %153 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %151, 0
  %154 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %153, ptr %152, 1
  %155 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %154, i64 0, 2
  %156 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %155, i64 16, 3, 0
  %157 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %156, i64 33, 4, 0
  %158 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %157, i64 8, 3, 1
  %159 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %158, i64 1, 4, 1
  %160 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 0
  %161 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %162 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %160, 0
  %163 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %162, ptr %161, 1
  %164 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %163, i64 0, 2
  %165 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, i64 16, 3, 0
  %166 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %165, i64 8, 4, 0
  %167 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %166, i64 8, 3, 1
  %168 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %167, i64 1, 4, 1
  %169 = call ptr @llvm.stacksave.p0()
  %170 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %159, ptr %170, align 8
  %171 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %170, 1
  %172 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %168, ptr %172, align 8
  %173 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %172, 1
  %174 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %171, ptr %174, align 8
  %175 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %173, ptr %175, align 8
  call void @memrefCopy(i64 4, ptr %174, ptr %175)
  call void @llvm.stackrestore.p0(ptr %169)
  %176 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %177 = ptrtoint ptr %176 to i64
  %178 = inttoptr i64 %177 to ptr
  %179 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %140, ptr %178, ptr %98, i32 0, i32 0, i32 125)
  %180 = call ptr @llvm.stacksave.p0()
  %181 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %181, align 8
  %182 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %181, 1
  %183 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %79, ptr %183, align 8
  %184 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %183, 1
  %185 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %182, ptr %185, align 8
  %186 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %184, ptr %186, align 8
  call void @memrefCopy(i64 4, ptr %185, ptr %186)
  call void @llvm.stackrestore.p0(ptr %180)
  %187 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %188 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %189 = insertvalue { ptr, ptr, i64 } poison, ptr %187, 0
  %190 = insertvalue { ptr, ptr, i64 } %189, ptr %188, 1
  %191 = insertvalue { ptr, ptr, i64 } %190, i64 0, 2
  %192 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %193 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %194 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %195 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %196 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %197 = extractvalue { ptr, ptr, i64 } %191, 0
  %198 = extractvalue { ptr, ptr, i64 } %191, 1
  %199 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %197, 0
  %200 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %199, ptr %198, 1
  %201 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %200, i64 8, 2
  %202 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %201, i64 8, 3, 0
  %203 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %202, i64 33, 4, 0
  %204 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %203, i64 8, 3, 1
  %205 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %204, i64 1, 4, 1
  %206 = call ptr @llvm.stacksave.p0()
  %207 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %205, ptr %207, align 8
  %208 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %207, 1
  %209 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %209, align 8
  %210 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %209, 1
  %211 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %208, ptr %211, align 8
  %212 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %210, ptr %212, align 8
  call void @memrefCopy(i64 4, ptr %211, ptr %212)
  call void @llvm.stackrestore.p0(ptr %206)
  %213 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %117, 3, 0
  %214 = mul i64 1, %213
  %215 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %117, 3, 1
  %216 = mul i64 %214, %215
  %217 = mul i64 %216, 4
  %218 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %117, 1
  %219 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %117, 2
  %220 = getelementptr float, ptr %218, i64 %219
  %221 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, 1
  %222 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, 2
  %223 = getelementptr float, ptr %221, i64 %222
  call void @llvm.memcpy.p0.p0.i64(ptr %223, ptr %220, i64 %217, i1 false)
  %224 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %225 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %226 = insertvalue { ptr, ptr, i64 } poison, ptr %224, 0
  %227 = insertvalue { ptr, ptr, i64 } %226, ptr %225, 1
  %228 = insertvalue { ptr, ptr, i64 } %227, i64 0, 2
  %229 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %230 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %231 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %232 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %233 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %234 = extractvalue { ptr, ptr, i64 } %228, 0
  %235 = extractvalue { ptr, ptr, i64 } %228, 1
  %236 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %234, 0
  %237 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %236, ptr %235, 1
  %238 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %237, i64 8, 2
  %239 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %238, i64 16, 3, 0
  %240 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %239, i64 33, 4, 0
  %241 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %240, i64 8, 3, 1
  %242 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %241, i64 1, 4, 1
  %243 = call ptr @llvm.stacksave.p0()
  %244 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %242, ptr %244, align 8
  %245 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %244, 1
  %246 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %168, ptr %246, align 8
  %247 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %246, 1
  %248 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %245, ptr %248, align 8
  %249 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %247, ptr %249, align 8
  call void @memrefCopy(i64 4, ptr %248, ptr %249)
  call void @llvm.stackrestore.p0(ptr %243)
  %250 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %140, ptr %178, ptr %98, i32 0, i32 42, i32 125)
  %251 = call ptr @llvm.stacksave.p0()
  %252 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %252, align 8
  %253 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %252, 1
  %254 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %205, ptr %254, align 8
  %255 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %254, 1
  %256 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %253, ptr %256, align 8
  %257 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %255, ptr %257, align 8
  call void @memrefCopy(i64 4, ptr %256, ptr %257)
  call void @llvm.stackrestore.p0(ptr %251)
  %258 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %259 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %260 = insertvalue { ptr, ptr, i64 } poison, ptr %258, 0
  %261 = insertvalue { ptr, ptr, i64 } %260, ptr %259, 1
  %262 = insertvalue { ptr, ptr, i64 } %261, i64 0, 2
  %263 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %264 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %265 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %266 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %267 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %268 = extractvalue { ptr, ptr, i64 } %262, 0
  %269 = extractvalue { ptr, ptr, i64 } %262, 1
  %270 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %268, 0
  %271 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %270, ptr %269, 1
  %272 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %271, i64 16, 2
  %273 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %272, i64 8, 3, 0
  %274 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %273, i64 33, 4, 0
  %275 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %274, i64 8, 3, 1
  %276 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %275, i64 1, 4, 1
  %277 = call ptr @llvm.stacksave.p0()
  %278 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %276, ptr %278, align 8
  %279 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %278, 1
  %280 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %280, align 8
  %281 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %280, 1
  %282 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %279, ptr %282, align 8
  %283 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %281, ptr %283, align 8
  call void @memrefCopy(i64 4, ptr %282, ptr %283)
  call void @llvm.stackrestore.p0(ptr %277)
  %284 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %117, 3, 0
  %285 = mul i64 1, %284
  %286 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %117, 3, 1
  %287 = mul i64 %285, %286
  %288 = mul i64 %287, 4
  %289 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %117, 1
  %290 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %117, 2
  %291 = getelementptr float, ptr %289, i64 %290
  %292 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, 1
  %293 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, 2
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
  %309 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %308, i64 16, 2
  %310 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %309, i64 16, 3, 0
  %311 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %310, i64 33, 4, 0
  %312 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %311, i64 8, 3, 1
  %313 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %312, i64 1, 4, 1
  %314 = call ptr @llvm.stacksave.p0()
  %315 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %313, ptr %315, align 8
  %316 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %315, 1
  %317 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %168, ptr %317, align 8
  %318 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %317, 1
  %319 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %316, ptr %319, align 8
  %320 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %318, ptr %320, align 8
  call void @memrefCopy(i64 4, ptr %319, ptr %320)
  call void @llvm.stackrestore.p0(ptr %314)
  %321 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %140, ptr %178, ptr %98, i32 0, i32 128, i32 125)
  %322 = call ptr @llvm.stacksave.p0()
  %323 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %323, align 8
  %324 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %323, 1
  %325 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %276, ptr %325, align 8
  %326 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %325, 1
  %327 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %324, ptr %327, align 8
  %328 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %326, ptr %328, align 8
  call void @memrefCopy(i64 4, ptr %327, ptr %328)
  call void @llvm.stackrestore.p0(ptr %322)
  %329 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %330 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %331 = insertvalue { ptr, ptr, i64 } poison, ptr %329, 0
  %332 = insertvalue { ptr, ptr, i64 } %331, ptr %330, 1
  %333 = insertvalue { ptr, ptr, i64 } %332, i64 0, 2
  %334 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %335 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %336 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %337 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %338 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %339 = extractvalue { ptr, ptr, i64 } %333, 0
  %340 = extractvalue { ptr, ptr, i64 } %333, 1
  %341 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %339, 0
  %342 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %341, ptr %340, 1
  %343 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %342, i64 24, 2
  %344 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %343, i64 8, 3, 0
  %345 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %344, i64 33, 4, 0
  %346 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %345, i64 8, 3, 1
  %347 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %346, i64 1, 4, 1
  %348 = call ptr @llvm.stacksave.p0()
  %349 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %347, ptr %349, align 8
  %350 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %349, 1
  %351 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %351, align 8
  %352 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %351, 1
  %353 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %350, ptr %353, align 8
  %354 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %352, ptr %354, align 8
  call void @memrefCopy(i64 4, ptr %353, ptr %354)
  call void @llvm.stackrestore.p0(ptr %348)
  %355 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %117, 3, 0
  %356 = mul i64 1, %355
  %357 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %117, 3, 1
  %358 = mul i64 %356, %357
  %359 = mul i64 %358, 4
  %360 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %117, 1
  %361 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %117, 2
  %362 = getelementptr float, ptr %360, i64 %361
  %363 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, 1
  %364 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, 2
  %365 = getelementptr float, ptr %363, i64 %364
  call void @llvm.memcpy.p0.p0.i64(ptr %365, ptr %362, i64 %359, i1 false)
  %366 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %367 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %368 = insertvalue { ptr, ptr, i64 } poison, ptr %366, 0
  %369 = insertvalue { ptr, ptr, i64 } %368, ptr %367, 1
  %370 = insertvalue { ptr, ptr, i64 } %369, i64 0, 2
  %371 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %372 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %373 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %374 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %375 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %376 = extractvalue { ptr, ptr, i64 } %370, 0
  %377 = extractvalue { ptr, ptr, i64 } %370, 1
  %378 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %376, 0
  %379 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %378, ptr %377, 1
  %380 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %379, i64 24, 2
  %381 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %380, i64 16, 3, 0
  %382 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %381, i64 33, 4, 0
  %383 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %382, i64 8, 3, 1
  %384 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %383, i64 1, 4, 1
  %385 = call ptr @llvm.stacksave.p0()
  %386 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %384, ptr %386, align 8
  %387 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %386, 1
  %388 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %168, ptr %388, align 8
  %389 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %388, 1
  %390 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %387, ptr %390, align 8
  %391 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %389, ptr %391, align 8
  call void @memrefCopy(i64 4, ptr %390, ptr %391)
  call void @llvm.stackrestore.p0(ptr %385)
  %392 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %140, ptr %178, ptr %98, i32 0, i32 197, i32 125)
  %393 = call ptr @llvm.stacksave.p0()
  %394 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %394, align 8
  %395 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %394, 1
  %396 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %347, ptr %396, align 8
  %397 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %396, 1
  %398 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %395, ptr %398, align 8
  %399 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %397, ptr %399, align 8
  call void @memrefCopy(i64 4, ptr %398, ptr %399)
  call void @llvm.stackrestore.p0(ptr %393)
  %400 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %401 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %402 = insertvalue { ptr, ptr, i64 } poison, ptr %400, 0
  %403 = insertvalue { ptr, ptr, i64 } %402, ptr %401, 1
  %404 = insertvalue { ptr, ptr, i64 } %403, i64 0, 2
  %405 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %406 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %407 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %408 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %409 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %410 = extractvalue { ptr, ptr, i64 } %404, 0
  %411 = extractvalue { ptr, ptr, i64 } %404, 1
  %412 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %410, 0
  %413 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %412, ptr %411, 1
  %414 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %413, i64 32, 2
  %415 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %414, i64 4, 3, 0
  %416 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %415, i64 33, 4, 0
  %417 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %416, i64 1, 3, 1
  %418 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %417, i64 1, 4, 1
  %419 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 0
  %420 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 1
  %421 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %419, 0
  %422 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %421, ptr %420, 1
  %423 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %422, i64 0, 2
  %424 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %423, i64 4, 3, 0
  %425 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %424, i64 1, 4, 0
  %426 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %425, i64 1, 3, 1
  %427 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %426, i64 1, 4, 1
  %428 = call ptr @llvm.stacksave.p0()
  %429 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %418, ptr %429, align 8
  %430 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %429, 1
  %431 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %427, ptr %431, align 8
  %432 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %431, 1
  %433 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %430, ptr %433, align 8
  %434 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %432, ptr %434, align 8
  call void @memrefCopy(i64 4, ptr %433, ptr %434)
  call void @llvm.stackrestore.p0(ptr %428)
  %435 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 1
  %436 = ptrtoint ptr %435 to i64
  %437 = inttoptr i64 %436 to ptr
  %438 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %439 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %440 = insertvalue { ptr, ptr, i64 } poison, ptr %438, 0
  %441 = insertvalue { ptr, ptr, i64 } %440, ptr %439, 1
  %442 = insertvalue { ptr, ptr, i64 } %441, i64 0, 2
  %443 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %444 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %445 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %446 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %447 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %448 = extractvalue { ptr, ptr, i64 } %442, 0
  %449 = extractvalue { ptr, ptr, i64 } %442, 1
  %450 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %448, 0
  %451 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %450, ptr %449, 1
  %452 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %451, i64 0, 2
  %453 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %452, i64 4, 3, 0
  %454 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %453, i64 16, 4, 0
  %455 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %454, i64 16, 3, 1
  %456 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %455, i64 1, 4, 1
  %457 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 0
  %458 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %459 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %457, 0
  %460 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %459, ptr %458, 1
  %461 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %460, i64 0, 2
  %462 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %461, i64 4, 3, 0
  %463 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %462, i64 16, 4, 0
  %464 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %463, i64 16, 3, 1
  %465 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %464, i64 1, 4, 1
  %466 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %456, 3, 0
  %467 = mul i64 1, %466
  %468 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %456, 3, 1
  %469 = mul i64 %467, %468
  %470 = mul i64 %469, 4
  %471 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %456, 1
  %472 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %456, 2
  %473 = getelementptr float, ptr %471, i64 %472
  %474 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %465, 1
  %475 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %465, 2
  %476 = getelementptr float, ptr %474, i64 %475
  call void @llvm.memcpy.p0.p0.i64(ptr %476, ptr %473, i64 %470, i1 false)
  %477 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %478 = ptrtoint ptr %477 to i64
  %479 = inttoptr i64 %478 to ptr
  %480 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %481 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %482 = insertvalue { ptr, ptr, i64 } poison, ptr %480, 0
  %483 = insertvalue { ptr, ptr, i64 } %482, ptr %481, 1
  %484 = insertvalue { ptr, ptr, i64 } %483, i64 0, 2
  %485 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %486 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %487 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %488 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %489 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %490 = extractvalue { ptr, ptr, i64 } %484, 0
  %491 = extractvalue { ptr, ptr, i64 } %484, 1
  %492 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %490, 0
  %493 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %492, ptr %491, 1
  %494 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %493, i64 32, 2
  %495 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %494, i64 16, 3, 0
  %496 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %495, i64 33, 4, 0
  %497 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %496, i64 1, 3, 1
  %498 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %497, i64 1, 4, 1
  %499 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 0
  %500 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %501 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %499, 0
  %502 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %501, ptr %500, 1
  %503 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %502, i64 0, 2
  %504 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %503, i64 16, 3, 0
  %505 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %504, i64 1, 4, 0
  %506 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %505, i64 1, 3, 1
  %507 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %506, i64 1, 4, 1
  %508 = call ptr @llvm.stacksave.p0()
  %509 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %498, ptr %509, align 8
  %510 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %509, 1
  %511 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %507, ptr %511, align 8
  %512 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %511, 1
  %513 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %510, ptr %513, align 8
  %514 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %512, ptr %514, align 8
  call void @memrefCopy(i64 4, ptr %513, ptr %514)
  call void @llvm.stackrestore.p0(ptr %508)
  %515 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %516 = ptrtoint ptr %515 to i64
  %517 = inttoptr i64 %516 to ptr
  %518 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 1, ptr %479, ptr %517, ptr %437, i32 1, i32 0, i32 117)
  %519 = call ptr @llvm.stacksave.p0()
  %520 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %427, ptr %520, align 8
  %521 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %520, 1
  %522 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %418, ptr %522, align 8
  %523 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %522, 1
  %524 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %521, ptr %524, align 8
  %525 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %523, ptr %525, align 8
  call void @memrefCopy(i64 4, ptr %524, ptr %525)
  call void @llvm.stackrestore.p0(ptr %519)
  %526 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %527 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %528 = insertvalue { ptr, ptr, i64 } poison, ptr %526, 0
  %529 = insertvalue { ptr, ptr, i64 } %528, ptr %527, 1
  %530 = insertvalue { ptr, ptr, i64 } %529, i64 0, 2
  %531 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %532 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %533 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %534 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %535 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %536 = extractvalue { ptr, ptr, i64 } %530, 0
  %537 = extractvalue { ptr, ptr, i64 } %530, 1
  %538 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %536, 0
  %539 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %538, ptr %537, 1
  %540 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %539, i64 164, 2
  %541 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %540, i64 4, 3, 0
  %542 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %541, i64 33, 4, 0
  %543 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %542, i64 1, 3, 1
  %544 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %543, i64 1, 4, 1
  %545 = call ptr @llvm.stacksave.p0()
  %546 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %544, ptr %546, align 8
  %547 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %546, 1
  %548 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %427, ptr %548, align 8
  %549 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %548, 1
  %550 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %547, ptr %550, align 8
  %551 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %549, ptr %551, align 8
  call void @memrefCopy(i64 4, ptr %550, ptr %551)
  call void @llvm.stackrestore.p0(ptr %545)
  %552 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %553 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %554 = insertvalue { ptr, ptr, i64 } poison, ptr %552, 0
  %555 = insertvalue { ptr, ptr, i64 } %554, ptr %553, 1
  %556 = insertvalue { ptr, ptr, i64 } %555, i64 0, 2
  %557 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %558 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %559 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %560 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %561 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %562 = extractvalue { ptr, ptr, i64 } %556, 0
  %563 = extractvalue { ptr, ptr, i64 } %556, 1
  %564 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %562, 0
  %565 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %564, ptr %563, 1
  %566 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %565, i64 64, 2
  %567 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %566, i64 4, 3, 0
  %568 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %567, i64 16, 4, 0
  %569 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %568, i64 16, 3, 1
  %570 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %569, i64 1, 4, 1
  %571 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %570, 3, 0
  %572 = mul i64 1, %571
  %573 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %570, 3, 1
  %574 = mul i64 %572, %573
  %575 = mul i64 %574, 4
  %576 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %570, 1
  %577 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %570, 2
  %578 = getelementptr float, ptr %576, i64 %577
  %579 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %465, 1
  %580 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %465, 2
  %581 = getelementptr float, ptr %579, i64 %580
  call void @llvm.memcpy.p0.p0.i64(ptr %581, ptr %578, i64 %575, i1 false)
  %582 = call ptr @llvm.stacksave.p0()
  %583 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %498, ptr %583, align 8
  %584 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %583, 1
  %585 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %507, ptr %585, align 8
  %586 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %585, 1
  %587 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %584, ptr %587, align 8
  %588 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %586, ptr %588, align 8
  call void @memrefCopy(i64 4, ptr %587, ptr %588)
  call void @llvm.stackrestore.p0(ptr %582)
  %589 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 1, ptr %479, ptr %517, ptr %437, i32 2, i32 0, i32 117)
  %590 = call ptr @llvm.stacksave.p0()
  %591 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %427, ptr %591, align 8
  %592 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %591, 1
  %593 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %544, ptr %593, align 8
  %594 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %593, 1
  %595 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %592, ptr %595, align 8
  %596 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %594, ptr %596, align 8
  call void @memrefCopy(i64 4, ptr %595, ptr %596)
  call void @llvm.stackrestore.p0(ptr %590)
  %597 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %598 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %599 = insertvalue { ptr, ptr, i64 } poison, ptr %597, 0
  %600 = insertvalue { ptr, ptr, i64 } %599, ptr %598, 1
  %601 = insertvalue { ptr, ptr, i64 } %600, i64 0, 2
  %602 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %603 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %604 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %605 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %606 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %607 = extractvalue { ptr, ptr, i64 } %601, 0
  %608 = extractvalue { ptr, ptr, i64 } %601, 1
  %609 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %607, 0
  %610 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %609, ptr %608, 1
  %611 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %610, i64 264, 2
  %612 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %611, i64 8, 3, 0
  %613 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %612, i64 33, 4, 0
  %614 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %613, i64 8, 3, 1
  %615 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %614, i64 1, 4, 1
  %616 = call ptr @llvm.stacksave.p0()
  %617 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %615, ptr %617, align 8
  %618 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %617, 1
  %619 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %619, align 8
  %620 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %619, 1
  %621 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %618, ptr %621, align 8
  %622 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %620, ptr %622, align 8
  call void @memrefCopy(i64 4, ptr %621, ptr %622)
  call void @llvm.stackrestore.p0(ptr %616)
  %623 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %624 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %625 = insertvalue { ptr, ptr, i64 } poison, ptr %623, 0
  %626 = insertvalue { ptr, ptr, i64 } %625, ptr %624, 1
  %627 = insertvalue { ptr, ptr, i64 } %626, i64 0, 2
  %628 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %629 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %630 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %631 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %632 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %633 = extractvalue { ptr, ptr, i64 } %627, 0
  %634 = extractvalue { ptr, ptr, i64 } %627, 1
  %635 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %633, 0
  %636 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %635, ptr %634, 1
  %637 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %636, i64 128, 2
  %638 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %637, i64 8, 3, 0
  %639 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %638, i64 16, 4, 0
  %640 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %639, i64 16, 3, 1
  %641 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %640, i64 1, 4, 1
  %642 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %641, 3, 0
  %643 = mul i64 1, %642
  %644 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %641, 3, 1
  %645 = mul i64 %643, %644
  %646 = mul i64 %645, 4
  %647 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %641, 1
  %648 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %641, 2
  %649 = getelementptr float, ptr %647, i64 %648
  %650 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, 1
  %651 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, 2
  %652 = getelementptr float, ptr %650, i64 %651
  call void @llvm.memcpy.p0.p0.i64(ptr %652, ptr %649, i64 %646, i1 false)
  %653 = call ptr @llvm.stacksave.p0()
  %654 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %159, ptr %654, align 8
  %655 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %654, 1
  %656 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %168, ptr %656, align 8
  %657 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %656, 1
  %658 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %655, ptr %658, align 8
  %659 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %657, ptr %659, align 8
  call void @memrefCopy(i64 4, ptr %658, ptr %659)
  call void @llvm.stackrestore.p0(ptr %653)
  %660 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %140, ptr %178, ptr %98, i32 0, i32 266, i32 125)
  %661 = call ptr @llvm.stacksave.p0()
  %662 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %662, align 8
  %663 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %662, 1
  %664 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %615, ptr %664, align 8
  %665 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %664, 1
  %666 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %663, ptr %666, align 8
  %667 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %665, ptr %667, align 8
  call void @memrefCopy(i64 4, ptr %666, ptr %667)
  call void @llvm.stackrestore.p0(ptr %661)
  %668 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %669 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %670 = insertvalue { ptr, ptr, i64 } poison, ptr %668, 0
  %671 = insertvalue { ptr, ptr, i64 } %670, ptr %669, 1
  %672 = insertvalue { ptr, ptr, i64 } %671, i64 0, 2
  %673 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %674 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %675 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %676 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %677 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %678 = extractvalue { ptr, ptr, i64 } %672, 0
  %679 = extractvalue { ptr, ptr, i64 } %672, 1
  %680 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %678, 0
  %681 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %680, ptr %679, 1
  %682 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %681, i64 272, 2
  %683 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %682, i64 8, 3, 0
  %684 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %683, i64 33, 4, 0
  %685 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %684, i64 8, 3, 1
  %686 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %685, i64 1, 4, 1
  %687 = call ptr @llvm.stacksave.p0()
  %688 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %686, ptr %688, align 8
  %689 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %688, 1
  %690 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %690, align 8
  %691 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %690, 1
  %692 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %689, ptr %692, align 8
  %693 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %691, ptr %693, align 8
  call void @memrefCopy(i64 4, ptr %692, ptr %693)
  call void @llvm.stackrestore.p0(ptr %687)
  %694 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %641, 3, 0
  %695 = mul i64 1, %694
  %696 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %641, 3, 1
  %697 = mul i64 %695, %696
  %698 = mul i64 %697, 4
  %699 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %641, 1
  %700 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %641, 2
  %701 = getelementptr float, ptr %699, i64 %700
  %702 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, 1
  %703 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, 2
  %704 = getelementptr float, ptr %702, i64 %703
  call void @llvm.memcpy.p0.p0.i64(ptr %704, ptr %701, i64 %698, i1 false)
  %705 = call ptr @llvm.stacksave.p0()
  %706 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %242, ptr %706, align 8
  %707 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %706, 1
  %708 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %168, ptr %708, align 8
  %709 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %708, 1
  %710 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %707, ptr %710, align 8
  %711 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %709, ptr %711, align 8
  call void @memrefCopy(i64 4, ptr %710, ptr %711)
  call void @llvm.stackrestore.p0(ptr %705)
  %712 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %140, ptr %178, ptr %98, i32 0, i32 335, i32 125)
  %713 = call ptr @llvm.stacksave.p0()
  %714 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %714, align 8
  %715 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %714, 1
  %716 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %686, ptr %716, align 8
  %717 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %716, 1
  %718 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %715, ptr %718, align 8
  %719 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %717, ptr %719, align 8
  call void @memrefCopy(i64 4, ptr %718, ptr %719)
  call void @llvm.stackrestore.p0(ptr %713)
  %720 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %721 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %722 = insertvalue { ptr, ptr, i64 } poison, ptr %720, 0
  %723 = insertvalue { ptr, ptr, i64 } %722, ptr %721, 1
  %724 = insertvalue { ptr, ptr, i64 } %723, i64 0, 2
  %725 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %726 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %727 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %728 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %729 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %730 = extractvalue { ptr, ptr, i64 } %724, 0
  %731 = extractvalue { ptr, ptr, i64 } %724, 1
  %732 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %730, 0
  %733 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %732, ptr %731, 1
  %734 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %733, i64 280, 2
  %735 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %734, i64 8, 3, 0
  %736 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %735, i64 33, 4, 0
  %737 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %736, i64 8, 3, 1
  %738 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %737, i64 1, 4, 1
  %739 = call ptr @llvm.stacksave.p0()
  %740 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %738, ptr %740, align 8
  %741 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %740, 1
  %742 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %742, align 8
  %743 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %742, 1
  %744 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %741, ptr %744, align 8
  %745 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %743, ptr %745, align 8
  call void @memrefCopy(i64 4, ptr %744, ptr %745)
  call void @llvm.stackrestore.p0(ptr %739)
  %746 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %641, 3, 0
  %747 = mul i64 1, %746
  %748 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %641, 3, 1
  %749 = mul i64 %747, %748
  %750 = mul i64 %749, 4
  %751 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %641, 1
  %752 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %641, 2
  %753 = getelementptr float, ptr %751, i64 %752
  %754 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, 1
  %755 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, 2
  %756 = getelementptr float, ptr %754, i64 %755
  call void @llvm.memcpy.p0.p0.i64(ptr %756, ptr %753, i64 %750, i1 false)
  %757 = call ptr @llvm.stacksave.p0()
  %758 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %313, ptr %758, align 8
  %759 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %758, 1
  %760 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %168, ptr %760, align 8
  %761 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %760, 1
  %762 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %759, ptr %762, align 8
  %763 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %761, ptr %763, align 8
  call void @memrefCopy(i64 4, ptr %762, ptr %763)
  call void @llvm.stackrestore.p0(ptr %757)
  %764 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %140, ptr %178, ptr %98, i32 0, i32 404, i32 125)
  %765 = call ptr @llvm.stacksave.p0()
  %766 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %766, align 8
  %767 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %766, 1
  %768 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %738, ptr %768, align 8
  %769 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %768, 1
  %770 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %767, ptr %770, align 8
  %771 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %769, ptr %771, align 8
  call void @memrefCopy(i64 4, ptr %770, ptr %771)
  call void @llvm.stackrestore.p0(ptr %765)
  %772 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %773 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %774 = insertvalue { ptr, ptr, i64 } poison, ptr %772, 0
  %775 = insertvalue { ptr, ptr, i64 } %774, ptr %773, 1
  %776 = insertvalue { ptr, ptr, i64 } %775, i64 0, 2
  %777 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %778 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %779 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %780 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %781 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %782 = extractvalue { ptr, ptr, i64 } %776, 0
  %783 = extractvalue { ptr, ptr, i64 } %776, 1
  %784 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %782, 0
  %785 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %784, ptr %783, 1
  %786 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %785, i64 288, 2
  %787 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %786, i64 8, 3, 0
  %788 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %787, i64 33, 4, 0
  %789 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %788, i64 8, 3, 1
  %790 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %789, i64 1, 4, 1
  %791 = call ptr @llvm.stacksave.p0()
  %792 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %790, ptr %792, align 8
  %793 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %792, 1
  %794 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %794, align 8
  %795 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %794, 1
  %796 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %793, ptr %796, align 8
  %797 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %795, ptr %797, align 8
  call void @memrefCopy(i64 4, ptr %796, ptr %797)
  call void @llvm.stackrestore.p0(ptr %791)
  %798 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %641, 3, 0
  %799 = mul i64 1, %798
  %800 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %641, 3, 1
  %801 = mul i64 %799, %800
  %802 = mul i64 %801, 4
  %803 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %641, 1
  %804 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %641, 2
  %805 = getelementptr float, ptr %803, i64 %804
  %806 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, 1
  %807 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, 2
  %808 = getelementptr float, ptr %806, i64 %807
  call void @llvm.memcpy.p0.p0.i64(ptr %808, ptr %805, i64 %802, i1 false)
  %809 = call ptr @llvm.stacksave.p0()
  %810 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %384, ptr %810, align 8
  %811 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %810, 1
  %812 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %168, ptr %812, align 8
  %813 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %812, 1
  %814 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %811, ptr %814, align 8
  %815 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %813, ptr %815, align 8
  call void @memrefCopy(i64 4, ptr %814, ptr %815)
  call void @llvm.stackrestore.p0(ptr %809)
  %816 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %140, ptr %178, ptr %98, i32 0, i32 473, i32 125)
  %817 = call ptr @llvm.stacksave.p0()
  %818 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %818, align 8
  %819 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %818, 1
  %820 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %790, ptr %820, align 8
  %821 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %820, 1
  %822 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %819, ptr %822, align 8
  %823 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %821, ptr %823, align 8
  call void @memrefCopy(i64 4, ptr %822, ptr %823)
  call void @llvm.stackrestore.p0(ptr %817)
  %824 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %825 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %826 = insertvalue { ptr, ptr, i64 } poison, ptr %824, 0
  %827 = insertvalue { ptr, ptr, i64 } %826, ptr %825, 1
  %828 = insertvalue { ptr, ptr, i64 } %827, i64 0, 2
  %829 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %830 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %831 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %832 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %833 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %834 = extractvalue { ptr, ptr, i64 } %828, 0
  %835 = extractvalue { ptr, ptr, i64 } %828, 1
  %836 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %834, 0
  %837 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %836, ptr %835, 1
  %838 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %837, i64 296, 2
  %839 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %838, i64 4, 3, 0
  %840 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %839, i64 33, 4, 0
  %841 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %840, i64 1, 3, 1
  %842 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %841, i64 1, 4, 1
  %843 = call ptr @llvm.stacksave.p0()
  %844 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %842, ptr %844, align 8
  %845 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %844, 1
  %846 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %427, ptr %846, align 8
  %847 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %846, 1
  %848 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %845, ptr %848, align 8
  %849 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %847, ptr %849, align 8
  call void @memrefCopy(i64 4, ptr %848, ptr %849)
  call void @llvm.stackrestore.p0(ptr %843)
  %850 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %851 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %852 = insertvalue { ptr, ptr, i64 } poison, ptr %850, 0
  %853 = insertvalue { ptr, ptr, i64 } %852, ptr %851, 1
  %854 = insertvalue { ptr, ptr, i64 } %853, i64 0, 2
  %855 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %856 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %857 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %858 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %859 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %860 = extractvalue { ptr, ptr, i64 } %854, 0
  %861 = extractvalue { ptr, ptr, i64 } %854, 1
  %862 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %860, 0
  %863 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %862, ptr %861, 1
  %864 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %863, i64 128, 2
  %865 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %864, i64 4, 3, 0
  %866 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %865, i64 16, 4, 0
  %867 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %866, i64 16, 3, 1
  %868 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %867, i64 1, 4, 1
  %869 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %868, 3, 0
  %870 = mul i64 1, %869
  %871 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %868, 3, 1
  %872 = mul i64 %870, %871
  %873 = mul i64 %872, 4
  %874 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %868, 1
  %875 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %868, 2
  %876 = getelementptr float, ptr %874, i64 %875
  %877 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %465, 1
  %878 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %465, 2
  %879 = getelementptr float, ptr %877, i64 %878
  call void @llvm.memcpy.p0.p0.i64(ptr %879, ptr %876, i64 %873, i1 false)
  %880 = call ptr @llvm.stacksave.p0()
  %881 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %498, ptr %881, align 8
  %882 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %881, 1
  %883 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %507, ptr %883, align 8
  %884 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %883, 1
  %885 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %882, ptr %885, align 8
  %886 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %884, ptr %886, align 8
  call void @memrefCopy(i64 4, ptr %885, ptr %886)
  call void @llvm.stackrestore.p0(ptr %880)
  %887 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 1, ptr %479, ptr %517, ptr %437, i32 3, i32 0, i32 117)
  %888 = call ptr @llvm.stacksave.p0()
  %889 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %427, ptr %889, align 8
  %890 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %889, 1
  %891 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %842, ptr %891, align 8
  %892 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %891, 1
  %893 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %890, ptr %893, align 8
  %894 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %892, ptr %894, align 8
  call void @memrefCopy(i64 4, ptr %893, ptr %894)
  call void @llvm.stackrestore.p0(ptr %888)
  %895 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %896 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %897 = insertvalue { ptr, ptr, i64 } poison, ptr %895, 0
  %898 = insertvalue { ptr, ptr, i64 } %897, ptr %896, 1
  %899 = insertvalue { ptr, ptr, i64 } %898, i64 0, 2
  %900 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %901 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %902 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %903 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %904 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %905 = extractvalue { ptr, ptr, i64 } %899, 0
  %906 = extractvalue { ptr, ptr, i64 } %899, 1
  %907 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %905, 0
  %908 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %907, ptr %906, 1
  %909 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %908, i64 428, 2
  %910 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %909, i64 4, 3, 0
  %911 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %910, i64 33, 4, 0
  %912 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %911, i64 1, 3, 1
  %913 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %912, i64 1, 4, 1
  %914 = call ptr @llvm.stacksave.p0()
  %915 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %913, ptr %915, align 8
  %916 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %915, 1
  %917 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %427, ptr %917, align 8
  %918 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %917, 1
  %919 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %916, ptr %919, align 8
  %920 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %918, ptr %920, align 8
  call void @memrefCopy(i64 4, ptr %919, ptr %920)
  call void @llvm.stackrestore.p0(ptr %914)
  %921 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %922 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %923 = insertvalue { ptr, ptr, i64 } poison, ptr %921, 0
  %924 = insertvalue { ptr, ptr, i64 } %923, ptr %922, 1
  %925 = insertvalue { ptr, ptr, i64 } %924, i64 0, 2
  %926 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %927 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %928 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %929 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %930 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %931 = extractvalue { ptr, ptr, i64 } %925, 0
  %932 = extractvalue { ptr, ptr, i64 } %925, 1
  %933 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %931, 0
  %934 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %933, ptr %932, 1
  %935 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %934, i64 192, 2
  %936 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %935, i64 4, 3, 0
  %937 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %936, i64 16, 4, 0
  %938 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %937, i64 16, 3, 1
  %939 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %938, i64 1, 4, 1
  %940 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %939, 3, 0
  %941 = mul i64 1, %940
  %942 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %939, 3, 1
  %943 = mul i64 %941, %942
  %944 = mul i64 %943, 4
  %945 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %939, 1
  %946 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %939, 2
  %947 = getelementptr float, ptr %945, i64 %946
  %948 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %465, 1
  %949 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %465, 2
  %950 = getelementptr float, ptr %948, i64 %949
  call void @llvm.memcpy.p0.p0.i64(ptr %950, ptr %947, i64 %944, i1 false)
  %951 = call ptr @llvm.stacksave.p0()
  %952 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %498, ptr %952, align 8
  %953 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %952, 1
  %954 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %507, ptr %954, align 8
  %955 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %954, 1
  %956 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %953, ptr %956, align 8
  %957 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %955, ptr %957, align 8
  call void @memrefCopy(i64 4, ptr %956, ptr %957)
  call void @llvm.stackrestore.p0(ptr %951)
  %958 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 1, ptr %479, ptr %517, ptr %437, i32 1, i32 42, i32 117)
  %959 = call ptr @llvm.stacksave.p0()
  %960 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %427, ptr %960, align 8
  %961 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %960, 1
  %962 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %913, ptr %962, align 8
  %963 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %962, 1
  %964 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %961, ptr %964, align 8
  %965 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %963, ptr %965, align 8
  call void @memrefCopy(i64 4, ptr %964, ptr %965)
  call void @llvm.stackrestore.p0(ptr %959)
  %966 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %967 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %968 = insertvalue { ptr, ptr, i64 } poison, ptr %966, 0
  %969 = insertvalue { ptr, ptr, i64 } %968, ptr %967, 1
  %970 = insertvalue { ptr, ptr, i64 } %969, i64 0, 2
  %971 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %972 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %973 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %974 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %975 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %976 = extractvalue { ptr, ptr, i64 } %970, 0
  %977 = extractvalue { ptr, ptr, i64 } %970, 1
  %978 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %976, 0
  %979 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %978, ptr %977, 1
  %980 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %979, i64 528, 2
  %981 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %980, i64 8, 3, 0
  %982 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %981, i64 33, 4, 0
  %983 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %982, i64 8, 3, 1
  %984 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %983, i64 1, 4, 1
  %985 = call ptr @llvm.stacksave.p0()
  %986 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %984, ptr %986, align 8
  %987 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %986, 1
  %988 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %988, align 8
  %989 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %988, 1
  %990 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %987, ptr %990, align 8
  %991 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %989, ptr %991, align 8
  call void @memrefCopy(i64 4, ptr %990, ptr %991)
  call void @llvm.stackrestore.p0(ptr %985)
  %992 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %993 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %994 = insertvalue { ptr, ptr, i64 } poison, ptr %992, 0
  %995 = insertvalue { ptr, ptr, i64 } %994, ptr %993, 1
  %996 = insertvalue { ptr, ptr, i64 } %995, i64 0, 2
  %997 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %998 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %999 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %1000 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %1001 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %1002 = extractvalue { ptr, ptr, i64 } %996, 0
  %1003 = extractvalue { ptr, ptr, i64 } %996, 1
  %1004 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1002, 0
  %1005 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1004, ptr %1003, 1
  %1006 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1005, i64 256, 2
  %1007 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1006, i64 8, 3, 0
  %1008 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1007, i64 16, 4, 0
  %1009 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1008, i64 16, 3, 1
  %1010 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1009, i64 1, 4, 1
  %1011 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1010, 3, 0
  %1012 = mul i64 1, %1011
  %1013 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1010, 3, 1
  %1014 = mul i64 %1012, %1013
  %1015 = mul i64 %1014, 4
  %1016 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1010, 1
  %1017 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1010, 2
  %1018 = getelementptr float, ptr %1016, i64 %1017
  %1019 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, 1
  %1020 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, 2
  %1021 = getelementptr float, ptr %1019, i64 %1020
  call void @llvm.memcpy.p0.p0.i64(ptr %1021, ptr %1018, i64 %1015, i1 false)
  %1022 = call ptr @llvm.stacksave.p0()
  %1023 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %159, ptr %1023, align 8
  %1024 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1023, 1
  %1025 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %168, ptr %1025, align 8
  %1026 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1025, 1
  %1027 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1024, ptr %1027, align 8
  %1028 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1026, ptr %1028, align 8
  call void @memrefCopy(i64 4, ptr %1027, ptr %1028)
  call void @llvm.stackrestore.p0(ptr %1022)
  %1029 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %140, ptr %178, ptr %98, i32 0, i32 542, i32 125)
  %1030 = call ptr @llvm.stacksave.p0()
  %1031 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %1031, align 8
  %1032 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1031, 1
  %1033 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %984, ptr %1033, align 8
  %1034 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1033, 1
  %1035 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1032, ptr %1035, align 8
  %1036 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1034, ptr %1036, align 8
  call void @memrefCopy(i64 4, ptr %1035, ptr %1036)
  call void @llvm.stackrestore.p0(ptr %1030)
  %1037 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %1038 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %1039 = insertvalue { ptr, ptr, i64 } poison, ptr %1037, 0
  %1040 = insertvalue { ptr, ptr, i64 } %1039, ptr %1038, 1
  %1041 = insertvalue { ptr, ptr, i64 } %1040, i64 0, 2
  %1042 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %1043 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %1044 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %1045 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %1046 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %1047 = extractvalue { ptr, ptr, i64 } %1041, 0
  %1048 = extractvalue { ptr, ptr, i64 } %1041, 1
  %1049 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1047, 0
  %1050 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1049, ptr %1048, 1
  %1051 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1050, i64 536, 2
  %1052 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1051, i64 8, 3, 0
  %1053 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1052, i64 33, 4, 0
  %1054 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1053, i64 8, 3, 1
  %1055 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1054, i64 1, 4, 1
  %1056 = call ptr @llvm.stacksave.p0()
  %1057 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1055, ptr %1057, align 8
  %1058 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1057, 1
  %1059 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %1059, align 8
  %1060 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1059, 1
  %1061 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1058, ptr %1061, align 8
  %1062 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1060, ptr %1062, align 8
  call void @memrefCopy(i64 4, ptr %1061, ptr %1062)
  call void @llvm.stackrestore.p0(ptr %1056)
  %1063 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1010, 3, 0
  %1064 = mul i64 1, %1063
  %1065 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1010, 3, 1
  %1066 = mul i64 %1064, %1065
  %1067 = mul i64 %1066, 4
  %1068 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1010, 1
  %1069 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1010, 2
  %1070 = getelementptr float, ptr %1068, i64 %1069
  %1071 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, 1
  %1072 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, 2
  %1073 = getelementptr float, ptr %1071, i64 %1072
  call void @llvm.memcpy.p0.p0.i64(ptr %1073, ptr %1070, i64 %1067, i1 false)
  %1074 = call ptr @llvm.stacksave.p0()
  %1075 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %242, ptr %1075, align 8
  %1076 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1075, 1
  %1077 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %168, ptr %1077, align 8
  %1078 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1077, 1
  %1079 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1076, ptr %1079, align 8
  %1080 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1078, ptr %1080, align 8
  call void @memrefCopy(i64 4, ptr %1079, ptr %1080)
  call void @llvm.stackrestore.p0(ptr %1074)
  %1081 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %140, ptr %178, ptr %98, i32 0, i32 611, i32 125)
  %1082 = call ptr @llvm.stacksave.p0()
  %1083 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %1083, align 8
  %1084 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1083, 1
  %1085 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1055, ptr %1085, align 8
  %1086 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1085, 1
  %1087 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1084, ptr %1087, align 8
  %1088 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1086, ptr %1088, align 8
  call void @memrefCopy(i64 4, ptr %1087, ptr %1088)
  call void @llvm.stackrestore.p0(ptr %1082)
  %1089 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %1090 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %1091 = insertvalue { ptr, ptr, i64 } poison, ptr %1089, 0
  %1092 = insertvalue { ptr, ptr, i64 } %1091, ptr %1090, 1
  %1093 = insertvalue { ptr, ptr, i64 } %1092, i64 0, 2
  %1094 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %1095 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %1096 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %1097 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %1098 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %1099 = extractvalue { ptr, ptr, i64 } %1093, 0
  %1100 = extractvalue { ptr, ptr, i64 } %1093, 1
  %1101 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1099, 0
  %1102 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1101, ptr %1100, 1
  %1103 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1102, i64 544, 2
  %1104 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1103, i64 8, 3, 0
  %1105 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1104, i64 33, 4, 0
  %1106 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1105, i64 8, 3, 1
  %1107 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1106, i64 1, 4, 1
  %1108 = call ptr @llvm.stacksave.p0()
  %1109 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1107, ptr %1109, align 8
  %1110 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1109, 1
  %1111 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %1111, align 8
  %1112 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1111, 1
  %1113 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1110, ptr %1113, align 8
  %1114 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1112, ptr %1114, align 8
  call void @memrefCopy(i64 4, ptr %1113, ptr %1114)
  call void @llvm.stackrestore.p0(ptr %1108)
  %1115 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1010, 3, 0
  %1116 = mul i64 1, %1115
  %1117 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1010, 3, 1
  %1118 = mul i64 %1116, %1117
  %1119 = mul i64 %1118, 4
  %1120 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1010, 1
  %1121 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1010, 2
  %1122 = getelementptr float, ptr %1120, i64 %1121
  %1123 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, 1
  %1124 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, 2
  %1125 = getelementptr float, ptr %1123, i64 %1124
  call void @llvm.memcpy.p0.p0.i64(ptr %1125, ptr %1122, i64 %1119, i1 false)
  %1126 = call ptr @llvm.stacksave.p0()
  %1127 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %313, ptr %1127, align 8
  %1128 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1127, 1
  %1129 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %168, ptr %1129, align 8
  %1130 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1129, 1
  %1131 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1128, ptr %1131, align 8
  %1132 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1130, ptr %1132, align 8
  call void @memrefCopy(i64 4, ptr %1131, ptr %1132)
  call void @llvm.stackrestore.p0(ptr %1126)
  %1133 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %140, ptr %178, ptr %98, i32 0, i32 680, i32 125)
  %1134 = call ptr @llvm.stacksave.p0()
  %1135 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %1135, align 8
  %1136 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1135, 1
  %1137 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1107, ptr %1137, align 8
  %1138 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1137, 1
  %1139 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1136, ptr %1139, align 8
  %1140 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1138, ptr %1140, align 8
  call void @memrefCopy(i64 4, ptr %1139, ptr %1140)
  call void @llvm.stackrestore.p0(ptr %1134)
  %1141 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %1142 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %1143 = insertvalue { ptr, ptr, i64 } poison, ptr %1141, 0
  %1144 = insertvalue { ptr, ptr, i64 } %1143, ptr %1142, 1
  %1145 = insertvalue { ptr, ptr, i64 } %1144, i64 0, 2
  %1146 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %1147 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %1148 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %1149 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %1150 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %1151 = extractvalue { ptr, ptr, i64 } %1145, 0
  %1152 = extractvalue { ptr, ptr, i64 } %1145, 1
  %1153 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1151, 0
  %1154 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1153, ptr %1152, 1
  %1155 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1154, i64 552, 2
  %1156 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1155, i64 8, 3, 0
  %1157 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1156, i64 33, 4, 0
  %1158 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1157, i64 8, 3, 1
  %1159 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1158, i64 1, 4, 1
  %1160 = call ptr @llvm.stacksave.p0()
  %1161 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1159, ptr %1161, align 8
  %1162 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1161, 1
  %1163 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %1163, align 8
  %1164 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1163, 1
  %1165 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1162, ptr %1165, align 8
  %1166 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1164, ptr %1166, align 8
  call void @memrefCopy(i64 4, ptr %1165, ptr %1166)
  call void @llvm.stackrestore.p0(ptr %1160)
  %1167 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1010, 3, 0
  %1168 = mul i64 1, %1167
  %1169 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1010, 3, 1
  %1170 = mul i64 %1168, %1169
  %1171 = mul i64 %1170, 4
  %1172 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1010, 1
  %1173 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1010, 2
  %1174 = getelementptr float, ptr %1172, i64 %1173
  %1175 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, 1
  %1176 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, 2
  %1177 = getelementptr float, ptr %1175, i64 %1176
  call void @llvm.memcpy.p0.p0.i64(ptr %1177, ptr %1174, i64 %1171, i1 false)
  %1178 = call ptr @llvm.stacksave.p0()
  %1179 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %384, ptr %1179, align 8
  %1180 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1179, 1
  %1181 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %168, ptr %1181, align 8
  %1182 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1181, 1
  %1183 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1180, ptr %1183, align 8
  %1184 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1182, ptr %1184, align 8
  call void @memrefCopy(i64 4, ptr %1183, ptr %1184)
  call void @llvm.stackrestore.p0(ptr %1178)
  %1185 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %140, ptr %178, ptr %98, i32 0, i32 749, i32 125)
  %1186 = call ptr @llvm.stacksave.p0()
  %1187 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %1187, align 8
  %1188 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1187, 1
  %1189 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1159, ptr %1189, align 8
  %1190 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1189, 1
  %1191 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1188, ptr %1191, align 8
  %1192 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1190, ptr %1192, align 8
  call void @memrefCopy(i64 4, ptr %1191, ptr %1192)
  call void @llvm.stackrestore.p0(ptr %1186)
  %1193 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %1194 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %1195 = insertvalue { ptr, ptr, i64 } poison, ptr %1193, 0
  %1196 = insertvalue { ptr, ptr, i64 } %1195, ptr %1194, 1
  %1197 = insertvalue { ptr, ptr, i64 } %1196, i64 0, 2
  %1198 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %1199 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %1200 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %1201 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %1202 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %1203 = extractvalue { ptr, ptr, i64 } %1197, 0
  %1204 = extractvalue { ptr, ptr, i64 } %1197, 1
  %1205 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1203, 0
  %1206 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1205, ptr %1204, 1
  %1207 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1206, i64 560, 2
  %1208 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1207, i64 4, 3, 0
  %1209 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1208, i64 33, 4, 0
  %1210 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1209, i64 1, 3, 1
  %1211 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1210, i64 1, 4, 1
  %1212 = call ptr @llvm.stacksave.p0()
  %1213 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1211, ptr %1213, align 8
  %1214 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1213, 1
  %1215 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %427, ptr %1215, align 8
  %1216 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1215, 1
  %1217 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1214, ptr %1217, align 8
  %1218 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1216, ptr %1218, align 8
  call void @memrefCopy(i64 4, ptr %1217, ptr %1218)
  call void @llvm.stackrestore.p0(ptr %1212)
  %1219 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %1220 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %1221 = insertvalue { ptr, ptr, i64 } poison, ptr %1219, 0
  %1222 = insertvalue { ptr, ptr, i64 } %1221, ptr %1220, 1
  %1223 = insertvalue { ptr, ptr, i64 } %1222, i64 0, 2
  %1224 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %1225 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %1226 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %1227 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %1228 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %1229 = extractvalue { ptr, ptr, i64 } %1223, 0
  %1230 = extractvalue { ptr, ptr, i64 } %1223, 1
  %1231 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1229, 0
  %1232 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1231, ptr %1230, 1
  %1233 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1232, i64 256, 2
  %1234 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1233, i64 4, 3, 0
  %1235 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1234, i64 16, 4, 0
  %1236 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1235, i64 16, 3, 1
  %1237 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1236, i64 1, 4, 1
  %1238 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1237, 3, 0
  %1239 = mul i64 1, %1238
  %1240 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1237, 3, 1
  %1241 = mul i64 %1239, %1240
  %1242 = mul i64 %1241, 4
  %1243 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1237, 1
  %1244 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1237, 2
  %1245 = getelementptr float, ptr %1243, i64 %1244
  %1246 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %465, 1
  %1247 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %465, 2
  %1248 = getelementptr float, ptr %1246, i64 %1247
  call void @llvm.memcpy.p0.p0.i64(ptr %1248, ptr %1245, i64 %1242, i1 false)
  %1249 = call ptr @llvm.stacksave.p0()
  %1250 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %498, ptr %1250, align 8
  %1251 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1250, 1
  %1252 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %507, ptr %1252, align 8
  %1253 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1252, 1
  %1254 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1251, ptr %1254, align 8
  %1255 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1253, ptr %1255, align 8
  call void @memrefCopy(i64 4, ptr %1254, ptr %1255)
  call void @llvm.stackrestore.p0(ptr %1249)
  %1256 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 1, ptr %479, ptr %517, ptr %437, i32 2, i32 42, i32 117)
  %1257 = call ptr @llvm.stacksave.p0()
  %1258 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %427, ptr %1258, align 8
  %1259 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1258, 1
  %1260 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1211, ptr %1260, align 8
  %1261 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1260, 1
  %1262 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1259, ptr %1262, align 8
  %1263 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1261, ptr %1263, align 8
  call void @memrefCopy(i64 4, ptr %1262, ptr %1263)
  call void @llvm.stackrestore.p0(ptr %1257)
  %1264 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %1265 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %1266 = insertvalue { ptr, ptr, i64 } poison, ptr %1264, 0
  %1267 = insertvalue { ptr, ptr, i64 } %1266, ptr %1265, 1
  %1268 = insertvalue { ptr, ptr, i64 } %1267, i64 0, 2
  %1269 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %1270 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %1271 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %1272 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %1273 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %1274 = extractvalue { ptr, ptr, i64 } %1268, 0
  %1275 = extractvalue { ptr, ptr, i64 } %1268, 1
  %1276 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1274, 0
  %1277 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1276, ptr %1275, 1
  %1278 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1277, i64 692, 2
  %1279 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1278, i64 4, 3, 0
  %1280 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1279, i64 33, 4, 0
  %1281 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1280, i64 1, 3, 1
  %1282 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1281, i64 1, 4, 1
  %1283 = call ptr @llvm.stacksave.p0()
  %1284 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1282, ptr %1284, align 8
  %1285 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1284, 1
  %1286 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %427, ptr %1286, align 8
  %1287 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1286, 1
  %1288 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1285, ptr %1288, align 8
  %1289 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1287, ptr %1289, align 8
  call void @memrefCopy(i64 4, ptr %1288, ptr %1289)
  call void @llvm.stackrestore.p0(ptr %1283)
  %1290 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %1291 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %1292 = insertvalue { ptr, ptr, i64 } poison, ptr %1290, 0
  %1293 = insertvalue { ptr, ptr, i64 } %1292, ptr %1291, 1
  %1294 = insertvalue { ptr, ptr, i64 } %1293, i64 0, 2
  %1295 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %1296 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %1297 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %1298 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %1299 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %1300 = extractvalue { ptr, ptr, i64 } %1294, 0
  %1301 = extractvalue { ptr, ptr, i64 } %1294, 1
  %1302 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1300, 0
  %1303 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1302, ptr %1301, 1
  %1304 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1303, i64 320, 2
  %1305 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1304, i64 4, 3, 0
  %1306 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1305, i64 16, 4, 0
  %1307 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1306, i64 16, 3, 1
  %1308 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1307, i64 1, 4, 1
  %1309 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1308, 3, 0
  %1310 = mul i64 1, %1309
  %1311 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1308, 3, 1
  %1312 = mul i64 %1310, %1311
  %1313 = mul i64 %1312, 4
  %1314 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1308, 1
  %1315 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1308, 2
  %1316 = getelementptr float, ptr %1314, i64 %1315
  %1317 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %465, 1
  %1318 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %465, 2
  %1319 = getelementptr float, ptr %1317, i64 %1318
  call void @llvm.memcpy.p0.p0.i64(ptr %1319, ptr %1316, i64 %1313, i1 false)
  %1320 = call ptr @llvm.stacksave.p0()
  %1321 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %498, ptr %1321, align 8
  %1322 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1321, 1
  %1323 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %507, ptr %1323, align 8
  %1324 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1323, 1
  %1325 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1322, ptr %1325, align 8
  %1326 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1324, ptr %1326, align 8
  call void @memrefCopy(i64 4, ptr %1325, ptr %1326)
  call void @llvm.stackrestore.p0(ptr %1320)
  %1327 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 1, ptr %479, ptr %517, ptr %437, i32 3, i32 42, i32 117)
  %1328 = call ptr @llvm.stacksave.p0()
  %1329 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %427, ptr %1329, align 8
  %1330 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1329, 1
  %1331 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1282, ptr %1331, align 8
  %1332 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1331, 1
  %1333 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1330, ptr %1333, align 8
  %1334 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1332, ptr %1334, align 8
  call void @memrefCopy(i64 4, ptr %1333, ptr %1334)
  call void @llvm.stackrestore.p0(ptr %1328)
  %1335 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %1336 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %1337 = insertvalue { ptr, ptr, i64 } poison, ptr %1335, 0
  %1338 = insertvalue { ptr, ptr, i64 } %1337, ptr %1336, 1
  %1339 = insertvalue { ptr, ptr, i64 } %1338, i64 0, 2
  %1340 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %1341 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %1342 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %1343 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %1344 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %1345 = extractvalue { ptr, ptr, i64 } %1339, 0
  %1346 = extractvalue { ptr, ptr, i64 } %1339, 1
  %1347 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1345, 0
  %1348 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1347, ptr %1346, 1
  %1349 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1348, i64 792, 2
  %1350 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1349, i64 4, 3, 0
  %1351 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1350, i64 33, 4, 0
  %1352 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1351, i64 4, 3, 1
  %1353 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1352, i64 1, 4, 1
  %1354 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 0
  %1355 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 1
  %1356 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1354, 0
  %1357 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1356, ptr %1355, 1
  %1358 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1357, i64 0, 2
  %1359 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1358, i64 4, 3, 0
  %1360 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1359, i64 4, 4, 0
  %1361 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1360, i64 4, 3, 1
  %1362 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1361, i64 1, 4, 1
  %1363 = call ptr @llvm.stacksave.p0()
  %1364 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1353, ptr %1364, align 8
  %1365 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1364, 1
  %1366 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1362, ptr %1366, align 8
  %1367 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1366, 1
  %1368 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1365, ptr %1368, align 8
  %1369 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1367, ptr %1369, align 8
  call void @memrefCopy(i64 4, ptr %1368, ptr %1369)
  call void @llvm.stackrestore.p0(ptr %1363)
  %1370 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 1
  %1371 = ptrtoint ptr %1370 to i64
  %1372 = inttoptr i64 %1371 to ptr
  %1373 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %1374 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %1375 = insertvalue { ptr, ptr, i64 } poison, ptr %1373, 0
  %1376 = insertvalue { ptr, ptr, i64 } %1375, ptr %1374, 1
  %1377 = insertvalue { ptr, ptr, i64 } %1376, i64 0, 2
  %1378 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %1379 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %1380 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %1381 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %1382 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %1383 = extractvalue { ptr, ptr, i64 } %1377, 0
  %1384 = extractvalue { ptr, ptr, i64 } %1377, 1
  %1385 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1383, 0
  %1386 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1385, ptr %1384, 1
  %1387 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1386, i64 384, 2
  %1388 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1387, i64 4, 3, 0
  %1389 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1388, i64 16, 4, 0
  %1390 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1389, i64 16, 3, 1
  %1391 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1390, i64 1, 4, 1
  %1392 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1391, 3, 0
  %1393 = mul i64 1, %1392
  %1394 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1391, 3, 1
  %1395 = mul i64 %1393, %1394
  %1396 = mul i64 %1395, 4
  %1397 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1391, 1
  %1398 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1391, 2
  %1399 = getelementptr float, ptr %1397, i64 %1398
  %1400 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %465, 1
  %1401 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %465, 2
  %1402 = getelementptr float, ptr %1400, i64 %1401
  call void @llvm.memcpy.p0.p0.i64(ptr %1402, ptr %1399, i64 %1396, i1 false)
  %1403 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %1404 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %1405 = insertvalue { ptr, ptr, i64 } poison, ptr %1403, 0
  %1406 = insertvalue { ptr, ptr, i64 } %1405, ptr %1404, 1
  %1407 = insertvalue { ptr, ptr, i64 } %1406, i64 0, 2
  %1408 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %1409 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %1410 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %1411 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %1412 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %1413 = extractvalue { ptr, ptr, i64 } %1407, 0
  %1414 = extractvalue { ptr, ptr, i64 } %1407, 1
  %1415 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1413, 0
  %1416 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1415, ptr %1414, 1
  %1417 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1416, i64 0, 2
  %1418 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1417, i64 16, 3, 0
  %1419 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1418, i64 33, 4, 0
  %1420 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1419, i64 4, 3, 1
  %1421 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1420, i64 1, 4, 1
  %1422 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 0
  %1423 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %1424 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1422, 0
  %1425 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1424, ptr %1423, 1
  %1426 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1425, i64 0, 2
  %1427 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1426, i64 16, 3, 0
  %1428 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1427, i64 4, 4, 0
  %1429 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1428, i64 4, 3, 1
  %1430 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1429, i64 1, 4, 1
  %1431 = call ptr @llvm.stacksave.p0()
  %1432 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1421, ptr %1432, align 8
  %1433 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1432, 1
  %1434 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1430, ptr %1434, align 8
  %1435 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1434, 1
  %1436 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1433, ptr %1436, align 8
  %1437 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1435, ptr %1437, align 8
  call void @memrefCopy(i64 4, ptr %1436, ptr %1437)
  call void @llvm.stackrestore.p0(ptr %1431)
  %1438 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %1439 = ptrtoint ptr %1438 to i64
  %1440 = inttoptr i64 %1439 to ptr
  %1441 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %479, ptr %1440, ptr %1372, i32 1, i32 120, i32 117)
  %1442 = call ptr @llvm.stacksave.p0()
  %1443 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1362, ptr %1443, align 8
  %1444 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1443, 1
  %1445 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1353, ptr %1445, align 8
  %1446 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1445, 1
  %1447 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1444, ptr %1447, align 8
  %1448 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1446, ptr %1448, align 8
  call void @memrefCopy(i64 4, ptr %1447, ptr %1448)
  call void @llvm.stackrestore.p0(ptr %1442)
  %1449 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %1450 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %1451 = insertvalue { ptr, ptr, i64 } poison, ptr %1449, 0
  %1452 = insertvalue { ptr, ptr, i64 } %1451, ptr %1450, 1
  %1453 = insertvalue { ptr, ptr, i64 } %1452, i64 0, 2
  %1454 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %1455 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %1456 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %1457 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %1458 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %1459 = extractvalue { ptr, ptr, i64 } %1453, 0
  %1460 = extractvalue { ptr, ptr, i64 } %1453, 1
  %1461 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1459, 0
  %1462 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1461, ptr %1460, 1
  %1463 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1462, i64 796, 2
  %1464 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1463, i64 4, 3, 0
  %1465 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1464, i64 33, 4, 0
  %1466 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1465, i64 4, 3, 1
  %1467 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1466, i64 1, 4, 1
  %1468 = call ptr @llvm.stacksave.p0()
  %1469 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1467, ptr %1469, align 8
  %1470 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1469, 1
  %1471 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1362, ptr %1471, align 8
  %1472 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1471, 1
  %1473 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1470, ptr %1473, align 8
  %1474 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1472, ptr %1474, align 8
  call void @memrefCopy(i64 4, ptr %1473, ptr %1474)
  call void @llvm.stackrestore.p0(ptr %1468)
  %1475 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1391, 3, 0
  %1476 = mul i64 1, %1475
  %1477 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1391, 3, 1
  %1478 = mul i64 %1476, %1477
  %1479 = mul i64 %1478, 4
  %1480 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1391, 1
  %1481 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1391, 2
  %1482 = getelementptr float, ptr %1480, i64 %1481
  %1483 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %465, 1
  %1484 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %465, 2
  %1485 = getelementptr float, ptr %1483, i64 %1484
  call void @llvm.memcpy.p0.p0.i64(ptr %1485, ptr %1482, i64 %1479, i1 false)
  %1486 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %1487 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %1488 = insertvalue { ptr, ptr, i64 } poison, ptr %1486, 0
  %1489 = insertvalue { ptr, ptr, i64 } %1488, ptr %1487, 1
  %1490 = insertvalue { ptr, ptr, i64 } %1489, i64 0, 2
  %1491 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %1492 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %1493 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %1494 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %1495 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %1496 = extractvalue { ptr, ptr, i64 } %1490, 0
  %1497 = extractvalue { ptr, ptr, i64 } %1490, 1
  %1498 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1496, 0
  %1499 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1498, ptr %1497, 1
  %1500 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1499, i64 4, 2
  %1501 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1500, i64 16, 3, 0
  %1502 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1501, i64 33, 4, 0
  %1503 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1502, i64 4, 3, 1
  %1504 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1503, i64 1, 4, 1
  %1505 = call ptr @llvm.stacksave.p0()
  %1506 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1504, ptr %1506, align 8
  %1507 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1506, 1
  %1508 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1430, ptr %1508, align 8
  %1509 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1508, 1
  %1510 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1507, ptr %1510, align 8
  %1511 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1509, ptr %1511, align 8
  call void @memrefCopy(i64 4, ptr %1510, ptr %1511)
  call void @llvm.stackrestore.p0(ptr %1505)
  %1512 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %479, ptr %1440, ptr %1372, i32 2, i32 120, i32 117)
  %1513 = call ptr @llvm.stacksave.p0()
  %1514 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1362, ptr %1514, align 8
  %1515 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1514, 1
  %1516 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1467, ptr %1516, align 8
  %1517 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1516, 1
  %1518 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1515, ptr %1518, align 8
  %1519 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1517, ptr %1519, align 8
  call void @memrefCopy(i64 4, ptr %1518, ptr %1519)
  call void @llvm.stackrestore.p0(ptr %1513)
  %1520 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %1521 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %1522 = insertvalue { ptr, ptr, i64 } poison, ptr %1520, 0
  %1523 = insertvalue { ptr, ptr, i64 } %1522, ptr %1521, 1
  %1524 = insertvalue { ptr, ptr, i64 } %1523, i64 0, 2
  %1525 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %1526 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %1527 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %1528 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %1529 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %1530 = extractvalue { ptr, ptr, i64 } %1524, 0
  %1531 = extractvalue { ptr, ptr, i64 } %1524, 1
  %1532 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1530, 0
  %1533 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1532, ptr %1531, 1
  %1534 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1533, i64 800, 2
  %1535 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1534, i64 4, 3, 0
  %1536 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1535, i64 33, 4, 0
  %1537 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1536, i64 4, 3, 1
  %1538 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1537, i64 1, 4, 1
  %1539 = call ptr @llvm.stacksave.p0()
  %1540 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1538, ptr %1540, align 8
  %1541 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1540, 1
  %1542 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1362, ptr %1542, align 8
  %1543 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1542, 1
  %1544 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1541, ptr %1544, align 8
  %1545 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1543, ptr %1545, align 8
  call void @memrefCopy(i64 4, ptr %1544, ptr %1545)
  call void @llvm.stackrestore.p0(ptr %1539)
  %1546 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1391, 3, 0
  %1547 = mul i64 1, %1546
  %1548 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1391, 3, 1
  %1549 = mul i64 %1547, %1548
  %1550 = mul i64 %1549, 4
  %1551 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1391, 1
  %1552 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1391, 2
  %1553 = getelementptr float, ptr %1551, i64 %1552
  %1554 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %465, 1
  %1555 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %465, 2
  %1556 = getelementptr float, ptr %1554, i64 %1555
  call void @llvm.memcpy.p0.p0.i64(ptr %1556, ptr %1553, i64 %1550, i1 false)
  %1557 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %1558 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %1559 = insertvalue { ptr, ptr, i64 } poison, ptr %1557, 0
  %1560 = insertvalue { ptr, ptr, i64 } %1559, ptr %1558, 1
  %1561 = insertvalue { ptr, ptr, i64 } %1560, i64 0, 2
  %1562 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %1563 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %1564 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %1565 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %1566 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %1567 = extractvalue { ptr, ptr, i64 } %1561, 0
  %1568 = extractvalue { ptr, ptr, i64 } %1561, 1
  %1569 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1567, 0
  %1570 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1569, ptr %1568, 1
  %1571 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1570, i64 8, 2
  %1572 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1571, i64 16, 3, 0
  %1573 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1572, i64 33, 4, 0
  %1574 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1573, i64 4, 3, 1
  %1575 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1574, i64 1, 4, 1
  %1576 = call ptr @llvm.stacksave.p0()
  %1577 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1575, ptr %1577, align 8
  %1578 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1577, 1
  %1579 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1430, ptr %1579, align 8
  %1580 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1579, 1
  %1581 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1578, ptr %1581, align 8
  %1582 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1580, ptr %1582, align 8
  call void @memrefCopy(i64 4, ptr %1581, ptr %1582)
  call void @llvm.stackrestore.p0(ptr %1576)
  %1583 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %479, ptr %1440, ptr %1372, i32 3, i32 120, i32 117)
  %1584 = call ptr @llvm.stacksave.p0()
  %1585 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1362, ptr %1585, align 8
  %1586 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1585, 1
  %1587 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1538, ptr %1587, align 8
  %1588 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1587, 1
  %1589 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1586, ptr %1589, align 8
  %1590 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1588, ptr %1590, align 8
  call void @memrefCopy(i64 4, ptr %1589, ptr %1590)
  call void @llvm.stackrestore.p0(ptr %1584)
  %1591 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %1592 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %1593 = insertvalue { ptr, ptr, i64 } poison, ptr %1591, 0
  %1594 = insertvalue { ptr, ptr, i64 } %1593, ptr %1592, 1
  %1595 = insertvalue { ptr, ptr, i64 } %1594, i64 0, 2
  %1596 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %1597 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %1598 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %1599 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %1600 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %1601 = extractvalue { ptr, ptr, i64 } %1595, 0
  %1602 = extractvalue { ptr, ptr, i64 } %1595, 1
  %1603 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1601, 0
  %1604 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1603, ptr %1602, 1
  %1605 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1604, i64 804, 2
  %1606 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1605, i64 4, 3, 0
  %1607 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1606, i64 33, 4, 0
  %1608 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1607, i64 4, 3, 1
  %1609 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1608, i64 1, 4, 1
  %1610 = call ptr @llvm.stacksave.p0()
  %1611 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1609, ptr %1611, align 8
  %1612 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1611, 1
  %1613 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1362, ptr %1613, align 8
  %1614 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1613, 1
  %1615 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1612, ptr %1615, align 8
  %1616 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1614, ptr %1616, align 8
  call void @memrefCopy(i64 4, ptr %1615, ptr %1616)
  call void @llvm.stackrestore.p0(ptr %1610)
  %1617 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1391, 3, 0
  %1618 = mul i64 1, %1617
  %1619 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1391, 3, 1
  %1620 = mul i64 %1618, %1619
  %1621 = mul i64 %1620, 4
  %1622 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1391, 1
  %1623 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1391, 2
  %1624 = getelementptr float, ptr %1622, i64 %1623
  %1625 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %465, 1
  %1626 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %465, 2
  %1627 = getelementptr float, ptr %1625, i64 %1626
  call void @llvm.memcpy.p0.p0.i64(ptr %1627, ptr %1624, i64 %1621, i1 false)
  %1628 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %1629 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %1630 = insertvalue { ptr, ptr, i64 } poison, ptr %1628, 0
  %1631 = insertvalue { ptr, ptr, i64 } %1630, ptr %1629, 1
  %1632 = insertvalue { ptr, ptr, i64 } %1631, i64 0, 2
  %1633 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %1634 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %1635 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %1636 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %1637 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %1638 = extractvalue { ptr, ptr, i64 } %1632, 0
  %1639 = extractvalue { ptr, ptr, i64 } %1632, 1
  %1640 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1638, 0
  %1641 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1640, ptr %1639, 1
  %1642 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1641, i64 12, 2
  %1643 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1642, i64 16, 3, 0
  %1644 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1643, i64 33, 4, 0
  %1645 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1644, i64 4, 3, 1
  %1646 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1645, i64 1, 4, 1
  %1647 = call ptr @llvm.stacksave.p0()
  %1648 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1646, ptr %1648, align 8
  %1649 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1648, 1
  %1650 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1430, ptr %1650, align 8
  %1651 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1650, 1
  %1652 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1649, ptr %1652, align 8
  %1653 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1651, ptr %1653, align 8
  call void @memrefCopy(i64 4, ptr %1652, ptr %1653)
  call void @llvm.stackrestore.p0(ptr %1647)
  %1654 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %479, ptr %1440, ptr %1372, i32 1, i32 189, i32 117)
  %1655 = call ptr @llvm.stacksave.p0()
  %1656 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1362, ptr %1656, align 8
  %1657 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1656, 1
  %1658 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1609, ptr %1658, align 8
  %1659 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1658, 1
  %1660 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1657, ptr %1660, align 8
  %1661 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1659, ptr %1661, align 8
  call void @memrefCopy(i64 4, ptr %1660, ptr %1661)
  call void @llvm.stackrestore.p0(ptr %1655)
  %1662 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %1663 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %1664 = insertvalue { ptr, ptr, i64 } poison, ptr %1662, 0
  %1665 = insertvalue { ptr, ptr, i64 } %1664, ptr %1663, 1
  %1666 = insertvalue { ptr, ptr, i64 } %1665, i64 0, 2
  %1667 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %1668 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %1669 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %1670 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %1671 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %1672 = extractvalue { ptr, ptr, i64 } %1666, 0
  %1673 = extractvalue { ptr, ptr, i64 } %1666, 1
  %1674 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1672, 0
  %1675 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1674, ptr %1673, 1
  %1676 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1675, i64 808, 2
  %1677 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1676, i64 4, 3, 0
  %1678 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1677, i64 33, 4, 0
  %1679 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1678, i64 4, 3, 1
  %1680 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1679, i64 1, 4, 1
  %1681 = call ptr @llvm.stacksave.p0()
  %1682 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1680, ptr %1682, align 8
  %1683 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1682, 1
  %1684 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1362, ptr %1684, align 8
  %1685 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1684, 1
  %1686 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1683, ptr %1686, align 8
  %1687 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1685, ptr %1687, align 8
  call void @memrefCopy(i64 4, ptr %1686, ptr %1687)
  call void @llvm.stackrestore.p0(ptr %1681)
  %1688 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1391, 3, 0
  %1689 = mul i64 1, %1688
  %1690 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1391, 3, 1
  %1691 = mul i64 %1689, %1690
  %1692 = mul i64 %1691, 4
  %1693 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1391, 1
  %1694 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1391, 2
  %1695 = getelementptr float, ptr %1693, i64 %1694
  %1696 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %465, 1
  %1697 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %465, 2
  %1698 = getelementptr float, ptr %1696, i64 %1697
  call void @llvm.memcpy.p0.p0.i64(ptr %1698, ptr %1695, i64 %1692, i1 false)
  %1699 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %1700 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %1701 = insertvalue { ptr, ptr, i64 } poison, ptr %1699, 0
  %1702 = insertvalue { ptr, ptr, i64 } %1701, ptr %1700, 1
  %1703 = insertvalue { ptr, ptr, i64 } %1702, i64 0, 2
  %1704 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %1705 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %1706 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %1707 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %1708 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %1709 = extractvalue { ptr, ptr, i64 } %1703, 0
  %1710 = extractvalue { ptr, ptr, i64 } %1703, 1
  %1711 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1709, 0
  %1712 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1711, ptr %1710, 1
  %1713 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1712, i64 16, 2
  %1714 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1713, i64 16, 3, 0
  %1715 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1714, i64 33, 4, 0
  %1716 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1715, i64 4, 3, 1
  %1717 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1716, i64 1, 4, 1
  %1718 = call ptr @llvm.stacksave.p0()
  %1719 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1717, ptr %1719, align 8
  %1720 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1719, 1
  %1721 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1430, ptr %1721, align 8
  %1722 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1721, 1
  %1723 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1720, ptr %1723, align 8
  %1724 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1722, ptr %1724, align 8
  call void @memrefCopy(i64 4, ptr %1723, ptr %1724)
  call void @llvm.stackrestore.p0(ptr %1718)
  %1725 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %479, ptr %1440, ptr %1372, i32 2, i32 189, i32 117)
  %1726 = call ptr @llvm.stacksave.p0()
  %1727 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1362, ptr %1727, align 8
  %1728 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1727, 1
  %1729 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1680, ptr %1729, align 8
  %1730 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1729, 1
  %1731 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1728, ptr %1731, align 8
  %1732 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1730, ptr %1732, align 8
  call void @memrefCopy(i64 4, ptr %1731, ptr %1732)
  call void @llvm.stackrestore.p0(ptr %1726)
  %1733 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %1734 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %1735 = insertvalue { ptr, ptr, i64 } poison, ptr %1733, 0
  %1736 = insertvalue { ptr, ptr, i64 } %1735, ptr %1734, 1
  %1737 = insertvalue { ptr, ptr, i64 } %1736, i64 0, 2
  %1738 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %1739 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %1740 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %1741 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %1742 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %1743 = extractvalue { ptr, ptr, i64 } %1737, 0
  %1744 = extractvalue { ptr, ptr, i64 } %1737, 1
  %1745 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1743, 0
  %1746 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1745, ptr %1744, 1
  %1747 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1746, i64 812, 2
  %1748 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1747, i64 4, 3, 0
  %1749 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1748, i64 33, 4, 0
  %1750 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1749, i64 4, 3, 1
  %1751 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1750, i64 1, 4, 1
  %1752 = call ptr @llvm.stacksave.p0()
  %1753 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1751, ptr %1753, align 8
  %1754 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1753, 1
  %1755 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1362, ptr %1755, align 8
  %1756 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1755, 1
  %1757 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1754, ptr %1757, align 8
  %1758 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1756, ptr %1758, align 8
  call void @memrefCopy(i64 4, ptr %1757, ptr %1758)
  call void @llvm.stackrestore.p0(ptr %1752)
  %1759 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1391, 3, 0
  %1760 = mul i64 1, %1759
  %1761 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1391, 3, 1
  %1762 = mul i64 %1760, %1761
  %1763 = mul i64 %1762, 4
  %1764 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1391, 1
  %1765 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1391, 2
  %1766 = getelementptr float, ptr %1764, i64 %1765
  %1767 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %465, 1
  %1768 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %465, 2
  %1769 = getelementptr float, ptr %1767, i64 %1768
  call void @llvm.memcpy.p0.p0.i64(ptr %1769, ptr %1766, i64 %1763, i1 false)
  %1770 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %1771 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %1772 = insertvalue { ptr, ptr, i64 } poison, ptr %1770, 0
  %1773 = insertvalue { ptr, ptr, i64 } %1772, ptr %1771, 1
  %1774 = insertvalue { ptr, ptr, i64 } %1773, i64 0, 2
  %1775 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %1776 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %1777 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %1778 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %1779 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %1780 = extractvalue { ptr, ptr, i64 } %1774, 0
  %1781 = extractvalue { ptr, ptr, i64 } %1774, 1
  %1782 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1780, 0
  %1783 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1782, ptr %1781, 1
  %1784 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1783, i64 20, 2
  %1785 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1784, i64 16, 3, 0
  %1786 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1785, i64 33, 4, 0
  %1787 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1786, i64 4, 3, 1
  %1788 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1787, i64 1, 4, 1
  %1789 = call ptr @llvm.stacksave.p0()
  %1790 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1788, ptr %1790, align 8
  %1791 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1790, 1
  %1792 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1430, ptr %1792, align 8
  %1793 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1792, 1
  %1794 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1791, ptr %1794, align 8
  %1795 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1793, ptr %1795, align 8
  call void @memrefCopy(i64 4, ptr %1794, ptr %1795)
  call void @llvm.stackrestore.p0(ptr %1789)
  %1796 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %479, ptr %1440, ptr %1372, i32 3, i32 189, i32 117)
  %1797 = call ptr @llvm.stacksave.p0()
  %1798 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1362, ptr %1798, align 8
  %1799 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1798, 1
  %1800 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1751, ptr %1800, align 8
  %1801 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1800, 1
  %1802 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1799, ptr %1802, align 8
  %1803 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1801, ptr %1803, align 8
  call void @memrefCopy(i64 4, ptr %1802, ptr %1803)
  call void @llvm.stackrestore.p0(ptr %1797)
  %1804 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %1805 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %1806 = insertvalue { ptr, ptr, i64 } poison, ptr %1804, 0
  %1807 = insertvalue { ptr, ptr, i64 } %1806, ptr %1805, 1
  %1808 = insertvalue { ptr, ptr, i64 } %1807, i64 0, 2
  %1809 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %1810 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %1811 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %1812 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %1813 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %1814 = extractvalue { ptr, ptr, i64 } %1808, 0
  %1815 = extractvalue { ptr, ptr, i64 } %1808, 1
  %1816 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1814, 0
  %1817 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1816, ptr %1815, 1
  %1818 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1817, i64 816, 2
  %1819 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1818, i64 4, 3, 0
  %1820 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1819, i64 33, 4, 0
  %1821 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1820, i64 4, 3, 1
  %1822 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1821, i64 1, 4, 1
  %1823 = call ptr @llvm.stacksave.p0()
  %1824 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1822, ptr %1824, align 8
  %1825 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1824, 1
  %1826 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1362, ptr %1826, align 8
  %1827 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1826, 1
  %1828 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1825, ptr %1828, align 8
  %1829 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1827, ptr %1829, align 8
  call void @memrefCopy(i64 4, ptr %1828, ptr %1829)
  call void @llvm.stackrestore.p0(ptr %1823)
  %1830 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1391, 3, 0
  %1831 = mul i64 1, %1830
  %1832 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1391, 3, 1
  %1833 = mul i64 %1831, %1832
  %1834 = mul i64 %1833, 4
  %1835 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1391, 1
  %1836 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1391, 2
  %1837 = getelementptr float, ptr %1835, i64 %1836
  %1838 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %465, 1
  %1839 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %465, 2
  %1840 = getelementptr float, ptr %1838, i64 %1839
  call void @llvm.memcpy.p0.p0.i64(ptr %1840, ptr %1837, i64 %1834, i1 false)
  %1841 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %1842 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %1843 = insertvalue { ptr, ptr, i64 } poison, ptr %1841, 0
  %1844 = insertvalue { ptr, ptr, i64 } %1843, ptr %1842, 1
  %1845 = insertvalue { ptr, ptr, i64 } %1844, i64 0, 2
  %1846 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %1847 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %1848 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %1849 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %1850 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %1851 = extractvalue { ptr, ptr, i64 } %1845, 0
  %1852 = extractvalue { ptr, ptr, i64 } %1845, 1
  %1853 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1851, 0
  %1854 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1853, ptr %1852, 1
  %1855 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1854, i64 24, 2
  %1856 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1855, i64 16, 3, 0
  %1857 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1856, i64 33, 4, 0
  %1858 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1857, i64 4, 3, 1
  %1859 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1858, i64 1, 4, 1
  %1860 = call ptr @llvm.stacksave.p0()
  %1861 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1859, ptr %1861, align 8
  %1862 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1861, 1
  %1863 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1430, ptr %1863, align 8
  %1864 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1863, 1
  %1865 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1862, ptr %1865, align 8
  %1866 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1864, ptr %1866, align 8
  call void @memrefCopy(i64 4, ptr %1865, ptr %1866)
  call void @llvm.stackrestore.p0(ptr %1860)
  %1867 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %479, ptr %1440, ptr %1372, i32 1, i32 258, i32 117)
  %1868 = call ptr @llvm.stacksave.p0()
  %1869 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1362, ptr %1869, align 8
  %1870 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1869, 1
  %1871 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1822, ptr %1871, align 8
  %1872 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1871, 1
  %1873 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1870, ptr %1873, align 8
  %1874 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1872, ptr %1874, align 8
  call void @memrefCopy(i64 4, ptr %1873, ptr %1874)
  call void @llvm.stackrestore.p0(ptr %1868)
  %1875 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %1876 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %1877 = insertvalue { ptr, ptr, i64 } poison, ptr %1875, 0
  %1878 = insertvalue { ptr, ptr, i64 } %1877, ptr %1876, 1
  %1879 = insertvalue { ptr, ptr, i64 } %1878, i64 0, 2
  %1880 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %1881 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %1882 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %1883 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %1884 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %1885 = extractvalue { ptr, ptr, i64 } %1879, 0
  %1886 = extractvalue { ptr, ptr, i64 } %1879, 1
  %1887 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1885, 0
  %1888 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1887, ptr %1886, 1
  %1889 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1888, i64 820, 2
  %1890 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1889, i64 4, 3, 0
  %1891 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1890, i64 33, 4, 0
  %1892 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1891, i64 4, 3, 1
  %1893 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1892, i64 1, 4, 1
  %1894 = call ptr @llvm.stacksave.p0()
  %1895 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1893, ptr %1895, align 8
  %1896 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1895, 1
  %1897 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1362, ptr %1897, align 8
  %1898 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1897, 1
  %1899 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1896, ptr %1899, align 8
  %1900 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1898, ptr %1900, align 8
  call void @memrefCopy(i64 4, ptr %1899, ptr %1900)
  call void @llvm.stackrestore.p0(ptr %1894)
  %1901 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1391, 3, 0
  %1902 = mul i64 1, %1901
  %1903 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1391, 3, 1
  %1904 = mul i64 %1902, %1903
  %1905 = mul i64 %1904, 4
  %1906 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1391, 1
  %1907 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1391, 2
  %1908 = getelementptr float, ptr %1906, i64 %1907
  %1909 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %465, 1
  %1910 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %465, 2
  %1911 = getelementptr float, ptr %1909, i64 %1910
  call void @llvm.memcpy.p0.p0.i64(ptr %1911, ptr %1908, i64 %1905, i1 false)
  %1912 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %1913 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %1914 = insertvalue { ptr, ptr, i64 } poison, ptr %1912, 0
  %1915 = insertvalue { ptr, ptr, i64 } %1914, ptr %1913, 1
  %1916 = insertvalue { ptr, ptr, i64 } %1915, i64 0, 2
  %1917 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %1918 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %1919 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %1920 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %1921 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %1922 = extractvalue { ptr, ptr, i64 } %1916, 0
  %1923 = extractvalue { ptr, ptr, i64 } %1916, 1
  %1924 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1922, 0
  %1925 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1924, ptr %1923, 1
  %1926 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1925, i64 28, 2
  %1927 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1926, i64 16, 3, 0
  %1928 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1927, i64 33, 4, 0
  %1929 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1928, i64 4, 3, 1
  %1930 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1929, i64 1, 4, 1
  %1931 = call ptr @llvm.stacksave.p0()
  %1932 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1930, ptr %1932, align 8
  %1933 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1932, 1
  %1934 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1430, ptr %1934, align 8
  %1935 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1934, 1
  %1936 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1933, ptr %1936, align 8
  %1937 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1935, ptr %1937, align 8
  call void @memrefCopy(i64 4, ptr %1936, ptr %1937)
  call void @llvm.stackrestore.p0(ptr %1931)
  %1938 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %479, ptr %1440, ptr %1372, i32 2, i32 258, i32 117)
  %1939 = call ptr @llvm.stacksave.p0()
  %1940 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1362, ptr %1940, align 8
  %1941 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1940, 1
  %1942 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1893, ptr %1942, align 8
  %1943 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1942, 1
  %1944 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1941, ptr %1944, align 8
  %1945 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1943, ptr %1945, align 8
  call void @memrefCopy(i64 4, ptr %1944, ptr %1945)
  call void @llvm.stackrestore.p0(ptr %1939)
  %1946 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %1947 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %1948 = insertvalue { ptr, ptr, i64 } poison, ptr %1946, 0
  %1949 = insertvalue { ptr, ptr, i64 } %1948, ptr %1947, 1
  %1950 = insertvalue { ptr, ptr, i64 } %1949, i64 0, 2
  %1951 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %1952 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %1953 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %1954 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %1955 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %1956 = extractvalue { ptr, ptr, i64 } %1950, 0
  %1957 = extractvalue { ptr, ptr, i64 } %1950, 1
  %1958 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1956, 0
  %1959 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1958, ptr %1957, 1
  %1960 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1959, i64 824, 2
  %1961 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1960, i64 4, 3, 0
  %1962 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1961, i64 33, 4, 0
  %1963 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1962, i64 1, 3, 1
  %1964 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1963, i64 1, 4, 1
  %1965 = call ptr @llvm.stacksave.p0()
  %1966 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1964, ptr %1966, align 8
  %1967 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1966, 1
  %1968 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %427, ptr %1968, align 8
  %1969 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1968, 1
  %1970 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1967, ptr %1970, align 8
  %1971 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1969, ptr %1971, align 8
  call void @memrefCopy(i64 4, ptr %1970, ptr %1971)
  call void @llvm.stackrestore.p0(ptr %1965)
  %1972 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1391, 3, 0
  %1973 = mul i64 1, %1972
  %1974 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1391, 3, 1
  %1975 = mul i64 %1973, %1974
  %1976 = mul i64 %1975, 4
  %1977 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1391, 1
  %1978 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1391, 2
  %1979 = getelementptr float, ptr %1977, i64 %1978
  %1980 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %465, 1
  %1981 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %465, 2
  %1982 = getelementptr float, ptr %1980, i64 %1981
  call void @llvm.memcpy.p0.p0.i64(ptr %1982, ptr %1979, i64 %1976, i1 false)
  %1983 = call ptr @llvm.stacksave.p0()
  %1984 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %498, ptr %1984, align 8
  %1985 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1984, 1
  %1986 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %507, ptr %1986, align 8
  %1987 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1986, 1
  %1988 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1985, ptr %1988, align 8
  %1989 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1987, ptr %1989, align 8
  call void @memrefCopy(i64 4, ptr %1988, ptr %1989)
  call void @llvm.stackrestore.p0(ptr %1983)
  %1990 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 1, ptr %479, ptr %517, ptr %437, i32 3, i32 258, i32 117)
  %1991 = call ptr @llvm.stacksave.p0()
  %1992 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %427, ptr %1992, align 8
  %1993 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1992, 1
  %1994 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1964, ptr %1994, align 8
  %1995 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1994, 1
  %1996 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1993, ptr %1996, align 8
  %1997 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1995, ptr %1997, align 8
  call void @memrefCopy(i64 4, ptr %1996, ptr %1997)
  call void @llvm.stackrestore.p0(ptr %1991)
  %1998 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %1999 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %2000 = insertvalue { ptr, ptr, i64 } poison, ptr %1998, 0
  %2001 = insertvalue { ptr, ptr, i64 } %2000, ptr %1999, 1
  %2002 = insertvalue { ptr, ptr, i64 } %2001, i64 0, 2
  %2003 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %2004 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %2005 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %2006 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %2007 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %2008 = extractvalue { ptr, ptr, i64 } %2002, 0
  %2009 = extractvalue { ptr, ptr, i64 } %2002, 1
  %2010 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2008, 0
  %2011 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2010, ptr %2009, 1
  %2012 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2011, i64 924, 2
  %2013 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2012, i64 4, 3, 0
  %2014 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2013, i64 33, 4, 0
  %2015 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2014, i64 4, 3, 1
  %2016 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2015, i64 1, 4, 1
  %2017 = call ptr @llvm.stacksave.p0()
  %2018 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2016, ptr %2018, align 8
  %2019 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2018, 1
  %2020 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1362, ptr %2020, align 8
  %2021 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2020, 1
  %2022 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2019, ptr %2022, align 8
  %2023 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2021, ptr %2023, align 8
  call void @memrefCopy(i64 4, ptr %2022, ptr %2023)
  call void @llvm.stackrestore.p0(ptr %2017)
  %2024 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %2025 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %2026 = insertvalue { ptr, ptr, i64 } poison, ptr %2024, 0
  %2027 = insertvalue { ptr, ptr, i64 } %2026, ptr %2025, 1
  %2028 = insertvalue { ptr, ptr, i64 } %2027, i64 0, 2
  %2029 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %2030 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %2031 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %2032 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %2033 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %2034 = extractvalue { ptr, ptr, i64 } %2028, 0
  %2035 = extractvalue { ptr, ptr, i64 } %2028, 1
  %2036 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2034, 0
  %2037 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2036, ptr %2035, 1
  %2038 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2037, i64 448, 2
  %2039 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2038, i64 4, 3, 0
  %2040 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2039, i64 16, 4, 0
  %2041 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2040, i64 16, 3, 1
  %2042 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2041, i64 1, 4, 1
  %2043 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2042, 3, 0
  %2044 = mul i64 1, %2043
  %2045 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2042, 3, 1
  %2046 = mul i64 %2044, %2045
  %2047 = mul i64 %2046, 4
  %2048 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2042, 1
  %2049 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2042, 2
  %2050 = getelementptr float, ptr %2048, i64 %2049
  %2051 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %465, 1
  %2052 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %465, 2
  %2053 = getelementptr float, ptr %2051, i64 %2052
  call void @llvm.memcpy.p0.p0.i64(ptr %2053, ptr %2050, i64 %2047, i1 false)
  %2054 = call ptr @llvm.stacksave.p0()
  %2055 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1421, ptr %2055, align 8
  %2056 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2055, 1
  %2057 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1430, ptr %2057, align 8
  %2058 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2057, 1
  %2059 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2056, ptr %2059, align 8
  %2060 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2058, ptr %2060, align 8
  call void @memrefCopy(i64 4, ptr %2059, ptr %2060)
  call void @llvm.stackrestore.p0(ptr %2054)
  %2061 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %479, ptr %1440, ptr %1372, i32 1, i32 327, i32 117)
  %2062 = call ptr @llvm.stacksave.p0()
  %2063 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1362, ptr %2063, align 8
  %2064 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2063, 1
  %2065 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2016, ptr %2065, align 8
  %2066 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2065, 1
  %2067 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2064, ptr %2067, align 8
  %2068 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2066, ptr %2068, align 8
  call void @memrefCopy(i64 4, ptr %2067, ptr %2068)
  call void @llvm.stackrestore.p0(ptr %2062)
  %2069 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %2070 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %2071 = insertvalue { ptr, ptr, i64 } poison, ptr %2069, 0
  %2072 = insertvalue { ptr, ptr, i64 } %2071, ptr %2070, 1
  %2073 = insertvalue { ptr, ptr, i64 } %2072, i64 0, 2
  %2074 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %2075 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %2076 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %2077 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %2078 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %2079 = extractvalue { ptr, ptr, i64 } %2073, 0
  %2080 = extractvalue { ptr, ptr, i64 } %2073, 1
  %2081 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2079, 0
  %2082 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2081, ptr %2080, 1
  %2083 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2082, i64 928, 2
  %2084 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2083, i64 4, 3, 0
  %2085 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2084, i64 33, 4, 0
  %2086 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2085, i64 4, 3, 1
  %2087 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2086, i64 1, 4, 1
  %2088 = call ptr @llvm.stacksave.p0()
  %2089 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2087, ptr %2089, align 8
  %2090 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2089, 1
  %2091 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1362, ptr %2091, align 8
  %2092 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2091, 1
  %2093 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2090, ptr %2093, align 8
  %2094 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2092, ptr %2094, align 8
  call void @memrefCopy(i64 4, ptr %2093, ptr %2094)
  call void @llvm.stackrestore.p0(ptr %2088)
  %2095 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2042, 3, 0
  %2096 = mul i64 1, %2095
  %2097 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2042, 3, 1
  %2098 = mul i64 %2096, %2097
  %2099 = mul i64 %2098, 4
  %2100 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2042, 1
  %2101 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2042, 2
  %2102 = getelementptr float, ptr %2100, i64 %2101
  %2103 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %465, 1
  %2104 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %465, 2
  %2105 = getelementptr float, ptr %2103, i64 %2104
  call void @llvm.memcpy.p0.p0.i64(ptr %2105, ptr %2102, i64 %2099, i1 false)
  %2106 = call ptr @llvm.stacksave.p0()
  %2107 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1504, ptr %2107, align 8
  %2108 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2107, 1
  %2109 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1430, ptr %2109, align 8
  %2110 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2109, 1
  %2111 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2108, ptr %2111, align 8
  %2112 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2110, ptr %2112, align 8
  call void @memrefCopy(i64 4, ptr %2111, ptr %2112)
  call void @llvm.stackrestore.p0(ptr %2106)
  %2113 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %479, ptr %1440, ptr %1372, i32 2, i32 327, i32 117)
  %2114 = call ptr @llvm.stacksave.p0()
  %2115 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1362, ptr %2115, align 8
  %2116 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2115, 1
  %2117 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2087, ptr %2117, align 8
  %2118 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2117, 1
  %2119 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2116, ptr %2119, align 8
  %2120 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2118, ptr %2120, align 8
  call void @memrefCopy(i64 4, ptr %2119, ptr %2120)
  call void @llvm.stackrestore.p0(ptr %2114)
  %2121 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %2122 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %2123 = insertvalue { ptr, ptr, i64 } poison, ptr %2121, 0
  %2124 = insertvalue { ptr, ptr, i64 } %2123, ptr %2122, 1
  %2125 = insertvalue { ptr, ptr, i64 } %2124, i64 0, 2
  %2126 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %2127 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %2128 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %2129 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %2130 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %2131 = extractvalue { ptr, ptr, i64 } %2125, 0
  %2132 = extractvalue { ptr, ptr, i64 } %2125, 1
  %2133 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2131, 0
  %2134 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2133, ptr %2132, 1
  %2135 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2134, i64 932, 2
  %2136 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2135, i64 4, 3, 0
  %2137 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2136, i64 33, 4, 0
  %2138 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2137, i64 4, 3, 1
  %2139 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2138, i64 1, 4, 1
  %2140 = call ptr @llvm.stacksave.p0()
  %2141 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2139, ptr %2141, align 8
  %2142 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2141, 1
  %2143 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1362, ptr %2143, align 8
  %2144 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2143, 1
  %2145 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2142, ptr %2145, align 8
  %2146 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2144, ptr %2146, align 8
  call void @memrefCopy(i64 4, ptr %2145, ptr %2146)
  call void @llvm.stackrestore.p0(ptr %2140)
  %2147 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2042, 3, 0
  %2148 = mul i64 1, %2147
  %2149 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2042, 3, 1
  %2150 = mul i64 %2148, %2149
  %2151 = mul i64 %2150, 4
  %2152 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2042, 1
  %2153 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2042, 2
  %2154 = getelementptr float, ptr %2152, i64 %2153
  %2155 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %465, 1
  %2156 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %465, 2
  %2157 = getelementptr float, ptr %2155, i64 %2156
  call void @llvm.memcpy.p0.p0.i64(ptr %2157, ptr %2154, i64 %2151, i1 false)
  %2158 = call ptr @llvm.stacksave.p0()
  %2159 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1575, ptr %2159, align 8
  %2160 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2159, 1
  %2161 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1430, ptr %2161, align 8
  %2162 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2161, 1
  %2163 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2160, ptr %2163, align 8
  %2164 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2162, ptr %2164, align 8
  call void @memrefCopy(i64 4, ptr %2163, ptr %2164)
  call void @llvm.stackrestore.p0(ptr %2158)
  %2165 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %479, ptr %1440, ptr %1372, i32 3, i32 327, i32 117)
  %2166 = call ptr @llvm.stacksave.p0()
  %2167 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1362, ptr %2167, align 8
  %2168 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2167, 1
  %2169 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2139, ptr %2169, align 8
  %2170 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2169, 1
  %2171 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2168, ptr %2171, align 8
  %2172 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2170, ptr %2172, align 8
  call void @memrefCopy(i64 4, ptr %2171, ptr %2172)
  call void @llvm.stackrestore.p0(ptr %2166)
  %2173 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %2174 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %2175 = insertvalue { ptr, ptr, i64 } poison, ptr %2173, 0
  %2176 = insertvalue { ptr, ptr, i64 } %2175, ptr %2174, 1
  %2177 = insertvalue { ptr, ptr, i64 } %2176, i64 0, 2
  %2178 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %2179 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %2180 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %2181 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %2182 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %2183 = extractvalue { ptr, ptr, i64 } %2177, 0
  %2184 = extractvalue { ptr, ptr, i64 } %2177, 1
  %2185 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2183, 0
  %2186 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2185, ptr %2184, 1
  %2187 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2186, i64 936, 2
  %2188 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2187, i64 4, 3, 0
  %2189 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2188, i64 33, 4, 0
  %2190 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2189, i64 4, 3, 1
  %2191 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2190, i64 1, 4, 1
  %2192 = call ptr @llvm.stacksave.p0()
  %2193 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2191, ptr %2193, align 8
  %2194 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2193, 1
  %2195 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1362, ptr %2195, align 8
  %2196 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2195, 1
  %2197 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2194, ptr %2197, align 8
  %2198 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2196, ptr %2198, align 8
  call void @memrefCopy(i64 4, ptr %2197, ptr %2198)
  call void @llvm.stackrestore.p0(ptr %2192)
  %2199 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2042, 3, 0
  %2200 = mul i64 1, %2199
  %2201 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2042, 3, 1
  %2202 = mul i64 %2200, %2201
  %2203 = mul i64 %2202, 4
  %2204 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2042, 1
  %2205 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2042, 2
  %2206 = getelementptr float, ptr %2204, i64 %2205
  %2207 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %465, 1
  %2208 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %465, 2
  %2209 = getelementptr float, ptr %2207, i64 %2208
  call void @llvm.memcpy.p0.p0.i64(ptr %2209, ptr %2206, i64 %2203, i1 false)
  %2210 = call ptr @llvm.stacksave.p0()
  %2211 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1646, ptr %2211, align 8
  %2212 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2211, 1
  %2213 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1430, ptr %2213, align 8
  %2214 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2213, 1
  %2215 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2212, ptr %2215, align 8
  %2216 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2214, ptr %2216, align 8
  call void @memrefCopy(i64 4, ptr %2215, ptr %2216)
  call void @llvm.stackrestore.p0(ptr %2210)
  %2217 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %479, ptr %1440, ptr %1372, i32 1, i32 396, i32 117)
  %2218 = call ptr @llvm.stacksave.p0()
  %2219 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1362, ptr %2219, align 8
  %2220 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2219, 1
  %2221 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2191, ptr %2221, align 8
  %2222 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2221, 1
  %2223 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2220, ptr %2223, align 8
  %2224 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2222, ptr %2224, align 8
  call void @memrefCopy(i64 4, ptr %2223, ptr %2224)
  call void @llvm.stackrestore.p0(ptr %2218)
  %2225 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %2226 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %2227 = insertvalue { ptr, ptr, i64 } poison, ptr %2225, 0
  %2228 = insertvalue { ptr, ptr, i64 } %2227, ptr %2226, 1
  %2229 = insertvalue { ptr, ptr, i64 } %2228, i64 0, 2
  %2230 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %2231 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %2232 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %2233 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %2234 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %2235 = extractvalue { ptr, ptr, i64 } %2229, 0
  %2236 = extractvalue { ptr, ptr, i64 } %2229, 1
  %2237 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2235, 0
  %2238 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2237, ptr %2236, 1
  %2239 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2238, i64 940, 2
  %2240 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2239, i64 4, 3, 0
  %2241 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2240, i64 33, 4, 0
  %2242 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2241, i64 4, 3, 1
  %2243 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2242, i64 1, 4, 1
  %2244 = call ptr @llvm.stacksave.p0()
  %2245 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2243, ptr %2245, align 8
  %2246 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2245, 1
  %2247 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1362, ptr %2247, align 8
  %2248 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2247, 1
  %2249 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2246, ptr %2249, align 8
  %2250 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2248, ptr %2250, align 8
  call void @memrefCopy(i64 4, ptr %2249, ptr %2250)
  call void @llvm.stackrestore.p0(ptr %2244)
  %2251 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2042, 3, 0
  %2252 = mul i64 1, %2251
  %2253 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2042, 3, 1
  %2254 = mul i64 %2252, %2253
  %2255 = mul i64 %2254, 4
  %2256 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2042, 1
  %2257 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2042, 2
  %2258 = getelementptr float, ptr %2256, i64 %2257
  %2259 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %465, 1
  %2260 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %465, 2
  %2261 = getelementptr float, ptr %2259, i64 %2260
  call void @llvm.memcpy.p0.p0.i64(ptr %2261, ptr %2258, i64 %2255, i1 false)
  %2262 = call ptr @llvm.stacksave.p0()
  %2263 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1717, ptr %2263, align 8
  %2264 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2263, 1
  %2265 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1430, ptr %2265, align 8
  %2266 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2265, 1
  %2267 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2264, ptr %2267, align 8
  %2268 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2266, ptr %2268, align 8
  call void @memrefCopy(i64 4, ptr %2267, ptr %2268)
  call void @llvm.stackrestore.p0(ptr %2262)
  %2269 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %479, ptr %1440, ptr %1372, i32 2, i32 396, i32 117)
  %2270 = call ptr @llvm.stacksave.p0()
  %2271 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1362, ptr %2271, align 8
  %2272 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2271, 1
  %2273 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2243, ptr %2273, align 8
  %2274 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2273, 1
  %2275 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2272, ptr %2275, align 8
  %2276 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2274, ptr %2276, align 8
  call void @memrefCopy(i64 4, ptr %2275, ptr %2276)
  call void @llvm.stackrestore.p0(ptr %2270)
  %2277 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %2278 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %2279 = insertvalue { ptr, ptr, i64 } poison, ptr %2277, 0
  %2280 = insertvalue { ptr, ptr, i64 } %2279, ptr %2278, 1
  %2281 = insertvalue { ptr, ptr, i64 } %2280, i64 0, 2
  %2282 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %2283 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %2284 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %2285 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %2286 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %2287 = extractvalue { ptr, ptr, i64 } %2281, 0
  %2288 = extractvalue { ptr, ptr, i64 } %2281, 1
  %2289 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2287, 0
  %2290 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2289, ptr %2288, 1
  %2291 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2290, i64 944, 2
  %2292 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2291, i64 4, 3, 0
  %2293 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2292, i64 33, 4, 0
  %2294 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2293, i64 4, 3, 1
  %2295 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2294, i64 1, 4, 1
  %2296 = call ptr @llvm.stacksave.p0()
  %2297 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2295, ptr %2297, align 8
  %2298 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2297, 1
  %2299 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1362, ptr %2299, align 8
  %2300 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2299, 1
  %2301 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2298, ptr %2301, align 8
  %2302 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2300, ptr %2302, align 8
  call void @memrefCopy(i64 4, ptr %2301, ptr %2302)
  call void @llvm.stackrestore.p0(ptr %2296)
  %2303 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2042, 3, 0
  %2304 = mul i64 1, %2303
  %2305 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2042, 3, 1
  %2306 = mul i64 %2304, %2305
  %2307 = mul i64 %2306, 4
  %2308 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2042, 1
  %2309 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2042, 2
  %2310 = getelementptr float, ptr %2308, i64 %2309
  %2311 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %465, 1
  %2312 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %465, 2
  %2313 = getelementptr float, ptr %2311, i64 %2312
  call void @llvm.memcpy.p0.p0.i64(ptr %2313, ptr %2310, i64 %2307, i1 false)
  %2314 = call ptr @llvm.stacksave.p0()
  %2315 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1788, ptr %2315, align 8
  %2316 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2315, 1
  %2317 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1430, ptr %2317, align 8
  %2318 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2317, 1
  %2319 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2316, ptr %2319, align 8
  %2320 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2318, ptr %2320, align 8
  call void @memrefCopy(i64 4, ptr %2319, ptr %2320)
  call void @llvm.stackrestore.p0(ptr %2314)
  %2321 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %479, ptr %1440, ptr %1372, i32 3, i32 396, i32 117)
  %2322 = call ptr @llvm.stacksave.p0()
  %2323 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1362, ptr %2323, align 8
  %2324 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2323, 1
  %2325 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2295, ptr %2325, align 8
  %2326 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2325, 1
  %2327 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2324, ptr %2327, align 8
  %2328 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2326, ptr %2328, align 8
  call void @memrefCopy(i64 4, ptr %2327, ptr %2328)
  call void @llvm.stackrestore.p0(ptr %2322)
  %2329 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %2330 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %2331 = insertvalue { ptr, ptr, i64 } poison, ptr %2329, 0
  %2332 = insertvalue { ptr, ptr, i64 } %2331, ptr %2330, 1
  %2333 = insertvalue { ptr, ptr, i64 } %2332, i64 0, 2
  %2334 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %2335 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %2336 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %2337 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %2338 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %2339 = extractvalue { ptr, ptr, i64 } %2333, 0
  %2340 = extractvalue { ptr, ptr, i64 } %2333, 1
  %2341 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2339, 0
  %2342 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2341, ptr %2340, 1
  %2343 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2342, i64 948, 2
  %2344 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2343, i64 4, 3, 0
  %2345 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2344, i64 33, 4, 0
  %2346 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2345, i64 4, 3, 1
  %2347 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2346, i64 1, 4, 1
  %2348 = call ptr @llvm.stacksave.p0()
  %2349 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2347, ptr %2349, align 8
  %2350 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2349, 1
  %2351 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1362, ptr %2351, align 8
  %2352 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2351, 1
  %2353 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2350, ptr %2353, align 8
  %2354 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2352, ptr %2354, align 8
  call void @memrefCopy(i64 4, ptr %2353, ptr %2354)
  call void @llvm.stackrestore.p0(ptr %2348)
  %2355 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2042, 3, 0
  %2356 = mul i64 1, %2355
  %2357 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2042, 3, 1
  %2358 = mul i64 %2356, %2357
  %2359 = mul i64 %2358, 4
  %2360 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2042, 1
  %2361 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2042, 2
  %2362 = getelementptr float, ptr %2360, i64 %2361
  %2363 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %465, 1
  %2364 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %465, 2
  %2365 = getelementptr float, ptr %2363, i64 %2364
  call void @llvm.memcpy.p0.p0.i64(ptr %2365, ptr %2362, i64 %2359, i1 false)
  %2366 = call ptr @llvm.stacksave.p0()
  %2367 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1859, ptr %2367, align 8
  %2368 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2367, 1
  %2369 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1430, ptr %2369, align 8
  %2370 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2369, 1
  %2371 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2368, ptr %2371, align 8
  %2372 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2370, ptr %2372, align 8
  call void @memrefCopy(i64 4, ptr %2371, ptr %2372)
  call void @llvm.stackrestore.p0(ptr %2366)
  %2373 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %479, ptr %1440, ptr %1372, i32 1, i32 465, i32 117)
  %2374 = call ptr @llvm.stacksave.p0()
  %2375 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1362, ptr %2375, align 8
  %2376 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2375, 1
  %2377 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2347, ptr %2377, align 8
  %2378 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2377, 1
  %2379 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2376, ptr %2379, align 8
  %2380 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2378, ptr %2380, align 8
  call void @memrefCopy(i64 4, ptr %2379, ptr %2380)
  call void @llvm.stackrestore.p0(ptr %2374)
  %2381 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %2382 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %2383 = insertvalue { ptr, ptr, i64 } poison, ptr %2381, 0
  %2384 = insertvalue { ptr, ptr, i64 } %2383, ptr %2382, 1
  %2385 = insertvalue { ptr, ptr, i64 } %2384, i64 0, 2
  %2386 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %2387 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %2388 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %2389 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %2390 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %2391 = extractvalue { ptr, ptr, i64 } %2385, 0
  %2392 = extractvalue { ptr, ptr, i64 } %2385, 1
  %2393 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2391, 0
  %2394 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2393, ptr %2392, 1
  %2395 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2394, i64 952, 2
  %2396 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2395, i64 4, 3, 0
  %2397 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2396, i64 33, 4, 0
  %2398 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2397, i64 4, 3, 1
  %2399 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2398, i64 1, 4, 1
  %2400 = call ptr @llvm.stacksave.p0()
  %2401 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2399, ptr %2401, align 8
  %2402 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2401, 1
  %2403 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1362, ptr %2403, align 8
  %2404 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2403, 1
  %2405 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2402, ptr %2405, align 8
  %2406 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2404, ptr %2406, align 8
  call void @memrefCopy(i64 4, ptr %2405, ptr %2406)
  call void @llvm.stackrestore.p0(ptr %2400)
  %2407 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2042, 3, 0
  %2408 = mul i64 1, %2407
  %2409 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2042, 3, 1
  %2410 = mul i64 %2408, %2409
  %2411 = mul i64 %2410, 4
  %2412 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2042, 1
  %2413 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2042, 2
  %2414 = getelementptr float, ptr %2412, i64 %2413
  %2415 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %465, 1
  %2416 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %465, 2
  %2417 = getelementptr float, ptr %2415, i64 %2416
  call void @llvm.memcpy.p0.p0.i64(ptr %2417, ptr %2414, i64 %2411, i1 false)
  %2418 = call ptr @llvm.stacksave.p0()
  %2419 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1930, ptr %2419, align 8
  %2420 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2419, 1
  %2421 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1430, ptr %2421, align 8
  %2422 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2421, 1
  %2423 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2420, ptr %2423, align 8
  %2424 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2422, ptr %2424, align 8
  call void @memrefCopy(i64 4, ptr %2423, ptr %2424)
  call void @llvm.stackrestore.p0(ptr %2418)
  %2425 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %479, ptr %1440, ptr %1372, i32 2, i32 465, i32 117)
  %2426 = call ptr @llvm.stacksave.p0()
  %2427 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1362, ptr %2427, align 8
  %2428 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2427, 1
  %2429 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2399, ptr %2429, align 8
  %2430 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2429, 1
  %2431 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2428, ptr %2431, align 8
  %2432 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2430, ptr %2432, align 8
  call void @memrefCopy(i64 4, ptr %2431, ptr %2432)
  call void @llvm.stackrestore.p0(ptr %2426)
  %2433 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %2434 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %2435 = insertvalue { ptr, ptr, i64 } poison, ptr %2433, 0
  %2436 = insertvalue { ptr, ptr, i64 } %2435, ptr %2434, 1
  %2437 = insertvalue { ptr, ptr, i64 } %2436, i64 0, 2
  %2438 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %2439 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %2440 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %2441 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %2442 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %2443 = extractvalue { ptr, ptr, i64 } %2437, 0
  %2444 = extractvalue { ptr, ptr, i64 } %2437, 1
  %2445 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2443, 0
  %2446 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2445, ptr %2444, 1
  %2447 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2446, i64 956, 2
  %2448 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2447, i64 4, 3, 0
  %2449 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2448, i64 33, 4, 0
  %2450 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2449, i64 1, 3, 1
  %2451 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2450, i64 1, 4, 1
  %2452 = call ptr @llvm.stacksave.p0()
  %2453 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2451, ptr %2453, align 8
  %2454 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2453, 1
  %2455 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %427, ptr %2455, align 8
  %2456 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2455, 1
  %2457 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2454, ptr %2457, align 8
  %2458 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2456, ptr %2458, align 8
  call void @memrefCopy(i64 4, ptr %2457, ptr %2458)
  call void @llvm.stackrestore.p0(ptr %2452)
  %2459 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2042, 3, 0
  %2460 = mul i64 1, %2459
  %2461 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2042, 3, 1
  %2462 = mul i64 %2460, %2461
  %2463 = mul i64 %2462, 4
  %2464 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2042, 1
  %2465 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2042, 2
  %2466 = getelementptr float, ptr %2464, i64 %2465
  %2467 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %465, 1
  %2468 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %465, 2
  %2469 = getelementptr float, ptr %2467, i64 %2468
  call void @llvm.memcpy.p0.p0.i64(ptr %2469, ptr %2466, i64 %2463, i1 false)
  %2470 = call ptr @llvm.stacksave.p0()
  %2471 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %498, ptr %2471, align 8
  %2472 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2471, 1
  %2473 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %507, ptr %2473, align 8
  %2474 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2473, 1
  %2475 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2472, ptr %2475, align 8
  %2476 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2474, ptr %2476, align 8
  call void @memrefCopy(i64 4, ptr %2475, ptr %2476)
  call void @llvm.stackrestore.p0(ptr %2470)
  %2477 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 1, ptr %479, ptr %517, ptr %437, i32 3, i32 465, i32 117)
  %2478 = call ptr @llvm.stacksave.p0()
  %2479 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %427, ptr %2479, align 8
  %2480 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2479, 1
  %2481 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2451, ptr %2481, align 8
  %2482 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2481, 1
  %2483 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2480, ptr %2483, align 8
  %2484 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2482, ptr %2484, align 8
  call void @memrefCopy(i64 4, ptr %2483, ptr %2484)
  call void @llvm.stackrestore.p0(ptr %2478)
  %2485 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %2486 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %2487 = insertvalue { ptr, ptr, i64 } poison, ptr %2485, 0
  %2488 = insertvalue { ptr, ptr, i64 } %2487, ptr %2486, 1
  %2489 = insertvalue { ptr, ptr, i64 } %2488, i64 0, 2
  %2490 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %2491 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %2492 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %2493 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %2494 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %2495 = extractvalue { ptr, ptr, i64 } %2489, 0
  %2496 = extractvalue { ptr, ptr, i64 } %2489, 1
  %2497 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2495, 0
  %2498 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2497, ptr %2496, 1
  %2499 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2498, i64 1056, 2
  %2500 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2499, i64 1, 3, 0
  %2501 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2500, i64 33, 4, 0
  %2502 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2501, i64 4, 3, 1
  %2503 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2502, i64 1, 4, 1
  %2504 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 0
  %2505 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 1
  %2506 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2504, 0
  %2507 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2506, ptr %2505, 1
  %2508 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2507, i64 0, 2
  %2509 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2508, i64 1, 3, 0
  %2510 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2509, i64 4, 4, 0
  %2511 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2510, i64 4, 3, 1
  %2512 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2511, i64 1, 4, 1
  %2513 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2503, 3, 0
  %2514 = mul i64 1, %2513
  %2515 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2503, 3, 1
  %2516 = mul i64 %2514, %2515
  %2517 = mul i64 %2516, 4
  %2518 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2503, 1
  %2519 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2503, 2
  %2520 = getelementptr float, ptr %2518, i64 %2519
  %2521 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2512, 1
  %2522 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2512, 2
  %2523 = getelementptr float, ptr %2521, i64 %2522
  call void @llvm.memcpy.p0.p0.i64(ptr %2523, ptr %2520, i64 %2517, i1 false)
  %2524 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 1
  %2525 = ptrtoint ptr %2524 to i64
  %2526 = inttoptr i64 %2525 to ptr
  %2527 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %2528 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %2529 = insertvalue { ptr, ptr, i64 } poison, ptr %2527, 0
  %2530 = insertvalue { ptr, ptr, i64 } %2529, ptr %2528, 1
  %2531 = insertvalue { ptr, ptr, i64 } %2530, i64 0, 2
  %2532 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %2533 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %2534 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %2535 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %2536 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %2537 = extractvalue { ptr, ptr, i64 } %2531, 0
  %2538 = extractvalue { ptr, ptr, i64 } %2531, 1
  %2539 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2537, 0
  %2540 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2539, ptr %2538, 1
  %2541 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2540, i64 512, 2
  %2542 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2541, i64 1, 3, 0
  %2543 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2542, i64 16, 4, 0
  %2544 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2543, i64 16, 3, 1
  %2545 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2544, i64 1, 4, 1
  %2546 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 0
  %2547 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %2548 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2546, 0
  %2549 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2548, ptr %2547, 1
  %2550 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2549, i64 0, 2
  %2551 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2550, i64 1, 3, 0
  %2552 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2551, i64 16, 4, 0
  %2553 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2552, i64 16, 3, 1
  %2554 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2553, i64 1, 4, 1
  %2555 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2545, 3, 0
  %2556 = mul i64 1, %2555
  %2557 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2545, 3, 1
  %2558 = mul i64 %2556, %2557
  %2559 = mul i64 %2558, 4
  %2560 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2545, 1
  %2561 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2545, 2
  %2562 = getelementptr float, ptr %2560, i64 %2561
  %2563 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2554, 1
  %2564 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2554, 2
  %2565 = getelementptr float, ptr %2563, i64 %2564
  call void @llvm.memcpy.p0.p0.i64(ptr %2565, ptr %2562, i64 %2559, i1 false)
  %2566 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %2567 = ptrtoint ptr %2566 to i64
  %2568 = inttoptr i64 %2567 to ptr
  %2569 = call ptr @llvm.stacksave.p0()
  %2570 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1421, ptr %2570, align 8
  %2571 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2570, 1
  %2572 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1430, ptr %2572, align 8
  %2573 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2572, 1
  %2574 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2571, ptr %2574, align 8
  %2575 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2573, ptr %2575, align 8
  call void @memrefCopy(i64 4, ptr %2574, ptr %2575)
  call void @llvm.stackrestore.p0(ptr %2569)
  %2576 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 1, i32 16, i32 4, ptr %2568, ptr %1440, ptr %2526, i32 1, i32 534, i32 117)
  %2577 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2512, 3, 0
  %2578 = mul i64 1, %2577
  %2579 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2512, 3, 1
  %2580 = mul i64 %2578, %2579
  %2581 = mul i64 %2580, 4
  %2582 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2512, 1
  %2583 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2512, 2
  %2584 = getelementptr float, ptr %2582, i64 %2583
  %2585 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2503, 1
  %2586 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2503, 2
  %2587 = getelementptr float, ptr %2585, i64 %2586
  call void @llvm.memcpy.p0.p0.i64(ptr %2587, ptr %2584, i64 %2581, i1 false)
  %2588 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %2589 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %2590 = insertvalue { ptr, ptr, i64 } poison, ptr %2588, 0
  %2591 = insertvalue { ptr, ptr, i64 } %2590, ptr %2589, 1
  %2592 = insertvalue { ptr, ptr, i64 } %2591, i64 0, 2
  %2593 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %2594 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %2595 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %2596 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %2597 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %2598 = extractvalue { ptr, ptr, i64 } %2592, 0
  %2599 = extractvalue { ptr, ptr, i64 } %2592, 1
  %2600 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2598, 0
  %2601 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2600, ptr %2599, 1
  %2602 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2601, i64 1060, 2
  %2603 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2602, i64 1, 3, 0
  %2604 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2603, i64 33, 4, 0
  %2605 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2604, i64 4, 3, 1
  %2606 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2605, i64 1, 4, 1
  %2607 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2606, 3, 0
  %2608 = mul i64 1, %2607
  %2609 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2606, 3, 1
  %2610 = mul i64 %2608, %2609
  %2611 = mul i64 %2610, 4
  %2612 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2606, 1
  %2613 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2606, 2
  %2614 = getelementptr float, ptr %2612, i64 %2613
  %2615 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2512, 1
  %2616 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2512, 2
  %2617 = getelementptr float, ptr %2615, i64 %2616
  call void @llvm.memcpy.p0.p0.i64(ptr %2617, ptr %2614, i64 %2611, i1 false)
  %2618 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2545, 3, 0
  %2619 = mul i64 1, %2618
  %2620 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2545, 3, 1
  %2621 = mul i64 %2619, %2620
  %2622 = mul i64 %2621, 4
  %2623 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2545, 1
  %2624 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2545, 2
  %2625 = getelementptr float, ptr %2623, i64 %2624
  %2626 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2554, 1
  %2627 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2554, 2
  %2628 = getelementptr float, ptr %2626, i64 %2627
  call void @llvm.memcpy.p0.p0.i64(ptr %2628, ptr %2625, i64 %2622, i1 false)
  %2629 = call ptr @llvm.stacksave.p0()
  %2630 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1504, ptr %2630, align 8
  %2631 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2630, 1
  %2632 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1430, ptr %2632, align 8
  %2633 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2632, 1
  %2634 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2631, ptr %2634, align 8
  %2635 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2633, ptr %2635, align 8
  call void @memrefCopy(i64 4, ptr %2634, ptr %2635)
  call void @llvm.stackrestore.p0(ptr %2629)
  %2636 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 1, i32 16, i32 4, ptr %2568, ptr %1440, ptr %2526, i32 2, i32 534, i32 117)
  %2637 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2512, 3, 0
  %2638 = mul i64 1, %2637
  %2639 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2512, 3, 1
  %2640 = mul i64 %2638, %2639
  %2641 = mul i64 %2640, 4
  %2642 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2512, 1
  %2643 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2512, 2
  %2644 = getelementptr float, ptr %2642, i64 %2643
  %2645 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2606, 1
  %2646 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2606, 2
  %2647 = getelementptr float, ptr %2645, i64 %2646
  call void @llvm.memcpy.p0.p0.i64(ptr %2647, ptr %2644, i64 %2641, i1 false)
  %2648 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %2649 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %2650 = insertvalue { ptr, ptr, i64 } poison, ptr %2648, 0
  %2651 = insertvalue { ptr, ptr, i64 } %2650, ptr %2649, 1
  %2652 = insertvalue { ptr, ptr, i64 } %2651, i64 0, 2
  %2653 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %2654 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %2655 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %2656 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %2657 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %2658 = extractvalue { ptr, ptr, i64 } %2652, 0
  %2659 = extractvalue { ptr, ptr, i64 } %2652, 1
  %2660 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2658, 0
  %2661 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2660, ptr %2659, 1
  %2662 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2661, i64 1064, 2
  %2663 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2662, i64 1, 3, 0
  %2664 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2663, i64 33, 4, 0
  %2665 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2664, i64 4, 3, 1
  %2666 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2665, i64 1, 4, 1
  %2667 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2666, 3, 0
  %2668 = mul i64 1, %2667
  %2669 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2666, 3, 1
  %2670 = mul i64 %2668, %2669
  %2671 = mul i64 %2670, 4
  %2672 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2666, 1
  %2673 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2666, 2
  %2674 = getelementptr float, ptr %2672, i64 %2673
  %2675 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2512, 1
  %2676 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2512, 2
  %2677 = getelementptr float, ptr %2675, i64 %2676
  call void @llvm.memcpy.p0.p0.i64(ptr %2677, ptr %2674, i64 %2671, i1 false)
  %2678 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2545, 3, 0
  %2679 = mul i64 1, %2678
  %2680 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2545, 3, 1
  %2681 = mul i64 %2679, %2680
  %2682 = mul i64 %2681, 4
  %2683 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2545, 1
  %2684 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2545, 2
  %2685 = getelementptr float, ptr %2683, i64 %2684
  %2686 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2554, 1
  %2687 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2554, 2
  %2688 = getelementptr float, ptr %2686, i64 %2687
  call void @llvm.memcpy.p0.p0.i64(ptr %2688, ptr %2685, i64 %2682, i1 false)
  %2689 = call ptr @llvm.stacksave.p0()
  %2690 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1575, ptr %2690, align 8
  %2691 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2690, 1
  %2692 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1430, ptr %2692, align 8
  %2693 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2692, 1
  %2694 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2691, ptr %2694, align 8
  %2695 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2693, ptr %2695, align 8
  call void @memrefCopy(i64 4, ptr %2694, ptr %2695)
  call void @llvm.stackrestore.p0(ptr %2689)
  %2696 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 1, i32 16, i32 4, ptr %2568, ptr %1440, ptr %2526, i32 3, i32 534, i32 117)
  %2697 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2512, 3, 0
  %2698 = mul i64 1, %2697
  %2699 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2512, 3, 1
  %2700 = mul i64 %2698, %2699
  %2701 = mul i64 %2700, 4
  %2702 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2512, 1
  %2703 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2512, 2
  %2704 = getelementptr float, ptr %2702, i64 %2703
  %2705 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2666, 1
  %2706 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2666, 2
  %2707 = getelementptr float, ptr %2705, i64 %2706
  call void @llvm.memcpy.p0.p0.i64(ptr %2707, ptr %2704, i64 %2701, i1 false)
  %2708 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %2709 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %2710 = insertvalue { ptr, ptr, i64 } poison, ptr %2708, 0
  %2711 = insertvalue { ptr, ptr, i64 } %2710, ptr %2709, 1
  %2712 = insertvalue { ptr, ptr, i64 } %2711, i64 0, 2
  %2713 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %2714 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %2715 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %2716 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %2717 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %2718 = extractvalue { ptr, ptr, i64 } %2712, 0
  %2719 = extractvalue { ptr, ptr, i64 } %2712, 1
  %2720 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2718, 0
  %2721 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2720, ptr %2719, 1
  %2722 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2721, i64 1068, 2
  %2723 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2722, i64 1, 3, 0
  %2724 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2723, i64 33, 4, 0
  %2725 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2724, i64 4, 3, 1
  %2726 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2725, i64 1, 4, 1
  %2727 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2726, 3, 0
  %2728 = mul i64 1, %2727
  %2729 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2726, 3, 1
  %2730 = mul i64 %2728, %2729
  %2731 = mul i64 %2730, 4
  %2732 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2726, 1
  %2733 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2726, 2
  %2734 = getelementptr float, ptr %2732, i64 %2733
  %2735 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2512, 1
  %2736 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2512, 2
  %2737 = getelementptr float, ptr %2735, i64 %2736
  call void @llvm.memcpy.p0.p0.i64(ptr %2737, ptr %2734, i64 %2731, i1 false)
  %2738 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2545, 3, 0
  %2739 = mul i64 1, %2738
  %2740 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2545, 3, 1
  %2741 = mul i64 %2739, %2740
  %2742 = mul i64 %2741, 4
  %2743 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2545, 1
  %2744 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2545, 2
  %2745 = getelementptr float, ptr %2743, i64 %2744
  %2746 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2554, 1
  %2747 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2554, 2
  %2748 = getelementptr float, ptr %2746, i64 %2747
  call void @llvm.memcpy.p0.p0.i64(ptr %2748, ptr %2745, i64 %2742, i1 false)
  %2749 = call ptr @llvm.stacksave.p0()
  %2750 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1646, ptr %2750, align 8
  %2751 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2750, 1
  %2752 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1430, ptr %2752, align 8
  %2753 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2752, 1
  %2754 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2751, ptr %2754, align 8
  %2755 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2753, ptr %2755, align 8
  call void @memrefCopy(i64 4, ptr %2754, ptr %2755)
  call void @llvm.stackrestore.p0(ptr %2749)
  %2756 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 1, i32 16, i32 4, ptr %2568, ptr %1440, ptr %2526, i32 1, i32 603, i32 117)
  %2757 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2512, 3, 0
  %2758 = mul i64 1, %2757
  %2759 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2512, 3, 1
  %2760 = mul i64 %2758, %2759
  %2761 = mul i64 %2760, 4
  %2762 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2512, 1
  %2763 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2512, 2
  %2764 = getelementptr float, ptr %2762, i64 %2763
  %2765 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2726, 1
  %2766 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2726, 2
  %2767 = getelementptr float, ptr %2765, i64 %2766
  call void @llvm.memcpy.p0.p0.i64(ptr %2767, ptr %2764, i64 %2761, i1 false)
  %2768 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %2769 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %2770 = insertvalue { ptr, ptr, i64 } poison, ptr %2768, 0
  %2771 = insertvalue { ptr, ptr, i64 } %2770, ptr %2769, 1
  %2772 = insertvalue { ptr, ptr, i64 } %2771, i64 0, 2
  %2773 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %2774 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %2775 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %2776 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %2777 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %2778 = extractvalue { ptr, ptr, i64 } %2772, 0
  %2779 = extractvalue { ptr, ptr, i64 } %2772, 1
  %2780 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2778, 0
  %2781 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2780, ptr %2779, 1
  %2782 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2781, i64 1072, 2
  %2783 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2782, i64 1, 3, 0
  %2784 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2783, i64 33, 4, 0
  %2785 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2784, i64 4, 3, 1
  %2786 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2785, i64 1, 4, 1
  %2787 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2786, 3, 0
  %2788 = mul i64 1, %2787
  %2789 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2786, 3, 1
  %2790 = mul i64 %2788, %2789
  %2791 = mul i64 %2790, 4
  %2792 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2786, 1
  %2793 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2786, 2
  %2794 = getelementptr float, ptr %2792, i64 %2793
  %2795 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2512, 1
  %2796 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2512, 2
  %2797 = getelementptr float, ptr %2795, i64 %2796
  call void @llvm.memcpy.p0.p0.i64(ptr %2797, ptr %2794, i64 %2791, i1 false)
  %2798 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2545, 3, 0
  %2799 = mul i64 1, %2798
  %2800 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2545, 3, 1
  %2801 = mul i64 %2799, %2800
  %2802 = mul i64 %2801, 4
  %2803 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2545, 1
  %2804 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2545, 2
  %2805 = getelementptr float, ptr %2803, i64 %2804
  %2806 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2554, 1
  %2807 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2554, 2
  %2808 = getelementptr float, ptr %2806, i64 %2807
  call void @llvm.memcpy.p0.p0.i64(ptr %2808, ptr %2805, i64 %2802, i1 false)
  %2809 = call ptr @llvm.stacksave.p0()
  %2810 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1717, ptr %2810, align 8
  %2811 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2810, 1
  %2812 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1430, ptr %2812, align 8
  %2813 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2812, 1
  %2814 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2811, ptr %2814, align 8
  %2815 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2813, ptr %2815, align 8
  call void @memrefCopy(i64 4, ptr %2814, ptr %2815)
  call void @llvm.stackrestore.p0(ptr %2809)
  %2816 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 1, i32 16, i32 4, ptr %2568, ptr %1440, ptr %2526, i32 2, i32 603, i32 117)
  %2817 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2512, 3, 0
  %2818 = mul i64 1, %2817
  %2819 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2512, 3, 1
  %2820 = mul i64 %2818, %2819
  %2821 = mul i64 %2820, 4
  %2822 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2512, 1
  %2823 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2512, 2
  %2824 = getelementptr float, ptr %2822, i64 %2823
  %2825 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2786, 1
  %2826 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2786, 2
  %2827 = getelementptr float, ptr %2825, i64 %2826
  call void @llvm.memcpy.p0.p0.i64(ptr %2827, ptr %2824, i64 %2821, i1 false)
  %2828 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %2829 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %2830 = insertvalue { ptr, ptr, i64 } poison, ptr %2828, 0
  %2831 = insertvalue { ptr, ptr, i64 } %2830, ptr %2829, 1
  %2832 = insertvalue { ptr, ptr, i64 } %2831, i64 0, 2
  %2833 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %2834 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %2835 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %2836 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %2837 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %2838 = extractvalue { ptr, ptr, i64 } %2832, 0
  %2839 = extractvalue { ptr, ptr, i64 } %2832, 1
  %2840 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2838, 0
  %2841 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2840, ptr %2839, 1
  %2842 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2841, i64 1076, 2
  %2843 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2842, i64 1, 3, 0
  %2844 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2843, i64 33, 4, 0
  %2845 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2844, i64 4, 3, 1
  %2846 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2845, i64 1, 4, 1
  %2847 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2846, 3, 0
  %2848 = mul i64 1, %2847
  %2849 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2846, 3, 1
  %2850 = mul i64 %2848, %2849
  %2851 = mul i64 %2850, 4
  %2852 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2846, 1
  %2853 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2846, 2
  %2854 = getelementptr float, ptr %2852, i64 %2853
  %2855 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2512, 1
  %2856 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2512, 2
  %2857 = getelementptr float, ptr %2855, i64 %2856
  call void @llvm.memcpy.p0.p0.i64(ptr %2857, ptr %2854, i64 %2851, i1 false)
  %2858 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2545, 3, 0
  %2859 = mul i64 1, %2858
  %2860 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2545, 3, 1
  %2861 = mul i64 %2859, %2860
  %2862 = mul i64 %2861, 4
  %2863 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2545, 1
  %2864 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2545, 2
  %2865 = getelementptr float, ptr %2863, i64 %2864
  %2866 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2554, 1
  %2867 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2554, 2
  %2868 = getelementptr float, ptr %2866, i64 %2867
  call void @llvm.memcpy.p0.p0.i64(ptr %2868, ptr %2865, i64 %2862, i1 false)
  %2869 = call ptr @llvm.stacksave.p0()
  %2870 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1788, ptr %2870, align 8
  %2871 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2870, 1
  %2872 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1430, ptr %2872, align 8
  %2873 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2872, 1
  %2874 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2871, ptr %2874, align 8
  %2875 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2873, ptr %2875, align 8
  call void @memrefCopy(i64 4, ptr %2874, ptr %2875)
  call void @llvm.stackrestore.p0(ptr %2869)
  %2876 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 1, i32 16, i32 4, ptr %2568, ptr %1440, ptr %2526, i32 3, i32 603, i32 117)
  %2877 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2512, 3, 0
  %2878 = mul i64 1, %2877
  %2879 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2512, 3, 1
  %2880 = mul i64 %2878, %2879
  %2881 = mul i64 %2880, 4
  %2882 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2512, 1
  %2883 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2512, 2
  %2884 = getelementptr float, ptr %2882, i64 %2883
  %2885 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2846, 1
  %2886 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2846, 2
  %2887 = getelementptr float, ptr %2885, i64 %2886
  call void @llvm.memcpy.p0.p0.i64(ptr %2887, ptr %2884, i64 %2881, i1 false)
  %2888 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %2889 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %2890 = insertvalue { ptr, ptr, i64 } poison, ptr %2888, 0
  %2891 = insertvalue { ptr, ptr, i64 } %2890, ptr %2889, 1
  %2892 = insertvalue { ptr, ptr, i64 } %2891, i64 0, 2
  %2893 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %2894 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %2895 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %2896 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %2897 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %2898 = extractvalue { ptr, ptr, i64 } %2892, 0
  %2899 = extractvalue { ptr, ptr, i64 } %2892, 1
  %2900 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2898, 0
  %2901 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2900, ptr %2899, 1
  %2902 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2901, i64 1080, 2
  %2903 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2902, i64 1, 3, 0
  %2904 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2903, i64 33, 4, 0
  %2905 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2904, i64 4, 3, 1
  %2906 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2905, i64 1, 4, 1
  %2907 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2906, 3, 0
  %2908 = mul i64 1, %2907
  %2909 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2906, 3, 1
  %2910 = mul i64 %2908, %2909
  %2911 = mul i64 %2910, 4
  %2912 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2906, 1
  %2913 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2906, 2
  %2914 = getelementptr float, ptr %2912, i64 %2913
  %2915 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2512, 1
  %2916 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2512, 2
  %2917 = getelementptr float, ptr %2915, i64 %2916
  call void @llvm.memcpy.p0.p0.i64(ptr %2917, ptr %2914, i64 %2911, i1 false)
  %2918 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2545, 3, 0
  %2919 = mul i64 1, %2918
  %2920 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2545, 3, 1
  %2921 = mul i64 %2919, %2920
  %2922 = mul i64 %2921, 4
  %2923 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2545, 1
  %2924 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2545, 2
  %2925 = getelementptr float, ptr %2923, i64 %2924
  %2926 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2554, 1
  %2927 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2554, 2
  %2928 = getelementptr float, ptr %2926, i64 %2927
  call void @llvm.memcpy.p0.p0.i64(ptr %2928, ptr %2925, i64 %2922, i1 false)
  %2929 = call ptr @llvm.stacksave.p0()
  %2930 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1859, ptr %2930, align 8
  %2931 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2930, 1
  %2932 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1430, ptr %2932, align 8
  %2933 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2932, 1
  %2934 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2931, ptr %2934, align 8
  %2935 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2933, ptr %2935, align 8
  call void @memrefCopy(i64 4, ptr %2934, ptr %2935)
  call void @llvm.stackrestore.p0(ptr %2929)
  %2936 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 1, i32 16, i32 4, ptr %2568, ptr %1440, ptr %2526, i32 1, i32 672, i32 117)
  %2937 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2512, 3, 0
  %2938 = mul i64 1, %2937
  %2939 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2512, 3, 1
  %2940 = mul i64 %2938, %2939
  %2941 = mul i64 %2940, 4
  %2942 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2512, 1
  %2943 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2512, 2
  %2944 = getelementptr float, ptr %2942, i64 %2943
  %2945 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2906, 1
  %2946 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2906, 2
  %2947 = getelementptr float, ptr %2945, i64 %2946
  call void @llvm.memcpy.p0.p0.i64(ptr %2947, ptr %2944, i64 %2941, i1 false)
  %2948 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %2949 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %2950 = insertvalue { ptr, ptr, i64 } poison, ptr %2948, 0
  %2951 = insertvalue { ptr, ptr, i64 } %2950, ptr %2949, 1
  %2952 = insertvalue { ptr, ptr, i64 } %2951, i64 0, 2
  %2953 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %2954 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %2955 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %2956 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %2957 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %2958 = extractvalue { ptr, ptr, i64 } %2952, 0
  %2959 = extractvalue { ptr, ptr, i64 } %2952, 1
  %2960 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2958, 0
  %2961 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2960, ptr %2959, 1
  %2962 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2961, i64 1084, 2
  %2963 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2962, i64 1, 3, 0
  %2964 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2963, i64 33, 4, 0
  %2965 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2964, i64 4, 3, 1
  %2966 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2965, i64 1, 4, 1
  %2967 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2966, 3, 0
  %2968 = mul i64 1, %2967
  %2969 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2966, 3, 1
  %2970 = mul i64 %2968, %2969
  %2971 = mul i64 %2970, 4
  %2972 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2966, 1
  %2973 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2966, 2
  %2974 = getelementptr float, ptr %2972, i64 %2973
  %2975 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2512, 1
  %2976 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2512, 2
  %2977 = getelementptr float, ptr %2975, i64 %2976
  call void @llvm.memcpy.p0.p0.i64(ptr %2977, ptr %2974, i64 %2971, i1 false)
  %2978 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2545, 3, 0
  %2979 = mul i64 1, %2978
  %2980 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2545, 3, 1
  %2981 = mul i64 %2979, %2980
  %2982 = mul i64 %2981, 4
  %2983 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2545, 1
  %2984 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2545, 2
  %2985 = getelementptr float, ptr %2983, i64 %2984
  %2986 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2554, 1
  %2987 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2554, 2
  %2988 = getelementptr float, ptr %2986, i64 %2987
  call void @llvm.memcpy.p0.p0.i64(ptr %2988, ptr %2985, i64 %2982, i1 false)
  %2989 = call ptr @llvm.stacksave.p0()
  %2990 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1930, ptr %2990, align 8
  %2991 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2990, 1
  %2992 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1430, ptr %2992, align 8
  %2993 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2992, 1
  %2994 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2991, ptr %2994, align 8
  %2995 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2993, ptr %2995, align 8
  call void @memrefCopy(i64 4, ptr %2994, ptr %2995)
  call void @llvm.stackrestore.p0(ptr %2989)
  %2996 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 1, i32 16, i32 4, ptr %2568, ptr %1440, ptr %2526, i32 2, i32 672, i32 117)
  %2997 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2512, 3, 0
  %2998 = mul i64 1, %2997
  %2999 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2512, 3, 1
  %3000 = mul i64 %2998, %2999
  %3001 = mul i64 %3000, 4
  %3002 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2512, 1
  %3003 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2512, 2
  %3004 = getelementptr float, ptr %3002, i64 %3003
  %3005 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2966, 1
  %3006 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2966, 2
  %3007 = getelementptr float, ptr %3005, i64 %3006
  call void @llvm.memcpy.p0.p0.i64(ptr %3007, ptr %3004, i64 %3001, i1 false)
  %3008 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %3009 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %3010 = insertvalue { ptr, ptr, i64 } poison, ptr %3008, 0
  %3011 = insertvalue { ptr, ptr, i64 } %3010, ptr %3009, 1
  %3012 = insertvalue { ptr, ptr, i64 } %3011, i64 0, 2
  %3013 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %3014 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %3015 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %3016 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %3017 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %3018 = extractvalue { ptr, ptr, i64 } %3012, 0
  %3019 = extractvalue { ptr, ptr, i64 } %3012, 1
  %3020 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %3018, 0
  %3021 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3020, ptr %3019, 1
  %3022 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3021, i64 1088, 2
  %3023 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3022, i64 1, 3, 0
  %3024 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3023, i64 33, 4, 0
  %3025 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3024, i64 1, 3, 1
  %3026 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3025, i64 1, 4, 1
  %3027 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 0
  %3028 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 1
  %3029 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %3027, 0
  %3030 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3029, ptr %3028, 1
  %3031 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3030, i64 0, 2
  %3032 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3031, i64 1, 3, 0
  %3033 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3032, i64 1, 4, 0
  %3034 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3033, i64 1, 3, 1
  %3035 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3034, i64 1, 4, 1
  %3036 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3026, 3, 0
  %3037 = mul i64 1, %3036
  %3038 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3026, 3, 1
  %3039 = mul i64 %3037, %3038
  %3040 = mul i64 %3039, 4
  %3041 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3026, 1
  %3042 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3026, 2
  %3043 = getelementptr float, ptr %3041, i64 %3042
  %3044 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3035, 1
  %3045 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3035, 2
  %3046 = getelementptr float, ptr %3044, i64 %3045
  call void @llvm.memcpy.p0.p0.i64(ptr %3046, ptr %3043, i64 %3040, i1 false)
  %3047 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 1
  %3048 = ptrtoint ptr %3047 to i64
  %3049 = inttoptr i64 %3048 to ptr
  %3050 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2545, 3, 0
  %3051 = mul i64 1, %3050
  %3052 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2545, 3, 1
  %3053 = mul i64 %3051, %3052
  %3054 = mul i64 %3053, 4
  %3055 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2545, 1
  %3056 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2545, 2
  %3057 = getelementptr float, ptr %3055, i64 %3056
  %3058 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2554, 1
  %3059 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2554, 2
  %3060 = getelementptr float, ptr %3058, i64 %3059
  call void @llvm.memcpy.p0.p0.i64(ptr %3060, ptr %3057, i64 %3054, i1 false)
  %3061 = call ptr @llvm.stacksave.p0()
  %3062 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %498, ptr %3062, align 8
  %3063 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3062, 1
  %3064 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %507, ptr %3064, align 8
  %3065 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3064, 1
  %3066 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3063, ptr %3066, align 8
  %3067 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3065, ptr %3067, align 8
  call void @memrefCopy(i64 4, ptr %3066, ptr %3067)
  call void @llvm.stackrestore.p0(ptr %3061)
  %3068 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 1, i32 16, i32 1, ptr %2568, ptr %517, ptr %3049, i32 3, i32 672, i32 117)
  %3069 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3035, 3, 0
  %3070 = mul i64 1, %3069
  %3071 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3035, 3, 1
  %3072 = mul i64 %3070, %3071
  %3073 = mul i64 %3072, 4
  %3074 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3035, 1
  %3075 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3035, 2
  %3076 = getelementptr float, ptr %3074, i64 %3075
  %3077 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3026, 1
  %3078 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3026, 2
  %3079 = getelementptr float, ptr %3077, i64 %3078
  call void @llvm.memcpy.p0.p0.i64(ptr %3079, ptr %3076, i64 %3073, i1 false)
  %3080 = call ptr @malloc(i64 4356)
  %3081 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %3080, 0
  %3082 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3081, ptr %3080, 1
  %3083 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3082, i64 0, 2
  %3084 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3083, i64 33, 3, 0
  %3085 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3084, i64 33, 3, 1
  %3086 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3085, i64 33, 4, 0
  %3087 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3086, i64 1, 4, 1
  %3088 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %3089 = mul i64 1, %3088
  %3090 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %3091 = mul i64 %3089, %3090
  %3092 = mul i64 %3091, 4
  %3093 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %3094 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %3095 = getelementptr float, ptr %3093, i64 %3094
  %3096 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3087, 1
  %3097 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3087, 2
  %3098 = getelementptr float, ptr %3096, i64 %3097
  call void @llvm.memcpy.p0.p0.i64(ptr %3098, ptr %3095, i64 %3092, i1 false)
  ret { ptr, ptr, i64, [2 x i64], [2 x i64] } %3087
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
