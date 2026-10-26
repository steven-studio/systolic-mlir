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
  %77 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %76, i64 15, 4, 0
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
  %157 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %156, i64 15, 4, 0
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
  %203 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %202, i64 15, 4, 0
  %204 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %203, i64 7, 3, 1
  %205 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %204, i64 1, 4, 1
  %206 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 0
  %207 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 1
  %208 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %206, 0
  %209 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %208, ptr %207, 1
  %210 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %209, i64 0, 2
  %211 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %210, i64 8, 3, 0
  %212 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %211, i64 7, 4, 0
  %213 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %212, i64 7, 3, 1
  %214 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %213, i64 1, 4, 1
  %215 = call ptr @llvm.stacksave.p0()
  %216 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %205, ptr %216, align 8
  %217 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %216, 1
  %218 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %214, ptr %218, align 8
  %219 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %218, 1
  %220 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %217, ptr %220, align 8
  %221 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %219, ptr %221, align 8
  call void @memrefCopy(i64 4, ptr %220, ptr %221)
  call void @llvm.stackrestore.p0(ptr %215)
  %222 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 1
  %223 = ptrtoint ptr %222 to i64
  %224 = inttoptr i64 %223 to ptr
  %225 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %117, 3, 0
  %226 = mul i64 1, %225
  %227 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %117, 3, 1
  %228 = mul i64 %226, %227
  %229 = mul i64 %228, 4
  %230 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %117, 1
  %231 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %117, 2
  %232 = getelementptr float, ptr %230, i64 %231
  %233 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, 1
  %234 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, 2
  %235 = getelementptr float, ptr %233, i64 %234
  call void @llvm.memcpy.p0.p0.i64(ptr %235, ptr %232, i64 %229, i1 false)
  %236 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %237 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %238 = insertvalue { ptr, ptr, i64 } poison, ptr %236, 0
  %239 = insertvalue { ptr, ptr, i64 } %238, ptr %237, 1
  %240 = insertvalue { ptr, ptr, i64 } %239, i64 0, 2
  %241 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %242 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %243 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %244 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %245 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %246 = extractvalue { ptr, ptr, i64 } %240, 0
  %247 = extractvalue { ptr, ptr, i64 } %240, 1
  %248 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %246, 0
  %249 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %248, ptr %247, 1
  %250 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %249, i64 8, 2
  %251 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %250, i64 16, 3, 0
  %252 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %251, i64 15, 4, 0
  %253 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %252, i64 7, 3, 1
  %254 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %253, i64 1, 4, 1
  %255 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 0
  %256 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %257 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %255, 0
  %258 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %257, ptr %256, 1
  %259 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %258, i64 0, 2
  %260 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %259, i64 16, 3, 0
  %261 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %260, i64 7, 4, 0
  %262 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %261, i64 7, 3, 1
  %263 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %262, i64 1, 4, 1
  %264 = call ptr @llvm.stacksave.p0()
  %265 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %254, ptr %265, align 8
  %266 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %265, 1
  %267 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %263, ptr %267, align 8
  %268 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %267, 1
  %269 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %266, ptr %269, align 8
  %270 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %268, ptr %270, align 8
  call void @memrefCopy(i64 4, ptr %269, ptr %270)
  call void @llvm.stackrestore.p0(ptr %264)
  %271 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %272 = ptrtoint ptr %271 to i64
  %273 = inttoptr i64 %272 to ptr
  %274 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 7, ptr %140, ptr %273, ptr %224, i32 0, i32 42, i32 125)
  %275 = call ptr @llvm.stacksave.p0()
  %276 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %214, ptr %276, align 8
  %277 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %276, 1
  %278 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %205, ptr %278, align 8
  %279 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %278, 1
  %280 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %277, ptr %280, align 8
  %281 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %279, ptr %281, align 8
  call void @memrefCopy(i64 4, ptr %280, ptr %281)
  call void @llvm.stackrestore.p0(ptr %275)
  %282 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %283 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %284 = insertvalue { ptr, ptr, i64 } poison, ptr %282, 0
  %285 = insertvalue { ptr, ptr, i64 } %284, ptr %283, 1
  %286 = insertvalue { ptr, ptr, i64 } %285, i64 0, 2
  %287 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %288 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %289 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %290 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %291 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %292 = extractvalue { ptr, ptr, i64 } %286, 0
  %293 = extractvalue { ptr, ptr, i64 } %286, 1
  %294 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %292, 0
  %295 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %294, ptr %293, 1
  %296 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %295, i64 120, 2
  %297 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %296, i64 7, 3, 0
  %298 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %297, i64 15, 4, 0
  %299 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %298, i64 8, 3, 1
  %300 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %299, i64 1, 4, 1
  %301 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 0
  %302 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 1
  %303 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %301, 0
  %304 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %303, ptr %302, 1
  %305 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %304, i64 0, 2
  %306 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %305, i64 7, 3, 0
  %307 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %306, i64 8, 4, 0
  %308 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %307, i64 8, 3, 1
  %309 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %308, i64 1, 4, 1
  %310 = call ptr @llvm.stacksave.p0()
  %311 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %300, ptr %311, align 8
  %312 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %311, 1
  %313 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %309, ptr %313, align 8
  %314 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %313, 1
  %315 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %312, ptr %315, align 8
  %316 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %314, ptr %316, align 8
  call void @memrefCopy(i64 4, ptr %315, ptr %316)
  call void @llvm.stackrestore.p0(ptr %310)
  %317 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 1
  %318 = ptrtoint ptr %317 to i64
  %319 = inttoptr i64 %318 to ptr
  %320 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %321 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %322 = insertvalue { ptr, ptr, i64 } poison, ptr %320, 0
  %323 = insertvalue { ptr, ptr, i64 } %322, ptr %321, 1
  %324 = insertvalue { ptr, ptr, i64 } %323, i64 0, 2
  %325 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %326 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %327 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %328 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %329 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %330 = extractvalue { ptr, ptr, i64 } %324, 0
  %331 = extractvalue { ptr, ptr, i64 } %324, 1
  %332 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %330, 0
  %333 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %332, ptr %331, 1
  %334 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %333, i64 128, 2
  %335 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %334, i64 7, 3, 0
  %336 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %335, i64 16, 4, 0
  %337 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %336, i64 16, 3, 1
  %338 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %337, i64 1, 4, 1
  %339 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 0
  %340 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %341 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %339, 0
  %342 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %341, ptr %340, 1
  %343 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %342, i64 0, 2
  %344 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %343, i64 7, 3, 0
  %345 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %344, i64 16, 4, 0
  %346 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %345, i64 16, 3, 1
  %347 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %346, i64 1, 4, 1
  %348 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %338, 3, 0
  %349 = mul i64 1, %348
  %350 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %338, 3, 1
  %351 = mul i64 %349, %350
  %352 = mul i64 %351, 4
  %353 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %338, 1
  %354 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %338, 2
  %355 = getelementptr float, ptr %353, i64 %354
  %356 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %347, 1
  %357 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %347, 2
  %358 = getelementptr float, ptr %356, i64 %357
  call void @llvm.memcpy.p0.p0.i64(ptr %358, ptr %355, i64 %352, i1 false)
  %359 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %360 = ptrtoint ptr %359 to i64
  %361 = inttoptr i64 %360 to ptr
  %362 = call ptr @llvm.stacksave.p0()
  %363 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %159, ptr %363, align 8
  %364 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %363, 1
  %365 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %168, ptr %365, align 8
  %366 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %365, 1
  %367 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %364, ptr %367, align 8
  %368 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %366, ptr %368, align 8
  call void @memrefCopy(i64 4, ptr %367, ptr %368)
  call void @llvm.stackrestore.p0(ptr %362)
  %369 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 7, i32 16, i32 8, ptr %361, ptr %178, ptr %319, i32 0, i32 128, i32 125)
  %370 = call ptr @llvm.stacksave.p0()
  %371 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %309, ptr %371, align 8
  %372 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %371, 1
  %373 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %300, ptr %373, align 8
  %374 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %373, 1
  %375 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %372, ptr %375, align 8
  %376 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %374, ptr %376, align 8
  call void @memrefCopy(i64 4, ptr %375, ptr %376)
  call void @llvm.stackrestore.p0(ptr %370)
  %377 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %378 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %379 = insertvalue { ptr, ptr, i64 } poison, ptr %377, 0
  %380 = insertvalue { ptr, ptr, i64 } %379, ptr %378, 1
  %381 = insertvalue { ptr, ptr, i64 } %380, i64 0, 2
  %382 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %383 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %384 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %385 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %386 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %387 = extractvalue { ptr, ptr, i64 } %381, 0
  %388 = extractvalue { ptr, ptr, i64 } %381, 1
  %389 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %387, 0
  %390 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %389, ptr %388, 1
  %391 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %390, i64 128, 2
  %392 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %391, i64 4, 3, 0
  %393 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %392, i64 15, 4, 0
  %394 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %393, i64 4, 3, 1
  %395 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %394, i64 1, 4, 1
  %396 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 0
  %397 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 1
  %398 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %396, 0
  %399 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %398, ptr %397, 1
  %400 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %399, i64 0, 2
  %401 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %400, i64 4, 3, 0
  %402 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %401, i64 4, 4, 0
  %403 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %402, i64 4, 3, 1
  %404 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %403, i64 1, 4, 1
  %405 = call ptr @llvm.stacksave.p0()
  %406 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %395, ptr %406, align 8
  %407 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %406, 1
  %408 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %404, ptr %408, align 8
  %409 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %408, 1
  %410 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %407, ptr %410, align 8
  %411 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %409, ptr %411, align 8
  call void @memrefCopy(i64 4, ptr %410, ptr %411)
  call void @llvm.stackrestore.p0(ptr %405)
  %412 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 1
  %413 = ptrtoint ptr %412 to i64
  %414 = inttoptr i64 %413 to ptr
  %415 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %416 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %417 = insertvalue { ptr, ptr, i64 } poison, ptr %415, 0
  %418 = insertvalue { ptr, ptr, i64 } %417, ptr %416, 1
  %419 = insertvalue { ptr, ptr, i64 } %418, i64 0, 2
  %420 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %421 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %422 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %423 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %424 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %425 = extractvalue { ptr, ptr, i64 } %419, 0
  %426 = extractvalue { ptr, ptr, i64 } %419, 1
  %427 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %425, 0
  %428 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %427, ptr %426, 1
  %429 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %428, i64 128, 2
  %430 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %429, i64 4, 3, 0
  %431 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %430, i64 16, 4, 0
  %432 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %431, i64 16, 3, 1
  %433 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %432, i64 1, 4, 1
  %434 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 0
  %435 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %436 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %434, 0
  %437 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %436, ptr %435, 1
  %438 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %437, i64 0, 2
  %439 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %438, i64 4, 3, 0
  %440 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %439, i64 16, 4, 0
  %441 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %440, i64 16, 3, 1
  %442 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %441, i64 1, 4, 1
  %443 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %433, 3, 0
  %444 = mul i64 1, %443
  %445 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %433, 3, 1
  %446 = mul i64 %444, %445
  %447 = mul i64 %446, 4
  %448 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %433, 1
  %449 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %433, 2
  %450 = getelementptr float, ptr %448, i64 %449
  %451 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %442, 1
  %452 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %442, 2
  %453 = getelementptr float, ptr %451, i64 %452
  call void @llvm.memcpy.p0.p0.i64(ptr %453, ptr %450, i64 %447, i1 false)
  %454 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %455 = ptrtoint ptr %454 to i64
  %456 = inttoptr i64 %455 to ptr
  %457 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %458 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %459 = insertvalue { ptr, ptr, i64 } poison, ptr %457, 0
  %460 = insertvalue { ptr, ptr, i64 } %459, ptr %458, 1
  %461 = insertvalue { ptr, ptr, i64 } %460, i64 0, 2
  %462 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %463 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %464 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %465 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %466 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %467 = extractvalue { ptr, ptr, i64 } %461, 0
  %468 = extractvalue { ptr, ptr, i64 } %461, 1
  %469 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %467, 0
  %470 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %469, ptr %468, 1
  %471 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %470, i64 8, 2
  %472 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %471, i64 16, 3, 0
  %473 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %472, i64 15, 4, 0
  %474 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %473, i64 4, 3, 1
  %475 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %474, i64 1, 4, 1
  %476 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 0
  %477 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %478 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %476, 0
  %479 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %478, ptr %477, 1
  %480 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %479, i64 0, 2
  %481 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %480, i64 16, 3, 0
  %482 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %481, i64 4, 4, 0
  %483 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %482, i64 4, 3, 1
  %484 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %483, i64 1, 4, 1
  %485 = call ptr @llvm.stacksave.p0()
  %486 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %475, ptr %486, align 8
  %487 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %486, 1
  %488 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %484, ptr %488, align 8
  %489 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %488, 1
  %490 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %487, ptr %490, align 8
  %491 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %489, ptr %491, align 8
  call void @memrefCopy(i64 4, ptr %490, ptr %491)
  call void @llvm.stackrestore.p0(ptr %485)
  %492 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %493 = ptrtoint ptr %492 to i64
  %494 = inttoptr i64 %493 to ptr
  %495 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %456, ptr %494, ptr %414, i32 1, i32 0, i32 117)
  %496 = call ptr @llvm.stacksave.p0()
  %497 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %404, ptr %497, align 8
  %498 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %497, 1
  %499 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %395, ptr %499, align 8
  %500 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %499, 1
  %501 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %498, ptr %501, align 8
  %502 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %500, ptr %502, align 8
  call void @memrefCopy(i64 4, ptr %501, ptr %502)
  call void @llvm.stackrestore.p0(ptr %496)
  %503 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %504 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %505 = insertvalue { ptr, ptr, i64 } poison, ptr %503, 0
  %506 = insertvalue { ptr, ptr, i64 } %505, ptr %504, 1
  %507 = insertvalue { ptr, ptr, i64 } %506, i64 0, 2
  %508 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %509 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %510 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %511 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %512 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %513 = extractvalue { ptr, ptr, i64 } %507, 0
  %514 = extractvalue { ptr, ptr, i64 } %507, 1
  %515 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %513, 0
  %516 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %515, ptr %514, 1
  %517 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %516, i64 132, 2
  %518 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %517, i64 4, 3, 0
  %519 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %518, i64 15, 4, 0
  %520 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %519, i64 3, 3, 1
  %521 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %520, i64 1, 4, 1
  %522 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 0
  %523 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 1
  %524 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %522, 0
  %525 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %524, ptr %523, 1
  %526 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %525, i64 0, 2
  %527 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %526, i64 4, 3, 0
  %528 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %527, i64 3, 4, 0
  %529 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %528, i64 3, 3, 1
  %530 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %529, i64 1, 4, 1
  %531 = call ptr @llvm.stacksave.p0()
  %532 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %521, ptr %532, align 8
  %533 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %532, 1
  %534 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %530, ptr %534, align 8
  %535 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %534, 1
  %536 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %533, ptr %536, align 8
  %537 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %535, ptr %537, align 8
  call void @memrefCopy(i64 4, ptr %536, ptr %537)
  call void @llvm.stackrestore.p0(ptr %531)
  %538 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 1
  %539 = ptrtoint ptr %538 to i64
  %540 = inttoptr i64 %539 to ptr
  %541 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %433, 3, 0
  %542 = mul i64 1, %541
  %543 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %433, 3, 1
  %544 = mul i64 %542, %543
  %545 = mul i64 %544, 4
  %546 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %433, 1
  %547 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %433, 2
  %548 = getelementptr float, ptr %546, i64 %547
  %549 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %442, 1
  %550 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %442, 2
  %551 = getelementptr float, ptr %549, i64 %550
  call void @llvm.memcpy.p0.p0.i64(ptr %551, ptr %548, i64 %545, i1 false)
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
  %566 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %565, i64 12, 2
  %567 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %566, i64 16, 3, 0
  %568 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %567, i64 15, 4, 0
  %569 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %568, i64 3, 3, 1
  %570 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %569, i64 1, 4, 1
  %571 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 0
  %572 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %573 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %571, 0
  %574 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %573, ptr %572, 1
  %575 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %574, i64 0, 2
  %576 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %575, i64 16, 3, 0
  %577 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %576, i64 3, 4, 0
  %578 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %577, i64 3, 3, 1
  %579 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %578, i64 1, 4, 1
  %580 = call ptr @llvm.stacksave.p0()
  %581 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %570, ptr %581, align 8
  %582 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %581, 1
  %583 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %579, ptr %583, align 8
  %584 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %583, 1
  %585 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %582, ptr %585, align 8
  %586 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %584, ptr %586, align 8
  call void @memrefCopy(i64 4, ptr %585, ptr %586)
  call void @llvm.stackrestore.p0(ptr %580)
  %587 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %588 = ptrtoint ptr %587 to i64
  %589 = inttoptr i64 %588 to ptr
  %590 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 3, ptr %456, ptr %589, ptr %540, i32 2, i32 0, i32 117)
  %591 = call ptr @llvm.stacksave.p0()
  %592 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %530, ptr %592, align 8
  %593 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %592, 1
  %594 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %521, ptr %594, align 8
  %595 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %594, 1
  %596 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %593, ptr %596, align 8
  %597 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %595, ptr %597, align 8
  call void @memrefCopy(i64 4, ptr %596, ptr %597)
  call void @llvm.stackrestore.p0(ptr %591)
  %598 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %599 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %600 = insertvalue { ptr, ptr, i64 } poison, ptr %598, 0
  %601 = insertvalue { ptr, ptr, i64 } %600, ptr %599, 1
  %602 = insertvalue { ptr, ptr, i64 } %601, i64 0, 2
  %603 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %604 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %605 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %606 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %607 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %608 = extractvalue { ptr, ptr, i64 } %602, 0
  %609 = extractvalue { ptr, ptr, i64 } %602, 1
  %610 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %608, 0
  %611 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %610, ptr %609, 1
  %612 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %611, i64 188, 2
  %613 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %612, i64 3, 3, 0
  %614 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %613, i64 15, 4, 0
  %615 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %614, i64 4, 3, 1
  %616 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %615, i64 1, 4, 1
  %617 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 0
  %618 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 1
  %619 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %617, 0
  %620 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %619, ptr %618, 1
  %621 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %620, i64 0, 2
  %622 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %621, i64 3, 3, 0
  %623 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %622, i64 4, 4, 0
  %624 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %623, i64 4, 3, 1
  %625 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %624, i64 1, 4, 1
  %626 = call ptr @llvm.stacksave.p0()
  %627 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %616, ptr %627, align 8
  %628 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %627, 1
  %629 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %625, ptr %629, align 8
  %630 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %629, 1
  %631 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %628, ptr %631, align 8
  %632 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %630, ptr %632, align 8
  call void @memrefCopy(i64 4, ptr %631, ptr %632)
  call void @llvm.stackrestore.p0(ptr %626)
  %633 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 1
  %634 = ptrtoint ptr %633 to i64
  %635 = inttoptr i64 %634 to ptr
  %636 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %637 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %638 = insertvalue { ptr, ptr, i64 } poison, ptr %636, 0
  %639 = insertvalue { ptr, ptr, i64 } %638, ptr %637, 1
  %640 = insertvalue { ptr, ptr, i64 } %639, i64 0, 2
  %641 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %642 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %643 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %644 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %645 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %646 = extractvalue { ptr, ptr, i64 } %640, 0
  %647 = extractvalue { ptr, ptr, i64 } %640, 1
  %648 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %646, 0
  %649 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %648, ptr %647, 1
  %650 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %649, i64 192, 2
  %651 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %650, i64 3, 3, 0
  %652 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %651, i64 16, 4, 0
  %653 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %652, i64 16, 3, 1
  %654 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %653, i64 1, 4, 1
  %655 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 0
  %656 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %657 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %655, 0
  %658 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %657, ptr %656, 1
  %659 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %658, i64 0, 2
  %660 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %659, i64 3, 3, 0
  %661 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %660, i64 16, 4, 0
  %662 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %661, i64 16, 3, 1
  %663 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %662, i64 1, 4, 1
  %664 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %654, 3, 0
  %665 = mul i64 1, %664
  %666 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %654, 3, 1
  %667 = mul i64 %665, %666
  %668 = mul i64 %667, 4
  %669 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %654, 1
  %670 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %654, 2
  %671 = getelementptr float, ptr %669, i64 %670
  %672 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %663, 1
  %673 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %663, 2
  %674 = getelementptr float, ptr %672, i64 %673
  call void @llvm.memcpy.p0.p0.i64(ptr %674, ptr %671, i64 %668, i1 false)
  %675 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %676 = ptrtoint ptr %675 to i64
  %677 = inttoptr i64 %676 to ptr
  %678 = call ptr @llvm.stacksave.p0()
  %679 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %475, ptr %679, align 8
  %680 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %679, 1
  %681 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %484, ptr %681, align 8
  %682 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %681, 1
  %683 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %680, ptr %683, align 8
  %684 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %682, ptr %684, align 8
  call void @memrefCopy(i64 4, ptr %683, ptr %684)
  call void @llvm.stackrestore.p0(ptr %678)
  %685 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 3, i32 16, i32 4, ptr %677, ptr %494, ptr %635, i32 3, i32 0, i32 117)
  %686 = call ptr @llvm.stacksave.p0()
  %687 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %625, ptr %687, align 8
  %688 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %687, 1
  %689 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %616, ptr %689, align 8
  %690 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %689, 1
  %691 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %688, ptr %691, align 8
  %692 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %690, ptr %692, align 8
  call void @memrefCopy(i64 4, ptr %691, ptr %692)
  call void @llvm.stackrestore.p0(ptr %686)
  %693 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %694 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %695 = insertvalue { ptr, ptr, i64 } poison, ptr %693, 0
  %696 = insertvalue { ptr, ptr, i64 } %695, ptr %694, 1
  %697 = insertvalue { ptr, ptr, i64 } %696, i64 0, 2
  %698 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %699 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %700 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %701 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %702 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %703 = extractvalue { ptr, ptr, i64 } %697, 0
  %704 = extractvalue { ptr, ptr, i64 } %697, 1
  %705 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %703, 0
  %706 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %705, ptr %704, 1
  %707 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %706, i64 192, 2
  %708 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %707, i64 3, 3, 0
  %709 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %708, i64 15, 4, 0
  %710 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %709, i64 3, 3, 1
  %711 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %710, i64 1, 4, 1
  %712 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 0
  %713 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 1
  %714 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %712, 0
  %715 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %714, ptr %713, 1
  %716 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %715, i64 0, 2
  %717 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %716, i64 3, 3, 0
  %718 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %717, i64 3, 4, 0
  %719 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %718, i64 3, 3, 1
  %720 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %719, i64 1, 4, 1
  %721 = call ptr @llvm.stacksave.p0()
  %722 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %711, ptr %722, align 8
  %723 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %722, 1
  %724 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %720, ptr %724, align 8
  %725 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %724, 1
  %726 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %723, ptr %726, align 8
  %727 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %725, ptr %727, align 8
  call void @memrefCopy(i64 4, ptr %726, ptr %727)
  call void @llvm.stackrestore.p0(ptr %721)
  %728 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 1
  %729 = ptrtoint ptr %728 to i64
  %730 = inttoptr i64 %729 to ptr
  %731 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %654, 3, 0
  %732 = mul i64 1, %731
  %733 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %654, 3, 1
  %734 = mul i64 %732, %733
  %735 = mul i64 %734, 4
  %736 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %654, 1
  %737 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %654, 2
  %738 = getelementptr float, ptr %736, i64 %737
  %739 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %663, 1
  %740 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %663, 2
  %741 = getelementptr float, ptr %739, i64 %740
  call void @llvm.memcpy.p0.p0.i64(ptr %741, ptr %738, i64 %735, i1 false)
  %742 = call ptr @llvm.stacksave.p0()
  %743 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %570, ptr %743, align 8
  %744 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %743, 1
  %745 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %579, ptr %745, align 8
  %746 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %745, 1
  %747 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %744, ptr %747, align 8
  %748 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %746, ptr %748, align 8
  call void @memrefCopy(i64 4, ptr %747, ptr %748)
  call void @llvm.stackrestore.p0(ptr %742)
  %749 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 3, i32 16, i32 3, ptr %677, ptr %589, ptr %730, i32 1, i32 42, i32 117)
  %750 = call ptr @llvm.stacksave.p0()
  %751 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %720, ptr %751, align 8
  %752 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %751, 1
  %753 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %711, ptr %753, align 8
  %754 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %753, 1
  %755 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %752, ptr %755, align 8
  %756 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %754, ptr %756, align 8
  call void @memrefCopy(i64 4, ptr %755, ptr %756)
  call void @llvm.stackrestore.p0(ptr %750)
  %757 = call ptr @malloc(i64 900)
  %758 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %757, 0
  %759 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %758, ptr %757, 1
  %760 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %759, i64 0, 2
  %761 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %760, i64 15, 3, 0
  %762 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %761, i64 15, 3, 1
  %763 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %762, i64 15, 4, 0
  %764 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %763, i64 1, 4, 1
  %765 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %766 = mul i64 1, %765
  %767 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %768 = mul i64 %766, %767
  %769 = mul i64 %768, 4
  %770 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %771 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %772 = getelementptr float, ptr %770, i64 %771
  %773 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %764, 1
  %774 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %764, 2
  %775 = getelementptr float, ptr %773, i64 %774
  call void @llvm.memcpy.p0.p0.i64(ptr %775, ptr %772, i64 %769, i1 false)
  ret { ptr, ptr, i64, [2 x i64], [2 x i64] } %764
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
