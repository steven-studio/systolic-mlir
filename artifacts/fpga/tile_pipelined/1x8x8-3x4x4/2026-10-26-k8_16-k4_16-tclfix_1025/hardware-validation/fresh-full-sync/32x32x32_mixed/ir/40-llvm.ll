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
  %77 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %76, i64 32, 4, 0
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
  %115 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %114, i64 32, 4, 0
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
  %153 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %152, i64 32, 4, 0
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
  %192 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %191, i64 32, 4, 0
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
  %216 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %215, i64 512, 2
  %217 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %216, i64 16, 3, 0
  %218 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %217, i64 32, 4, 0
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
  %229 = call ptr @llvm.stacksave.p0()
  %230 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %230, align 8
  %231 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %230, 1
  %232 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %79, ptr %232, align 8
  %233 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %232, 1
  %234 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %231, ptr %234, align 8
  %235 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %233, ptr %235, align 8
  call void @memrefCopy(i64 4, ptr %234, ptr %235)
  call void @llvm.stackrestore.p0(ptr %229)
  %236 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %237 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %238 = insertvalue { ptr, ptr, i64 } poison, ptr %236, 0
  %239 = insertvalue { ptr, ptr, i64 } %238, ptr %237, 1
  %240 = insertvalue { ptr, ptr, i64 } %239, i64 0, 2
  %241 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %242 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %243 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %244 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %245 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %246 = extractvalue { ptr, ptr, i64 } %240, 0
  %247 = extractvalue { ptr, ptr, i64 } %240, 1
  %248 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %246, 0
  %249 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %248, ptr %247, 1
  %250 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %249, i64 8, 2
  %251 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %250, i64 8, 3, 0
  %252 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %251, i64 32, 4, 0
  %253 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %252, i64 8, 3, 1
  %254 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %253, i64 1, 4, 1
  %255 = call ptr @llvm.stacksave.p0()
  %256 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %254, ptr %256, align 8
  %257 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %256, 1
  %258 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %258, align 8
  %259 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %258, 1
  %260 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %257, ptr %260, align 8
  %261 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %259, ptr %261, align 8
  call void @memrefCopy(i64 4, ptr %260, ptr %261)
  call void @llvm.stackrestore.p0(ptr %255)
  %262 = call ptr @llvm.stacksave.p0()
  %263 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %117, ptr %263, align 8
  %264 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %263, 1
  %265 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %265, align 8
  %266 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %265, 1
  %267 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %264, ptr %267, align 8
  %268 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %266, ptr %268, align 8
  call void @memrefCopy(i64 4, ptr %267, ptr %268)
  call void @llvm.stackrestore.p0(ptr %262)
  %269 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %270 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %271 = insertvalue { ptr, ptr, i64 } poison, ptr %269, 0
  %272 = insertvalue { ptr, ptr, i64 } %271, ptr %270, 1
  %273 = insertvalue { ptr, ptr, i64 } %272, i64 0, 2
  %274 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %275 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %276 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %277 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %278 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %279 = extractvalue { ptr, ptr, i64 } %273, 0
  %280 = extractvalue { ptr, ptr, i64 } %273, 1
  %281 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %279, 0
  %282 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %281, ptr %280, 1
  %283 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %282, i64 8, 2
  %284 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %283, i64 16, 3, 0
  %285 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %284, i64 32, 4, 0
  %286 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %285, i64 8, 3, 1
  %287 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %286, i64 1, 4, 1
  %288 = call ptr @llvm.stacksave.p0()
  %289 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %287, ptr %289, align 8
  %290 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %289, 1
  %291 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %291, align 8
  %292 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %291, 1
  %293 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %290, ptr %293, align 8
  %294 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %292, ptr %294, align 8
  call void @memrefCopy(i64 4, ptr %293, ptr %294)
  call void @llvm.stackrestore.p0(ptr %288)
  %295 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 128, i32 125)
  %296 = call ptr @llvm.stacksave.p0()
  %297 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %194, ptr %297, align 8
  %298 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %297, 1
  %299 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %299, align 8
  %300 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %299, 1
  %301 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %298, ptr %301, align 8
  %302 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %300, ptr %302, align 8
  call void @memrefCopy(i64 4, ptr %301, ptr %302)
  call void @llvm.stackrestore.p0(ptr %296)
  %303 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %304 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %305 = insertvalue { ptr, ptr, i64 } poison, ptr %303, 0
  %306 = insertvalue { ptr, ptr, i64 } %305, ptr %304, 1
  %307 = insertvalue { ptr, ptr, i64 } %306, i64 0, 2
  %308 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %309 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %310 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %311 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %312 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %313 = extractvalue { ptr, ptr, i64 } %307, 0
  %314 = extractvalue { ptr, ptr, i64 } %307, 1
  %315 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %313, 0
  %316 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %315, ptr %314, 1
  %317 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %316, i64 520, 2
  %318 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %317, i64 16, 3, 0
  %319 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %318, i64 32, 4, 0
  %320 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %319, i64 8, 3, 1
  %321 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %320, i64 1, 4, 1
  %322 = call ptr @llvm.stacksave.p0()
  %323 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %321, ptr %323, align 8
  %324 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %323, 1
  %325 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %325, align 8
  %326 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %325, 1
  %327 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %324, ptr %327, align 8
  %328 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %326, ptr %328, align 8
  call void @memrefCopy(i64 4, ptr %327, ptr %328)
  call void @llvm.stackrestore.p0(ptr %322)
  %329 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 197, i32 125)
  %330 = call ptr @llvm.stacksave.p0()
  %331 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %331, align 8
  %332 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %331, 1
  %333 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %254, ptr %333, align 8
  %334 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %333, 1
  %335 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %332, ptr %335, align 8
  %336 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %334, ptr %336, align 8
  call void @memrefCopy(i64 4, ptr %335, ptr %336)
  call void @llvm.stackrestore.p0(ptr %330)
  %337 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %338 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %339 = insertvalue { ptr, ptr, i64 } poison, ptr %337, 0
  %340 = insertvalue { ptr, ptr, i64 } %339, ptr %338, 1
  %341 = insertvalue { ptr, ptr, i64 } %340, i64 0, 2
  %342 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %343 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %344 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %345 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %346 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %347 = extractvalue { ptr, ptr, i64 } %341, 0
  %348 = extractvalue { ptr, ptr, i64 } %341, 1
  %349 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %347, 0
  %350 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %349, ptr %348, 1
  %351 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %350, i64 16, 2
  %352 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %351, i64 8, 3, 0
  %353 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %352, i64 32, 4, 0
  %354 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %353, i64 8, 3, 1
  %355 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %354, i64 1, 4, 1
  %356 = call ptr @llvm.stacksave.p0()
  %357 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %355, ptr %357, align 8
  %358 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %357, 1
  %359 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %359, align 8
  %360 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %359, 1
  %361 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %358, ptr %361, align 8
  %362 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %360, ptr %362, align 8
  call void @memrefCopy(i64 4, ptr %361, ptr %362)
  call void @llvm.stackrestore.p0(ptr %356)
  %363 = call ptr @llvm.stacksave.p0()
  %364 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %117, ptr %364, align 8
  %365 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %364, 1
  %366 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %366, align 8
  %367 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %366, 1
  %368 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %365, ptr %368, align 8
  %369 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %367, ptr %369, align 8
  call void @memrefCopy(i64 4, ptr %368, ptr %369)
  call void @llvm.stackrestore.p0(ptr %363)
  %370 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %371 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %372 = insertvalue { ptr, ptr, i64 } poison, ptr %370, 0
  %373 = insertvalue { ptr, ptr, i64 } %372, ptr %371, 1
  %374 = insertvalue { ptr, ptr, i64 } %373, i64 0, 2
  %375 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %376 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %377 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %378 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %379 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %380 = extractvalue { ptr, ptr, i64 } %374, 0
  %381 = extractvalue { ptr, ptr, i64 } %374, 1
  %382 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %380, 0
  %383 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %382, ptr %381, 1
  %384 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %383, i64 16, 2
  %385 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %384, i64 16, 3, 0
  %386 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %385, i64 32, 4, 0
  %387 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %386, i64 8, 3, 1
  %388 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %387, i64 1, 4, 1
  %389 = call ptr @llvm.stacksave.p0()
  %390 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %388, ptr %390, align 8
  %391 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %390, 1
  %392 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %392, align 8
  %393 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %392, 1
  %394 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %391, ptr %394, align 8
  %395 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %393, ptr %395, align 8
  call void @memrefCopy(i64 4, ptr %394, ptr %395)
  call void @llvm.stackrestore.p0(ptr %389)
  %396 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 266, i32 125)
  %397 = call ptr @llvm.stacksave.p0()
  %398 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %194, ptr %398, align 8
  %399 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %398, 1
  %400 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %400, align 8
  %401 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %400, 1
  %402 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %399, ptr %402, align 8
  %403 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %401, ptr %403, align 8
  call void @memrefCopy(i64 4, ptr %402, ptr %403)
  call void @llvm.stackrestore.p0(ptr %397)
  %404 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %405 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %406 = insertvalue { ptr, ptr, i64 } poison, ptr %404, 0
  %407 = insertvalue { ptr, ptr, i64 } %406, ptr %405, 1
  %408 = insertvalue { ptr, ptr, i64 } %407, i64 0, 2
  %409 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %410 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %411 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %412 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %413 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %414 = extractvalue { ptr, ptr, i64 } %408, 0
  %415 = extractvalue { ptr, ptr, i64 } %408, 1
  %416 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %414, 0
  %417 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %416, ptr %415, 1
  %418 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %417, i64 528, 2
  %419 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %418, i64 16, 3, 0
  %420 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %419, i64 32, 4, 0
  %421 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %420, i64 8, 3, 1
  %422 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %421, i64 1, 4, 1
  %423 = call ptr @llvm.stacksave.p0()
  %424 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %422, ptr %424, align 8
  %425 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %424, 1
  %426 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %426, align 8
  %427 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %426, 1
  %428 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %425, ptr %428, align 8
  %429 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %427, ptr %429, align 8
  call void @memrefCopy(i64 4, ptr %428, ptr %429)
  call void @llvm.stackrestore.p0(ptr %423)
  %430 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 335, i32 125)
  %431 = call ptr @llvm.stacksave.p0()
  %432 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %432, align 8
  %433 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %432, 1
  %434 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %355, ptr %434, align 8
  %435 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %434, 1
  %436 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %433, ptr %436, align 8
  %437 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %435, ptr %437, align 8
  call void @memrefCopy(i64 4, ptr %436, ptr %437)
  call void @llvm.stackrestore.p0(ptr %431)
  %438 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %439 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %440 = insertvalue { ptr, ptr, i64 } poison, ptr %438, 0
  %441 = insertvalue { ptr, ptr, i64 } %440, ptr %439, 1
  %442 = insertvalue { ptr, ptr, i64 } %441, i64 0, 2
  %443 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %444 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %445 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %446 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %447 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %448 = extractvalue { ptr, ptr, i64 } %442, 0
  %449 = extractvalue { ptr, ptr, i64 } %442, 1
  %450 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %448, 0
  %451 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %450, ptr %449, 1
  %452 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %451, i64 24, 2
  %453 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %452, i64 8, 3, 0
  %454 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %453, i64 32, 4, 0
  %455 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %454, i64 8, 3, 1
  %456 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %455, i64 1, 4, 1
  %457 = call ptr @llvm.stacksave.p0()
  %458 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %456, ptr %458, align 8
  %459 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %458, 1
  %460 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %460, align 8
  %461 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %460, 1
  %462 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %459, ptr %462, align 8
  %463 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %461, ptr %463, align 8
  call void @memrefCopy(i64 4, ptr %462, ptr %463)
  call void @llvm.stackrestore.p0(ptr %457)
  %464 = call ptr @llvm.stacksave.p0()
  %465 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %117, ptr %465, align 8
  %466 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %465, 1
  %467 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %467, align 8
  %468 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %467, 1
  %469 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %466, ptr %469, align 8
  %470 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %468, ptr %470, align 8
  call void @memrefCopy(i64 4, ptr %469, ptr %470)
  call void @llvm.stackrestore.p0(ptr %464)
  %471 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %472 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %473 = insertvalue { ptr, ptr, i64 } poison, ptr %471, 0
  %474 = insertvalue { ptr, ptr, i64 } %473, ptr %472, 1
  %475 = insertvalue { ptr, ptr, i64 } %474, i64 0, 2
  %476 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %477 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %478 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %479 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %480 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %481 = extractvalue { ptr, ptr, i64 } %475, 0
  %482 = extractvalue { ptr, ptr, i64 } %475, 1
  %483 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %481, 0
  %484 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %483, ptr %482, 1
  %485 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %484, i64 24, 2
  %486 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %485, i64 16, 3, 0
  %487 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %486, i64 32, 4, 0
  %488 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %487, i64 8, 3, 1
  %489 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %488, i64 1, 4, 1
  %490 = call ptr @llvm.stacksave.p0()
  %491 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %489, ptr %491, align 8
  %492 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %491, 1
  %493 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %493, align 8
  %494 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %493, 1
  %495 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %492, ptr %495, align 8
  %496 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %494, ptr %496, align 8
  call void @memrefCopy(i64 4, ptr %495, ptr %496)
  call void @llvm.stackrestore.p0(ptr %490)
  %497 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 404, i32 125)
  %498 = call ptr @llvm.stacksave.p0()
  %499 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %194, ptr %499, align 8
  %500 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %499, 1
  %501 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %501, align 8
  %502 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %501, 1
  %503 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %500, ptr %503, align 8
  %504 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %502, ptr %504, align 8
  call void @memrefCopy(i64 4, ptr %503, ptr %504)
  call void @llvm.stackrestore.p0(ptr %498)
  %505 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %506 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %507 = insertvalue { ptr, ptr, i64 } poison, ptr %505, 0
  %508 = insertvalue { ptr, ptr, i64 } %507, ptr %506, 1
  %509 = insertvalue { ptr, ptr, i64 } %508, i64 0, 2
  %510 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %511 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %512 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %513 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %514 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %515 = extractvalue { ptr, ptr, i64 } %509, 0
  %516 = extractvalue { ptr, ptr, i64 } %509, 1
  %517 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %515, 0
  %518 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %517, ptr %516, 1
  %519 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %518, i64 536, 2
  %520 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %519, i64 16, 3, 0
  %521 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %520, i64 32, 4, 0
  %522 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %521, i64 8, 3, 1
  %523 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %522, i64 1, 4, 1
  %524 = call ptr @llvm.stacksave.p0()
  %525 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %523, ptr %525, align 8
  %526 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %525, 1
  %527 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %527, align 8
  %528 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %527, 1
  %529 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %526, ptr %529, align 8
  %530 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %528, ptr %530, align 8
  call void @memrefCopy(i64 4, ptr %529, ptr %530)
  call void @llvm.stackrestore.p0(ptr %524)
  %531 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 473, i32 125)
  %532 = call ptr @llvm.stacksave.p0()
  %533 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %533, align 8
  %534 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %533, 1
  %535 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %456, ptr %535, align 8
  %536 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %535, 1
  %537 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %534, ptr %537, align 8
  %538 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %536, ptr %538, align 8
  call void @memrefCopy(i64 4, ptr %537, ptr %538)
  call void @llvm.stackrestore.p0(ptr %532)
  %539 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %540 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %541 = insertvalue { ptr, ptr, i64 } poison, ptr %539, 0
  %542 = insertvalue { ptr, ptr, i64 } %541, ptr %540, 1
  %543 = insertvalue { ptr, ptr, i64 } %542, i64 0, 2
  %544 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %545 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %546 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %547 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %548 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %549 = extractvalue { ptr, ptr, i64 } %543, 0
  %550 = extractvalue { ptr, ptr, i64 } %543, 1
  %551 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %549, 0
  %552 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %551, ptr %550, 1
  %553 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %552, i64 256, 2
  %554 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %553, i64 8, 3, 0
  %555 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %554, i64 32, 4, 0
  %556 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %555, i64 8, 3, 1
  %557 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %556, i64 1, 4, 1
  %558 = call ptr @llvm.stacksave.p0()
  %559 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %557, ptr %559, align 8
  %560 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %559, 1
  %561 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %561, align 8
  %562 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %561, 1
  %563 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %560, ptr %563, align 8
  %564 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %562, ptr %564, align 8
  call void @memrefCopy(i64 4, ptr %563, ptr %564)
  call void @llvm.stackrestore.p0(ptr %558)
  %565 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %566 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %567 = insertvalue { ptr, ptr, i64 } poison, ptr %565, 0
  %568 = insertvalue { ptr, ptr, i64 } %567, ptr %566, 1
  %569 = insertvalue { ptr, ptr, i64 } %568, i64 0, 2
  %570 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %571 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %572 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %573 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %574 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %575 = extractvalue { ptr, ptr, i64 } %569, 0
  %576 = extractvalue { ptr, ptr, i64 } %569, 1
  %577 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %575, 0
  %578 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %577, ptr %576, 1
  %579 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %578, i64 256, 2
  %580 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %579, i64 8, 3, 0
  %581 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %580, i64 32, 4, 0
  %582 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %581, i64 16, 3, 1
  %583 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %582, i64 1, 4, 1
  %584 = call ptr @llvm.stacksave.p0()
  %585 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %583, ptr %585, align 8
  %586 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %585, 1
  %587 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %587, align 8
  %588 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %587, 1
  %589 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %586, ptr %589, align 8
  %590 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %588, ptr %590, align 8
  call void @memrefCopy(i64 4, ptr %589, ptr %590)
  call void @llvm.stackrestore.p0(ptr %584)
  %591 = call ptr @llvm.stacksave.p0()
  %592 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %155, ptr %592, align 8
  %593 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %592, 1
  %594 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %594, align 8
  %595 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %594, 1
  %596 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %593, ptr %596, align 8
  %597 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %595, ptr %597, align 8
  call void @memrefCopy(i64 4, ptr %596, ptr %597)
  call void @llvm.stackrestore.p0(ptr %591)
  %598 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 542, i32 125)
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
  %613 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %612, i64 272, 2
  %614 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %613, i64 8, 3, 0
  %615 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %614, i64 32, 4, 0
  %616 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %615, i64 16, 3, 1
  %617 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %616, i64 1, 4, 1
  %618 = call ptr @llvm.stacksave.p0()
  %619 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %617, ptr %619, align 8
  %620 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %619, 1
  %621 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %621, align 8
  %622 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %621, 1
  %623 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %620, ptr %623, align 8
  %624 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %622, ptr %624, align 8
  call void @memrefCopy(i64 4, ptr %623, ptr %624)
  call void @llvm.stackrestore.p0(ptr %618)
  %625 = call ptr @llvm.stacksave.p0()
  %626 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %220, ptr %626, align 8
  %627 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %626, 1
  %628 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %628, align 8
  %629 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %628, 1
  %630 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %627, ptr %630, align 8
  %631 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %629, ptr %631, align 8
  call void @memrefCopy(i64 4, ptr %630, ptr %631)
  call void @llvm.stackrestore.p0(ptr %625)
  %632 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 611, i32 125)
  %633 = call ptr @llvm.stacksave.p0()
  %634 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %634, align 8
  %635 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %634, 1
  %636 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %557, ptr %636, align 8
  %637 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %636, 1
  %638 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %635, ptr %638, align 8
  %639 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %637, ptr %639, align 8
  call void @memrefCopy(i64 4, ptr %638, ptr %639)
  call void @llvm.stackrestore.p0(ptr %633)
  %640 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %641 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %642 = insertvalue { ptr, ptr, i64 } poison, ptr %640, 0
  %643 = insertvalue { ptr, ptr, i64 } %642, ptr %641, 1
  %644 = insertvalue { ptr, ptr, i64 } %643, i64 0, 2
  %645 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %646 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %647 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %648 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %649 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %650 = extractvalue { ptr, ptr, i64 } %644, 0
  %651 = extractvalue { ptr, ptr, i64 } %644, 1
  %652 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %650, 0
  %653 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %652, ptr %651, 1
  %654 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %653, i64 264, 2
  %655 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %654, i64 8, 3, 0
  %656 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %655, i64 32, 4, 0
  %657 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %656, i64 8, 3, 1
  %658 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %657, i64 1, 4, 1
  %659 = call ptr @llvm.stacksave.p0()
  %660 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %658, ptr %660, align 8
  %661 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %660, 1
  %662 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %662, align 8
  %663 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %662, 1
  %664 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %661, ptr %664, align 8
  %665 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %663, ptr %665, align 8
  call void @memrefCopy(i64 4, ptr %664, ptr %665)
  call void @llvm.stackrestore.p0(ptr %659)
  %666 = call ptr @llvm.stacksave.p0()
  %667 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %583, ptr %667, align 8
  %668 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %667, 1
  %669 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %669, align 8
  %670 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %669, 1
  %671 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %668, ptr %671, align 8
  %672 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %670, ptr %672, align 8
  call void @memrefCopy(i64 4, ptr %671, ptr %672)
  call void @llvm.stackrestore.p0(ptr %666)
  %673 = call ptr @llvm.stacksave.p0()
  %674 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %287, ptr %674, align 8
  %675 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %674, 1
  %676 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %676, align 8
  %677 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %676, 1
  %678 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %675, ptr %678, align 8
  %679 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %677, ptr %679, align 8
  call void @memrefCopy(i64 4, ptr %678, ptr %679)
  call void @llvm.stackrestore.p0(ptr %673)
  %680 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 680, i32 125)
  %681 = call ptr @llvm.stacksave.p0()
  %682 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %617, ptr %682, align 8
  %683 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %682, 1
  %684 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %684, align 8
  %685 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %684, 1
  %686 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %683, ptr %686, align 8
  %687 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %685, ptr %687, align 8
  call void @memrefCopy(i64 4, ptr %686, ptr %687)
  call void @llvm.stackrestore.p0(ptr %681)
  %688 = call ptr @llvm.stacksave.p0()
  %689 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %321, ptr %689, align 8
  %690 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %689, 1
  %691 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %691, align 8
  %692 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %691, 1
  %693 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %690, ptr %693, align 8
  %694 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %692, ptr %694, align 8
  call void @memrefCopy(i64 4, ptr %693, ptr %694)
  call void @llvm.stackrestore.p0(ptr %688)
  %695 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 749, i32 125)
  %696 = call ptr @llvm.stacksave.p0()
  %697 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %697, align 8
  %698 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %697, 1
  %699 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %658, ptr %699, align 8
  %700 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %699, 1
  %701 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %698, ptr %701, align 8
  %702 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %700, ptr %702, align 8
  call void @memrefCopy(i64 4, ptr %701, ptr %702)
  call void @llvm.stackrestore.p0(ptr %696)
  %703 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %704 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %705 = insertvalue { ptr, ptr, i64 } poison, ptr %703, 0
  %706 = insertvalue { ptr, ptr, i64 } %705, ptr %704, 1
  %707 = insertvalue { ptr, ptr, i64 } %706, i64 0, 2
  %708 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %709 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %710 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %711 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %712 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %713 = extractvalue { ptr, ptr, i64 } %707, 0
  %714 = extractvalue { ptr, ptr, i64 } %707, 1
  %715 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %713, 0
  %716 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %715, ptr %714, 1
  %717 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %716, i64 272, 2
  %718 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %717, i64 8, 3, 0
  %719 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %718, i64 32, 4, 0
  %720 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %719, i64 8, 3, 1
  %721 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %720, i64 1, 4, 1
  %722 = call ptr @llvm.stacksave.p0()
  %723 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %721, ptr %723, align 8
  %724 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %723, 1
  %725 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %725, align 8
  %726 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %725, 1
  %727 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %724, ptr %727, align 8
  %728 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %726, ptr %728, align 8
  call void @memrefCopy(i64 4, ptr %727, ptr %728)
  call void @llvm.stackrestore.p0(ptr %722)
  %729 = call ptr @llvm.stacksave.p0()
  %730 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %583, ptr %730, align 8
  %731 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %730, 1
  %732 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %732, align 8
  %733 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %732, 1
  %734 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %731, ptr %734, align 8
  %735 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %733, ptr %735, align 8
  call void @memrefCopy(i64 4, ptr %734, ptr %735)
  call void @llvm.stackrestore.p0(ptr %729)
  %736 = call ptr @llvm.stacksave.p0()
  %737 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %388, ptr %737, align 8
  %738 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %737, 1
  %739 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %739, align 8
  %740 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %739, 1
  %741 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %738, ptr %741, align 8
  %742 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %740, ptr %742, align 8
  call void @memrefCopy(i64 4, ptr %741, ptr %742)
  call void @llvm.stackrestore.p0(ptr %736)
  %743 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 818, i32 125)
  %744 = call ptr @llvm.stacksave.p0()
  %745 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %617, ptr %745, align 8
  %746 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %745, 1
  %747 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %747, align 8
  %748 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %747, 1
  %749 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %746, ptr %749, align 8
  %750 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %748, ptr %750, align 8
  call void @memrefCopy(i64 4, ptr %749, ptr %750)
  call void @llvm.stackrestore.p0(ptr %744)
  %751 = call ptr @llvm.stacksave.p0()
  %752 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %422, ptr %752, align 8
  %753 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %752, 1
  %754 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %754, align 8
  %755 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %754, 1
  %756 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %753, ptr %756, align 8
  %757 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %755, ptr %757, align 8
  call void @memrefCopy(i64 4, ptr %756, ptr %757)
  call void @llvm.stackrestore.p0(ptr %751)
  %758 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 887, i32 125)
  %759 = call ptr @llvm.stacksave.p0()
  %760 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %760, align 8
  %761 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %760, 1
  %762 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %721, ptr %762, align 8
  %763 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %762, 1
  %764 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %761, ptr %764, align 8
  %765 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %763, ptr %765, align 8
  call void @memrefCopy(i64 4, ptr %764, ptr %765)
  call void @llvm.stackrestore.p0(ptr %759)
  %766 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %767 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %768 = insertvalue { ptr, ptr, i64 } poison, ptr %766, 0
  %769 = insertvalue { ptr, ptr, i64 } %768, ptr %767, 1
  %770 = insertvalue { ptr, ptr, i64 } %769, i64 0, 2
  %771 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %772 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %773 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %774 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %775 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %776 = extractvalue { ptr, ptr, i64 } %770, 0
  %777 = extractvalue { ptr, ptr, i64 } %770, 1
  %778 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %776, 0
  %779 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %778, ptr %777, 1
  %780 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %779, i64 280, 2
  %781 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %780, i64 8, 3, 0
  %782 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %781, i64 32, 4, 0
  %783 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %782, i64 8, 3, 1
  %784 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %783, i64 1, 4, 1
  %785 = call ptr @llvm.stacksave.p0()
  %786 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %784, ptr %786, align 8
  %787 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %786, 1
  %788 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %788, align 8
  %789 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %788, 1
  %790 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %787, ptr %790, align 8
  %791 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %789, ptr %791, align 8
  call void @memrefCopy(i64 4, ptr %790, ptr %791)
  call void @llvm.stackrestore.p0(ptr %785)
  %792 = call ptr @llvm.stacksave.p0()
  %793 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %583, ptr %793, align 8
  %794 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %793, 1
  %795 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %795, align 8
  %796 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %795, 1
  %797 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %794, ptr %797, align 8
  %798 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %796, ptr %798, align 8
  call void @memrefCopy(i64 4, ptr %797, ptr %798)
  call void @llvm.stackrestore.p0(ptr %792)
  %799 = call ptr @llvm.stacksave.p0()
  %800 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %489, ptr %800, align 8
  %801 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %800, 1
  %802 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %802, align 8
  %803 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %802, 1
  %804 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %801, ptr %804, align 8
  %805 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %803, ptr %805, align 8
  call void @memrefCopy(i64 4, ptr %804, ptr %805)
  call void @llvm.stackrestore.p0(ptr %799)
  %806 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 956, i32 125)
  %807 = call ptr @llvm.stacksave.p0()
  %808 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %617, ptr %808, align 8
  %809 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %808, 1
  %810 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %810, align 8
  %811 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %810, 1
  %812 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %809, ptr %812, align 8
  %813 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %811, ptr %813, align 8
  call void @memrefCopy(i64 4, ptr %812, ptr %813)
  call void @llvm.stackrestore.p0(ptr %807)
  %814 = call ptr @llvm.stacksave.p0()
  %815 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %523, ptr %815, align 8
  %816 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %815, 1
  %817 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %817, align 8
  %818 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %817, 1
  %819 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %816, ptr %819, align 8
  %820 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %818, ptr %820, align 8
  call void @memrefCopy(i64 4, ptr %819, ptr %820)
  call void @llvm.stackrestore.p0(ptr %814)
  %821 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 1025, i32 125)
  %822 = call ptr @llvm.stacksave.p0()
  %823 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %823, align 8
  %824 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %823, 1
  %825 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %784, ptr %825, align 8
  %826 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %825, 1
  %827 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %824, ptr %827, align 8
  %828 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %826, ptr %828, align 8
  call void @memrefCopy(i64 4, ptr %827, ptr %828)
  call void @llvm.stackrestore.p0(ptr %822)
  %829 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %830 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %831 = insertvalue { ptr, ptr, i64 } poison, ptr %829, 0
  %832 = insertvalue { ptr, ptr, i64 } %831, ptr %830, 1
  %833 = insertvalue { ptr, ptr, i64 } %832, i64 0, 2
  %834 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %835 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %836 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %837 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %838 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %839 = extractvalue { ptr, ptr, i64 } %833, 0
  %840 = extractvalue { ptr, ptr, i64 } %833, 1
  %841 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %839, 0
  %842 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %841, ptr %840, 1
  %843 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %842, i64 512, 2
  %844 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %843, i64 8, 3, 0
  %845 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %844, i64 32, 4, 0
  %846 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %845, i64 8, 3, 1
  %847 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %846, i64 1, 4, 1
  %848 = call ptr @llvm.stacksave.p0()
  %849 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %847, ptr %849, align 8
  %850 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %849, 1
  %851 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %851, align 8
  %852 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %851, 1
  %853 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %850, ptr %853, align 8
  %854 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %852, ptr %854, align 8
  call void @memrefCopy(i64 4, ptr %853, ptr %854)
  call void @llvm.stackrestore.p0(ptr %848)
  %855 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %856 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %857 = insertvalue { ptr, ptr, i64 } poison, ptr %855, 0
  %858 = insertvalue { ptr, ptr, i64 } %857, ptr %856, 1
  %859 = insertvalue { ptr, ptr, i64 } %858, i64 0, 2
  %860 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %861 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %862 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %863 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %864 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %865 = extractvalue { ptr, ptr, i64 } %859, 0
  %866 = extractvalue { ptr, ptr, i64 } %859, 1
  %867 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %865, 0
  %868 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %867, ptr %866, 1
  %869 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %868, i64 512, 2
  %870 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %869, i64 8, 3, 0
  %871 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %870, i64 32, 4, 0
  %872 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %871, i64 16, 3, 1
  %873 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %872, i64 1, 4, 1
  %874 = call ptr @llvm.stacksave.p0()
  %875 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %873, ptr %875, align 8
  %876 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %875, 1
  %877 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %877, align 8
  %878 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %877, 1
  %879 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %876, ptr %879, align 8
  %880 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %878, ptr %880, align 8
  call void @memrefCopy(i64 4, ptr %879, ptr %880)
  call void @llvm.stackrestore.p0(ptr %874)
  %881 = call ptr @llvm.stacksave.p0()
  %882 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %155, ptr %882, align 8
  %883 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %882, 1
  %884 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %884, align 8
  %885 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %884, 1
  %886 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %883, ptr %886, align 8
  %887 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %885, ptr %887, align 8
  call void @memrefCopy(i64 4, ptr %886, ptr %887)
  call void @llvm.stackrestore.p0(ptr %881)
  %888 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 1094, i32 125)
  %889 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %890 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %891 = insertvalue { ptr, ptr, i64 } poison, ptr %889, 0
  %892 = insertvalue { ptr, ptr, i64 } %891, ptr %890, 1
  %893 = insertvalue { ptr, ptr, i64 } %892, i64 0, 2
  %894 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %895 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %896 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %897 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %898 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %899 = extractvalue { ptr, ptr, i64 } %893, 0
  %900 = extractvalue { ptr, ptr, i64 } %893, 1
  %901 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %899, 0
  %902 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %901, ptr %900, 1
  %903 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %902, i64 528, 2
  %904 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %903, i64 8, 3, 0
  %905 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %904, i64 32, 4, 0
  %906 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %905, i64 16, 3, 1
  %907 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %906, i64 1, 4, 1
  %908 = call ptr @llvm.stacksave.p0()
  %909 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %907, ptr %909, align 8
  %910 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %909, 1
  %911 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %911, align 8
  %912 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %911, 1
  %913 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %910, ptr %913, align 8
  %914 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %912, ptr %914, align 8
  call void @memrefCopy(i64 4, ptr %913, ptr %914)
  call void @llvm.stackrestore.p0(ptr %908)
  %915 = call ptr @llvm.stacksave.p0()
  %916 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %220, ptr %916, align 8
  %917 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %916, 1
  %918 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %918, align 8
  %919 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %918, 1
  %920 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %917, ptr %920, align 8
  %921 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %919, ptr %921, align 8
  call void @memrefCopy(i64 4, ptr %920, ptr %921)
  call void @llvm.stackrestore.p0(ptr %915)
  %922 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 1163, i32 125)
  %923 = call ptr @llvm.stacksave.p0()
  %924 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %924, align 8
  %925 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %924, 1
  %926 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %847, ptr %926, align 8
  %927 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %926, 1
  %928 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %925, ptr %928, align 8
  %929 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %927, ptr %929, align 8
  call void @memrefCopy(i64 4, ptr %928, ptr %929)
  call void @llvm.stackrestore.p0(ptr %923)
  %930 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %931 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %932 = insertvalue { ptr, ptr, i64 } poison, ptr %930, 0
  %933 = insertvalue { ptr, ptr, i64 } %932, ptr %931, 1
  %934 = insertvalue { ptr, ptr, i64 } %933, i64 0, 2
  %935 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %936 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %937 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %938 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %939 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %940 = extractvalue { ptr, ptr, i64 } %934, 0
  %941 = extractvalue { ptr, ptr, i64 } %934, 1
  %942 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %940, 0
  %943 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %942, ptr %941, 1
  %944 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %943, i64 520, 2
  %945 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %944, i64 8, 3, 0
  %946 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %945, i64 32, 4, 0
  %947 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %946, i64 8, 3, 1
  %948 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %947, i64 1, 4, 1
  %949 = call ptr @llvm.stacksave.p0()
  %950 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %948, ptr %950, align 8
  %951 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %950, 1
  %952 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %952, align 8
  %953 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %952, 1
  %954 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %951, ptr %954, align 8
  %955 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %953, ptr %955, align 8
  call void @memrefCopy(i64 4, ptr %954, ptr %955)
  call void @llvm.stackrestore.p0(ptr %949)
  %956 = call ptr @llvm.stacksave.p0()
  %957 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %873, ptr %957, align 8
  %958 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %957, 1
  %959 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %959, align 8
  %960 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %959, 1
  %961 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %958, ptr %961, align 8
  %962 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %960, ptr %962, align 8
  call void @memrefCopy(i64 4, ptr %961, ptr %962)
  call void @llvm.stackrestore.p0(ptr %956)
  %963 = call ptr @llvm.stacksave.p0()
  %964 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %287, ptr %964, align 8
  %965 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %964, 1
  %966 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %966, align 8
  %967 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %966, 1
  %968 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %965, ptr %968, align 8
  %969 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %967, ptr %969, align 8
  call void @memrefCopy(i64 4, ptr %968, ptr %969)
  call void @llvm.stackrestore.p0(ptr %963)
  %970 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 1232, i32 125)
  %971 = call ptr @llvm.stacksave.p0()
  %972 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %907, ptr %972, align 8
  %973 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %972, 1
  %974 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %974, align 8
  %975 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %974, 1
  %976 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %973, ptr %976, align 8
  %977 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %975, ptr %977, align 8
  call void @memrefCopy(i64 4, ptr %976, ptr %977)
  call void @llvm.stackrestore.p0(ptr %971)
  %978 = call ptr @llvm.stacksave.p0()
  %979 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %321, ptr %979, align 8
  %980 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %979, 1
  %981 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %981, align 8
  %982 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %981, 1
  %983 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %980, ptr %983, align 8
  %984 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %982, ptr %984, align 8
  call void @memrefCopy(i64 4, ptr %983, ptr %984)
  call void @llvm.stackrestore.p0(ptr %978)
  %985 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 1301, i32 125)
  %986 = call ptr @llvm.stacksave.p0()
  %987 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %987, align 8
  %988 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %987, 1
  %989 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %948, ptr %989, align 8
  %990 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %989, 1
  %991 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %988, ptr %991, align 8
  %992 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %990, ptr %992, align 8
  call void @memrefCopy(i64 4, ptr %991, ptr %992)
  call void @llvm.stackrestore.p0(ptr %986)
  %993 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %994 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %995 = insertvalue { ptr, ptr, i64 } poison, ptr %993, 0
  %996 = insertvalue { ptr, ptr, i64 } %995, ptr %994, 1
  %997 = insertvalue { ptr, ptr, i64 } %996, i64 0, 2
  %998 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %999 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %1000 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %1001 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %1002 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %1003 = extractvalue { ptr, ptr, i64 } %997, 0
  %1004 = extractvalue { ptr, ptr, i64 } %997, 1
  %1005 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1003, 0
  %1006 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1005, ptr %1004, 1
  %1007 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1006, i64 528, 2
  %1008 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1007, i64 4, 3, 0
  %1009 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1008, i64 32, 4, 0
  %1010 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1009, i64 4, 3, 1
  %1011 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1010, i64 1, 4, 1
  %1012 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 0
  %1013 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 1
  %1014 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1012, 0
  %1015 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1014, ptr %1013, 1
  %1016 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1015, i64 0, 2
  %1017 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1016, i64 4, 3, 0
  %1018 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1017, i64 4, 4, 0
  %1019 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1018, i64 4, 3, 1
  %1020 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1019, i64 1, 4, 1
  %1021 = call ptr @llvm.stacksave.p0()
  %1022 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1011, ptr %1022, align 8
  %1023 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1022, 1
  %1024 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1020, ptr %1024, align 8
  %1025 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1024, 1
  %1026 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1023, ptr %1026, align 8
  %1027 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1025, ptr %1027, align 8
  call void @memrefCopy(i64 4, ptr %1026, ptr %1027)
  call void @llvm.stackrestore.p0(ptr %1021)
  %1028 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 1
  %1029 = ptrtoint ptr %1028 to i64
  %1030 = inttoptr i64 %1029 to ptr
  %1031 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %1032 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %1033 = insertvalue { ptr, ptr, i64 } poison, ptr %1031, 0
  %1034 = insertvalue { ptr, ptr, i64 } %1033, ptr %1032, 1
  %1035 = insertvalue { ptr, ptr, i64 } %1034, i64 0, 2
  %1036 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %1037 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %1038 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %1039 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %1040 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %1041 = extractvalue { ptr, ptr, i64 } %1035, 0
  %1042 = extractvalue { ptr, ptr, i64 } %1035, 1
  %1043 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1041, 0
  %1044 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1043, ptr %1042, 1
  %1045 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1044, i64 512, 2
  %1046 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1045, i64 4, 3, 0
  %1047 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1046, i64 32, 4, 0
  %1048 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1047, i64 16, 3, 1
  %1049 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1048, i64 1, 4, 1
  %1050 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 0
  %1051 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %1052 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1050, 0
  %1053 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1052, ptr %1051, 1
  %1054 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1053, i64 0, 2
  %1055 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1054, i64 4, 3, 0
  %1056 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1055, i64 16, 4, 0
  %1057 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1056, i64 16, 3, 1
  %1058 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1057, i64 1, 4, 1
  %1059 = call ptr @llvm.stacksave.p0()
  %1060 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1049, ptr %1060, align 8
  %1061 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1060, 1
  %1062 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1058, ptr %1062, align 8
  %1063 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1062, 1
  %1064 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1061, ptr %1064, align 8
  %1065 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1063, ptr %1065, align 8
  call void @memrefCopy(i64 4, ptr %1064, ptr %1065)
  call void @llvm.stackrestore.p0(ptr %1059)
  %1066 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %1067 = ptrtoint ptr %1066 to i64
  %1068 = inttoptr i64 %1067 to ptr
  %1069 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %1070 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %1071 = insertvalue { ptr, ptr, i64 } poison, ptr %1069, 0
  %1072 = insertvalue { ptr, ptr, i64 } %1071, ptr %1070, 1
  %1073 = insertvalue { ptr, ptr, i64 } %1072, i64 0, 2
  %1074 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %1075 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %1076 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %1077 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %1078 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %1079 = extractvalue { ptr, ptr, i64 } %1073, 0
  %1080 = extractvalue { ptr, ptr, i64 } %1073, 1
  %1081 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1079, 0
  %1082 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1081, ptr %1080, 1
  %1083 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1082, i64 16, 2
  %1084 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1083, i64 16, 3, 0
  %1085 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1084, i64 32, 4, 0
  %1086 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1085, i64 4, 3, 1
  %1087 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1086, i64 1, 4, 1
  %1088 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 0
  %1089 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %1090 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1088, 0
  %1091 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1090, ptr %1089, 1
  %1092 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1091, i64 0, 2
  %1093 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1092, i64 16, 3, 0
  %1094 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1093, i64 4, 4, 0
  %1095 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1094, i64 4, 3, 1
  %1096 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1095, i64 1, 4, 1
  %1097 = call ptr @llvm.stacksave.p0()
  %1098 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1087, ptr %1098, align 8
  %1099 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1098, 1
  %1100 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1096, ptr %1100, align 8
  %1101 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1100, 1
  %1102 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1099, ptr %1102, align 8
  %1103 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1101, ptr %1103, align 8
  call void @memrefCopy(i64 4, ptr %1102, ptr %1103)
  call void @llvm.stackrestore.p0(ptr %1097)
  %1104 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %1105 = ptrtoint ptr %1104 to i64
  %1106 = inttoptr i64 %1105 to ptr
  %1107 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1068, ptr %1106, ptr %1030, i32 1, i32 0, i32 117)
  %1108 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %1109 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %1110 = insertvalue { ptr, ptr, i64 } poison, ptr %1108, 0
  %1111 = insertvalue { ptr, ptr, i64 } %1110, ptr %1109, 1
  %1112 = insertvalue { ptr, ptr, i64 } %1111, i64 0, 2
  %1113 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %1114 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %1115 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %1116 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %1117 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %1118 = extractvalue { ptr, ptr, i64 } %1112, 0
  %1119 = extractvalue { ptr, ptr, i64 } %1112, 1
  %1120 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1118, 0
  %1121 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1120, ptr %1119, 1
  %1122 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1121, i64 528, 2
  %1123 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1122, i64 4, 3, 0
  %1124 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1123, i64 32, 4, 0
  %1125 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1124, i64 16, 3, 1
  %1126 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1125, i64 1, 4, 1
  %1127 = call ptr @llvm.stacksave.p0()
  %1128 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1126, ptr %1128, align 8
  %1129 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1128, 1
  %1130 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1058, ptr %1130, align 8
  %1131 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1130, 1
  %1132 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1129, ptr %1132, align 8
  %1133 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1131, ptr %1133, align 8
  call void @memrefCopy(i64 4, ptr %1132, ptr %1133)
  call void @llvm.stackrestore.p0(ptr %1127)
  %1134 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %1135 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %1136 = insertvalue { ptr, ptr, i64 } poison, ptr %1134, 0
  %1137 = insertvalue { ptr, ptr, i64 } %1136, ptr %1135, 1
  %1138 = insertvalue { ptr, ptr, i64 } %1137, i64 0, 2
  %1139 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %1140 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %1141 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %1142 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %1143 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %1144 = extractvalue { ptr, ptr, i64 } %1138, 0
  %1145 = extractvalue { ptr, ptr, i64 } %1138, 1
  %1146 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1144, 0
  %1147 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1146, ptr %1145, 1
  %1148 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1147, i64 528, 2
  %1149 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1148, i64 16, 3, 0
  %1150 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1149, i64 32, 4, 0
  %1151 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1150, i64 4, 3, 1
  %1152 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1151, i64 1, 4, 1
  %1153 = call ptr @llvm.stacksave.p0()
  %1154 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1152, ptr %1154, align 8
  %1155 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1154, 1
  %1156 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1096, ptr %1156, align 8
  %1157 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1156, 1
  %1158 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1155, ptr %1158, align 8
  %1159 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1157, ptr %1159, align 8
  call void @memrefCopy(i64 4, ptr %1158, ptr %1159)
  call void @llvm.stackrestore.p0(ptr %1153)
  %1160 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1068, ptr %1106, ptr %1030, i32 1, i32 42, i32 117)
  %1161 = call ptr @llvm.stacksave.p0()
  %1162 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1020, ptr %1162, align 8
  %1163 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1162, 1
  %1164 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1011, ptr %1164, align 8
  %1165 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1164, 1
  %1166 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1163, ptr %1166, align 8
  %1167 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1165, ptr %1167, align 8
  call void @memrefCopy(i64 4, ptr %1166, ptr %1167)
  call void @llvm.stackrestore.p0(ptr %1161)
  %1168 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %1169 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %1170 = insertvalue { ptr, ptr, i64 } poison, ptr %1168, 0
  %1171 = insertvalue { ptr, ptr, i64 } %1170, ptr %1169, 1
  %1172 = insertvalue { ptr, ptr, i64 } %1171, i64 0, 2
  %1173 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %1174 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %1175 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %1176 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %1177 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %1178 = extractvalue { ptr, ptr, i64 } %1172, 0
  %1179 = extractvalue { ptr, ptr, i64 } %1172, 1
  %1180 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1178, 0
  %1181 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1180, ptr %1179, 1
  %1182 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1181, i64 532, 2
  %1183 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1182, i64 4, 3, 0
  %1184 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1183, i64 32, 4, 0
  %1185 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1184, i64 4, 3, 1
  %1186 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1185, i64 1, 4, 1
  %1187 = call ptr @llvm.stacksave.p0()
  %1188 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1186, ptr %1188, align 8
  %1189 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1188, 1
  %1190 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1020, ptr %1190, align 8
  %1191 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1190, 1
  %1192 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1189, ptr %1192, align 8
  %1193 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1191, ptr %1193, align 8
  call void @memrefCopy(i64 4, ptr %1192, ptr %1193)
  call void @llvm.stackrestore.p0(ptr %1187)
  %1194 = call ptr @llvm.stacksave.p0()
  %1195 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1049, ptr %1195, align 8
  %1196 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1195, 1
  %1197 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1058, ptr %1197, align 8
  %1198 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1197, 1
  %1199 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1196, ptr %1199, align 8
  %1200 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1198, ptr %1200, align 8
  call void @memrefCopy(i64 4, ptr %1199, ptr %1200)
  call void @llvm.stackrestore.p0(ptr %1194)
  %1201 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %1202 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %1203 = insertvalue { ptr, ptr, i64 } poison, ptr %1201, 0
  %1204 = insertvalue { ptr, ptr, i64 } %1203, ptr %1202, 1
  %1205 = insertvalue { ptr, ptr, i64 } %1204, i64 0, 2
  %1206 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %1207 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %1208 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %1209 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %1210 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %1211 = extractvalue { ptr, ptr, i64 } %1205, 0
  %1212 = extractvalue { ptr, ptr, i64 } %1205, 1
  %1213 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1211, 0
  %1214 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1213, ptr %1212, 1
  %1215 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1214, i64 20, 2
  %1216 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1215, i64 16, 3, 0
  %1217 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1216, i64 32, 4, 0
  %1218 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1217, i64 4, 3, 1
  %1219 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1218, i64 1, 4, 1
  %1220 = call ptr @llvm.stacksave.p0()
  %1221 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1219, ptr %1221, align 8
  %1222 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1221, 1
  %1223 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1096, ptr %1223, align 8
  %1224 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1223, 1
  %1225 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1222, ptr %1225, align 8
  %1226 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1224, ptr %1226, align 8
  call void @memrefCopy(i64 4, ptr %1225, ptr %1226)
  call void @llvm.stackrestore.p0(ptr %1220)
  %1227 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1068, ptr %1106, ptr %1030, i32 2, i32 0, i32 117)
  %1228 = call ptr @llvm.stacksave.p0()
  %1229 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1126, ptr %1229, align 8
  %1230 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1229, 1
  %1231 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1058, ptr %1231, align 8
  %1232 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1231, 1
  %1233 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1230, ptr %1233, align 8
  %1234 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1232, ptr %1234, align 8
  call void @memrefCopy(i64 4, ptr %1233, ptr %1234)
  call void @llvm.stackrestore.p0(ptr %1228)
  %1235 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %1236 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %1237 = insertvalue { ptr, ptr, i64 } poison, ptr %1235, 0
  %1238 = insertvalue { ptr, ptr, i64 } %1237, ptr %1236, 1
  %1239 = insertvalue { ptr, ptr, i64 } %1238, i64 0, 2
  %1240 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %1241 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %1242 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %1243 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %1244 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %1245 = extractvalue { ptr, ptr, i64 } %1239, 0
  %1246 = extractvalue { ptr, ptr, i64 } %1239, 1
  %1247 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1245, 0
  %1248 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1247, ptr %1246, 1
  %1249 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1248, i64 532, 2
  %1250 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1249, i64 16, 3, 0
  %1251 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1250, i64 32, 4, 0
  %1252 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1251, i64 4, 3, 1
  %1253 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1252, i64 1, 4, 1
  %1254 = call ptr @llvm.stacksave.p0()
  %1255 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1253, ptr %1255, align 8
  %1256 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1255, 1
  %1257 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1096, ptr %1257, align 8
  %1258 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1257, 1
  %1259 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1256, ptr %1259, align 8
  %1260 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1258, ptr %1260, align 8
  call void @memrefCopy(i64 4, ptr %1259, ptr %1260)
  call void @llvm.stackrestore.p0(ptr %1254)
  %1261 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1068, ptr %1106, ptr %1030, i32 2, i32 42, i32 117)
  %1262 = call ptr @llvm.stacksave.p0()
  %1263 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1020, ptr %1263, align 8
  %1264 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1263, 1
  %1265 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1186, ptr %1265, align 8
  %1266 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1265, 1
  %1267 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1264, ptr %1267, align 8
  %1268 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1266, ptr %1268, align 8
  call void @memrefCopy(i64 4, ptr %1267, ptr %1268)
  call void @llvm.stackrestore.p0(ptr %1262)
  %1269 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %1270 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %1271 = insertvalue { ptr, ptr, i64 } poison, ptr %1269, 0
  %1272 = insertvalue { ptr, ptr, i64 } %1271, ptr %1270, 1
  %1273 = insertvalue { ptr, ptr, i64 } %1272, i64 0, 2
  %1274 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %1275 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %1276 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %1277 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %1278 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %1279 = extractvalue { ptr, ptr, i64 } %1273, 0
  %1280 = extractvalue { ptr, ptr, i64 } %1273, 1
  %1281 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1279, 0
  %1282 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1281, ptr %1280, 1
  %1283 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1282, i64 536, 2
  %1284 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1283, i64 4, 3, 0
  %1285 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1284, i64 32, 4, 0
  %1286 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1285, i64 4, 3, 1
  %1287 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1286, i64 1, 4, 1
  %1288 = call ptr @llvm.stacksave.p0()
  %1289 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1287, ptr %1289, align 8
  %1290 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1289, 1
  %1291 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1020, ptr %1291, align 8
  %1292 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1291, 1
  %1293 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1290, ptr %1293, align 8
  %1294 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1292, ptr %1294, align 8
  call void @memrefCopy(i64 4, ptr %1293, ptr %1294)
  call void @llvm.stackrestore.p0(ptr %1288)
  %1295 = call ptr @llvm.stacksave.p0()
  %1296 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1049, ptr %1296, align 8
  %1297 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1296, 1
  %1298 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1058, ptr %1298, align 8
  %1299 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1298, 1
  %1300 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1297, ptr %1300, align 8
  %1301 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1299, ptr %1301, align 8
  call void @memrefCopy(i64 4, ptr %1300, ptr %1301)
  call void @llvm.stackrestore.p0(ptr %1295)
  %1302 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %1303 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %1304 = insertvalue { ptr, ptr, i64 } poison, ptr %1302, 0
  %1305 = insertvalue { ptr, ptr, i64 } %1304, ptr %1303, 1
  %1306 = insertvalue { ptr, ptr, i64 } %1305, i64 0, 2
  %1307 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %1308 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %1309 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %1310 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %1311 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %1312 = extractvalue { ptr, ptr, i64 } %1306, 0
  %1313 = extractvalue { ptr, ptr, i64 } %1306, 1
  %1314 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1312, 0
  %1315 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1314, ptr %1313, 1
  %1316 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1315, i64 24, 2
  %1317 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1316, i64 16, 3, 0
  %1318 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1317, i64 32, 4, 0
  %1319 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1318, i64 4, 3, 1
  %1320 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1319, i64 1, 4, 1
  %1321 = call ptr @llvm.stacksave.p0()
  %1322 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1320, ptr %1322, align 8
  %1323 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1322, 1
  %1324 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1096, ptr %1324, align 8
  %1325 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1324, 1
  %1326 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1323, ptr %1326, align 8
  %1327 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1325, ptr %1327, align 8
  call void @memrefCopy(i64 4, ptr %1326, ptr %1327)
  call void @llvm.stackrestore.p0(ptr %1321)
  %1328 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1068, ptr %1106, ptr %1030, i32 3, i32 0, i32 117)
  %1329 = call ptr @llvm.stacksave.p0()
  %1330 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1126, ptr %1330, align 8
  %1331 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1330, 1
  %1332 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1058, ptr %1332, align 8
  %1333 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1332, 1
  %1334 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1331, ptr %1334, align 8
  %1335 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1333, ptr %1335, align 8
  call void @memrefCopy(i64 4, ptr %1334, ptr %1335)
  call void @llvm.stackrestore.p0(ptr %1329)
  %1336 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %1337 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %1338 = insertvalue { ptr, ptr, i64 } poison, ptr %1336, 0
  %1339 = insertvalue { ptr, ptr, i64 } %1338, ptr %1337, 1
  %1340 = insertvalue { ptr, ptr, i64 } %1339, i64 0, 2
  %1341 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %1342 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %1343 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %1344 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %1345 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %1346 = extractvalue { ptr, ptr, i64 } %1340, 0
  %1347 = extractvalue { ptr, ptr, i64 } %1340, 1
  %1348 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1346, 0
  %1349 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1348, ptr %1347, 1
  %1350 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1349, i64 536, 2
  %1351 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1350, i64 16, 3, 0
  %1352 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1351, i64 32, 4, 0
  %1353 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1352, i64 4, 3, 1
  %1354 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1353, i64 1, 4, 1
  %1355 = call ptr @llvm.stacksave.p0()
  %1356 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1354, ptr %1356, align 8
  %1357 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1356, 1
  %1358 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1096, ptr %1358, align 8
  %1359 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1358, 1
  %1360 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1357, ptr %1360, align 8
  %1361 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1359, ptr %1361, align 8
  call void @memrefCopy(i64 4, ptr %1360, ptr %1361)
  call void @llvm.stackrestore.p0(ptr %1355)
  %1362 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1068, ptr %1106, ptr %1030, i32 3, i32 42, i32 117)
  %1363 = call ptr @llvm.stacksave.p0()
  %1364 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1020, ptr %1364, align 8
  %1365 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1364, 1
  %1366 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1287, ptr %1366, align 8
  %1367 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1366, 1
  %1368 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1365, ptr %1368, align 8
  %1369 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1367, ptr %1369, align 8
  call void @memrefCopy(i64 4, ptr %1368, ptr %1369)
  call void @llvm.stackrestore.p0(ptr %1363)
  %1370 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %1371 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %1372 = insertvalue { ptr, ptr, i64 } poison, ptr %1370, 0
  %1373 = insertvalue { ptr, ptr, i64 } %1372, ptr %1371, 1
  %1374 = insertvalue { ptr, ptr, i64 } %1373, i64 0, 2
  %1375 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %1376 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %1377 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %1378 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %1379 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %1380 = extractvalue { ptr, ptr, i64 } %1374, 0
  %1381 = extractvalue { ptr, ptr, i64 } %1374, 1
  %1382 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1380, 0
  %1383 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1382, ptr %1381, 1
  %1384 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1383, i64 540, 2
  %1385 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1384, i64 4, 3, 0
  %1386 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1385, i64 32, 4, 0
  %1387 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1386, i64 4, 3, 1
  %1388 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1387, i64 1, 4, 1
  %1389 = call ptr @llvm.stacksave.p0()
  %1390 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1388, ptr %1390, align 8
  %1391 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1390, 1
  %1392 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1020, ptr %1392, align 8
  %1393 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1392, 1
  %1394 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1391, ptr %1394, align 8
  %1395 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1393, ptr %1395, align 8
  call void @memrefCopy(i64 4, ptr %1394, ptr %1395)
  call void @llvm.stackrestore.p0(ptr %1389)
  %1396 = call ptr @llvm.stacksave.p0()
  %1397 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1049, ptr %1397, align 8
  %1398 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1397, 1
  %1399 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1058, ptr %1399, align 8
  %1400 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1399, 1
  %1401 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1398, ptr %1401, align 8
  %1402 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1400, ptr %1402, align 8
  call void @memrefCopy(i64 4, ptr %1401, ptr %1402)
  call void @llvm.stackrestore.p0(ptr %1396)
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
  %1417 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1416, i64 28, 2
  %1418 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1417, i64 16, 3, 0
  %1419 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1418, i64 32, 4, 0
  %1420 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1419, i64 4, 3, 1
  %1421 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1420, i64 1, 4, 1
  %1422 = call ptr @llvm.stacksave.p0()
  %1423 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1421, ptr %1423, align 8
  %1424 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1423, 1
  %1425 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1096, ptr %1425, align 8
  %1426 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1425, 1
  %1427 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1424, ptr %1427, align 8
  %1428 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1426, ptr %1428, align 8
  call void @memrefCopy(i64 4, ptr %1427, ptr %1428)
  call void @llvm.stackrestore.p0(ptr %1422)
  %1429 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1068, ptr %1106, ptr %1030, i32 1, i32 120, i32 117)
  %1430 = call ptr @llvm.stacksave.p0()
  %1431 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1126, ptr %1431, align 8
  %1432 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1431, 1
  %1433 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1058, ptr %1433, align 8
  %1434 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1433, 1
  %1435 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1432, ptr %1435, align 8
  %1436 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1434, ptr %1436, align 8
  call void @memrefCopy(i64 4, ptr %1435, ptr %1436)
  call void @llvm.stackrestore.p0(ptr %1430)
  %1437 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %1438 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %1439 = insertvalue { ptr, ptr, i64 } poison, ptr %1437, 0
  %1440 = insertvalue { ptr, ptr, i64 } %1439, ptr %1438, 1
  %1441 = insertvalue { ptr, ptr, i64 } %1440, i64 0, 2
  %1442 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %1443 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %1444 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %1445 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %1446 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %1447 = extractvalue { ptr, ptr, i64 } %1441, 0
  %1448 = extractvalue { ptr, ptr, i64 } %1441, 1
  %1449 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1447, 0
  %1450 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1449, ptr %1448, 1
  %1451 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1450, i64 540, 2
  %1452 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1451, i64 16, 3, 0
  %1453 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1452, i64 32, 4, 0
  %1454 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1453, i64 4, 3, 1
  %1455 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1454, i64 1, 4, 1
  %1456 = call ptr @llvm.stacksave.p0()
  %1457 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1455, ptr %1457, align 8
  %1458 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1457, 1
  %1459 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1096, ptr %1459, align 8
  %1460 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1459, 1
  %1461 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1458, ptr %1461, align 8
  %1462 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1460, ptr %1462, align 8
  call void @memrefCopy(i64 4, ptr %1461, ptr %1462)
  call void @llvm.stackrestore.p0(ptr %1456)
  %1463 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1068, ptr %1106, ptr %1030, i32 1, i32 189, i32 117)
  %1464 = call ptr @llvm.stacksave.p0()
  %1465 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1020, ptr %1465, align 8
  %1466 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1465, 1
  %1467 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1388, ptr %1467, align 8
  %1468 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1467, 1
  %1469 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1466, ptr %1469, align 8
  %1470 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1468, ptr %1470, align 8
  call void @memrefCopy(i64 4, ptr %1469, ptr %1470)
  call void @llvm.stackrestore.p0(ptr %1464)
  %1471 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %1472 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %1473 = insertvalue { ptr, ptr, i64 } poison, ptr %1471, 0
  %1474 = insertvalue { ptr, ptr, i64 } %1473, ptr %1472, 1
  %1475 = insertvalue { ptr, ptr, i64 } %1474, i64 0, 2
  %1476 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %1477 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %1478 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %1479 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %1480 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %1481 = extractvalue { ptr, ptr, i64 } %1475, 0
  %1482 = extractvalue { ptr, ptr, i64 } %1475, 1
  %1483 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1481, 0
  %1484 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1483, ptr %1482, 1
  %1485 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1484, i64 656, 2
  %1486 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1485, i64 4, 3, 0
  %1487 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1486, i64 32, 4, 0
  %1488 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1487, i64 4, 3, 1
  %1489 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1488, i64 1, 4, 1
  %1490 = call ptr @llvm.stacksave.p0()
  %1491 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1489, ptr %1491, align 8
  %1492 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1491, 1
  %1493 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1020, ptr %1493, align 8
  %1494 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1493, 1
  %1495 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1492, ptr %1495, align 8
  %1496 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1494, ptr %1496, align 8
  call void @memrefCopy(i64 4, ptr %1495, ptr %1496)
  call void @llvm.stackrestore.p0(ptr %1490)
  %1497 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %1498 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %1499 = insertvalue { ptr, ptr, i64 } poison, ptr %1497, 0
  %1500 = insertvalue { ptr, ptr, i64 } %1499, ptr %1498, 1
  %1501 = insertvalue { ptr, ptr, i64 } %1500, i64 0, 2
  %1502 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %1503 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %1504 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %1505 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %1506 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %1507 = extractvalue { ptr, ptr, i64 } %1501, 0
  %1508 = extractvalue { ptr, ptr, i64 } %1501, 1
  %1509 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1507, 0
  %1510 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1509, ptr %1508, 1
  %1511 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1510, i64 640, 2
  %1512 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1511, i64 4, 3, 0
  %1513 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1512, i64 32, 4, 0
  %1514 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1513, i64 16, 3, 1
  %1515 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1514, i64 1, 4, 1
  %1516 = call ptr @llvm.stacksave.p0()
  %1517 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1515, ptr %1517, align 8
  %1518 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1517, 1
  %1519 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1058, ptr %1519, align 8
  %1520 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1519, 1
  %1521 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1518, ptr %1521, align 8
  %1522 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1520, ptr %1522, align 8
  call void @memrefCopy(i64 4, ptr %1521, ptr %1522)
  call void @llvm.stackrestore.p0(ptr %1516)
  %1523 = call ptr @llvm.stacksave.p0()
  %1524 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1087, ptr %1524, align 8
  %1525 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1524, 1
  %1526 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1096, ptr %1526, align 8
  %1527 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1526, 1
  %1528 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1525, ptr %1528, align 8
  %1529 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1527, ptr %1529, align 8
  call void @memrefCopy(i64 4, ptr %1528, ptr %1529)
  call void @llvm.stackrestore.p0(ptr %1523)
  %1530 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1068, ptr %1106, ptr %1030, i32 2, i32 120, i32 117)
  %1531 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %1532 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %1533 = insertvalue { ptr, ptr, i64 } poison, ptr %1531, 0
  %1534 = insertvalue { ptr, ptr, i64 } %1533, ptr %1532, 1
  %1535 = insertvalue { ptr, ptr, i64 } %1534, i64 0, 2
  %1536 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %1537 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %1538 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %1539 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %1540 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %1541 = extractvalue { ptr, ptr, i64 } %1535, 0
  %1542 = extractvalue { ptr, ptr, i64 } %1535, 1
  %1543 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1541, 0
  %1544 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1543, ptr %1542, 1
  %1545 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1544, i64 656, 2
  %1546 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1545, i64 4, 3, 0
  %1547 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1546, i64 32, 4, 0
  %1548 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1547, i64 16, 3, 1
  %1549 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1548, i64 1, 4, 1
  %1550 = call ptr @llvm.stacksave.p0()
  %1551 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1549, ptr %1551, align 8
  %1552 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1551, 1
  %1553 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1058, ptr %1553, align 8
  %1554 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1553, 1
  %1555 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1552, ptr %1555, align 8
  %1556 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1554, ptr %1556, align 8
  call void @memrefCopy(i64 4, ptr %1555, ptr %1556)
  call void @llvm.stackrestore.p0(ptr %1550)
  %1557 = call ptr @llvm.stacksave.p0()
  %1558 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1152, ptr %1558, align 8
  %1559 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1558, 1
  %1560 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1096, ptr %1560, align 8
  %1561 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1560, 1
  %1562 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1559, ptr %1562, align 8
  %1563 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1561, ptr %1563, align 8
  call void @memrefCopy(i64 4, ptr %1562, ptr %1563)
  call void @llvm.stackrestore.p0(ptr %1557)
  %1564 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1068, ptr %1106, ptr %1030, i32 2, i32 189, i32 117)
  %1565 = call ptr @llvm.stacksave.p0()
  %1566 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1020, ptr %1566, align 8
  %1567 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1566, 1
  %1568 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1489, ptr %1568, align 8
  %1569 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1568, 1
  %1570 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1567, ptr %1570, align 8
  %1571 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1569, ptr %1571, align 8
  call void @memrefCopy(i64 4, ptr %1570, ptr %1571)
  call void @llvm.stackrestore.p0(ptr %1565)
  %1572 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %1573 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %1574 = insertvalue { ptr, ptr, i64 } poison, ptr %1572, 0
  %1575 = insertvalue { ptr, ptr, i64 } %1574, ptr %1573, 1
  %1576 = insertvalue { ptr, ptr, i64 } %1575, i64 0, 2
  %1577 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %1578 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %1579 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %1580 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %1581 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %1582 = extractvalue { ptr, ptr, i64 } %1576, 0
  %1583 = extractvalue { ptr, ptr, i64 } %1576, 1
  %1584 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1582, 0
  %1585 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1584, ptr %1583, 1
  %1586 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1585, i64 660, 2
  %1587 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1586, i64 4, 3, 0
  %1588 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1587, i64 32, 4, 0
  %1589 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1588, i64 4, 3, 1
  %1590 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1589, i64 1, 4, 1
  %1591 = call ptr @llvm.stacksave.p0()
  %1592 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1590, ptr %1592, align 8
  %1593 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1592, 1
  %1594 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1020, ptr %1594, align 8
  %1595 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1594, 1
  %1596 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1593, ptr %1596, align 8
  %1597 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1595, ptr %1597, align 8
  call void @memrefCopy(i64 4, ptr %1596, ptr %1597)
  call void @llvm.stackrestore.p0(ptr %1591)
  %1598 = call ptr @llvm.stacksave.p0()
  %1599 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1515, ptr %1599, align 8
  %1600 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1599, 1
  %1601 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1058, ptr %1601, align 8
  %1602 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1601, 1
  %1603 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1600, ptr %1603, align 8
  %1604 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1602, ptr %1604, align 8
  call void @memrefCopy(i64 4, ptr %1603, ptr %1604)
  call void @llvm.stackrestore.p0(ptr %1598)
  %1605 = call ptr @llvm.stacksave.p0()
  %1606 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1219, ptr %1606, align 8
  %1607 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1606, 1
  %1608 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1096, ptr %1608, align 8
  %1609 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1608, 1
  %1610 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1607, ptr %1610, align 8
  %1611 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1609, ptr %1611, align 8
  call void @memrefCopy(i64 4, ptr %1610, ptr %1611)
  call void @llvm.stackrestore.p0(ptr %1605)
  %1612 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1068, ptr %1106, ptr %1030, i32 3, i32 120, i32 117)
  %1613 = call ptr @llvm.stacksave.p0()
  %1614 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1549, ptr %1614, align 8
  %1615 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1614, 1
  %1616 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1058, ptr %1616, align 8
  %1617 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1616, 1
  %1618 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1615, ptr %1618, align 8
  %1619 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1617, ptr %1619, align 8
  call void @memrefCopy(i64 4, ptr %1618, ptr %1619)
  call void @llvm.stackrestore.p0(ptr %1613)
  %1620 = call ptr @llvm.stacksave.p0()
  %1621 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1253, ptr %1621, align 8
  %1622 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1621, 1
  %1623 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1096, ptr %1623, align 8
  %1624 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1623, 1
  %1625 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1622, ptr %1625, align 8
  %1626 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1624, ptr %1626, align 8
  call void @memrefCopy(i64 4, ptr %1625, ptr %1626)
  call void @llvm.stackrestore.p0(ptr %1620)
  %1627 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1068, ptr %1106, ptr %1030, i32 3, i32 189, i32 117)
  %1628 = call ptr @llvm.stacksave.p0()
  %1629 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1020, ptr %1629, align 8
  %1630 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1629, 1
  %1631 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1590, ptr %1631, align 8
  %1632 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1631, 1
  %1633 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1630, ptr %1633, align 8
  %1634 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1632, ptr %1634, align 8
  call void @memrefCopy(i64 4, ptr %1633, ptr %1634)
  call void @llvm.stackrestore.p0(ptr %1628)
  %1635 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %1636 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %1637 = insertvalue { ptr, ptr, i64 } poison, ptr %1635, 0
  %1638 = insertvalue { ptr, ptr, i64 } %1637, ptr %1636, 1
  %1639 = insertvalue { ptr, ptr, i64 } %1638, i64 0, 2
  %1640 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %1641 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %1642 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %1643 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %1644 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %1645 = extractvalue { ptr, ptr, i64 } %1639, 0
  %1646 = extractvalue { ptr, ptr, i64 } %1639, 1
  %1647 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1645, 0
  %1648 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1647, ptr %1646, 1
  %1649 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1648, i64 664, 2
  %1650 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1649, i64 4, 3, 0
  %1651 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1650, i64 32, 4, 0
  %1652 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1651, i64 4, 3, 1
  %1653 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1652, i64 1, 4, 1
  %1654 = call ptr @llvm.stacksave.p0()
  %1655 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1653, ptr %1655, align 8
  %1656 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1655, 1
  %1657 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1020, ptr %1657, align 8
  %1658 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1657, 1
  %1659 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1656, ptr %1659, align 8
  %1660 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1658, ptr %1660, align 8
  call void @memrefCopy(i64 4, ptr %1659, ptr %1660)
  call void @llvm.stackrestore.p0(ptr %1654)
  %1661 = call ptr @llvm.stacksave.p0()
  %1662 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1515, ptr %1662, align 8
  %1663 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1662, 1
  %1664 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1058, ptr %1664, align 8
  %1665 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1664, 1
  %1666 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1663, ptr %1666, align 8
  %1667 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1665, ptr %1667, align 8
  call void @memrefCopy(i64 4, ptr %1666, ptr %1667)
  call void @llvm.stackrestore.p0(ptr %1661)
  %1668 = call ptr @llvm.stacksave.p0()
  %1669 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1320, ptr %1669, align 8
  %1670 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1669, 1
  %1671 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1096, ptr %1671, align 8
  %1672 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1671, 1
  %1673 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1670, ptr %1673, align 8
  %1674 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1672, ptr %1674, align 8
  call void @memrefCopy(i64 4, ptr %1673, ptr %1674)
  call void @llvm.stackrestore.p0(ptr %1668)
  %1675 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1068, ptr %1106, ptr %1030, i32 1, i32 258, i32 117)
  %1676 = call ptr @llvm.stacksave.p0()
  %1677 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1549, ptr %1677, align 8
  %1678 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1677, 1
  %1679 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1058, ptr %1679, align 8
  %1680 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1679, 1
  %1681 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1678, ptr %1681, align 8
  %1682 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1680, ptr %1682, align 8
  call void @memrefCopy(i64 4, ptr %1681, ptr %1682)
  call void @llvm.stackrestore.p0(ptr %1676)
  %1683 = call ptr @llvm.stacksave.p0()
  %1684 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1354, ptr %1684, align 8
  %1685 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1684, 1
  %1686 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1096, ptr %1686, align 8
  %1687 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1686, 1
  %1688 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1685, ptr %1688, align 8
  %1689 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1687, ptr %1689, align 8
  call void @memrefCopy(i64 4, ptr %1688, ptr %1689)
  call void @llvm.stackrestore.p0(ptr %1683)
  %1690 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1068, ptr %1106, ptr %1030, i32 1, i32 327, i32 117)
  %1691 = call ptr @llvm.stacksave.p0()
  %1692 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1020, ptr %1692, align 8
  %1693 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1692, 1
  %1694 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1653, ptr %1694, align 8
  %1695 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1694, 1
  %1696 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1693, ptr %1696, align 8
  %1697 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1695, ptr %1697, align 8
  call void @memrefCopy(i64 4, ptr %1696, ptr %1697)
  call void @llvm.stackrestore.p0(ptr %1691)
  %1698 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %1699 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %1700 = insertvalue { ptr, ptr, i64 } poison, ptr %1698, 0
  %1701 = insertvalue { ptr, ptr, i64 } %1700, ptr %1699, 1
  %1702 = insertvalue { ptr, ptr, i64 } %1701, i64 0, 2
  %1703 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %1704 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %1705 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %1706 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %1707 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %1708 = extractvalue { ptr, ptr, i64 } %1702, 0
  %1709 = extractvalue { ptr, ptr, i64 } %1702, 1
  %1710 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1708, 0
  %1711 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1710, ptr %1709, 1
  %1712 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1711, i64 668, 2
  %1713 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1712, i64 4, 3, 0
  %1714 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1713, i64 32, 4, 0
  %1715 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1714, i64 4, 3, 1
  %1716 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1715, i64 1, 4, 1
  %1717 = call ptr @llvm.stacksave.p0()
  %1718 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1716, ptr %1718, align 8
  %1719 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1718, 1
  %1720 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1020, ptr %1720, align 8
  %1721 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1720, 1
  %1722 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1719, ptr %1722, align 8
  %1723 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1721, ptr %1723, align 8
  call void @memrefCopy(i64 4, ptr %1722, ptr %1723)
  call void @llvm.stackrestore.p0(ptr %1717)
  %1724 = call ptr @llvm.stacksave.p0()
  %1725 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1515, ptr %1725, align 8
  %1726 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1725, 1
  %1727 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1058, ptr %1727, align 8
  %1728 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1727, 1
  %1729 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1726, ptr %1729, align 8
  %1730 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1728, ptr %1730, align 8
  call void @memrefCopy(i64 4, ptr %1729, ptr %1730)
  call void @llvm.stackrestore.p0(ptr %1724)
  %1731 = call ptr @llvm.stacksave.p0()
  %1732 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1421, ptr %1732, align 8
  %1733 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1732, 1
  %1734 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1096, ptr %1734, align 8
  %1735 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1734, 1
  %1736 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1733, ptr %1736, align 8
  %1737 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1735, ptr %1737, align 8
  call void @memrefCopy(i64 4, ptr %1736, ptr %1737)
  call void @llvm.stackrestore.p0(ptr %1731)
  %1738 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1068, ptr %1106, ptr %1030, i32 2, i32 258, i32 117)
  %1739 = call ptr @llvm.stacksave.p0()
  %1740 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1549, ptr %1740, align 8
  %1741 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1740, 1
  %1742 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1058, ptr %1742, align 8
  %1743 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1742, 1
  %1744 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1741, ptr %1744, align 8
  %1745 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1743, ptr %1745, align 8
  call void @memrefCopy(i64 4, ptr %1744, ptr %1745)
  call void @llvm.stackrestore.p0(ptr %1739)
  %1746 = call ptr @llvm.stacksave.p0()
  %1747 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1455, ptr %1747, align 8
  %1748 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1747, 1
  %1749 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1096, ptr %1749, align 8
  %1750 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1749, 1
  %1751 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1748, ptr %1751, align 8
  %1752 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1750, ptr %1752, align 8
  call void @memrefCopy(i64 4, ptr %1751, ptr %1752)
  call void @llvm.stackrestore.p0(ptr %1746)
  %1753 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1068, ptr %1106, ptr %1030, i32 2, i32 327, i32 117)
  %1754 = call ptr @llvm.stacksave.p0()
  %1755 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1020, ptr %1755, align 8
  %1756 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1755, 1
  %1757 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1716, ptr %1757, align 8
  %1758 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1757, 1
  %1759 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1756, ptr %1759, align 8
  %1760 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1758, ptr %1760, align 8
  call void @memrefCopy(i64 4, ptr %1759, ptr %1760)
  call void @llvm.stackrestore.p0(ptr %1754)
  %1761 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %1762 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %1763 = insertvalue { ptr, ptr, i64 } poison, ptr %1761, 0
  %1764 = insertvalue { ptr, ptr, i64 } %1763, ptr %1762, 1
  %1765 = insertvalue { ptr, ptr, i64 } %1764, i64 0, 2
  %1766 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %1767 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %1768 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %1769 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %1770 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %1771 = extractvalue { ptr, ptr, i64 } %1765, 0
  %1772 = extractvalue { ptr, ptr, i64 } %1765, 1
  %1773 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1771, 0
  %1774 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1773, ptr %1772, 1
  %1775 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1774, i64 768, 2
  %1776 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1775, i64 4, 3, 0
  %1777 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1776, i64 32, 4, 0
  %1778 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, i64 4, 3, 1
  %1779 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1778, i64 1, 4, 1
  %1780 = call ptr @llvm.stacksave.p0()
  %1781 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1779, ptr %1781, align 8
  %1782 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1781, 1
  %1783 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1020, ptr %1783, align 8
  %1784 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1783, 1
  %1785 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1782, ptr %1785, align 8
  %1786 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1784, ptr %1786, align 8
  call void @memrefCopy(i64 4, ptr %1785, ptr %1786)
  call void @llvm.stackrestore.p0(ptr %1780)
  %1787 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %1788 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %1789 = insertvalue { ptr, ptr, i64 } poison, ptr %1787, 0
  %1790 = insertvalue { ptr, ptr, i64 } %1789, ptr %1788, 1
  %1791 = insertvalue { ptr, ptr, i64 } %1790, i64 0, 2
  %1792 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %1793 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %1794 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %1795 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %1796 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %1797 = extractvalue { ptr, ptr, i64 } %1791, 0
  %1798 = extractvalue { ptr, ptr, i64 } %1791, 1
  %1799 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1797, 0
  %1800 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1799, ptr %1798, 1
  %1801 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1800, i64 768, 2
  %1802 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1801, i64 4, 3, 0
  %1803 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1802, i64 32, 4, 0
  %1804 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1803, i64 16, 3, 1
  %1805 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1804, i64 1, 4, 1
  %1806 = call ptr @llvm.stacksave.p0()
  %1807 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1805, ptr %1807, align 8
  %1808 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1807, 1
  %1809 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1058, ptr %1809, align 8
  %1810 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1809, 1
  %1811 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1808, ptr %1811, align 8
  %1812 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1810, ptr %1812, align 8
  call void @memrefCopy(i64 4, ptr %1811, ptr %1812)
  call void @llvm.stackrestore.p0(ptr %1806)
  %1813 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %1814 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %1815 = insertvalue { ptr, ptr, i64 } poison, ptr %1813, 0
  %1816 = insertvalue { ptr, ptr, i64 } %1815, ptr %1814, 1
  %1817 = insertvalue { ptr, ptr, i64 } %1816, i64 0, 2
  %1818 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %1819 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %1820 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %1821 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %1822 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %1823 = extractvalue { ptr, ptr, i64 } %1817, 0
  %1824 = extractvalue { ptr, ptr, i64 } %1817, 1
  %1825 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1823, 0
  %1826 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1825, ptr %1824, 1
  %1827 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1826, i64 0, 2
  %1828 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1827, i64 16, 3, 0
  %1829 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1828, i64 32, 4, 0
  %1830 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1829, i64 4, 3, 1
  %1831 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1830, i64 1, 4, 1
  %1832 = call ptr @llvm.stacksave.p0()
  %1833 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1831, ptr %1833, align 8
  %1834 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1833, 1
  %1835 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1096, ptr %1835, align 8
  %1836 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1835, 1
  %1837 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1834, ptr %1837, align 8
  %1838 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1836, ptr %1838, align 8
  call void @memrefCopy(i64 4, ptr %1837, ptr %1838)
  call void @llvm.stackrestore.p0(ptr %1832)
  %1839 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1068, ptr %1106, ptr %1030, i32 3, i32 258, i32 117)
  %1840 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %1841 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %1842 = insertvalue { ptr, ptr, i64 } poison, ptr %1840, 0
  %1843 = insertvalue { ptr, ptr, i64 } %1842, ptr %1841, 1
  %1844 = insertvalue { ptr, ptr, i64 } %1843, i64 0, 2
  %1845 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %1846 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %1847 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %1848 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %1849 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %1850 = extractvalue { ptr, ptr, i64 } %1844, 0
  %1851 = extractvalue { ptr, ptr, i64 } %1844, 1
  %1852 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1850, 0
  %1853 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1852, ptr %1851, 1
  %1854 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1853, i64 784, 2
  %1855 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1854, i64 4, 3, 0
  %1856 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1855, i64 32, 4, 0
  %1857 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1856, i64 16, 3, 1
  %1858 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1857, i64 1, 4, 1
  %1859 = call ptr @llvm.stacksave.p0()
  %1860 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1858, ptr %1860, align 8
  %1861 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1860, 1
  %1862 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1058, ptr %1862, align 8
  %1863 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1862, 1
  %1864 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1861, ptr %1864, align 8
  %1865 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1863, ptr %1865, align 8
  call void @memrefCopy(i64 4, ptr %1864, ptr %1865)
  call void @llvm.stackrestore.p0(ptr %1859)
  %1866 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %1867 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %1868 = insertvalue { ptr, ptr, i64 } poison, ptr %1866, 0
  %1869 = insertvalue { ptr, ptr, i64 } %1868, ptr %1867, 1
  %1870 = insertvalue { ptr, ptr, i64 } %1869, i64 0, 2
  %1871 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %1872 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %1873 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %1874 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %1875 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %1876 = extractvalue { ptr, ptr, i64 } %1870, 0
  %1877 = extractvalue { ptr, ptr, i64 } %1870, 1
  %1878 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1876, 0
  %1879 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1878, ptr %1877, 1
  %1880 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1879, i64 512, 2
  %1881 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1880, i64 16, 3, 0
  %1882 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1881, i64 32, 4, 0
  %1883 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1882, i64 4, 3, 1
  %1884 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1883, i64 1, 4, 1
  %1885 = call ptr @llvm.stacksave.p0()
  %1886 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1884, ptr %1886, align 8
  %1887 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1886, 1
  %1888 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1096, ptr %1888, align 8
  %1889 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1888, 1
  %1890 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1887, ptr %1890, align 8
  %1891 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1889, ptr %1891, align 8
  call void @memrefCopy(i64 4, ptr %1890, ptr %1891)
  call void @llvm.stackrestore.p0(ptr %1885)
  %1892 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1068, ptr %1106, ptr %1030, i32 3, i32 327, i32 117)
  %1893 = call ptr @llvm.stacksave.p0()
  %1894 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1020, ptr %1894, align 8
  %1895 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1894, 1
  %1896 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1779, ptr %1896, align 8
  %1897 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1896, 1
  %1898 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1895, ptr %1898, align 8
  %1899 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1897, ptr %1899, align 8
  call void @memrefCopy(i64 4, ptr %1898, ptr %1899)
  call void @llvm.stackrestore.p0(ptr %1893)
  %1900 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %1901 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %1902 = insertvalue { ptr, ptr, i64 } poison, ptr %1900, 0
  %1903 = insertvalue { ptr, ptr, i64 } %1902, ptr %1901, 1
  %1904 = insertvalue { ptr, ptr, i64 } %1903, i64 0, 2
  %1905 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %1906 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %1907 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %1908 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %1909 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %1910 = extractvalue { ptr, ptr, i64 } %1904, 0
  %1911 = extractvalue { ptr, ptr, i64 } %1904, 1
  %1912 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1910, 0
  %1913 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1912, ptr %1911, 1
  %1914 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1913, i64 772, 2
  %1915 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1914, i64 4, 3, 0
  %1916 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1915, i64 32, 4, 0
  %1917 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1916, i64 4, 3, 1
  %1918 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1917, i64 1, 4, 1
  %1919 = call ptr @llvm.stacksave.p0()
  %1920 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1918, ptr %1920, align 8
  %1921 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1920, 1
  %1922 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1020, ptr %1922, align 8
  %1923 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1922, 1
  %1924 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1921, ptr %1924, align 8
  %1925 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1923, ptr %1925, align 8
  call void @memrefCopy(i64 4, ptr %1924, ptr %1925)
  call void @llvm.stackrestore.p0(ptr %1919)
  %1926 = call ptr @llvm.stacksave.p0()
  %1927 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1805, ptr %1927, align 8
  %1928 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1927, 1
  %1929 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1058, ptr %1929, align 8
  %1930 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1929, 1
  %1931 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1928, ptr %1931, align 8
  %1932 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1930, ptr %1932, align 8
  call void @memrefCopy(i64 4, ptr %1931, ptr %1932)
  call void @llvm.stackrestore.p0(ptr %1926)
  %1933 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %1934 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %1935 = insertvalue { ptr, ptr, i64 } poison, ptr %1933, 0
  %1936 = insertvalue { ptr, ptr, i64 } %1935, ptr %1934, 1
  %1937 = insertvalue { ptr, ptr, i64 } %1936, i64 0, 2
  %1938 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %1939 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %1940 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %1941 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %1942 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %1943 = extractvalue { ptr, ptr, i64 } %1937, 0
  %1944 = extractvalue { ptr, ptr, i64 } %1937, 1
  %1945 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1943, 0
  %1946 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1945, ptr %1944, 1
  %1947 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1946, i64 4, 2
  %1948 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1947, i64 16, 3, 0
  %1949 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1948, i64 32, 4, 0
  %1950 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1949, i64 4, 3, 1
  %1951 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1950, i64 1, 4, 1
  %1952 = call ptr @llvm.stacksave.p0()
  %1953 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1951, ptr %1953, align 8
  %1954 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1953, 1
  %1955 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1096, ptr %1955, align 8
  %1956 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1955, 1
  %1957 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1954, ptr %1957, align 8
  %1958 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1956, ptr %1958, align 8
  call void @memrefCopy(i64 4, ptr %1957, ptr %1958)
  call void @llvm.stackrestore.p0(ptr %1952)
  %1959 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1068, ptr %1106, ptr %1030, i32 1, i32 396, i32 117)
  %1960 = call ptr @llvm.stacksave.p0()
  %1961 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1858, ptr %1961, align 8
  %1962 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1961, 1
  %1963 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1058, ptr %1963, align 8
  %1964 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1963, 1
  %1965 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1962, ptr %1965, align 8
  %1966 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1964, ptr %1966, align 8
  call void @memrefCopy(i64 4, ptr %1965, ptr %1966)
  call void @llvm.stackrestore.p0(ptr %1960)
  %1967 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %1968 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %1969 = insertvalue { ptr, ptr, i64 } poison, ptr %1967, 0
  %1970 = insertvalue { ptr, ptr, i64 } %1969, ptr %1968, 1
  %1971 = insertvalue { ptr, ptr, i64 } %1970, i64 0, 2
  %1972 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %1973 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %1974 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %1975 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %1976 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %1977 = extractvalue { ptr, ptr, i64 } %1971, 0
  %1978 = extractvalue { ptr, ptr, i64 } %1971, 1
  %1979 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1977, 0
  %1980 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1979, ptr %1978, 1
  %1981 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1980, i64 516, 2
  %1982 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1981, i64 16, 3, 0
  %1983 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1982, i64 32, 4, 0
  %1984 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1983, i64 4, 3, 1
  %1985 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1984, i64 1, 4, 1
  %1986 = call ptr @llvm.stacksave.p0()
  %1987 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1985, ptr %1987, align 8
  %1988 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1987, 1
  %1989 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1096, ptr %1989, align 8
  %1990 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1989, 1
  %1991 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1988, ptr %1991, align 8
  %1992 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1990, ptr %1992, align 8
  call void @memrefCopy(i64 4, ptr %1991, ptr %1992)
  call void @llvm.stackrestore.p0(ptr %1986)
  %1993 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1068, ptr %1106, ptr %1030, i32 1, i32 465, i32 117)
  %1994 = call ptr @llvm.stacksave.p0()
  %1995 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1020, ptr %1995, align 8
  %1996 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1995, 1
  %1997 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1918, ptr %1997, align 8
  %1998 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1997, 1
  %1999 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1996, ptr %1999, align 8
  %2000 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1998, ptr %2000, align 8
  call void @memrefCopy(i64 4, ptr %1999, ptr %2000)
  call void @llvm.stackrestore.p0(ptr %1994)
  %2001 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %2002 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %2003 = insertvalue { ptr, ptr, i64 } poison, ptr %2001, 0
  %2004 = insertvalue { ptr, ptr, i64 } %2003, ptr %2002, 1
  %2005 = insertvalue { ptr, ptr, i64 } %2004, i64 0, 2
  %2006 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %2007 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %2008 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %2009 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %2010 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %2011 = extractvalue { ptr, ptr, i64 } %2005, 0
  %2012 = extractvalue { ptr, ptr, i64 } %2005, 1
  %2013 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2011, 0
  %2014 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2013, ptr %2012, 1
  %2015 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2014, i64 776, 2
  %2016 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2015, i64 4, 3, 0
  %2017 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2016, i64 32, 4, 0
  %2018 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2017, i64 4, 3, 1
  %2019 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2018, i64 1, 4, 1
  %2020 = call ptr @llvm.stacksave.p0()
  %2021 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2019, ptr %2021, align 8
  %2022 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2021, 1
  %2023 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1020, ptr %2023, align 8
  %2024 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2023, 1
  %2025 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2022, ptr %2025, align 8
  %2026 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2024, ptr %2026, align 8
  call void @memrefCopy(i64 4, ptr %2025, ptr %2026)
  call void @llvm.stackrestore.p0(ptr %2020)
  %2027 = call ptr @llvm.stacksave.p0()
  %2028 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1805, ptr %2028, align 8
  %2029 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2028, 1
  %2030 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1058, ptr %2030, align 8
  %2031 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2030, 1
  %2032 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2029, ptr %2032, align 8
  %2033 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2031, ptr %2033, align 8
  call void @memrefCopy(i64 4, ptr %2032, ptr %2033)
  call void @llvm.stackrestore.p0(ptr %2027)
  %2034 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %2035 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %2036 = insertvalue { ptr, ptr, i64 } poison, ptr %2034, 0
  %2037 = insertvalue { ptr, ptr, i64 } %2036, ptr %2035, 1
  %2038 = insertvalue { ptr, ptr, i64 } %2037, i64 0, 2
  %2039 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %2040 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %2041 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %2042 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %2043 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %2044 = extractvalue { ptr, ptr, i64 } %2038, 0
  %2045 = extractvalue { ptr, ptr, i64 } %2038, 1
  %2046 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2044, 0
  %2047 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2046, ptr %2045, 1
  %2048 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2047, i64 8, 2
  %2049 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2048, i64 16, 3, 0
  %2050 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2049, i64 32, 4, 0
  %2051 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2050, i64 4, 3, 1
  %2052 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2051, i64 1, 4, 1
  %2053 = call ptr @llvm.stacksave.p0()
  %2054 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2052, ptr %2054, align 8
  %2055 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2054, 1
  %2056 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1096, ptr %2056, align 8
  %2057 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2056, 1
  %2058 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2055, ptr %2058, align 8
  %2059 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2057, ptr %2059, align 8
  call void @memrefCopy(i64 4, ptr %2058, ptr %2059)
  call void @llvm.stackrestore.p0(ptr %2053)
  %2060 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1068, ptr %1106, ptr %1030, i32 2, i32 396, i32 117)
  %2061 = call ptr @llvm.stacksave.p0()
  %2062 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1858, ptr %2062, align 8
  %2063 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2062, 1
  %2064 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1058, ptr %2064, align 8
  %2065 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2064, 1
  %2066 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2063, ptr %2066, align 8
  %2067 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2065, ptr %2067, align 8
  call void @memrefCopy(i64 4, ptr %2066, ptr %2067)
  call void @llvm.stackrestore.p0(ptr %2061)
  %2068 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %2069 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %2070 = insertvalue { ptr, ptr, i64 } poison, ptr %2068, 0
  %2071 = insertvalue { ptr, ptr, i64 } %2070, ptr %2069, 1
  %2072 = insertvalue { ptr, ptr, i64 } %2071, i64 0, 2
  %2073 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %2074 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %2075 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %2076 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %2077 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %2078 = extractvalue { ptr, ptr, i64 } %2072, 0
  %2079 = extractvalue { ptr, ptr, i64 } %2072, 1
  %2080 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2078, 0
  %2081 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2080, ptr %2079, 1
  %2082 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2081, i64 520, 2
  %2083 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2082, i64 16, 3, 0
  %2084 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2083, i64 32, 4, 0
  %2085 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2084, i64 4, 3, 1
  %2086 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2085, i64 1, 4, 1
  %2087 = call ptr @llvm.stacksave.p0()
  %2088 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2086, ptr %2088, align 8
  %2089 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2088, 1
  %2090 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1096, ptr %2090, align 8
  %2091 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2090, 1
  %2092 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2089, ptr %2092, align 8
  %2093 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2091, ptr %2093, align 8
  call void @memrefCopy(i64 4, ptr %2092, ptr %2093)
  call void @llvm.stackrestore.p0(ptr %2087)
  %2094 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1068, ptr %1106, ptr %1030, i32 2, i32 465, i32 117)
  %2095 = call ptr @llvm.stacksave.p0()
  %2096 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1020, ptr %2096, align 8
  %2097 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2096, 1
  %2098 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2019, ptr %2098, align 8
  %2099 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2098, 1
  %2100 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2097, ptr %2100, align 8
  %2101 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2099, ptr %2101, align 8
  call void @memrefCopy(i64 4, ptr %2100, ptr %2101)
  call void @llvm.stackrestore.p0(ptr %2095)
  %2102 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %2103 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %2104 = insertvalue { ptr, ptr, i64 } poison, ptr %2102, 0
  %2105 = insertvalue { ptr, ptr, i64 } %2104, ptr %2103, 1
  %2106 = insertvalue { ptr, ptr, i64 } %2105, i64 0, 2
  %2107 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %2108 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %2109 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %2110 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %2111 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %2112 = extractvalue { ptr, ptr, i64 } %2106, 0
  %2113 = extractvalue { ptr, ptr, i64 } %2106, 1
  %2114 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2112, 0
  %2115 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2114, ptr %2113, 1
  %2116 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2115, i64 780, 2
  %2117 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2116, i64 4, 3, 0
  %2118 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2117, i64 32, 4, 0
  %2119 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2118, i64 4, 3, 1
  %2120 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2119, i64 1, 4, 1
  %2121 = call ptr @llvm.stacksave.p0()
  %2122 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2120, ptr %2122, align 8
  %2123 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2122, 1
  %2124 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1020, ptr %2124, align 8
  %2125 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2124, 1
  %2126 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2123, ptr %2126, align 8
  %2127 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2125, ptr %2127, align 8
  call void @memrefCopy(i64 4, ptr %2126, ptr %2127)
  call void @llvm.stackrestore.p0(ptr %2121)
  %2128 = call ptr @llvm.stacksave.p0()
  %2129 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1805, ptr %2129, align 8
  %2130 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2129, 1
  %2131 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1058, ptr %2131, align 8
  %2132 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2131, 1
  %2133 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2130, ptr %2133, align 8
  %2134 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2132, ptr %2134, align 8
  call void @memrefCopy(i64 4, ptr %2133, ptr %2134)
  call void @llvm.stackrestore.p0(ptr %2128)
  %2135 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %2136 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %2137 = insertvalue { ptr, ptr, i64 } poison, ptr %2135, 0
  %2138 = insertvalue { ptr, ptr, i64 } %2137, ptr %2136, 1
  %2139 = insertvalue { ptr, ptr, i64 } %2138, i64 0, 2
  %2140 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %2141 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %2142 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %2143 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %2144 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %2145 = extractvalue { ptr, ptr, i64 } %2139, 0
  %2146 = extractvalue { ptr, ptr, i64 } %2139, 1
  %2147 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2145, 0
  %2148 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2147, ptr %2146, 1
  %2149 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2148, i64 12, 2
  %2150 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2149, i64 16, 3, 0
  %2151 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2150, i64 32, 4, 0
  %2152 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2151, i64 4, 3, 1
  %2153 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2152, i64 1, 4, 1
  %2154 = call ptr @llvm.stacksave.p0()
  %2155 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2153, ptr %2155, align 8
  %2156 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2155, 1
  %2157 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1096, ptr %2157, align 8
  %2158 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2157, 1
  %2159 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2156, ptr %2159, align 8
  %2160 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2158, ptr %2160, align 8
  call void @memrefCopy(i64 4, ptr %2159, ptr %2160)
  call void @llvm.stackrestore.p0(ptr %2154)
  %2161 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1068, ptr %1106, ptr %1030, i32 3, i32 396, i32 117)
  %2162 = call ptr @llvm.stacksave.p0()
  %2163 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1858, ptr %2163, align 8
  %2164 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2163, 1
  %2165 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1058, ptr %2165, align 8
  %2166 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2165, 1
  %2167 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2164, ptr %2167, align 8
  %2168 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2166, ptr %2168, align 8
  call void @memrefCopy(i64 4, ptr %2167, ptr %2168)
  call void @llvm.stackrestore.p0(ptr %2162)
  %2169 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %2170 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %2171 = insertvalue { ptr, ptr, i64 } poison, ptr %2169, 0
  %2172 = insertvalue { ptr, ptr, i64 } %2171, ptr %2170, 1
  %2173 = insertvalue { ptr, ptr, i64 } %2172, i64 0, 2
  %2174 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %2175 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %2176 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %2177 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %2178 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %2179 = extractvalue { ptr, ptr, i64 } %2173, 0
  %2180 = extractvalue { ptr, ptr, i64 } %2173, 1
  %2181 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2179, 0
  %2182 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2181, ptr %2180, 1
  %2183 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2182, i64 524, 2
  %2184 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2183, i64 16, 3, 0
  %2185 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2184, i64 32, 4, 0
  %2186 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2185, i64 4, 3, 1
  %2187 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2186, i64 1, 4, 1
  %2188 = call ptr @llvm.stacksave.p0()
  %2189 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2187, ptr %2189, align 8
  %2190 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2189, 1
  %2191 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1096, ptr %2191, align 8
  %2192 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2191, 1
  %2193 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2190, ptr %2193, align 8
  %2194 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2192, ptr %2194, align 8
  call void @memrefCopy(i64 4, ptr %2193, ptr %2194)
  call void @llvm.stackrestore.p0(ptr %2188)
  %2195 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1068, ptr %1106, ptr %1030, i32 3, i32 465, i32 117)
  %2196 = call ptr @llvm.stacksave.p0()
  %2197 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1020, ptr %2197, align 8
  %2198 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2197, 1
  %2199 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2120, ptr %2199, align 8
  %2200 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2199, 1
  %2201 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2198, ptr %2201, align 8
  %2202 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2200, ptr %2202, align 8
  call void @memrefCopy(i64 4, ptr %2201, ptr %2202)
  call void @llvm.stackrestore.p0(ptr %2196)
  %2203 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %2204 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %2205 = insertvalue { ptr, ptr, i64 } poison, ptr %2203, 0
  %2206 = insertvalue { ptr, ptr, i64 } %2205, ptr %2204, 1
  %2207 = insertvalue { ptr, ptr, i64 } %2206, i64 0, 2
  %2208 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %2209 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %2210 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %2211 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %2212 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %2213 = extractvalue { ptr, ptr, i64 } %2207, 0
  %2214 = extractvalue { ptr, ptr, i64 } %2207, 1
  %2215 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2213, 0
  %2216 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2215, ptr %2214, 1
  %2217 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2216, i64 784, 2
  %2218 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2217, i64 4, 3, 0
  %2219 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2218, i64 32, 4, 0
  %2220 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2219, i64 4, 3, 1
  %2221 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2220, i64 1, 4, 1
  %2222 = call ptr @llvm.stacksave.p0()
  %2223 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2221, ptr %2223, align 8
  %2224 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2223, 1
  %2225 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1020, ptr %2225, align 8
  %2226 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2225, 1
  %2227 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2224, ptr %2227, align 8
  %2228 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2226, ptr %2228, align 8
  call void @memrefCopy(i64 4, ptr %2227, ptr %2228)
  call void @llvm.stackrestore.p0(ptr %2222)
  %2229 = call ptr @llvm.stacksave.p0()
  %2230 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1805, ptr %2230, align 8
  %2231 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2230, 1
  %2232 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1058, ptr %2232, align 8
  %2233 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2232, 1
  %2234 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2231, ptr %2234, align 8
  %2235 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2233, ptr %2235, align 8
  call void @memrefCopy(i64 4, ptr %2234, ptr %2235)
  call void @llvm.stackrestore.p0(ptr %2229)
  %2236 = call ptr @llvm.stacksave.p0()
  %2237 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1087, ptr %2237, align 8
  %2238 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2237, 1
  %2239 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1096, ptr %2239, align 8
  %2240 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2239, 1
  %2241 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2238, ptr %2241, align 8
  %2242 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2240, ptr %2242, align 8
  call void @memrefCopy(i64 4, ptr %2241, ptr %2242)
  call void @llvm.stackrestore.p0(ptr %2236)
  %2243 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1068, ptr %1106, ptr %1030, i32 1, i32 534, i32 117)
  %2244 = call ptr @llvm.stacksave.p0()
  %2245 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1858, ptr %2245, align 8
  %2246 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2245, 1
  %2247 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1058, ptr %2247, align 8
  %2248 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2247, 1
  %2249 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2246, ptr %2249, align 8
  %2250 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2248, ptr %2250, align 8
  call void @memrefCopy(i64 4, ptr %2249, ptr %2250)
  call void @llvm.stackrestore.p0(ptr %2244)
  %2251 = call ptr @llvm.stacksave.p0()
  %2252 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1152, ptr %2252, align 8
  %2253 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2252, 1
  %2254 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1096, ptr %2254, align 8
  %2255 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2254, 1
  %2256 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2253, ptr %2256, align 8
  %2257 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2255, ptr %2257, align 8
  call void @memrefCopy(i64 4, ptr %2256, ptr %2257)
  call void @llvm.stackrestore.p0(ptr %2251)
  %2258 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1068, ptr %1106, ptr %1030, i32 1, i32 603, i32 117)
  %2259 = call ptr @llvm.stacksave.p0()
  %2260 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1020, ptr %2260, align 8
  %2261 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2260, 1
  %2262 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2221, ptr %2262, align 8
  %2263 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2262, 1
  %2264 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2261, ptr %2264, align 8
  %2265 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2263, ptr %2265, align 8
  call void @memrefCopy(i64 4, ptr %2264, ptr %2265)
  call void @llvm.stackrestore.p0(ptr %2259)
  %2266 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %2267 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %2268 = insertvalue { ptr, ptr, i64 } poison, ptr %2266, 0
  %2269 = insertvalue { ptr, ptr, i64 } %2268, ptr %2267, 1
  %2270 = insertvalue { ptr, ptr, i64 } %2269, i64 0, 2
  %2271 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %2272 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %2273 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %2274 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %2275 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %2276 = extractvalue { ptr, ptr, i64 } %2270, 0
  %2277 = extractvalue { ptr, ptr, i64 } %2270, 1
  %2278 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2276, 0
  %2279 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2278, ptr %2277, 1
  %2280 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2279, i64 788, 2
  %2281 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2280, i64 4, 3, 0
  %2282 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2281, i64 32, 4, 0
  %2283 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2282, i64 4, 3, 1
  %2284 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2283, i64 1, 4, 1
  %2285 = call ptr @llvm.stacksave.p0()
  %2286 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2284, ptr %2286, align 8
  %2287 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2286, 1
  %2288 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1020, ptr %2288, align 8
  %2289 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2288, 1
  %2290 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2287, ptr %2290, align 8
  %2291 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2289, ptr %2291, align 8
  call void @memrefCopy(i64 4, ptr %2290, ptr %2291)
  call void @llvm.stackrestore.p0(ptr %2285)
  %2292 = call ptr @llvm.stacksave.p0()
  %2293 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1805, ptr %2293, align 8
  %2294 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2293, 1
  %2295 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1058, ptr %2295, align 8
  %2296 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2295, 1
  %2297 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2294, ptr %2297, align 8
  %2298 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2296, ptr %2298, align 8
  call void @memrefCopy(i64 4, ptr %2297, ptr %2298)
  call void @llvm.stackrestore.p0(ptr %2292)
  %2299 = call ptr @llvm.stacksave.p0()
  %2300 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1219, ptr %2300, align 8
  %2301 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2300, 1
  %2302 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1096, ptr %2302, align 8
  %2303 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2302, 1
  %2304 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2301, ptr %2304, align 8
  %2305 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2303, ptr %2305, align 8
  call void @memrefCopy(i64 4, ptr %2304, ptr %2305)
  call void @llvm.stackrestore.p0(ptr %2299)
  %2306 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1068, ptr %1106, ptr %1030, i32 2, i32 534, i32 117)
  %2307 = call ptr @llvm.stacksave.p0()
  %2308 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1858, ptr %2308, align 8
  %2309 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2308, 1
  %2310 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1058, ptr %2310, align 8
  %2311 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2310, 1
  %2312 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2309, ptr %2312, align 8
  %2313 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2311, ptr %2313, align 8
  call void @memrefCopy(i64 4, ptr %2312, ptr %2313)
  call void @llvm.stackrestore.p0(ptr %2307)
  %2314 = call ptr @llvm.stacksave.p0()
  %2315 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1253, ptr %2315, align 8
  %2316 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2315, 1
  %2317 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1096, ptr %2317, align 8
  %2318 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2317, 1
  %2319 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2316, ptr %2319, align 8
  %2320 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2318, ptr %2320, align 8
  call void @memrefCopy(i64 4, ptr %2319, ptr %2320)
  call void @llvm.stackrestore.p0(ptr %2314)
  %2321 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1068, ptr %1106, ptr %1030, i32 2, i32 603, i32 117)
  %2322 = call ptr @llvm.stacksave.p0()
  %2323 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1020, ptr %2323, align 8
  %2324 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2323, 1
  %2325 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2284, ptr %2325, align 8
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
  %2343 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2342, i64 792, 2
  %2344 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2343, i64 4, 3, 0
  %2345 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2344, i64 32, 4, 0
  %2346 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2345, i64 4, 3, 1
  %2347 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2346, i64 1, 4, 1
  %2348 = call ptr @llvm.stacksave.p0()
  %2349 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2347, ptr %2349, align 8
  %2350 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2349, 1
  %2351 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1020, ptr %2351, align 8
  %2352 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2351, 1
  %2353 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2350, ptr %2353, align 8
  %2354 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2352, ptr %2354, align 8
  call void @memrefCopy(i64 4, ptr %2353, ptr %2354)
  call void @llvm.stackrestore.p0(ptr %2348)
  %2355 = call ptr @llvm.stacksave.p0()
  %2356 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1805, ptr %2356, align 8
  %2357 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2356, 1
  %2358 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1058, ptr %2358, align 8
  %2359 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2358, 1
  %2360 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2357, ptr %2360, align 8
  %2361 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2359, ptr %2361, align 8
  call void @memrefCopy(i64 4, ptr %2360, ptr %2361)
  call void @llvm.stackrestore.p0(ptr %2355)
  %2362 = call ptr @llvm.stacksave.p0()
  %2363 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1320, ptr %2363, align 8
  %2364 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2363, 1
  %2365 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1096, ptr %2365, align 8
  %2366 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2365, 1
  %2367 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2364, ptr %2367, align 8
  %2368 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2366, ptr %2368, align 8
  call void @memrefCopy(i64 4, ptr %2367, ptr %2368)
  call void @llvm.stackrestore.p0(ptr %2362)
  %2369 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1068, ptr %1106, ptr %1030, i32 3, i32 534, i32 117)
  %2370 = call ptr @llvm.stacksave.p0()
  %2371 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1858, ptr %2371, align 8
  %2372 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2371, 1
  %2373 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1058, ptr %2373, align 8
  %2374 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2373, 1
  %2375 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2372, ptr %2375, align 8
  %2376 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2374, ptr %2376, align 8
  call void @memrefCopy(i64 4, ptr %2375, ptr %2376)
  call void @llvm.stackrestore.p0(ptr %2370)
  %2377 = call ptr @llvm.stacksave.p0()
  %2378 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1354, ptr %2378, align 8
  %2379 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2378, 1
  %2380 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1096, ptr %2380, align 8
  %2381 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2380, 1
  %2382 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2379, ptr %2382, align 8
  %2383 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2381, ptr %2383, align 8
  call void @memrefCopy(i64 4, ptr %2382, ptr %2383)
  call void @llvm.stackrestore.p0(ptr %2377)
  %2384 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1068, ptr %1106, ptr %1030, i32 3, i32 603, i32 117)
  %2385 = call ptr @llvm.stacksave.p0()
  %2386 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1020, ptr %2386, align 8
  %2387 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2386, 1
  %2388 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2347, ptr %2388, align 8
  %2389 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2388, 1
  %2390 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2387, ptr %2390, align 8
  %2391 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2389, ptr %2391, align 8
  call void @memrefCopy(i64 4, ptr %2390, ptr %2391)
  call void @llvm.stackrestore.p0(ptr %2385)
  %2392 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %2393 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %2394 = insertvalue { ptr, ptr, i64 } poison, ptr %2392, 0
  %2395 = insertvalue { ptr, ptr, i64 } %2394, ptr %2393, 1
  %2396 = insertvalue { ptr, ptr, i64 } %2395, i64 0, 2
  %2397 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %2398 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %2399 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %2400 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %2401 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %2402 = extractvalue { ptr, ptr, i64 } %2396, 0
  %2403 = extractvalue { ptr, ptr, i64 } %2396, 1
  %2404 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2402, 0
  %2405 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2404, ptr %2403, 1
  %2406 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2405, i64 796, 2
  %2407 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2406, i64 4, 3, 0
  %2408 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2407, i64 32, 4, 0
  %2409 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2408, i64 4, 3, 1
  %2410 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2409, i64 1, 4, 1
  %2411 = call ptr @llvm.stacksave.p0()
  %2412 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2410, ptr %2412, align 8
  %2413 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2412, 1
  %2414 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1020, ptr %2414, align 8
  %2415 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2414, 1
  %2416 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2413, ptr %2416, align 8
  %2417 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2415, ptr %2417, align 8
  call void @memrefCopy(i64 4, ptr %2416, ptr %2417)
  call void @llvm.stackrestore.p0(ptr %2411)
  %2418 = call ptr @llvm.stacksave.p0()
  %2419 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1805, ptr %2419, align 8
  %2420 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2419, 1
  %2421 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1058, ptr %2421, align 8
  %2422 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2421, 1
  %2423 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2420, ptr %2423, align 8
  %2424 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2422, ptr %2424, align 8
  call void @memrefCopy(i64 4, ptr %2423, ptr %2424)
  call void @llvm.stackrestore.p0(ptr %2418)
  %2425 = call ptr @llvm.stacksave.p0()
  %2426 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1421, ptr %2426, align 8
  %2427 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2426, 1
  %2428 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1096, ptr %2428, align 8
  %2429 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2428, 1
  %2430 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2427, ptr %2430, align 8
  %2431 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2429, ptr %2431, align 8
  call void @memrefCopy(i64 4, ptr %2430, ptr %2431)
  call void @llvm.stackrestore.p0(ptr %2425)
  %2432 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1068, ptr %1106, ptr %1030, i32 1, i32 672, i32 117)
  %2433 = call ptr @llvm.stacksave.p0()
  %2434 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1858, ptr %2434, align 8
  %2435 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2434, 1
  %2436 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1058, ptr %2436, align 8
  %2437 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2436, 1
  %2438 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2435, ptr %2438, align 8
  %2439 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2437, ptr %2439, align 8
  call void @memrefCopy(i64 4, ptr %2438, ptr %2439)
  call void @llvm.stackrestore.p0(ptr %2433)
  %2440 = call ptr @llvm.stacksave.p0()
  %2441 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1455, ptr %2441, align 8
  %2442 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2441, 1
  %2443 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1096, ptr %2443, align 8
  %2444 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2443, 1
  %2445 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2442, ptr %2445, align 8
  %2446 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2444, ptr %2446, align 8
  call void @memrefCopy(i64 4, ptr %2445, ptr %2446)
  call void @llvm.stackrestore.p0(ptr %2440)
  %2447 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1068, ptr %1106, ptr %1030, i32 1, i32 741, i32 117)
  %2448 = call ptr @llvm.stacksave.p0()
  %2449 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1020, ptr %2449, align 8
  %2450 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2449, 1
  %2451 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2410, ptr %2451, align 8
  %2452 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2451, 1
  %2453 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2450, ptr %2453, align 8
  %2454 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2452, ptr %2454, align 8
  call void @memrefCopy(i64 4, ptr %2453, ptr %2454)
  call void @llvm.stackrestore.p0(ptr %2448)
  %2455 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %2456 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %2457 = insertvalue { ptr, ptr, i64 } poison, ptr %2455, 0
  %2458 = insertvalue { ptr, ptr, i64 } %2457, ptr %2456, 1
  %2459 = insertvalue { ptr, ptr, i64 } %2458, i64 0, 2
  %2460 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %2461 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %2462 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %2463 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %2464 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %2465 = extractvalue { ptr, ptr, i64 } %2459, 0
  %2466 = extractvalue { ptr, ptr, i64 } %2459, 1
  %2467 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2465, 0
  %2468 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2467, ptr %2466, 1
  %2469 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2468, i64 896, 2
  %2470 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2469, i64 4, 3, 0
  %2471 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2470, i64 32, 4, 0
  %2472 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2471, i64 4, 3, 1
  %2473 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2472, i64 1, 4, 1
  %2474 = call ptr @llvm.stacksave.p0()
  %2475 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2473, ptr %2475, align 8
  %2476 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2475, 1
  %2477 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1020, ptr %2477, align 8
  %2478 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2477, 1
  %2479 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2476, ptr %2479, align 8
  %2480 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2478, ptr %2480, align 8
  call void @memrefCopy(i64 4, ptr %2479, ptr %2480)
  call void @llvm.stackrestore.p0(ptr %2474)
  %2481 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %2482 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %2483 = insertvalue { ptr, ptr, i64 } poison, ptr %2481, 0
  %2484 = insertvalue { ptr, ptr, i64 } %2483, ptr %2482, 1
  %2485 = insertvalue { ptr, ptr, i64 } %2484, i64 0, 2
  %2486 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %2487 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %2488 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %2489 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %2490 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %2491 = extractvalue { ptr, ptr, i64 } %2485, 0
  %2492 = extractvalue { ptr, ptr, i64 } %2485, 1
  %2493 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2491, 0
  %2494 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2493, ptr %2492, 1
  %2495 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2494, i64 896, 2
  %2496 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2495, i64 4, 3, 0
  %2497 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2496, i64 32, 4, 0
  %2498 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2497, i64 16, 3, 1
  %2499 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2498, i64 1, 4, 1
  %2500 = call ptr @llvm.stacksave.p0()
  %2501 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2499, ptr %2501, align 8
  %2502 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2501, 1
  %2503 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1058, ptr %2503, align 8
  %2504 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2503, 1
  %2505 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2502, ptr %2505, align 8
  %2506 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2504, ptr %2506, align 8
  call void @memrefCopy(i64 4, ptr %2505, ptr %2506)
  call void @llvm.stackrestore.p0(ptr %2500)
  %2507 = call ptr @llvm.stacksave.p0()
  %2508 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1831, ptr %2508, align 8
  %2509 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2508, 1
  %2510 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1096, ptr %2510, align 8
  %2511 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2510, 1
  %2512 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2509, ptr %2512, align 8
  %2513 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2511, ptr %2513, align 8
  call void @memrefCopy(i64 4, ptr %2512, ptr %2513)
  call void @llvm.stackrestore.p0(ptr %2507)
  %2514 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1068, ptr %1106, ptr %1030, i32 2, i32 672, i32 117)
  %2515 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %2516 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %2517 = insertvalue { ptr, ptr, i64 } poison, ptr %2515, 0
  %2518 = insertvalue { ptr, ptr, i64 } %2517, ptr %2516, 1
  %2519 = insertvalue { ptr, ptr, i64 } %2518, i64 0, 2
  %2520 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %2521 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %2522 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %2523 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %2524 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %2525 = extractvalue { ptr, ptr, i64 } %2519, 0
  %2526 = extractvalue { ptr, ptr, i64 } %2519, 1
  %2527 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2525, 0
  %2528 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2527, ptr %2526, 1
  %2529 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2528, i64 912, 2
  %2530 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2529, i64 4, 3, 0
  %2531 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2530, i64 32, 4, 0
  %2532 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2531, i64 16, 3, 1
  %2533 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2532, i64 1, 4, 1
  %2534 = call ptr @llvm.stacksave.p0()
  %2535 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2533, ptr %2535, align 8
  %2536 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2535, 1
  %2537 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1058, ptr %2537, align 8
  %2538 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2537, 1
  %2539 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2536, ptr %2539, align 8
  %2540 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2538, ptr %2540, align 8
  call void @memrefCopy(i64 4, ptr %2539, ptr %2540)
  call void @llvm.stackrestore.p0(ptr %2534)
  %2541 = call ptr @llvm.stacksave.p0()
  %2542 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1884, ptr %2542, align 8
  %2543 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2542, 1
  %2544 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1096, ptr %2544, align 8
  %2545 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2544, 1
  %2546 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2543, ptr %2546, align 8
  %2547 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2545, ptr %2547, align 8
  call void @memrefCopy(i64 4, ptr %2546, ptr %2547)
  call void @llvm.stackrestore.p0(ptr %2541)
  %2548 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1068, ptr %1106, ptr %1030, i32 2, i32 741, i32 117)
  %2549 = call ptr @llvm.stacksave.p0()
  %2550 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1020, ptr %2550, align 8
  %2551 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2550, 1
  %2552 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2473, ptr %2552, align 8
  %2553 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2552, 1
  %2554 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2551, ptr %2554, align 8
  %2555 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2553, ptr %2555, align 8
  call void @memrefCopy(i64 4, ptr %2554, ptr %2555)
  call void @llvm.stackrestore.p0(ptr %2549)
  %2556 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %2557 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %2558 = insertvalue { ptr, ptr, i64 } poison, ptr %2556, 0
  %2559 = insertvalue { ptr, ptr, i64 } %2558, ptr %2557, 1
  %2560 = insertvalue { ptr, ptr, i64 } %2559, i64 0, 2
  %2561 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %2562 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %2563 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %2564 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %2565 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %2566 = extractvalue { ptr, ptr, i64 } %2560, 0
  %2567 = extractvalue { ptr, ptr, i64 } %2560, 1
  %2568 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2566, 0
  %2569 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2568, ptr %2567, 1
  %2570 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2569, i64 900, 2
  %2571 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2570, i64 4, 3, 0
  %2572 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2571, i64 32, 4, 0
  %2573 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2572, i64 4, 3, 1
  %2574 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2573, i64 1, 4, 1
  %2575 = call ptr @llvm.stacksave.p0()
  %2576 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2574, ptr %2576, align 8
  %2577 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2576, 1
  %2578 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1020, ptr %2578, align 8
  %2579 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2578, 1
  %2580 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2577, ptr %2580, align 8
  %2581 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2579, ptr %2581, align 8
  call void @memrefCopy(i64 4, ptr %2580, ptr %2581)
  call void @llvm.stackrestore.p0(ptr %2575)
  %2582 = call ptr @llvm.stacksave.p0()
  %2583 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2499, ptr %2583, align 8
  %2584 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2583, 1
  %2585 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1058, ptr %2585, align 8
  %2586 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2585, 1
  %2587 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2584, ptr %2587, align 8
  %2588 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2586, ptr %2588, align 8
  call void @memrefCopy(i64 4, ptr %2587, ptr %2588)
  call void @llvm.stackrestore.p0(ptr %2582)
  %2589 = call ptr @llvm.stacksave.p0()
  %2590 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1951, ptr %2590, align 8
  %2591 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2590, 1
  %2592 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1096, ptr %2592, align 8
  %2593 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2592, 1
  %2594 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2591, ptr %2594, align 8
  %2595 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2593, ptr %2595, align 8
  call void @memrefCopy(i64 4, ptr %2594, ptr %2595)
  call void @llvm.stackrestore.p0(ptr %2589)
  %2596 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1068, ptr %1106, ptr %1030, i32 3, i32 672, i32 117)
  %2597 = call ptr @llvm.stacksave.p0()
  %2598 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2533, ptr %2598, align 8
  %2599 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2598, 1
  %2600 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1058, ptr %2600, align 8
  %2601 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2600, 1
  %2602 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2599, ptr %2602, align 8
  %2603 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2601, ptr %2603, align 8
  call void @memrefCopy(i64 4, ptr %2602, ptr %2603)
  call void @llvm.stackrestore.p0(ptr %2597)
  %2604 = call ptr @llvm.stacksave.p0()
  %2605 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1985, ptr %2605, align 8
  %2606 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2605, 1
  %2607 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1096, ptr %2607, align 8
  %2608 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2607, 1
  %2609 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2606, ptr %2609, align 8
  %2610 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2608, ptr %2610, align 8
  call void @memrefCopy(i64 4, ptr %2609, ptr %2610)
  call void @llvm.stackrestore.p0(ptr %2604)
  %2611 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1068, ptr %1106, ptr %1030, i32 3, i32 741, i32 117)
  %2612 = call ptr @llvm.stacksave.p0()
  %2613 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1020, ptr %2613, align 8
  %2614 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2613, 1
  %2615 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2574, ptr %2615, align 8
  %2616 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2615, 1
  %2617 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2614, ptr %2617, align 8
  %2618 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2616, ptr %2618, align 8
  call void @memrefCopy(i64 4, ptr %2617, ptr %2618)
  call void @llvm.stackrestore.p0(ptr %2612)
  %2619 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %2620 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %2621 = insertvalue { ptr, ptr, i64 } poison, ptr %2619, 0
  %2622 = insertvalue { ptr, ptr, i64 } %2621, ptr %2620, 1
  %2623 = insertvalue { ptr, ptr, i64 } %2622, i64 0, 2
  %2624 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %2625 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %2626 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %2627 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %2628 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %2629 = extractvalue { ptr, ptr, i64 } %2623, 0
  %2630 = extractvalue { ptr, ptr, i64 } %2623, 1
  %2631 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2629, 0
  %2632 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2631, ptr %2630, 1
  %2633 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2632, i64 904, 2
  %2634 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2633, i64 4, 3, 0
  %2635 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2634, i64 32, 4, 0
  %2636 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2635, i64 4, 3, 1
  %2637 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2636, i64 1, 4, 1
  %2638 = call ptr @llvm.stacksave.p0()
  %2639 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2637, ptr %2639, align 8
  %2640 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2639, 1
  %2641 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1020, ptr %2641, align 8
  %2642 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2641, 1
  %2643 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2640, ptr %2643, align 8
  %2644 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2642, ptr %2644, align 8
  call void @memrefCopy(i64 4, ptr %2643, ptr %2644)
  call void @llvm.stackrestore.p0(ptr %2638)
  %2645 = call ptr @llvm.stacksave.p0()
  %2646 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2499, ptr %2646, align 8
  %2647 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2646, 1
  %2648 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1058, ptr %2648, align 8
  %2649 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2648, 1
  %2650 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2647, ptr %2650, align 8
  %2651 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2649, ptr %2651, align 8
  call void @memrefCopy(i64 4, ptr %2650, ptr %2651)
  call void @llvm.stackrestore.p0(ptr %2645)
  %2652 = call ptr @llvm.stacksave.p0()
  %2653 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2052, ptr %2653, align 8
  %2654 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2653, 1
  %2655 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1096, ptr %2655, align 8
  %2656 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2655, 1
  %2657 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2654, ptr %2657, align 8
  %2658 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2656, ptr %2658, align 8
  call void @memrefCopy(i64 4, ptr %2657, ptr %2658)
  call void @llvm.stackrestore.p0(ptr %2652)
  %2659 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1068, ptr %1106, ptr %1030, i32 1, i32 810, i32 117)
  %2660 = call ptr @llvm.stacksave.p0()
  %2661 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2533, ptr %2661, align 8
  %2662 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2661, 1
  %2663 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1058, ptr %2663, align 8
  %2664 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2663, 1
  %2665 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2662, ptr %2665, align 8
  %2666 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2664, ptr %2666, align 8
  call void @memrefCopy(i64 4, ptr %2665, ptr %2666)
  call void @llvm.stackrestore.p0(ptr %2660)
  %2667 = call ptr @llvm.stacksave.p0()
  %2668 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2086, ptr %2668, align 8
  %2669 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2668, 1
  %2670 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1096, ptr %2670, align 8
  %2671 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2670, 1
  %2672 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2669, ptr %2672, align 8
  %2673 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2671, ptr %2673, align 8
  call void @memrefCopy(i64 4, ptr %2672, ptr %2673)
  call void @llvm.stackrestore.p0(ptr %2667)
  %2674 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1068, ptr %1106, ptr %1030, i32 1, i32 879, i32 117)
  %2675 = call ptr @llvm.stacksave.p0()
  %2676 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1020, ptr %2676, align 8
  %2677 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2676, 1
  %2678 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2637, ptr %2678, align 8
  %2679 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2678, 1
  %2680 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2677, ptr %2680, align 8
  %2681 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2679, ptr %2681, align 8
  call void @memrefCopy(i64 4, ptr %2680, ptr %2681)
  call void @llvm.stackrestore.p0(ptr %2675)
  %2682 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %2683 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %2684 = insertvalue { ptr, ptr, i64 } poison, ptr %2682, 0
  %2685 = insertvalue { ptr, ptr, i64 } %2684, ptr %2683, 1
  %2686 = insertvalue { ptr, ptr, i64 } %2685, i64 0, 2
  %2687 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %2688 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %2689 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %2690 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %2691 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %2692 = extractvalue { ptr, ptr, i64 } %2686, 0
  %2693 = extractvalue { ptr, ptr, i64 } %2686, 1
  %2694 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2692, 0
  %2695 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2694, ptr %2693, 1
  %2696 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2695, i64 908, 2
  %2697 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2696, i64 4, 3, 0
  %2698 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2697, i64 32, 4, 0
  %2699 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2698, i64 4, 3, 1
  %2700 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2699, i64 1, 4, 1
  %2701 = call ptr @llvm.stacksave.p0()
  %2702 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2700, ptr %2702, align 8
  %2703 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2702, 1
  %2704 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1020, ptr %2704, align 8
  %2705 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2704, 1
  %2706 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2703, ptr %2706, align 8
  %2707 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2705, ptr %2707, align 8
  call void @memrefCopy(i64 4, ptr %2706, ptr %2707)
  call void @llvm.stackrestore.p0(ptr %2701)
  %2708 = call ptr @llvm.stacksave.p0()
  %2709 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2499, ptr %2709, align 8
  %2710 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2709, 1
  %2711 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1058, ptr %2711, align 8
  %2712 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2711, 1
  %2713 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2710, ptr %2713, align 8
  %2714 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2712, ptr %2714, align 8
  call void @memrefCopy(i64 4, ptr %2713, ptr %2714)
  call void @llvm.stackrestore.p0(ptr %2708)
  %2715 = call ptr @llvm.stacksave.p0()
  %2716 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2153, ptr %2716, align 8
  %2717 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2716, 1
  %2718 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1096, ptr %2718, align 8
  %2719 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2718, 1
  %2720 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2717, ptr %2720, align 8
  %2721 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2719, ptr %2721, align 8
  call void @memrefCopy(i64 4, ptr %2720, ptr %2721)
  call void @llvm.stackrestore.p0(ptr %2715)
  %2722 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1068, ptr %1106, ptr %1030, i32 2, i32 810, i32 117)
  %2723 = call ptr @llvm.stacksave.p0()
  %2724 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2533, ptr %2724, align 8
  %2725 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2724, 1
  %2726 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1058, ptr %2726, align 8
  %2727 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2726, 1
  %2728 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2725, ptr %2728, align 8
  %2729 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2727, ptr %2729, align 8
  call void @memrefCopy(i64 4, ptr %2728, ptr %2729)
  call void @llvm.stackrestore.p0(ptr %2723)
  %2730 = call ptr @llvm.stacksave.p0()
  %2731 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2187, ptr %2731, align 8
  %2732 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2731, 1
  %2733 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1096, ptr %2733, align 8
  %2734 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2733, 1
  %2735 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2732, ptr %2735, align 8
  %2736 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2734, ptr %2736, align 8
  call void @memrefCopy(i64 4, ptr %2735, ptr %2736)
  call void @llvm.stackrestore.p0(ptr %2730)
  %2737 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1068, ptr %1106, ptr %1030, i32 2, i32 879, i32 117)
  %2738 = call ptr @llvm.stacksave.p0()
  %2739 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1020, ptr %2739, align 8
  %2740 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2739, 1
  %2741 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2700, ptr %2741, align 8
  %2742 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2741, 1
  %2743 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2740, ptr %2743, align 8
  %2744 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2742, ptr %2744, align 8
  call void @memrefCopy(i64 4, ptr %2743, ptr %2744)
  call void @llvm.stackrestore.p0(ptr %2738)
  %2745 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %2746 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %2747 = insertvalue { ptr, ptr, i64 } poison, ptr %2745, 0
  %2748 = insertvalue { ptr, ptr, i64 } %2747, ptr %2746, 1
  %2749 = insertvalue { ptr, ptr, i64 } %2748, i64 0, 2
  %2750 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %2751 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %2752 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %2753 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %2754 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %2755 = extractvalue { ptr, ptr, i64 } %2749, 0
  %2756 = extractvalue { ptr, ptr, i64 } %2749, 1
  %2757 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2755, 0
  %2758 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2757, ptr %2756, 1
  %2759 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2758, i64 912, 2
  %2760 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2759, i64 4, 3, 0
  %2761 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2760, i64 32, 4, 0
  %2762 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2761, i64 4, 3, 1
  %2763 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2762, i64 1, 4, 1
  %2764 = call ptr @llvm.stacksave.p0()
  %2765 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2763, ptr %2765, align 8
  %2766 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2765, 1
  %2767 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1020, ptr %2767, align 8
  %2768 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2767, 1
  %2769 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2766, ptr %2769, align 8
  %2770 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2768, ptr %2770, align 8
  call void @memrefCopy(i64 4, ptr %2769, ptr %2770)
  call void @llvm.stackrestore.p0(ptr %2764)
  %2771 = call ptr @llvm.stacksave.p0()
  %2772 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2499, ptr %2772, align 8
  %2773 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2772, 1
  %2774 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1058, ptr %2774, align 8
  %2775 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2774, 1
  %2776 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2773, ptr %2776, align 8
  %2777 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2775, ptr %2777, align 8
  call void @memrefCopy(i64 4, ptr %2776, ptr %2777)
  call void @llvm.stackrestore.p0(ptr %2771)
  %2778 = call ptr @llvm.stacksave.p0()
  %2779 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1087, ptr %2779, align 8
  %2780 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2779, 1
  %2781 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1096, ptr %2781, align 8
  %2782 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2781, 1
  %2783 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2780, ptr %2783, align 8
  %2784 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2782, ptr %2784, align 8
  call void @memrefCopy(i64 4, ptr %2783, ptr %2784)
  call void @llvm.stackrestore.p0(ptr %2778)
  %2785 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1068, ptr %1106, ptr %1030, i32 3, i32 810, i32 117)
  %2786 = call ptr @llvm.stacksave.p0()
  %2787 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2533, ptr %2787, align 8
  %2788 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2787, 1
  %2789 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1058, ptr %2789, align 8
  %2790 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2789, 1
  %2791 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2788, ptr %2791, align 8
  %2792 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2790, ptr %2792, align 8
  call void @memrefCopy(i64 4, ptr %2791, ptr %2792)
  call void @llvm.stackrestore.p0(ptr %2786)
  %2793 = call ptr @llvm.stacksave.p0()
  %2794 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1152, ptr %2794, align 8
  %2795 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2794, 1
  %2796 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1096, ptr %2796, align 8
  %2797 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2796, 1
  %2798 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2795, ptr %2798, align 8
  %2799 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2797, ptr %2799, align 8
  call void @memrefCopy(i64 4, ptr %2798, ptr %2799)
  call void @llvm.stackrestore.p0(ptr %2793)
  %2800 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1068, ptr %1106, ptr %1030, i32 3, i32 879, i32 117)
  %2801 = call ptr @llvm.stacksave.p0()
  %2802 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1020, ptr %2802, align 8
  %2803 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2802, 1
  %2804 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2763, ptr %2804, align 8
  %2805 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2804, 1
  %2806 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2803, ptr %2806, align 8
  %2807 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2805, ptr %2807, align 8
  call void @memrefCopy(i64 4, ptr %2806, ptr %2807)
  call void @llvm.stackrestore.p0(ptr %2801)
  %2808 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %2809 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %2810 = insertvalue { ptr, ptr, i64 } poison, ptr %2808, 0
  %2811 = insertvalue { ptr, ptr, i64 } %2810, ptr %2809, 1
  %2812 = insertvalue { ptr, ptr, i64 } %2811, i64 0, 2
  %2813 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %2814 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %2815 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %2816 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %2817 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %2818 = extractvalue { ptr, ptr, i64 } %2812, 0
  %2819 = extractvalue { ptr, ptr, i64 } %2812, 1
  %2820 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2818, 0
  %2821 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2820, ptr %2819, 1
  %2822 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2821, i64 916, 2
  %2823 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2822, i64 4, 3, 0
  %2824 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2823, i64 32, 4, 0
  %2825 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2824, i64 4, 3, 1
  %2826 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2825, i64 1, 4, 1
  %2827 = call ptr @llvm.stacksave.p0()
  %2828 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2826, ptr %2828, align 8
  %2829 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2828, 1
  %2830 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1020, ptr %2830, align 8
  %2831 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2830, 1
  %2832 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2829, ptr %2832, align 8
  %2833 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2831, ptr %2833, align 8
  call void @memrefCopy(i64 4, ptr %2832, ptr %2833)
  call void @llvm.stackrestore.p0(ptr %2827)
  %2834 = call ptr @llvm.stacksave.p0()
  %2835 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2499, ptr %2835, align 8
  %2836 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2835, 1
  %2837 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1058, ptr %2837, align 8
  %2838 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2837, 1
  %2839 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2836, ptr %2839, align 8
  %2840 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2838, ptr %2840, align 8
  call void @memrefCopy(i64 4, ptr %2839, ptr %2840)
  call void @llvm.stackrestore.p0(ptr %2834)
  %2841 = call ptr @llvm.stacksave.p0()
  %2842 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1219, ptr %2842, align 8
  %2843 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2842, 1
  %2844 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1096, ptr %2844, align 8
  %2845 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2844, 1
  %2846 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2843, ptr %2846, align 8
  %2847 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2845, ptr %2847, align 8
  call void @memrefCopy(i64 4, ptr %2846, ptr %2847)
  call void @llvm.stackrestore.p0(ptr %2841)
  %2848 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1068, ptr %1106, ptr %1030, i32 1, i32 948, i32 117)
  %2849 = call ptr @llvm.stacksave.p0()
  %2850 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2533, ptr %2850, align 8
  %2851 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2850, 1
  %2852 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1058, ptr %2852, align 8
  %2853 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2852, 1
  %2854 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2851, ptr %2854, align 8
  %2855 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2853, ptr %2855, align 8
  call void @memrefCopy(i64 4, ptr %2854, ptr %2855)
  call void @llvm.stackrestore.p0(ptr %2849)
  %2856 = call ptr @llvm.stacksave.p0()
  %2857 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1253, ptr %2857, align 8
  %2858 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2857, 1
  %2859 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1096, ptr %2859, align 8
  %2860 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2859, 1
  %2861 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2858, ptr %2861, align 8
  %2862 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2860, ptr %2862, align 8
  call void @memrefCopy(i64 4, ptr %2861, ptr %2862)
  call void @llvm.stackrestore.p0(ptr %2856)
  %2863 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1068, ptr %1106, ptr %1030, i32 1, i32 1017, i32 117)
  %2864 = call ptr @llvm.stacksave.p0()
  %2865 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1020, ptr %2865, align 8
  %2866 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2865, 1
  %2867 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2826, ptr %2867, align 8
  %2868 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2867, 1
  %2869 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2866, ptr %2869, align 8
  %2870 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2868, ptr %2870, align 8
  call void @memrefCopy(i64 4, ptr %2869, ptr %2870)
  call void @llvm.stackrestore.p0(ptr %2864)
  %2871 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %2872 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %2873 = insertvalue { ptr, ptr, i64 } poison, ptr %2871, 0
  %2874 = insertvalue { ptr, ptr, i64 } %2873, ptr %2872, 1
  %2875 = insertvalue { ptr, ptr, i64 } %2874, i64 0, 2
  %2876 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %2877 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %2878 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %2879 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %2880 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %2881 = extractvalue { ptr, ptr, i64 } %2875, 0
  %2882 = extractvalue { ptr, ptr, i64 } %2875, 1
  %2883 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2881, 0
  %2884 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2883, ptr %2882, 1
  %2885 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2884, i64 920, 2
  %2886 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2885, i64 4, 3, 0
  %2887 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2886, i64 32, 4, 0
  %2888 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2887, i64 4, 3, 1
  %2889 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2888, i64 1, 4, 1
  %2890 = call ptr @llvm.stacksave.p0()
  %2891 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2889, ptr %2891, align 8
  %2892 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2891, 1
  %2893 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1020, ptr %2893, align 8
  %2894 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2893, 1
  %2895 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2892, ptr %2895, align 8
  %2896 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2894, ptr %2896, align 8
  call void @memrefCopy(i64 4, ptr %2895, ptr %2896)
  call void @llvm.stackrestore.p0(ptr %2890)
  %2897 = call ptr @llvm.stacksave.p0()
  %2898 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2499, ptr %2898, align 8
  %2899 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2898, 1
  %2900 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1058, ptr %2900, align 8
  %2901 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2900, 1
  %2902 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2899, ptr %2902, align 8
  %2903 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2901, ptr %2903, align 8
  call void @memrefCopy(i64 4, ptr %2902, ptr %2903)
  call void @llvm.stackrestore.p0(ptr %2897)
  %2904 = call ptr @llvm.stacksave.p0()
  %2905 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1320, ptr %2905, align 8
  %2906 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2905, 1
  %2907 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1096, ptr %2907, align 8
  %2908 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2907, 1
  %2909 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2906, ptr %2909, align 8
  %2910 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2908, ptr %2910, align 8
  call void @memrefCopy(i64 4, ptr %2909, ptr %2910)
  call void @llvm.stackrestore.p0(ptr %2904)
  %2911 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1068, ptr %1106, ptr %1030, i32 2, i32 948, i32 117)
  %2912 = call ptr @llvm.stacksave.p0()
  %2913 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2533, ptr %2913, align 8
  %2914 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2913, 1
  %2915 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1058, ptr %2915, align 8
  %2916 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2915, 1
  %2917 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2914, ptr %2917, align 8
  %2918 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2916, ptr %2918, align 8
  call void @memrefCopy(i64 4, ptr %2917, ptr %2918)
  call void @llvm.stackrestore.p0(ptr %2912)
  %2919 = call ptr @llvm.stacksave.p0()
  %2920 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1354, ptr %2920, align 8
  %2921 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2920, 1
  %2922 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1096, ptr %2922, align 8
  %2923 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2922, 1
  %2924 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2921, ptr %2924, align 8
  %2925 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2923, ptr %2925, align 8
  call void @memrefCopy(i64 4, ptr %2924, ptr %2925)
  call void @llvm.stackrestore.p0(ptr %2919)
  %2926 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1068, ptr %1106, ptr %1030, i32 2, i32 1017, i32 117)
  %2927 = call ptr @llvm.stacksave.p0()
  %2928 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1020, ptr %2928, align 8
  %2929 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2928, 1
  %2930 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2889, ptr %2930, align 8
  %2931 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2930, 1
  %2932 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2929, ptr %2932, align 8
  %2933 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2931, ptr %2933, align 8
  call void @memrefCopy(i64 4, ptr %2932, ptr %2933)
  call void @llvm.stackrestore.p0(ptr %2927)
  %2934 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %2935 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %2936 = insertvalue { ptr, ptr, i64 } poison, ptr %2934, 0
  %2937 = insertvalue { ptr, ptr, i64 } %2936, ptr %2935, 1
  %2938 = insertvalue { ptr, ptr, i64 } %2937, i64 0, 2
  %2939 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %2940 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %2941 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %2942 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %2943 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %2944 = extractvalue { ptr, ptr, i64 } %2938, 0
  %2945 = extractvalue { ptr, ptr, i64 } %2938, 1
  %2946 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2944, 0
  %2947 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2946, ptr %2945, 1
  %2948 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2947, i64 924, 2
  %2949 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2948, i64 4, 3, 0
  %2950 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2949, i64 32, 4, 0
  %2951 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2950, i64 4, 3, 1
  %2952 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2951, i64 1, 4, 1
  %2953 = call ptr @llvm.stacksave.p0()
  %2954 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2952, ptr %2954, align 8
  %2955 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2954, 1
  %2956 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1020, ptr %2956, align 8
  %2957 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2956, 1
  %2958 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2955, ptr %2958, align 8
  %2959 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2957, ptr %2959, align 8
  call void @memrefCopy(i64 4, ptr %2958, ptr %2959)
  call void @llvm.stackrestore.p0(ptr %2953)
  %2960 = call ptr @llvm.stacksave.p0()
  %2961 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2499, ptr %2961, align 8
  %2962 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2961, 1
  %2963 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1058, ptr %2963, align 8
  %2964 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2963, 1
  %2965 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2962, ptr %2965, align 8
  %2966 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2964, ptr %2966, align 8
  call void @memrefCopy(i64 4, ptr %2965, ptr %2966)
  call void @llvm.stackrestore.p0(ptr %2960)
  %2967 = call ptr @llvm.stacksave.p0()
  %2968 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1421, ptr %2968, align 8
  %2969 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2968, 1
  %2970 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1096, ptr %2970, align 8
  %2971 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2970, 1
  %2972 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2969, ptr %2972, align 8
  %2973 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2971, ptr %2973, align 8
  call void @memrefCopy(i64 4, ptr %2972, ptr %2973)
  call void @llvm.stackrestore.p0(ptr %2967)
  %2974 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1068, ptr %1106, ptr %1030, i32 3, i32 948, i32 117)
  %2975 = call ptr @llvm.stacksave.p0()
  %2976 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2533, ptr %2976, align 8
  %2977 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2976, 1
  %2978 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1058, ptr %2978, align 8
  %2979 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2978, 1
  %2980 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2977, ptr %2980, align 8
  %2981 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2979, ptr %2981, align 8
  call void @memrefCopy(i64 4, ptr %2980, ptr %2981)
  call void @llvm.stackrestore.p0(ptr %2975)
  %2982 = call ptr @llvm.stacksave.p0()
  %2983 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1455, ptr %2983, align 8
  %2984 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2983, 1
  %2985 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1096, ptr %2985, align 8
  %2986 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2985, 1
  %2987 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2984, ptr %2987, align 8
  %2988 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2986, ptr %2988, align 8
  call void @memrefCopy(i64 4, ptr %2987, ptr %2988)
  call void @llvm.stackrestore.p0(ptr %2982)
  %2989 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1068, ptr %1106, ptr %1030, i32 3, i32 1017, i32 117)
  %2990 = call ptr @llvm.stacksave.p0()
  %2991 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1020, ptr %2991, align 8
  %2992 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2991, 1
  %2993 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2952, ptr %2993, align 8
  %2994 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2993, 1
  %2995 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2992, ptr %2995, align 8
  %2996 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2994, ptr %2996, align 8
  call void @memrefCopy(i64 4, ptr %2995, ptr %2996)
  call void @llvm.stackrestore.p0(ptr %2990)
  %2997 = call ptr @malloc(i64 4096)
  %2998 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2997, 0
  %2999 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2998, ptr %2997, 1
  %3000 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2999, i64 0, 2
  %3001 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3000, i64 32, 3, 0
  %3002 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3001, i64 32, 3, 1
  %3003 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3002, i64 32, 4, 0
  %3004 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3003, i64 1, 4, 1
  %3005 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %3006 = mul i64 1, %3005
  %3007 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %3008 = mul i64 %3006, %3007
  %3009 = mul i64 %3008, 4
  %3010 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %3011 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %3012 = getelementptr float, ptr %3010, i64 %3011
  %3013 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3004, 1
  %3014 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3004, 2
  %3015 = getelementptr float, ptr %3013, i64 %3014
  call void @llvm.memcpy.p0.p0.i64(ptr %3015, ptr %3012, i64 %3009, i1 false)
  ret { ptr, ptr, i64, [2 x i64], [2 x i64] } %3004
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
