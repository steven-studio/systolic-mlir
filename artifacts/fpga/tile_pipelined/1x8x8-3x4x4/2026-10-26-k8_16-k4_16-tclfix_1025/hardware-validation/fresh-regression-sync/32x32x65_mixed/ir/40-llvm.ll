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
  %115 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %114, i64 65, 4, 0
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
  %192 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %191, i64 65, 4, 0
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
  %245 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %244, i64 65, 4, 0
  %246 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %245, i64 16, 3, 1
  %247 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %246, i64 1, 4, 1
  %248 = call ptr @llvm.stacksave.p0()
  %249 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %247, ptr %249, align 8
  %250 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %249, 1
  %251 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %251, align 8
  %252 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %251, 1
  %253 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %250, ptr %253, align 8
  %254 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %252, ptr %254, align 8
  call void @memrefCopy(i64 4, ptr %253, ptr %254)
  call void @llvm.stackrestore.p0(ptr %248)
  %255 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %256 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %257 = insertvalue { ptr, ptr, i64 } poison, ptr %255, 0
  %258 = insertvalue { ptr, ptr, i64 } %257, ptr %256, 1
  %259 = insertvalue { ptr, ptr, i64 } %258, i64 0, 2
  %260 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %261 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %262 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %263 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %264 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %265 = extractvalue { ptr, ptr, i64 } %259, 0
  %266 = extractvalue { ptr, ptr, i64 } %259, 1
  %267 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %265, 0
  %268 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %267, ptr %266, 1
  %269 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %268, i64 1024, 2
  %270 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %269, i64 16, 3, 0
  %271 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %270, i64 32, 4, 0
  %272 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %271, i64 8, 3, 1
  %273 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %272, i64 1, 4, 1
  %274 = call ptr @llvm.stacksave.p0()
  %275 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %273, ptr %275, align 8
  %276 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %275, 1
  %277 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %277, align 8
  %278 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %277, 1
  %279 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %276, ptr %279, align 8
  %280 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %278, ptr %280, align 8
  call void @memrefCopy(i64 4, ptr %279, ptr %280)
  call void @llvm.stackrestore.p0(ptr %274)
  %281 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 128, i32 125)
  %282 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %283 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %284 = insertvalue { ptr, ptr, i64 } poison, ptr %282, 0
  %285 = insertvalue { ptr, ptr, i64 } %284, ptr %283, 1
  %286 = insertvalue { ptr, ptr, i64 } %285, i64 0, 2
  %287 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %288 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %289 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %290 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %291 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %292 = extractvalue { ptr, ptr, i64 } %286, 0
  %293 = extractvalue { ptr, ptr, i64 } %286, 1
  %294 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %292, 0
  %295 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %294, ptr %293, 1
  %296 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %295, i64 48, 2
  %297 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %296, i64 8, 3, 0
  %298 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %297, i64 65, 4, 0
  %299 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %298, i64 16, 3, 1
  %300 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %299, i64 1, 4, 1
  %301 = call ptr @llvm.stacksave.p0()
  %302 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %300, ptr %302, align 8
  %303 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %302, 1
  %304 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %304, align 8
  %305 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %304, 1
  %306 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %303, ptr %306, align 8
  %307 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %305, ptr %307, align 8
  call void @memrefCopy(i64 4, ptr %306, ptr %307)
  call void @llvm.stackrestore.p0(ptr %301)
  %308 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %309 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %310 = insertvalue { ptr, ptr, i64 } poison, ptr %308, 0
  %311 = insertvalue { ptr, ptr, i64 } %310, ptr %309, 1
  %312 = insertvalue { ptr, ptr, i64 } %311, i64 0, 2
  %313 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %314 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %315 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %316 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %317 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %318 = extractvalue { ptr, ptr, i64 } %312, 0
  %319 = extractvalue { ptr, ptr, i64 } %312, 1
  %320 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %318, 0
  %321 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %320, ptr %319, 1
  %322 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %321, i64 1536, 2
  %323 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %322, i64 16, 3, 0
  %324 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %323, i64 32, 4, 0
  %325 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %324, i64 8, 3, 1
  %326 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %325, i64 1, 4, 1
  %327 = call ptr @llvm.stacksave.p0()
  %328 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %326, ptr %328, align 8
  %329 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %328, 1
  %330 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %330, align 8
  %331 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %330, 1
  %332 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %329, ptr %332, align 8
  %333 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %331, ptr %333, align 8
  call void @memrefCopy(i64 4, ptr %332, ptr %333)
  call void @llvm.stackrestore.p0(ptr %327)
  %334 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 197, i32 125)
  %335 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %336 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %337 = insertvalue { ptr, ptr, i64 } poison, ptr %335, 0
  %338 = insertvalue { ptr, ptr, i64 } %337, ptr %336, 1
  %339 = insertvalue { ptr, ptr, i64 } %338, i64 0, 2
  %340 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %341 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %342 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %343 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %344 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %345 = extractvalue { ptr, ptr, i64 } %339, 0
  %346 = extractvalue { ptr, ptr, i64 } %339, 1
  %347 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %345, 0
  %348 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %347, ptr %346, 1
  %349 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %348, i64 64, 2
  %350 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %349, i64 8, 3, 0
  %351 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %350, i64 65, 4, 0
  %352 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %351, i64 1, 3, 1
  %353 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %352, i64 1, 4, 1
  %354 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 0
  %355 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %356 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %354, 0
  %357 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %356, ptr %355, 1
  %358 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %357, i64 0, 2
  %359 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %358, i64 8, 3, 0
  %360 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %359, i64 1, 4, 0
  %361 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %360, i64 1, 3, 1
  %362 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %361, i64 1, 4, 1
  %363 = call ptr @llvm.stacksave.p0()
  %364 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %353, ptr %364, align 8
  %365 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %364, 1
  %366 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %362, ptr %366, align 8
  %367 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %366, 1
  %368 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %365, ptr %368, align 8
  %369 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %367, ptr %369, align 8
  call void @memrefCopy(i64 4, ptr %368, ptr %369)
  call void @llvm.stackrestore.p0(ptr %363)
  %370 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %371 = ptrtoint ptr %370 to i64
  %372 = inttoptr i64 %371 to ptr
  %373 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %374 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %375 = insertvalue { ptr, ptr, i64 } poison, ptr %373, 0
  %376 = insertvalue { ptr, ptr, i64 } %375, ptr %374, 1
  %377 = insertvalue { ptr, ptr, i64 } %376, i64 0, 2
  %378 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %379 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %380 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %381 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %382 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %383 = extractvalue { ptr, ptr, i64 } %377, 0
  %384 = extractvalue { ptr, ptr, i64 } %377, 1
  %385 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %383, 0
  %386 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %385, ptr %384, 1
  %387 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %386, i64 2048, 2
  %388 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %387, i64 1, 3, 0
  %389 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %388, i64 32, 4, 0
  %390 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %389, i64 8, 3, 1
  %391 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %390, i64 1, 4, 1
  %392 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 0
  %393 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %394 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %392, 0
  %395 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %394, ptr %393, 1
  %396 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %395, i64 0, 2
  %397 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %396, i64 1, 3, 0
  %398 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %397, i64 8, 4, 0
  %399 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %398, i64 8, 3, 1
  %400 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %399, i64 1, 4, 1
  %401 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %391, 3, 0
  %402 = mul i64 1, %401
  %403 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %391, 3, 1
  %404 = mul i64 %402, %403
  %405 = mul i64 %404, 4
  %406 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %391, 1
  %407 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %391, 2
  %408 = getelementptr float, ptr %406, i64 %407
  %409 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %400, 1
  %410 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %400, 2
  %411 = getelementptr float, ptr %409, i64 %410
  call void @llvm.memcpy.p0.p0.i64(ptr %411, ptr %408, i64 %405, i1 false)
  %412 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %413 = ptrtoint ptr %412 to i64
  %414 = inttoptr i64 %413 to ptr
  %415 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 1, i32 8, ptr %372, ptr %414, ptr %98, i32 0, i32 266, i32 110)
  %416 = call ptr @llvm.stacksave.p0()
  %417 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %417, align 8
  %418 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %417, 1
  %419 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %79, ptr %419, align 8
  %420 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %419, 1
  %421 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %418, ptr %421, align 8
  %422 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %420, ptr %422, align 8
  call void @memrefCopy(i64 4, ptr %421, ptr %422)
  call void @llvm.stackrestore.p0(ptr %416)
  %423 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %424 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %425 = insertvalue { ptr, ptr, i64 } poison, ptr %423, 0
  %426 = insertvalue { ptr, ptr, i64 } %425, ptr %424, 1
  %427 = insertvalue { ptr, ptr, i64 } %426, i64 0, 2
  %428 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %429 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %430 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %431 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %432 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %433 = extractvalue { ptr, ptr, i64 } %427, 0
  %434 = extractvalue { ptr, ptr, i64 } %427, 1
  %435 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %433, 0
  %436 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %435, ptr %434, 1
  %437 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %436, i64 8, 2
  %438 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %437, i64 8, 3, 0
  %439 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %438, i64 32, 4, 0
  %440 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %439, i64 8, 3, 1
  %441 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %440, i64 1, 4, 1
  %442 = call ptr @llvm.stacksave.p0()
  %443 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %441, ptr %443, align 8
  %444 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %443, 1
  %445 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %445, align 8
  %446 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %445, 1
  %447 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %444, ptr %447, align 8
  %448 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %446, ptr %448, align 8
  call void @memrefCopy(i64 4, ptr %447, ptr %448)
  call void @llvm.stackrestore.p0(ptr %442)
  %449 = call ptr @llvm.stacksave.p0()
  %450 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %117, ptr %450, align 8
  %451 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %450, 1
  %452 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %452, align 8
  %453 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %452, 1
  %454 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %451, ptr %454, align 8
  %455 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %453, ptr %455, align 8
  call void @memrefCopy(i64 4, ptr %454, ptr %455)
  call void @llvm.stackrestore.p0(ptr %449)
  %456 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %457 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %458 = insertvalue { ptr, ptr, i64 } poison, ptr %456, 0
  %459 = insertvalue { ptr, ptr, i64 } %458, ptr %457, 1
  %460 = insertvalue { ptr, ptr, i64 } %459, i64 0, 2
  %461 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %462 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %463 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %464 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %465 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %466 = extractvalue { ptr, ptr, i64 } %460, 0
  %467 = extractvalue { ptr, ptr, i64 } %460, 1
  %468 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %466, 0
  %469 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %468, ptr %467, 1
  %470 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %469, i64 8, 2
  %471 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %470, i64 16, 3, 0
  %472 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %471, i64 32, 4, 0
  %473 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %472, i64 8, 3, 1
  %474 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %473, i64 1, 4, 1
  %475 = call ptr @llvm.stacksave.p0()
  %476 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %474, ptr %476, align 8
  %477 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %476, 1
  %478 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %478, align 8
  %479 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %478, 1
  %480 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %477, ptr %480, align 8
  %481 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %479, ptr %481, align 8
  call void @memrefCopy(i64 4, ptr %480, ptr %481)
  call void @llvm.stackrestore.p0(ptr %475)
  %482 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 335, i32 125)
  %483 = call ptr @llvm.stacksave.p0()
  %484 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %194, ptr %484, align 8
  %485 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %484, 1
  %486 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %486, align 8
  %487 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %486, 1
  %488 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %485, ptr %488, align 8
  %489 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %487, ptr %489, align 8
  call void @memrefCopy(i64 4, ptr %488, ptr %489)
  call void @llvm.stackrestore.p0(ptr %483)
  %490 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %491 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %492 = insertvalue { ptr, ptr, i64 } poison, ptr %490, 0
  %493 = insertvalue { ptr, ptr, i64 } %492, ptr %491, 1
  %494 = insertvalue { ptr, ptr, i64 } %493, i64 0, 2
  %495 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %496 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %497 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %498 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %499 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %500 = extractvalue { ptr, ptr, i64 } %494, 0
  %501 = extractvalue { ptr, ptr, i64 } %494, 1
  %502 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %500, 0
  %503 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %502, ptr %501, 1
  %504 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %503, i64 520, 2
  %505 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %504, i64 16, 3, 0
  %506 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %505, i64 32, 4, 0
  %507 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %506, i64 8, 3, 1
  %508 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %507, i64 1, 4, 1
  %509 = call ptr @llvm.stacksave.p0()
  %510 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %508, ptr %510, align 8
  %511 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %510, 1
  %512 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %512, align 8
  %513 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %512, 1
  %514 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %511, ptr %514, align 8
  %515 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %513, ptr %515, align 8
  call void @memrefCopy(i64 4, ptr %514, ptr %515)
  call void @llvm.stackrestore.p0(ptr %509)
  %516 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 404, i32 125)
  %517 = call ptr @llvm.stacksave.p0()
  %518 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %247, ptr %518, align 8
  %519 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %518, 1
  %520 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %520, align 8
  %521 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %520, 1
  %522 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %519, ptr %522, align 8
  %523 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %521, ptr %523, align 8
  call void @memrefCopy(i64 4, ptr %522, ptr %523)
  call void @llvm.stackrestore.p0(ptr %517)
  %524 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %525 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %526 = insertvalue { ptr, ptr, i64 } poison, ptr %524, 0
  %527 = insertvalue { ptr, ptr, i64 } %526, ptr %525, 1
  %528 = insertvalue { ptr, ptr, i64 } %527, i64 0, 2
  %529 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %530 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %531 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %532 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %533 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %534 = extractvalue { ptr, ptr, i64 } %528, 0
  %535 = extractvalue { ptr, ptr, i64 } %528, 1
  %536 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %534, 0
  %537 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %536, ptr %535, 1
  %538 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %537, i64 1032, 2
  %539 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %538, i64 16, 3, 0
  %540 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %539, i64 32, 4, 0
  %541 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %540, i64 8, 3, 1
  %542 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %541, i64 1, 4, 1
  %543 = call ptr @llvm.stacksave.p0()
  %544 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %542, ptr %544, align 8
  %545 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %544, 1
  %546 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %546, align 8
  %547 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %546, 1
  %548 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %545, ptr %548, align 8
  %549 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %547, ptr %549, align 8
  call void @memrefCopy(i64 4, ptr %548, ptr %549)
  call void @llvm.stackrestore.p0(ptr %543)
  %550 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 473, i32 125)
  %551 = call ptr @llvm.stacksave.p0()
  %552 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %300, ptr %552, align 8
  %553 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %552, 1
  %554 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %554, align 8
  %555 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %554, 1
  %556 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %553, ptr %556, align 8
  %557 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %555, ptr %557, align 8
  call void @memrefCopy(i64 4, ptr %556, ptr %557)
  call void @llvm.stackrestore.p0(ptr %551)
  %558 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %559 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %560 = insertvalue { ptr, ptr, i64 } poison, ptr %558, 0
  %561 = insertvalue { ptr, ptr, i64 } %560, ptr %559, 1
  %562 = insertvalue { ptr, ptr, i64 } %561, i64 0, 2
  %563 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %564 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %565 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %566 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %567 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %568 = extractvalue { ptr, ptr, i64 } %562, 0
  %569 = extractvalue { ptr, ptr, i64 } %562, 1
  %570 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %568, 0
  %571 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %570, ptr %569, 1
  %572 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %571, i64 1544, 2
  %573 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %572, i64 16, 3, 0
  %574 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %573, i64 32, 4, 0
  %575 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %574, i64 8, 3, 1
  %576 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %575, i64 1, 4, 1
  %577 = call ptr @llvm.stacksave.p0()
  %578 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %576, ptr %578, align 8
  %579 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %578, 1
  %580 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %580, align 8
  %581 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %580, 1
  %582 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %579, ptr %582, align 8
  %583 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %581, ptr %583, align 8
  call void @memrefCopy(i64 4, ptr %582, ptr %583)
  call void @llvm.stackrestore.p0(ptr %577)
  %584 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 542, i32 125)
  %585 = call ptr @llvm.stacksave.p0()
  %586 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %353, ptr %586, align 8
  %587 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %586, 1
  %588 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %362, ptr %588, align 8
  %589 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %588, 1
  %590 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %587, ptr %590, align 8
  %591 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %589, ptr %591, align 8
  call void @memrefCopy(i64 4, ptr %590, ptr %591)
  call void @llvm.stackrestore.p0(ptr %585)
  %592 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %593 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %594 = insertvalue { ptr, ptr, i64 } poison, ptr %592, 0
  %595 = insertvalue { ptr, ptr, i64 } %594, ptr %593, 1
  %596 = insertvalue { ptr, ptr, i64 } %595, i64 0, 2
  %597 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %598 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %599 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %600 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %601 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %602 = extractvalue { ptr, ptr, i64 } %596, 0
  %603 = extractvalue { ptr, ptr, i64 } %596, 1
  %604 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %602, 0
  %605 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %604, ptr %603, 1
  %606 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %605, i64 2056, 2
  %607 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %606, i64 1, 3, 0
  %608 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %607, i64 32, 4, 0
  %609 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %608, i64 8, 3, 1
  %610 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %609, i64 1, 4, 1
  %611 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %610, 3, 0
  %612 = mul i64 1, %611
  %613 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %610, 3, 1
  %614 = mul i64 %612, %613
  %615 = mul i64 %614, 4
  %616 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %610, 1
  %617 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %610, 2
  %618 = getelementptr float, ptr %616, i64 %617
  %619 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %400, 1
  %620 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %400, 2
  %621 = getelementptr float, ptr %619, i64 %620
  call void @llvm.memcpy.p0.p0.i64(ptr %621, ptr %618, i64 %615, i1 false)
  %622 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 1, i32 8, ptr %372, ptr %414, ptr %98, i32 0, i32 611, i32 110)
  %623 = call ptr @llvm.stacksave.p0()
  %624 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %624, align 8
  %625 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %624, 1
  %626 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %441, ptr %626, align 8
  %627 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %626, 1
  %628 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %625, ptr %628, align 8
  %629 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %627, ptr %629, align 8
  call void @memrefCopy(i64 4, ptr %628, ptr %629)
  call void @llvm.stackrestore.p0(ptr %623)
  %630 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %631 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %632 = insertvalue { ptr, ptr, i64 } poison, ptr %630, 0
  %633 = insertvalue { ptr, ptr, i64 } %632, ptr %631, 1
  %634 = insertvalue { ptr, ptr, i64 } %633, i64 0, 2
  %635 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %636 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %637 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %638 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %639 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %640 = extractvalue { ptr, ptr, i64 } %634, 0
  %641 = extractvalue { ptr, ptr, i64 } %634, 1
  %642 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %640, 0
  %643 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %642, ptr %641, 1
  %644 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %643, i64 16, 2
  %645 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %644, i64 8, 3, 0
  %646 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %645, i64 32, 4, 0
  %647 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %646, i64 8, 3, 1
  %648 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %647, i64 1, 4, 1
  %649 = call ptr @llvm.stacksave.p0()
  %650 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %648, ptr %650, align 8
  %651 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %650, 1
  %652 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %652, align 8
  %653 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %652, 1
  %654 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %651, ptr %654, align 8
  %655 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %653, ptr %655, align 8
  call void @memrefCopy(i64 4, ptr %654, ptr %655)
  call void @llvm.stackrestore.p0(ptr %649)
  %656 = call ptr @llvm.stacksave.p0()
  %657 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %117, ptr %657, align 8
  %658 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %657, 1
  %659 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %659, align 8
  %660 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %659, 1
  %661 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %658, ptr %661, align 8
  %662 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %660, ptr %662, align 8
  call void @memrefCopy(i64 4, ptr %661, ptr %662)
  call void @llvm.stackrestore.p0(ptr %656)
  %663 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %664 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %665 = insertvalue { ptr, ptr, i64 } poison, ptr %663, 0
  %666 = insertvalue { ptr, ptr, i64 } %665, ptr %664, 1
  %667 = insertvalue { ptr, ptr, i64 } %666, i64 0, 2
  %668 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %669 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %670 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %671 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %672 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %673 = extractvalue { ptr, ptr, i64 } %667, 0
  %674 = extractvalue { ptr, ptr, i64 } %667, 1
  %675 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %673, 0
  %676 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %675, ptr %674, 1
  %677 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %676, i64 16, 2
  %678 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %677, i64 16, 3, 0
  %679 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %678, i64 32, 4, 0
  %680 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %679, i64 8, 3, 1
  %681 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %680, i64 1, 4, 1
  %682 = call ptr @llvm.stacksave.p0()
  %683 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %681, ptr %683, align 8
  %684 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %683, 1
  %685 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %685, align 8
  %686 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %685, 1
  %687 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %684, ptr %687, align 8
  %688 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %686, ptr %688, align 8
  call void @memrefCopy(i64 4, ptr %687, ptr %688)
  call void @llvm.stackrestore.p0(ptr %682)
  %689 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 680, i32 125)
  %690 = call ptr @llvm.stacksave.p0()
  %691 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %194, ptr %691, align 8
  %692 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %691, 1
  %693 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %693, align 8
  %694 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %693, 1
  %695 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %692, ptr %695, align 8
  %696 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %694, ptr %696, align 8
  call void @memrefCopy(i64 4, ptr %695, ptr %696)
  call void @llvm.stackrestore.p0(ptr %690)
  %697 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %698 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %699 = insertvalue { ptr, ptr, i64 } poison, ptr %697, 0
  %700 = insertvalue { ptr, ptr, i64 } %699, ptr %698, 1
  %701 = insertvalue { ptr, ptr, i64 } %700, i64 0, 2
  %702 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %703 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %704 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %705 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %706 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %707 = extractvalue { ptr, ptr, i64 } %701, 0
  %708 = extractvalue { ptr, ptr, i64 } %701, 1
  %709 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %707, 0
  %710 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %709, ptr %708, 1
  %711 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %710, i64 528, 2
  %712 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %711, i64 16, 3, 0
  %713 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %712, i64 32, 4, 0
  %714 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %713, i64 8, 3, 1
  %715 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %714, i64 1, 4, 1
  %716 = call ptr @llvm.stacksave.p0()
  %717 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %715, ptr %717, align 8
  %718 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %717, 1
  %719 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %719, align 8
  %720 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %719, 1
  %721 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %718, ptr %721, align 8
  %722 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %720, ptr %722, align 8
  call void @memrefCopy(i64 4, ptr %721, ptr %722)
  call void @llvm.stackrestore.p0(ptr %716)
  %723 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 749, i32 125)
  %724 = call ptr @llvm.stacksave.p0()
  %725 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %247, ptr %725, align 8
  %726 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %725, 1
  %727 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %727, align 8
  %728 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %727, 1
  %729 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %726, ptr %729, align 8
  %730 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %728, ptr %730, align 8
  call void @memrefCopy(i64 4, ptr %729, ptr %730)
  call void @llvm.stackrestore.p0(ptr %724)
  %731 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %732 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %733 = insertvalue { ptr, ptr, i64 } poison, ptr %731, 0
  %734 = insertvalue { ptr, ptr, i64 } %733, ptr %732, 1
  %735 = insertvalue { ptr, ptr, i64 } %734, i64 0, 2
  %736 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %737 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %738 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %739 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %740 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %741 = extractvalue { ptr, ptr, i64 } %735, 0
  %742 = extractvalue { ptr, ptr, i64 } %735, 1
  %743 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %741, 0
  %744 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %743, ptr %742, 1
  %745 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %744, i64 1040, 2
  %746 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %745, i64 16, 3, 0
  %747 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %746, i64 32, 4, 0
  %748 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %747, i64 8, 3, 1
  %749 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %748, i64 1, 4, 1
  %750 = call ptr @llvm.stacksave.p0()
  %751 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %749, ptr %751, align 8
  %752 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %751, 1
  %753 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %753, align 8
  %754 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %753, 1
  %755 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %752, ptr %755, align 8
  %756 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %754, ptr %756, align 8
  call void @memrefCopy(i64 4, ptr %755, ptr %756)
  call void @llvm.stackrestore.p0(ptr %750)
  %757 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 818, i32 125)
  %758 = call ptr @llvm.stacksave.p0()
  %759 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %300, ptr %759, align 8
  %760 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %759, 1
  %761 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %761, align 8
  %762 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %761, 1
  %763 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %760, ptr %763, align 8
  %764 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %762, ptr %764, align 8
  call void @memrefCopy(i64 4, ptr %763, ptr %764)
  call void @llvm.stackrestore.p0(ptr %758)
  %765 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %766 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %767 = insertvalue { ptr, ptr, i64 } poison, ptr %765, 0
  %768 = insertvalue { ptr, ptr, i64 } %767, ptr %766, 1
  %769 = insertvalue { ptr, ptr, i64 } %768, i64 0, 2
  %770 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %771 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %772 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %773 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %774 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %775 = extractvalue { ptr, ptr, i64 } %769, 0
  %776 = extractvalue { ptr, ptr, i64 } %769, 1
  %777 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %775, 0
  %778 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %777, ptr %776, 1
  %779 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %778, i64 1552, 2
  %780 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %779, i64 16, 3, 0
  %781 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %780, i64 32, 4, 0
  %782 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %781, i64 8, 3, 1
  %783 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %782, i64 1, 4, 1
  %784 = call ptr @llvm.stacksave.p0()
  %785 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %783, ptr %785, align 8
  %786 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %785, 1
  %787 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %787, align 8
  %788 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %787, 1
  %789 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %786, ptr %789, align 8
  %790 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %788, ptr %790, align 8
  call void @memrefCopy(i64 4, ptr %789, ptr %790)
  call void @llvm.stackrestore.p0(ptr %784)
  %791 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 887, i32 125)
  %792 = call ptr @llvm.stacksave.p0()
  %793 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %353, ptr %793, align 8
  %794 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %793, 1
  %795 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %362, ptr %795, align 8
  %796 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %795, 1
  %797 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %794, ptr %797, align 8
  %798 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %796, ptr %798, align 8
  call void @memrefCopy(i64 4, ptr %797, ptr %798)
  call void @llvm.stackrestore.p0(ptr %792)
  %799 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %800 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %801 = insertvalue { ptr, ptr, i64 } poison, ptr %799, 0
  %802 = insertvalue { ptr, ptr, i64 } %801, ptr %800, 1
  %803 = insertvalue { ptr, ptr, i64 } %802, i64 0, 2
  %804 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %805 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %806 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %807 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %808 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %809 = extractvalue { ptr, ptr, i64 } %803, 0
  %810 = extractvalue { ptr, ptr, i64 } %803, 1
  %811 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %809, 0
  %812 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %811, ptr %810, 1
  %813 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %812, i64 2064, 2
  %814 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %813, i64 1, 3, 0
  %815 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %814, i64 32, 4, 0
  %816 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %815, i64 8, 3, 1
  %817 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %816, i64 1, 4, 1
  %818 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %817, 3, 0
  %819 = mul i64 1, %818
  %820 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %817, 3, 1
  %821 = mul i64 %819, %820
  %822 = mul i64 %821, 4
  %823 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %817, 1
  %824 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %817, 2
  %825 = getelementptr float, ptr %823, i64 %824
  %826 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %400, 1
  %827 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %400, 2
  %828 = getelementptr float, ptr %826, i64 %827
  call void @llvm.memcpy.p0.p0.i64(ptr %828, ptr %825, i64 %822, i1 false)
  %829 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 1, i32 8, ptr %372, ptr %414, ptr %98, i32 0, i32 956, i32 110)
  %830 = call ptr @llvm.stacksave.p0()
  %831 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %831, align 8
  %832 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %831, 1
  %833 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %648, ptr %833, align 8
  %834 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %833, 1
  %835 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %832, ptr %835, align 8
  %836 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %834, ptr %836, align 8
  call void @memrefCopy(i64 4, ptr %835, ptr %836)
  call void @llvm.stackrestore.p0(ptr %830)
  %837 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %838 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %839 = insertvalue { ptr, ptr, i64 } poison, ptr %837, 0
  %840 = insertvalue { ptr, ptr, i64 } %839, ptr %838, 1
  %841 = insertvalue { ptr, ptr, i64 } %840, i64 0, 2
  %842 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %843 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %844 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %845 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %846 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %847 = extractvalue { ptr, ptr, i64 } %841, 0
  %848 = extractvalue { ptr, ptr, i64 } %841, 1
  %849 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %847, 0
  %850 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %849, ptr %848, 1
  %851 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %850, i64 24, 2
  %852 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %851, i64 8, 3, 0
  %853 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %852, i64 32, 4, 0
  %854 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %853, i64 8, 3, 1
  %855 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %854, i64 1, 4, 1
  %856 = call ptr @llvm.stacksave.p0()
  %857 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %855, ptr %857, align 8
  %858 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %857, 1
  %859 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %859, align 8
  %860 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %859, 1
  %861 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %858, ptr %861, align 8
  %862 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %860, ptr %862, align 8
  call void @memrefCopy(i64 4, ptr %861, ptr %862)
  call void @llvm.stackrestore.p0(ptr %856)
  %863 = call ptr @llvm.stacksave.p0()
  %864 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %117, ptr %864, align 8
  %865 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %864, 1
  %866 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %866, align 8
  %867 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %866, 1
  %868 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %865, ptr %868, align 8
  %869 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %867, ptr %869, align 8
  call void @memrefCopy(i64 4, ptr %868, ptr %869)
  call void @llvm.stackrestore.p0(ptr %863)
  %870 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %871 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %872 = insertvalue { ptr, ptr, i64 } poison, ptr %870, 0
  %873 = insertvalue { ptr, ptr, i64 } %872, ptr %871, 1
  %874 = insertvalue { ptr, ptr, i64 } %873, i64 0, 2
  %875 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %876 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %877 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %878 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %879 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %880 = extractvalue { ptr, ptr, i64 } %874, 0
  %881 = extractvalue { ptr, ptr, i64 } %874, 1
  %882 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %880, 0
  %883 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %882, ptr %881, 1
  %884 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %883, i64 24, 2
  %885 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %884, i64 16, 3, 0
  %886 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %885, i64 32, 4, 0
  %887 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %886, i64 8, 3, 1
  %888 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %887, i64 1, 4, 1
  %889 = call ptr @llvm.stacksave.p0()
  %890 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %888, ptr %890, align 8
  %891 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %890, 1
  %892 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %892, align 8
  %893 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %892, 1
  %894 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %891, ptr %894, align 8
  %895 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %893, ptr %895, align 8
  call void @memrefCopy(i64 4, ptr %894, ptr %895)
  call void @llvm.stackrestore.p0(ptr %889)
  %896 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 1025, i32 125)
  %897 = call ptr @llvm.stacksave.p0()
  %898 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %194, ptr %898, align 8
  %899 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %898, 1
  %900 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %900, align 8
  %901 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %900, 1
  %902 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %899, ptr %902, align 8
  %903 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %901, ptr %903, align 8
  call void @memrefCopy(i64 4, ptr %902, ptr %903)
  call void @llvm.stackrestore.p0(ptr %897)
  %904 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %905 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %906 = insertvalue { ptr, ptr, i64 } poison, ptr %904, 0
  %907 = insertvalue { ptr, ptr, i64 } %906, ptr %905, 1
  %908 = insertvalue { ptr, ptr, i64 } %907, i64 0, 2
  %909 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %910 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %911 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %912 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %913 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %914 = extractvalue { ptr, ptr, i64 } %908, 0
  %915 = extractvalue { ptr, ptr, i64 } %908, 1
  %916 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %914, 0
  %917 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %916, ptr %915, 1
  %918 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %917, i64 536, 2
  %919 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %918, i64 16, 3, 0
  %920 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %919, i64 32, 4, 0
  %921 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %920, i64 8, 3, 1
  %922 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %921, i64 1, 4, 1
  %923 = call ptr @llvm.stacksave.p0()
  %924 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %922, ptr %924, align 8
  %925 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %924, 1
  %926 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %926, align 8
  %927 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %926, 1
  %928 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %925, ptr %928, align 8
  %929 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %927, ptr %929, align 8
  call void @memrefCopy(i64 4, ptr %928, ptr %929)
  call void @llvm.stackrestore.p0(ptr %923)
  %930 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 1094, i32 125)
  %931 = call ptr @llvm.stacksave.p0()
  %932 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %247, ptr %932, align 8
  %933 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %932, 1
  %934 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %934, align 8
  %935 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %934, 1
  %936 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %933, ptr %936, align 8
  %937 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %935, ptr %937, align 8
  call void @memrefCopy(i64 4, ptr %936, ptr %937)
  call void @llvm.stackrestore.p0(ptr %931)
  %938 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %939 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %940 = insertvalue { ptr, ptr, i64 } poison, ptr %938, 0
  %941 = insertvalue { ptr, ptr, i64 } %940, ptr %939, 1
  %942 = insertvalue { ptr, ptr, i64 } %941, i64 0, 2
  %943 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %944 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %945 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %946 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %947 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %948 = extractvalue { ptr, ptr, i64 } %942, 0
  %949 = extractvalue { ptr, ptr, i64 } %942, 1
  %950 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %948, 0
  %951 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %950, ptr %949, 1
  %952 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %951, i64 1048, 2
  %953 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %952, i64 16, 3, 0
  %954 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %953, i64 32, 4, 0
  %955 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %954, i64 8, 3, 1
  %956 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %955, i64 1, 4, 1
  %957 = call ptr @llvm.stacksave.p0()
  %958 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %956, ptr %958, align 8
  %959 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %958, 1
  %960 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %960, align 8
  %961 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %960, 1
  %962 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %959, ptr %962, align 8
  %963 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %961, ptr %963, align 8
  call void @memrefCopy(i64 4, ptr %962, ptr %963)
  call void @llvm.stackrestore.p0(ptr %957)
  %964 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 1163, i32 125)
  %965 = call ptr @llvm.stacksave.p0()
  %966 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %300, ptr %966, align 8
  %967 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %966, 1
  %968 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %968, align 8
  %969 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %968, 1
  %970 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %967, ptr %970, align 8
  %971 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %969, ptr %971, align 8
  call void @memrefCopy(i64 4, ptr %970, ptr %971)
  call void @llvm.stackrestore.p0(ptr %965)
  %972 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %973 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %974 = insertvalue { ptr, ptr, i64 } poison, ptr %972, 0
  %975 = insertvalue { ptr, ptr, i64 } %974, ptr %973, 1
  %976 = insertvalue { ptr, ptr, i64 } %975, i64 0, 2
  %977 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %978 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %979 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %980 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %981 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %982 = extractvalue { ptr, ptr, i64 } %976, 0
  %983 = extractvalue { ptr, ptr, i64 } %976, 1
  %984 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %982, 0
  %985 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %984, ptr %983, 1
  %986 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %985, i64 1560, 2
  %987 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %986, i64 16, 3, 0
  %988 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %987, i64 32, 4, 0
  %989 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %988, i64 8, 3, 1
  %990 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %989, i64 1, 4, 1
  %991 = call ptr @llvm.stacksave.p0()
  %992 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %990, ptr %992, align 8
  %993 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %992, 1
  %994 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %994, align 8
  %995 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %994, 1
  %996 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %993, ptr %996, align 8
  %997 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %995, ptr %997, align 8
  call void @memrefCopy(i64 4, ptr %996, ptr %997)
  call void @llvm.stackrestore.p0(ptr %991)
  %998 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 1232, i32 125)
  %999 = call ptr @llvm.stacksave.p0()
  %1000 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %353, ptr %1000, align 8
  %1001 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1000, 1
  %1002 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %362, ptr %1002, align 8
  %1003 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1002, 1
  %1004 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1001, ptr %1004, align 8
  %1005 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1003, ptr %1005, align 8
  call void @memrefCopy(i64 4, ptr %1004, ptr %1005)
  call void @llvm.stackrestore.p0(ptr %999)
  %1006 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %1007 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %1008 = insertvalue { ptr, ptr, i64 } poison, ptr %1006, 0
  %1009 = insertvalue { ptr, ptr, i64 } %1008, ptr %1007, 1
  %1010 = insertvalue { ptr, ptr, i64 } %1009, i64 0, 2
  %1011 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %1012 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %1013 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %1014 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %1015 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %1016 = extractvalue { ptr, ptr, i64 } %1010, 0
  %1017 = extractvalue { ptr, ptr, i64 } %1010, 1
  %1018 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1016, 0
  %1019 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1018, ptr %1017, 1
  %1020 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1019, i64 2072, 2
  %1021 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1020, i64 1, 3, 0
  %1022 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1021, i64 32, 4, 0
  %1023 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1022, i64 8, 3, 1
  %1024 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1023, i64 1, 4, 1
  %1025 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1024, 3, 0
  %1026 = mul i64 1, %1025
  %1027 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1024, 3, 1
  %1028 = mul i64 %1026, %1027
  %1029 = mul i64 %1028, 4
  %1030 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1024, 1
  %1031 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1024, 2
  %1032 = getelementptr float, ptr %1030, i64 %1031
  %1033 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %400, 1
  %1034 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %400, 2
  %1035 = getelementptr float, ptr %1033, i64 %1034
  call void @llvm.memcpy.p0.p0.i64(ptr %1035, ptr %1032, i64 %1029, i1 false)
  %1036 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 1, i32 8, ptr %372, ptr %414, ptr %98, i32 0, i32 1301, i32 110)
  %1037 = call ptr @llvm.stacksave.p0()
  %1038 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %1038, align 8
  %1039 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1038, 1
  %1040 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %855, ptr %1040, align 8
  %1041 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1040, 1
  %1042 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1039, ptr %1042, align 8
  %1043 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1041, ptr %1043, align 8
  call void @memrefCopy(i64 4, ptr %1042, ptr %1043)
  call void @llvm.stackrestore.p0(ptr %1037)
  %1044 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %1045 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %1046 = insertvalue { ptr, ptr, i64 } poison, ptr %1044, 0
  %1047 = insertvalue { ptr, ptr, i64 } %1046, ptr %1045, 1
  %1048 = insertvalue { ptr, ptr, i64 } %1047, i64 0, 2
  %1049 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %1050 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %1051 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %1052 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %1053 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %1054 = extractvalue { ptr, ptr, i64 } %1048, 0
  %1055 = extractvalue { ptr, ptr, i64 } %1048, 1
  %1056 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1054, 0
  %1057 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1056, ptr %1055, 1
  %1058 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1057, i64 256, 2
  %1059 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1058, i64 8, 3, 0
  %1060 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1059, i64 32, 4, 0
  %1061 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1060, i64 8, 3, 1
  %1062 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1061, i64 1, 4, 1
  %1063 = call ptr @llvm.stacksave.p0()
  %1064 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1062, ptr %1064, align 8
  %1065 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1064, 1
  %1066 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %1066, align 8
  %1067 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1066, 1
  %1068 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1065, ptr %1068, align 8
  %1069 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1067, ptr %1069, align 8
  call void @memrefCopy(i64 4, ptr %1068, ptr %1069)
  call void @llvm.stackrestore.p0(ptr %1063)
  %1070 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %1071 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %1072 = insertvalue { ptr, ptr, i64 } poison, ptr %1070, 0
  %1073 = insertvalue { ptr, ptr, i64 } %1072, ptr %1071, 1
  %1074 = insertvalue { ptr, ptr, i64 } %1073, i64 0, 2
  %1075 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %1076 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %1077 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %1078 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %1079 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %1080 = extractvalue { ptr, ptr, i64 } %1074, 0
  %1081 = extractvalue { ptr, ptr, i64 } %1074, 1
  %1082 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1080, 0
  %1083 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1082, ptr %1081, 1
  %1084 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1083, i64 520, 2
  %1085 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1084, i64 8, 3, 0
  %1086 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1085, i64 65, 4, 0
  %1087 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1086, i64 16, 3, 1
  %1088 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1087, i64 1, 4, 1
  %1089 = call ptr @llvm.stacksave.p0()
  %1090 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1088, ptr %1090, align 8
  %1091 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1090, 1
  %1092 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %1092, align 8
  %1093 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1092, 1
  %1094 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1091, ptr %1094, align 8
  %1095 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1093, ptr %1095, align 8
  call void @memrefCopy(i64 4, ptr %1094, ptr %1095)
  call void @llvm.stackrestore.p0(ptr %1089)
  %1096 = call ptr @llvm.stacksave.p0()
  %1097 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %155, ptr %1097, align 8
  %1098 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1097, 1
  %1099 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %1099, align 8
  %1100 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1099, 1
  %1101 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1098, ptr %1101, align 8
  %1102 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1100, ptr %1102, align 8
  call void @memrefCopy(i64 4, ptr %1101, ptr %1102)
  call void @llvm.stackrestore.p0(ptr %1096)
  %1103 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 1370, i32 125)
  %1104 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %1105 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %1106 = insertvalue { ptr, ptr, i64 } poison, ptr %1104, 0
  %1107 = insertvalue { ptr, ptr, i64 } %1106, ptr %1105, 1
  %1108 = insertvalue { ptr, ptr, i64 } %1107, i64 0, 2
  %1109 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %1110 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %1111 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %1112 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %1113 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %1114 = extractvalue { ptr, ptr, i64 } %1108, 0
  %1115 = extractvalue { ptr, ptr, i64 } %1108, 1
  %1116 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1114, 0
  %1117 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1116, ptr %1115, 1
  %1118 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1117, i64 536, 2
  %1119 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1118, i64 8, 3, 0
  %1120 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1119, i64 65, 4, 0
  %1121 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1120, i64 16, 3, 1
  %1122 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1121, i64 1, 4, 1
  %1123 = call ptr @llvm.stacksave.p0()
  %1124 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1122, ptr %1124, align 8
  %1125 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1124, 1
  %1126 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %1126, align 8
  %1127 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1126, 1
  %1128 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1125, ptr %1128, align 8
  %1129 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1127, ptr %1129, align 8
  call void @memrefCopy(i64 4, ptr %1128, ptr %1129)
  call void @llvm.stackrestore.p0(ptr %1123)
  %1130 = call ptr @llvm.stacksave.p0()
  %1131 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %220, ptr %1131, align 8
  %1132 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1131, 1
  %1133 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %1133, align 8
  %1134 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1133, 1
  %1135 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1132, ptr %1135, align 8
  %1136 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1134, ptr %1136, align 8
  call void @memrefCopy(i64 4, ptr %1135, ptr %1136)
  call void @llvm.stackrestore.p0(ptr %1130)
  %1137 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 1439, i32 125)
  %1138 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %1139 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %1140 = insertvalue { ptr, ptr, i64 } poison, ptr %1138, 0
  %1141 = insertvalue { ptr, ptr, i64 } %1140, ptr %1139, 1
  %1142 = insertvalue { ptr, ptr, i64 } %1141, i64 0, 2
  %1143 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %1144 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %1145 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %1146 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %1147 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %1148 = extractvalue { ptr, ptr, i64 } %1142, 0
  %1149 = extractvalue { ptr, ptr, i64 } %1142, 1
  %1150 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1148, 0
  %1151 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1150, ptr %1149, 1
  %1152 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1151, i64 552, 2
  %1153 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1152, i64 8, 3, 0
  %1154 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1153, i64 65, 4, 0
  %1155 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1154, i64 16, 3, 1
  %1156 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1155, i64 1, 4, 1
  %1157 = call ptr @llvm.stacksave.p0()
  %1158 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1156, ptr %1158, align 8
  %1159 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1158, 1
  %1160 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %1160, align 8
  %1161 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1160, 1
  %1162 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1159, ptr %1162, align 8
  %1163 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1161, ptr %1163, align 8
  call void @memrefCopy(i64 4, ptr %1162, ptr %1163)
  call void @llvm.stackrestore.p0(ptr %1157)
  %1164 = call ptr @llvm.stacksave.p0()
  %1165 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %273, ptr %1165, align 8
  %1166 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1165, 1
  %1167 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %1167, align 8
  %1168 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1167, 1
  %1169 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1166, ptr %1169, align 8
  %1170 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1168, ptr %1170, align 8
  call void @memrefCopy(i64 4, ptr %1169, ptr %1170)
  call void @llvm.stackrestore.p0(ptr %1164)
  %1171 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 1508, i32 125)
  %1172 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %1173 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %1174 = insertvalue { ptr, ptr, i64 } poison, ptr %1172, 0
  %1175 = insertvalue { ptr, ptr, i64 } %1174, ptr %1173, 1
  %1176 = insertvalue { ptr, ptr, i64 } %1175, i64 0, 2
  %1177 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %1178 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %1179 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %1180 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %1181 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %1182 = extractvalue { ptr, ptr, i64 } %1176, 0
  %1183 = extractvalue { ptr, ptr, i64 } %1176, 1
  %1184 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1182, 0
  %1185 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1184, ptr %1183, 1
  %1186 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1185, i64 568, 2
  %1187 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1186, i64 8, 3, 0
  %1188 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1187, i64 65, 4, 0
  %1189 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1188, i64 16, 3, 1
  %1190 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1189, i64 1, 4, 1
  %1191 = call ptr @llvm.stacksave.p0()
  %1192 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1190, ptr %1192, align 8
  %1193 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1192, 1
  %1194 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %1194, align 8
  %1195 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1194, 1
  %1196 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1193, ptr %1196, align 8
  %1197 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1195, ptr %1197, align 8
  call void @memrefCopy(i64 4, ptr %1196, ptr %1197)
  call void @llvm.stackrestore.p0(ptr %1191)
  %1198 = call ptr @llvm.stacksave.p0()
  %1199 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %326, ptr %1199, align 8
  %1200 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1199, 1
  %1201 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %1201, align 8
  %1202 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1201, 1
  %1203 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1200, ptr %1203, align 8
  %1204 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1202, ptr %1204, align 8
  call void @memrefCopy(i64 4, ptr %1203, ptr %1204)
  call void @llvm.stackrestore.p0(ptr %1198)
  %1205 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 1577, i32 125)
  %1206 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %1207 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %1208 = insertvalue { ptr, ptr, i64 } poison, ptr %1206, 0
  %1209 = insertvalue { ptr, ptr, i64 } %1208, ptr %1207, 1
  %1210 = insertvalue { ptr, ptr, i64 } %1209, i64 0, 2
  %1211 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %1212 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %1213 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %1214 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %1215 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %1216 = extractvalue { ptr, ptr, i64 } %1210, 0
  %1217 = extractvalue { ptr, ptr, i64 } %1210, 1
  %1218 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1216, 0
  %1219 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1218, ptr %1217, 1
  %1220 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1219, i64 584, 2
  %1221 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1220, i64 8, 3, 0
  %1222 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1221, i64 65, 4, 0
  %1223 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1222, i64 1, 3, 1
  %1224 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1223, i64 1, 4, 1
  %1225 = call ptr @llvm.stacksave.p0()
  %1226 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1224, ptr %1226, align 8
  %1227 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1226, 1
  %1228 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %362, ptr %1228, align 8
  %1229 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1228, 1
  %1230 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1227, ptr %1230, align 8
  %1231 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1229, ptr %1231, align 8
  call void @memrefCopy(i64 4, ptr %1230, ptr %1231)
  call void @llvm.stackrestore.p0(ptr %1225)
  %1232 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %391, 3, 0
  %1233 = mul i64 1, %1232
  %1234 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %391, 3, 1
  %1235 = mul i64 %1233, %1234
  %1236 = mul i64 %1235, 4
  %1237 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %391, 1
  %1238 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %391, 2
  %1239 = getelementptr float, ptr %1237, i64 %1238
  %1240 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %400, 1
  %1241 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %400, 2
  %1242 = getelementptr float, ptr %1240, i64 %1241
  call void @llvm.memcpy.p0.p0.i64(ptr %1242, ptr %1239, i64 %1236, i1 false)
  %1243 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 1, i32 8, ptr %372, ptr %414, ptr %98, i32 0, i32 1646, i32 110)
  %1244 = call ptr @llvm.stacksave.p0()
  %1245 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %1245, align 8
  %1246 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1245, 1
  %1247 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1062, ptr %1247, align 8
  %1248 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1247, 1
  %1249 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1246, ptr %1249, align 8
  %1250 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1248, ptr %1250, align 8
  call void @memrefCopy(i64 4, ptr %1249, ptr %1250)
  call void @llvm.stackrestore.p0(ptr %1244)
  %1251 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %1252 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %1253 = insertvalue { ptr, ptr, i64 } poison, ptr %1251, 0
  %1254 = insertvalue { ptr, ptr, i64 } %1253, ptr %1252, 1
  %1255 = insertvalue { ptr, ptr, i64 } %1254, i64 0, 2
  %1256 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %1257 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %1258 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %1259 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %1260 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %1261 = extractvalue { ptr, ptr, i64 } %1255, 0
  %1262 = extractvalue { ptr, ptr, i64 } %1255, 1
  %1263 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1261, 0
  %1264 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1263, ptr %1262, 1
  %1265 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1264, i64 264, 2
  %1266 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1265, i64 8, 3, 0
  %1267 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1266, i64 32, 4, 0
  %1268 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1267, i64 8, 3, 1
  %1269 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1268, i64 1, 4, 1
  %1270 = call ptr @llvm.stacksave.p0()
  %1271 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1269, ptr %1271, align 8
  %1272 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1271, 1
  %1273 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %1273, align 8
  %1274 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1273, 1
  %1275 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1272, ptr %1275, align 8
  %1276 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1274, ptr %1276, align 8
  call void @memrefCopy(i64 4, ptr %1275, ptr %1276)
  call void @llvm.stackrestore.p0(ptr %1270)
  %1277 = call ptr @llvm.stacksave.p0()
  %1278 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1088, ptr %1278, align 8
  %1279 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1278, 1
  %1280 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %1280, align 8
  %1281 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1280, 1
  %1282 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1279, ptr %1282, align 8
  %1283 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1281, ptr %1283, align 8
  call void @memrefCopy(i64 4, ptr %1282, ptr %1283)
  call void @llvm.stackrestore.p0(ptr %1277)
  %1284 = call ptr @llvm.stacksave.p0()
  %1285 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %474, ptr %1285, align 8
  %1286 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1285, 1
  %1287 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %1287, align 8
  %1288 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1287, 1
  %1289 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1286, ptr %1289, align 8
  %1290 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1288, ptr %1290, align 8
  call void @memrefCopy(i64 4, ptr %1289, ptr %1290)
  call void @llvm.stackrestore.p0(ptr %1284)
  %1291 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 1715, i32 125)
  %1292 = call ptr @llvm.stacksave.p0()
  %1293 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1122, ptr %1293, align 8
  %1294 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1293, 1
  %1295 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %1295, align 8
  %1296 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1295, 1
  %1297 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1294, ptr %1297, align 8
  %1298 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1296, ptr %1298, align 8
  call void @memrefCopy(i64 4, ptr %1297, ptr %1298)
  call void @llvm.stackrestore.p0(ptr %1292)
  %1299 = call ptr @llvm.stacksave.p0()
  %1300 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %508, ptr %1300, align 8
  %1301 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1300, 1
  %1302 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %1302, align 8
  %1303 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1302, 1
  %1304 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1301, ptr %1304, align 8
  %1305 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1303, ptr %1305, align 8
  call void @memrefCopy(i64 4, ptr %1304, ptr %1305)
  call void @llvm.stackrestore.p0(ptr %1299)
  %1306 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 1784, i32 125)
  %1307 = call ptr @llvm.stacksave.p0()
  %1308 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1156, ptr %1308, align 8
  %1309 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1308, 1
  %1310 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %1310, align 8
  %1311 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1310, 1
  %1312 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1309, ptr %1312, align 8
  %1313 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1311, ptr %1313, align 8
  call void @memrefCopy(i64 4, ptr %1312, ptr %1313)
  call void @llvm.stackrestore.p0(ptr %1307)
  %1314 = call ptr @llvm.stacksave.p0()
  %1315 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %542, ptr %1315, align 8
  %1316 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1315, 1
  %1317 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %1317, align 8
  %1318 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1317, 1
  %1319 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1316, ptr %1319, align 8
  %1320 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1318, ptr %1320, align 8
  call void @memrefCopy(i64 4, ptr %1319, ptr %1320)
  call void @llvm.stackrestore.p0(ptr %1314)
  %1321 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 1853, i32 125)
  %1322 = call ptr @llvm.stacksave.p0()
  %1323 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1190, ptr %1323, align 8
  %1324 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1323, 1
  %1325 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %1325, align 8
  %1326 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1325, 1
  %1327 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1324, ptr %1327, align 8
  %1328 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1326, ptr %1328, align 8
  call void @memrefCopy(i64 4, ptr %1327, ptr %1328)
  call void @llvm.stackrestore.p0(ptr %1322)
  %1329 = call ptr @llvm.stacksave.p0()
  %1330 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %576, ptr %1330, align 8
  %1331 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1330, 1
  %1332 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %1332, align 8
  %1333 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1332, 1
  %1334 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1331, ptr %1334, align 8
  %1335 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1333, ptr %1335, align 8
  call void @memrefCopy(i64 4, ptr %1334, ptr %1335)
  call void @llvm.stackrestore.p0(ptr %1329)
  %1336 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 1922, i32 125)
  %1337 = call ptr @llvm.stacksave.p0()
  %1338 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1224, ptr %1338, align 8
  %1339 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1338, 1
  %1340 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %362, ptr %1340, align 8
  %1341 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1340, 1
  %1342 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1339, ptr %1342, align 8
  %1343 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1341, ptr %1343, align 8
  call void @memrefCopy(i64 4, ptr %1342, ptr %1343)
  call void @llvm.stackrestore.p0(ptr %1337)
  %1344 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %610, 3, 0
  %1345 = mul i64 1, %1344
  %1346 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %610, 3, 1
  %1347 = mul i64 %1345, %1346
  %1348 = mul i64 %1347, 4
  %1349 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %610, 1
  %1350 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %610, 2
  %1351 = getelementptr float, ptr %1349, i64 %1350
  %1352 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %400, 1
  %1353 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %400, 2
  %1354 = getelementptr float, ptr %1352, i64 %1353
  call void @llvm.memcpy.p0.p0.i64(ptr %1354, ptr %1351, i64 %1348, i1 false)
  %1355 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 1, i32 8, ptr %372, ptr %414, ptr %98, i32 0, i32 1991, i32 110)
  %1356 = call ptr @llvm.stacksave.p0()
  %1357 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %1357, align 8
  %1358 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1357, 1
  %1359 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1269, ptr %1359, align 8
  %1360 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1359, 1
  %1361 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1358, ptr %1361, align 8
  %1362 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1360, ptr %1362, align 8
  call void @memrefCopy(i64 4, ptr %1361, ptr %1362)
  call void @llvm.stackrestore.p0(ptr %1356)
  %1363 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %1364 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %1365 = insertvalue { ptr, ptr, i64 } poison, ptr %1363, 0
  %1366 = insertvalue { ptr, ptr, i64 } %1365, ptr %1364, 1
  %1367 = insertvalue { ptr, ptr, i64 } %1366, i64 0, 2
  %1368 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %1369 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %1370 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %1371 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %1372 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %1373 = extractvalue { ptr, ptr, i64 } %1367, 0
  %1374 = extractvalue { ptr, ptr, i64 } %1367, 1
  %1375 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1373, 0
  %1376 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1375, ptr %1374, 1
  %1377 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1376, i64 272, 2
  %1378 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1377, i64 8, 3, 0
  %1379 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1378, i64 32, 4, 0
  %1380 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1379, i64 8, 3, 1
  %1381 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1380, i64 1, 4, 1
  %1382 = call ptr @llvm.stacksave.p0()
  %1383 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1381, ptr %1383, align 8
  %1384 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1383, 1
  %1385 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %1385, align 8
  %1386 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1385, 1
  %1387 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1384, ptr %1387, align 8
  %1388 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1386, ptr %1388, align 8
  call void @memrefCopy(i64 4, ptr %1387, ptr %1388)
  call void @llvm.stackrestore.p0(ptr %1382)
  %1389 = call ptr @llvm.stacksave.p0()
  %1390 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1088, ptr %1390, align 8
  %1391 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1390, 1
  %1392 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %1392, align 8
  %1393 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1392, 1
  %1394 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1391, ptr %1394, align 8
  %1395 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1393, ptr %1395, align 8
  call void @memrefCopy(i64 4, ptr %1394, ptr %1395)
  call void @llvm.stackrestore.p0(ptr %1389)
  %1396 = call ptr @llvm.stacksave.p0()
  %1397 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %681, ptr %1397, align 8
  %1398 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1397, 1
  %1399 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %1399, align 8
  %1400 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1399, 1
  %1401 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1398, ptr %1401, align 8
  %1402 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1400, ptr %1402, align 8
  call void @memrefCopy(i64 4, ptr %1401, ptr %1402)
  call void @llvm.stackrestore.p0(ptr %1396)
  %1403 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 2060, i32 125)
  %1404 = call ptr @llvm.stacksave.p0()
  %1405 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1122, ptr %1405, align 8
  %1406 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1405, 1
  %1407 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %1407, align 8
  %1408 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1407, 1
  %1409 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1406, ptr %1409, align 8
  %1410 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1408, ptr %1410, align 8
  call void @memrefCopy(i64 4, ptr %1409, ptr %1410)
  call void @llvm.stackrestore.p0(ptr %1404)
  %1411 = call ptr @llvm.stacksave.p0()
  %1412 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %715, ptr %1412, align 8
  %1413 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1412, 1
  %1414 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %1414, align 8
  %1415 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1414, 1
  %1416 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1413, ptr %1416, align 8
  %1417 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1415, ptr %1417, align 8
  call void @memrefCopy(i64 4, ptr %1416, ptr %1417)
  call void @llvm.stackrestore.p0(ptr %1411)
  %1418 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 2129, i32 125)
  %1419 = call ptr @llvm.stacksave.p0()
  %1420 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1156, ptr %1420, align 8
  %1421 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1420, 1
  %1422 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %1422, align 8
  %1423 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1422, 1
  %1424 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1421, ptr %1424, align 8
  %1425 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1423, ptr %1425, align 8
  call void @memrefCopy(i64 4, ptr %1424, ptr %1425)
  call void @llvm.stackrestore.p0(ptr %1419)
  %1426 = call ptr @llvm.stacksave.p0()
  %1427 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %749, ptr %1427, align 8
  %1428 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1427, 1
  %1429 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %1429, align 8
  %1430 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1429, 1
  %1431 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1428, ptr %1431, align 8
  %1432 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1430, ptr %1432, align 8
  call void @memrefCopy(i64 4, ptr %1431, ptr %1432)
  call void @llvm.stackrestore.p0(ptr %1426)
  %1433 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 2198, i32 125)
  %1434 = call ptr @llvm.stacksave.p0()
  %1435 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1190, ptr %1435, align 8
  %1436 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1435, 1
  %1437 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %1437, align 8
  %1438 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1437, 1
  %1439 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1436, ptr %1439, align 8
  %1440 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1438, ptr %1440, align 8
  call void @memrefCopy(i64 4, ptr %1439, ptr %1440)
  call void @llvm.stackrestore.p0(ptr %1434)
  %1441 = call ptr @llvm.stacksave.p0()
  %1442 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %783, ptr %1442, align 8
  %1443 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1442, 1
  %1444 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %1444, align 8
  %1445 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1444, 1
  %1446 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1443, ptr %1446, align 8
  %1447 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1445, ptr %1447, align 8
  call void @memrefCopy(i64 4, ptr %1446, ptr %1447)
  call void @llvm.stackrestore.p0(ptr %1441)
  %1448 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 2267, i32 125)
  %1449 = call ptr @llvm.stacksave.p0()
  %1450 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1224, ptr %1450, align 8
  %1451 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1450, 1
  %1452 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %362, ptr %1452, align 8
  %1453 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1452, 1
  %1454 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1451, ptr %1454, align 8
  %1455 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1453, ptr %1455, align 8
  call void @memrefCopy(i64 4, ptr %1454, ptr %1455)
  call void @llvm.stackrestore.p0(ptr %1449)
  %1456 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %817, 3, 0
  %1457 = mul i64 1, %1456
  %1458 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %817, 3, 1
  %1459 = mul i64 %1457, %1458
  %1460 = mul i64 %1459, 4
  %1461 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %817, 1
  %1462 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %817, 2
  %1463 = getelementptr float, ptr %1461, i64 %1462
  %1464 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %400, 1
  %1465 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %400, 2
  %1466 = getelementptr float, ptr %1464, i64 %1465
  call void @llvm.memcpy.p0.p0.i64(ptr %1466, ptr %1463, i64 %1460, i1 false)
  %1467 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 1, i32 8, ptr %372, ptr %414, ptr %98, i32 0, i32 2336, i32 110)
  %1468 = call ptr @llvm.stacksave.p0()
  %1469 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %1469, align 8
  %1470 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1469, 1
  %1471 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1381, ptr %1471, align 8
  %1472 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1471, 1
  %1473 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1470, ptr %1473, align 8
  %1474 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1472, ptr %1474, align 8
  call void @memrefCopy(i64 4, ptr %1473, ptr %1474)
  call void @llvm.stackrestore.p0(ptr %1468)
  %1475 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %1476 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %1477 = insertvalue { ptr, ptr, i64 } poison, ptr %1475, 0
  %1478 = insertvalue { ptr, ptr, i64 } %1477, ptr %1476, 1
  %1479 = insertvalue { ptr, ptr, i64 } %1478, i64 0, 2
  %1480 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %1481 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %1482 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %1483 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %1484 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %1485 = extractvalue { ptr, ptr, i64 } %1479, 0
  %1486 = extractvalue { ptr, ptr, i64 } %1479, 1
  %1487 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1485, 0
  %1488 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1487, ptr %1486, 1
  %1489 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1488, i64 280, 2
  %1490 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1489, i64 8, 3, 0
  %1491 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1490, i64 32, 4, 0
  %1492 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1491, i64 8, 3, 1
  %1493 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1492, i64 1, 4, 1
  %1494 = call ptr @llvm.stacksave.p0()
  %1495 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1493, ptr %1495, align 8
  %1496 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1495, 1
  %1497 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %1497, align 8
  %1498 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1497, 1
  %1499 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1496, ptr %1499, align 8
  %1500 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1498, ptr %1500, align 8
  call void @memrefCopy(i64 4, ptr %1499, ptr %1500)
  call void @llvm.stackrestore.p0(ptr %1494)
  %1501 = call ptr @llvm.stacksave.p0()
  %1502 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1088, ptr %1502, align 8
  %1503 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1502, 1
  %1504 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %1504, align 8
  %1505 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1504, 1
  %1506 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1503, ptr %1506, align 8
  %1507 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1505, ptr %1507, align 8
  call void @memrefCopy(i64 4, ptr %1506, ptr %1507)
  call void @llvm.stackrestore.p0(ptr %1501)
  %1508 = call ptr @llvm.stacksave.p0()
  %1509 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %888, ptr %1509, align 8
  %1510 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1509, 1
  %1511 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %1511, align 8
  %1512 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1511, 1
  %1513 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1510, ptr %1513, align 8
  %1514 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1512, ptr %1514, align 8
  call void @memrefCopy(i64 4, ptr %1513, ptr %1514)
  call void @llvm.stackrestore.p0(ptr %1508)
  %1515 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 2405, i32 125)
  %1516 = call ptr @llvm.stacksave.p0()
  %1517 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1122, ptr %1517, align 8
  %1518 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1517, 1
  %1519 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %1519, align 8
  %1520 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1519, 1
  %1521 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1518, ptr %1521, align 8
  %1522 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1520, ptr %1522, align 8
  call void @memrefCopy(i64 4, ptr %1521, ptr %1522)
  call void @llvm.stackrestore.p0(ptr %1516)
  %1523 = call ptr @llvm.stacksave.p0()
  %1524 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %922, ptr %1524, align 8
  %1525 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1524, 1
  %1526 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %1526, align 8
  %1527 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1526, 1
  %1528 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1525, ptr %1528, align 8
  %1529 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1527, ptr %1529, align 8
  call void @memrefCopy(i64 4, ptr %1528, ptr %1529)
  call void @llvm.stackrestore.p0(ptr %1523)
  %1530 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 2474, i32 125)
  %1531 = call ptr @llvm.stacksave.p0()
  %1532 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1156, ptr %1532, align 8
  %1533 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1532, 1
  %1534 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %1534, align 8
  %1535 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1534, 1
  %1536 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1533, ptr %1536, align 8
  %1537 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1535, ptr %1537, align 8
  call void @memrefCopy(i64 4, ptr %1536, ptr %1537)
  call void @llvm.stackrestore.p0(ptr %1531)
  %1538 = call ptr @llvm.stacksave.p0()
  %1539 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %956, ptr %1539, align 8
  %1540 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1539, 1
  %1541 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %1541, align 8
  %1542 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1541, 1
  %1543 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1540, ptr %1543, align 8
  %1544 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1542, ptr %1544, align 8
  call void @memrefCopy(i64 4, ptr %1543, ptr %1544)
  call void @llvm.stackrestore.p0(ptr %1538)
  %1545 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 2543, i32 125)
  %1546 = call ptr @llvm.stacksave.p0()
  %1547 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1190, ptr %1547, align 8
  %1548 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1547, 1
  %1549 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %1549, align 8
  %1550 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1549, 1
  %1551 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1548, ptr %1551, align 8
  %1552 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1550, ptr %1552, align 8
  call void @memrefCopy(i64 4, ptr %1551, ptr %1552)
  call void @llvm.stackrestore.p0(ptr %1546)
  %1553 = call ptr @llvm.stacksave.p0()
  %1554 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %990, ptr %1554, align 8
  %1555 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1554, 1
  %1556 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %1556, align 8
  %1557 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1556, 1
  %1558 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1555, ptr %1558, align 8
  %1559 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1557, ptr %1559, align 8
  call void @memrefCopy(i64 4, ptr %1558, ptr %1559)
  call void @llvm.stackrestore.p0(ptr %1553)
  %1560 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 2612, i32 125)
  %1561 = call ptr @llvm.stacksave.p0()
  %1562 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1224, ptr %1562, align 8
  %1563 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1562, 1
  %1564 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %362, ptr %1564, align 8
  %1565 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1564, 1
  %1566 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1563, ptr %1566, align 8
  %1567 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1565, ptr %1567, align 8
  call void @memrefCopy(i64 4, ptr %1566, ptr %1567)
  call void @llvm.stackrestore.p0(ptr %1561)
  %1568 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1024, 3, 0
  %1569 = mul i64 1, %1568
  %1570 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1024, 3, 1
  %1571 = mul i64 %1569, %1570
  %1572 = mul i64 %1571, 4
  %1573 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1024, 1
  %1574 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1024, 2
  %1575 = getelementptr float, ptr %1573, i64 %1574
  %1576 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %400, 1
  %1577 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %400, 2
  %1578 = getelementptr float, ptr %1576, i64 %1577
  call void @llvm.memcpy.p0.p0.i64(ptr %1578, ptr %1575, i64 %1572, i1 false)
  %1579 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 1, i32 8, ptr %372, ptr %414, ptr %98, i32 0, i32 2681, i32 110)
  %1580 = call ptr @llvm.stacksave.p0()
  %1581 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %1581, align 8
  %1582 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1581, 1
  %1583 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1493, ptr %1583, align 8
  %1584 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1583, 1
  %1585 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1582, ptr %1585, align 8
  %1586 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1584, ptr %1586, align 8
  call void @memrefCopy(i64 4, ptr %1585, ptr %1586)
  call void @llvm.stackrestore.p0(ptr %1580)
  %1587 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %1588 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %1589 = insertvalue { ptr, ptr, i64 } poison, ptr %1587, 0
  %1590 = insertvalue { ptr, ptr, i64 } %1589, ptr %1588, 1
  %1591 = insertvalue { ptr, ptr, i64 } %1590, i64 0, 2
  %1592 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %1593 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %1594 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %1595 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %1596 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %1597 = extractvalue { ptr, ptr, i64 } %1591, 0
  %1598 = extractvalue { ptr, ptr, i64 } %1591, 1
  %1599 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1597, 0
  %1600 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1599, ptr %1598, 1
  %1601 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1600, i64 512, 2
  %1602 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1601, i64 8, 3, 0
  %1603 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1602, i64 32, 4, 0
  %1604 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1603, i64 8, 3, 1
  %1605 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1604, i64 1, 4, 1
  %1606 = call ptr @llvm.stacksave.p0()
  %1607 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1605, ptr %1607, align 8
  %1608 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1607, 1
  %1609 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %1609, align 8
  %1610 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1609, 1
  %1611 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1608, ptr %1611, align 8
  %1612 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1610, ptr %1612, align 8
  call void @memrefCopy(i64 4, ptr %1611, ptr %1612)
  call void @llvm.stackrestore.p0(ptr %1606)
  %1613 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %1614 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %1615 = insertvalue { ptr, ptr, i64 } poison, ptr %1613, 0
  %1616 = insertvalue { ptr, ptr, i64 } %1615, ptr %1614, 1
  %1617 = insertvalue { ptr, ptr, i64 } %1616, i64 0, 2
  %1618 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %1619 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %1620 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %1621 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %1622 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %1623 = extractvalue { ptr, ptr, i64 } %1617, 0
  %1624 = extractvalue { ptr, ptr, i64 } %1617, 1
  %1625 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1623, 0
  %1626 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1625, ptr %1624, 1
  %1627 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1626, i64 1040, 2
  %1628 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1627, i64 8, 3, 0
  %1629 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1628, i64 65, 4, 0
  %1630 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1629, i64 16, 3, 1
  %1631 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1630, i64 1, 4, 1
  %1632 = call ptr @llvm.stacksave.p0()
  %1633 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1631, ptr %1633, align 8
  %1634 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1633, 1
  %1635 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %1635, align 8
  %1636 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1635, 1
  %1637 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1634, ptr %1637, align 8
  %1638 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1636, ptr %1638, align 8
  call void @memrefCopy(i64 4, ptr %1637, ptr %1638)
  call void @llvm.stackrestore.p0(ptr %1632)
  %1639 = call ptr @llvm.stacksave.p0()
  %1640 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %155, ptr %1640, align 8
  %1641 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1640, 1
  %1642 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %1642, align 8
  %1643 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1642, 1
  %1644 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1641, ptr %1644, align 8
  %1645 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1643, ptr %1645, align 8
  call void @memrefCopy(i64 4, ptr %1644, ptr %1645)
  call void @llvm.stackrestore.p0(ptr %1639)
  %1646 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 2750, i32 125)
  %1647 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %1648 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %1649 = insertvalue { ptr, ptr, i64 } poison, ptr %1647, 0
  %1650 = insertvalue { ptr, ptr, i64 } %1649, ptr %1648, 1
  %1651 = insertvalue { ptr, ptr, i64 } %1650, i64 0, 2
  %1652 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %1653 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %1654 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %1655 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %1656 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %1657 = extractvalue { ptr, ptr, i64 } %1651, 0
  %1658 = extractvalue { ptr, ptr, i64 } %1651, 1
  %1659 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1657, 0
  %1660 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1659, ptr %1658, 1
  %1661 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1660, i64 1056, 2
  %1662 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1661, i64 8, 3, 0
  %1663 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1662, i64 65, 4, 0
  %1664 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1663, i64 16, 3, 1
  %1665 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1664, i64 1, 4, 1
  %1666 = call ptr @llvm.stacksave.p0()
  %1667 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1665, ptr %1667, align 8
  %1668 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1667, 1
  %1669 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %1669, align 8
  %1670 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1669, 1
  %1671 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1668, ptr %1671, align 8
  %1672 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1670, ptr %1672, align 8
  call void @memrefCopy(i64 4, ptr %1671, ptr %1672)
  call void @llvm.stackrestore.p0(ptr %1666)
  %1673 = call ptr @llvm.stacksave.p0()
  %1674 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %220, ptr %1674, align 8
  %1675 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1674, 1
  %1676 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %1676, align 8
  %1677 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1676, 1
  %1678 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1675, ptr %1678, align 8
  %1679 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1677, ptr %1679, align 8
  call void @memrefCopy(i64 4, ptr %1678, ptr %1679)
  call void @llvm.stackrestore.p0(ptr %1673)
  %1680 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 2819, i32 125)
  %1681 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %1682 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %1683 = insertvalue { ptr, ptr, i64 } poison, ptr %1681, 0
  %1684 = insertvalue { ptr, ptr, i64 } %1683, ptr %1682, 1
  %1685 = insertvalue { ptr, ptr, i64 } %1684, i64 0, 2
  %1686 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %1687 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %1688 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %1689 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %1690 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %1691 = extractvalue { ptr, ptr, i64 } %1685, 0
  %1692 = extractvalue { ptr, ptr, i64 } %1685, 1
  %1693 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1691, 0
  %1694 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1693, ptr %1692, 1
  %1695 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1694, i64 1072, 2
  %1696 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1695, i64 8, 3, 0
  %1697 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1696, i64 65, 4, 0
  %1698 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1697, i64 16, 3, 1
  %1699 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1698, i64 1, 4, 1
  %1700 = call ptr @llvm.stacksave.p0()
  %1701 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1699, ptr %1701, align 8
  %1702 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1701, 1
  %1703 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %1703, align 8
  %1704 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1703, 1
  %1705 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1702, ptr %1705, align 8
  %1706 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1704, ptr %1706, align 8
  call void @memrefCopy(i64 4, ptr %1705, ptr %1706)
  call void @llvm.stackrestore.p0(ptr %1700)
  %1707 = call ptr @llvm.stacksave.p0()
  %1708 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %273, ptr %1708, align 8
  %1709 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1708, 1
  %1710 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %1710, align 8
  %1711 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1710, 1
  %1712 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1709, ptr %1712, align 8
  %1713 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1711, ptr %1713, align 8
  call void @memrefCopy(i64 4, ptr %1712, ptr %1713)
  call void @llvm.stackrestore.p0(ptr %1707)
  %1714 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 2888, i32 125)
  %1715 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %1716 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %1717 = insertvalue { ptr, ptr, i64 } poison, ptr %1715, 0
  %1718 = insertvalue { ptr, ptr, i64 } %1717, ptr %1716, 1
  %1719 = insertvalue { ptr, ptr, i64 } %1718, i64 0, 2
  %1720 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %1721 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %1722 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %1723 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %1724 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %1725 = extractvalue { ptr, ptr, i64 } %1719, 0
  %1726 = extractvalue { ptr, ptr, i64 } %1719, 1
  %1727 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1725, 0
  %1728 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1727, ptr %1726, 1
  %1729 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1728, i64 1088, 2
  %1730 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1729, i64 8, 3, 0
  %1731 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1730, i64 65, 4, 0
  %1732 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1731, i64 16, 3, 1
  %1733 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1732, i64 1, 4, 1
  %1734 = call ptr @llvm.stacksave.p0()
  %1735 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1733, ptr %1735, align 8
  %1736 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1735, 1
  %1737 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %1737, align 8
  %1738 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1737, 1
  %1739 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1736, ptr %1739, align 8
  %1740 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1738, ptr %1740, align 8
  call void @memrefCopy(i64 4, ptr %1739, ptr %1740)
  call void @llvm.stackrestore.p0(ptr %1734)
  %1741 = call ptr @llvm.stacksave.p0()
  %1742 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %326, ptr %1742, align 8
  %1743 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1742, 1
  %1744 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %1744, align 8
  %1745 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1744, 1
  %1746 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1743, ptr %1746, align 8
  %1747 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1745, ptr %1747, align 8
  call void @memrefCopy(i64 4, ptr %1746, ptr %1747)
  call void @llvm.stackrestore.p0(ptr %1741)
  %1748 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 2957, i32 125)
  %1749 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %1750 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %1751 = insertvalue { ptr, ptr, i64 } poison, ptr %1749, 0
  %1752 = insertvalue { ptr, ptr, i64 } %1751, ptr %1750, 1
  %1753 = insertvalue { ptr, ptr, i64 } %1752, i64 0, 2
  %1754 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %1755 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %1756 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %1757 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %1758 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %1759 = extractvalue { ptr, ptr, i64 } %1753, 0
  %1760 = extractvalue { ptr, ptr, i64 } %1753, 1
  %1761 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1759, 0
  %1762 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1761, ptr %1760, 1
  %1763 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1762, i64 1104, 2
  %1764 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1763, i64 8, 3, 0
  %1765 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1764, i64 65, 4, 0
  %1766 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1765, i64 1, 3, 1
  %1767 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1766, i64 1, 4, 1
  %1768 = call ptr @llvm.stacksave.p0()
  %1769 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1767, ptr %1769, align 8
  %1770 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1769, 1
  %1771 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %362, ptr %1771, align 8
  %1772 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1771, 1
  %1773 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1770, ptr %1773, align 8
  %1774 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1772, ptr %1774, align 8
  call void @memrefCopy(i64 4, ptr %1773, ptr %1774)
  call void @llvm.stackrestore.p0(ptr %1768)
  %1775 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %391, 3, 0
  %1776 = mul i64 1, %1775
  %1777 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %391, 3, 1
  %1778 = mul i64 %1776, %1777
  %1779 = mul i64 %1778, 4
  %1780 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %391, 1
  %1781 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %391, 2
  %1782 = getelementptr float, ptr %1780, i64 %1781
  %1783 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %400, 1
  %1784 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %400, 2
  %1785 = getelementptr float, ptr %1783, i64 %1784
  call void @llvm.memcpy.p0.p0.i64(ptr %1785, ptr %1782, i64 %1779, i1 false)
  %1786 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 1, i32 8, ptr %372, ptr %414, ptr %98, i32 0, i32 3026, i32 110)
  %1787 = call ptr @llvm.stacksave.p0()
  %1788 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %1788, align 8
  %1789 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1788, 1
  %1790 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1605, ptr %1790, align 8
  %1791 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1790, 1
  %1792 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1789, ptr %1792, align 8
  %1793 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1791, ptr %1793, align 8
  call void @memrefCopy(i64 4, ptr %1792, ptr %1793)
  call void @llvm.stackrestore.p0(ptr %1787)
  %1794 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %1795 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %1796 = insertvalue { ptr, ptr, i64 } poison, ptr %1794, 0
  %1797 = insertvalue { ptr, ptr, i64 } %1796, ptr %1795, 1
  %1798 = insertvalue { ptr, ptr, i64 } %1797, i64 0, 2
  %1799 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %1800 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %1801 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %1802 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %1803 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %1804 = extractvalue { ptr, ptr, i64 } %1798, 0
  %1805 = extractvalue { ptr, ptr, i64 } %1798, 1
  %1806 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1804, 0
  %1807 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1806, ptr %1805, 1
  %1808 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1807, i64 520, 2
  %1809 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1808, i64 8, 3, 0
  %1810 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1809, i64 32, 4, 0
  %1811 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1810, i64 8, 3, 1
  %1812 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1811, i64 1, 4, 1
  %1813 = call ptr @llvm.stacksave.p0()
  %1814 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1812, ptr %1814, align 8
  %1815 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1814, 1
  %1816 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %1816, align 8
  %1817 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1816, 1
  %1818 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1815, ptr %1818, align 8
  %1819 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1817, ptr %1819, align 8
  call void @memrefCopy(i64 4, ptr %1818, ptr %1819)
  call void @llvm.stackrestore.p0(ptr %1813)
  %1820 = call ptr @llvm.stacksave.p0()
  %1821 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1631, ptr %1821, align 8
  %1822 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1821, 1
  %1823 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %1823, align 8
  %1824 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1823, 1
  %1825 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1822, ptr %1825, align 8
  %1826 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1824, ptr %1826, align 8
  call void @memrefCopy(i64 4, ptr %1825, ptr %1826)
  call void @llvm.stackrestore.p0(ptr %1820)
  %1827 = call ptr @llvm.stacksave.p0()
  %1828 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %474, ptr %1828, align 8
  %1829 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1828, 1
  %1830 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %1830, align 8
  %1831 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1830, 1
  %1832 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1829, ptr %1832, align 8
  %1833 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1831, ptr %1833, align 8
  call void @memrefCopy(i64 4, ptr %1832, ptr %1833)
  call void @llvm.stackrestore.p0(ptr %1827)
  %1834 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 3095, i32 125)
  %1835 = call ptr @llvm.stacksave.p0()
  %1836 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1665, ptr %1836, align 8
  %1837 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1836, 1
  %1838 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %1838, align 8
  %1839 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1838, 1
  %1840 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1837, ptr %1840, align 8
  %1841 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1839, ptr %1841, align 8
  call void @memrefCopy(i64 4, ptr %1840, ptr %1841)
  call void @llvm.stackrestore.p0(ptr %1835)
  %1842 = call ptr @llvm.stacksave.p0()
  %1843 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %508, ptr %1843, align 8
  %1844 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1843, 1
  %1845 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %1845, align 8
  %1846 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1845, 1
  %1847 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1844, ptr %1847, align 8
  %1848 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1846, ptr %1848, align 8
  call void @memrefCopy(i64 4, ptr %1847, ptr %1848)
  call void @llvm.stackrestore.p0(ptr %1842)
  %1849 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 3164, i32 125)
  %1850 = call ptr @llvm.stacksave.p0()
  %1851 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1699, ptr %1851, align 8
  %1852 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1851, 1
  %1853 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %1853, align 8
  %1854 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1853, 1
  %1855 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1852, ptr %1855, align 8
  %1856 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1854, ptr %1856, align 8
  call void @memrefCopy(i64 4, ptr %1855, ptr %1856)
  call void @llvm.stackrestore.p0(ptr %1850)
  %1857 = call ptr @llvm.stacksave.p0()
  %1858 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %542, ptr %1858, align 8
  %1859 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1858, 1
  %1860 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %1860, align 8
  %1861 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1860, 1
  %1862 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1859, ptr %1862, align 8
  %1863 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1861, ptr %1863, align 8
  call void @memrefCopy(i64 4, ptr %1862, ptr %1863)
  call void @llvm.stackrestore.p0(ptr %1857)
  %1864 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 3233, i32 125)
  %1865 = call ptr @llvm.stacksave.p0()
  %1866 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1733, ptr %1866, align 8
  %1867 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1866, 1
  %1868 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %1868, align 8
  %1869 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1868, 1
  %1870 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1867, ptr %1870, align 8
  %1871 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1869, ptr %1871, align 8
  call void @memrefCopy(i64 4, ptr %1870, ptr %1871)
  call void @llvm.stackrestore.p0(ptr %1865)
  %1872 = call ptr @llvm.stacksave.p0()
  %1873 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %576, ptr %1873, align 8
  %1874 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1873, 1
  %1875 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %1875, align 8
  %1876 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1875, 1
  %1877 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1874, ptr %1877, align 8
  %1878 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1876, ptr %1878, align 8
  call void @memrefCopy(i64 4, ptr %1877, ptr %1878)
  call void @llvm.stackrestore.p0(ptr %1872)
  %1879 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 3302, i32 125)
  %1880 = call ptr @llvm.stacksave.p0()
  %1881 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1767, ptr %1881, align 8
  %1882 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1881, 1
  %1883 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %362, ptr %1883, align 8
  %1884 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1883, 1
  %1885 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1882, ptr %1885, align 8
  %1886 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1884, ptr %1886, align 8
  call void @memrefCopy(i64 4, ptr %1885, ptr %1886)
  call void @llvm.stackrestore.p0(ptr %1880)
  %1887 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %610, 3, 0
  %1888 = mul i64 1, %1887
  %1889 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %610, 3, 1
  %1890 = mul i64 %1888, %1889
  %1891 = mul i64 %1890, 4
  %1892 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %610, 1
  %1893 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %610, 2
  %1894 = getelementptr float, ptr %1892, i64 %1893
  %1895 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %400, 1
  %1896 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %400, 2
  %1897 = getelementptr float, ptr %1895, i64 %1896
  call void @llvm.memcpy.p0.p0.i64(ptr %1897, ptr %1894, i64 %1891, i1 false)
  %1898 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 1, i32 8, ptr %372, ptr %414, ptr %98, i32 0, i32 3371, i32 110)
  %1899 = call ptr @llvm.stacksave.p0()
  %1900 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %1900, align 8
  %1901 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1900, 1
  %1902 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1812, ptr %1902, align 8
  %1903 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1902, 1
  %1904 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1901, ptr %1904, align 8
  %1905 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1903, ptr %1905, align 8
  call void @memrefCopy(i64 4, ptr %1904, ptr %1905)
  call void @llvm.stackrestore.p0(ptr %1899)
  %1906 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %1907 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %1908 = insertvalue { ptr, ptr, i64 } poison, ptr %1906, 0
  %1909 = insertvalue { ptr, ptr, i64 } %1908, ptr %1907, 1
  %1910 = insertvalue { ptr, ptr, i64 } %1909, i64 0, 2
  %1911 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %1912 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %1913 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %1914 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %1915 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %1916 = extractvalue { ptr, ptr, i64 } %1910, 0
  %1917 = extractvalue { ptr, ptr, i64 } %1910, 1
  %1918 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1916, 0
  %1919 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1918, ptr %1917, 1
  %1920 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1919, i64 528, 2
  %1921 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1920, i64 4, 3, 0
  %1922 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1921, i64 32, 4, 0
  %1923 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1922, i64 4, 3, 1
  %1924 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1923, i64 1, 4, 1
  %1925 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 0
  %1926 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 1
  %1927 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1925, 0
  %1928 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1927, ptr %1926, 1
  %1929 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1928, i64 0, 2
  %1930 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1929, i64 4, 3, 0
  %1931 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1930, i64 4, 4, 0
  %1932 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1931, i64 4, 3, 1
  %1933 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1932, i64 1, 4, 1
  %1934 = call ptr @llvm.stacksave.p0()
  %1935 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1924, ptr %1935, align 8
  %1936 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1935, 1
  %1937 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1933, ptr %1937, align 8
  %1938 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1937, 1
  %1939 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1936, ptr %1939, align 8
  %1940 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1938, ptr %1940, align 8
  call void @memrefCopy(i64 4, ptr %1939, ptr %1940)
  call void @llvm.stackrestore.p0(ptr %1934)
  %1941 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 1
  %1942 = ptrtoint ptr %1941 to i64
  %1943 = inttoptr i64 %1942 to ptr
  %1944 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %1945 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %1946 = insertvalue { ptr, ptr, i64 } poison, ptr %1944, 0
  %1947 = insertvalue { ptr, ptr, i64 } %1946, ptr %1945, 1
  %1948 = insertvalue { ptr, ptr, i64 } %1947, i64 0, 2
  %1949 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %1950 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %1951 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %1952 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %1953 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %1954 = extractvalue { ptr, ptr, i64 } %1948, 0
  %1955 = extractvalue { ptr, ptr, i64 } %1948, 1
  %1956 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1954, 0
  %1957 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1956, ptr %1955, 1
  %1958 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1957, i64 1040, 2
  %1959 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1958, i64 4, 3, 0
  %1960 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1959, i64 65, 4, 0
  %1961 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1960, i64 16, 3, 1
  %1962 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1961, i64 1, 4, 1
  %1963 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 0
  %1964 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %1965 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1963, 0
  %1966 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1965, ptr %1964, 1
  %1967 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1966, i64 0, 2
  %1968 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1967, i64 4, 3, 0
  %1969 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1968, i64 16, 4, 0
  %1970 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1969, i64 16, 3, 1
  %1971 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1970, i64 1, 4, 1
  %1972 = call ptr @llvm.stacksave.p0()
  %1973 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1962, ptr %1973, align 8
  %1974 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1973, 1
  %1975 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %1975, align 8
  %1976 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1975, 1
  %1977 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1974, ptr %1977, align 8
  %1978 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1976, ptr %1978, align 8
  call void @memrefCopy(i64 4, ptr %1977, ptr %1978)
  call void @llvm.stackrestore.p0(ptr %1972)
  %1979 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %1980 = ptrtoint ptr %1979 to i64
  %1981 = inttoptr i64 %1980 to ptr
  %1982 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %1983 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %1984 = insertvalue { ptr, ptr, i64 } poison, ptr %1982, 0
  %1985 = insertvalue { ptr, ptr, i64 } %1984, ptr %1983, 1
  %1986 = insertvalue { ptr, ptr, i64 } %1985, i64 0, 2
  %1987 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %1988 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %1989 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %1990 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %1991 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %1992 = extractvalue { ptr, ptr, i64 } %1986, 0
  %1993 = extractvalue { ptr, ptr, i64 } %1986, 1
  %1994 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1992, 0
  %1995 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1994, ptr %1993, 1
  %1996 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1995, i64 16, 2
  %1997 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1996, i64 16, 3, 0
  %1998 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1997, i64 32, 4, 0
  %1999 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1998, i64 4, 3, 1
  %2000 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1999, i64 1, 4, 1
  %2001 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 0
  %2002 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %2003 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2001, 0
  %2004 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2003, ptr %2002, 1
  %2005 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2004, i64 0, 2
  %2006 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2005, i64 16, 3, 0
  %2007 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2006, i64 4, 4, 0
  %2008 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2007, i64 4, 3, 1
  %2009 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2008, i64 1, 4, 1
  %2010 = call ptr @llvm.stacksave.p0()
  %2011 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2000, ptr %2011, align 8
  %2012 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2011, 1
  %2013 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %2013, align 8
  %2014 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2013, 1
  %2015 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2012, ptr %2015, align 8
  %2016 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2014, ptr %2016, align 8
  call void @memrefCopy(i64 4, ptr %2015, ptr %2016)
  call void @llvm.stackrestore.p0(ptr %2010)
  %2017 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %2018 = ptrtoint ptr %2017 to i64
  %2019 = inttoptr i64 %2018 to ptr
  %2020 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 1, i32 0, i32 117)
  %2021 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %2022 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %2023 = insertvalue { ptr, ptr, i64 } poison, ptr %2021, 0
  %2024 = insertvalue { ptr, ptr, i64 } %2023, ptr %2022, 1
  %2025 = insertvalue { ptr, ptr, i64 } %2024, i64 0, 2
  %2026 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %2027 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %2028 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %2029 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %2030 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %2031 = extractvalue { ptr, ptr, i64 } %2025, 0
  %2032 = extractvalue { ptr, ptr, i64 } %2025, 1
  %2033 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2031, 0
  %2034 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2033, ptr %2032, 1
  %2035 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2034, i64 1056, 2
  %2036 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2035, i64 4, 3, 0
  %2037 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2036, i64 65, 4, 0
  %2038 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2037, i64 16, 3, 1
  %2039 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2038, i64 1, 4, 1
  %2040 = call ptr @llvm.stacksave.p0()
  %2041 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2039, ptr %2041, align 8
  %2042 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2041, 1
  %2043 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %2043, align 8
  %2044 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2043, 1
  %2045 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2042, ptr %2045, align 8
  %2046 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2044, ptr %2046, align 8
  call void @memrefCopy(i64 4, ptr %2045, ptr %2046)
  call void @llvm.stackrestore.p0(ptr %2040)
  %2047 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %2048 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %2049 = insertvalue { ptr, ptr, i64 } poison, ptr %2047, 0
  %2050 = insertvalue { ptr, ptr, i64 } %2049, ptr %2048, 1
  %2051 = insertvalue { ptr, ptr, i64 } %2050, i64 0, 2
  %2052 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %2053 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %2054 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %2055 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %2056 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %2057 = extractvalue { ptr, ptr, i64 } %2051, 0
  %2058 = extractvalue { ptr, ptr, i64 } %2051, 1
  %2059 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2057, 0
  %2060 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2059, ptr %2058, 1
  %2061 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2060, i64 528, 2
  %2062 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2061, i64 16, 3, 0
  %2063 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2062, i64 32, 4, 0
  %2064 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2063, i64 4, 3, 1
  %2065 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2064, i64 1, 4, 1
  %2066 = call ptr @llvm.stacksave.p0()
  %2067 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2065, ptr %2067, align 8
  %2068 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2067, 1
  %2069 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %2069, align 8
  %2070 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2069, 1
  %2071 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2068, ptr %2071, align 8
  %2072 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2070, ptr %2072, align 8
  call void @memrefCopy(i64 4, ptr %2071, ptr %2072)
  call void @llvm.stackrestore.p0(ptr %2066)
  %2073 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 1, i32 42, i32 117)
  %2074 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %2075 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %2076 = insertvalue { ptr, ptr, i64 } poison, ptr %2074, 0
  %2077 = insertvalue { ptr, ptr, i64 } %2076, ptr %2075, 1
  %2078 = insertvalue { ptr, ptr, i64 } %2077, i64 0, 2
  %2079 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %2080 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %2081 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %2082 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %2083 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %2084 = extractvalue { ptr, ptr, i64 } %2078, 0
  %2085 = extractvalue { ptr, ptr, i64 } %2078, 1
  %2086 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2084, 0
  %2087 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2086, ptr %2085, 1
  %2088 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2087, i64 1072, 2
  %2089 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2088, i64 4, 3, 0
  %2090 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2089, i64 65, 4, 0
  %2091 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2090, i64 16, 3, 1
  %2092 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2091, i64 1, 4, 1
  %2093 = call ptr @llvm.stacksave.p0()
  %2094 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2092, ptr %2094, align 8
  %2095 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2094, 1
  %2096 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %2096, align 8
  %2097 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2096, 1
  %2098 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2095, ptr %2098, align 8
  %2099 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2097, ptr %2099, align 8
  call void @memrefCopy(i64 4, ptr %2098, ptr %2099)
  call void @llvm.stackrestore.p0(ptr %2093)
  %2100 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %2101 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %2102 = insertvalue { ptr, ptr, i64 } poison, ptr %2100, 0
  %2103 = insertvalue { ptr, ptr, i64 } %2102, ptr %2101, 1
  %2104 = insertvalue { ptr, ptr, i64 } %2103, i64 0, 2
  %2105 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %2106 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %2107 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %2108 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %2109 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %2110 = extractvalue { ptr, ptr, i64 } %2104, 0
  %2111 = extractvalue { ptr, ptr, i64 } %2104, 1
  %2112 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2110, 0
  %2113 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2112, ptr %2111, 1
  %2114 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2113, i64 1040, 2
  %2115 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2114, i64 16, 3, 0
  %2116 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2115, i64 32, 4, 0
  %2117 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2116, i64 4, 3, 1
  %2118 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2117, i64 1, 4, 1
  %2119 = call ptr @llvm.stacksave.p0()
  %2120 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2118, ptr %2120, align 8
  %2121 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2120, 1
  %2122 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %2122, align 8
  %2123 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2122, 1
  %2124 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2121, ptr %2124, align 8
  %2125 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2123, ptr %2125, align 8
  call void @memrefCopy(i64 4, ptr %2124, ptr %2125)
  call void @llvm.stackrestore.p0(ptr %2119)
  %2126 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 1, i32 120, i32 117)
  %2127 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %2128 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %2129 = insertvalue { ptr, ptr, i64 } poison, ptr %2127, 0
  %2130 = insertvalue { ptr, ptr, i64 } %2129, ptr %2128, 1
  %2131 = insertvalue { ptr, ptr, i64 } %2130, i64 0, 2
  %2132 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %2133 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %2134 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %2135 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %2136 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %2137 = extractvalue { ptr, ptr, i64 } %2131, 0
  %2138 = extractvalue { ptr, ptr, i64 } %2131, 1
  %2139 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2137, 0
  %2140 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2139, ptr %2138, 1
  %2141 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2140, i64 1088, 2
  %2142 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2141, i64 4, 3, 0
  %2143 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2142, i64 65, 4, 0
  %2144 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2143, i64 16, 3, 1
  %2145 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2144, i64 1, 4, 1
  %2146 = call ptr @llvm.stacksave.p0()
  %2147 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2145, ptr %2147, align 8
  %2148 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2147, 1
  %2149 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %2149, align 8
  %2150 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2149, 1
  %2151 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2148, ptr %2151, align 8
  %2152 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2150, ptr %2152, align 8
  call void @memrefCopy(i64 4, ptr %2151, ptr %2152)
  call void @llvm.stackrestore.p0(ptr %2146)
  %2153 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %2154 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %2155 = insertvalue { ptr, ptr, i64 } poison, ptr %2153, 0
  %2156 = insertvalue { ptr, ptr, i64 } %2155, ptr %2154, 1
  %2157 = insertvalue { ptr, ptr, i64 } %2156, i64 0, 2
  %2158 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %2159 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %2160 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %2161 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %2162 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %2163 = extractvalue { ptr, ptr, i64 } %2157, 0
  %2164 = extractvalue { ptr, ptr, i64 } %2157, 1
  %2165 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2163, 0
  %2166 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2165, ptr %2164, 1
  %2167 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2166, i64 1552, 2
  %2168 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2167, i64 16, 3, 0
  %2169 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2168, i64 32, 4, 0
  %2170 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2169, i64 4, 3, 1
  %2171 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2170, i64 1, 4, 1
  %2172 = call ptr @llvm.stacksave.p0()
  %2173 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2171, ptr %2173, align 8
  %2174 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2173, 1
  %2175 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %2175, align 8
  %2176 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2175, 1
  %2177 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2174, ptr %2177, align 8
  %2178 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2176, ptr %2178, align 8
  call void @memrefCopy(i64 4, ptr %2177, ptr %2178)
  call void @llvm.stackrestore.p0(ptr %2172)
  %2179 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 1, i32 189, i32 117)
  %2180 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %2181 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %2182 = insertvalue { ptr, ptr, i64 } poison, ptr %2180, 0
  %2183 = insertvalue { ptr, ptr, i64 } %2182, ptr %2181, 1
  %2184 = insertvalue { ptr, ptr, i64 } %2183, i64 0, 2
  %2185 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %2186 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %2187 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %2188 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %2189 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %2190 = extractvalue { ptr, ptr, i64 } %2184, 0
  %2191 = extractvalue { ptr, ptr, i64 } %2184, 1
  %2192 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2190, 0
  %2193 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2192, ptr %2191, 1
  %2194 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2193, i64 1104, 2
  %2195 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2194, i64 4, 3, 0
  %2196 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2195, i64 65, 4, 0
  %2197 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2196, i64 1, 3, 1
  %2198 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2197, i64 1, 4, 1
  %2199 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 0
  %2200 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %2201 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2199, 0
  %2202 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2201, ptr %2200, 1
  %2203 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2202, i64 0, 2
  %2204 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2203, i64 4, 3, 0
  %2205 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2204, i64 1, 4, 0
  %2206 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2205, i64 1, 3, 1
  %2207 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2206, i64 1, 4, 1
  %2208 = call ptr @llvm.stacksave.p0()
  %2209 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2198, ptr %2209, align 8
  %2210 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2209, 1
  %2211 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2207, ptr %2211, align 8
  %2212 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2211, 1
  %2213 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2210, ptr %2213, align 8
  %2214 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2212, ptr %2214, align 8
  call void @memrefCopy(i64 4, ptr %2213, ptr %2214)
  call void @llvm.stackrestore.p0(ptr %2208)
  %2215 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %2216 = ptrtoint ptr %2215 to i64
  %2217 = inttoptr i64 %2216 to ptr
  %2218 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %2219 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %2220 = insertvalue { ptr, ptr, i64 } poison, ptr %2218, 0
  %2221 = insertvalue { ptr, ptr, i64 } %2220, ptr %2219, 1
  %2222 = insertvalue { ptr, ptr, i64 } %2221, i64 0, 2
  %2223 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %2224 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %2225 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %2226 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %2227 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %2228 = extractvalue { ptr, ptr, i64 } %2222, 0
  %2229 = extractvalue { ptr, ptr, i64 } %2222, 1
  %2230 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2228, 0
  %2231 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2230, ptr %2229, 1
  %2232 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2231, i64 2064, 2
  %2233 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2232, i64 1, 3, 0
  %2234 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2233, i64 32, 4, 0
  %2235 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2234, i64 4, 3, 1
  %2236 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2235, i64 1, 4, 1
  %2237 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 0
  %2238 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %2239 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2237, 0
  %2240 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2239, ptr %2238, 1
  %2241 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2240, i64 0, 2
  %2242 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2241, i64 1, 3, 0
  %2243 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2242, i64 4, 4, 0
  %2244 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2243, i64 4, 3, 1
  %2245 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2244, i64 1, 4, 1
  %2246 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2236, 3, 0
  %2247 = mul i64 1, %2246
  %2248 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2236, 3, 1
  %2249 = mul i64 %2247, %2248
  %2250 = mul i64 %2249, 4
  %2251 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2236, 1
  %2252 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2236, 2
  %2253 = getelementptr float, ptr %2251, i64 %2252
  %2254 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2245, 1
  %2255 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2245, 2
  %2256 = getelementptr float, ptr %2254, i64 %2255
  call void @llvm.memcpy.p0.p0.i64(ptr %2256, ptr %2253, i64 %2250, i1 false)
  %2257 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %2258 = ptrtoint ptr %2257 to i64
  %2259 = inttoptr i64 %2258 to ptr
  %2260 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 1, i32 4, ptr %2217, ptr %2259, ptr %1943, i32 1, i32 258, i32 102)
  %2261 = call ptr @llvm.stacksave.p0()
  %2262 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1933, ptr %2262, align 8
  %2263 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2262, 1
  %2264 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1924, ptr %2264, align 8
  %2265 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2264, 1
  %2266 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2263, ptr %2266, align 8
  %2267 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2265, ptr %2267, align 8
  call void @memrefCopy(i64 4, ptr %2266, ptr %2267)
  call void @llvm.stackrestore.p0(ptr %2261)
  %2268 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %2269 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %2270 = insertvalue { ptr, ptr, i64 } poison, ptr %2268, 0
  %2271 = insertvalue { ptr, ptr, i64 } %2270, ptr %2269, 1
  %2272 = insertvalue { ptr, ptr, i64 } %2271, i64 0, 2
  %2273 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %2274 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %2275 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %2276 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %2277 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %2278 = extractvalue { ptr, ptr, i64 } %2272, 0
  %2279 = extractvalue { ptr, ptr, i64 } %2272, 1
  %2280 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2278, 0
  %2281 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2280, ptr %2279, 1
  %2282 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2281, i64 532, 2
  %2283 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2282, i64 4, 3, 0
  %2284 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2283, i64 32, 4, 0
  %2285 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2284, i64 4, 3, 1
  %2286 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2285, i64 1, 4, 1
  %2287 = call ptr @llvm.stacksave.p0()
  %2288 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2286, ptr %2288, align 8
  %2289 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2288, 1
  %2290 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1933, ptr %2290, align 8
  %2291 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2290, 1
  %2292 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2289, ptr %2292, align 8
  %2293 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2291, ptr %2293, align 8
  call void @memrefCopy(i64 4, ptr %2292, ptr %2293)
  call void @llvm.stackrestore.p0(ptr %2287)
  %2294 = call ptr @llvm.stacksave.p0()
  %2295 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1962, ptr %2295, align 8
  %2296 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2295, 1
  %2297 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %2297, align 8
  %2298 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2297, 1
  %2299 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2296, ptr %2299, align 8
  %2300 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2298, ptr %2300, align 8
  call void @memrefCopy(i64 4, ptr %2299, ptr %2300)
  call void @llvm.stackrestore.p0(ptr %2294)
  %2301 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %2302 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %2303 = insertvalue { ptr, ptr, i64 } poison, ptr %2301, 0
  %2304 = insertvalue { ptr, ptr, i64 } %2303, ptr %2302, 1
  %2305 = insertvalue { ptr, ptr, i64 } %2304, i64 0, 2
  %2306 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %2307 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %2308 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %2309 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %2310 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %2311 = extractvalue { ptr, ptr, i64 } %2305, 0
  %2312 = extractvalue { ptr, ptr, i64 } %2305, 1
  %2313 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2311, 0
  %2314 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2313, ptr %2312, 1
  %2315 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2314, i64 20, 2
  %2316 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2315, i64 16, 3, 0
  %2317 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2316, i64 32, 4, 0
  %2318 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2317, i64 4, 3, 1
  %2319 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2318, i64 1, 4, 1
  %2320 = call ptr @llvm.stacksave.p0()
  %2321 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2319, ptr %2321, align 8
  %2322 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2321, 1
  %2323 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %2323, align 8
  %2324 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2323, 1
  %2325 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2322, ptr %2325, align 8
  %2326 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2324, ptr %2326, align 8
  call void @memrefCopy(i64 4, ptr %2325, ptr %2326)
  call void @llvm.stackrestore.p0(ptr %2320)
  %2327 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 2, i32 0, i32 117)
  %2328 = call ptr @llvm.stacksave.p0()
  %2329 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2039, ptr %2329, align 8
  %2330 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2329, 1
  %2331 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %2331, align 8
  %2332 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2331, 1
  %2333 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2330, ptr %2333, align 8
  %2334 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2332, ptr %2334, align 8
  call void @memrefCopy(i64 4, ptr %2333, ptr %2334)
  call void @llvm.stackrestore.p0(ptr %2328)
  %2335 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %2336 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %2337 = insertvalue { ptr, ptr, i64 } poison, ptr %2335, 0
  %2338 = insertvalue { ptr, ptr, i64 } %2337, ptr %2336, 1
  %2339 = insertvalue { ptr, ptr, i64 } %2338, i64 0, 2
  %2340 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %2341 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %2342 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %2343 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %2344 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %2345 = extractvalue { ptr, ptr, i64 } %2339, 0
  %2346 = extractvalue { ptr, ptr, i64 } %2339, 1
  %2347 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2345, 0
  %2348 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2347, ptr %2346, 1
  %2349 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2348, i64 532, 2
  %2350 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2349, i64 16, 3, 0
  %2351 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2350, i64 32, 4, 0
  %2352 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2351, i64 4, 3, 1
  %2353 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2352, i64 1, 4, 1
  %2354 = call ptr @llvm.stacksave.p0()
  %2355 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2353, ptr %2355, align 8
  %2356 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2355, 1
  %2357 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %2357, align 8
  %2358 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2357, 1
  %2359 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2356, ptr %2359, align 8
  %2360 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2358, ptr %2360, align 8
  call void @memrefCopy(i64 4, ptr %2359, ptr %2360)
  call void @llvm.stackrestore.p0(ptr %2354)
  %2361 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 2, i32 42, i32 117)
  %2362 = call ptr @llvm.stacksave.p0()
  %2363 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2092, ptr %2363, align 8
  %2364 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2363, 1
  %2365 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %2365, align 8
  %2366 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2365, 1
  %2367 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2364, ptr %2367, align 8
  %2368 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2366, ptr %2368, align 8
  call void @memrefCopy(i64 4, ptr %2367, ptr %2368)
  call void @llvm.stackrestore.p0(ptr %2362)
  %2369 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %2370 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %2371 = insertvalue { ptr, ptr, i64 } poison, ptr %2369, 0
  %2372 = insertvalue { ptr, ptr, i64 } %2371, ptr %2370, 1
  %2373 = insertvalue { ptr, ptr, i64 } %2372, i64 0, 2
  %2374 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %2375 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %2376 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %2377 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %2378 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %2379 = extractvalue { ptr, ptr, i64 } %2373, 0
  %2380 = extractvalue { ptr, ptr, i64 } %2373, 1
  %2381 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2379, 0
  %2382 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2381, ptr %2380, 1
  %2383 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2382, i64 1044, 2
  %2384 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2383, i64 16, 3, 0
  %2385 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2384, i64 32, 4, 0
  %2386 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2385, i64 4, 3, 1
  %2387 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2386, i64 1, 4, 1
  %2388 = call ptr @llvm.stacksave.p0()
  %2389 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2387, ptr %2389, align 8
  %2390 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2389, 1
  %2391 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %2391, align 8
  %2392 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2391, 1
  %2393 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2390, ptr %2393, align 8
  %2394 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2392, ptr %2394, align 8
  call void @memrefCopy(i64 4, ptr %2393, ptr %2394)
  call void @llvm.stackrestore.p0(ptr %2388)
  %2395 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 2, i32 120, i32 117)
  %2396 = call ptr @llvm.stacksave.p0()
  %2397 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2145, ptr %2397, align 8
  %2398 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2397, 1
  %2399 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %2399, align 8
  %2400 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2399, 1
  %2401 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2398, ptr %2401, align 8
  %2402 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2400, ptr %2402, align 8
  call void @memrefCopy(i64 4, ptr %2401, ptr %2402)
  call void @llvm.stackrestore.p0(ptr %2396)
  %2403 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %2404 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %2405 = insertvalue { ptr, ptr, i64 } poison, ptr %2403, 0
  %2406 = insertvalue { ptr, ptr, i64 } %2405, ptr %2404, 1
  %2407 = insertvalue { ptr, ptr, i64 } %2406, i64 0, 2
  %2408 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %2409 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %2410 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %2411 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %2412 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %2413 = extractvalue { ptr, ptr, i64 } %2407, 0
  %2414 = extractvalue { ptr, ptr, i64 } %2407, 1
  %2415 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2413, 0
  %2416 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2415, ptr %2414, 1
  %2417 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2416, i64 1556, 2
  %2418 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2417, i64 16, 3, 0
  %2419 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2418, i64 32, 4, 0
  %2420 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2419, i64 4, 3, 1
  %2421 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2420, i64 1, 4, 1
  %2422 = call ptr @llvm.stacksave.p0()
  %2423 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2421, ptr %2423, align 8
  %2424 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2423, 1
  %2425 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %2425, align 8
  %2426 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2425, 1
  %2427 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2424, ptr %2427, align 8
  %2428 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2426, ptr %2428, align 8
  call void @memrefCopy(i64 4, ptr %2427, ptr %2428)
  call void @llvm.stackrestore.p0(ptr %2422)
  %2429 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 2, i32 189, i32 117)
  %2430 = call ptr @llvm.stacksave.p0()
  %2431 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2198, ptr %2431, align 8
  %2432 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2431, 1
  %2433 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2207, ptr %2433, align 8
  %2434 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2433, 1
  %2435 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2432, ptr %2435, align 8
  %2436 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2434, ptr %2436, align 8
  call void @memrefCopy(i64 4, ptr %2435, ptr %2436)
  call void @llvm.stackrestore.p0(ptr %2430)
  %2437 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %2438 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %2439 = insertvalue { ptr, ptr, i64 } poison, ptr %2437, 0
  %2440 = insertvalue { ptr, ptr, i64 } %2439, ptr %2438, 1
  %2441 = insertvalue { ptr, ptr, i64 } %2440, i64 0, 2
  %2442 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %2443 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %2444 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %2445 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %2446 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %2447 = extractvalue { ptr, ptr, i64 } %2441, 0
  %2448 = extractvalue { ptr, ptr, i64 } %2441, 1
  %2449 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2447, 0
  %2450 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2449, ptr %2448, 1
  %2451 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2450, i64 2068, 2
  %2452 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2451, i64 1, 3, 0
  %2453 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2452, i64 32, 4, 0
  %2454 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2453, i64 4, 3, 1
  %2455 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2454, i64 1, 4, 1
  %2456 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2455, 3, 0
  %2457 = mul i64 1, %2456
  %2458 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2455, 3, 1
  %2459 = mul i64 %2457, %2458
  %2460 = mul i64 %2459, 4
  %2461 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2455, 1
  %2462 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2455, 2
  %2463 = getelementptr float, ptr %2461, i64 %2462
  %2464 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2245, 1
  %2465 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2245, 2
  %2466 = getelementptr float, ptr %2464, i64 %2465
  call void @llvm.memcpy.p0.p0.i64(ptr %2466, ptr %2463, i64 %2460, i1 false)
  %2467 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 1, i32 4, ptr %2217, ptr %2259, ptr %1943, i32 2, i32 258, i32 102)
  %2468 = call ptr @llvm.stacksave.p0()
  %2469 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1933, ptr %2469, align 8
  %2470 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2469, 1
  %2471 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2286, ptr %2471, align 8
  %2472 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2471, 1
  %2473 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2470, ptr %2473, align 8
  %2474 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2472, ptr %2474, align 8
  call void @memrefCopy(i64 4, ptr %2473, ptr %2474)
  call void @llvm.stackrestore.p0(ptr %2468)
  %2475 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %2476 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %2477 = insertvalue { ptr, ptr, i64 } poison, ptr %2475, 0
  %2478 = insertvalue { ptr, ptr, i64 } %2477, ptr %2476, 1
  %2479 = insertvalue { ptr, ptr, i64 } %2478, i64 0, 2
  %2480 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %2481 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %2482 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %2483 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %2484 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %2485 = extractvalue { ptr, ptr, i64 } %2479, 0
  %2486 = extractvalue { ptr, ptr, i64 } %2479, 1
  %2487 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2485, 0
  %2488 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2487, ptr %2486, 1
  %2489 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2488, i64 536, 2
  %2490 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2489, i64 4, 3, 0
  %2491 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2490, i64 32, 4, 0
  %2492 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2491, i64 4, 3, 1
  %2493 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2492, i64 1, 4, 1
  %2494 = call ptr @llvm.stacksave.p0()
  %2495 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2493, ptr %2495, align 8
  %2496 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2495, 1
  %2497 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1933, ptr %2497, align 8
  %2498 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2497, 1
  %2499 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2496, ptr %2499, align 8
  %2500 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2498, ptr %2500, align 8
  call void @memrefCopy(i64 4, ptr %2499, ptr %2500)
  call void @llvm.stackrestore.p0(ptr %2494)
  %2501 = call ptr @llvm.stacksave.p0()
  %2502 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1962, ptr %2502, align 8
  %2503 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2502, 1
  %2504 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %2504, align 8
  %2505 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2504, 1
  %2506 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2503, ptr %2506, align 8
  %2507 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2505, ptr %2507, align 8
  call void @memrefCopy(i64 4, ptr %2506, ptr %2507)
  call void @llvm.stackrestore.p0(ptr %2501)
  %2508 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %2509 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %2510 = insertvalue { ptr, ptr, i64 } poison, ptr %2508, 0
  %2511 = insertvalue { ptr, ptr, i64 } %2510, ptr %2509, 1
  %2512 = insertvalue { ptr, ptr, i64 } %2511, i64 0, 2
  %2513 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %2514 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %2515 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %2516 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %2517 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %2518 = extractvalue { ptr, ptr, i64 } %2512, 0
  %2519 = extractvalue { ptr, ptr, i64 } %2512, 1
  %2520 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2518, 0
  %2521 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2520, ptr %2519, 1
  %2522 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2521, i64 24, 2
  %2523 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2522, i64 16, 3, 0
  %2524 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2523, i64 32, 4, 0
  %2525 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2524, i64 4, 3, 1
  %2526 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2525, i64 1, 4, 1
  %2527 = call ptr @llvm.stacksave.p0()
  %2528 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2526, ptr %2528, align 8
  %2529 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2528, 1
  %2530 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %2530, align 8
  %2531 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2530, 1
  %2532 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2529, ptr %2532, align 8
  %2533 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2531, ptr %2533, align 8
  call void @memrefCopy(i64 4, ptr %2532, ptr %2533)
  call void @llvm.stackrestore.p0(ptr %2527)
  %2534 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 3, i32 0, i32 117)
  %2535 = call ptr @llvm.stacksave.p0()
  %2536 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2039, ptr %2536, align 8
  %2537 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2536, 1
  %2538 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %2538, align 8
  %2539 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2538, 1
  %2540 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2537, ptr %2540, align 8
  %2541 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2539, ptr %2541, align 8
  call void @memrefCopy(i64 4, ptr %2540, ptr %2541)
  call void @llvm.stackrestore.p0(ptr %2535)
  %2542 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %2543 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %2544 = insertvalue { ptr, ptr, i64 } poison, ptr %2542, 0
  %2545 = insertvalue { ptr, ptr, i64 } %2544, ptr %2543, 1
  %2546 = insertvalue { ptr, ptr, i64 } %2545, i64 0, 2
  %2547 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %2548 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %2549 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %2550 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %2551 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %2552 = extractvalue { ptr, ptr, i64 } %2546, 0
  %2553 = extractvalue { ptr, ptr, i64 } %2546, 1
  %2554 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2552, 0
  %2555 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2554, ptr %2553, 1
  %2556 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2555, i64 536, 2
  %2557 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2556, i64 16, 3, 0
  %2558 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2557, i64 32, 4, 0
  %2559 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2558, i64 4, 3, 1
  %2560 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2559, i64 1, 4, 1
  %2561 = call ptr @llvm.stacksave.p0()
  %2562 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2560, ptr %2562, align 8
  %2563 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2562, 1
  %2564 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %2564, align 8
  %2565 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2564, 1
  %2566 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2563, ptr %2566, align 8
  %2567 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2565, ptr %2567, align 8
  call void @memrefCopy(i64 4, ptr %2566, ptr %2567)
  call void @llvm.stackrestore.p0(ptr %2561)
  %2568 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 3, i32 42, i32 117)
  %2569 = call ptr @llvm.stacksave.p0()
  %2570 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2092, ptr %2570, align 8
  %2571 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2570, 1
  %2572 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %2572, align 8
  %2573 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2572, 1
  %2574 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2571, ptr %2574, align 8
  %2575 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2573, ptr %2575, align 8
  call void @memrefCopy(i64 4, ptr %2574, ptr %2575)
  call void @llvm.stackrestore.p0(ptr %2569)
  %2576 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %2577 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %2578 = insertvalue { ptr, ptr, i64 } poison, ptr %2576, 0
  %2579 = insertvalue { ptr, ptr, i64 } %2578, ptr %2577, 1
  %2580 = insertvalue { ptr, ptr, i64 } %2579, i64 0, 2
  %2581 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %2582 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %2583 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %2584 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %2585 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %2586 = extractvalue { ptr, ptr, i64 } %2580, 0
  %2587 = extractvalue { ptr, ptr, i64 } %2580, 1
  %2588 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2586, 0
  %2589 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2588, ptr %2587, 1
  %2590 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2589, i64 1048, 2
  %2591 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2590, i64 16, 3, 0
  %2592 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2591, i64 32, 4, 0
  %2593 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2592, i64 4, 3, 1
  %2594 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2593, i64 1, 4, 1
  %2595 = call ptr @llvm.stacksave.p0()
  %2596 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2594, ptr %2596, align 8
  %2597 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2596, 1
  %2598 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %2598, align 8
  %2599 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2598, 1
  %2600 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2597, ptr %2600, align 8
  %2601 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2599, ptr %2601, align 8
  call void @memrefCopy(i64 4, ptr %2600, ptr %2601)
  call void @llvm.stackrestore.p0(ptr %2595)
  %2602 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 3, i32 120, i32 117)
  %2603 = call ptr @llvm.stacksave.p0()
  %2604 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2145, ptr %2604, align 8
  %2605 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2604, 1
  %2606 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %2606, align 8
  %2607 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2606, 1
  %2608 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2605, ptr %2608, align 8
  %2609 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2607, ptr %2609, align 8
  call void @memrefCopy(i64 4, ptr %2608, ptr %2609)
  call void @llvm.stackrestore.p0(ptr %2603)
  %2610 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %2611 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %2612 = insertvalue { ptr, ptr, i64 } poison, ptr %2610, 0
  %2613 = insertvalue { ptr, ptr, i64 } %2612, ptr %2611, 1
  %2614 = insertvalue { ptr, ptr, i64 } %2613, i64 0, 2
  %2615 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %2616 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %2617 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %2618 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %2619 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %2620 = extractvalue { ptr, ptr, i64 } %2614, 0
  %2621 = extractvalue { ptr, ptr, i64 } %2614, 1
  %2622 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2620, 0
  %2623 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2622, ptr %2621, 1
  %2624 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2623, i64 1560, 2
  %2625 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2624, i64 16, 3, 0
  %2626 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2625, i64 32, 4, 0
  %2627 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2626, i64 4, 3, 1
  %2628 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2627, i64 1, 4, 1
  %2629 = call ptr @llvm.stacksave.p0()
  %2630 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2628, ptr %2630, align 8
  %2631 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2630, 1
  %2632 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %2632, align 8
  %2633 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2632, 1
  %2634 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2631, ptr %2634, align 8
  %2635 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2633, ptr %2635, align 8
  call void @memrefCopy(i64 4, ptr %2634, ptr %2635)
  call void @llvm.stackrestore.p0(ptr %2629)
  %2636 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 3, i32 189, i32 117)
  %2637 = call ptr @llvm.stacksave.p0()
  %2638 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2198, ptr %2638, align 8
  %2639 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2638, 1
  %2640 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2207, ptr %2640, align 8
  %2641 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2640, 1
  %2642 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2639, ptr %2642, align 8
  %2643 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2641, ptr %2643, align 8
  call void @memrefCopy(i64 4, ptr %2642, ptr %2643)
  call void @llvm.stackrestore.p0(ptr %2637)
  %2644 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %2645 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %2646 = insertvalue { ptr, ptr, i64 } poison, ptr %2644, 0
  %2647 = insertvalue { ptr, ptr, i64 } %2646, ptr %2645, 1
  %2648 = insertvalue { ptr, ptr, i64 } %2647, i64 0, 2
  %2649 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %2650 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %2651 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %2652 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %2653 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %2654 = extractvalue { ptr, ptr, i64 } %2648, 0
  %2655 = extractvalue { ptr, ptr, i64 } %2648, 1
  %2656 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2654, 0
  %2657 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2656, ptr %2655, 1
  %2658 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2657, i64 2072, 2
  %2659 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2658, i64 1, 3, 0
  %2660 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2659, i64 32, 4, 0
  %2661 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2660, i64 4, 3, 1
  %2662 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2661, i64 1, 4, 1
  %2663 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2662, 3, 0
  %2664 = mul i64 1, %2663
  %2665 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2662, 3, 1
  %2666 = mul i64 %2664, %2665
  %2667 = mul i64 %2666, 4
  %2668 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2662, 1
  %2669 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2662, 2
  %2670 = getelementptr float, ptr %2668, i64 %2669
  %2671 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2245, 1
  %2672 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2245, 2
  %2673 = getelementptr float, ptr %2671, i64 %2672
  call void @llvm.memcpy.p0.p0.i64(ptr %2673, ptr %2670, i64 %2667, i1 false)
  %2674 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 1, i32 4, ptr %2217, ptr %2259, ptr %1943, i32 3, i32 258, i32 102)
  %2675 = call ptr @llvm.stacksave.p0()
  %2676 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1933, ptr %2676, align 8
  %2677 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2676, 1
  %2678 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2493, ptr %2678, align 8
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
  %2696 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2695, i64 540, 2
  %2697 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2696, i64 4, 3, 0
  %2698 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2697, i64 32, 4, 0
  %2699 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2698, i64 4, 3, 1
  %2700 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2699, i64 1, 4, 1
  %2701 = call ptr @llvm.stacksave.p0()
  %2702 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2700, ptr %2702, align 8
  %2703 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2702, 1
  %2704 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1933, ptr %2704, align 8
  %2705 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2704, 1
  %2706 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2703, ptr %2706, align 8
  %2707 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2705, ptr %2707, align 8
  call void @memrefCopy(i64 4, ptr %2706, ptr %2707)
  call void @llvm.stackrestore.p0(ptr %2701)
  %2708 = call ptr @llvm.stacksave.p0()
  %2709 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1962, ptr %2709, align 8
  %2710 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2709, 1
  %2711 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %2711, align 8
  %2712 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2711, 1
  %2713 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2710, ptr %2713, align 8
  %2714 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2712, ptr %2714, align 8
  call void @memrefCopy(i64 4, ptr %2713, ptr %2714)
  call void @llvm.stackrestore.p0(ptr %2708)
  %2715 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %2716 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %2717 = insertvalue { ptr, ptr, i64 } poison, ptr %2715, 0
  %2718 = insertvalue { ptr, ptr, i64 } %2717, ptr %2716, 1
  %2719 = insertvalue { ptr, ptr, i64 } %2718, i64 0, 2
  %2720 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %2721 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %2722 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %2723 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %2724 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %2725 = extractvalue { ptr, ptr, i64 } %2719, 0
  %2726 = extractvalue { ptr, ptr, i64 } %2719, 1
  %2727 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2725, 0
  %2728 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2727, ptr %2726, 1
  %2729 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2728, i64 28, 2
  %2730 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2729, i64 16, 3, 0
  %2731 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2730, i64 32, 4, 0
  %2732 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2731, i64 4, 3, 1
  %2733 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2732, i64 1, 4, 1
  %2734 = call ptr @llvm.stacksave.p0()
  %2735 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2733, ptr %2735, align 8
  %2736 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2735, 1
  %2737 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %2737, align 8
  %2738 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2737, 1
  %2739 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2736, ptr %2739, align 8
  %2740 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2738, ptr %2740, align 8
  call void @memrefCopy(i64 4, ptr %2739, ptr %2740)
  call void @llvm.stackrestore.p0(ptr %2734)
  %2741 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 1, i32 327, i32 117)
  %2742 = call ptr @llvm.stacksave.p0()
  %2743 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2039, ptr %2743, align 8
  %2744 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2743, 1
  %2745 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %2745, align 8
  %2746 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2745, 1
  %2747 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2744, ptr %2747, align 8
  %2748 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2746, ptr %2748, align 8
  call void @memrefCopy(i64 4, ptr %2747, ptr %2748)
  call void @llvm.stackrestore.p0(ptr %2742)
  %2749 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %2750 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %2751 = insertvalue { ptr, ptr, i64 } poison, ptr %2749, 0
  %2752 = insertvalue { ptr, ptr, i64 } %2751, ptr %2750, 1
  %2753 = insertvalue { ptr, ptr, i64 } %2752, i64 0, 2
  %2754 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %2755 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %2756 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %2757 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %2758 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %2759 = extractvalue { ptr, ptr, i64 } %2753, 0
  %2760 = extractvalue { ptr, ptr, i64 } %2753, 1
  %2761 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2759, 0
  %2762 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2761, ptr %2760, 1
  %2763 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2762, i64 540, 2
  %2764 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2763, i64 16, 3, 0
  %2765 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2764, i64 32, 4, 0
  %2766 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2765, i64 4, 3, 1
  %2767 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2766, i64 1, 4, 1
  %2768 = call ptr @llvm.stacksave.p0()
  %2769 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2767, ptr %2769, align 8
  %2770 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2769, 1
  %2771 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %2771, align 8
  %2772 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2771, 1
  %2773 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2770, ptr %2773, align 8
  %2774 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2772, ptr %2774, align 8
  call void @memrefCopy(i64 4, ptr %2773, ptr %2774)
  call void @llvm.stackrestore.p0(ptr %2768)
  %2775 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 1, i32 396, i32 117)
  %2776 = call ptr @llvm.stacksave.p0()
  %2777 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2092, ptr %2777, align 8
  %2778 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2777, 1
  %2779 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %2779, align 8
  %2780 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2779, 1
  %2781 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2778, ptr %2781, align 8
  %2782 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2780, ptr %2782, align 8
  call void @memrefCopy(i64 4, ptr %2781, ptr %2782)
  call void @llvm.stackrestore.p0(ptr %2776)
  %2783 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %2784 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %2785 = insertvalue { ptr, ptr, i64 } poison, ptr %2783, 0
  %2786 = insertvalue { ptr, ptr, i64 } %2785, ptr %2784, 1
  %2787 = insertvalue { ptr, ptr, i64 } %2786, i64 0, 2
  %2788 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %2789 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %2790 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %2791 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %2792 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %2793 = extractvalue { ptr, ptr, i64 } %2787, 0
  %2794 = extractvalue { ptr, ptr, i64 } %2787, 1
  %2795 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2793, 0
  %2796 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2795, ptr %2794, 1
  %2797 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2796, i64 1052, 2
  %2798 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2797, i64 16, 3, 0
  %2799 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2798, i64 32, 4, 0
  %2800 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2799, i64 4, 3, 1
  %2801 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2800, i64 1, 4, 1
  %2802 = call ptr @llvm.stacksave.p0()
  %2803 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2801, ptr %2803, align 8
  %2804 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2803, 1
  %2805 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %2805, align 8
  %2806 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2805, 1
  %2807 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2804, ptr %2807, align 8
  %2808 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2806, ptr %2808, align 8
  call void @memrefCopy(i64 4, ptr %2807, ptr %2808)
  call void @llvm.stackrestore.p0(ptr %2802)
  %2809 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 1, i32 465, i32 117)
  %2810 = call ptr @llvm.stacksave.p0()
  %2811 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2145, ptr %2811, align 8
  %2812 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2811, 1
  %2813 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %2813, align 8
  %2814 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2813, 1
  %2815 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2812, ptr %2815, align 8
  %2816 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2814, ptr %2816, align 8
  call void @memrefCopy(i64 4, ptr %2815, ptr %2816)
  call void @llvm.stackrestore.p0(ptr %2810)
  %2817 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %2818 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %2819 = insertvalue { ptr, ptr, i64 } poison, ptr %2817, 0
  %2820 = insertvalue { ptr, ptr, i64 } %2819, ptr %2818, 1
  %2821 = insertvalue { ptr, ptr, i64 } %2820, i64 0, 2
  %2822 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %2823 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %2824 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %2825 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %2826 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %2827 = extractvalue { ptr, ptr, i64 } %2821, 0
  %2828 = extractvalue { ptr, ptr, i64 } %2821, 1
  %2829 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2827, 0
  %2830 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2829, ptr %2828, 1
  %2831 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2830, i64 1564, 2
  %2832 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2831, i64 16, 3, 0
  %2833 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2832, i64 32, 4, 0
  %2834 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2833, i64 4, 3, 1
  %2835 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2834, i64 1, 4, 1
  %2836 = call ptr @llvm.stacksave.p0()
  %2837 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2835, ptr %2837, align 8
  %2838 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2837, 1
  %2839 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %2839, align 8
  %2840 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2839, 1
  %2841 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2838, ptr %2841, align 8
  %2842 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2840, ptr %2842, align 8
  call void @memrefCopy(i64 4, ptr %2841, ptr %2842)
  call void @llvm.stackrestore.p0(ptr %2836)
  %2843 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 1, i32 534, i32 117)
  %2844 = call ptr @llvm.stacksave.p0()
  %2845 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2198, ptr %2845, align 8
  %2846 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2845, 1
  %2847 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2207, ptr %2847, align 8
  %2848 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2847, 1
  %2849 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2846, ptr %2849, align 8
  %2850 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2848, ptr %2850, align 8
  call void @memrefCopy(i64 4, ptr %2849, ptr %2850)
  call void @llvm.stackrestore.p0(ptr %2844)
  %2851 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %2852 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %2853 = insertvalue { ptr, ptr, i64 } poison, ptr %2851, 0
  %2854 = insertvalue { ptr, ptr, i64 } %2853, ptr %2852, 1
  %2855 = insertvalue { ptr, ptr, i64 } %2854, i64 0, 2
  %2856 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %2857 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %2858 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %2859 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %2860 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %2861 = extractvalue { ptr, ptr, i64 } %2855, 0
  %2862 = extractvalue { ptr, ptr, i64 } %2855, 1
  %2863 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2861, 0
  %2864 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2863, ptr %2862, 1
  %2865 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2864, i64 2076, 2
  %2866 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2865, i64 1, 3, 0
  %2867 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2866, i64 32, 4, 0
  %2868 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2867, i64 4, 3, 1
  %2869 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2868, i64 1, 4, 1
  %2870 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2869, 3, 0
  %2871 = mul i64 1, %2870
  %2872 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2869, 3, 1
  %2873 = mul i64 %2871, %2872
  %2874 = mul i64 %2873, 4
  %2875 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2869, 1
  %2876 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2869, 2
  %2877 = getelementptr float, ptr %2875, i64 %2876
  %2878 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2245, 1
  %2879 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2245, 2
  %2880 = getelementptr float, ptr %2878, i64 %2879
  call void @llvm.memcpy.p0.p0.i64(ptr %2880, ptr %2877, i64 %2874, i1 false)
  %2881 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 1, i32 4, ptr %2217, ptr %2259, ptr %1943, i32 1, i32 603, i32 102)
  %2882 = call ptr @llvm.stacksave.p0()
  %2883 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1933, ptr %2883, align 8
  %2884 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2883, 1
  %2885 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2700, ptr %2885, align 8
  %2886 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2885, 1
  %2887 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2884, ptr %2887, align 8
  %2888 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2886, ptr %2888, align 8
  call void @memrefCopy(i64 4, ptr %2887, ptr %2888)
  call void @llvm.stackrestore.p0(ptr %2882)
  %2889 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %2890 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %2891 = insertvalue { ptr, ptr, i64 } poison, ptr %2889, 0
  %2892 = insertvalue { ptr, ptr, i64 } %2891, ptr %2890, 1
  %2893 = insertvalue { ptr, ptr, i64 } %2892, i64 0, 2
  %2894 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %2895 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %2896 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %2897 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %2898 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %2899 = extractvalue { ptr, ptr, i64 } %2893, 0
  %2900 = extractvalue { ptr, ptr, i64 } %2893, 1
  %2901 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2899, 0
  %2902 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2901, ptr %2900, 1
  %2903 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2902, i64 656, 2
  %2904 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2903, i64 4, 3, 0
  %2905 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2904, i64 32, 4, 0
  %2906 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2905, i64 4, 3, 1
  %2907 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2906, i64 1, 4, 1
  %2908 = call ptr @llvm.stacksave.p0()
  %2909 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2907, ptr %2909, align 8
  %2910 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2909, 1
  %2911 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1933, ptr %2911, align 8
  %2912 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2911, 1
  %2913 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2910, ptr %2913, align 8
  %2914 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2912, ptr %2914, align 8
  call void @memrefCopy(i64 4, ptr %2913, ptr %2914)
  call void @llvm.stackrestore.p0(ptr %2908)
  %2915 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %2916 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %2917 = insertvalue { ptr, ptr, i64 } poison, ptr %2915, 0
  %2918 = insertvalue { ptr, ptr, i64 } %2917, ptr %2916, 1
  %2919 = insertvalue { ptr, ptr, i64 } %2918, i64 0, 2
  %2920 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %2921 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %2922 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %2923 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %2924 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %2925 = extractvalue { ptr, ptr, i64 } %2919, 0
  %2926 = extractvalue { ptr, ptr, i64 } %2919, 1
  %2927 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2925, 0
  %2928 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2927, ptr %2926, 1
  %2929 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2928, i64 1300, 2
  %2930 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2929, i64 4, 3, 0
  %2931 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2930, i64 65, 4, 0
  %2932 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2931, i64 16, 3, 1
  %2933 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2932, i64 1, 4, 1
  %2934 = call ptr @llvm.stacksave.p0()
  %2935 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2933, ptr %2935, align 8
  %2936 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2935, 1
  %2937 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %2937, align 8
  %2938 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2937, 1
  %2939 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2936, ptr %2939, align 8
  %2940 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2938, ptr %2940, align 8
  call void @memrefCopy(i64 4, ptr %2939, ptr %2940)
  call void @llvm.stackrestore.p0(ptr %2934)
  %2941 = call ptr @llvm.stacksave.p0()
  %2942 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2000, ptr %2942, align 8
  %2943 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2942, 1
  %2944 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %2944, align 8
  %2945 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2944, 1
  %2946 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2943, ptr %2946, align 8
  %2947 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2945, ptr %2947, align 8
  call void @memrefCopy(i64 4, ptr %2946, ptr %2947)
  call void @llvm.stackrestore.p0(ptr %2941)
  %2948 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 2, i32 327, i32 117)
  %2949 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %2950 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %2951 = insertvalue { ptr, ptr, i64 } poison, ptr %2949, 0
  %2952 = insertvalue { ptr, ptr, i64 } %2951, ptr %2950, 1
  %2953 = insertvalue { ptr, ptr, i64 } %2952, i64 0, 2
  %2954 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %2955 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %2956 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %2957 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %2958 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %2959 = extractvalue { ptr, ptr, i64 } %2953, 0
  %2960 = extractvalue { ptr, ptr, i64 } %2953, 1
  %2961 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2959, 0
  %2962 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2961, ptr %2960, 1
  %2963 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2962, i64 1316, 2
  %2964 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2963, i64 4, 3, 0
  %2965 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2964, i64 65, 4, 0
  %2966 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2965, i64 16, 3, 1
  %2967 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2966, i64 1, 4, 1
  %2968 = call ptr @llvm.stacksave.p0()
  %2969 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2967, ptr %2969, align 8
  %2970 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2969, 1
  %2971 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %2971, align 8
  %2972 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2971, 1
  %2973 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2970, ptr %2973, align 8
  %2974 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2972, ptr %2974, align 8
  call void @memrefCopy(i64 4, ptr %2973, ptr %2974)
  call void @llvm.stackrestore.p0(ptr %2968)
  %2975 = call ptr @llvm.stacksave.p0()
  %2976 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2065, ptr %2976, align 8
  %2977 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2976, 1
  %2978 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %2978, align 8
  %2979 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2978, 1
  %2980 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2977, ptr %2980, align 8
  %2981 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2979, ptr %2981, align 8
  call void @memrefCopy(i64 4, ptr %2980, ptr %2981)
  call void @llvm.stackrestore.p0(ptr %2975)
  %2982 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 2, i32 396, i32 117)
  %2983 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %2984 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %2985 = insertvalue { ptr, ptr, i64 } poison, ptr %2983, 0
  %2986 = insertvalue { ptr, ptr, i64 } %2985, ptr %2984, 1
  %2987 = insertvalue { ptr, ptr, i64 } %2986, i64 0, 2
  %2988 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %2989 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %2990 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %2991 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %2992 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %2993 = extractvalue { ptr, ptr, i64 } %2987, 0
  %2994 = extractvalue { ptr, ptr, i64 } %2987, 1
  %2995 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2993, 0
  %2996 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2995, ptr %2994, 1
  %2997 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2996, i64 1332, 2
  %2998 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2997, i64 4, 3, 0
  %2999 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2998, i64 65, 4, 0
  %3000 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2999, i64 16, 3, 1
  %3001 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3000, i64 1, 4, 1
  %3002 = call ptr @llvm.stacksave.p0()
  %3003 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3001, ptr %3003, align 8
  %3004 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3003, 1
  %3005 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %3005, align 8
  %3006 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3005, 1
  %3007 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3004, ptr %3007, align 8
  %3008 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3006, ptr %3008, align 8
  call void @memrefCopy(i64 4, ptr %3007, ptr %3008)
  call void @llvm.stackrestore.p0(ptr %3002)
  %3009 = call ptr @llvm.stacksave.p0()
  %3010 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2118, ptr %3010, align 8
  %3011 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3010, 1
  %3012 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %3012, align 8
  %3013 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3012, 1
  %3014 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3011, ptr %3014, align 8
  %3015 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3013, ptr %3015, align 8
  call void @memrefCopy(i64 4, ptr %3014, ptr %3015)
  call void @llvm.stackrestore.p0(ptr %3009)
  %3016 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 2, i32 465, i32 117)
  %3017 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %3018 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %3019 = insertvalue { ptr, ptr, i64 } poison, ptr %3017, 0
  %3020 = insertvalue { ptr, ptr, i64 } %3019, ptr %3018, 1
  %3021 = insertvalue { ptr, ptr, i64 } %3020, i64 0, 2
  %3022 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %3023 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %3024 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %3025 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %3026 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %3027 = extractvalue { ptr, ptr, i64 } %3021, 0
  %3028 = extractvalue { ptr, ptr, i64 } %3021, 1
  %3029 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %3027, 0
  %3030 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3029, ptr %3028, 1
  %3031 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3030, i64 1348, 2
  %3032 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3031, i64 4, 3, 0
  %3033 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3032, i64 65, 4, 0
  %3034 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3033, i64 16, 3, 1
  %3035 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3034, i64 1, 4, 1
  %3036 = call ptr @llvm.stacksave.p0()
  %3037 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3035, ptr %3037, align 8
  %3038 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3037, 1
  %3039 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %3039, align 8
  %3040 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3039, 1
  %3041 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3038, ptr %3041, align 8
  %3042 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3040, ptr %3042, align 8
  call void @memrefCopy(i64 4, ptr %3041, ptr %3042)
  call void @llvm.stackrestore.p0(ptr %3036)
  %3043 = call ptr @llvm.stacksave.p0()
  %3044 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2171, ptr %3044, align 8
  %3045 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3044, 1
  %3046 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %3046, align 8
  %3047 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3046, 1
  %3048 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3045, ptr %3048, align 8
  %3049 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3047, ptr %3049, align 8
  call void @memrefCopy(i64 4, ptr %3048, ptr %3049)
  call void @llvm.stackrestore.p0(ptr %3043)
  %3050 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 2, i32 534, i32 117)
  %3051 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %3052 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %3053 = insertvalue { ptr, ptr, i64 } poison, ptr %3051, 0
  %3054 = insertvalue { ptr, ptr, i64 } %3053, ptr %3052, 1
  %3055 = insertvalue { ptr, ptr, i64 } %3054, i64 0, 2
  %3056 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %3057 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %3058 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %3059 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %3060 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %3061 = extractvalue { ptr, ptr, i64 } %3055, 0
  %3062 = extractvalue { ptr, ptr, i64 } %3055, 1
  %3063 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %3061, 0
  %3064 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3063, ptr %3062, 1
  %3065 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3064, i64 1364, 2
  %3066 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3065, i64 4, 3, 0
  %3067 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3066, i64 65, 4, 0
  %3068 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3067, i64 1, 3, 1
  %3069 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3068, i64 1, 4, 1
  %3070 = call ptr @llvm.stacksave.p0()
  %3071 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3069, ptr %3071, align 8
  %3072 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3071, 1
  %3073 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2207, ptr %3073, align 8
  %3074 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3073, 1
  %3075 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3072, ptr %3075, align 8
  %3076 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3074, ptr %3076, align 8
  call void @memrefCopy(i64 4, ptr %3075, ptr %3076)
  call void @llvm.stackrestore.p0(ptr %3070)
  %3077 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2236, 3, 0
  %3078 = mul i64 1, %3077
  %3079 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2236, 3, 1
  %3080 = mul i64 %3078, %3079
  %3081 = mul i64 %3080, 4
  %3082 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2236, 1
  %3083 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2236, 2
  %3084 = getelementptr float, ptr %3082, i64 %3083
  %3085 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2245, 1
  %3086 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2245, 2
  %3087 = getelementptr float, ptr %3085, i64 %3086
  call void @llvm.memcpy.p0.p0.i64(ptr %3087, ptr %3084, i64 %3081, i1 false)
  %3088 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 1, i32 4, ptr %2217, ptr %2259, ptr %1943, i32 2, i32 603, i32 102)
  %3089 = call ptr @llvm.stacksave.p0()
  %3090 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1933, ptr %3090, align 8
  %3091 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3090, 1
  %3092 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2907, ptr %3092, align 8
  %3093 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3092, 1
  %3094 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3091, ptr %3094, align 8
  %3095 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3093, ptr %3095, align 8
  call void @memrefCopy(i64 4, ptr %3094, ptr %3095)
  call void @llvm.stackrestore.p0(ptr %3089)
  %3096 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %3097 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %3098 = insertvalue { ptr, ptr, i64 } poison, ptr %3096, 0
  %3099 = insertvalue { ptr, ptr, i64 } %3098, ptr %3097, 1
  %3100 = insertvalue { ptr, ptr, i64 } %3099, i64 0, 2
  %3101 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %3102 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %3103 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %3104 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %3105 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %3106 = extractvalue { ptr, ptr, i64 } %3100, 0
  %3107 = extractvalue { ptr, ptr, i64 } %3100, 1
  %3108 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %3106, 0
  %3109 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3108, ptr %3107, 1
  %3110 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3109, i64 660, 2
  %3111 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3110, i64 4, 3, 0
  %3112 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3111, i64 32, 4, 0
  %3113 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3112, i64 4, 3, 1
  %3114 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3113, i64 1, 4, 1
  %3115 = call ptr @llvm.stacksave.p0()
  %3116 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3114, ptr %3116, align 8
  %3117 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3116, 1
  %3118 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1933, ptr %3118, align 8
  %3119 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3118, 1
  %3120 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3117, ptr %3120, align 8
  %3121 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3119, ptr %3121, align 8
  call void @memrefCopy(i64 4, ptr %3120, ptr %3121)
  call void @llvm.stackrestore.p0(ptr %3115)
  %3122 = call ptr @llvm.stacksave.p0()
  %3123 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2933, ptr %3123, align 8
  %3124 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3123, 1
  %3125 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %3125, align 8
  %3126 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3125, 1
  %3127 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3124, ptr %3127, align 8
  %3128 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3126, ptr %3128, align 8
  call void @memrefCopy(i64 4, ptr %3127, ptr %3128)
  call void @llvm.stackrestore.p0(ptr %3122)
  %3129 = call ptr @llvm.stacksave.p0()
  %3130 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2319, ptr %3130, align 8
  %3131 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3130, 1
  %3132 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %3132, align 8
  %3133 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3132, 1
  %3134 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3131, ptr %3134, align 8
  %3135 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3133, ptr %3135, align 8
  call void @memrefCopy(i64 4, ptr %3134, ptr %3135)
  call void @llvm.stackrestore.p0(ptr %3129)
  %3136 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 3, i32 327, i32 117)
  %3137 = call ptr @llvm.stacksave.p0()
  %3138 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2967, ptr %3138, align 8
  %3139 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3138, 1
  %3140 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %3140, align 8
  %3141 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3140, 1
  %3142 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3139, ptr %3142, align 8
  %3143 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3141, ptr %3143, align 8
  call void @memrefCopy(i64 4, ptr %3142, ptr %3143)
  call void @llvm.stackrestore.p0(ptr %3137)
  %3144 = call ptr @llvm.stacksave.p0()
  %3145 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2353, ptr %3145, align 8
  %3146 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3145, 1
  %3147 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %3147, align 8
  %3148 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3147, 1
  %3149 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3146, ptr %3149, align 8
  %3150 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3148, ptr %3150, align 8
  call void @memrefCopy(i64 4, ptr %3149, ptr %3150)
  call void @llvm.stackrestore.p0(ptr %3144)
  %3151 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 3, i32 396, i32 117)
  %3152 = call ptr @llvm.stacksave.p0()
  %3153 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3001, ptr %3153, align 8
  %3154 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3153, 1
  %3155 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %3155, align 8
  %3156 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3155, 1
  %3157 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3154, ptr %3157, align 8
  %3158 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3156, ptr %3158, align 8
  call void @memrefCopy(i64 4, ptr %3157, ptr %3158)
  call void @llvm.stackrestore.p0(ptr %3152)
  %3159 = call ptr @llvm.stacksave.p0()
  %3160 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2387, ptr %3160, align 8
  %3161 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3160, 1
  %3162 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %3162, align 8
  %3163 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3162, 1
  %3164 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3161, ptr %3164, align 8
  %3165 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3163, ptr %3165, align 8
  call void @memrefCopy(i64 4, ptr %3164, ptr %3165)
  call void @llvm.stackrestore.p0(ptr %3159)
  %3166 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 3, i32 465, i32 117)
  %3167 = call ptr @llvm.stacksave.p0()
  %3168 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3035, ptr %3168, align 8
  %3169 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3168, 1
  %3170 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %3170, align 8
  %3171 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3170, 1
  %3172 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3169, ptr %3172, align 8
  %3173 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3171, ptr %3173, align 8
  call void @memrefCopy(i64 4, ptr %3172, ptr %3173)
  call void @llvm.stackrestore.p0(ptr %3167)
  %3174 = call ptr @llvm.stacksave.p0()
  %3175 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2421, ptr %3175, align 8
  %3176 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3175, 1
  %3177 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %3177, align 8
  %3178 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3177, 1
  %3179 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3176, ptr %3179, align 8
  %3180 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3178, ptr %3180, align 8
  call void @memrefCopy(i64 4, ptr %3179, ptr %3180)
  call void @llvm.stackrestore.p0(ptr %3174)
  %3181 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 3, i32 534, i32 117)
  %3182 = call ptr @llvm.stacksave.p0()
  %3183 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3069, ptr %3183, align 8
  %3184 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3183, 1
  %3185 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2207, ptr %3185, align 8
  %3186 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3185, 1
  %3187 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3184, ptr %3187, align 8
  %3188 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3186, ptr %3188, align 8
  call void @memrefCopy(i64 4, ptr %3187, ptr %3188)
  call void @llvm.stackrestore.p0(ptr %3182)
  %3189 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2455, 3, 0
  %3190 = mul i64 1, %3189
  %3191 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2455, 3, 1
  %3192 = mul i64 %3190, %3191
  %3193 = mul i64 %3192, 4
  %3194 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2455, 1
  %3195 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2455, 2
  %3196 = getelementptr float, ptr %3194, i64 %3195
  %3197 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2245, 1
  %3198 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2245, 2
  %3199 = getelementptr float, ptr %3197, i64 %3198
  call void @llvm.memcpy.p0.p0.i64(ptr %3199, ptr %3196, i64 %3193, i1 false)
  %3200 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 1, i32 4, ptr %2217, ptr %2259, ptr %1943, i32 3, i32 603, i32 102)
  %3201 = call ptr @llvm.stacksave.p0()
  %3202 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1933, ptr %3202, align 8
  %3203 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3202, 1
  %3204 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3114, ptr %3204, align 8
  %3205 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3204, 1
  %3206 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3203, ptr %3206, align 8
  %3207 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3205, ptr %3207, align 8
  call void @memrefCopy(i64 4, ptr %3206, ptr %3207)
  call void @llvm.stackrestore.p0(ptr %3201)
  %3208 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %3209 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %3210 = insertvalue { ptr, ptr, i64 } poison, ptr %3208, 0
  %3211 = insertvalue { ptr, ptr, i64 } %3210, ptr %3209, 1
  %3212 = insertvalue { ptr, ptr, i64 } %3211, i64 0, 2
  %3213 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %3214 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %3215 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %3216 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %3217 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %3218 = extractvalue { ptr, ptr, i64 } %3212, 0
  %3219 = extractvalue { ptr, ptr, i64 } %3212, 1
  %3220 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %3218, 0
  %3221 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3220, ptr %3219, 1
  %3222 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3221, i64 664, 2
  %3223 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3222, i64 4, 3, 0
  %3224 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3223, i64 32, 4, 0
  %3225 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3224, i64 4, 3, 1
  %3226 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3225, i64 1, 4, 1
  %3227 = call ptr @llvm.stacksave.p0()
  %3228 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3226, ptr %3228, align 8
  %3229 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3228, 1
  %3230 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1933, ptr %3230, align 8
  %3231 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3230, 1
  %3232 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3229, ptr %3232, align 8
  %3233 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3231, ptr %3233, align 8
  call void @memrefCopy(i64 4, ptr %3232, ptr %3233)
  call void @llvm.stackrestore.p0(ptr %3227)
  %3234 = call ptr @llvm.stacksave.p0()
  %3235 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2933, ptr %3235, align 8
  %3236 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3235, 1
  %3237 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %3237, align 8
  %3238 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3237, 1
  %3239 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3236, ptr %3239, align 8
  %3240 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3238, ptr %3240, align 8
  call void @memrefCopy(i64 4, ptr %3239, ptr %3240)
  call void @llvm.stackrestore.p0(ptr %3234)
  %3241 = call ptr @llvm.stacksave.p0()
  %3242 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2526, ptr %3242, align 8
  %3243 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3242, 1
  %3244 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %3244, align 8
  %3245 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3244, 1
  %3246 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3243, ptr %3246, align 8
  %3247 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3245, ptr %3247, align 8
  call void @memrefCopy(i64 4, ptr %3246, ptr %3247)
  call void @llvm.stackrestore.p0(ptr %3241)
  %3248 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 1, i32 672, i32 117)
  %3249 = call ptr @llvm.stacksave.p0()
  %3250 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2967, ptr %3250, align 8
  %3251 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3250, 1
  %3252 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %3252, align 8
  %3253 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3252, 1
  %3254 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3251, ptr %3254, align 8
  %3255 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3253, ptr %3255, align 8
  call void @memrefCopy(i64 4, ptr %3254, ptr %3255)
  call void @llvm.stackrestore.p0(ptr %3249)
  %3256 = call ptr @llvm.stacksave.p0()
  %3257 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2560, ptr %3257, align 8
  %3258 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3257, 1
  %3259 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %3259, align 8
  %3260 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3259, 1
  %3261 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3258, ptr %3261, align 8
  %3262 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3260, ptr %3262, align 8
  call void @memrefCopy(i64 4, ptr %3261, ptr %3262)
  call void @llvm.stackrestore.p0(ptr %3256)
  %3263 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 1, i32 741, i32 117)
  %3264 = call ptr @llvm.stacksave.p0()
  %3265 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3001, ptr %3265, align 8
  %3266 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3265, 1
  %3267 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %3267, align 8
  %3268 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3267, 1
  %3269 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3266, ptr %3269, align 8
  %3270 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3268, ptr %3270, align 8
  call void @memrefCopy(i64 4, ptr %3269, ptr %3270)
  call void @llvm.stackrestore.p0(ptr %3264)
  %3271 = call ptr @llvm.stacksave.p0()
  %3272 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2594, ptr %3272, align 8
  %3273 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3272, 1
  %3274 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %3274, align 8
  %3275 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3274, 1
  %3276 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3273, ptr %3276, align 8
  %3277 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3275, ptr %3277, align 8
  call void @memrefCopy(i64 4, ptr %3276, ptr %3277)
  call void @llvm.stackrestore.p0(ptr %3271)
  %3278 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 1, i32 810, i32 117)
  %3279 = call ptr @llvm.stacksave.p0()
  %3280 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3035, ptr %3280, align 8
  %3281 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3280, 1
  %3282 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %3282, align 8
  %3283 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3282, 1
  %3284 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3281, ptr %3284, align 8
  %3285 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3283, ptr %3285, align 8
  call void @memrefCopy(i64 4, ptr %3284, ptr %3285)
  call void @llvm.stackrestore.p0(ptr %3279)
  %3286 = call ptr @llvm.stacksave.p0()
  %3287 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2628, ptr %3287, align 8
  %3288 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3287, 1
  %3289 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %3289, align 8
  %3290 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3289, 1
  %3291 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3288, ptr %3291, align 8
  %3292 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3290, ptr %3292, align 8
  call void @memrefCopy(i64 4, ptr %3291, ptr %3292)
  call void @llvm.stackrestore.p0(ptr %3286)
  %3293 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 1, i32 879, i32 117)
  %3294 = call ptr @llvm.stacksave.p0()
  %3295 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3069, ptr %3295, align 8
  %3296 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3295, 1
  %3297 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2207, ptr %3297, align 8
  %3298 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3297, 1
  %3299 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3296, ptr %3299, align 8
  %3300 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3298, ptr %3300, align 8
  call void @memrefCopy(i64 4, ptr %3299, ptr %3300)
  call void @llvm.stackrestore.p0(ptr %3294)
  %3301 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2662, 3, 0
  %3302 = mul i64 1, %3301
  %3303 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2662, 3, 1
  %3304 = mul i64 %3302, %3303
  %3305 = mul i64 %3304, 4
  %3306 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2662, 1
  %3307 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2662, 2
  %3308 = getelementptr float, ptr %3306, i64 %3307
  %3309 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2245, 1
  %3310 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2245, 2
  %3311 = getelementptr float, ptr %3309, i64 %3310
  call void @llvm.memcpy.p0.p0.i64(ptr %3311, ptr %3308, i64 %3305, i1 false)
  %3312 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 1, i32 4, ptr %2217, ptr %2259, ptr %1943, i32 1, i32 948, i32 102)
  %3313 = call ptr @llvm.stacksave.p0()
  %3314 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1933, ptr %3314, align 8
  %3315 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3314, 1
  %3316 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3226, ptr %3316, align 8
  %3317 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3316, 1
  %3318 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3315, ptr %3318, align 8
  %3319 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3317, ptr %3319, align 8
  call void @memrefCopy(i64 4, ptr %3318, ptr %3319)
  call void @llvm.stackrestore.p0(ptr %3313)
  %3320 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %3321 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %3322 = insertvalue { ptr, ptr, i64 } poison, ptr %3320, 0
  %3323 = insertvalue { ptr, ptr, i64 } %3322, ptr %3321, 1
  %3324 = insertvalue { ptr, ptr, i64 } %3323, i64 0, 2
  %3325 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %3326 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %3327 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %3328 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %3329 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %3330 = extractvalue { ptr, ptr, i64 } %3324, 0
  %3331 = extractvalue { ptr, ptr, i64 } %3324, 1
  %3332 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %3330, 0
  %3333 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3332, ptr %3331, 1
  %3334 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3333, i64 668, 2
  %3335 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3334, i64 4, 3, 0
  %3336 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3335, i64 32, 4, 0
  %3337 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3336, i64 4, 3, 1
  %3338 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3337, i64 1, 4, 1
  %3339 = call ptr @llvm.stacksave.p0()
  %3340 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3338, ptr %3340, align 8
  %3341 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3340, 1
  %3342 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1933, ptr %3342, align 8
  %3343 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3342, 1
  %3344 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3341, ptr %3344, align 8
  %3345 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3343, ptr %3345, align 8
  call void @memrefCopy(i64 4, ptr %3344, ptr %3345)
  call void @llvm.stackrestore.p0(ptr %3339)
  %3346 = call ptr @llvm.stacksave.p0()
  %3347 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2933, ptr %3347, align 8
  %3348 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3347, 1
  %3349 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %3349, align 8
  %3350 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3349, 1
  %3351 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3348, ptr %3351, align 8
  %3352 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3350, ptr %3352, align 8
  call void @memrefCopy(i64 4, ptr %3351, ptr %3352)
  call void @llvm.stackrestore.p0(ptr %3346)
  %3353 = call ptr @llvm.stacksave.p0()
  %3354 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2733, ptr %3354, align 8
  %3355 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3354, 1
  %3356 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %3356, align 8
  %3357 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3356, 1
  %3358 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3355, ptr %3358, align 8
  %3359 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3357, ptr %3359, align 8
  call void @memrefCopy(i64 4, ptr %3358, ptr %3359)
  call void @llvm.stackrestore.p0(ptr %3353)
  %3360 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 2, i32 672, i32 117)
  %3361 = call ptr @llvm.stacksave.p0()
  %3362 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2967, ptr %3362, align 8
  %3363 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3362, 1
  %3364 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %3364, align 8
  %3365 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3364, 1
  %3366 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3363, ptr %3366, align 8
  %3367 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3365, ptr %3367, align 8
  call void @memrefCopy(i64 4, ptr %3366, ptr %3367)
  call void @llvm.stackrestore.p0(ptr %3361)
  %3368 = call ptr @llvm.stacksave.p0()
  %3369 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2767, ptr %3369, align 8
  %3370 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3369, 1
  %3371 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %3371, align 8
  %3372 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3371, 1
  %3373 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3370, ptr %3373, align 8
  %3374 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3372, ptr %3374, align 8
  call void @memrefCopy(i64 4, ptr %3373, ptr %3374)
  call void @llvm.stackrestore.p0(ptr %3368)
  %3375 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 2, i32 741, i32 117)
  %3376 = call ptr @llvm.stacksave.p0()
  %3377 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3001, ptr %3377, align 8
  %3378 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3377, 1
  %3379 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %3379, align 8
  %3380 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3379, 1
  %3381 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3378, ptr %3381, align 8
  %3382 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3380, ptr %3382, align 8
  call void @memrefCopy(i64 4, ptr %3381, ptr %3382)
  call void @llvm.stackrestore.p0(ptr %3376)
  %3383 = call ptr @llvm.stacksave.p0()
  %3384 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2801, ptr %3384, align 8
  %3385 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3384, 1
  %3386 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %3386, align 8
  %3387 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3386, 1
  %3388 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3385, ptr %3388, align 8
  %3389 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3387, ptr %3389, align 8
  call void @memrefCopy(i64 4, ptr %3388, ptr %3389)
  call void @llvm.stackrestore.p0(ptr %3383)
  %3390 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 2, i32 810, i32 117)
  %3391 = call ptr @llvm.stacksave.p0()
  %3392 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3035, ptr %3392, align 8
  %3393 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3392, 1
  %3394 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %3394, align 8
  %3395 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3394, 1
  %3396 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3393, ptr %3396, align 8
  %3397 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3395, ptr %3397, align 8
  call void @memrefCopy(i64 4, ptr %3396, ptr %3397)
  call void @llvm.stackrestore.p0(ptr %3391)
  %3398 = call ptr @llvm.stacksave.p0()
  %3399 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2835, ptr %3399, align 8
  %3400 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3399, 1
  %3401 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %3401, align 8
  %3402 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3401, 1
  %3403 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3400, ptr %3403, align 8
  %3404 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3402, ptr %3404, align 8
  call void @memrefCopy(i64 4, ptr %3403, ptr %3404)
  call void @llvm.stackrestore.p0(ptr %3398)
  %3405 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 2, i32 879, i32 117)
  %3406 = call ptr @llvm.stacksave.p0()
  %3407 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3069, ptr %3407, align 8
  %3408 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3407, 1
  %3409 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2207, ptr %3409, align 8
  %3410 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3409, 1
  %3411 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3408, ptr %3411, align 8
  %3412 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3410, ptr %3412, align 8
  call void @memrefCopy(i64 4, ptr %3411, ptr %3412)
  call void @llvm.stackrestore.p0(ptr %3406)
  %3413 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2869, 3, 0
  %3414 = mul i64 1, %3413
  %3415 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2869, 3, 1
  %3416 = mul i64 %3414, %3415
  %3417 = mul i64 %3416, 4
  %3418 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2869, 1
  %3419 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2869, 2
  %3420 = getelementptr float, ptr %3418, i64 %3419
  %3421 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2245, 1
  %3422 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2245, 2
  %3423 = getelementptr float, ptr %3421, i64 %3422
  call void @llvm.memcpy.p0.p0.i64(ptr %3423, ptr %3420, i64 %3417, i1 false)
  %3424 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 1, i32 4, ptr %2217, ptr %2259, ptr %1943, i32 2, i32 948, i32 102)
  %3425 = call ptr @llvm.stacksave.p0()
  %3426 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1933, ptr %3426, align 8
  %3427 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3426, 1
  %3428 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3338, ptr %3428, align 8
  %3429 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3428, 1
  %3430 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3427, ptr %3430, align 8
  %3431 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3429, ptr %3431, align 8
  call void @memrefCopy(i64 4, ptr %3430, ptr %3431)
  call void @llvm.stackrestore.p0(ptr %3425)
  %3432 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %3433 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %3434 = insertvalue { ptr, ptr, i64 } poison, ptr %3432, 0
  %3435 = insertvalue { ptr, ptr, i64 } %3434, ptr %3433, 1
  %3436 = insertvalue { ptr, ptr, i64 } %3435, i64 0, 2
  %3437 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %3438 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %3439 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %3440 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %3441 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %3442 = extractvalue { ptr, ptr, i64 } %3436, 0
  %3443 = extractvalue { ptr, ptr, i64 } %3436, 1
  %3444 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %3442, 0
  %3445 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3444, ptr %3443, 1
  %3446 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3445, i64 768, 2
  %3447 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3446, i64 4, 3, 0
  %3448 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3447, i64 32, 4, 0
  %3449 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3448, i64 4, 3, 1
  %3450 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3449, i64 1, 4, 1
  %3451 = call ptr @llvm.stacksave.p0()
  %3452 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3450, ptr %3452, align 8
  %3453 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3452, 1
  %3454 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1933, ptr %3454, align 8
  %3455 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3454, 1
  %3456 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3453, ptr %3456, align 8
  %3457 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3455, ptr %3457, align 8
  call void @memrefCopy(i64 4, ptr %3456, ptr %3457)
  call void @llvm.stackrestore.p0(ptr %3451)
  %3458 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %3459 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %3460 = insertvalue { ptr, ptr, i64 } poison, ptr %3458, 0
  %3461 = insertvalue { ptr, ptr, i64 } %3460, ptr %3459, 1
  %3462 = insertvalue { ptr, ptr, i64 } %3461, i64 0, 2
  %3463 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %3464 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %3465 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %3466 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %3467 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %3468 = extractvalue { ptr, ptr, i64 } %3462, 0
  %3469 = extractvalue { ptr, ptr, i64 } %3462, 1
  %3470 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %3468, 0
  %3471 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3470, ptr %3469, 1
  %3472 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3471, i64 1560, 2
  %3473 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3472, i64 4, 3, 0
  %3474 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3473, i64 65, 4, 0
  %3475 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3474, i64 16, 3, 1
  %3476 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3475, i64 1, 4, 1
  %3477 = call ptr @llvm.stacksave.p0()
  %3478 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3476, ptr %3478, align 8
  %3479 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3478, 1
  %3480 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %3480, align 8
  %3481 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3480, 1
  %3482 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3479, ptr %3482, align 8
  %3483 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3481, ptr %3483, align 8
  call void @memrefCopy(i64 4, ptr %3482, ptr %3483)
  call void @llvm.stackrestore.p0(ptr %3477)
  %3484 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %3485 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %3486 = insertvalue { ptr, ptr, i64 } poison, ptr %3484, 0
  %3487 = insertvalue { ptr, ptr, i64 } %3486, ptr %3485, 1
  %3488 = insertvalue { ptr, ptr, i64 } %3487, i64 0, 2
  %3489 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %3490 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %3491 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %3492 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %3493 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %3494 = extractvalue { ptr, ptr, i64 } %3488, 0
  %3495 = extractvalue { ptr, ptr, i64 } %3488, 1
  %3496 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %3494, 0
  %3497 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3496, ptr %3495, 1
  %3498 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3497, i64 0, 2
  %3499 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3498, i64 16, 3, 0
  %3500 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3499, i64 32, 4, 0
  %3501 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3500, i64 4, 3, 1
  %3502 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3501, i64 1, 4, 1
  %3503 = call ptr @llvm.stacksave.p0()
  %3504 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3502, ptr %3504, align 8
  %3505 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3504, 1
  %3506 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %3506, align 8
  %3507 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3506, 1
  %3508 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3505, ptr %3508, align 8
  %3509 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3507, ptr %3509, align 8
  call void @memrefCopy(i64 4, ptr %3508, ptr %3509)
  call void @llvm.stackrestore.p0(ptr %3503)
  %3510 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 3, i32 672, i32 117)
  %3511 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %3512 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %3513 = insertvalue { ptr, ptr, i64 } poison, ptr %3511, 0
  %3514 = insertvalue { ptr, ptr, i64 } %3513, ptr %3512, 1
  %3515 = insertvalue { ptr, ptr, i64 } %3514, i64 0, 2
  %3516 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %3517 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %3518 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %3519 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %3520 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %3521 = extractvalue { ptr, ptr, i64 } %3515, 0
  %3522 = extractvalue { ptr, ptr, i64 } %3515, 1
  %3523 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %3521, 0
  %3524 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3523, ptr %3522, 1
  %3525 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3524, i64 1576, 2
  %3526 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3525, i64 4, 3, 0
  %3527 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3526, i64 65, 4, 0
  %3528 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3527, i64 16, 3, 1
  %3529 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3528, i64 1, 4, 1
  %3530 = call ptr @llvm.stacksave.p0()
  %3531 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3529, ptr %3531, align 8
  %3532 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3531, 1
  %3533 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %3533, align 8
  %3534 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3533, 1
  %3535 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3532, ptr %3535, align 8
  %3536 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3534, ptr %3536, align 8
  call void @memrefCopy(i64 4, ptr %3535, ptr %3536)
  call void @llvm.stackrestore.p0(ptr %3530)
  %3537 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %3538 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %3539 = insertvalue { ptr, ptr, i64 } poison, ptr %3537, 0
  %3540 = insertvalue { ptr, ptr, i64 } %3539, ptr %3538, 1
  %3541 = insertvalue { ptr, ptr, i64 } %3540, i64 0, 2
  %3542 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %3543 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %3544 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %3545 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %3546 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %3547 = extractvalue { ptr, ptr, i64 } %3541, 0
  %3548 = extractvalue { ptr, ptr, i64 } %3541, 1
  %3549 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %3547, 0
  %3550 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3549, ptr %3548, 1
  %3551 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3550, i64 512, 2
  %3552 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3551, i64 16, 3, 0
  %3553 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3552, i64 32, 4, 0
  %3554 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3553, i64 4, 3, 1
  %3555 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3554, i64 1, 4, 1
  %3556 = call ptr @llvm.stacksave.p0()
  %3557 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3555, ptr %3557, align 8
  %3558 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3557, 1
  %3559 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %3559, align 8
  %3560 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3559, 1
  %3561 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3558, ptr %3561, align 8
  %3562 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3560, ptr %3562, align 8
  call void @memrefCopy(i64 4, ptr %3561, ptr %3562)
  call void @llvm.stackrestore.p0(ptr %3556)
  %3563 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 3, i32 741, i32 117)
  %3564 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %3565 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %3566 = insertvalue { ptr, ptr, i64 } poison, ptr %3564, 0
  %3567 = insertvalue { ptr, ptr, i64 } %3566, ptr %3565, 1
  %3568 = insertvalue { ptr, ptr, i64 } %3567, i64 0, 2
  %3569 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %3570 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %3571 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %3572 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %3573 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %3574 = extractvalue { ptr, ptr, i64 } %3568, 0
  %3575 = extractvalue { ptr, ptr, i64 } %3568, 1
  %3576 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %3574, 0
  %3577 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3576, ptr %3575, 1
  %3578 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3577, i64 1592, 2
  %3579 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3578, i64 4, 3, 0
  %3580 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3579, i64 65, 4, 0
  %3581 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3580, i64 16, 3, 1
  %3582 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3581, i64 1, 4, 1
  %3583 = call ptr @llvm.stacksave.p0()
  %3584 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3582, ptr %3584, align 8
  %3585 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3584, 1
  %3586 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %3586, align 8
  %3587 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3586, 1
  %3588 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3585, ptr %3588, align 8
  %3589 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3587, ptr %3589, align 8
  call void @memrefCopy(i64 4, ptr %3588, ptr %3589)
  call void @llvm.stackrestore.p0(ptr %3583)
  %3590 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %3591 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %3592 = insertvalue { ptr, ptr, i64 } poison, ptr %3590, 0
  %3593 = insertvalue { ptr, ptr, i64 } %3592, ptr %3591, 1
  %3594 = insertvalue { ptr, ptr, i64 } %3593, i64 0, 2
  %3595 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %3596 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %3597 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %3598 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %3599 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %3600 = extractvalue { ptr, ptr, i64 } %3594, 0
  %3601 = extractvalue { ptr, ptr, i64 } %3594, 1
  %3602 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %3600, 0
  %3603 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3602, ptr %3601, 1
  %3604 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3603, i64 1024, 2
  %3605 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3604, i64 16, 3, 0
  %3606 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3605, i64 32, 4, 0
  %3607 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3606, i64 4, 3, 1
  %3608 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3607, i64 1, 4, 1
  %3609 = call ptr @llvm.stacksave.p0()
  %3610 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3608, ptr %3610, align 8
  %3611 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3610, 1
  %3612 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %3612, align 8
  %3613 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3612, 1
  %3614 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3611, ptr %3614, align 8
  %3615 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3613, ptr %3615, align 8
  call void @memrefCopy(i64 4, ptr %3614, ptr %3615)
  call void @llvm.stackrestore.p0(ptr %3609)
  %3616 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 3, i32 810, i32 117)
  %3617 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %3618 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %3619 = insertvalue { ptr, ptr, i64 } poison, ptr %3617, 0
  %3620 = insertvalue { ptr, ptr, i64 } %3619, ptr %3618, 1
  %3621 = insertvalue { ptr, ptr, i64 } %3620, i64 0, 2
  %3622 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %3623 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %3624 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %3625 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %3626 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %3627 = extractvalue { ptr, ptr, i64 } %3621, 0
  %3628 = extractvalue { ptr, ptr, i64 } %3621, 1
  %3629 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %3627, 0
  %3630 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3629, ptr %3628, 1
  %3631 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3630, i64 1608, 2
  %3632 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3631, i64 4, 3, 0
  %3633 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3632, i64 65, 4, 0
  %3634 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3633, i64 16, 3, 1
  %3635 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3634, i64 1, 4, 1
  %3636 = call ptr @llvm.stacksave.p0()
  %3637 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3635, ptr %3637, align 8
  %3638 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3637, 1
  %3639 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %3639, align 8
  %3640 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3639, 1
  %3641 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3638, ptr %3641, align 8
  %3642 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3640, ptr %3642, align 8
  call void @memrefCopy(i64 4, ptr %3641, ptr %3642)
  call void @llvm.stackrestore.p0(ptr %3636)
  %3643 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %3644 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %3645 = insertvalue { ptr, ptr, i64 } poison, ptr %3643, 0
  %3646 = insertvalue { ptr, ptr, i64 } %3645, ptr %3644, 1
  %3647 = insertvalue { ptr, ptr, i64 } %3646, i64 0, 2
  %3648 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %3649 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %3650 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %3651 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %3652 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %3653 = extractvalue { ptr, ptr, i64 } %3647, 0
  %3654 = extractvalue { ptr, ptr, i64 } %3647, 1
  %3655 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %3653, 0
  %3656 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3655, ptr %3654, 1
  %3657 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3656, i64 1536, 2
  %3658 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3657, i64 16, 3, 0
  %3659 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3658, i64 32, 4, 0
  %3660 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3659, i64 4, 3, 1
  %3661 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3660, i64 1, 4, 1
  %3662 = call ptr @llvm.stacksave.p0()
  %3663 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3661, ptr %3663, align 8
  %3664 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3663, 1
  %3665 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %3665, align 8
  %3666 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3665, 1
  %3667 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3664, ptr %3667, align 8
  %3668 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3666, ptr %3668, align 8
  call void @memrefCopy(i64 4, ptr %3667, ptr %3668)
  call void @llvm.stackrestore.p0(ptr %3662)
  %3669 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 3, i32 879, i32 117)
  %3670 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %3671 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %3672 = insertvalue { ptr, ptr, i64 } poison, ptr %3670, 0
  %3673 = insertvalue { ptr, ptr, i64 } %3672, ptr %3671, 1
  %3674 = insertvalue { ptr, ptr, i64 } %3673, i64 0, 2
  %3675 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %3676 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %3677 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %3678 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %3679 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %3680 = extractvalue { ptr, ptr, i64 } %3674, 0
  %3681 = extractvalue { ptr, ptr, i64 } %3674, 1
  %3682 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %3680, 0
  %3683 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3682, ptr %3681, 1
  %3684 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3683, i64 1624, 2
  %3685 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3684, i64 4, 3, 0
  %3686 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3685, i64 65, 4, 0
  %3687 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3686, i64 1, 3, 1
  %3688 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3687, i64 1, 4, 1
  %3689 = call ptr @llvm.stacksave.p0()
  %3690 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3688, ptr %3690, align 8
  %3691 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3690, 1
  %3692 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2207, ptr %3692, align 8
  %3693 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3692, 1
  %3694 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3691, ptr %3694, align 8
  %3695 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3693, ptr %3695, align 8
  call void @memrefCopy(i64 4, ptr %3694, ptr %3695)
  call void @llvm.stackrestore.p0(ptr %3689)
  %3696 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %3697 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %3698 = insertvalue { ptr, ptr, i64 } poison, ptr %3696, 0
  %3699 = insertvalue { ptr, ptr, i64 } %3698, ptr %3697, 1
  %3700 = insertvalue { ptr, ptr, i64 } %3699, i64 0, 2
  %3701 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %3702 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %3703 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %3704 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %3705 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %3706 = extractvalue { ptr, ptr, i64 } %3700, 0
  %3707 = extractvalue { ptr, ptr, i64 } %3700, 1
  %3708 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %3706, 0
  %3709 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3708, ptr %3707, 1
  %3710 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3709, i64 2048, 2
  %3711 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3710, i64 1, 3, 0
  %3712 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3711, i64 32, 4, 0
  %3713 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3712, i64 4, 3, 1
  %3714 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3713, i64 1, 4, 1
  %3715 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3714, 3, 0
  %3716 = mul i64 1, %3715
  %3717 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3714, 3, 1
  %3718 = mul i64 %3716, %3717
  %3719 = mul i64 %3718, 4
  %3720 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3714, 1
  %3721 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3714, 2
  %3722 = getelementptr float, ptr %3720, i64 %3721
  %3723 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2245, 1
  %3724 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2245, 2
  %3725 = getelementptr float, ptr %3723, i64 %3724
  call void @llvm.memcpy.p0.p0.i64(ptr %3725, ptr %3722, i64 %3719, i1 false)
  %3726 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 1, i32 4, ptr %2217, ptr %2259, ptr %1943, i32 3, i32 948, i32 102)
  %3727 = call ptr @llvm.stacksave.p0()
  %3728 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1933, ptr %3728, align 8
  %3729 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3728, 1
  %3730 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3450, ptr %3730, align 8
  %3731 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3730, 1
  %3732 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3729, ptr %3732, align 8
  %3733 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3731, ptr %3733, align 8
  call void @memrefCopy(i64 4, ptr %3732, ptr %3733)
  call void @llvm.stackrestore.p0(ptr %3727)
  %3734 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %3735 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %3736 = insertvalue { ptr, ptr, i64 } poison, ptr %3734, 0
  %3737 = insertvalue { ptr, ptr, i64 } %3736, ptr %3735, 1
  %3738 = insertvalue { ptr, ptr, i64 } %3737, i64 0, 2
  %3739 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %3740 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %3741 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %3742 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %3743 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %3744 = extractvalue { ptr, ptr, i64 } %3738, 0
  %3745 = extractvalue { ptr, ptr, i64 } %3738, 1
  %3746 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %3744, 0
  %3747 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3746, ptr %3745, 1
  %3748 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3747, i64 772, 2
  %3749 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3748, i64 4, 3, 0
  %3750 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3749, i64 32, 4, 0
  %3751 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3750, i64 4, 3, 1
  %3752 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3751, i64 1, 4, 1
  %3753 = call ptr @llvm.stacksave.p0()
  %3754 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3752, ptr %3754, align 8
  %3755 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3754, 1
  %3756 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1933, ptr %3756, align 8
  %3757 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3756, 1
  %3758 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3755, ptr %3758, align 8
  %3759 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3757, ptr %3759, align 8
  call void @memrefCopy(i64 4, ptr %3758, ptr %3759)
  call void @llvm.stackrestore.p0(ptr %3753)
  %3760 = call ptr @llvm.stacksave.p0()
  %3761 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3476, ptr %3761, align 8
  %3762 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3761, 1
  %3763 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %3763, align 8
  %3764 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3763, 1
  %3765 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3762, ptr %3765, align 8
  %3766 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3764, ptr %3766, align 8
  call void @memrefCopy(i64 4, ptr %3765, ptr %3766)
  call void @llvm.stackrestore.p0(ptr %3760)
  %3767 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %3768 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %3769 = insertvalue { ptr, ptr, i64 } poison, ptr %3767, 0
  %3770 = insertvalue { ptr, ptr, i64 } %3769, ptr %3768, 1
  %3771 = insertvalue { ptr, ptr, i64 } %3770, i64 0, 2
  %3772 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %3773 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %3774 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %3775 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %3776 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %3777 = extractvalue { ptr, ptr, i64 } %3771, 0
  %3778 = extractvalue { ptr, ptr, i64 } %3771, 1
  %3779 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %3777, 0
  %3780 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3779, ptr %3778, 1
  %3781 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3780, i64 4, 2
  %3782 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3781, i64 16, 3, 0
  %3783 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3782, i64 32, 4, 0
  %3784 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3783, i64 4, 3, 1
  %3785 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3784, i64 1, 4, 1
  %3786 = call ptr @llvm.stacksave.p0()
  %3787 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3785, ptr %3787, align 8
  %3788 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3787, 1
  %3789 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %3789, align 8
  %3790 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3789, 1
  %3791 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3788, ptr %3791, align 8
  %3792 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3790, ptr %3792, align 8
  call void @memrefCopy(i64 4, ptr %3791, ptr %3792)
  call void @llvm.stackrestore.p0(ptr %3786)
  %3793 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 1, i32 1017, i32 117)
  %3794 = call ptr @llvm.stacksave.p0()
  %3795 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3529, ptr %3795, align 8
  %3796 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3795, 1
  %3797 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %3797, align 8
  %3798 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3797, 1
  %3799 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3796, ptr %3799, align 8
  %3800 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3798, ptr %3800, align 8
  call void @memrefCopy(i64 4, ptr %3799, ptr %3800)
  call void @llvm.stackrestore.p0(ptr %3794)
  %3801 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %3802 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %3803 = insertvalue { ptr, ptr, i64 } poison, ptr %3801, 0
  %3804 = insertvalue { ptr, ptr, i64 } %3803, ptr %3802, 1
  %3805 = insertvalue { ptr, ptr, i64 } %3804, i64 0, 2
  %3806 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %3807 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %3808 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %3809 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %3810 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %3811 = extractvalue { ptr, ptr, i64 } %3805, 0
  %3812 = extractvalue { ptr, ptr, i64 } %3805, 1
  %3813 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %3811, 0
  %3814 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3813, ptr %3812, 1
  %3815 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3814, i64 516, 2
  %3816 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3815, i64 16, 3, 0
  %3817 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3816, i64 32, 4, 0
  %3818 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3817, i64 4, 3, 1
  %3819 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3818, i64 1, 4, 1
  %3820 = call ptr @llvm.stacksave.p0()
  %3821 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3819, ptr %3821, align 8
  %3822 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3821, 1
  %3823 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %3823, align 8
  %3824 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3823, 1
  %3825 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3822, ptr %3825, align 8
  %3826 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3824, ptr %3826, align 8
  call void @memrefCopy(i64 4, ptr %3825, ptr %3826)
  call void @llvm.stackrestore.p0(ptr %3820)
  %3827 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 1, i32 1086, i32 117)
  %3828 = call ptr @llvm.stacksave.p0()
  %3829 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3582, ptr %3829, align 8
  %3830 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3829, 1
  %3831 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %3831, align 8
  %3832 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3831, 1
  %3833 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3830, ptr %3833, align 8
  %3834 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3832, ptr %3834, align 8
  call void @memrefCopy(i64 4, ptr %3833, ptr %3834)
  call void @llvm.stackrestore.p0(ptr %3828)
  %3835 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %3836 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %3837 = insertvalue { ptr, ptr, i64 } poison, ptr %3835, 0
  %3838 = insertvalue { ptr, ptr, i64 } %3837, ptr %3836, 1
  %3839 = insertvalue { ptr, ptr, i64 } %3838, i64 0, 2
  %3840 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %3841 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %3842 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %3843 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %3844 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %3845 = extractvalue { ptr, ptr, i64 } %3839, 0
  %3846 = extractvalue { ptr, ptr, i64 } %3839, 1
  %3847 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %3845, 0
  %3848 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3847, ptr %3846, 1
  %3849 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3848, i64 1028, 2
  %3850 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3849, i64 16, 3, 0
  %3851 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3850, i64 32, 4, 0
  %3852 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3851, i64 4, 3, 1
  %3853 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3852, i64 1, 4, 1
  %3854 = call ptr @llvm.stacksave.p0()
  %3855 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3853, ptr %3855, align 8
  %3856 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3855, 1
  %3857 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %3857, align 8
  %3858 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3857, 1
  %3859 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3856, ptr %3859, align 8
  %3860 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3858, ptr %3860, align 8
  call void @memrefCopy(i64 4, ptr %3859, ptr %3860)
  call void @llvm.stackrestore.p0(ptr %3854)
  %3861 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 1, i32 1155, i32 117)
  %3862 = call ptr @llvm.stacksave.p0()
  %3863 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3635, ptr %3863, align 8
  %3864 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3863, 1
  %3865 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %3865, align 8
  %3866 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3865, 1
  %3867 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3864, ptr %3867, align 8
  %3868 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3866, ptr %3868, align 8
  call void @memrefCopy(i64 4, ptr %3867, ptr %3868)
  call void @llvm.stackrestore.p0(ptr %3862)
  %3869 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %3870 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %3871 = insertvalue { ptr, ptr, i64 } poison, ptr %3869, 0
  %3872 = insertvalue { ptr, ptr, i64 } %3871, ptr %3870, 1
  %3873 = insertvalue { ptr, ptr, i64 } %3872, i64 0, 2
  %3874 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %3875 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %3876 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %3877 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %3878 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %3879 = extractvalue { ptr, ptr, i64 } %3873, 0
  %3880 = extractvalue { ptr, ptr, i64 } %3873, 1
  %3881 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %3879, 0
  %3882 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3881, ptr %3880, 1
  %3883 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3882, i64 1540, 2
  %3884 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3883, i64 16, 3, 0
  %3885 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3884, i64 32, 4, 0
  %3886 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3885, i64 4, 3, 1
  %3887 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3886, i64 1, 4, 1
  %3888 = call ptr @llvm.stacksave.p0()
  %3889 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3887, ptr %3889, align 8
  %3890 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3889, 1
  %3891 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %3891, align 8
  %3892 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3891, 1
  %3893 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3890, ptr %3893, align 8
  %3894 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3892, ptr %3894, align 8
  call void @memrefCopy(i64 4, ptr %3893, ptr %3894)
  call void @llvm.stackrestore.p0(ptr %3888)
  %3895 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 1, i32 1224, i32 117)
  %3896 = call ptr @llvm.stacksave.p0()
  %3897 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3688, ptr %3897, align 8
  %3898 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3897, 1
  %3899 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2207, ptr %3899, align 8
  %3900 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3899, 1
  %3901 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3898, ptr %3901, align 8
  %3902 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3900, ptr %3902, align 8
  call void @memrefCopy(i64 4, ptr %3901, ptr %3902)
  call void @llvm.stackrestore.p0(ptr %3896)
  %3903 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %3904 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %3905 = insertvalue { ptr, ptr, i64 } poison, ptr %3903, 0
  %3906 = insertvalue { ptr, ptr, i64 } %3905, ptr %3904, 1
  %3907 = insertvalue { ptr, ptr, i64 } %3906, i64 0, 2
  %3908 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %3909 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %3910 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %3911 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %3912 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %3913 = extractvalue { ptr, ptr, i64 } %3907, 0
  %3914 = extractvalue { ptr, ptr, i64 } %3907, 1
  %3915 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %3913, 0
  %3916 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3915, ptr %3914, 1
  %3917 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3916, i64 2052, 2
  %3918 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3917, i64 1, 3, 0
  %3919 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3918, i64 32, 4, 0
  %3920 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3919, i64 4, 3, 1
  %3921 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3920, i64 1, 4, 1
  %3922 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3921, 3, 0
  %3923 = mul i64 1, %3922
  %3924 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3921, 3, 1
  %3925 = mul i64 %3923, %3924
  %3926 = mul i64 %3925, 4
  %3927 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3921, 1
  %3928 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3921, 2
  %3929 = getelementptr float, ptr %3927, i64 %3928
  %3930 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2245, 1
  %3931 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2245, 2
  %3932 = getelementptr float, ptr %3930, i64 %3931
  call void @llvm.memcpy.p0.p0.i64(ptr %3932, ptr %3929, i64 %3926, i1 false)
  %3933 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 1, i32 4, ptr %2217, ptr %2259, ptr %1943, i32 1, i32 1293, i32 102)
  %3934 = call ptr @llvm.stacksave.p0()
  %3935 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1933, ptr %3935, align 8
  %3936 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3935, 1
  %3937 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3752, ptr %3937, align 8
  %3938 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3937, 1
  %3939 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3936, ptr %3939, align 8
  %3940 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3938, ptr %3940, align 8
  call void @memrefCopy(i64 4, ptr %3939, ptr %3940)
  call void @llvm.stackrestore.p0(ptr %3934)
  %3941 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %3942 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %3943 = insertvalue { ptr, ptr, i64 } poison, ptr %3941, 0
  %3944 = insertvalue { ptr, ptr, i64 } %3943, ptr %3942, 1
  %3945 = insertvalue { ptr, ptr, i64 } %3944, i64 0, 2
  %3946 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %3947 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %3948 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %3949 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %3950 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %3951 = extractvalue { ptr, ptr, i64 } %3945, 0
  %3952 = extractvalue { ptr, ptr, i64 } %3945, 1
  %3953 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %3951, 0
  %3954 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3953, ptr %3952, 1
  %3955 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3954, i64 776, 2
  %3956 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3955, i64 4, 3, 0
  %3957 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3956, i64 32, 4, 0
  %3958 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3957, i64 4, 3, 1
  %3959 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3958, i64 1, 4, 1
  %3960 = call ptr @llvm.stacksave.p0()
  %3961 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3959, ptr %3961, align 8
  %3962 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3961, 1
  %3963 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1933, ptr %3963, align 8
  %3964 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3963, 1
  %3965 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3962, ptr %3965, align 8
  %3966 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3964, ptr %3966, align 8
  call void @memrefCopy(i64 4, ptr %3965, ptr %3966)
  call void @llvm.stackrestore.p0(ptr %3960)
  %3967 = call ptr @llvm.stacksave.p0()
  %3968 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3476, ptr %3968, align 8
  %3969 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3968, 1
  %3970 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %3970, align 8
  %3971 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3970, 1
  %3972 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3969, ptr %3972, align 8
  %3973 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3971, ptr %3973, align 8
  call void @memrefCopy(i64 4, ptr %3972, ptr %3973)
  call void @llvm.stackrestore.p0(ptr %3967)
  %3974 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %3975 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %3976 = insertvalue { ptr, ptr, i64 } poison, ptr %3974, 0
  %3977 = insertvalue { ptr, ptr, i64 } %3976, ptr %3975, 1
  %3978 = insertvalue { ptr, ptr, i64 } %3977, i64 0, 2
  %3979 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %3980 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %3981 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %3982 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %3983 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %3984 = extractvalue { ptr, ptr, i64 } %3978, 0
  %3985 = extractvalue { ptr, ptr, i64 } %3978, 1
  %3986 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %3984, 0
  %3987 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3986, ptr %3985, 1
  %3988 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3987, i64 8, 2
  %3989 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3988, i64 16, 3, 0
  %3990 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3989, i64 32, 4, 0
  %3991 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3990, i64 4, 3, 1
  %3992 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3991, i64 1, 4, 1
  %3993 = call ptr @llvm.stacksave.p0()
  %3994 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3992, ptr %3994, align 8
  %3995 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3994, 1
  %3996 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %3996, align 8
  %3997 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3996, 1
  %3998 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3995, ptr %3998, align 8
  %3999 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3997, ptr %3999, align 8
  call void @memrefCopy(i64 4, ptr %3998, ptr %3999)
  call void @llvm.stackrestore.p0(ptr %3993)
  %4000 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 2, i32 1017, i32 117)
  %4001 = call ptr @llvm.stacksave.p0()
  %4002 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3529, ptr %4002, align 8
  %4003 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4002, 1
  %4004 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %4004, align 8
  %4005 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4004, 1
  %4006 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4003, ptr %4006, align 8
  %4007 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4005, ptr %4007, align 8
  call void @memrefCopy(i64 4, ptr %4006, ptr %4007)
  call void @llvm.stackrestore.p0(ptr %4001)
  %4008 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %4009 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %4010 = insertvalue { ptr, ptr, i64 } poison, ptr %4008, 0
  %4011 = insertvalue { ptr, ptr, i64 } %4010, ptr %4009, 1
  %4012 = insertvalue { ptr, ptr, i64 } %4011, i64 0, 2
  %4013 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %4014 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %4015 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %4016 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %4017 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %4018 = extractvalue { ptr, ptr, i64 } %4012, 0
  %4019 = extractvalue { ptr, ptr, i64 } %4012, 1
  %4020 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %4018, 0
  %4021 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4020, ptr %4019, 1
  %4022 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4021, i64 520, 2
  %4023 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4022, i64 16, 3, 0
  %4024 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4023, i64 32, 4, 0
  %4025 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4024, i64 4, 3, 1
  %4026 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4025, i64 1, 4, 1
  %4027 = call ptr @llvm.stacksave.p0()
  %4028 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4026, ptr %4028, align 8
  %4029 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4028, 1
  %4030 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %4030, align 8
  %4031 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4030, 1
  %4032 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4029, ptr %4032, align 8
  %4033 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4031, ptr %4033, align 8
  call void @memrefCopy(i64 4, ptr %4032, ptr %4033)
  call void @llvm.stackrestore.p0(ptr %4027)
  %4034 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 2, i32 1086, i32 117)
  %4035 = call ptr @llvm.stacksave.p0()
  %4036 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3582, ptr %4036, align 8
  %4037 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4036, 1
  %4038 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %4038, align 8
  %4039 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4038, 1
  %4040 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4037, ptr %4040, align 8
  %4041 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4039, ptr %4041, align 8
  call void @memrefCopy(i64 4, ptr %4040, ptr %4041)
  call void @llvm.stackrestore.p0(ptr %4035)
  %4042 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %4043 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %4044 = insertvalue { ptr, ptr, i64 } poison, ptr %4042, 0
  %4045 = insertvalue { ptr, ptr, i64 } %4044, ptr %4043, 1
  %4046 = insertvalue { ptr, ptr, i64 } %4045, i64 0, 2
  %4047 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %4048 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %4049 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %4050 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %4051 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %4052 = extractvalue { ptr, ptr, i64 } %4046, 0
  %4053 = extractvalue { ptr, ptr, i64 } %4046, 1
  %4054 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %4052, 0
  %4055 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4054, ptr %4053, 1
  %4056 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4055, i64 1032, 2
  %4057 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4056, i64 16, 3, 0
  %4058 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4057, i64 32, 4, 0
  %4059 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4058, i64 4, 3, 1
  %4060 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4059, i64 1, 4, 1
  %4061 = call ptr @llvm.stacksave.p0()
  %4062 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4060, ptr %4062, align 8
  %4063 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4062, 1
  %4064 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %4064, align 8
  %4065 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4064, 1
  %4066 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4063, ptr %4066, align 8
  %4067 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4065, ptr %4067, align 8
  call void @memrefCopy(i64 4, ptr %4066, ptr %4067)
  call void @llvm.stackrestore.p0(ptr %4061)
  %4068 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 2, i32 1155, i32 117)
  %4069 = call ptr @llvm.stacksave.p0()
  %4070 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3635, ptr %4070, align 8
  %4071 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4070, 1
  %4072 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %4072, align 8
  %4073 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4072, 1
  %4074 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4071, ptr %4074, align 8
  %4075 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4073, ptr %4075, align 8
  call void @memrefCopy(i64 4, ptr %4074, ptr %4075)
  call void @llvm.stackrestore.p0(ptr %4069)
  %4076 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %4077 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %4078 = insertvalue { ptr, ptr, i64 } poison, ptr %4076, 0
  %4079 = insertvalue { ptr, ptr, i64 } %4078, ptr %4077, 1
  %4080 = insertvalue { ptr, ptr, i64 } %4079, i64 0, 2
  %4081 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %4082 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %4083 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %4084 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %4085 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %4086 = extractvalue { ptr, ptr, i64 } %4080, 0
  %4087 = extractvalue { ptr, ptr, i64 } %4080, 1
  %4088 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %4086, 0
  %4089 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4088, ptr %4087, 1
  %4090 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4089, i64 1544, 2
  %4091 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4090, i64 16, 3, 0
  %4092 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4091, i64 32, 4, 0
  %4093 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4092, i64 4, 3, 1
  %4094 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4093, i64 1, 4, 1
  %4095 = call ptr @llvm.stacksave.p0()
  %4096 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4094, ptr %4096, align 8
  %4097 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4096, 1
  %4098 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %4098, align 8
  %4099 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4098, 1
  %4100 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4097, ptr %4100, align 8
  %4101 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4099, ptr %4101, align 8
  call void @memrefCopy(i64 4, ptr %4100, ptr %4101)
  call void @llvm.stackrestore.p0(ptr %4095)
  %4102 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 2, i32 1224, i32 117)
  %4103 = call ptr @llvm.stacksave.p0()
  %4104 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3688, ptr %4104, align 8
  %4105 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4104, 1
  %4106 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2207, ptr %4106, align 8
  %4107 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4106, 1
  %4108 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4105, ptr %4108, align 8
  %4109 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4107, ptr %4109, align 8
  call void @memrefCopy(i64 4, ptr %4108, ptr %4109)
  call void @llvm.stackrestore.p0(ptr %4103)
  %4110 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %4111 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %4112 = insertvalue { ptr, ptr, i64 } poison, ptr %4110, 0
  %4113 = insertvalue { ptr, ptr, i64 } %4112, ptr %4111, 1
  %4114 = insertvalue { ptr, ptr, i64 } %4113, i64 0, 2
  %4115 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %4116 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %4117 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %4118 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %4119 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %4120 = extractvalue { ptr, ptr, i64 } %4114, 0
  %4121 = extractvalue { ptr, ptr, i64 } %4114, 1
  %4122 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %4120, 0
  %4123 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4122, ptr %4121, 1
  %4124 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4123, i64 2056, 2
  %4125 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4124, i64 1, 3, 0
  %4126 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4125, i64 32, 4, 0
  %4127 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4126, i64 4, 3, 1
  %4128 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4127, i64 1, 4, 1
  %4129 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4128, 3, 0
  %4130 = mul i64 1, %4129
  %4131 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4128, 3, 1
  %4132 = mul i64 %4130, %4131
  %4133 = mul i64 %4132, 4
  %4134 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4128, 1
  %4135 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4128, 2
  %4136 = getelementptr float, ptr %4134, i64 %4135
  %4137 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2245, 1
  %4138 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2245, 2
  %4139 = getelementptr float, ptr %4137, i64 %4138
  call void @llvm.memcpy.p0.p0.i64(ptr %4139, ptr %4136, i64 %4133, i1 false)
  %4140 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 1, i32 4, ptr %2217, ptr %2259, ptr %1943, i32 2, i32 1293, i32 102)
  %4141 = call ptr @llvm.stacksave.p0()
  %4142 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1933, ptr %4142, align 8
  %4143 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4142, 1
  %4144 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3959, ptr %4144, align 8
  %4145 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4144, 1
  %4146 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4143, ptr %4146, align 8
  %4147 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4145, ptr %4147, align 8
  call void @memrefCopy(i64 4, ptr %4146, ptr %4147)
  call void @llvm.stackrestore.p0(ptr %4141)
  %4148 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %4149 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %4150 = insertvalue { ptr, ptr, i64 } poison, ptr %4148, 0
  %4151 = insertvalue { ptr, ptr, i64 } %4150, ptr %4149, 1
  %4152 = insertvalue { ptr, ptr, i64 } %4151, i64 0, 2
  %4153 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %4154 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %4155 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %4156 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %4157 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %4158 = extractvalue { ptr, ptr, i64 } %4152, 0
  %4159 = extractvalue { ptr, ptr, i64 } %4152, 1
  %4160 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %4158, 0
  %4161 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4160, ptr %4159, 1
  %4162 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4161, i64 780, 2
  %4163 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4162, i64 4, 3, 0
  %4164 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4163, i64 32, 4, 0
  %4165 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4164, i64 4, 3, 1
  %4166 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4165, i64 1, 4, 1
  %4167 = call ptr @llvm.stacksave.p0()
  %4168 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4166, ptr %4168, align 8
  %4169 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4168, 1
  %4170 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1933, ptr %4170, align 8
  %4171 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4170, 1
  %4172 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4169, ptr %4172, align 8
  %4173 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4171, ptr %4173, align 8
  call void @memrefCopy(i64 4, ptr %4172, ptr %4173)
  call void @llvm.stackrestore.p0(ptr %4167)
  %4174 = call ptr @llvm.stacksave.p0()
  %4175 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3476, ptr %4175, align 8
  %4176 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4175, 1
  %4177 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %4177, align 8
  %4178 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4177, 1
  %4179 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4176, ptr %4179, align 8
  %4180 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4178, ptr %4180, align 8
  call void @memrefCopy(i64 4, ptr %4179, ptr %4180)
  call void @llvm.stackrestore.p0(ptr %4174)
  %4181 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %4182 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %4183 = insertvalue { ptr, ptr, i64 } poison, ptr %4181, 0
  %4184 = insertvalue { ptr, ptr, i64 } %4183, ptr %4182, 1
  %4185 = insertvalue { ptr, ptr, i64 } %4184, i64 0, 2
  %4186 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %4187 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %4188 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %4189 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %4190 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %4191 = extractvalue { ptr, ptr, i64 } %4185, 0
  %4192 = extractvalue { ptr, ptr, i64 } %4185, 1
  %4193 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %4191, 0
  %4194 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4193, ptr %4192, 1
  %4195 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4194, i64 12, 2
  %4196 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4195, i64 16, 3, 0
  %4197 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4196, i64 32, 4, 0
  %4198 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4197, i64 4, 3, 1
  %4199 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4198, i64 1, 4, 1
  %4200 = call ptr @llvm.stacksave.p0()
  %4201 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4199, ptr %4201, align 8
  %4202 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4201, 1
  %4203 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %4203, align 8
  %4204 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4203, 1
  %4205 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4202, ptr %4205, align 8
  %4206 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4204, ptr %4206, align 8
  call void @memrefCopy(i64 4, ptr %4205, ptr %4206)
  call void @llvm.stackrestore.p0(ptr %4200)
  %4207 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 3, i32 1017, i32 117)
  %4208 = call ptr @llvm.stacksave.p0()
  %4209 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3529, ptr %4209, align 8
  %4210 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4209, 1
  %4211 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %4211, align 8
  %4212 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4211, 1
  %4213 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4210, ptr %4213, align 8
  %4214 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4212, ptr %4214, align 8
  call void @memrefCopy(i64 4, ptr %4213, ptr %4214)
  call void @llvm.stackrestore.p0(ptr %4208)
  %4215 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %4216 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %4217 = insertvalue { ptr, ptr, i64 } poison, ptr %4215, 0
  %4218 = insertvalue { ptr, ptr, i64 } %4217, ptr %4216, 1
  %4219 = insertvalue { ptr, ptr, i64 } %4218, i64 0, 2
  %4220 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %4221 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %4222 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %4223 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %4224 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %4225 = extractvalue { ptr, ptr, i64 } %4219, 0
  %4226 = extractvalue { ptr, ptr, i64 } %4219, 1
  %4227 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %4225, 0
  %4228 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4227, ptr %4226, 1
  %4229 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4228, i64 524, 2
  %4230 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4229, i64 16, 3, 0
  %4231 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4230, i64 32, 4, 0
  %4232 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4231, i64 4, 3, 1
  %4233 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4232, i64 1, 4, 1
  %4234 = call ptr @llvm.stacksave.p0()
  %4235 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4233, ptr %4235, align 8
  %4236 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4235, 1
  %4237 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %4237, align 8
  %4238 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4237, 1
  %4239 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4236, ptr %4239, align 8
  %4240 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4238, ptr %4240, align 8
  call void @memrefCopy(i64 4, ptr %4239, ptr %4240)
  call void @llvm.stackrestore.p0(ptr %4234)
  %4241 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 3, i32 1086, i32 117)
  %4242 = call ptr @llvm.stacksave.p0()
  %4243 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3582, ptr %4243, align 8
  %4244 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4243, 1
  %4245 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %4245, align 8
  %4246 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4245, 1
  %4247 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4244, ptr %4247, align 8
  %4248 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4246, ptr %4248, align 8
  call void @memrefCopy(i64 4, ptr %4247, ptr %4248)
  call void @llvm.stackrestore.p0(ptr %4242)
  %4249 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %4250 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %4251 = insertvalue { ptr, ptr, i64 } poison, ptr %4249, 0
  %4252 = insertvalue { ptr, ptr, i64 } %4251, ptr %4250, 1
  %4253 = insertvalue { ptr, ptr, i64 } %4252, i64 0, 2
  %4254 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %4255 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %4256 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %4257 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %4258 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %4259 = extractvalue { ptr, ptr, i64 } %4253, 0
  %4260 = extractvalue { ptr, ptr, i64 } %4253, 1
  %4261 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %4259, 0
  %4262 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4261, ptr %4260, 1
  %4263 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4262, i64 1036, 2
  %4264 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4263, i64 16, 3, 0
  %4265 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4264, i64 32, 4, 0
  %4266 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4265, i64 4, 3, 1
  %4267 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4266, i64 1, 4, 1
  %4268 = call ptr @llvm.stacksave.p0()
  %4269 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4267, ptr %4269, align 8
  %4270 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4269, 1
  %4271 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %4271, align 8
  %4272 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4271, 1
  %4273 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4270, ptr %4273, align 8
  %4274 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4272, ptr %4274, align 8
  call void @memrefCopy(i64 4, ptr %4273, ptr %4274)
  call void @llvm.stackrestore.p0(ptr %4268)
  %4275 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 3, i32 1155, i32 117)
  %4276 = call ptr @llvm.stacksave.p0()
  %4277 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3635, ptr %4277, align 8
  %4278 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4277, 1
  %4279 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %4279, align 8
  %4280 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4279, 1
  %4281 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4278, ptr %4281, align 8
  %4282 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4280, ptr %4282, align 8
  call void @memrefCopy(i64 4, ptr %4281, ptr %4282)
  call void @llvm.stackrestore.p0(ptr %4276)
  %4283 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %4284 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %4285 = insertvalue { ptr, ptr, i64 } poison, ptr %4283, 0
  %4286 = insertvalue { ptr, ptr, i64 } %4285, ptr %4284, 1
  %4287 = insertvalue { ptr, ptr, i64 } %4286, i64 0, 2
  %4288 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %4289 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %4290 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %4291 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %4292 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %4293 = extractvalue { ptr, ptr, i64 } %4287, 0
  %4294 = extractvalue { ptr, ptr, i64 } %4287, 1
  %4295 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %4293, 0
  %4296 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4295, ptr %4294, 1
  %4297 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4296, i64 1548, 2
  %4298 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4297, i64 16, 3, 0
  %4299 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4298, i64 32, 4, 0
  %4300 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4299, i64 4, 3, 1
  %4301 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4300, i64 1, 4, 1
  %4302 = call ptr @llvm.stacksave.p0()
  %4303 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4301, ptr %4303, align 8
  %4304 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4303, 1
  %4305 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %4305, align 8
  %4306 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4305, 1
  %4307 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4304, ptr %4307, align 8
  %4308 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4306, ptr %4308, align 8
  call void @memrefCopy(i64 4, ptr %4307, ptr %4308)
  call void @llvm.stackrestore.p0(ptr %4302)
  %4309 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 3, i32 1224, i32 117)
  %4310 = call ptr @llvm.stacksave.p0()
  %4311 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3688, ptr %4311, align 8
  %4312 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4311, 1
  %4313 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2207, ptr %4313, align 8
  %4314 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4313, 1
  %4315 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4312, ptr %4315, align 8
  %4316 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4314, ptr %4316, align 8
  call void @memrefCopy(i64 4, ptr %4315, ptr %4316)
  call void @llvm.stackrestore.p0(ptr %4310)
  %4317 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %4318 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %4319 = insertvalue { ptr, ptr, i64 } poison, ptr %4317, 0
  %4320 = insertvalue { ptr, ptr, i64 } %4319, ptr %4318, 1
  %4321 = insertvalue { ptr, ptr, i64 } %4320, i64 0, 2
  %4322 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %4323 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %4324 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %4325 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %4326 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %4327 = extractvalue { ptr, ptr, i64 } %4321, 0
  %4328 = extractvalue { ptr, ptr, i64 } %4321, 1
  %4329 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %4327, 0
  %4330 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4329, ptr %4328, 1
  %4331 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4330, i64 2060, 2
  %4332 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4331, i64 1, 3, 0
  %4333 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4332, i64 32, 4, 0
  %4334 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4333, i64 4, 3, 1
  %4335 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4334, i64 1, 4, 1
  %4336 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4335, 3, 0
  %4337 = mul i64 1, %4336
  %4338 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4335, 3, 1
  %4339 = mul i64 %4337, %4338
  %4340 = mul i64 %4339, 4
  %4341 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4335, 1
  %4342 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4335, 2
  %4343 = getelementptr float, ptr %4341, i64 %4342
  %4344 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2245, 1
  %4345 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2245, 2
  %4346 = getelementptr float, ptr %4344, i64 %4345
  call void @llvm.memcpy.p0.p0.i64(ptr %4346, ptr %4343, i64 %4340, i1 false)
  %4347 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 1, i32 4, ptr %2217, ptr %2259, ptr %1943, i32 3, i32 1293, i32 102)
  %4348 = call ptr @llvm.stacksave.p0()
  %4349 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1933, ptr %4349, align 8
  %4350 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4349, 1
  %4351 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4166, ptr %4351, align 8
  %4352 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4351, 1
  %4353 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4350, ptr %4353, align 8
  %4354 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4352, ptr %4354, align 8
  call void @memrefCopy(i64 4, ptr %4353, ptr %4354)
  call void @llvm.stackrestore.p0(ptr %4348)
  %4355 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %4356 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %4357 = insertvalue { ptr, ptr, i64 } poison, ptr %4355, 0
  %4358 = insertvalue { ptr, ptr, i64 } %4357, ptr %4356, 1
  %4359 = insertvalue { ptr, ptr, i64 } %4358, i64 0, 2
  %4360 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %4361 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %4362 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %4363 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %4364 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %4365 = extractvalue { ptr, ptr, i64 } %4359, 0
  %4366 = extractvalue { ptr, ptr, i64 } %4359, 1
  %4367 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %4365, 0
  %4368 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4367, ptr %4366, 1
  %4369 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4368, i64 784, 2
  %4370 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4369, i64 4, 3, 0
  %4371 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4370, i64 32, 4, 0
  %4372 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4371, i64 4, 3, 1
  %4373 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4372, i64 1, 4, 1
  %4374 = call ptr @llvm.stacksave.p0()
  %4375 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4373, ptr %4375, align 8
  %4376 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4375, 1
  %4377 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1933, ptr %4377, align 8
  %4378 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4377, 1
  %4379 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4376, ptr %4379, align 8
  %4380 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4378, ptr %4380, align 8
  call void @memrefCopy(i64 4, ptr %4379, ptr %4380)
  call void @llvm.stackrestore.p0(ptr %4374)
  %4381 = call ptr @llvm.stacksave.p0()
  %4382 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3476, ptr %4382, align 8
  %4383 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4382, 1
  %4384 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %4384, align 8
  %4385 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4384, 1
  %4386 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4383, ptr %4386, align 8
  %4387 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4385, ptr %4387, align 8
  call void @memrefCopy(i64 4, ptr %4386, ptr %4387)
  call void @llvm.stackrestore.p0(ptr %4381)
  %4388 = call ptr @llvm.stacksave.p0()
  %4389 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2000, ptr %4389, align 8
  %4390 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4389, 1
  %4391 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %4391, align 8
  %4392 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4391, 1
  %4393 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4390, ptr %4393, align 8
  %4394 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4392, ptr %4394, align 8
  call void @memrefCopy(i64 4, ptr %4393, ptr %4394)
  call void @llvm.stackrestore.p0(ptr %4388)
  %4395 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 1, i32 1362, i32 117)
  %4396 = call ptr @llvm.stacksave.p0()
  %4397 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3529, ptr %4397, align 8
  %4398 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4397, 1
  %4399 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %4399, align 8
  %4400 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4399, 1
  %4401 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4398, ptr %4401, align 8
  %4402 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4400, ptr %4402, align 8
  call void @memrefCopy(i64 4, ptr %4401, ptr %4402)
  call void @llvm.stackrestore.p0(ptr %4396)
  %4403 = call ptr @llvm.stacksave.p0()
  %4404 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2065, ptr %4404, align 8
  %4405 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4404, 1
  %4406 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %4406, align 8
  %4407 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4406, 1
  %4408 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4405, ptr %4408, align 8
  %4409 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4407, ptr %4409, align 8
  call void @memrefCopy(i64 4, ptr %4408, ptr %4409)
  call void @llvm.stackrestore.p0(ptr %4403)
  %4410 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 1, i32 1431, i32 117)
  %4411 = call ptr @llvm.stacksave.p0()
  %4412 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3582, ptr %4412, align 8
  %4413 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4412, 1
  %4414 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %4414, align 8
  %4415 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4414, 1
  %4416 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4413, ptr %4416, align 8
  %4417 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4415, ptr %4417, align 8
  call void @memrefCopy(i64 4, ptr %4416, ptr %4417)
  call void @llvm.stackrestore.p0(ptr %4411)
  %4418 = call ptr @llvm.stacksave.p0()
  %4419 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2118, ptr %4419, align 8
  %4420 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4419, 1
  %4421 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %4421, align 8
  %4422 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4421, 1
  %4423 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4420, ptr %4423, align 8
  %4424 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4422, ptr %4424, align 8
  call void @memrefCopy(i64 4, ptr %4423, ptr %4424)
  call void @llvm.stackrestore.p0(ptr %4418)
  %4425 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 1, i32 1500, i32 117)
  %4426 = call ptr @llvm.stacksave.p0()
  %4427 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3635, ptr %4427, align 8
  %4428 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4427, 1
  %4429 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %4429, align 8
  %4430 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4429, 1
  %4431 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4428, ptr %4431, align 8
  %4432 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4430, ptr %4432, align 8
  call void @memrefCopy(i64 4, ptr %4431, ptr %4432)
  call void @llvm.stackrestore.p0(ptr %4426)
  %4433 = call ptr @llvm.stacksave.p0()
  %4434 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2171, ptr %4434, align 8
  %4435 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4434, 1
  %4436 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %4436, align 8
  %4437 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4436, 1
  %4438 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4435, ptr %4438, align 8
  %4439 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4437, ptr %4439, align 8
  call void @memrefCopy(i64 4, ptr %4438, ptr %4439)
  call void @llvm.stackrestore.p0(ptr %4433)
  %4440 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 1, i32 1569, i32 117)
  %4441 = call ptr @llvm.stacksave.p0()
  %4442 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3688, ptr %4442, align 8
  %4443 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4442, 1
  %4444 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2207, ptr %4444, align 8
  %4445 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4444, 1
  %4446 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4443, ptr %4446, align 8
  %4447 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4445, ptr %4447, align 8
  call void @memrefCopy(i64 4, ptr %4446, ptr %4447)
  call void @llvm.stackrestore.p0(ptr %4441)
  %4448 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2236, 3, 0
  %4449 = mul i64 1, %4448
  %4450 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2236, 3, 1
  %4451 = mul i64 %4449, %4450
  %4452 = mul i64 %4451, 4
  %4453 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2236, 1
  %4454 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2236, 2
  %4455 = getelementptr float, ptr %4453, i64 %4454
  %4456 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2245, 1
  %4457 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2245, 2
  %4458 = getelementptr float, ptr %4456, i64 %4457
  call void @llvm.memcpy.p0.p0.i64(ptr %4458, ptr %4455, i64 %4452, i1 false)
  %4459 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 1, i32 4, ptr %2217, ptr %2259, ptr %1943, i32 1, i32 1638, i32 102)
  %4460 = call ptr @llvm.stacksave.p0()
  %4461 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1933, ptr %4461, align 8
  %4462 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4461, 1
  %4463 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4373, ptr %4463, align 8
  %4464 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4463, 1
  %4465 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4462, ptr %4465, align 8
  %4466 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4464, ptr %4466, align 8
  call void @memrefCopy(i64 4, ptr %4465, ptr %4466)
  call void @llvm.stackrestore.p0(ptr %4460)
  %4467 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %4468 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %4469 = insertvalue { ptr, ptr, i64 } poison, ptr %4467, 0
  %4470 = insertvalue { ptr, ptr, i64 } %4469, ptr %4468, 1
  %4471 = insertvalue { ptr, ptr, i64 } %4470, i64 0, 2
  %4472 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %4473 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %4474 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %4475 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %4476 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %4477 = extractvalue { ptr, ptr, i64 } %4471, 0
  %4478 = extractvalue { ptr, ptr, i64 } %4471, 1
  %4479 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %4477, 0
  %4480 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4479, ptr %4478, 1
  %4481 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4480, i64 788, 2
  %4482 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4481, i64 4, 3, 0
  %4483 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4482, i64 32, 4, 0
  %4484 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4483, i64 4, 3, 1
  %4485 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4484, i64 1, 4, 1
  %4486 = call ptr @llvm.stacksave.p0()
  %4487 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4485, ptr %4487, align 8
  %4488 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4487, 1
  %4489 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1933, ptr %4489, align 8
  %4490 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4489, 1
  %4491 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4488, ptr %4491, align 8
  %4492 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4490, ptr %4492, align 8
  call void @memrefCopy(i64 4, ptr %4491, ptr %4492)
  call void @llvm.stackrestore.p0(ptr %4486)
  %4493 = call ptr @llvm.stacksave.p0()
  %4494 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3476, ptr %4494, align 8
  %4495 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4494, 1
  %4496 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %4496, align 8
  %4497 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4496, 1
  %4498 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4495, ptr %4498, align 8
  %4499 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4497, ptr %4499, align 8
  call void @memrefCopy(i64 4, ptr %4498, ptr %4499)
  call void @llvm.stackrestore.p0(ptr %4493)
  %4500 = call ptr @llvm.stacksave.p0()
  %4501 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2319, ptr %4501, align 8
  %4502 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4501, 1
  %4503 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %4503, align 8
  %4504 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4503, 1
  %4505 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4502, ptr %4505, align 8
  %4506 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4504, ptr %4506, align 8
  call void @memrefCopy(i64 4, ptr %4505, ptr %4506)
  call void @llvm.stackrestore.p0(ptr %4500)
  %4507 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 2, i32 1362, i32 117)
  %4508 = call ptr @llvm.stacksave.p0()
  %4509 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3529, ptr %4509, align 8
  %4510 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4509, 1
  %4511 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %4511, align 8
  %4512 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4511, 1
  %4513 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4510, ptr %4513, align 8
  %4514 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4512, ptr %4514, align 8
  call void @memrefCopy(i64 4, ptr %4513, ptr %4514)
  call void @llvm.stackrestore.p0(ptr %4508)
  %4515 = call ptr @llvm.stacksave.p0()
  %4516 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2353, ptr %4516, align 8
  %4517 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4516, 1
  %4518 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %4518, align 8
  %4519 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4518, 1
  %4520 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4517, ptr %4520, align 8
  %4521 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4519, ptr %4521, align 8
  call void @memrefCopy(i64 4, ptr %4520, ptr %4521)
  call void @llvm.stackrestore.p0(ptr %4515)
  %4522 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 2, i32 1431, i32 117)
  %4523 = call ptr @llvm.stacksave.p0()
  %4524 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3582, ptr %4524, align 8
  %4525 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4524, 1
  %4526 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %4526, align 8
  %4527 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4526, 1
  %4528 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4525, ptr %4528, align 8
  %4529 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4527, ptr %4529, align 8
  call void @memrefCopy(i64 4, ptr %4528, ptr %4529)
  call void @llvm.stackrestore.p0(ptr %4523)
  %4530 = call ptr @llvm.stacksave.p0()
  %4531 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2387, ptr %4531, align 8
  %4532 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4531, 1
  %4533 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %4533, align 8
  %4534 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4533, 1
  %4535 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4532, ptr %4535, align 8
  %4536 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4534, ptr %4536, align 8
  call void @memrefCopy(i64 4, ptr %4535, ptr %4536)
  call void @llvm.stackrestore.p0(ptr %4530)
  %4537 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 2, i32 1500, i32 117)
  %4538 = call ptr @llvm.stacksave.p0()
  %4539 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3635, ptr %4539, align 8
  %4540 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4539, 1
  %4541 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %4541, align 8
  %4542 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4541, 1
  %4543 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4540, ptr %4543, align 8
  %4544 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4542, ptr %4544, align 8
  call void @memrefCopy(i64 4, ptr %4543, ptr %4544)
  call void @llvm.stackrestore.p0(ptr %4538)
  %4545 = call ptr @llvm.stacksave.p0()
  %4546 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2421, ptr %4546, align 8
  %4547 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4546, 1
  %4548 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %4548, align 8
  %4549 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4548, 1
  %4550 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4547, ptr %4550, align 8
  %4551 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4549, ptr %4551, align 8
  call void @memrefCopy(i64 4, ptr %4550, ptr %4551)
  call void @llvm.stackrestore.p0(ptr %4545)
  %4552 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 2, i32 1569, i32 117)
  %4553 = call ptr @llvm.stacksave.p0()
  %4554 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3688, ptr %4554, align 8
  %4555 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4554, 1
  %4556 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2207, ptr %4556, align 8
  %4557 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4556, 1
  %4558 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4555, ptr %4558, align 8
  %4559 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4557, ptr %4559, align 8
  call void @memrefCopy(i64 4, ptr %4558, ptr %4559)
  call void @llvm.stackrestore.p0(ptr %4553)
  %4560 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2455, 3, 0
  %4561 = mul i64 1, %4560
  %4562 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2455, 3, 1
  %4563 = mul i64 %4561, %4562
  %4564 = mul i64 %4563, 4
  %4565 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2455, 1
  %4566 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2455, 2
  %4567 = getelementptr float, ptr %4565, i64 %4566
  %4568 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2245, 1
  %4569 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2245, 2
  %4570 = getelementptr float, ptr %4568, i64 %4569
  call void @llvm.memcpy.p0.p0.i64(ptr %4570, ptr %4567, i64 %4564, i1 false)
  %4571 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 1, i32 4, ptr %2217, ptr %2259, ptr %1943, i32 2, i32 1638, i32 102)
  %4572 = call ptr @llvm.stacksave.p0()
  %4573 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1933, ptr %4573, align 8
  %4574 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4573, 1
  %4575 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4485, ptr %4575, align 8
  %4576 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4575, 1
  %4577 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4574, ptr %4577, align 8
  %4578 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4576, ptr %4578, align 8
  call void @memrefCopy(i64 4, ptr %4577, ptr %4578)
  call void @llvm.stackrestore.p0(ptr %4572)
  %4579 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %4580 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %4581 = insertvalue { ptr, ptr, i64 } poison, ptr %4579, 0
  %4582 = insertvalue { ptr, ptr, i64 } %4581, ptr %4580, 1
  %4583 = insertvalue { ptr, ptr, i64 } %4582, i64 0, 2
  %4584 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %4585 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %4586 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %4587 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %4588 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %4589 = extractvalue { ptr, ptr, i64 } %4583, 0
  %4590 = extractvalue { ptr, ptr, i64 } %4583, 1
  %4591 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %4589, 0
  %4592 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4591, ptr %4590, 1
  %4593 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4592, i64 792, 2
  %4594 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4593, i64 4, 3, 0
  %4595 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4594, i64 32, 4, 0
  %4596 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4595, i64 4, 3, 1
  %4597 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4596, i64 1, 4, 1
  %4598 = call ptr @llvm.stacksave.p0()
  %4599 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4597, ptr %4599, align 8
  %4600 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4599, 1
  %4601 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1933, ptr %4601, align 8
  %4602 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4601, 1
  %4603 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4600, ptr %4603, align 8
  %4604 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4602, ptr %4604, align 8
  call void @memrefCopy(i64 4, ptr %4603, ptr %4604)
  call void @llvm.stackrestore.p0(ptr %4598)
  %4605 = call ptr @llvm.stacksave.p0()
  %4606 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3476, ptr %4606, align 8
  %4607 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4606, 1
  %4608 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %4608, align 8
  %4609 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4608, 1
  %4610 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4607, ptr %4610, align 8
  %4611 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4609, ptr %4611, align 8
  call void @memrefCopy(i64 4, ptr %4610, ptr %4611)
  call void @llvm.stackrestore.p0(ptr %4605)
  %4612 = call ptr @llvm.stacksave.p0()
  %4613 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2526, ptr %4613, align 8
  %4614 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4613, 1
  %4615 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %4615, align 8
  %4616 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4615, 1
  %4617 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4614, ptr %4617, align 8
  %4618 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4616, ptr %4618, align 8
  call void @memrefCopy(i64 4, ptr %4617, ptr %4618)
  call void @llvm.stackrestore.p0(ptr %4612)
  %4619 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 3, i32 1362, i32 117)
  %4620 = call ptr @llvm.stacksave.p0()
  %4621 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3529, ptr %4621, align 8
  %4622 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4621, 1
  %4623 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %4623, align 8
  %4624 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4623, 1
  %4625 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4622, ptr %4625, align 8
  %4626 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4624, ptr %4626, align 8
  call void @memrefCopy(i64 4, ptr %4625, ptr %4626)
  call void @llvm.stackrestore.p0(ptr %4620)
  %4627 = call ptr @llvm.stacksave.p0()
  %4628 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2560, ptr %4628, align 8
  %4629 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4628, 1
  %4630 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %4630, align 8
  %4631 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4630, 1
  %4632 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4629, ptr %4632, align 8
  %4633 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4631, ptr %4633, align 8
  call void @memrefCopy(i64 4, ptr %4632, ptr %4633)
  call void @llvm.stackrestore.p0(ptr %4627)
  %4634 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 3, i32 1431, i32 117)
  %4635 = call ptr @llvm.stacksave.p0()
  %4636 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3582, ptr %4636, align 8
  %4637 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4636, 1
  %4638 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %4638, align 8
  %4639 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4638, 1
  %4640 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4637, ptr %4640, align 8
  %4641 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4639, ptr %4641, align 8
  call void @memrefCopy(i64 4, ptr %4640, ptr %4641)
  call void @llvm.stackrestore.p0(ptr %4635)
  %4642 = call ptr @llvm.stacksave.p0()
  %4643 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2594, ptr %4643, align 8
  %4644 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4643, 1
  %4645 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %4645, align 8
  %4646 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4645, 1
  %4647 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4644, ptr %4647, align 8
  %4648 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4646, ptr %4648, align 8
  call void @memrefCopy(i64 4, ptr %4647, ptr %4648)
  call void @llvm.stackrestore.p0(ptr %4642)
  %4649 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 3, i32 1500, i32 117)
  %4650 = call ptr @llvm.stacksave.p0()
  %4651 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3635, ptr %4651, align 8
  %4652 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4651, 1
  %4653 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %4653, align 8
  %4654 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4653, 1
  %4655 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4652, ptr %4655, align 8
  %4656 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4654, ptr %4656, align 8
  call void @memrefCopy(i64 4, ptr %4655, ptr %4656)
  call void @llvm.stackrestore.p0(ptr %4650)
  %4657 = call ptr @llvm.stacksave.p0()
  %4658 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2628, ptr %4658, align 8
  %4659 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4658, 1
  %4660 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %4660, align 8
  %4661 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4660, 1
  %4662 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4659, ptr %4662, align 8
  %4663 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4661, ptr %4663, align 8
  call void @memrefCopy(i64 4, ptr %4662, ptr %4663)
  call void @llvm.stackrestore.p0(ptr %4657)
  %4664 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 3, i32 1569, i32 117)
  %4665 = call ptr @llvm.stacksave.p0()
  %4666 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3688, ptr %4666, align 8
  %4667 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4666, 1
  %4668 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2207, ptr %4668, align 8
  %4669 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4668, 1
  %4670 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4667, ptr %4670, align 8
  %4671 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4669, ptr %4671, align 8
  call void @memrefCopy(i64 4, ptr %4670, ptr %4671)
  call void @llvm.stackrestore.p0(ptr %4665)
  %4672 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2662, 3, 0
  %4673 = mul i64 1, %4672
  %4674 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2662, 3, 1
  %4675 = mul i64 %4673, %4674
  %4676 = mul i64 %4675, 4
  %4677 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2662, 1
  %4678 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2662, 2
  %4679 = getelementptr float, ptr %4677, i64 %4678
  %4680 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2245, 1
  %4681 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2245, 2
  %4682 = getelementptr float, ptr %4680, i64 %4681
  call void @llvm.memcpy.p0.p0.i64(ptr %4682, ptr %4679, i64 %4676, i1 false)
  %4683 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 1, i32 4, ptr %2217, ptr %2259, ptr %1943, i32 3, i32 1638, i32 102)
  %4684 = call ptr @llvm.stacksave.p0()
  %4685 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1933, ptr %4685, align 8
  %4686 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4685, 1
  %4687 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4597, ptr %4687, align 8
  %4688 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4687, 1
  %4689 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4686, ptr %4689, align 8
  %4690 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4688, ptr %4690, align 8
  call void @memrefCopy(i64 4, ptr %4689, ptr %4690)
  call void @llvm.stackrestore.p0(ptr %4684)
  %4691 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %4692 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %4693 = insertvalue { ptr, ptr, i64 } poison, ptr %4691, 0
  %4694 = insertvalue { ptr, ptr, i64 } %4693, ptr %4692, 1
  %4695 = insertvalue { ptr, ptr, i64 } %4694, i64 0, 2
  %4696 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %4697 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %4698 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %4699 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %4700 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %4701 = extractvalue { ptr, ptr, i64 } %4695, 0
  %4702 = extractvalue { ptr, ptr, i64 } %4695, 1
  %4703 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %4701, 0
  %4704 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4703, ptr %4702, 1
  %4705 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4704, i64 796, 2
  %4706 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4705, i64 4, 3, 0
  %4707 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4706, i64 32, 4, 0
  %4708 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4707, i64 4, 3, 1
  %4709 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4708, i64 1, 4, 1
  %4710 = call ptr @llvm.stacksave.p0()
  %4711 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4709, ptr %4711, align 8
  %4712 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4711, 1
  %4713 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1933, ptr %4713, align 8
  %4714 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4713, 1
  %4715 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4712, ptr %4715, align 8
  %4716 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4714, ptr %4716, align 8
  call void @memrefCopy(i64 4, ptr %4715, ptr %4716)
  call void @llvm.stackrestore.p0(ptr %4710)
  %4717 = call ptr @llvm.stacksave.p0()
  %4718 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3476, ptr %4718, align 8
  %4719 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4718, 1
  %4720 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %4720, align 8
  %4721 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4720, 1
  %4722 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4719, ptr %4722, align 8
  %4723 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4721, ptr %4723, align 8
  call void @memrefCopy(i64 4, ptr %4722, ptr %4723)
  call void @llvm.stackrestore.p0(ptr %4717)
  %4724 = call ptr @llvm.stacksave.p0()
  %4725 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2733, ptr %4725, align 8
  %4726 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4725, 1
  %4727 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %4727, align 8
  %4728 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4727, 1
  %4729 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4726, ptr %4729, align 8
  %4730 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4728, ptr %4730, align 8
  call void @memrefCopy(i64 4, ptr %4729, ptr %4730)
  call void @llvm.stackrestore.p0(ptr %4724)
  %4731 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 1, i32 1707, i32 117)
  %4732 = call ptr @llvm.stacksave.p0()
  %4733 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3529, ptr %4733, align 8
  %4734 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4733, 1
  %4735 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %4735, align 8
  %4736 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4735, 1
  %4737 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4734, ptr %4737, align 8
  %4738 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4736, ptr %4738, align 8
  call void @memrefCopy(i64 4, ptr %4737, ptr %4738)
  call void @llvm.stackrestore.p0(ptr %4732)
  %4739 = call ptr @llvm.stacksave.p0()
  %4740 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2767, ptr %4740, align 8
  %4741 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4740, 1
  %4742 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %4742, align 8
  %4743 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4742, 1
  %4744 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4741, ptr %4744, align 8
  %4745 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4743, ptr %4745, align 8
  call void @memrefCopy(i64 4, ptr %4744, ptr %4745)
  call void @llvm.stackrestore.p0(ptr %4739)
  %4746 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 1, i32 1776, i32 117)
  %4747 = call ptr @llvm.stacksave.p0()
  %4748 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3582, ptr %4748, align 8
  %4749 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4748, 1
  %4750 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %4750, align 8
  %4751 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4750, 1
  %4752 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4749, ptr %4752, align 8
  %4753 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4751, ptr %4753, align 8
  call void @memrefCopy(i64 4, ptr %4752, ptr %4753)
  call void @llvm.stackrestore.p0(ptr %4747)
  %4754 = call ptr @llvm.stacksave.p0()
  %4755 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2801, ptr %4755, align 8
  %4756 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4755, 1
  %4757 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %4757, align 8
  %4758 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4757, 1
  %4759 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4756, ptr %4759, align 8
  %4760 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4758, ptr %4760, align 8
  call void @memrefCopy(i64 4, ptr %4759, ptr %4760)
  call void @llvm.stackrestore.p0(ptr %4754)
  %4761 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 1, i32 1845, i32 117)
  %4762 = call ptr @llvm.stacksave.p0()
  %4763 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3635, ptr %4763, align 8
  %4764 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4763, 1
  %4765 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %4765, align 8
  %4766 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4765, 1
  %4767 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4764, ptr %4767, align 8
  %4768 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4766, ptr %4768, align 8
  call void @memrefCopy(i64 4, ptr %4767, ptr %4768)
  call void @llvm.stackrestore.p0(ptr %4762)
  %4769 = call ptr @llvm.stacksave.p0()
  %4770 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2835, ptr %4770, align 8
  %4771 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4770, 1
  %4772 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %4772, align 8
  %4773 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4772, 1
  %4774 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4771, ptr %4774, align 8
  %4775 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4773, ptr %4775, align 8
  call void @memrefCopy(i64 4, ptr %4774, ptr %4775)
  call void @llvm.stackrestore.p0(ptr %4769)
  %4776 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 1, i32 1914, i32 117)
  %4777 = call ptr @llvm.stacksave.p0()
  %4778 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3688, ptr %4778, align 8
  %4779 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4778, 1
  %4780 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2207, ptr %4780, align 8
  %4781 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4780, 1
  %4782 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4779, ptr %4782, align 8
  %4783 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4781, ptr %4783, align 8
  call void @memrefCopy(i64 4, ptr %4782, ptr %4783)
  call void @llvm.stackrestore.p0(ptr %4777)
  %4784 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2869, 3, 0
  %4785 = mul i64 1, %4784
  %4786 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2869, 3, 1
  %4787 = mul i64 %4785, %4786
  %4788 = mul i64 %4787, 4
  %4789 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2869, 1
  %4790 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2869, 2
  %4791 = getelementptr float, ptr %4789, i64 %4790
  %4792 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2245, 1
  %4793 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2245, 2
  %4794 = getelementptr float, ptr %4792, i64 %4793
  call void @llvm.memcpy.p0.p0.i64(ptr %4794, ptr %4791, i64 %4788, i1 false)
  %4795 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 1, i32 4, ptr %2217, ptr %2259, ptr %1943, i32 1, i32 1983, i32 102)
  %4796 = call ptr @llvm.stacksave.p0()
  %4797 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1933, ptr %4797, align 8
  %4798 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4797, 1
  %4799 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4709, ptr %4799, align 8
  %4800 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4799, 1
  %4801 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4798, ptr %4801, align 8
  %4802 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4800, ptr %4802, align 8
  call void @memrefCopy(i64 4, ptr %4801, ptr %4802)
  call void @llvm.stackrestore.p0(ptr %4796)
  %4803 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %4804 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %4805 = insertvalue { ptr, ptr, i64 } poison, ptr %4803, 0
  %4806 = insertvalue { ptr, ptr, i64 } %4805, ptr %4804, 1
  %4807 = insertvalue { ptr, ptr, i64 } %4806, i64 0, 2
  %4808 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %4809 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %4810 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %4811 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %4812 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %4813 = extractvalue { ptr, ptr, i64 } %4807, 0
  %4814 = extractvalue { ptr, ptr, i64 } %4807, 1
  %4815 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %4813, 0
  %4816 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4815, ptr %4814, 1
  %4817 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4816, i64 896, 2
  %4818 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4817, i64 4, 3, 0
  %4819 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4818, i64 32, 4, 0
  %4820 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4819, i64 4, 3, 1
  %4821 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4820, i64 1, 4, 1
  %4822 = call ptr @llvm.stacksave.p0()
  %4823 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4821, ptr %4823, align 8
  %4824 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4823, 1
  %4825 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1933, ptr %4825, align 8
  %4826 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4825, 1
  %4827 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4824, ptr %4827, align 8
  %4828 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4826, ptr %4828, align 8
  call void @memrefCopy(i64 4, ptr %4827, ptr %4828)
  call void @llvm.stackrestore.p0(ptr %4822)
  %4829 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %4830 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %4831 = insertvalue { ptr, ptr, i64 } poison, ptr %4829, 0
  %4832 = insertvalue { ptr, ptr, i64 } %4831, ptr %4830, 1
  %4833 = insertvalue { ptr, ptr, i64 } %4832, i64 0, 2
  %4834 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %4835 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %4836 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %4837 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %4838 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %4839 = extractvalue { ptr, ptr, i64 } %4833, 0
  %4840 = extractvalue { ptr, ptr, i64 } %4833, 1
  %4841 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %4839, 0
  %4842 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4841, ptr %4840, 1
  %4843 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4842, i64 1820, 2
  %4844 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4843, i64 4, 3, 0
  %4845 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4844, i64 65, 4, 0
  %4846 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4845, i64 16, 3, 1
  %4847 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4846, i64 1, 4, 1
  %4848 = call ptr @llvm.stacksave.p0()
  %4849 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4847, ptr %4849, align 8
  %4850 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4849, 1
  %4851 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %4851, align 8
  %4852 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4851, 1
  %4853 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4850, ptr %4853, align 8
  %4854 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4852, ptr %4854, align 8
  call void @memrefCopy(i64 4, ptr %4853, ptr %4854)
  call void @llvm.stackrestore.p0(ptr %4848)
  %4855 = call ptr @llvm.stacksave.p0()
  %4856 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3502, ptr %4856, align 8
  %4857 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4856, 1
  %4858 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %4858, align 8
  %4859 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4858, 1
  %4860 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4857, ptr %4860, align 8
  %4861 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4859, ptr %4861, align 8
  call void @memrefCopy(i64 4, ptr %4860, ptr %4861)
  call void @llvm.stackrestore.p0(ptr %4855)
  %4862 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 2, i32 1707, i32 117)
  %4863 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %4864 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %4865 = insertvalue { ptr, ptr, i64 } poison, ptr %4863, 0
  %4866 = insertvalue { ptr, ptr, i64 } %4865, ptr %4864, 1
  %4867 = insertvalue { ptr, ptr, i64 } %4866, i64 0, 2
  %4868 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %4869 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %4870 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %4871 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %4872 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %4873 = extractvalue { ptr, ptr, i64 } %4867, 0
  %4874 = extractvalue { ptr, ptr, i64 } %4867, 1
  %4875 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %4873, 0
  %4876 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4875, ptr %4874, 1
  %4877 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4876, i64 1836, 2
  %4878 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4877, i64 4, 3, 0
  %4879 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4878, i64 65, 4, 0
  %4880 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4879, i64 16, 3, 1
  %4881 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4880, i64 1, 4, 1
  %4882 = call ptr @llvm.stacksave.p0()
  %4883 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4881, ptr %4883, align 8
  %4884 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4883, 1
  %4885 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %4885, align 8
  %4886 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4885, 1
  %4887 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4884, ptr %4887, align 8
  %4888 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4886, ptr %4888, align 8
  call void @memrefCopy(i64 4, ptr %4887, ptr %4888)
  call void @llvm.stackrestore.p0(ptr %4882)
  %4889 = call ptr @llvm.stacksave.p0()
  %4890 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3555, ptr %4890, align 8
  %4891 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4890, 1
  %4892 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %4892, align 8
  %4893 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4892, 1
  %4894 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4891, ptr %4894, align 8
  %4895 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4893, ptr %4895, align 8
  call void @memrefCopy(i64 4, ptr %4894, ptr %4895)
  call void @llvm.stackrestore.p0(ptr %4889)
  %4896 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 2, i32 1776, i32 117)
  %4897 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %4898 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %4899 = insertvalue { ptr, ptr, i64 } poison, ptr %4897, 0
  %4900 = insertvalue { ptr, ptr, i64 } %4899, ptr %4898, 1
  %4901 = insertvalue { ptr, ptr, i64 } %4900, i64 0, 2
  %4902 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %4903 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %4904 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %4905 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %4906 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %4907 = extractvalue { ptr, ptr, i64 } %4901, 0
  %4908 = extractvalue { ptr, ptr, i64 } %4901, 1
  %4909 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %4907, 0
  %4910 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4909, ptr %4908, 1
  %4911 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4910, i64 1852, 2
  %4912 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4911, i64 4, 3, 0
  %4913 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4912, i64 65, 4, 0
  %4914 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4913, i64 16, 3, 1
  %4915 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4914, i64 1, 4, 1
  %4916 = call ptr @llvm.stacksave.p0()
  %4917 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4915, ptr %4917, align 8
  %4918 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4917, 1
  %4919 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %4919, align 8
  %4920 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4919, 1
  %4921 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4918, ptr %4921, align 8
  %4922 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4920, ptr %4922, align 8
  call void @memrefCopy(i64 4, ptr %4921, ptr %4922)
  call void @llvm.stackrestore.p0(ptr %4916)
  %4923 = call ptr @llvm.stacksave.p0()
  %4924 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3608, ptr %4924, align 8
  %4925 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4924, 1
  %4926 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %4926, align 8
  %4927 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4926, 1
  %4928 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4925, ptr %4928, align 8
  %4929 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4927, ptr %4929, align 8
  call void @memrefCopy(i64 4, ptr %4928, ptr %4929)
  call void @llvm.stackrestore.p0(ptr %4923)
  %4930 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 2, i32 1845, i32 117)
  %4931 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %4932 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %4933 = insertvalue { ptr, ptr, i64 } poison, ptr %4931, 0
  %4934 = insertvalue { ptr, ptr, i64 } %4933, ptr %4932, 1
  %4935 = insertvalue { ptr, ptr, i64 } %4934, i64 0, 2
  %4936 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %4937 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %4938 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %4939 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %4940 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %4941 = extractvalue { ptr, ptr, i64 } %4935, 0
  %4942 = extractvalue { ptr, ptr, i64 } %4935, 1
  %4943 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %4941, 0
  %4944 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4943, ptr %4942, 1
  %4945 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4944, i64 1868, 2
  %4946 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4945, i64 4, 3, 0
  %4947 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4946, i64 65, 4, 0
  %4948 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4947, i64 16, 3, 1
  %4949 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4948, i64 1, 4, 1
  %4950 = call ptr @llvm.stacksave.p0()
  %4951 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4949, ptr %4951, align 8
  %4952 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4951, 1
  %4953 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %4953, align 8
  %4954 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4953, 1
  %4955 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4952, ptr %4955, align 8
  %4956 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4954, ptr %4956, align 8
  call void @memrefCopy(i64 4, ptr %4955, ptr %4956)
  call void @llvm.stackrestore.p0(ptr %4950)
  %4957 = call ptr @llvm.stacksave.p0()
  %4958 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3661, ptr %4958, align 8
  %4959 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4958, 1
  %4960 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %4960, align 8
  %4961 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4960, 1
  %4962 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4959, ptr %4962, align 8
  %4963 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4961, ptr %4963, align 8
  call void @memrefCopy(i64 4, ptr %4962, ptr %4963)
  call void @llvm.stackrestore.p0(ptr %4957)
  %4964 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 2, i32 1914, i32 117)
  %4965 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %4966 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %4967 = insertvalue { ptr, ptr, i64 } poison, ptr %4965, 0
  %4968 = insertvalue { ptr, ptr, i64 } %4967, ptr %4966, 1
  %4969 = insertvalue { ptr, ptr, i64 } %4968, i64 0, 2
  %4970 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %4971 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %4972 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %4973 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %4974 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %4975 = extractvalue { ptr, ptr, i64 } %4969, 0
  %4976 = extractvalue { ptr, ptr, i64 } %4969, 1
  %4977 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %4975, 0
  %4978 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4977, ptr %4976, 1
  %4979 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4978, i64 1884, 2
  %4980 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4979, i64 4, 3, 0
  %4981 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4980, i64 65, 4, 0
  %4982 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4981, i64 1, 3, 1
  %4983 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4982, i64 1, 4, 1
  %4984 = call ptr @llvm.stacksave.p0()
  %4985 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4983, ptr %4985, align 8
  %4986 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4985, 1
  %4987 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2207, ptr %4987, align 8
  %4988 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4987, 1
  %4989 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4986, ptr %4989, align 8
  %4990 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4988, ptr %4990, align 8
  call void @memrefCopy(i64 4, ptr %4989, ptr %4990)
  call void @llvm.stackrestore.p0(ptr %4984)
  %4991 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3714, 3, 0
  %4992 = mul i64 1, %4991
  %4993 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3714, 3, 1
  %4994 = mul i64 %4992, %4993
  %4995 = mul i64 %4994, 4
  %4996 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3714, 1
  %4997 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3714, 2
  %4998 = getelementptr float, ptr %4996, i64 %4997
  %4999 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2245, 1
  %5000 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2245, 2
  %5001 = getelementptr float, ptr %4999, i64 %5000
  call void @llvm.memcpy.p0.p0.i64(ptr %5001, ptr %4998, i64 %4995, i1 false)
  %5002 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 1, i32 4, ptr %2217, ptr %2259, ptr %1943, i32 2, i32 1983, i32 102)
  %5003 = call ptr @llvm.stacksave.p0()
  %5004 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1933, ptr %5004, align 8
  %5005 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5004, 1
  %5006 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4821, ptr %5006, align 8
  %5007 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5006, 1
  %5008 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5005, ptr %5008, align 8
  %5009 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5007, ptr %5009, align 8
  call void @memrefCopy(i64 4, ptr %5008, ptr %5009)
  call void @llvm.stackrestore.p0(ptr %5003)
  %5010 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %5011 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %5012 = insertvalue { ptr, ptr, i64 } poison, ptr %5010, 0
  %5013 = insertvalue { ptr, ptr, i64 } %5012, ptr %5011, 1
  %5014 = insertvalue { ptr, ptr, i64 } %5013, i64 0, 2
  %5015 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %5016 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %5017 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %5018 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %5019 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %5020 = extractvalue { ptr, ptr, i64 } %5014, 0
  %5021 = extractvalue { ptr, ptr, i64 } %5014, 1
  %5022 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %5020, 0
  %5023 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5022, ptr %5021, 1
  %5024 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5023, i64 900, 2
  %5025 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5024, i64 4, 3, 0
  %5026 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5025, i64 32, 4, 0
  %5027 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5026, i64 4, 3, 1
  %5028 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5027, i64 1, 4, 1
  %5029 = call ptr @llvm.stacksave.p0()
  %5030 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %5028, ptr %5030, align 8
  %5031 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5030, 1
  %5032 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1933, ptr %5032, align 8
  %5033 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5032, 1
  %5034 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5031, ptr %5034, align 8
  %5035 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5033, ptr %5035, align 8
  call void @memrefCopy(i64 4, ptr %5034, ptr %5035)
  call void @llvm.stackrestore.p0(ptr %5029)
  %5036 = call ptr @llvm.stacksave.p0()
  %5037 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4847, ptr %5037, align 8
  %5038 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5037, 1
  %5039 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %5039, align 8
  %5040 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5039, 1
  %5041 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5038, ptr %5041, align 8
  %5042 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5040, ptr %5042, align 8
  call void @memrefCopy(i64 4, ptr %5041, ptr %5042)
  call void @llvm.stackrestore.p0(ptr %5036)
  %5043 = call ptr @llvm.stacksave.p0()
  %5044 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3785, ptr %5044, align 8
  %5045 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5044, 1
  %5046 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %5046, align 8
  %5047 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5046, 1
  %5048 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5045, ptr %5048, align 8
  %5049 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5047, ptr %5049, align 8
  call void @memrefCopy(i64 4, ptr %5048, ptr %5049)
  call void @llvm.stackrestore.p0(ptr %5043)
  %5050 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 3, i32 1707, i32 117)
  %5051 = call ptr @llvm.stacksave.p0()
  %5052 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4881, ptr %5052, align 8
  %5053 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5052, 1
  %5054 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %5054, align 8
  %5055 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5054, 1
  %5056 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5053, ptr %5056, align 8
  %5057 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5055, ptr %5057, align 8
  call void @memrefCopy(i64 4, ptr %5056, ptr %5057)
  call void @llvm.stackrestore.p0(ptr %5051)
  %5058 = call ptr @llvm.stacksave.p0()
  %5059 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3819, ptr %5059, align 8
  %5060 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5059, 1
  %5061 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %5061, align 8
  %5062 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5061, 1
  %5063 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5060, ptr %5063, align 8
  %5064 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5062, ptr %5064, align 8
  call void @memrefCopy(i64 4, ptr %5063, ptr %5064)
  call void @llvm.stackrestore.p0(ptr %5058)
  %5065 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 3, i32 1776, i32 117)
  %5066 = call ptr @llvm.stacksave.p0()
  %5067 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4915, ptr %5067, align 8
  %5068 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5067, 1
  %5069 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %5069, align 8
  %5070 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5069, 1
  %5071 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5068, ptr %5071, align 8
  %5072 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5070, ptr %5072, align 8
  call void @memrefCopy(i64 4, ptr %5071, ptr %5072)
  call void @llvm.stackrestore.p0(ptr %5066)
  %5073 = call ptr @llvm.stacksave.p0()
  %5074 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3853, ptr %5074, align 8
  %5075 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5074, 1
  %5076 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %5076, align 8
  %5077 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5076, 1
  %5078 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5075, ptr %5078, align 8
  %5079 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5077, ptr %5079, align 8
  call void @memrefCopy(i64 4, ptr %5078, ptr %5079)
  call void @llvm.stackrestore.p0(ptr %5073)
  %5080 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 3, i32 1845, i32 117)
  %5081 = call ptr @llvm.stacksave.p0()
  %5082 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4949, ptr %5082, align 8
  %5083 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5082, 1
  %5084 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %5084, align 8
  %5085 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5084, 1
  %5086 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5083, ptr %5086, align 8
  %5087 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5085, ptr %5087, align 8
  call void @memrefCopy(i64 4, ptr %5086, ptr %5087)
  call void @llvm.stackrestore.p0(ptr %5081)
  %5088 = call ptr @llvm.stacksave.p0()
  %5089 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3887, ptr %5089, align 8
  %5090 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5089, 1
  %5091 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %5091, align 8
  %5092 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5091, 1
  %5093 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5090, ptr %5093, align 8
  %5094 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5092, ptr %5094, align 8
  call void @memrefCopy(i64 4, ptr %5093, ptr %5094)
  call void @llvm.stackrestore.p0(ptr %5088)
  %5095 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 3, i32 1914, i32 117)
  %5096 = call ptr @llvm.stacksave.p0()
  %5097 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4983, ptr %5097, align 8
  %5098 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5097, 1
  %5099 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2207, ptr %5099, align 8
  %5100 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5099, 1
  %5101 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5098, ptr %5101, align 8
  %5102 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5100, ptr %5102, align 8
  call void @memrefCopy(i64 4, ptr %5101, ptr %5102)
  call void @llvm.stackrestore.p0(ptr %5096)
  %5103 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3921, 3, 0
  %5104 = mul i64 1, %5103
  %5105 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3921, 3, 1
  %5106 = mul i64 %5104, %5105
  %5107 = mul i64 %5106, 4
  %5108 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3921, 1
  %5109 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3921, 2
  %5110 = getelementptr float, ptr %5108, i64 %5109
  %5111 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2245, 1
  %5112 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2245, 2
  %5113 = getelementptr float, ptr %5111, i64 %5112
  call void @llvm.memcpy.p0.p0.i64(ptr %5113, ptr %5110, i64 %5107, i1 false)
  %5114 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 1, i32 4, ptr %2217, ptr %2259, ptr %1943, i32 3, i32 1983, i32 102)
  %5115 = call ptr @llvm.stacksave.p0()
  %5116 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1933, ptr %5116, align 8
  %5117 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5116, 1
  %5118 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %5028, ptr %5118, align 8
  %5119 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5118, 1
  %5120 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5117, ptr %5120, align 8
  %5121 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5119, ptr %5121, align 8
  call void @memrefCopy(i64 4, ptr %5120, ptr %5121)
  call void @llvm.stackrestore.p0(ptr %5115)
  %5122 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %5123 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %5124 = insertvalue { ptr, ptr, i64 } poison, ptr %5122, 0
  %5125 = insertvalue { ptr, ptr, i64 } %5124, ptr %5123, 1
  %5126 = insertvalue { ptr, ptr, i64 } %5125, i64 0, 2
  %5127 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %5128 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %5129 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %5130 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %5131 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %5132 = extractvalue { ptr, ptr, i64 } %5126, 0
  %5133 = extractvalue { ptr, ptr, i64 } %5126, 1
  %5134 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %5132, 0
  %5135 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5134, ptr %5133, 1
  %5136 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5135, i64 904, 2
  %5137 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5136, i64 4, 3, 0
  %5138 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5137, i64 32, 4, 0
  %5139 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5138, i64 4, 3, 1
  %5140 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5139, i64 1, 4, 1
  %5141 = call ptr @llvm.stacksave.p0()
  %5142 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %5140, ptr %5142, align 8
  %5143 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5142, 1
  %5144 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1933, ptr %5144, align 8
  %5145 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5144, 1
  %5146 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5143, ptr %5146, align 8
  %5147 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5145, ptr %5147, align 8
  call void @memrefCopy(i64 4, ptr %5146, ptr %5147)
  call void @llvm.stackrestore.p0(ptr %5141)
  %5148 = call ptr @llvm.stacksave.p0()
  %5149 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4847, ptr %5149, align 8
  %5150 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5149, 1
  %5151 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %5151, align 8
  %5152 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5151, 1
  %5153 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5150, ptr %5153, align 8
  %5154 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5152, ptr %5154, align 8
  call void @memrefCopy(i64 4, ptr %5153, ptr %5154)
  call void @llvm.stackrestore.p0(ptr %5148)
  %5155 = call ptr @llvm.stacksave.p0()
  %5156 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3992, ptr %5156, align 8
  %5157 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5156, 1
  %5158 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %5158, align 8
  %5159 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5158, 1
  %5160 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5157, ptr %5160, align 8
  %5161 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5159, ptr %5161, align 8
  call void @memrefCopy(i64 4, ptr %5160, ptr %5161)
  call void @llvm.stackrestore.p0(ptr %5155)
  %5162 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 1, i32 2052, i32 117)
  %5163 = call ptr @llvm.stacksave.p0()
  %5164 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4881, ptr %5164, align 8
  %5165 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5164, 1
  %5166 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %5166, align 8
  %5167 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5166, 1
  %5168 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5165, ptr %5168, align 8
  %5169 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5167, ptr %5169, align 8
  call void @memrefCopy(i64 4, ptr %5168, ptr %5169)
  call void @llvm.stackrestore.p0(ptr %5163)
  %5170 = call ptr @llvm.stacksave.p0()
  %5171 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4026, ptr %5171, align 8
  %5172 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5171, 1
  %5173 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %5173, align 8
  %5174 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5173, 1
  %5175 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5172, ptr %5175, align 8
  %5176 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5174, ptr %5176, align 8
  call void @memrefCopy(i64 4, ptr %5175, ptr %5176)
  call void @llvm.stackrestore.p0(ptr %5170)
  %5177 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 1, i32 2121, i32 117)
  %5178 = call ptr @llvm.stacksave.p0()
  %5179 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4915, ptr %5179, align 8
  %5180 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5179, 1
  %5181 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %5181, align 8
  %5182 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5181, 1
  %5183 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5180, ptr %5183, align 8
  %5184 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5182, ptr %5184, align 8
  call void @memrefCopy(i64 4, ptr %5183, ptr %5184)
  call void @llvm.stackrestore.p0(ptr %5178)
  %5185 = call ptr @llvm.stacksave.p0()
  %5186 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4060, ptr %5186, align 8
  %5187 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5186, 1
  %5188 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %5188, align 8
  %5189 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5188, 1
  %5190 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5187, ptr %5190, align 8
  %5191 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5189, ptr %5191, align 8
  call void @memrefCopy(i64 4, ptr %5190, ptr %5191)
  call void @llvm.stackrestore.p0(ptr %5185)
  %5192 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 1, i32 2190, i32 117)
  %5193 = call ptr @llvm.stacksave.p0()
  %5194 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4949, ptr %5194, align 8
  %5195 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5194, 1
  %5196 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %5196, align 8
  %5197 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5196, 1
  %5198 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5195, ptr %5198, align 8
  %5199 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5197, ptr %5199, align 8
  call void @memrefCopy(i64 4, ptr %5198, ptr %5199)
  call void @llvm.stackrestore.p0(ptr %5193)
  %5200 = call ptr @llvm.stacksave.p0()
  %5201 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4094, ptr %5201, align 8
  %5202 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5201, 1
  %5203 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %5203, align 8
  %5204 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5203, 1
  %5205 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5202, ptr %5205, align 8
  %5206 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5204, ptr %5206, align 8
  call void @memrefCopy(i64 4, ptr %5205, ptr %5206)
  call void @llvm.stackrestore.p0(ptr %5200)
  %5207 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 1, i32 2259, i32 117)
  %5208 = call ptr @llvm.stacksave.p0()
  %5209 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4983, ptr %5209, align 8
  %5210 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5209, 1
  %5211 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2207, ptr %5211, align 8
  %5212 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5211, 1
  %5213 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5210, ptr %5213, align 8
  %5214 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5212, ptr %5214, align 8
  call void @memrefCopy(i64 4, ptr %5213, ptr %5214)
  call void @llvm.stackrestore.p0(ptr %5208)
  %5215 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4128, 3, 0
  %5216 = mul i64 1, %5215
  %5217 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4128, 3, 1
  %5218 = mul i64 %5216, %5217
  %5219 = mul i64 %5218, 4
  %5220 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4128, 1
  %5221 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4128, 2
  %5222 = getelementptr float, ptr %5220, i64 %5221
  %5223 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2245, 1
  %5224 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2245, 2
  %5225 = getelementptr float, ptr %5223, i64 %5224
  call void @llvm.memcpy.p0.p0.i64(ptr %5225, ptr %5222, i64 %5219, i1 false)
  %5226 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 1, i32 4, ptr %2217, ptr %2259, ptr %1943, i32 1, i32 2328, i32 102)
  %5227 = call ptr @llvm.stacksave.p0()
  %5228 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1933, ptr %5228, align 8
  %5229 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5228, 1
  %5230 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %5140, ptr %5230, align 8
  %5231 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5230, 1
  %5232 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5229, ptr %5232, align 8
  %5233 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5231, ptr %5233, align 8
  call void @memrefCopy(i64 4, ptr %5232, ptr %5233)
  call void @llvm.stackrestore.p0(ptr %5227)
  %5234 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %5235 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %5236 = insertvalue { ptr, ptr, i64 } poison, ptr %5234, 0
  %5237 = insertvalue { ptr, ptr, i64 } %5236, ptr %5235, 1
  %5238 = insertvalue { ptr, ptr, i64 } %5237, i64 0, 2
  %5239 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %5240 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %5241 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %5242 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %5243 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %5244 = extractvalue { ptr, ptr, i64 } %5238, 0
  %5245 = extractvalue { ptr, ptr, i64 } %5238, 1
  %5246 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %5244, 0
  %5247 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5246, ptr %5245, 1
  %5248 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5247, i64 908, 2
  %5249 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5248, i64 4, 3, 0
  %5250 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5249, i64 32, 4, 0
  %5251 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5250, i64 4, 3, 1
  %5252 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5251, i64 1, 4, 1
  %5253 = call ptr @llvm.stacksave.p0()
  %5254 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %5252, ptr %5254, align 8
  %5255 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5254, 1
  %5256 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1933, ptr %5256, align 8
  %5257 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5256, 1
  %5258 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5255, ptr %5258, align 8
  %5259 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5257, ptr %5259, align 8
  call void @memrefCopy(i64 4, ptr %5258, ptr %5259)
  call void @llvm.stackrestore.p0(ptr %5253)
  %5260 = call ptr @llvm.stacksave.p0()
  %5261 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4847, ptr %5261, align 8
  %5262 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5261, 1
  %5263 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %5263, align 8
  %5264 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5263, 1
  %5265 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5262, ptr %5265, align 8
  %5266 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5264, ptr %5266, align 8
  call void @memrefCopy(i64 4, ptr %5265, ptr %5266)
  call void @llvm.stackrestore.p0(ptr %5260)
  %5267 = call ptr @llvm.stacksave.p0()
  %5268 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4199, ptr %5268, align 8
  %5269 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5268, 1
  %5270 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %5270, align 8
  %5271 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5270, 1
  %5272 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5269, ptr %5272, align 8
  %5273 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5271, ptr %5273, align 8
  call void @memrefCopy(i64 4, ptr %5272, ptr %5273)
  call void @llvm.stackrestore.p0(ptr %5267)
  %5274 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 2, i32 2052, i32 117)
  %5275 = call ptr @llvm.stacksave.p0()
  %5276 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4881, ptr %5276, align 8
  %5277 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5276, 1
  %5278 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %5278, align 8
  %5279 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5278, 1
  %5280 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5277, ptr %5280, align 8
  %5281 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5279, ptr %5281, align 8
  call void @memrefCopy(i64 4, ptr %5280, ptr %5281)
  call void @llvm.stackrestore.p0(ptr %5275)
  %5282 = call ptr @llvm.stacksave.p0()
  %5283 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4233, ptr %5283, align 8
  %5284 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5283, 1
  %5285 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %5285, align 8
  %5286 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5285, 1
  %5287 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5284, ptr %5287, align 8
  %5288 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5286, ptr %5288, align 8
  call void @memrefCopy(i64 4, ptr %5287, ptr %5288)
  call void @llvm.stackrestore.p0(ptr %5282)
  %5289 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 2, i32 2121, i32 117)
  %5290 = call ptr @llvm.stacksave.p0()
  %5291 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4915, ptr %5291, align 8
  %5292 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5291, 1
  %5293 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %5293, align 8
  %5294 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5293, 1
  %5295 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5292, ptr %5295, align 8
  %5296 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5294, ptr %5296, align 8
  call void @memrefCopy(i64 4, ptr %5295, ptr %5296)
  call void @llvm.stackrestore.p0(ptr %5290)
  %5297 = call ptr @llvm.stacksave.p0()
  %5298 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4267, ptr %5298, align 8
  %5299 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5298, 1
  %5300 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %5300, align 8
  %5301 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5300, 1
  %5302 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5299, ptr %5302, align 8
  %5303 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5301, ptr %5303, align 8
  call void @memrefCopy(i64 4, ptr %5302, ptr %5303)
  call void @llvm.stackrestore.p0(ptr %5297)
  %5304 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 2, i32 2190, i32 117)
  %5305 = call ptr @llvm.stacksave.p0()
  %5306 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4949, ptr %5306, align 8
  %5307 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5306, 1
  %5308 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %5308, align 8
  %5309 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5308, 1
  %5310 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5307, ptr %5310, align 8
  %5311 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5309, ptr %5311, align 8
  call void @memrefCopy(i64 4, ptr %5310, ptr %5311)
  call void @llvm.stackrestore.p0(ptr %5305)
  %5312 = call ptr @llvm.stacksave.p0()
  %5313 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4301, ptr %5313, align 8
  %5314 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5313, 1
  %5315 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %5315, align 8
  %5316 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5315, 1
  %5317 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5314, ptr %5317, align 8
  %5318 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5316, ptr %5318, align 8
  call void @memrefCopy(i64 4, ptr %5317, ptr %5318)
  call void @llvm.stackrestore.p0(ptr %5312)
  %5319 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 2, i32 2259, i32 117)
  %5320 = call ptr @llvm.stacksave.p0()
  %5321 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4983, ptr %5321, align 8
  %5322 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5321, 1
  %5323 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2207, ptr %5323, align 8
  %5324 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5323, 1
  %5325 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5322, ptr %5325, align 8
  %5326 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5324, ptr %5326, align 8
  call void @memrefCopy(i64 4, ptr %5325, ptr %5326)
  call void @llvm.stackrestore.p0(ptr %5320)
  %5327 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4335, 3, 0
  %5328 = mul i64 1, %5327
  %5329 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4335, 3, 1
  %5330 = mul i64 %5328, %5329
  %5331 = mul i64 %5330, 4
  %5332 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4335, 1
  %5333 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4335, 2
  %5334 = getelementptr float, ptr %5332, i64 %5333
  %5335 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2245, 1
  %5336 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2245, 2
  %5337 = getelementptr float, ptr %5335, i64 %5336
  call void @llvm.memcpy.p0.p0.i64(ptr %5337, ptr %5334, i64 %5331, i1 false)
  %5338 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 1, i32 4, ptr %2217, ptr %2259, ptr %1943, i32 2, i32 2328, i32 102)
  %5339 = call ptr @llvm.stacksave.p0()
  %5340 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1933, ptr %5340, align 8
  %5341 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5340, 1
  %5342 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %5252, ptr %5342, align 8
  %5343 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5342, 1
  %5344 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5341, ptr %5344, align 8
  %5345 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5343, ptr %5345, align 8
  call void @memrefCopy(i64 4, ptr %5344, ptr %5345)
  call void @llvm.stackrestore.p0(ptr %5339)
  %5346 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %5347 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %5348 = insertvalue { ptr, ptr, i64 } poison, ptr %5346, 0
  %5349 = insertvalue { ptr, ptr, i64 } %5348, ptr %5347, 1
  %5350 = insertvalue { ptr, ptr, i64 } %5349, i64 0, 2
  %5351 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %5352 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %5353 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %5354 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %5355 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %5356 = extractvalue { ptr, ptr, i64 } %5350, 0
  %5357 = extractvalue { ptr, ptr, i64 } %5350, 1
  %5358 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %5356, 0
  %5359 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5358, ptr %5357, 1
  %5360 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5359, i64 912, 2
  %5361 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5360, i64 4, 3, 0
  %5362 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5361, i64 32, 4, 0
  %5363 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5362, i64 4, 3, 1
  %5364 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5363, i64 1, 4, 1
  %5365 = call ptr @llvm.stacksave.p0()
  %5366 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %5364, ptr %5366, align 8
  %5367 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5366, 1
  %5368 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1933, ptr %5368, align 8
  %5369 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5368, 1
  %5370 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5367, ptr %5370, align 8
  %5371 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5369, ptr %5371, align 8
  call void @memrefCopy(i64 4, ptr %5370, ptr %5371)
  call void @llvm.stackrestore.p0(ptr %5365)
  %5372 = call ptr @llvm.stacksave.p0()
  %5373 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4847, ptr %5373, align 8
  %5374 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5373, 1
  %5375 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %5375, align 8
  %5376 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5375, 1
  %5377 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5374, ptr %5377, align 8
  %5378 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5376, ptr %5378, align 8
  call void @memrefCopy(i64 4, ptr %5377, ptr %5378)
  call void @llvm.stackrestore.p0(ptr %5372)
  %5379 = call ptr @llvm.stacksave.p0()
  %5380 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2000, ptr %5380, align 8
  %5381 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5380, 1
  %5382 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %5382, align 8
  %5383 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5382, 1
  %5384 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5381, ptr %5384, align 8
  %5385 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5383, ptr %5385, align 8
  call void @memrefCopy(i64 4, ptr %5384, ptr %5385)
  call void @llvm.stackrestore.p0(ptr %5379)
  %5386 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 3, i32 2052, i32 117)
  %5387 = call ptr @llvm.stacksave.p0()
  %5388 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4881, ptr %5388, align 8
  %5389 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5388, 1
  %5390 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %5390, align 8
  %5391 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5390, 1
  %5392 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5389, ptr %5392, align 8
  %5393 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5391, ptr %5393, align 8
  call void @memrefCopy(i64 4, ptr %5392, ptr %5393)
  call void @llvm.stackrestore.p0(ptr %5387)
  %5394 = call ptr @llvm.stacksave.p0()
  %5395 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2065, ptr %5395, align 8
  %5396 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5395, 1
  %5397 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %5397, align 8
  %5398 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5397, 1
  %5399 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5396, ptr %5399, align 8
  %5400 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5398, ptr %5400, align 8
  call void @memrefCopy(i64 4, ptr %5399, ptr %5400)
  call void @llvm.stackrestore.p0(ptr %5394)
  %5401 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 3, i32 2121, i32 117)
  %5402 = call ptr @llvm.stacksave.p0()
  %5403 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4915, ptr %5403, align 8
  %5404 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5403, 1
  %5405 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %5405, align 8
  %5406 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5405, 1
  %5407 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5404, ptr %5407, align 8
  %5408 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5406, ptr %5408, align 8
  call void @memrefCopy(i64 4, ptr %5407, ptr %5408)
  call void @llvm.stackrestore.p0(ptr %5402)
  %5409 = call ptr @llvm.stacksave.p0()
  %5410 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2118, ptr %5410, align 8
  %5411 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5410, 1
  %5412 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %5412, align 8
  %5413 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5412, 1
  %5414 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5411, ptr %5414, align 8
  %5415 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5413, ptr %5415, align 8
  call void @memrefCopy(i64 4, ptr %5414, ptr %5415)
  call void @llvm.stackrestore.p0(ptr %5409)
  %5416 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 3, i32 2190, i32 117)
  %5417 = call ptr @llvm.stacksave.p0()
  %5418 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4949, ptr %5418, align 8
  %5419 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5418, 1
  %5420 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %5420, align 8
  %5421 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5420, 1
  %5422 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5419, ptr %5422, align 8
  %5423 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5421, ptr %5423, align 8
  call void @memrefCopy(i64 4, ptr %5422, ptr %5423)
  call void @llvm.stackrestore.p0(ptr %5417)
  %5424 = call ptr @llvm.stacksave.p0()
  %5425 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2171, ptr %5425, align 8
  %5426 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5425, 1
  %5427 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %5427, align 8
  %5428 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5427, 1
  %5429 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5426, ptr %5429, align 8
  %5430 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5428, ptr %5430, align 8
  call void @memrefCopy(i64 4, ptr %5429, ptr %5430)
  call void @llvm.stackrestore.p0(ptr %5424)
  %5431 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 3, i32 2259, i32 117)
  %5432 = call ptr @llvm.stacksave.p0()
  %5433 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4983, ptr %5433, align 8
  %5434 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5433, 1
  %5435 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2207, ptr %5435, align 8
  %5436 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5435, 1
  %5437 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5434, ptr %5437, align 8
  %5438 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5436, ptr %5438, align 8
  call void @memrefCopy(i64 4, ptr %5437, ptr %5438)
  call void @llvm.stackrestore.p0(ptr %5432)
  %5439 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2236, 3, 0
  %5440 = mul i64 1, %5439
  %5441 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2236, 3, 1
  %5442 = mul i64 %5440, %5441
  %5443 = mul i64 %5442, 4
  %5444 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2236, 1
  %5445 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2236, 2
  %5446 = getelementptr float, ptr %5444, i64 %5445
  %5447 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2245, 1
  %5448 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2245, 2
  %5449 = getelementptr float, ptr %5447, i64 %5448
  call void @llvm.memcpy.p0.p0.i64(ptr %5449, ptr %5446, i64 %5443, i1 false)
  %5450 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 1, i32 4, ptr %2217, ptr %2259, ptr %1943, i32 3, i32 2328, i32 102)
  %5451 = call ptr @llvm.stacksave.p0()
  %5452 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1933, ptr %5452, align 8
  %5453 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5452, 1
  %5454 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %5364, ptr %5454, align 8
  %5455 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5454, 1
  %5456 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5453, ptr %5456, align 8
  %5457 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5455, ptr %5457, align 8
  call void @memrefCopy(i64 4, ptr %5456, ptr %5457)
  call void @llvm.stackrestore.p0(ptr %5451)
  %5458 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %5459 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %5460 = insertvalue { ptr, ptr, i64 } poison, ptr %5458, 0
  %5461 = insertvalue { ptr, ptr, i64 } %5460, ptr %5459, 1
  %5462 = insertvalue { ptr, ptr, i64 } %5461, i64 0, 2
  %5463 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %5464 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %5465 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %5466 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %5467 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %5468 = extractvalue { ptr, ptr, i64 } %5462, 0
  %5469 = extractvalue { ptr, ptr, i64 } %5462, 1
  %5470 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %5468, 0
  %5471 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5470, ptr %5469, 1
  %5472 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5471, i64 916, 2
  %5473 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5472, i64 4, 3, 0
  %5474 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5473, i64 32, 4, 0
  %5475 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5474, i64 4, 3, 1
  %5476 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5475, i64 1, 4, 1
  %5477 = call ptr @llvm.stacksave.p0()
  %5478 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %5476, ptr %5478, align 8
  %5479 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5478, 1
  %5480 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1933, ptr %5480, align 8
  %5481 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5480, 1
  %5482 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5479, ptr %5482, align 8
  %5483 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5481, ptr %5483, align 8
  call void @memrefCopy(i64 4, ptr %5482, ptr %5483)
  call void @llvm.stackrestore.p0(ptr %5477)
  %5484 = call ptr @llvm.stacksave.p0()
  %5485 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4847, ptr %5485, align 8
  %5486 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5485, 1
  %5487 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %5487, align 8
  %5488 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5487, 1
  %5489 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5486, ptr %5489, align 8
  %5490 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5488, ptr %5490, align 8
  call void @memrefCopy(i64 4, ptr %5489, ptr %5490)
  call void @llvm.stackrestore.p0(ptr %5484)
  %5491 = call ptr @llvm.stacksave.p0()
  %5492 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2319, ptr %5492, align 8
  %5493 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5492, 1
  %5494 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %5494, align 8
  %5495 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5494, 1
  %5496 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5493, ptr %5496, align 8
  %5497 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5495, ptr %5497, align 8
  call void @memrefCopy(i64 4, ptr %5496, ptr %5497)
  call void @llvm.stackrestore.p0(ptr %5491)
  %5498 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 1, i32 2397, i32 117)
  %5499 = call ptr @llvm.stacksave.p0()
  %5500 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4881, ptr %5500, align 8
  %5501 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5500, 1
  %5502 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %5502, align 8
  %5503 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5502, 1
  %5504 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5501, ptr %5504, align 8
  %5505 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5503, ptr %5505, align 8
  call void @memrefCopy(i64 4, ptr %5504, ptr %5505)
  call void @llvm.stackrestore.p0(ptr %5499)
  %5506 = call ptr @llvm.stacksave.p0()
  %5507 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2353, ptr %5507, align 8
  %5508 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5507, 1
  %5509 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %5509, align 8
  %5510 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5509, 1
  %5511 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5508, ptr %5511, align 8
  %5512 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5510, ptr %5512, align 8
  call void @memrefCopy(i64 4, ptr %5511, ptr %5512)
  call void @llvm.stackrestore.p0(ptr %5506)
  %5513 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 1, i32 2466, i32 117)
  %5514 = call ptr @llvm.stacksave.p0()
  %5515 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4915, ptr %5515, align 8
  %5516 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5515, 1
  %5517 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %5517, align 8
  %5518 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5517, 1
  %5519 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5516, ptr %5519, align 8
  %5520 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5518, ptr %5520, align 8
  call void @memrefCopy(i64 4, ptr %5519, ptr %5520)
  call void @llvm.stackrestore.p0(ptr %5514)
  %5521 = call ptr @llvm.stacksave.p0()
  %5522 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2387, ptr %5522, align 8
  %5523 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5522, 1
  %5524 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %5524, align 8
  %5525 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5524, 1
  %5526 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5523, ptr %5526, align 8
  %5527 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5525, ptr %5527, align 8
  call void @memrefCopy(i64 4, ptr %5526, ptr %5527)
  call void @llvm.stackrestore.p0(ptr %5521)
  %5528 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 1, i32 2535, i32 117)
  %5529 = call ptr @llvm.stacksave.p0()
  %5530 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4949, ptr %5530, align 8
  %5531 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5530, 1
  %5532 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %5532, align 8
  %5533 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5532, 1
  %5534 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5531, ptr %5534, align 8
  %5535 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5533, ptr %5535, align 8
  call void @memrefCopy(i64 4, ptr %5534, ptr %5535)
  call void @llvm.stackrestore.p0(ptr %5529)
  %5536 = call ptr @llvm.stacksave.p0()
  %5537 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2421, ptr %5537, align 8
  %5538 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5537, 1
  %5539 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %5539, align 8
  %5540 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5539, 1
  %5541 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5538, ptr %5541, align 8
  %5542 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5540, ptr %5542, align 8
  call void @memrefCopy(i64 4, ptr %5541, ptr %5542)
  call void @llvm.stackrestore.p0(ptr %5536)
  %5543 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 1, i32 2604, i32 117)
  %5544 = call ptr @llvm.stacksave.p0()
  %5545 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4983, ptr %5545, align 8
  %5546 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5545, 1
  %5547 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2207, ptr %5547, align 8
  %5548 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5547, 1
  %5549 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5546, ptr %5549, align 8
  %5550 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5548, ptr %5550, align 8
  call void @memrefCopy(i64 4, ptr %5549, ptr %5550)
  call void @llvm.stackrestore.p0(ptr %5544)
  %5551 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2455, 3, 0
  %5552 = mul i64 1, %5551
  %5553 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2455, 3, 1
  %5554 = mul i64 %5552, %5553
  %5555 = mul i64 %5554, 4
  %5556 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2455, 1
  %5557 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2455, 2
  %5558 = getelementptr float, ptr %5556, i64 %5557
  %5559 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2245, 1
  %5560 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2245, 2
  %5561 = getelementptr float, ptr %5559, i64 %5560
  call void @llvm.memcpy.p0.p0.i64(ptr %5561, ptr %5558, i64 %5555, i1 false)
  %5562 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 1, i32 4, ptr %2217, ptr %2259, ptr %1943, i32 1, i32 2673, i32 102)
  %5563 = call ptr @llvm.stacksave.p0()
  %5564 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1933, ptr %5564, align 8
  %5565 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5564, 1
  %5566 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %5476, ptr %5566, align 8
  %5567 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5566, 1
  %5568 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5565, ptr %5568, align 8
  %5569 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5567, ptr %5569, align 8
  call void @memrefCopy(i64 4, ptr %5568, ptr %5569)
  call void @llvm.stackrestore.p0(ptr %5563)
  %5570 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %5571 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %5572 = insertvalue { ptr, ptr, i64 } poison, ptr %5570, 0
  %5573 = insertvalue { ptr, ptr, i64 } %5572, ptr %5571, 1
  %5574 = insertvalue { ptr, ptr, i64 } %5573, i64 0, 2
  %5575 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %5576 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %5577 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %5578 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %5579 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %5580 = extractvalue { ptr, ptr, i64 } %5574, 0
  %5581 = extractvalue { ptr, ptr, i64 } %5574, 1
  %5582 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %5580, 0
  %5583 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5582, ptr %5581, 1
  %5584 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5583, i64 920, 2
  %5585 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5584, i64 4, 3, 0
  %5586 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5585, i64 32, 4, 0
  %5587 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5586, i64 4, 3, 1
  %5588 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5587, i64 1, 4, 1
  %5589 = call ptr @llvm.stacksave.p0()
  %5590 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %5588, ptr %5590, align 8
  %5591 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5590, 1
  %5592 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1933, ptr %5592, align 8
  %5593 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5592, 1
  %5594 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5591, ptr %5594, align 8
  %5595 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5593, ptr %5595, align 8
  call void @memrefCopy(i64 4, ptr %5594, ptr %5595)
  call void @llvm.stackrestore.p0(ptr %5589)
  %5596 = call ptr @llvm.stacksave.p0()
  %5597 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4847, ptr %5597, align 8
  %5598 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5597, 1
  %5599 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %5599, align 8
  %5600 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5599, 1
  %5601 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5598, ptr %5601, align 8
  %5602 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5600, ptr %5602, align 8
  call void @memrefCopy(i64 4, ptr %5601, ptr %5602)
  call void @llvm.stackrestore.p0(ptr %5596)
  %5603 = call ptr @llvm.stacksave.p0()
  %5604 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2526, ptr %5604, align 8
  %5605 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5604, 1
  %5606 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %5606, align 8
  %5607 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5606, 1
  %5608 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5605, ptr %5608, align 8
  %5609 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5607, ptr %5609, align 8
  call void @memrefCopy(i64 4, ptr %5608, ptr %5609)
  call void @llvm.stackrestore.p0(ptr %5603)
  %5610 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 2, i32 2397, i32 117)
  %5611 = call ptr @llvm.stacksave.p0()
  %5612 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4881, ptr %5612, align 8
  %5613 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5612, 1
  %5614 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %5614, align 8
  %5615 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5614, 1
  %5616 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5613, ptr %5616, align 8
  %5617 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5615, ptr %5617, align 8
  call void @memrefCopy(i64 4, ptr %5616, ptr %5617)
  call void @llvm.stackrestore.p0(ptr %5611)
  %5618 = call ptr @llvm.stacksave.p0()
  %5619 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2560, ptr %5619, align 8
  %5620 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5619, 1
  %5621 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %5621, align 8
  %5622 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5621, 1
  %5623 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5620, ptr %5623, align 8
  %5624 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5622, ptr %5624, align 8
  call void @memrefCopy(i64 4, ptr %5623, ptr %5624)
  call void @llvm.stackrestore.p0(ptr %5618)
  %5625 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 2, i32 2466, i32 117)
  %5626 = call ptr @llvm.stacksave.p0()
  %5627 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4915, ptr %5627, align 8
  %5628 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5627, 1
  %5629 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %5629, align 8
  %5630 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5629, 1
  %5631 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5628, ptr %5631, align 8
  %5632 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5630, ptr %5632, align 8
  call void @memrefCopy(i64 4, ptr %5631, ptr %5632)
  call void @llvm.stackrestore.p0(ptr %5626)
  %5633 = call ptr @llvm.stacksave.p0()
  %5634 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2594, ptr %5634, align 8
  %5635 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5634, 1
  %5636 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %5636, align 8
  %5637 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5636, 1
  %5638 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5635, ptr %5638, align 8
  %5639 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5637, ptr %5639, align 8
  call void @memrefCopy(i64 4, ptr %5638, ptr %5639)
  call void @llvm.stackrestore.p0(ptr %5633)
  %5640 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 2, i32 2535, i32 117)
  %5641 = call ptr @llvm.stacksave.p0()
  %5642 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4949, ptr %5642, align 8
  %5643 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5642, 1
  %5644 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %5644, align 8
  %5645 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5644, 1
  %5646 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5643, ptr %5646, align 8
  %5647 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5645, ptr %5647, align 8
  call void @memrefCopy(i64 4, ptr %5646, ptr %5647)
  call void @llvm.stackrestore.p0(ptr %5641)
  %5648 = call ptr @llvm.stacksave.p0()
  %5649 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2628, ptr %5649, align 8
  %5650 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5649, 1
  %5651 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %5651, align 8
  %5652 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5651, 1
  %5653 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5650, ptr %5653, align 8
  %5654 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5652, ptr %5654, align 8
  call void @memrefCopy(i64 4, ptr %5653, ptr %5654)
  call void @llvm.stackrestore.p0(ptr %5648)
  %5655 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 2, i32 2604, i32 117)
  %5656 = call ptr @llvm.stacksave.p0()
  %5657 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4983, ptr %5657, align 8
  %5658 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5657, 1
  %5659 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2207, ptr %5659, align 8
  %5660 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5659, 1
  %5661 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5658, ptr %5661, align 8
  %5662 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5660, ptr %5662, align 8
  call void @memrefCopy(i64 4, ptr %5661, ptr %5662)
  call void @llvm.stackrestore.p0(ptr %5656)
  %5663 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2662, 3, 0
  %5664 = mul i64 1, %5663
  %5665 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2662, 3, 1
  %5666 = mul i64 %5664, %5665
  %5667 = mul i64 %5666, 4
  %5668 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2662, 1
  %5669 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2662, 2
  %5670 = getelementptr float, ptr %5668, i64 %5669
  %5671 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2245, 1
  %5672 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2245, 2
  %5673 = getelementptr float, ptr %5671, i64 %5672
  call void @llvm.memcpy.p0.p0.i64(ptr %5673, ptr %5670, i64 %5667, i1 false)
  %5674 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 1, i32 4, ptr %2217, ptr %2259, ptr %1943, i32 2, i32 2673, i32 102)
  %5675 = call ptr @llvm.stacksave.p0()
  %5676 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1933, ptr %5676, align 8
  %5677 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5676, 1
  %5678 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %5588, ptr %5678, align 8
  %5679 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5678, 1
  %5680 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5677, ptr %5680, align 8
  %5681 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5679, ptr %5681, align 8
  call void @memrefCopy(i64 4, ptr %5680, ptr %5681)
  call void @llvm.stackrestore.p0(ptr %5675)
  %5682 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %5683 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %5684 = insertvalue { ptr, ptr, i64 } poison, ptr %5682, 0
  %5685 = insertvalue { ptr, ptr, i64 } %5684, ptr %5683, 1
  %5686 = insertvalue { ptr, ptr, i64 } %5685, i64 0, 2
  %5687 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %5688 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %5689 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %5690 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %5691 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %5692 = extractvalue { ptr, ptr, i64 } %5686, 0
  %5693 = extractvalue { ptr, ptr, i64 } %5686, 1
  %5694 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %5692, 0
  %5695 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5694, ptr %5693, 1
  %5696 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5695, i64 924, 2
  %5697 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5696, i64 4, 3, 0
  %5698 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5697, i64 32, 4, 0
  %5699 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5698, i64 4, 3, 1
  %5700 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5699, i64 1, 4, 1
  %5701 = call ptr @llvm.stacksave.p0()
  %5702 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %5700, ptr %5702, align 8
  %5703 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5702, 1
  %5704 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1933, ptr %5704, align 8
  %5705 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5704, 1
  %5706 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5703, ptr %5706, align 8
  %5707 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5705, ptr %5707, align 8
  call void @memrefCopy(i64 4, ptr %5706, ptr %5707)
  call void @llvm.stackrestore.p0(ptr %5701)
  %5708 = call ptr @llvm.stacksave.p0()
  %5709 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4847, ptr %5709, align 8
  %5710 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5709, 1
  %5711 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %5711, align 8
  %5712 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5711, 1
  %5713 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5710, ptr %5713, align 8
  %5714 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5712, ptr %5714, align 8
  call void @memrefCopy(i64 4, ptr %5713, ptr %5714)
  call void @llvm.stackrestore.p0(ptr %5708)
  %5715 = call ptr @llvm.stacksave.p0()
  %5716 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2733, ptr %5716, align 8
  %5717 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5716, 1
  %5718 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %5718, align 8
  %5719 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5718, 1
  %5720 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5717, ptr %5720, align 8
  %5721 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5719, ptr %5721, align 8
  call void @memrefCopy(i64 4, ptr %5720, ptr %5721)
  call void @llvm.stackrestore.p0(ptr %5715)
  %5722 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 3, i32 2397, i32 117)
  %5723 = call ptr @llvm.stacksave.p0()
  %5724 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4881, ptr %5724, align 8
  %5725 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5724, 1
  %5726 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %5726, align 8
  %5727 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5726, 1
  %5728 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5725, ptr %5728, align 8
  %5729 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5727, ptr %5729, align 8
  call void @memrefCopy(i64 4, ptr %5728, ptr %5729)
  call void @llvm.stackrestore.p0(ptr %5723)
  %5730 = call ptr @llvm.stacksave.p0()
  %5731 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2767, ptr %5731, align 8
  %5732 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5731, 1
  %5733 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %5733, align 8
  %5734 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5733, 1
  %5735 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5732, ptr %5735, align 8
  %5736 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5734, ptr %5736, align 8
  call void @memrefCopy(i64 4, ptr %5735, ptr %5736)
  call void @llvm.stackrestore.p0(ptr %5730)
  %5737 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 3, i32 2466, i32 117)
  %5738 = call ptr @llvm.stacksave.p0()
  %5739 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4915, ptr %5739, align 8
  %5740 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5739, 1
  %5741 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %5741, align 8
  %5742 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5741, 1
  %5743 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5740, ptr %5743, align 8
  %5744 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5742, ptr %5744, align 8
  call void @memrefCopy(i64 4, ptr %5743, ptr %5744)
  call void @llvm.stackrestore.p0(ptr %5738)
  %5745 = call ptr @llvm.stacksave.p0()
  %5746 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2801, ptr %5746, align 8
  %5747 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5746, 1
  %5748 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %5748, align 8
  %5749 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5748, 1
  %5750 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5747, ptr %5750, align 8
  %5751 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5749, ptr %5751, align 8
  call void @memrefCopy(i64 4, ptr %5750, ptr %5751)
  call void @llvm.stackrestore.p0(ptr %5745)
  %5752 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 3, i32 2535, i32 117)
  %5753 = call ptr @llvm.stacksave.p0()
  %5754 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4949, ptr %5754, align 8
  %5755 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5754, 1
  %5756 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, ptr %5756, align 8
  %5757 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5756, 1
  %5758 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5755, ptr %5758, align 8
  %5759 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5757, ptr %5759, align 8
  call void @memrefCopy(i64 4, ptr %5758, ptr %5759)
  call void @llvm.stackrestore.p0(ptr %5753)
  %5760 = call ptr @llvm.stacksave.p0()
  %5761 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2835, ptr %5761, align 8
  %5762 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5761, 1
  %5763 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2009, ptr %5763, align 8
  %5764 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5763, 1
  %5765 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5762, ptr %5765, align 8
  %5766 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5764, ptr %5766, align 8
  call void @memrefCopy(i64 4, ptr %5765, ptr %5766)
  call void @llvm.stackrestore.p0(ptr %5760)
  %5767 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1981, ptr %2019, ptr %1943, i32 3, i32 2604, i32 117)
  %5768 = call ptr @llvm.stacksave.p0()
  %5769 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4983, ptr %5769, align 8
  %5770 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5769, 1
  %5771 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2207, ptr %5771, align 8
  %5772 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5771, 1
  %5773 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5770, ptr %5773, align 8
  %5774 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5772, ptr %5774, align 8
  call void @memrefCopy(i64 4, ptr %5773, ptr %5774)
  call void @llvm.stackrestore.p0(ptr %5768)
  %5775 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2869, 3, 0
  %5776 = mul i64 1, %5775
  %5777 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2869, 3, 1
  %5778 = mul i64 %5776, %5777
  %5779 = mul i64 %5778, 4
  %5780 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2869, 1
  %5781 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2869, 2
  %5782 = getelementptr float, ptr %5780, i64 %5781
  %5783 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2245, 1
  %5784 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2245, 2
  %5785 = getelementptr float, ptr %5783, i64 %5784
  call void @llvm.memcpy.p0.p0.i64(ptr %5785, ptr %5782, i64 %5779, i1 false)
  %5786 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 1, i32 4, ptr %2217, ptr %2259, ptr %1943, i32 3, i32 2673, i32 102)
  %5787 = call ptr @llvm.stacksave.p0()
  %5788 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1933, ptr %5788, align 8
  %5789 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5788, 1
  %5790 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %5700, ptr %5790, align 8
  %5791 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5790, 1
  %5792 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5789, ptr %5792, align 8
  %5793 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5791, ptr %5793, align 8
  call void @memrefCopy(i64 4, ptr %5792, ptr %5793)
  call void @llvm.stackrestore.p0(ptr %5787)
  %5794 = call ptr @malloc(i64 4096)
  %5795 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %5794, 0
  %5796 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5795, ptr %5794, 1
  %5797 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5796, i64 0, 2
  %5798 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5797, i64 32, 3, 0
  %5799 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5798, i64 32, 3, 1
  %5800 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5799, i64 32, 4, 0
  %5801 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5800, i64 1, 4, 1
  %5802 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %5803 = mul i64 1, %5802
  %5804 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %5805 = mul i64 %5803, %5804
  %5806 = mul i64 %5805, 4
  %5807 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %5808 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %5809 = getelementptr float, ptr %5807, i64 %5808
  %5810 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5801, 1
  %5811 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5801, 2
  %5812 = getelementptr float, ptr %5810, i64 %5811
  call void @llvm.memcpy.p0.p0.i64(ptr %5812, ptr %5809, i64 %5806, i1 false)
  ret { ptr, ptr, i64, [2 x i64], [2 x i64] } %5801
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
