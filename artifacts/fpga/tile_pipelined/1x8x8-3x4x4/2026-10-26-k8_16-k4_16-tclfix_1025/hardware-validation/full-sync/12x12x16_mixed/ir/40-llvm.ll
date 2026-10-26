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
  %77 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %76, i64 12, 4, 0
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
  %157 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %156, i64 12, 4, 0
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
  %202 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %201, i64 4, 3, 0
  %203 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %202, i64 12, 4, 0
  %204 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %203, i64 4, 3, 1
  %205 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %204, i64 1, 4, 1
  %206 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 0
  %207 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 1
  %208 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %206, 0
  %209 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %208, ptr %207, 1
  %210 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %209, i64 0, 2
  %211 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %210, i64 4, 3, 0
  %212 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %211, i64 4, 4, 0
  %213 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %212, i64 4, 3, 1
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
  %239 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %238, i64 0, 2
  %240 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %239, i64 4, 3, 0
  %241 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %240, i64 16, 4, 0
  %242 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %241, i64 16, 3, 1
  %243 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %242, i64 1, 4, 1
  %244 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 0
  %245 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %246 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %244, 0
  %247 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %246, ptr %245, 1
  %248 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %247, i64 0, 2
  %249 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %248, i64 4, 3, 0
  %250 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %249, i64 16, 4, 0
  %251 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %250, i64 16, 3, 1
  %252 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %251, i64 1, 4, 1
  %253 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %243, 3, 0
  %254 = mul i64 1, %253
  %255 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %243, 3, 1
  %256 = mul i64 %254, %255
  %257 = mul i64 %256, 4
  %258 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %243, 1
  %259 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %243, 2
  %260 = getelementptr float, ptr %258, i64 %259
  %261 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %252, 1
  %262 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %252, 2
  %263 = getelementptr float, ptr %261, i64 %262
  call void @llvm.memcpy.p0.p0.i64(ptr %263, ptr %260, i64 %257, i1 false)
  %264 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %265 = ptrtoint ptr %264 to i64
  %266 = inttoptr i64 %265 to ptr
  %267 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %268 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %269 = insertvalue { ptr, ptr, i64 } poison, ptr %267, 0
  %270 = insertvalue { ptr, ptr, i64 } %269, ptr %268, 1
  %271 = insertvalue { ptr, ptr, i64 } %270, i64 0, 2
  %272 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %273 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %274 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %275 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %276 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %277 = extractvalue { ptr, ptr, i64 } %271, 0
  %278 = extractvalue { ptr, ptr, i64 } %271, 1
  %279 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %277, 0
  %280 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %279, ptr %278, 1
  %281 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %280, i64 8, 2
  %282 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %281, i64 16, 3, 0
  %283 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %282, i64 12, 4, 0
  %284 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %283, i64 4, 3, 1
  %285 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %284, i64 1, 4, 1
  %286 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 0
  %287 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %288 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %286, 0
  %289 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %288, ptr %287, 1
  %290 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %289, i64 0, 2
  %291 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %290, i64 16, 3, 0
  %292 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %291, i64 4, 4, 0
  %293 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %292, i64 4, 3, 1
  %294 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %293, i64 1, 4, 1
  %295 = call ptr @llvm.stacksave.p0()
  %296 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %285, ptr %296, align 8
  %297 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %296, 1
  %298 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %294, ptr %298, align 8
  %299 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %298, 1
  %300 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %297, ptr %300, align 8
  %301 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %299, ptr %301, align 8
  call void @memrefCopy(i64 4, ptr %300, ptr %301)
  call void @llvm.stackrestore.p0(ptr %295)
  %302 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %303 = ptrtoint ptr %302 to i64
  %304 = inttoptr i64 %303 to ptr
  %305 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %266, ptr %304, ptr %224, i32 1, i32 0, i32 117)
  %306 = call ptr @llvm.stacksave.p0()
  %307 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %214, ptr %307, align 8
  %308 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %307, 1
  %309 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %205, ptr %309, align 8
  %310 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %309, 1
  %311 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %308, ptr %311, align 8
  %312 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %310, ptr %312, align 8
  call void @memrefCopy(i64 4, ptr %311, ptr %312)
  call void @llvm.stackrestore.p0(ptr %306)
  %313 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %314 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %315 = insertvalue { ptr, ptr, i64 } poison, ptr %313, 0
  %316 = insertvalue { ptr, ptr, i64 } %315, ptr %314, 1
  %317 = insertvalue { ptr, ptr, i64 } %316, i64 0, 2
  %318 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %319 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %320 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %321 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %322 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %323 = extractvalue { ptr, ptr, i64 } %317, 0
  %324 = extractvalue { ptr, ptr, i64 } %317, 1
  %325 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %323, 0
  %326 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %325, ptr %324, 1
  %327 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %326, i64 56, 2
  %328 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %327, i64 4, 3, 0
  %329 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %328, i64 12, 4, 0
  %330 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %329, i64 4, 3, 1
  %331 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %330, i64 1, 4, 1
  %332 = call ptr @llvm.stacksave.p0()
  %333 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %331, ptr %333, align 8
  %334 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %333, 1
  %335 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %214, ptr %335, align 8
  %336 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %335, 1
  %337 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %334, ptr %337, align 8
  %338 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %336, ptr %338, align 8
  call void @memrefCopy(i64 4, ptr %337, ptr %338)
  call void @llvm.stackrestore.p0(ptr %332)
  %339 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %340 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %341 = insertvalue { ptr, ptr, i64 } poison, ptr %339, 0
  %342 = insertvalue { ptr, ptr, i64 } %341, ptr %340, 1
  %343 = insertvalue { ptr, ptr, i64 } %342, i64 0, 2
  %344 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %345 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %346 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %347 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %348 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %349 = extractvalue { ptr, ptr, i64 } %343, 0
  %350 = extractvalue { ptr, ptr, i64 } %343, 1
  %351 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %349, 0
  %352 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %351, ptr %350, 1
  %353 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %352, i64 64, 2
  %354 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %353, i64 4, 3, 0
  %355 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %354, i64 16, 4, 0
  %356 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %355, i64 16, 3, 1
  %357 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %356, i64 1, 4, 1
  %358 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %357, 3, 0
  %359 = mul i64 1, %358
  %360 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %357, 3, 1
  %361 = mul i64 %359, %360
  %362 = mul i64 %361, 4
  %363 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %357, 1
  %364 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %357, 2
  %365 = getelementptr float, ptr %363, i64 %364
  %366 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %252, 1
  %367 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %252, 2
  %368 = getelementptr float, ptr %366, i64 %367
  call void @llvm.memcpy.p0.p0.i64(ptr %368, ptr %365, i64 %362, i1 false)
  %369 = call ptr @llvm.stacksave.p0()
  %370 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %285, ptr %370, align 8
  %371 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %370, 1
  %372 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %294, ptr %372, align 8
  %373 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %372, 1
  %374 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %371, ptr %374, align 8
  %375 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %373, ptr %375, align 8
  call void @memrefCopy(i64 4, ptr %374, ptr %375)
  call void @llvm.stackrestore.p0(ptr %369)
  %376 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %266, ptr %304, ptr %224, i32 2, i32 0, i32 117)
  %377 = call ptr @llvm.stacksave.p0()
  %378 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %214, ptr %378, align 8
  %379 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %378, 1
  %380 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %331, ptr %380, align 8
  %381 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %380, 1
  %382 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %379, ptr %382, align 8
  %383 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %381, ptr %383, align 8
  call void @memrefCopy(i64 4, ptr %382, ptr %383)
  call void @llvm.stackrestore.p0(ptr %377)
  %384 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %385 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %386 = insertvalue { ptr, ptr, i64 } poison, ptr %384, 0
  %387 = insertvalue { ptr, ptr, i64 } %386, ptr %385, 1
  %388 = insertvalue { ptr, ptr, i64 } %387, i64 0, 2
  %389 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %390 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %391 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %392 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %393 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %394 = extractvalue { ptr, ptr, i64 } %388, 0
  %395 = extractvalue { ptr, ptr, i64 } %388, 1
  %396 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %394, 0
  %397 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %396, ptr %395, 1
  %398 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %397, i64 96, 2
  %399 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %398, i64 4, 3, 0
  %400 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %399, i64 12, 4, 0
  %401 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %400, i64 4, 3, 1
  %402 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %401, i64 1, 4, 1
  %403 = call ptr @llvm.stacksave.p0()
  %404 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %402, ptr %404, align 8
  %405 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %404, 1
  %406 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %214, ptr %406, align 8
  %407 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %406, 1
  %408 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %405, ptr %408, align 8
  %409 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %407, ptr %409, align 8
  call void @memrefCopy(i64 4, ptr %408, ptr %409)
  call void @llvm.stackrestore.p0(ptr %403)
  %410 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %411 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %412 = insertvalue { ptr, ptr, i64 } poison, ptr %410, 0
  %413 = insertvalue { ptr, ptr, i64 } %412, ptr %411, 1
  %414 = insertvalue { ptr, ptr, i64 } %413, i64 0, 2
  %415 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %416 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %417 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %418 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %419 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %420 = extractvalue { ptr, ptr, i64 } %414, 0
  %421 = extractvalue { ptr, ptr, i64 } %414, 1
  %422 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %420, 0
  %423 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %422, ptr %421, 1
  %424 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %423, i64 128, 2
  %425 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %424, i64 4, 3, 0
  %426 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %425, i64 16, 4, 0
  %427 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %426, i64 16, 3, 1
  %428 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %427, i64 1, 4, 1
  %429 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %428, 3, 0
  %430 = mul i64 1, %429
  %431 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %428, 3, 1
  %432 = mul i64 %430, %431
  %433 = mul i64 %432, 4
  %434 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %428, 1
  %435 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %428, 2
  %436 = getelementptr float, ptr %434, i64 %435
  %437 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %252, 1
  %438 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %252, 2
  %439 = getelementptr float, ptr %437, i64 %438
  call void @llvm.memcpy.p0.p0.i64(ptr %439, ptr %436, i64 %433, i1 false)
  %440 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %441 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %442 = insertvalue { ptr, ptr, i64 } poison, ptr %440, 0
  %443 = insertvalue { ptr, ptr, i64 } %442, ptr %441, 1
  %444 = insertvalue { ptr, ptr, i64 } %443, i64 0, 2
  %445 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %446 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %447 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %448 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %449 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %450 = extractvalue { ptr, ptr, i64 } %444, 0
  %451 = extractvalue { ptr, ptr, i64 } %444, 1
  %452 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %450, 0
  %453 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %452, ptr %451, 1
  %454 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %453, i64 0, 2
  %455 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %454, i64 16, 3, 0
  %456 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %455, i64 12, 4, 0
  %457 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %456, i64 4, 3, 1
  %458 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %457, i64 1, 4, 1
  %459 = call ptr @llvm.stacksave.p0()
  %460 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %458, ptr %460, align 8
  %461 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %460, 1
  %462 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %294, ptr %462, align 8
  %463 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %462, 1
  %464 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %461, ptr %464, align 8
  %465 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %463, ptr %465, align 8
  call void @memrefCopy(i64 4, ptr %464, ptr %465)
  call void @llvm.stackrestore.p0(ptr %459)
  %466 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %266, ptr %304, ptr %224, i32 3, i32 0, i32 117)
  %467 = call ptr @llvm.stacksave.p0()
  %468 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %214, ptr %468, align 8
  %469 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %468, 1
  %470 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %402, ptr %470, align 8
  %471 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %470, 1
  %472 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %469, ptr %472, align 8
  %473 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %471, ptr %473, align 8
  call void @memrefCopy(i64 4, ptr %472, ptr %473)
  call void @llvm.stackrestore.p0(ptr %467)
  %474 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %475 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %476 = insertvalue { ptr, ptr, i64 } poison, ptr %474, 0
  %477 = insertvalue { ptr, ptr, i64 } %476, ptr %475, 1
  %478 = insertvalue { ptr, ptr, i64 } %477, i64 0, 2
  %479 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %480 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %481 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %482 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %483 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %484 = extractvalue { ptr, ptr, i64 } %478, 0
  %485 = extractvalue { ptr, ptr, i64 } %478, 1
  %486 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %484, 0
  %487 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %486, ptr %485, 1
  %488 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %487, i64 100, 2
  %489 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %488, i64 4, 3, 0
  %490 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %489, i64 12, 4, 0
  %491 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %490, i64 4, 3, 1
  %492 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %491, i64 1, 4, 1
  %493 = call ptr @llvm.stacksave.p0()
  %494 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %492, ptr %494, align 8
  %495 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %494, 1
  %496 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %214, ptr %496, align 8
  %497 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %496, 1
  %498 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %495, ptr %498, align 8
  %499 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %497, ptr %499, align 8
  call void @memrefCopy(i64 4, ptr %498, ptr %499)
  call void @llvm.stackrestore.p0(ptr %493)
  %500 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %428, 3, 0
  %501 = mul i64 1, %500
  %502 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %428, 3, 1
  %503 = mul i64 %501, %502
  %504 = mul i64 %503, 4
  %505 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %428, 1
  %506 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %428, 2
  %507 = getelementptr float, ptr %505, i64 %506
  %508 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %252, 1
  %509 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %252, 2
  %510 = getelementptr float, ptr %508, i64 %509
  call void @llvm.memcpy.p0.p0.i64(ptr %510, ptr %507, i64 %504, i1 false)
  %511 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %512 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %513 = insertvalue { ptr, ptr, i64 } poison, ptr %511, 0
  %514 = insertvalue { ptr, ptr, i64 } %513, ptr %512, 1
  %515 = insertvalue { ptr, ptr, i64 } %514, i64 0, 2
  %516 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %517 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %518 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %519 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %520 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %521 = extractvalue { ptr, ptr, i64 } %515, 0
  %522 = extractvalue { ptr, ptr, i64 } %515, 1
  %523 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %521, 0
  %524 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %523, ptr %522, 1
  %525 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %524, i64 4, 2
  %526 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %525, i64 16, 3, 0
  %527 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %526, i64 12, 4, 0
  %528 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %527, i64 4, 3, 1
  %529 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %528, i64 1, 4, 1
  %530 = call ptr @llvm.stacksave.p0()
  %531 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %529, ptr %531, align 8
  %532 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %531, 1
  %533 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %294, ptr %533, align 8
  %534 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %533, 1
  %535 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %532, ptr %535, align 8
  %536 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %534, ptr %536, align 8
  call void @memrefCopy(i64 4, ptr %535, ptr %536)
  call void @llvm.stackrestore.p0(ptr %530)
  %537 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %266, ptr %304, ptr %224, i32 1, i32 42, i32 117)
  %538 = call ptr @llvm.stacksave.p0()
  %539 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %214, ptr %539, align 8
  %540 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %539, 1
  %541 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %492, ptr %541, align 8
  %542 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %541, 1
  %543 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %540, ptr %543, align 8
  %544 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %542, ptr %544, align 8
  call void @memrefCopy(i64 4, ptr %543, ptr %544)
  call void @llvm.stackrestore.p0(ptr %538)
  %545 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %546 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %547 = insertvalue { ptr, ptr, i64 } poison, ptr %545, 0
  %548 = insertvalue { ptr, ptr, i64 } %547, ptr %546, 1
  %549 = insertvalue { ptr, ptr, i64 } %548, i64 0, 2
  %550 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %551 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %552 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %553 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %554 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %555 = extractvalue { ptr, ptr, i64 } %549, 0
  %556 = extractvalue { ptr, ptr, i64 } %549, 1
  %557 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %555, 0
  %558 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %557, ptr %556, 1
  %559 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %558, i64 104, 2
  %560 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %559, i64 4, 3, 0
  %561 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %560, i64 12, 4, 0
  %562 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %561, i64 4, 3, 1
  %563 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %562, i64 1, 4, 1
  %564 = call ptr @llvm.stacksave.p0()
  %565 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %563, ptr %565, align 8
  %566 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %565, 1
  %567 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %214, ptr %567, align 8
  %568 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %567, 1
  %569 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %566, ptr %569, align 8
  %570 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %568, ptr %570, align 8
  call void @memrefCopy(i64 4, ptr %569, ptr %570)
  call void @llvm.stackrestore.p0(ptr %564)
  %571 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %428, 3, 0
  %572 = mul i64 1, %571
  %573 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %428, 3, 1
  %574 = mul i64 %572, %573
  %575 = mul i64 %574, 4
  %576 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %428, 1
  %577 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %428, 2
  %578 = getelementptr float, ptr %576, i64 %577
  %579 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %252, 1
  %580 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %252, 2
  %581 = getelementptr float, ptr %579, i64 %580
  call void @llvm.memcpy.p0.p0.i64(ptr %581, ptr %578, i64 %575, i1 false)
  %582 = call ptr @llvm.stacksave.p0()
  %583 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %285, ptr %583, align 8
  %584 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %583, 1
  %585 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %294, ptr %585, align 8
  %586 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %585, 1
  %587 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %584, ptr %587, align 8
  %588 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %586, ptr %588, align 8
  call void @memrefCopy(i64 4, ptr %587, ptr %588)
  call void @llvm.stackrestore.p0(ptr %582)
  %589 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %266, ptr %304, ptr %224, i32 2, i32 42, i32 117)
  %590 = call ptr @llvm.stacksave.p0()
  %591 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %214, ptr %591, align 8
  %592 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %591, 1
  %593 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %563, ptr %593, align 8
  %594 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %593, 1
  %595 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %592, ptr %595, align 8
  %596 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %594, ptr %596, align 8
  call void @memrefCopy(i64 4, ptr %595, ptr %596)
  call void @llvm.stackrestore.p0(ptr %590)
  %597 = call ptr @malloc(i64 576)
  %598 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %597, 0
  %599 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %598, ptr %597, 1
  %600 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %599, i64 0, 2
  %601 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %600, i64 12, 3, 0
  %602 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %601, i64 12, 3, 1
  %603 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %602, i64 12, 4, 0
  %604 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %603, i64 1, 4, 1
  %605 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %606 = mul i64 1, %605
  %607 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %608 = mul i64 %606, %607
  %609 = mul i64 %608, 4
  %610 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %611 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %612 = getelementptr float, ptr %610, i64 %611
  %613 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %604, 1
  %614 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %604, 2
  %615 = getelementptr float, ptr %613, i64 %614
  call void @llvm.memcpy.p0.p0.i64(ptr %615, ptr %612, i64 %609, i1 false)
  ret { ptr, ptr, i64, [2 x i64], [2 x i64] } %604
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
