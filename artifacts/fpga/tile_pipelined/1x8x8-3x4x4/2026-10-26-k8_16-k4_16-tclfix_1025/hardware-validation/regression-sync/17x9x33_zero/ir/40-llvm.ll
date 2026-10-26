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
  %77 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %76, i64 9, 4, 0
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
  %115 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %114, i64 33, 4, 0
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
  %127 = call ptr @llvm.stacksave.p0()
  %128 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %117, ptr %128, align 8
  %129 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %128, 1
  %130 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %130, align 8
  %131 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %130, 1
  %132 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %129, ptr %132, align 8
  %133 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %131, ptr %133, align 8
  call void @memrefCopy(i64 4, ptr %132, ptr %133)
  call void @llvm.stackrestore.p0(ptr %127)
  %134 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %135 = ptrtoint ptr %134 to i64
  %136 = inttoptr i64 %135 to ptr
  %137 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %138 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %139 = insertvalue { ptr, ptr, i64 } poison, ptr %137, 0
  %140 = insertvalue { ptr, ptr, i64 } %139, ptr %138, 1
  %141 = insertvalue { ptr, ptr, i64 } %140, i64 0, 2
  %142 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %143 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %144 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %145 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %146 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %147 = extractvalue { ptr, ptr, i64 } %141, 0
  %148 = extractvalue { ptr, ptr, i64 } %141, 1
  %149 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %147, 0
  %150 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %149, ptr %148, 1
  %151 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %150, i64 0, 2
  %152 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %151, i64 16, 3, 0
  %153 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %152, i64 9, 4, 0
  %154 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %153, i64 8, 3, 1
  %155 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %154, i64 1, 4, 1
  %156 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 0
  %157 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %158 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %156, 0
  %159 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %158, ptr %157, 1
  %160 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %159, i64 0, 2
  %161 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %160, i64 16, 3, 0
  %162 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %161, i64 8, 4, 0
  %163 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %162, i64 8, 3, 1
  %164 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %163, i64 1, 4, 1
  %165 = call ptr @llvm.stacksave.p0()
  %166 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %155, ptr %166, align 8
  %167 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %166, 1
  %168 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %168, align 8
  %169 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %168, 1
  %170 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %167, ptr %170, align 8
  %171 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %169, ptr %171, align 8
  call void @memrefCopy(i64 4, ptr %170, ptr %171)
  call void @llvm.stackrestore.p0(ptr %165)
  %172 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %173 = ptrtoint ptr %172 to i64
  %174 = inttoptr i64 %173 to ptr
  %175 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 0, i32 125)
  %176 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %177 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %178 = insertvalue { ptr, ptr, i64 } poison, ptr %176, 0
  %179 = insertvalue { ptr, ptr, i64 } %178, ptr %177, 1
  %180 = insertvalue { ptr, ptr, i64 } %179, i64 0, 2
  %181 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %182 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %183 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %184 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %185 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %186 = extractvalue { ptr, ptr, i64 } %180, 0
  %187 = extractvalue { ptr, ptr, i64 } %180, 1
  %188 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %186, 0
  %189 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %188, ptr %187, 1
  %190 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %189, i64 16, 2
  %191 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %190, i64 8, 3, 0
  %192 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %191, i64 33, 4, 0
  %193 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %192, i64 16, 3, 1
  %194 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %193, i64 1, 4, 1
  %195 = call ptr @llvm.stacksave.p0()
  %196 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %194, ptr %196, align 8
  %197 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %196, 1
  %198 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %198, align 8
  %199 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %198, 1
  %200 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %197, ptr %200, align 8
  %201 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %199, ptr %201, align 8
  call void @memrefCopy(i64 4, ptr %200, ptr %201)
  call void @llvm.stackrestore.p0(ptr %195)
  %202 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %203 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %204 = insertvalue { ptr, ptr, i64 } poison, ptr %202, 0
  %205 = insertvalue { ptr, ptr, i64 } %204, ptr %203, 1
  %206 = insertvalue { ptr, ptr, i64 } %205, i64 0, 2
  %207 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %208 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %209 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %210 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %211 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %212 = extractvalue { ptr, ptr, i64 } %206, 0
  %213 = extractvalue { ptr, ptr, i64 } %206, 1
  %214 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %212, 0
  %215 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %214, ptr %213, 1
  %216 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %215, i64 144, 2
  %217 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %216, i64 16, 3, 0
  %218 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %217, i64 9, 4, 0
  %219 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %218, i64 8, 3, 1
  %220 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %219, i64 1, 4, 1
  %221 = call ptr @llvm.stacksave.p0()
  %222 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %220, ptr %222, align 8
  %223 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %222, 1
  %224 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %224, align 8
  %225 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %224, 1
  %226 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %223, ptr %226, align 8
  %227 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %225, ptr %227, align 8
  call void @memrefCopy(i64 4, ptr %226, ptr %227)
  call void @llvm.stackrestore.p0(ptr %221)
  %228 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 42, i32 125)
  %229 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %230 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %231 = insertvalue { ptr, ptr, i64 } poison, ptr %229, 0
  %232 = insertvalue { ptr, ptr, i64 } %231, ptr %230, 1
  %233 = insertvalue { ptr, ptr, i64 } %232, i64 0, 2
  %234 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %235 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %236 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %237 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %238 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %239 = extractvalue { ptr, ptr, i64 } %233, 0
  %240 = extractvalue { ptr, ptr, i64 } %233, 1
  %241 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %239, 0
  %242 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %241, ptr %240, 1
  %243 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %242, i64 32, 2
  %244 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %243, i64 8, 3, 0
  %245 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %244, i64 33, 4, 0
  %246 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %245, i64 1, 3, 1
  %247 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %246, i64 1, 4, 1
  %248 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 0
  %249 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %250 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %248, 0
  %251 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %250, ptr %249, 1
  %252 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %251, i64 0, 2
  %253 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %252, i64 8, 3, 0
  %254 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %253, i64 1, 4, 0
  %255 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %254, i64 1, 3, 1
  %256 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %255, i64 1, 4, 1
  %257 = call ptr @llvm.stacksave.p0()
  %258 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %247, ptr %258, align 8
  %259 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %258, 1
  %260 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %256, ptr %260, align 8
  %261 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %260, 1
  %262 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %259, ptr %262, align 8
  %263 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %261, ptr %263, align 8
  call void @memrefCopy(i64 4, ptr %262, ptr %263)
  call void @llvm.stackrestore.p0(ptr %257)
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
  %281 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %280, i64 288, 2
  %282 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %281, i64 1, 3, 0
  %283 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %282, i64 9, 4, 0
  %284 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %283, i64 8, 3, 1
  %285 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %284, i64 1, 4, 1
  %286 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 0
  %287 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %288 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %286, 0
  %289 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %288, ptr %287, 1
  %290 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %289, i64 0, 2
  %291 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %290, i64 1, 3, 0
  %292 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %291, i64 8, 4, 0
  %293 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %292, i64 8, 3, 1
  %294 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %293, i64 1, 4, 1
  %295 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %285, 3, 0
  %296 = mul i64 1, %295
  %297 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %285, 3, 1
  %298 = mul i64 %296, %297
  %299 = mul i64 %298, 4
  %300 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %285, 1
  %301 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %285, 2
  %302 = getelementptr float, ptr %300, i64 %301
  %303 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %294, 1
  %304 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %294, 2
  %305 = getelementptr float, ptr %303, i64 %304
  call void @llvm.memcpy.p0.p0.i64(ptr %305, ptr %302, i64 %299, i1 false)
  %306 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %307 = ptrtoint ptr %306 to i64
  %308 = inttoptr i64 %307 to ptr
  %309 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 1, i32 8, ptr %266, ptr %308, ptr %98, i32 0, i32 128, i32 110)
  %310 = call ptr @llvm.stacksave.p0()
  %311 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %311, align 8
  %312 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %311, 1
  %313 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %79, ptr %313, align 8
  %314 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %313, 1
  %315 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %312, ptr %315, align 8
  %316 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %314, ptr %316, align 8
  call void @memrefCopy(i64 4, ptr %315, ptr %316)
  call void @llvm.stackrestore.p0(ptr %310)
  %317 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %318 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %319 = insertvalue { ptr, ptr, i64 } poison, ptr %317, 0
  %320 = insertvalue { ptr, ptr, i64 } %319, ptr %318, 1
  %321 = insertvalue { ptr, ptr, i64 } %320, i64 0, 2
  %322 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %323 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %324 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %325 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %326 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %327 = extractvalue { ptr, ptr, i64 } %321, 0
  %328 = extractvalue { ptr, ptr, i64 } %321, 1
  %329 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %327, 0
  %330 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %329, ptr %328, 1
  %331 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %330, i64 8, 2
  %332 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %331, i64 4, 3, 0
  %333 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %332, i64 9, 4, 0
  %334 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %333, i64 1, 3, 1
  %335 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %334, i64 1, 4, 1
  %336 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 0
  %337 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 1
  %338 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %336, 0
  %339 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %338, ptr %337, 1
  %340 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %339, i64 0, 2
  %341 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %340, i64 4, 3, 0
  %342 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %341, i64 1, 4, 0
  %343 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %342, i64 1, 3, 1
  %344 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %343, i64 1, 4, 1
  %345 = call ptr @llvm.stacksave.p0()
  %346 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %335, ptr %346, align 8
  %347 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %346, 1
  %348 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %344, ptr %348, align 8
  %349 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %348, 1
  %350 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %347, ptr %350, align 8
  %351 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %349, ptr %351, align 8
  call void @memrefCopy(i64 4, ptr %350, ptr %351)
  call void @llvm.stackrestore.p0(ptr %345)
  %352 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 1
  %353 = ptrtoint ptr %352 to i64
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
  %369 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %368, i64 0, 2
  %370 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %369, i64 4, 3, 0
  %371 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %370, i64 33, 4, 0
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
  %409 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %408, i64 9, 4, 0
  %410 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %409, i64 1, 3, 1
  %411 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %410, i64 1, 4, 1
  %412 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 0
  %413 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %414 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %412, 0
  %415 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %414, ptr %413, 1
  %416 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %415, i64 0, 2
  %417 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %416, i64 16, 3, 0
  %418 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %417, i64 1, 4, 0
  %419 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %418, i64 1, 3, 1
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
  %431 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 1, ptr %392, ptr %430, ptr %354, i32 1, i32 0, i32 117)
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
  %446 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %445, i64 16, 2
  %447 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %446, i64 4, 3, 0
  %448 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %447, i64 33, 4, 0
  %449 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %448, i64 16, 3, 1
  %450 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %449, i64 1, 4, 1
  %451 = call ptr @llvm.stacksave.p0()
  %452 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %450, ptr %452, align 8
  %453 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %452, 1
  %454 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %382, ptr %454, align 8
  %455 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %454, 1
  %456 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %453, ptr %456, align 8
  %457 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %455, ptr %457, align 8
  call void @memrefCopy(i64 4, ptr %456, ptr %457)
  call void @llvm.stackrestore.p0(ptr %451)
  %458 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %459 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %460 = insertvalue { ptr, ptr, i64 } poison, ptr %458, 0
  %461 = insertvalue { ptr, ptr, i64 } %460, ptr %459, 1
  %462 = insertvalue { ptr, ptr, i64 } %461, i64 0, 2
  %463 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %464 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %465 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %466 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %467 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %468 = extractvalue { ptr, ptr, i64 } %462, 0
  %469 = extractvalue { ptr, ptr, i64 } %462, 1
  %470 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %468, 0
  %471 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %470, ptr %469, 1
  %472 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %471, i64 152, 2
  %473 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %472, i64 16, 3, 0
  %474 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %473, i64 9, 4, 0
  %475 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %474, i64 1, 3, 1
  %476 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %475, i64 1, 4, 1
  %477 = call ptr @llvm.stacksave.p0()
  %478 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %476, ptr %478, align 8
  %479 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %478, 1
  %480 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %420, ptr %480, align 8
  %481 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %480, 1
  %482 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %479, ptr %482, align 8
  %483 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %481, ptr %483, align 8
  call void @memrefCopy(i64 4, ptr %482, ptr %483)
  call void @llvm.stackrestore.p0(ptr %477)
  %484 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 1, ptr %392, ptr %430, ptr %354, i32 1, i32 42, i32 117)
  %485 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %486 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %487 = insertvalue { ptr, ptr, i64 } poison, ptr %485, 0
  %488 = insertvalue { ptr, ptr, i64 } %487, ptr %486, 1
  %489 = insertvalue { ptr, ptr, i64 } %488, i64 0, 2
  %490 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %491 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %492 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %493 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %494 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %495 = extractvalue { ptr, ptr, i64 } %489, 0
  %496 = extractvalue { ptr, ptr, i64 } %489, 1
  %497 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %495, 0
  %498 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %497, ptr %496, 1
  %499 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %498, i64 32, 2
  %500 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %499, i64 4, 3, 0
  %501 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %500, i64 33, 4, 0
  %502 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %501, i64 1, 3, 1
  %503 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %502, i64 1, 4, 1
  %504 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 0
  %505 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %506 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %504, 0
  %507 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %506, ptr %505, 1
  %508 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %507, i64 0, 2
  %509 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %508, i64 4, 3, 0
  %510 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %509, i64 1, 4, 0
  %511 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %510, i64 1, 3, 1
  %512 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %511, i64 1, 4, 1
  %513 = call ptr @llvm.stacksave.p0()
  %514 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %503, ptr %514, align 8
  %515 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %514, 1
  %516 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %512, ptr %516, align 8
  %517 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %516, 1
  %518 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %515, ptr %518, align 8
  %519 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %517, ptr %519, align 8
  call void @memrefCopy(i64 4, ptr %518, ptr %519)
  call void @llvm.stackrestore.p0(ptr %513)
  %520 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %521 = ptrtoint ptr %520 to i64
  %522 = inttoptr i64 %521 to ptr
  %523 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %524 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %525 = insertvalue { ptr, ptr, i64 } poison, ptr %523, 0
  %526 = insertvalue { ptr, ptr, i64 } %525, ptr %524, 1
  %527 = insertvalue { ptr, ptr, i64 } %526, i64 0, 2
  %528 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %529 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %530 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %531 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %532 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %533 = extractvalue { ptr, ptr, i64 } %527, 0
  %534 = extractvalue { ptr, ptr, i64 } %527, 1
  %535 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %533, 0
  %536 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %535, ptr %534, 1
  %537 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %536, i64 296, 2
  %538 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %537, i64 1, 3, 0
  %539 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %538, i64 9, 4, 0
  %540 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %539, i64 1, 3, 1
  %541 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %540, i64 1, 4, 1
  %542 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 0
  %543 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %544 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %542, 0
  %545 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %544, ptr %543, 1
  %546 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %545, i64 0, 2
  %547 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %546, i64 1, 3, 0
  %548 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %547, i64 1, 4, 0
  %549 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %548, i64 1, 3, 1
  %550 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %549, i64 1, 4, 1
  %551 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %541, 3, 0
  %552 = mul i64 1, %551
  %553 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %541, 3, 1
  %554 = mul i64 %552, %553
  %555 = mul i64 %554, 4
  %556 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %541, 1
  %557 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %541, 2
  %558 = getelementptr float, ptr %556, i64 %557
  %559 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %550, 1
  %560 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %550, 2
  %561 = getelementptr float, ptr %559, i64 %560
  call void @llvm.memcpy.p0.p0.i64(ptr %561, ptr %558, i64 %555, i1 false)
  %562 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %563 = ptrtoint ptr %562 to i64
  %564 = inttoptr i64 %563 to ptr
  %565 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 1, i32 1, ptr %522, ptr %564, ptr %354, i32 1, i32 120, i32 102)
  %566 = call ptr @llvm.stacksave.p0()
  %567 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %344, ptr %567, align 8
  %568 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %567, 1
  %569 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %335, ptr %569, align 8
  %570 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %569, 1
  %571 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %568, ptr %571, align 8
  %572 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %570, ptr %572, align 8
  call void @memrefCopy(i64 4, ptr %571, ptr %572)
  call void @llvm.stackrestore.p0(ptr %566)
  %573 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %574 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %575 = insertvalue { ptr, ptr, i64 } poison, ptr %573, 0
  %576 = insertvalue { ptr, ptr, i64 } %575, ptr %574, 1
  %577 = insertvalue { ptr, ptr, i64 } %576, i64 0, 2
  %578 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %579 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %580 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %581 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %582 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %583 = extractvalue { ptr, ptr, i64 } %577, 0
  %584 = extractvalue { ptr, ptr, i64 } %577, 1
  %585 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %583, 0
  %586 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %585, ptr %584, 1
  %587 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %586, i64 44, 2
  %588 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %587, i64 4, 3, 0
  %589 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %588, i64 9, 4, 0
  %590 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %589, i64 1, 3, 1
  %591 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %590, i64 1, 4, 1
  %592 = call ptr @llvm.stacksave.p0()
  %593 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %591, ptr %593, align 8
  %594 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %593, 1
  %595 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %344, ptr %595, align 8
  %596 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %595, 1
  %597 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %594, ptr %597, align 8
  %598 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %596, ptr %598, align 8
  call void @memrefCopy(i64 4, ptr %597, ptr %598)
  call void @llvm.stackrestore.p0(ptr %592)
  %599 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %600 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %601 = insertvalue { ptr, ptr, i64 } poison, ptr %599, 0
  %602 = insertvalue { ptr, ptr, i64 } %601, ptr %600, 1
  %603 = insertvalue { ptr, ptr, i64 } %602, i64 0, 2
  %604 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %605 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %606 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %607 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %608 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %609 = extractvalue { ptr, ptr, i64 } %603, 0
  %610 = extractvalue { ptr, ptr, i64 } %603, 1
  %611 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %609, 0
  %612 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %611, ptr %610, 1
  %613 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %612, i64 132, 2
  %614 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %613, i64 4, 3, 0
  %615 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %614, i64 33, 4, 0
  %616 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %615, i64 16, 3, 1
  %617 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %616, i64 1, 4, 1
  %618 = call ptr @llvm.stacksave.p0()
  %619 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %617, ptr %619, align 8
  %620 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %619, 1
  %621 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %382, ptr %621, align 8
  %622 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %621, 1
  %623 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %620, ptr %623, align 8
  %624 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %622, ptr %624, align 8
  call void @memrefCopy(i64 4, ptr %623, ptr %624)
  call void @llvm.stackrestore.p0(ptr %618)
  %625 = call ptr @llvm.stacksave.p0()
  %626 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %411, ptr %626, align 8
  %627 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %626, 1
  %628 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %420, ptr %628, align 8
  %629 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %628, 1
  %630 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %627, ptr %630, align 8
  %631 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %629, ptr %631, align 8
  call void @memrefCopy(i64 4, ptr %630, ptr %631)
  call void @llvm.stackrestore.p0(ptr %625)
  %632 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 1, ptr %392, ptr %430, ptr %354, i32 2, i32 0, i32 117)
  %633 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %634 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %635 = insertvalue { ptr, ptr, i64 } poison, ptr %633, 0
  %636 = insertvalue { ptr, ptr, i64 } %635, ptr %634, 1
  %637 = insertvalue { ptr, ptr, i64 } %636, i64 0, 2
  %638 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %639 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %640 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %641 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %642 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %643 = extractvalue { ptr, ptr, i64 } %637, 0
  %644 = extractvalue { ptr, ptr, i64 } %637, 1
  %645 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %643, 0
  %646 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %645, ptr %644, 1
  %647 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %646, i64 148, 2
  %648 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %647, i64 4, 3, 0
  %649 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %648, i64 33, 4, 0
  %650 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %649, i64 16, 3, 1
  %651 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %650, i64 1, 4, 1
  %652 = call ptr @llvm.stacksave.p0()
  %653 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %651, ptr %653, align 8
  %654 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %653, 1
  %655 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %382, ptr %655, align 8
  %656 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %655, 1
  %657 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %654, ptr %657, align 8
  %658 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %656, ptr %658, align 8
  call void @memrefCopy(i64 4, ptr %657, ptr %658)
  call void @llvm.stackrestore.p0(ptr %652)
  %659 = call ptr @llvm.stacksave.p0()
  %660 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %476, ptr %660, align 8
  %661 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %660, 1
  %662 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %420, ptr %662, align 8
  %663 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %662, 1
  %664 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %661, ptr %664, align 8
  %665 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %663, ptr %665, align 8
  call void @memrefCopy(i64 4, ptr %664, ptr %665)
  call void @llvm.stackrestore.p0(ptr %659)
  %666 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 1, ptr %392, ptr %430, ptr %354, i32 2, i32 42, i32 117)
  %667 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %668 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %669 = insertvalue { ptr, ptr, i64 } poison, ptr %667, 0
  %670 = insertvalue { ptr, ptr, i64 } %669, ptr %668, 1
  %671 = insertvalue { ptr, ptr, i64 } %670, i64 0, 2
  %672 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %673 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %674 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %675 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %676 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %677 = extractvalue { ptr, ptr, i64 } %671, 0
  %678 = extractvalue { ptr, ptr, i64 } %671, 1
  %679 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %677, 0
  %680 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %679, ptr %678, 1
  %681 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %680, i64 164, 2
  %682 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %681, i64 4, 3, 0
  %683 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %682, i64 33, 4, 0
  %684 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %683, i64 1, 3, 1
  %685 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %684, i64 1, 4, 1
  %686 = call ptr @llvm.stacksave.p0()
  %687 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %685, ptr %687, align 8
  %688 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %687, 1
  %689 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %512, ptr %689, align 8
  %690 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %689, 1
  %691 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %688, ptr %691, align 8
  %692 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %690, ptr %692, align 8
  call void @memrefCopy(i64 4, ptr %691, ptr %692)
  call void @llvm.stackrestore.p0(ptr %686)
  %693 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %541, 3, 0
  %694 = mul i64 1, %693
  %695 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %541, 3, 1
  %696 = mul i64 %694, %695
  %697 = mul i64 %696, 4
  %698 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %541, 1
  %699 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %541, 2
  %700 = getelementptr float, ptr %698, i64 %699
  %701 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %550, 1
  %702 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %550, 2
  %703 = getelementptr float, ptr %701, i64 %702
  call void @llvm.memcpy.p0.p0.i64(ptr %703, ptr %700, i64 %697, i1 false)
  %704 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 1, i32 1, ptr %522, ptr %564, ptr %354, i32 2, i32 120, i32 102)
  %705 = call ptr @llvm.stacksave.p0()
  %706 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %344, ptr %706, align 8
  %707 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %706, 1
  %708 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %591, ptr %708, align 8
  %709 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %708, 1
  %710 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %707, ptr %710, align 8
  %711 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %709, ptr %711, align 8
  call void @memrefCopy(i64 4, ptr %710, ptr %711)
  call void @llvm.stackrestore.p0(ptr %705)
  %712 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %713 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %714 = insertvalue { ptr, ptr, i64 } poison, ptr %712, 0
  %715 = insertvalue { ptr, ptr, i64 } %714, ptr %713, 1
  %716 = insertvalue { ptr, ptr, i64 } %715, i64 0, 2
  %717 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %718 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %719 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %720 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %721 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %722 = extractvalue { ptr, ptr, i64 } %716, 0
  %723 = extractvalue { ptr, ptr, i64 } %716, 1
  %724 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %722, 0
  %725 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %724, ptr %723, 1
  %726 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %725, i64 72, 2
  %727 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %726, i64 8, 3, 0
  %728 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %727, i64 9, 4, 0
  %729 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %728, i64 8, 3, 1
  %730 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %729, i64 1, 4, 1
  %731 = call ptr @llvm.stacksave.p0()
  %732 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %730, ptr %732, align 8
  %733 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %732, 1
  %734 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %734, align 8
  %735 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %734, 1
  %736 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %733, ptr %736, align 8
  %737 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %735, ptr %737, align 8
  call void @memrefCopy(i64 4, ptr %736, ptr %737)
  call void @llvm.stackrestore.p0(ptr %731)
  %738 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %739 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %740 = insertvalue { ptr, ptr, i64 } poison, ptr %738, 0
  %741 = insertvalue { ptr, ptr, i64 } %740, ptr %739, 1
  %742 = insertvalue { ptr, ptr, i64 } %741, i64 0, 2
  %743 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %744 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %745 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %746 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %747 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %748 = extractvalue { ptr, ptr, i64 } %742, 0
  %749 = extractvalue { ptr, ptr, i64 } %742, 1
  %750 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %748, 0
  %751 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %750, ptr %749, 1
  %752 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %751, i64 264, 2
  %753 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %752, i64 8, 3, 0
  %754 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %753, i64 33, 4, 0
  %755 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %754, i64 16, 3, 1
  %756 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %755, i64 1, 4, 1
  %757 = call ptr @llvm.stacksave.p0()
  %758 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %756, ptr %758, align 8
  %759 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %758, 1
  %760 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %760, align 8
  %761 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %760, 1
  %762 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %759, ptr %762, align 8
  %763 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %761, ptr %763, align 8
  call void @memrefCopy(i64 4, ptr %762, ptr %763)
  call void @llvm.stackrestore.p0(ptr %757)
  %764 = call ptr @llvm.stacksave.p0()
  %765 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %155, ptr %765, align 8
  %766 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %765, 1
  %767 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %767, align 8
  %768 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %767, 1
  %769 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %766, ptr %769, align 8
  %770 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %768, ptr %770, align 8
  call void @memrefCopy(i64 4, ptr %769, ptr %770)
  call void @llvm.stackrestore.p0(ptr %764)
  %771 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 197, i32 125)
  %772 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %773 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %774 = insertvalue { ptr, ptr, i64 } poison, ptr %772, 0
  %775 = insertvalue { ptr, ptr, i64 } %774, ptr %773, 1
  %776 = insertvalue { ptr, ptr, i64 } %775, i64 0, 2
  %777 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %778 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %779 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %780 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %781 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %782 = extractvalue { ptr, ptr, i64 } %776, 0
  %783 = extractvalue { ptr, ptr, i64 } %776, 1
  %784 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %782, 0
  %785 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %784, ptr %783, 1
  %786 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %785, i64 280, 2
  %787 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %786, i64 8, 3, 0
  %788 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %787, i64 33, 4, 0
  %789 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %788, i64 16, 3, 1
  %790 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %789, i64 1, 4, 1
  %791 = call ptr @llvm.stacksave.p0()
  %792 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %790, ptr %792, align 8
  %793 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %792, 1
  %794 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %794, align 8
  %795 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %794, 1
  %796 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %793, ptr %796, align 8
  %797 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %795, ptr %797, align 8
  call void @memrefCopy(i64 4, ptr %796, ptr %797)
  call void @llvm.stackrestore.p0(ptr %791)
  %798 = call ptr @llvm.stacksave.p0()
  %799 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %220, ptr %799, align 8
  %800 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %799, 1
  %801 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %801, align 8
  %802 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %801, 1
  %803 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %800, ptr %803, align 8
  %804 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %802, ptr %804, align 8
  call void @memrefCopy(i64 4, ptr %803, ptr %804)
  call void @llvm.stackrestore.p0(ptr %798)
  %805 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 266, i32 125)
  %806 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %807 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %808 = insertvalue { ptr, ptr, i64 } poison, ptr %806, 0
  %809 = insertvalue { ptr, ptr, i64 } %808, ptr %807, 1
  %810 = insertvalue { ptr, ptr, i64 } %809, i64 0, 2
  %811 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %812 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %813 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %814 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %815 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %816 = extractvalue { ptr, ptr, i64 } %810, 0
  %817 = extractvalue { ptr, ptr, i64 } %810, 1
  %818 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %816, 0
  %819 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %818, ptr %817, 1
  %820 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %819, i64 296, 2
  %821 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %820, i64 8, 3, 0
  %822 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %821, i64 33, 4, 0
  %823 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %822, i64 1, 3, 1
  %824 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %823, i64 1, 4, 1
  %825 = call ptr @llvm.stacksave.p0()
  %826 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %824, ptr %826, align 8
  %827 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %826, 1
  %828 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %256, ptr %828, align 8
  %829 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %828, 1
  %830 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %827, ptr %830, align 8
  %831 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %829, ptr %831, align 8
  call void @memrefCopy(i64 4, ptr %830, ptr %831)
  call void @llvm.stackrestore.p0(ptr %825)
  %832 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %285, 3, 0
  %833 = mul i64 1, %832
  %834 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %285, 3, 1
  %835 = mul i64 %833, %834
  %836 = mul i64 %835, 4
  %837 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %285, 1
  %838 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %285, 2
  %839 = getelementptr float, ptr %837, i64 %838
  %840 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %294, 1
  %841 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %294, 2
  %842 = getelementptr float, ptr %840, i64 %841
  call void @llvm.memcpy.p0.p0.i64(ptr %842, ptr %839, i64 %836, i1 false)
  %843 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 1, i32 8, ptr %266, ptr %308, ptr %98, i32 0, i32 335, i32 110)
  %844 = call ptr @llvm.stacksave.p0()
  %845 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %845, align 8
  %846 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %845, 1
  %847 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %730, ptr %847, align 8
  %848 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %847, 1
  %849 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %846, ptr %849, align 8
  %850 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %848, ptr %850, align 8
  call void @memrefCopy(i64 4, ptr %849, ptr %850)
  call void @llvm.stackrestore.p0(ptr %844)
  %851 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %852 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %853 = insertvalue { ptr, ptr, i64 } poison, ptr %851, 0
  %854 = insertvalue { ptr, ptr, i64 } %853, ptr %852, 1
  %855 = insertvalue { ptr, ptr, i64 } %854, i64 0, 2
  %856 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %857 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %858 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %859 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %860 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %861 = extractvalue { ptr, ptr, i64 } %855, 0
  %862 = extractvalue { ptr, ptr, i64 } %855, 1
  %863 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %861, 0
  %864 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %863, ptr %862, 1
  %865 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %864, i64 80, 2
  %866 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %865, i64 4, 3, 0
  %867 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %866, i64 9, 4, 0
  %868 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %867, i64 1, 3, 1
  %869 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %868, i64 1, 4, 1
  %870 = call ptr @llvm.stacksave.p0()
  %871 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %869, ptr %871, align 8
  %872 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %871, 1
  %873 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %344, ptr %873, align 8
  %874 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %873, 1
  %875 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %872, ptr %875, align 8
  %876 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %874, ptr %876, align 8
  call void @memrefCopy(i64 4, ptr %875, ptr %876)
  call void @llvm.stackrestore.p0(ptr %870)
  %877 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %878 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %879 = insertvalue { ptr, ptr, i64 } poison, ptr %877, 0
  %880 = insertvalue { ptr, ptr, i64 } %879, ptr %878, 1
  %881 = insertvalue { ptr, ptr, i64 } %880, i64 0, 2
  %882 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %883 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %884 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %885 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %886 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %887 = extractvalue { ptr, ptr, i64 } %881, 0
  %888 = extractvalue { ptr, ptr, i64 } %881, 1
  %889 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %887, 0
  %890 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %889, ptr %888, 1
  %891 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %890, i64 264, 2
  %892 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %891, i64 4, 3, 0
  %893 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %892, i64 33, 4, 0
  %894 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %893, i64 16, 3, 1
  %895 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %894, i64 1, 4, 1
  %896 = call ptr @llvm.stacksave.p0()
  %897 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %895, ptr %897, align 8
  %898 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %897, 1
  %899 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %382, ptr %899, align 8
  %900 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %899, 1
  %901 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %898, ptr %901, align 8
  %902 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %900, ptr %902, align 8
  call void @memrefCopy(i64 4, ptr %901, ptr %902)
  call void @llvm.stackrestore.p0(ptr %896)
  %903 = call ptr @llvm.stacksave.p0()
  %904 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %411, ptr %904, align 8
  %905 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %904, 1
  %906 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %420, ptr %906, align 8
  %907 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %906, 1
  %908 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %905, ptr %908, align 8
  %909 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %907, ptr %909, align 8
  call void @memrefCopy(i64 4, ptr %908, ptr %909)
  call void @llvm.stackrestore.p0(ptr %903)
  %910 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 1, ptr %392, ptr %430, ptr %354, i32 3, i32 0, i32 117)
  %911 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %912 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %913 = insertvalue { ptr, ptr, i64 } poison, ptr %911, 0
  %914 = insertvalue { ptr, ptr, i64 } %913, ptr %912, 1
  %915 = insertvalue { ptr, ptr, i64 } %914, i64 0, 2
  %916 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %917 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %918 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %919 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %920 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %921 = extractvalue { ptr, ptr, i64 } %915, 0
  %922 = extractvalue { ptr, ptr, i64 } %915, 1
  %923 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %921, 0
  %924 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %923, ptr %922, 1
  %925 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %924, i64 280, 2
  %926 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %925, i64 4, 3, 0
  %927 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %926, i64 33, 4, 0
  %928 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %927, i64 16, 3, 1
  %929 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %928, i64 1, 4, 1
  %930 = call ptr @llvm.stacksave.p0()
  %931 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %929, ptr %931, align 8
  %932 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %931, 1
  %933 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %382, ptr %933, align 8
  %934 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %933, 1
  %935 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %932, ptr %935, align 8
  %936 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %934, ptr %936, align 8
  call void @memrefCopy(i64 4, ptr %935, ptr %936)
  call void @llvm.stackrestore.p0(ptr %930)
  %937 = call ptr @llvm.stacksave.p0()
  %938 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %476, ptr %938, align 8
  %939 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %938, 1
  %940 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %420, ptr %940, align 8
  %941 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %940, 1
  %942 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %939, ptr %942, align 8
  %943 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %941, ptr %943, align 8
  call void @memrefCopy(i64 4, ptr %942, ptr %943)
  call void @llvm.stackrestore.p0(ptr %937)
  %944 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 1, ptr %392, ptr %430, ptr %354, i32 3, i32 42, i32 117)
  %945 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %946 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %947 = insertvalue { ptr, ptr, i64 } poison, ptr %945, 0
  %948 = insertvalue { ptr, ptr, i64 } %947, ptr %946, 1
  %949 = insertvalue { ptr, ptr, i64 } %948, i64 0, 2
  %950 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %951 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %952 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %953 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %954 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %955 = extractvalue { ptr, ptr, i64 } %949, 0
  %956 = extractvalue { ptr, ptr, i64 } %949, 1
  %957 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %955, 0
  %958 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %957, ptr %956, 1
  %959 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %958, i64 296, 2
  %960 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %959, i64 4, 3, 0
  %961 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %960, i64 33, 4, 0
  %962 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %961, i64 1, 3, 1
  %963 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %962, i64 1, 4, 1
  %964 = call ptr @llvm.stacksave.p0()
  %965 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %963, ptr %965, align 8
  %966 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %965, 1
  %967 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %512, ptr %967, align 8
  %968 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %967, 1
  %969 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %966, ptr %969, align 8
  %970 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %968, ptr %970, align 8
  call void @memrefCopy(i64 4, ptr %969, ptr %970)
  call void @llvm.stackrestore.p0(ptr %964)
  %971 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %541, 3, 0
  %972 = mul i64 1, %971
  %973 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %541, 3, 1
  %974 = mul i64 %972, %973
  %975 = mul i64 %974, 4
  %976 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %541, 1
  %977 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %541, 2
  %978 = getelementptr float, ptr %976, i64 %977
  %979 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %550, 1
  %980 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %550, 2
  %981 = getelementptr float, ptr %979, i64 %980
  call void @llvm.memcpy.p0.p0.i64(ptr %981, ptr %978, i64 %975, i1 false)
  %982 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 1, i32 1, ptr %522, ptr %564, ptr %354, i32 3, i32 120, i32 102)
  %983 = call ptr @llvm.stacksave.p0()
  %984 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %344, ptr %984, align 8
  %985 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %984, 1
  %986 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %869, ptr %986, align 8
  %987 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %986, 1
  %988 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %985, ptr %988, align 8
  %989 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %987, ptr %989, align 8
  call void @memrefCopy(i64 4, ptr %988, ptr %989)
  call void @llvm.stackrestore.p0(ptr %983)
  %990 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %991 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %992 = insertvalue { ptr, ptr, i64 } poison, ptr %990, 0
  %993 = insertvalue { ptr, ptr, i64 } %992, ptr %991, 1
  %994 = insertvalue { ptr, ptr, i64 } %993, i64 0, 2
  %995 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %996 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %997 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %998 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %999 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %1000 = extractvalue { ptr, ptr, i64 } %994, 0
  %1001 = extractvalue { ptr, ptr, i64 } %994, 1
  %1002 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1000, 0
  %1003 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1002, ptr %1001, 1
  %1004 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1003, i64 116, 2
  %1005 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1004, i64 4, 3, 0
  %1006 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1005, i64 9, 4, 0
  %1007 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1006, i64 1, 3, 1
  %1008 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1007, i64 1, 4, 1
  %1009 = call ptr @llvm.stacksave.p0()
  %1010 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1008, ptr %1010, align 8
  %1011 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1010, 1
  %1012 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %344, ptr %1012, align 8
  %1013 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1012, 1
  %1014 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1011, ptr %1014, align 8
  %1015 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1013, ptr %1015, align 8
  call void @memrefCopy(i64 4, ptr %1014, ptr %1015)
  call void @llvm.stackrestore.p0(ptr %1009)
  %1016 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %1017 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %1018 = insertvalue { ptr, ptr, i64 } poison, ptr %1016, 0
  %1019 = insertvalue { ptr, ptr, i64 } %1018, ptr %1017, 1
  %1020 = insertvalue { ptr, ptr, i64 } %1019, i64 0, 2
  %1021 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %1022 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %1023 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %1024 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %1025 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %1026 = extractvalue { ptr, ptr, i64 } %1020, 0
  %1027 = extractvalue { ptr, ptr, i64 } %1020, 1
  %1028 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1026, 0
  %1029 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1028, ptr %1027, 1
  %1030 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1029, i64 396, 2
  %1031 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1030, i64 4, 3, 0
  %1032 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1031, i64 33, 4, 0
  %1033 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1032, i64 16, 3, 1
  %1034 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1033, i64 1, 4, 1
  %1035 = call ptr @llvm.stacksave.p0()
  %1036 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1034, ptr %1036, align 8
  %1037 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1036, 1
  %1038 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %382, ptr %1038, align 8
  %1039 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1038, 1
  %1040 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1037, ptr %1040, align 8
  %1041 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1039, ptr %1041, align 8
  call void @memrefCopy(i64 4, ptr %1040, ptr %1041)
  call void @llvm.stackrestore.p0(ptr %1035)
  %1042 = call ptr @llvm.stacksave.p0()
  %1043 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %411, ptr %1043, align 8
  %1044 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1043, 1
  %1045 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %420, ptr %1045, align 8
  %1046 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1045, 1
  %1047 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1044, ptr %1047, align 8
  %1048 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1046, ptr %1048, align 8
  call void @memrefCopy(i64 4, ptr %1047, ptr %1048)
  call void @llvm.stackrestore.p0(ptr %1042)
  %1049 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 1, ptr %392, ptr %430, ptr %354, i32 1, i32 189, i32 117)
  %1050 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %1051 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %1052 = insertvalue { ptr, ptr, i64 } poison, ptr %1050, 0
  %1053 = insertvalue { ptr, ptr, i64 } %1052, ptr %1051, 1
  %1054 = insertvalue { ptr, ptr, i64 } %1053, i64 0, 2
  %1055 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %1056 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %1057 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %1058 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %1059 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %1060 = extractvalue { ptr, ptr, i64 } %1054, 0
  %1061 = extractvalue { ptr, ptr, i64 } %1054, 1
  %1062 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1060, 0
  %1063 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1062, ptr %1061, 1
  %1064 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1063, i64 412, 2
  %1065 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1064, i64 4, 3, 0
  %1066 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1065, i64 33, 4, 0
  %1067 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1066, i64 16, 3, 1
  %1068 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1067, i64 1, 4, 1
  %1069 = call ptr @llvm.stacksave.p0()
  %1070 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1068, ptr %1070, align 8
  %1071 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1070, 1
  %1072 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %382, ptr %1072, align 8
  %1073 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1072, 1
  %1074 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1071, ptr %1074, align 8
  %1075 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1073, ptr %1075, align 8
  call void @memrefCopy(i64 4, ptr %1074, ptr %1075)
  call void @llvm.stackrestore.p0(ptr %1069)
  %1076 = call ptr @llvm.stacksave.p0()
  %1077 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %476, ptr %1077, align 8
  %1078 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1077, 1
  %1079 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %420, ptr %1079, align 8
  %1080 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1079, 1
  %1081 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1078, ptr %1081, align 8
  %1082 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1080, ptr %1082, align 8
  call void @memrefCopy(i64 4, ptr %1081, ptr %1082)
  call void @llvm.stackrestore.p0(ptr %1076)
  %1083 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 1, ptr %392, ptr %430, ptr %354, i32 1, i32 258, i32 117)
  %1084 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %1085 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %1086 = insertvalue { ptr, ptr, i64 } poison, ptr %1084, 0
  %1087 = insertvalue { ptr, ptr, i64 } %1086, ptr %1085, 1
  %1088 = insertvalue { ptr, ptr, i64 } %1087, i64 0, 2
  %1089 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %1090 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %1091 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %1092 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %1093 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %1094 = extractvalue { ptr, ptr, i64 } %1088, 0
  %1095 = extractvalue { ptr, ptr, i64 } %1088, 1
  %1096 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1094, 0
  %1097 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1096, ptr %1095, 1
  %1098 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1097, i64 428, 2
  %1099 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1098, i64 4, 3, 0
  %1100 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1099, i64 33, 4, 0
  %1101 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1100, i64 1, 3, 1
  %1102 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1101, i64 1, 4, 1
  %1103 = call ptr @llvm.stacksave.p0()
  %1104 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1102, ptr %1104, align 8
  %1105 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1104, 1
  %1106 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %512, ptr %1106, align 8
  %1107 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1106, 1
  %1108 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1105, ptr %1108, align 8
  %1109 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1107, ptr %1109, align 8
  call void @memrefCopy(i64 4, ptr %1108, ptr %1109)
  call void @llvm.stackrestore.p0(ptr %1103)
  %1110 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %541, 3, 0
  %1111 = mul i64 1, %1110
  %1112 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %541, 3, 1
  %1113 = mul i64 %1111, %1112
  %1114 = mul i64 %1113, 4
  %1115 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %541, 1
  %1116 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %541, 2
  %1117 = getelementptr float, ptr %1115, i64 %1116
  %1118 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %550, 1
  %1119 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %550, 2
  %1120 = getelementptr float, ptr %1118, i64 %1119
  call void @llvm.memcpy.p0.p0.i64(ptr %1120, ptr %1117, i64 %1114, i1 false)
  %1121 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 1, i32 1, ptr %522, ptr %564, ptr %354, i32 1, i32 327, i32 102)
  %1122 = call ptr @llvm.stacksave.p0()
  %1123 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %344, ptr %1123, align 8
  %1124 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1123, 1
  %1125 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1008, ptr %1125, align 8
  %1126 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1125, 1
  %1127 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1124, ptr %1127, align 8
  %1128 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1126, ptr %1128, align 8
  call void @memrefCopy(i64 4, ptr %1127, ptr %1128)
  call void @llvm.stackrestore.p0(ptr %1122)
  %1129 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %1130 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %1131 = insertvalue { ptr, ptr, i64 } poison, ptr %1129, 0
  %1132 = insertvalue { ptr, ptr, i64 } %1131, ptr %1130, 1
  %1133 = insertvalue { ptr, ptr, i64 } %1132, i64 0, 2
  %1134 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %1135 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %1136 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %1137 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %1138 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %1139 = extractvalue { ptr, ptr, i64 } %1133, 0
  %1140 = extractvalue { ptr, ptr, i64 } %1133, 1
  %1141 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1139, 0
  %1142 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1141, ptr %1140, 1
  %1143 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1142, i64 144, 2
  %1144 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1143, i64 1, 3, 0
  %1145 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1144, i64 9, 4, 0
  %1146 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1145, i64 4, 3, 1
  %1147 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1146, i64 1, 4, 1
  %1148 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 0
  %1149 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 1
  %1150 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1148, 0
  %1151 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1150, ptr %1149, 1
  %1152 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1151, i64 0, 2
  %1153 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1152, i64 1, 3, 0
  %1154 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1153, i64 4, 4, 0
  %1155 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1154, i64 4, 3, 1
  %1156 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1155, i64 1, 4, 1
  %1157 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1147, 3, 0
  %1158 = mul i64 1, %1157
  %1159 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1147, 3, 1
  %1160 = mul i64 %1158, %1159
  %1161 = mul i64 %1160, 4
  %1162 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1147, 1
  %1163 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1147, 2
  %1164 = getelementptr float, ptr %1162, i64 %1163
  %1165 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1156, 1
  %1166 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1156, 2
  %1167 = getelementptr float, ptr %1165, i64 %1166
  call void @llvm.memcpy.p0.p0.i64(ptr %1167, ptr %1164, i64 %1161, i1 false)
  %1168 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 1
  %1169 = ptrtoint ptr %1168 to i64
  %1170 = inttoptr i64 %1169 to ptr
  %1171 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %1172 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %1173 = insertvalue { ptr, ptr, i64 } poison, ptr %1171, 0
  %1174 = insertvalue { ptr, ptr, i64 } %1173, ptr %1172, 1
  %1175 = insertvalue { ptr, ptr, i64 } %1174, i64 0, 2
  %1176 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %1177 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %1178 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %1179 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %1180 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %1181 = extractvalue { ptr, ptr, i64 } %1175, 0
  %1182 = extractvalue { ptr, ptr, i64 } %1175, 1
  %1183 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1181, 0
  %1184 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1183, ptr %1182, 1
  %1185 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1184, i64 528, 2
  %1186 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1185, i64 1, 3, 0
  %1187 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1186, i64 33, 4, 0
  %1188 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1187, i64 16, 3, 1
  %1189 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1188, i64 1, 4, 1
  %1190 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 0
  %1191 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %1192 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1190, 0
  %1193 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1192, ptr %1191, 1
  %1194 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1193, i64 0, 2
  %1195 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1194, i64 1, 3, 0
  %1196 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1195, i64 16, 4, 0
  %1197 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1196, i64 16, 3, 1
  %1198 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1197, i64 1, 4, 1
  %1199 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1189, 3, 0
  %1200 = mul i64 1, %1199
  %1201 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1189, 3, 1
  %1202 = mul i64 %1200, %1201
  %1203 = mul i64 %1202, 4
  %1204 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1189, 1
  %1205 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1189, 2
  %1206 = getelementptr float, ptr %1204, i64 %1205
  %1207 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1198, 1
  %1208 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1198, 2
  %1209 = getelementptr float, ptr %1207, i64 %1208
  call void @llvm.memcpy.p0.p0.i64(ptr %1209, ptr %1206, i64 %1203, i1 false)
  %1210 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %1211 = ptrtoint ptr %1210 to i64
  %1212 = inttoptr i64 %1211 to ptr
  %1213 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %1214 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %1215 = insertvalue { ptr, ptr, i64 } poison, ptr %1213, 0
  %1216 = insertvalue { ptr, ptr, i64 } %1215, ptr %1214, 1
  %1217 = insertvalue { ptr, ptr, i64 } %1216, i64 0, 2
  %1218 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %1219 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %1220 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %1221 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %1222 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %1223 = extractvalue { ptr, ptr, i64 } %1217, 0
  %1224 = extractvalue { ptr, ptr, i64 } %1217, 1
  %1225 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1223, 0
  %1226 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1225, ptr %1224, 1
  %1227 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1226, i64 0, 2
  %1228 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1227, i64 16, 3, 0
  %1229 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1228, i64 9, 4, 0
  %1230 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1229, i64 4, 3, 1
  %1231 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1230, i64 1, 4, 1
  %1232 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 0
  %1233 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %1234 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1232, 0
  %1235 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1234, ptr %1233, 1
  %1236 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1235, i64 0, 2
  %1237 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1236, i64 16, 3, 0
  %1238 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1237, i64 4, 4, 0
  %1239 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1238, i64 4, 3, 1
  %1240 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1239, i64 1, 4, 1
  %1241 = call ptr @llvm.stacksave.p0()
  %1242 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1231, ptr %1242, align 8
  %1243 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1242, 1
  %1244 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1240, ptr %1244, align 8
  %1245 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1244, 1
  %1246 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1243, ptr %1246, align 8
  %1247 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1245, ptr %1247, align 8
  call void @memrefCopy(i64 4, ptr %1246, ptr %1247)
  call void @llvm.stackrestore.p0(ptr %1241)
  %1248 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %1249 = ptrtoint ptr %1248 to i64
  %1250 = inttoptr i64 %1249 to ptr
  %1251 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 1, i32 16, i32 4, ptr %1212, ptr %1250, ptr %1170, i32 2, i32 189, i32 117)
  %1252 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %1253 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %1254 = insertvalue { ptr, ptr, i64 } poison, ptr %1252, 0
  %1255 = insertvalue { ptr, ptr, i64 } %1254, ptr %1253, 1
  %1256 = insertvalue { ptr, ptr, i64 } %1255, i64 0, 2
  %1257 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %1258 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %1259 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %1260 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %1261 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %1262 = extractvalue { ptr, ptr, i64 } %1256, 0
  %1263 = extractvalue { ptr, ptr, i64 } %1256, 1
  %1264 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1262, 0
  %1265 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1264, ptr %1263, 1
  %1266 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1265, i64 544, 2
  %1267 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1266, i64 1, 3, 0
  %1268 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1267, i64 33, 4, 0
  %1269 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1268, i64 16, 3, 1
  %1270 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1269, i64 1, 4, 1
  %1271 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1270, 3, 0
  %1272 = mul i64 1, %1271
  %1273 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1270, 3, 1
  %1274 = mul i64 %1272, %1273
  %1275 = mul i64 %1274, 4
  %1276 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1270, 1
  %1277 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1270, 2
  %1278 = getelementptr float, ptr %1276, i64 %1277
  %1279 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1198, 1
  %1280 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1198, 2
  %1281 = getelementptr float, ptr %1279, i64 %1280
  call void @llvm.memcpy.p0.p0.i64(ptr %1281, ptr %1278, i64 %1275, i1 false)
  %1282 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %1283 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %1284 = insertvalue { ptr, ptr, i64 } poison, ptr %1282, 0
  %1285 = insertvalue { ptr, ptr, i64 } %1284, ptr %1283, 1
  %1286 = insertvalue { ptr, ptr, i64 } %1285, i64 0, 2
  %1287 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %1288 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %1289 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %1290 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %1291 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %1292 = extractvalue { ptr, ptr, i64 } %1286, 0
  %1293 = extractvalue { ptr, ptr, i64 } %1286, 1
  %1294 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1292, 0
  %1295 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1294, ptr %1293, 1
  %1296 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1295, i64 144, 2
  %1297 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1296, i64 16, 3, 0
  %1298 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1297, i64 9, 4, 0
  %1299 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1298, i64 4, 3, 1
  %1300 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1299, i64 1, 4, 1
  %1301 = call ptr @llvm.stacksave.p0()
  %1302 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1300, ptr %1302, align 8
  %1303 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1302, 1
  %1304 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1240, ptr %1304, align 8
  %1305 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1304, 1
  %1306 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1303, ptr %1306, align 8
  %1307 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1305, ptr %1307, align 8
  call void @memrefCopy(i64 4, ptr %1306, ptr %1307)
  call void @llvm.stackrestore.p0(ptr %1301)
  %1308 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 1, i32 16, i32 4, ptr %1212, ptr %1250, ptr %1170, i32 2, i32 258, i32 117)
  %1309 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %1310 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %1311 = insertvalue { ptr, ptr, i64 } poison, ptr %1309, 0
  %1312 = insertvalue { ptr, ptr, i64 } %1311, ptr %1310, 1
  %1313 = insertvalue { ptr, ptr, i64 } %1312, i64 0, 2
  %1314 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %1315 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %1316 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %1317 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %1318 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %1319 = extractvalue { ptr, ptr, i64 } %1313, 0
  %1320 = extractvalue { ptr, ptr, i64 } %1313, 1
  %1321 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1319, 0
  %1322 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1321, ptr %1320, 1
  %1323 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1322, i64 560, 2
  %1324 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1323, i64 1, 3, 0
  %1325 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1324, i64 33, 4, 0
  %1326 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1325, i64 1, 3, 1
  %1327 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1326, i64 1, 4, 1
  %1328 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 0
  %1329 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %1330 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1328, 0
  %1331 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1330, ptr %1329, 1
  %1332 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1331, i64 0, 2
  %1333 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1332, i64 1, 3, 0
  %1334 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1333, i64 1, 4, 0
  %1335 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1334, i64 1, 3, 1
  %1336 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1335, i64 1, 4, 1
  %1337 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1327, 3, 0
  %1338 = mul i64 1, %1337
  %1339 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1327, 3, 1
  %1340 = mul i64 %1338, %1339
  %1341 = mul i64 %1340, 4
  %1342 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1327, 1
  %1343 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1327, 2
  %1344 = getelementptr float, ptr %1342, i64 %1343
  %1345 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1336, 1
  %1346 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1336, 2
  %1347 = getelementptr float, ptr %1345, i64 %1346
  call void @llvm.memcpy.p0.p0.i64(ptr %1347, ptr %1344, i64 %1341, i1 false)
  %1348 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %1349 = ptrtoint ptr %1348 to i64
  %1350 = inttoptr i64 %1349 to ptr
  %1351 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %1352 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %1353 = insertvalue { ptr, ptr, i64 } poison, ptr %1351, 0
  %1354 = insertvalue { ptr, ptr, i64 } %1353, ptr %1352, 1
  %1355 = insertvalue { ptr, ptr, i64 } %1354, i64 0, 2
  %1356 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %1357 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %1358 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %1359 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %1360 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %1361 = extractvalue { ptr, ptr, i64 } %1355, 0
  %1362 = extractvalue { ptr, ptr, i64 } %1355, 1
  %1363 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1361, 0
  %1364 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1363, ptr %1362, 1
  %1365 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1364, i64 288, 2
  %1366 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1365, i64 1, 3, 0
  %1367 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1366, i64 9, 4, 0
  %1368 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1367, i64 4, 3, 1
  %1369 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1368, i64 1, 4, 1
  %1370 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 0
  %1371 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %1372 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1370, 0
  %1373 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1372, ptr %1371, 1
  %1374 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1373, i64 0, 2
  %1375 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1374, i64 1, 3, 0
  %1376 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1375, i64 4, 4, 0
  %1377 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1376, i64 4, 3, 1
  %1378 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1377, i64 1, 4, 1
  %1379 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1369, 3, 0
  %1380 = mul i64 1, %1379
  %1381 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1369, 3, 1
  %1382 = mul i64 %1380, %1381
  %1383 = mul i64 %1382, 4
  %1384 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1369, 1
  %1385 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1369, 2
  %1386 = getelementptr float, ptr %1384, i64 %1385
  %1387 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1378, 1
  %1388 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1378, 2
  %1389 = getelementptr float, ptr %1387, i64 %1388
  call void @llvm.memcpy.p0.p0.i64(ptr %1389, ptr %1386, i64 %1383, i1 false)
  %1390 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %1391 = ptrtoint ptr %1390 to i64
  %1392 = inttoptr i64 %1391 to ptr
  %1393 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 1, i32 1, i32 4, ptr %1350, ptr %1392, ptr %1170, i32 2, i32 327, i32 102)
  %1394 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1156, 3, 0
  %1395 = mul i64 1, %1394
  %1396 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1156, 3, 1
  %1397 = mul i64 %1395, %1396
  %1398 = mul i64 %1397, 4
  %1399 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1156, 1
  %1400 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1156, 2
  %1401 = getelementptr float, ptr %1399, i64 %1400
  %1402 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1147, 1
  %1403 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1147, 2
  %1404 = getelementptr float, ptr %1402, i64 %1403
  call void @llvm.memcpy.p0.p0.i64(ptr %1404, ptr %1401, i64 %1398, i1 false)
  %1405 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %1406 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %1407 = insertvalue { ptr, ptr, i64 } poison, ptr %1405, 0
  %1408 = insertvalue { ptr, ptr, i64 } %1407, ptr %1406, 1
  %1409 = insertvalue { ptr, ptr, i64 } %1408, i64 0, 2
  %1410 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %1411 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %1412 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %1413 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %1414 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %1415 = extractvalue { ptr, ptr, i64 } %1409, 0
  %1416 = extractvalue { ptr, ptr, i64 } %1409, 1
  %1417 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1415, 0
  %1418 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1417, ptr %1416, 1
  %1419 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1418, i64 148, 2
  %1420 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1419, i64 1, 3, 0
  %1421 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1420, i64 9, 4, 0
  %1422 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1421, i64 4, 3, 1
  %1423 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1422, i64 1, 4, 1
  %1424 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1423, 3, 0
  %1425 = mul i64 1, %1424
  %1426 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1423, 3, 1
  %1427 = mul i64 %1425, %1426
  %1428 = mul i64 %1427, 4
  %1429 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1423, 1
  %1430 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1423, 2
  %1431 = getelementptr float, ptr %1429, i64 %1430
  %1432 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1156, 1
  %1433 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1156, 2
  %1434 = getelementptr float, ptr %1432, i64 %1433
  call void @llvm.memcpy.p0.p0.i64(ptr %1434, ptr %1431, i64 %1428, i1 false)
  %1435 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1189, 3, 0
  %1436 = mul i64 1, %1435
  %1437 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1189, 3, 1
  %1438 = mul i64 %1436, %1437
  %1439 = mul i64 %1438, 4
  %1440 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1189, 1
  %1441 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1189, 2
  %1442 = getelementptr float, ptr %1440, i64 %1441
  %1443 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1198, 1
  %1444 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1198, 2
  %1445 = getelementptr float, ptr %1443, i64 %1444
  call void @llvm.memcpy.p0.p0.i64(ptr %1445, ptr %1442, i64 %1439, i1 false)
  %1446 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %1447 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %1448 = insertvalue { ptr, ptr, i64 } poison, ptr %1446, 0
  %1449 = insertvalue { ptr, ptr, i64 } %1448, ptr %1447, 1
  %1450 = insertvalue { ptr, ptr, i64 } %1449, i64 0, 2
  %1451 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %1452 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %1453 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %1454 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %1455 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %1456 = extractvalue { ptr, ptr, i64 } %1450, 0
  %1457 = extractvalue { ptr, ptr, i64 } %1450, 1
  %1458 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1456, 0
  %1459 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1458, ptr %1457, 1
  %1460 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1459, i64 4, 2
  %1461 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1460, i64 16, 3, 0
  %1462 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1461, i64 9, 4, 0
  %1463 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1462, i64 4, 3, 1
  %1464 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1463, i64 1, 4, 1
  %1465 = call ptr @llvm.stacksave.p0()
  %1466 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1464, ptr %1466, align 8
  %1467 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1466, 1
  %1468 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1240, ptr %1468, align 8
  %1469 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1468, 1
  %1470 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1467, ptr %1470, align 8
  %1471 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1469, ptr %1471, align 8
  call void @memrefCopy(i64 4, ptr %1470, ptr %1471)
  call void @llvm.stackrestore.p0(ptr %1465)
  %1472 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 1, i32 16, i32 4, ptr %1212, ptr %1250, ptr %1170, i32 3, i32 189, i32 117)
  %1473 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1270, 3, 0
  %1474 = mul i64 1, %1473
  %1475 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1270, 3, 1
  %1476 = mul i64 %1474, %1475
  %1477 = mul i64 %1476, 4
  %1478 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1270, 1
  %1479 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1270, 2
  %1480 = getelementptr float, ptr %1478, i64 %1479
  %1481 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1198, 1
  %1482 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1198, 2
  %1483 = getelementptr float, ptr %1481, i64 %1482
  call void @llvm.memcpy.p0.p0.i64(ptr %1483, ptr %1480, i64 %1477, i1 false)
  %1484 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %1485 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %1486 = insertvalue { ptr, ptr, i64 } poison, ptr %1484, 0
  %1487 = insertvalue { ptr, ptr, i64 } %1486, ptr %1485, 1
  %1488 = insertvalue { ptr, ptr, i64 } %1487, i64 0, 2
  %1489 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %1490 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %1491 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %1492 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %1493 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %1494 = extractvalue { ptr, ptr, i64 } %1488, 0
  %1495 = extractvalue { ptr, ptr, i64 } %1488, 1
  %1496 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1494, 0
  %1497 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1496, ptr %1495, 1
  %1498 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1497, i64 148, 2
  %1499 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1498, i64 16, 3, 0
  %1500 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1499, i64 9, 4, 0
  %1501 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1500, i64 4, 3, 1
  %1502 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1501, i64 1, 4, 1
  %1503 = call ptr @llvm.stacksave.p0()
  %1504 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1502, ptr %1504, align 8
  %1505 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1504, 1
  %1506 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1240, ptr %1506, align 8
  %1507 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1506, 1
  %1508 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1505, ptr %1508, align 8
  %1509 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1507, ptr %1509, align 8
  call void @memrefCopy(i64 4, ptr %1508, ptr %1509)
  call void @llvm.stackrestore.p0(ptr %1503)
  %1510 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 1, i32 16, i32 4, ptr %1212, ptr %1250, ptr %1170, i32 3, i32 258, i32 117)
  %1511 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1327, 3, 0
  %1512 = mul i64 1, %1511
  %1513 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1327, 3, 1
  %1514 = mul i64 %1512, %1513
  %1515 = mul i64 %1514, 4
  %1516 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1327, 1
  %1517 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1327, 2
  %1518 = getelementptr float, ptr %1516, i64 %1517
  %1519 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1336, 1
  %1520 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1336, 2
  %1521 = getelementptr float, ptr %1519, i64 %1520
  call void @llvm.memcpy.p0.p0.i64(ptr %1521, ptr %1518, i64 %1515, i1 false)
  %1522 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %1523 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %1524 = insertvalue { ptr, ptr, i64 } poison, ptr %1522, 0
  %1525 = insertvalue { ptr, ptr, i64 } %1524, ptr %1523, 1
  %1526 = insertvalue { ptr, ptr, i64 } %1525, i64 0, 2
  %1527 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %1528 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %1529 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %1530 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %1531 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %1532 = extractvalue { ptr, ptr, i64 } %1526, 0
  %1533 = extractvalue { ptr, ptr, i64 } %1526, 1
  %1534 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1532, 0
  %1535 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1534, ptr %1533, 1
  %1536 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1535, i64 292, 2
  %1537 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1536, i64 1, 3, 0
  %1538 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1537, i64 9, 4, 0
  %1539 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1538, i64 4, 3, 1
  %1540 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1539, i64 1, 4, 1
  %1541 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1540, 3, 0
  %1542 = mul i64 1, %1541
  %1543 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1540, 3, 1
  %1544 = mul i64 %1542, %1543
  %1545 = mul i64 %1544, 4
  %1546 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1540, 1
  %1547 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1540, 2
  %1548 = getelementptr float, ptr %1546, i64 %1547
  %1549 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1378, 1
  %1550 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1378, 2
  %1551 = getelementptr float, ptr %1549, i64 %1550
  call void @llvm.memcpy.p0.p0.i64(ptr %1551, ptr %1548, i64 %1545, i1 false)
  %1552 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 1, i32 1, i32 4, ptr %1350, ptr %1392, ptr %1170, i32 3, i32 327, i32 102)
  %1553 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1156, 3, 0
  %1554 = mul i64 1, %1553
  %1555 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1156, 3, 1
  %1556 = mul i64 %1554, %1555
  %1557 = mul i64 %1556, 4
  %1558 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1156, 1
  %1559 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1156, 2
  %1560 = getelementptr float, ptr %1558, i64 %1559
  %1561 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1423, 1
  %1562 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1423, 2
  %1563 = getelementptr float, ptr %1561, i64 %1562
  call void @llvm.memcpy.p0.p0.i64(ptr %1563, ptr %1560, i64 %1557, i1 false)
  %1564 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %1565 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %1566 = insertvalue { ptr, ptr, i64 } poison, ptr %1564, 0
  %1567 = insertvalue { ptr, ptr, i64 } %1566, ptr %1565, 1
  %1568 = insertvalue { ptr, ptr, i64 } %1567, i64 0, 2
  %1569 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %1570 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %1571 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %1572 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %1573 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %1574 = extractvalue { ptr, ptr, i64 } %1568, 0
  %1575 = extractvalue { ptr, ptr, i64 } %1568, 1
  %1576 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1574, 0
  %1577 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1576, ptr %1575, 1
  %1578 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1577, i64 152, 2
  %1579 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1578, i64 1, 3, 0
  %1580 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1579, i64 9, 4, 0
  %1581 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1580, i64 1, 3, 1
  %1582 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1581, i64 1, 4, 1
  %1583 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 0
  %1584 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 1
  %1585 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1583, 0
  %1586 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1585, ptr %1584, 1
  %1587 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1586, i64 0, 2
  %1588 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1587, i64 1, 3, 0
  %1589 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1588, i64 1, 4, 0
  %1590 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1589, i64 1, 3, 1
  %1591 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1590, i64 1, 4, 1
  %1592 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1582, 3, 0
  %1593 = mul i64 1, %1592
  %1594 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1582, 3, 1
  %1595 = mul i64 %1593, %1594
  %1596 = mul i64 %1595, 4
  %1597 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1582, 1
  %1598 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1582, 2
  %1599 = getelementptr float, ptr %1597, i64 %1598
  %1600 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1591, 1
  %1601 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1591, 2
  %1602 = getelementptr float, ptr %1600, i64 %1601
  call void @llvm.memcpy.p0.p0.i64(ptr %1602, ptr %1599, i64 %1596, i1 false)
  %1603 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 1
  %1604 = ptrtoint ptr %1603 to i64
  %1605 = inttoptr i64 %1604 to ptr
  %1606 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1189, 3, 0
  %1607 = mul i64 1, %1606
  %1608 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1189, 3, 1
  %1609 = mul i64 %1607, %1608
  %1610 = mul i64 %1609, 4
  %1611 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1189, 1
  %1612 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1189, 2
  %1613 = getelementptr float, ptr %1611, i64 %1612
  %1614 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1198, 1
  %1615 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1198, 2
  %1616 = getelementptr float, ptr %1614, i64 %1615
  call void @llvm.memcpy.p0.p0.i64(ptr %1616, ptr %1613, i64 %1610, i1 false)
  %1617 = call ptr @llvm.stacksave.p0()
  %1618 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %411, ptr %1618, align 8
  %1619 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1618, 1
  %1620 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %420, ptr %1620, align 8
  %1621 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1620, 1
  %1622 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1619, ptr %1622, align 8
  %1623 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1621, ptr %1623, align 8
  call void @memrefCopy(i64 4, ptr %1622, ptr %1623)
  call void @llvm.stackrestore.p0(ptr %1617)
  %1624 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 1, i32 16, i32 1, ptr %1212, ptr %430, ptr %1605, i32 1, i32 396, i32 117)
  %1625 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1270, 3, 0
  %1626 = mul i64 1, %1625
  %1627 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1270, 3, 1
  %1628 = mul i64 %1626, %1627
  %1629 = mul i64 %1628, 4
  %1630 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1270, 1
  %1631 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1270, 2
  %1632 = getelementptr float, ptr %1630, i64 %1631
  %1633 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1198, 1
  %1634 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1198, 2
  %1635 = getelementptr float, ptr %1633, i64 %1634
  call void @llvm.memcpy.p0.p0.i64(ptr %1635, ptr %1632, i64 %1629, i1 false)
  %1636 = call ptr @llvm.stacksave.p0()
  %1637 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %476, ptr %1637, align 8
  %1638 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1637, 1
  %1639 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %420, ptr %1639, align 8
  %1640 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1639, 1
  %1641 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1638, ptr %1641, align 8
  %1642 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1640, ptr %1642, align 8
  call void @memrefCopy(i64 4, ptr %1641, ptr %1642)
  call void @llvm.stackrestore.p0(ptr %1636)
  %1643 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 1, i32 16, i32 1, ptr %1212, ptr %430, ptr %1605, i32 1, i32 465, i32 117)
  %1644 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1327, 3, 0
  %1645 = mul i64 1, %1644
  %1646 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1327, 3, 1
  %1647 = mul i64 %1645, %1646
  %1648 = mul i64 %1647, 4
  %1649 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1327, 1
  %1650 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1327, 2
  %1651 = getelementptr float, ptr %1649, i64 %1650
  %1652 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1336, 1
  %1653 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1336, 2
  %1654 = getelementptr float, ptr %1652, i64 %1653
  call void @llvm.memcpy.p0.p0.i64(ptr %1654, ptr %1651, i64 %1648, i1 false)
  %1655 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %541, 3, 0
  %1656 = mul i64 1, %1655
  %1657 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %541, 3, 1
  %1658 = mul i64 %1656, %1657
  %1659 = mul i64 %1658, 4
  %1660 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %541, 1
  %1661 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %541, 2
  %1662 = getelementptr float, ptr %1660, i64 %1661
  %1663 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %550, 1
  %1664 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %550, 2
  %1665 = getelementptr float, ptr %1663, i64 %1664
  call void @llvm.memcpy.p0.p0.i64(ptr %1665, ptr %1662, i64 %1659, i1 false)
  %1666 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 1, i32 1, i32 1, ptr %1350, ptr %564, ptr %1605, i32 1, i32 534, i32 102)
  %1667 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1591, 3, 0
  %1668 = mul i64 1, %1667
  %1669 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1591, 3, 1
  %1670 = mul i64 %1668, %1669
  %1671 = mul i64 %1670, 4
  %1672 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1591, 1
  %1673 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1591, 2
  %1674 = getelementptr float, ptr %1672, i64 %1673
  %1675 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1582, 1
  %1676 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1582, 2
  %1677 = getelementptr float, ptr %1675, i64 %1676
  call void @llvm.memcpy.p0.p0.i64(ptr %1677, ptr %1674, i64 %1671, i1 false)
  %1678 = call ptr @malloc(i64 612)
  %1679 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1678, 0
  %1680 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1679, ptr %1678, 1
  %1681 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1680, i64 0, 2
  %1682 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1681, i64 17, 3, 0
  %1683 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1682, i64 9, 3, 1
  %1684 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1683, i64 9, 4, 0
  %1685 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1684, i64 1, 4, 1
  %1686 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %1687 = mul i64 1, %1686
  %1688 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %1689 = mul i64 %1687, %1688
  %1690 = mul i64 %1689, 4
  %1691 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %1692 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %1693 = getelementptr float, ptr %1691, i64 %1692
  %1694 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1685, 1
  %1695 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1685, 2
  %1696 = getelementptr float, ptr %1694, i64 %1695
  call void @llvm.memcpy.p0.p0.i64(ptr %1696, ptr %1693, i64 %1690, i1 false)
  ret { ptr, ptr, i64, [2 x i64], [2 x i64] } %1685
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
