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
  %115 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %114, i64 35, 4, 0
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
  %153 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %152, i64 12, 4, 0
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
  %192 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %191, i64 35, 4, 0
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
  %216 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %215, i64 192, 2
  %217 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %216, i64 16, 3, 0
  %218 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %217, i64 12, 4, 0
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
  %245 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %244, i64 35, 4, 0
  %246 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %245, i64 3, 3, 1
  %247 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %246, i64 1, 4, 1
  %248 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 0
  %249 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %250 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %248, 0
  %251 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %250, ptr %249, 1
  %252 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %251, i64 0, 2
  %253 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %252, i64 8, 3, 0
  %254 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %253, i64 3, 4, 0
  %255 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %254, i64 3, 3, 1
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
  %281 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %280, i64 384, 2
  %282 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %281, i64 3, 3, 0
  %283 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %282, i64 12, 4, 0
  %284 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %283, i64 8, 3, 1
  %285 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %284, i64 1, 4, 1
  %286 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 0
  %287 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %288 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %286, 0
  %289 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %288, ptr %287, 1
  %290 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %289, i64 0, 2
  %291 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %290, i64 3, 3, 0
  %292 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %291, i64 8, 4, 0
  %293 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %292, i64 8, 3, 1
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
  %305 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 3, i32 8, ptr %266, ptr %304, ptr %98, i32 0, i32 128, i32 112)
  %306 = call ptr @llvm.stacksave.p0()
  %307 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %307, align 8
  %308 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %307, 1
  %309 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %79, ptr %309, align 8
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
  %327 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %326, i64 8, 2
  %328 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %327, i64 4, 3, 0
  %329 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %328, i64 12, 4, 0
  %330 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %329, i64 4, 3, 1
  %331 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %330, i64 1, 4, 1
  %332 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 0
  %333 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 1
  %334 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %332, 0
  %335 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %334, ptr %333, 1
  %336 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %335, i64 0, 2
  %337 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %336, i64 4, 3, 0
  %338 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %337, i64 4, 4, 0
  %339 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %338, i64 4, 3, 1
  %340 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %339, i64 1, 4, 1
  %341 = call ptr @llvm.stacksave.p0()
  %342 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %331, ptr %342, align 8
  %343 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %342, 1
  %344 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %340, ptr %344, align 8
  %345 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %344, 1
  %346 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %343, ptr %346, align 8
  %347 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %345, ptr %347, align 8
  call void @memrefCopy(i64 4, ptr %346, ptr %347)
  call void @llvm.stackrestore.p0(ptr %341)
  %348 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 1
  %349 = ptrtoint ptr %348 to i64
  %350 = inttoptr i64 %349 to ptr
  %351 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %352 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %353 = insertvalue { ptr, ptr, i64 } poison, ptr %351, 0
  %354 = insertvalue { ptr, ptr, i64 } %353, ptr %352, 1
  %355 = insertvalue { ptr, ptr, i64 } %354, i64 0, 2
  %356 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %357 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %358 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %359 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %360 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %361 = extractvalue { ptr, ptr, i64 } %355, 0
  %362 = extractvalue { ptr, ptr, i64 } %355, 1
  %363 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %361, 0
  %364 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %363, ptr %362, 1
  %365 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %364, i64 0, 2
  %366 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %365, i64 4, 3, 0
  %367 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %366, i64 35, 4, 0
  %368 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %367, i64 16, 3, 1
  %369 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %368, i64 1, 4, 1
  %370 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 0
  %371 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %372 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %370, 0
  %373 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %372, ptr %371, 1
  %374 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %373, i64 0, 2
  %375 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %374, i64 4, 3, 0
  %376 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %375, i64 16, 4, 0
  %377 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %376, i64 16, 3, 1
  %378 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %377, i64 1, 4, 1
  %379 = call ptr @llvm.stacksave.p0()
  %380 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %369, ptr %380, align 8
  %381 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %380, 1
  %382 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %378, ptr %382, align 8
  %383 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %382, 1
  %384 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %381, ptr %384, align 8
  %385 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %383, ptr %385, align 8
  call void @memrefCopy(i64 4, ptr %384, ptr %385)
  call void @llvm.stackrestore.p0(ptr %379)
  %386 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %387 = ptrtoint ptr %386 to i64
  %388 = inttoptr i64 %387 to ptr
  %389 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %390 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %391 = insertvalue { ptr, ptr, i64 } poison, ptr %389, 0
  %392 = insertvalue { ptr, ptr, i64 } %391, ptr %390, 1
  %393 = insertvalue { ptr, ptr, i64 } %392, i64 0, 2
  %394 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %395 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %396 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %397 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %398 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %399 = extractvalue { ptr, ptr, i64 } %393, 0
  %400 = extractvalue { ptr, ptr, i64 } %393, 1
  %401 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %399, 0
  %402 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %401, ptr %400, 1
  %403 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %402, i64 8, 2
  %404 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %403, i64 16, 3, 0
  %405 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %404, i64 12, 4, 0
  %406 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %405, i64 4, 3, 1
  %407 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %406, i64 1, 4, 1
  %408 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 0
  %409 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %410 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %408, 0
  %411 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %410, ptr %409, 1
  %412 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %411, i64 0, 2
  %413 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %412, i64 16, 3, 0
  %414 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %413, i64 4, 4, 0
  %415 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %414, i64 4, 3, 1
  %416 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %415, i64 1, 4, 1
  %417 = call ptr @llvm.stacksave.p0()
  %418 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %407, ptr %418, align 8
  %419 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %418, 1
  %420 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %416, ptr %420, align 8
  %421 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %420, 1
  %422 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %419, ptr %422, align 8
  %423 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %421, ptr %423, align 8
  call void @memrefCopy(i64 4, ptr %422, ptr %423)
  call void @llvm.stackrestore.p0(ptr %417)
  %424 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %425 = ptrtoint ptr %424 to i64
  %426 = inttoptr i64 %425 to ptr
  %427 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %388, ptr %426, ptr %350, i32 1, i32 0, i32 117)
  %428 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %429 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %430 = insertvalue { ptr, ptr, i64 } poison, ptr %428, 0
  %431 = insertvalue { ptr, ptr, i64 } %430, ptr %429, 1
  %432 = insertvalue { ptr, ptr, i64 } %431, i64 0, 2
  %433 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %434 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %435 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %436 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %437 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %438 = extractvalue { ptr, ptr, i64 } %432, 0
  %439 = extractvalue { ptr, ptr, i64 } %432, 1
  %440 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %438, 0
  %441 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %440, ptr %439, 1
  %442 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %441, i64 16, 2
  %443 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %442, i64 4, 3, 0
  %444 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %443, i64 35, 4, 0
  %445 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %444, i64 16, 3, 1
  %446 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %445, i64 1, 4, 1
  %447 = call ptr @llvm.stacksave.p0()
  %448 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %446, ptr %448, align 8
  %449 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %448, 1
  %450 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %378, ptr %450, align 8
  %451 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %450, 1
  %452 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %449, ptr %452, align 8
  %453 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %451, ptr %453, align 8
  call void @memrefCopy(i64 4, ptr %452, ptr %453)
  call void @llvm.stackrestore.p0(ptr %447)
  %454 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %455 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %456 = insertvalue { ptr, ptr, i64 } poison, ptr %454, 0
  %457 = insertvalue { ptr, ptr, i64 } %456, ptr %455, 1
  %458 = insertvalue { ptr, ptr, i64 } %457, i64 0, 2
  %459 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %460 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %461 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %462 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %463 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %464 = extractvalue { ptr, ptr, i64 } %458, 0
  %465 = extractvalue { ptr, ptr, i64 } %458, 1
  %466 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %464, 0
  %467 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %466, ptr %465, 1
  %468 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %467, i64 200, 2
  %469 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %468, i64 16, 3, 0
  %470 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %469, i64 12, 4, 0
  %471 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %470, i64 4, 3, 1
  %472 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %471, i64 1, 4, 1
  %473 = call ptr @llvm.stacksave.p0()
  %474 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %472, ptr %474, align 8
  %475 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %474, 1
  %476 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %416, ptr %476, align 8
  %477 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %476, 1
  %478 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %475, ptr %478, align 8
  %479 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %477, ptr %479, align 8
  call void @memrefCopy(i64 4, ptr %478, ptr %479)
  call void @llvm.stackrestore.p0(ptr %473)
  %480 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %388, ptr %426, ptr %350, i32 1, i32 42, i32 117)
  %481 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %482 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %483 = insertvalue { ptr, ptr, i64 } poison, ptr %481, 0
  %484 = insertvalue { ptr, ptr, i64 } %483, ptr %482, 1
  %485 = insertvalue { ptr, ptr, i64 } %484, i64 0, 2
  %486 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %487 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %488 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %489 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %490 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %491 = extractvalue { ptr, ptr, i64 } %485, 0
  %492 = extractvalue { ptr, ptr, i64 } %485, 1
  %493 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %491, 0
  %494 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %493, ptr %492, 1
  %495 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %494, i64 32, 2
  %496 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %495, i64 4, 3, 0
  %497 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %496, i64 35, 4, 0
  %498 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %497, i64 3, 3, 1
  %499 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %498, i64 1, 4, 1
  %500 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 0
  %501 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %502 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %500, 0
  %503 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %502, ptr %501, 1
  %504 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %503, i64 0, 2
  %505 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %504, i64 4, 3, 0
  %506 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %505, i64 3, 4, 0
  %507 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %506, i64 3, 3, 1
  %508 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %507, i64 1, 4, 1
  %509 = call ptr @llvm.stacksave.p0()
  %510 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %499, ptr %510, align 8
  %511 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %510, 1
  %512 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %508, ptr %512, align 8
  %513 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %512, 1
  %514 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %511, ptr %514, align 8
  %515 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %513, ptr %515, align 8
  call void @memrefCopy(i64 4, ptr %514, ptr %515)
  call void @llvm.stackrestore.p0(ptr %509)
  %516 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %517 = ptrtoint ptr %516 to i64
  %518 = inttoptr i64 %517 to ptr
  %519 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %520 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %521 = insertvalue { ptr, ptr, i64 } poison, ptr %519, 0
  %522 = insertvalue { ptr, ptr, i64 } %521, ptr %520, 1
  %523 = insertvalue { ptr, ptr, i64 } %522, i64 0, 2
  %524 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %525 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %526 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %527 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %528 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %529 = extractvalue { ptr, ptr, i64 } %523, 0
  %530 = extractvalue { ptr, ptr, i64 } %523, 1
  %531 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %529, 0
  %532 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %531, ptr %530, 1
  %533 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %532, i64 392, 2
  %534 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %533, i64 3, 3, 0
  %535 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %534, i64 12, 4, 0
  %536 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %535, i64 4, 3, 1
  %537 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %536, i64 1, 4, 1
  %538 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 0
  %539 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %540 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %538, 0
  %541 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %540, ptr %539, 1
  %542 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %541, i64 0, 2
  %543 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %542, i64 3, 3, 0
  %544 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %543, i64 4, 4, 0
  %545 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %544, i64 4, 3, 1
  %546 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %545, i64 1, 4, 1
  %547 = call ptr @llvm.stacksave.p0()
  %548 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %537, ptr %548, align 8
  %549 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %548, 1
  %550 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %546, ptr %550, align 8
  %551 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %550, 1
  %552 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %549, ptr %552, align 8
  %553 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %551, ptr %553, align 8
  call void @memrefCopy(i64 4, ptr %552, ptr %553)
  call void @llvm.stackrestore.p0(ptr %547)
  %554 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %555 = ptrtoint ptr %554 to i64
  %556 = inttoptr i64 %555 to ptr
  %557 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 3, i32 4, ptr %518, ptr %556, ptr %350, i32 1, i32 120, i32 104)
  %558 = call ptr @llvm.stacksave.p0()
  %559 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %340, ptr %559, align 8
  %560 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %559, 1
  %561 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %331, ptr %561, align 8
  %562 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %561, 1
  %563 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %560, ptr %563, align 8
  %564 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %562, ptr %564, align 8
  call void @memrefCopy(i64 4, ptr %563, ptr %564)
  call void @llvm.stackrestore.p0(ptr %558)
  %565 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %566 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %567 = insertvalue { ptr, ptr, i64 } poison, ptr %565, 0
  %568 = insertvalue { ptr, ptr, i64 } %567, ptr %566, 1
  %569 = insertvalue { ptr, ptr, i64 } %568, i64 0, 2
  %570 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %571 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %572 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %573 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %574 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %575 = extractvalue { ptr, ptr, i64 } %569, 0
  %576 = extractvalue { ptr, ptr, i64 } %569, 1
  %577 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %575, 0
  %578 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %577, ptr %576, 1
  %579 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %578, i64 56, 2
  %580 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %579, i64 4, 3, 0
  %581 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %580, i64 12, 4, 0
  %582 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %581, i64 4, 3, 1
  %583 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %582, i64 1, 4, 1
  %584 = call ptr @llvm.stacksave.p0()
  %585 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %583, ptr %585, align 8
  %586 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %585, 1
  %587 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %340, ptr %587, align 8
  %588 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %587, 1
  %589 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %586, ptr %589, align 8
  %590 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %588, ptr %590, align 8
  call void @memrefCopy(i64 4, ptr %589, ptr %590)
  call void @llvm.stackrestore.p0(ptr %584)
  %591 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %592 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %593 = insertvalue { ptr, ptr, i64 } poison, ptr %591, 0
  %594 = insertvalue { ptr, ptr, i64 } %593, ptr %592, 1
  %595 = insertvalue { ptr, ptr, i64 } %594, i64 0, 2
  %596 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %597 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %598 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %599 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %600 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %601 = extractvalue { ptr, ptr, i64 } %595, 0
  %602 = extractvalue { ptr, ptr, i64 } %595, 1
  %603 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %601, 0
  %604 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %603, ptr %602, 1
  %605 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %604, i64 140, 2
  %606 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %605, i64 4, 3, 0
  %607 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %606, i64 35, 4, 0
  %608 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %607, i64 16, 3, 1
  %609 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %608, i64 1, 4, 1
  %610 = call ptr @llvm.stacksave.p0()
  %611 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %609, ptr %611, align 8
  %612 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %611, 1
  %613 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %378, ptr %613, align 8
  %614 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %613, 1
  %615 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %612, ptr %615, align 8
  %616 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %614, ptr %616, align 8
  call void @memrefCopy(i64 4, ptr %615, ptr %616)
  call void @llvm.stackrestore.p0(ptr %610)
  %617 = call ptr @llvm.stacksave.p0()
  %618 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %407, ptr %618, align 8
  %619 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %618, 1
  %620 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %416, ptr %620, align 8
  %621 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %620, 1
  %622 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %619, ptr %622, align 8
  %623 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %621, ptr %623, align 8
  call void @memrefCopy(i64 4, ptr %622, ptr %623)
  call void @llvm.stackrestore.p0(ptr %617)
  %624 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %388, ptr %426, ptr %350, i32 2, i32 0, i32 117)
  %625 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %626 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %627 = insertvalue { ptr, ptr, i64 } poison, ptr %625, 0
  %628 = insertvalue { ptr, ptr, i64 } %627, ptr %626, 1
  %629 = insertvalue { ptr, ptr, i64 } %628, i64 0, 2
  %630 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %631 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %632 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %633 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %634 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %635 = extractvalue { ptr, ptr, i64 } %629, 0
  %636 = extractvalue { ptr, ptr, i64 } %629, 1
  %637 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %635, 0
  %638 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %637, ptr %636, 1
  %639 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %638, i64 156, 2
  %640 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %639, i64 4, 3, 0
  %641 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %640, i64 35, 4, 0
  %642 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %641, i64 16, 3, 1
  %643 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %642, i64 1, 4, 1
  %644 = call ptr @llvm.stacksave.p0()
  %645 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %643, ptr %645, align 8
  %646 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %645, 1
  %647 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %378, ptr %647, align 8
  %648 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %647, 1
  %649 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %646, ptr %649, align 8
  %650 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %648, ptr %650, align 8
  call void @memrefCopy(i64 4, ptr %649, ptr %650)
  call void @llvm.stackrestore.p0(ptr %644)
  %651 = call ptr @llvm.stacksave.p0()
  %652 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %472, ptr %652, align 8
  %653 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %652, 1
  %654 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %416, ptr %654, align 8
  %655 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %654, 1
  %656 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %653, ptr %656, align 8
  %657 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %655, ptr %657, align 8
  call void @memrefCopy(i64 4, ptr %656, ptr %657)
  call void @llvm.stackrestore.p0(ptr %651)
  %658 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %388, ptr %426, ptr %350, i32 2, i32 42, i32 117)
  %659 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %660 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %661 = insertvalue { ptr, ptr, i64 } poison, ptr %659, 0
  %662 = insertvalue { ptr, ptr, i64 } %661, ptr %660, 1
  %663 = insertvalue { ptr, ptr, i64 } %662, i64 0, 2
  %664 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %665 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %666 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %667 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %668 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %669 = extractvalue { ptr, ptr, i64 } %663, 0
  %670 = extractvalue { ptr, ptr, i64 } %663, 1
  %671 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %669, 0
  %672 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %671, ptr %670, 1
  %673 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %672, i64 172, 2
  %674 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %673, i64 4, 3, 0
  %675 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %674, i64 35, 4, 0
  %676 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %675, i64 3, 3, 1
  %677 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %676, i64 1, 4, 1
  %678 = call ptr @llvm.stacksave.p0()
  %679 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %677, ptr %679, align 8
  %680 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %679, 1
  %681 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %508, ptr %681, align 8
  %682 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %681, 1
  %683 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %680, ptr %683, align 8
  %684 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %682, ptr %684, align 8
  call void @memrefCopy(i64 4, ptr %683, ptr %684)
  call void @llvm.stackrestore.p0(ptr %678)
  %685 = call ptr @llvm.stacksave.p0()
  %686 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %537, ptr %686, align 8
  %687 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %686, 1
  %688 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %546, ptr %688, align 8
  %689 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %688, 1
  %690 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %687, ptr %690, align 8
  %691 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %689, ptr %691, align 8
  call void @memrefCopy(i64 4, ptr %690, ptr %691)
  call void @llvm.stackrestore.p0(ptr %685)
  %692 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 3, i32 4, ptr %518, ptr %556, ptr %350, i32 2, i32 120, i32 104)
  %693 = call ptr @llvm.stacksave.p0()
  %694 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %340, ptr %694, align 8
  %695 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %694, 1
  %696 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %583, ptr %696, align 8
  %697 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %696, 1
  %698 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %695, ptr %698, align 8
  %699 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %697, ptr %699, align 8
  call void @memrefCopy(i64 4, ptr %698, ptr %699)
  call void @llvm.stackrestore.p0(ptr %693)
  %700 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %701 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %702 = insertvalue { ptr, ptr, i64 } poison, ptr %700, 0
  %703 = insertvalue { ptr, ptr, i64 } %702, ptr %701, 1
  %704 = insertvalue { ptr, ptr, i64 } %703, i64 0, 2
  %705 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %706 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %707 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %708 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %709 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %710 = extractvalue { ptr, ptr, i64 } %704, 0
  %711 = extractvalue { ptr, ptr, i64 } %704, 1
  %712 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %710, 0
  %713 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %712, ptr %711, 1
  %714 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %713, i64 96, 2
  %715 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %714, i64 4, 3, 0
  %716 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %715, i64 12, 4, 0
  %717 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %716, i64 4, 3, 1
  %718 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %717, i64 1, 4, 1
  %719 = call ptr @llvm.stacksave.p0()
  %720 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %718, ptr %720, align 8
  %721 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %720, 1
  %722 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %340, ptr %722, align 8
  %723 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %722, 1
  %724 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %721, ptr %724, align 8
  %725 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %723, ptr %725, align 8
  call void @memrefCopy(i64 4, ptr %724, ptr %725)
  call void @llvm.stackrestore.p0(ptr %719)
  %726 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %727 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %728 = insertvalue { ptr, ptr, i64 } poison, ptr %726, 0
  %729 = insertvalue { ptr, ptr, i64 } %728, ptr %727, 1
  %730 = insertvalue { ptr, ptr, i64 } %729, i64 0, 2
  %731 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %732 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %733 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %734 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %735 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %736 = extractvalue { ptr, ptr, i64 } %730, 0
  %737 = extractvalue { ptr, ptr, i64 } %730, 1
  %738 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %736, 0
  %739 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %738, ptr %737, 1
  %740 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %739, i64 280, 2
  %741 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %740, i64 4, 3, 0
  %742 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %741, i64 35, 4, 0
  %743 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %742, i64 16, 3, 1
  %744 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %743, i64 1, 4, 1
  %745 = call ptr @llvm.stacksave.p0()
  %746 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %744, ptr %746, align 8
  %747 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %746, 1
  %748 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %378, ptr %748, align 8
  %749 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %748, 1
  %750 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %747, ptr %750, align 8
  %751 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %749, ptr %751, align 8
  call void @memrefCopy(i64 4, ptr %750, ptr %751)
  call void @llvm.stackrestore.p0(ptr %745)
  %752 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %753 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %754 = insertvalue { ptr, ptr, i64 } poison, ptr %752, 0
  %755 = insertvalue { ptr, ptr, i64 } %754, ptr %753, 1
  %756 = insertvalue { ptr, ptr, i64 } %755, i64 0, 2
  %757 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %758 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %759 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %760 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %761 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %762 = extractvalue { ptr, ptr, i64 } %756, 0
  %763 = extractvalue { ptr, ptr, i64 } %756, 1
  %764 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %762, 0
  %765 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %764, ptr %763, 1
  %766 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %765, i64 0, 2
  %767 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %766, i64 16, 3, 0
  %768 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %767, i64 12, 4, 0
  %769 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %768, i64 4, 3, 1
  %770 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %769, i64 1, 4, 1
  %771 = call ptr @llvm.stacksave.p0()
  %772 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %770, ptr %772, align 8
  %773 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %772, 1
  %774 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %416, ptr %774, align 8
  %775 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %774, 1
  %776 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %773, ptr %776, align 8
  %777 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %775, ptr %777, align 8
  call void @memrefCopy(i64 4, ptr %776, ptr %777)
  call void @llvm.stackrestore.p0(ptr %771)
  %778 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %388, ptr %426, ptr %350, i32 3, i32 0, i32 117)
  %779 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %780 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %781 = insertvalue { ptr, ptr, i64 } poison, ptr %779, 0
  %782 = insertvalue { ptr, ptr, i64 } %781, ptr %780, 1
  %783 = insertvalue { ptr, ptr, i64 } %782, i64 0, 2
  %784 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %785 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %786 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %787 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %788 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %789 = extractvalue { ptr, ptr, i64 } %783, 0
  %790 = extractvalue { ptr, ptr, i64 } %783, 1
  %791 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %789, 0
  %792 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %791, ptr %790, 1
  %793 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %792, i64 296, 2
  %794 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %793, i64 4, 3, 0
  %795 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %794, i64 35, 4, 0
  %796 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %795, i64 16, 3, 1
  %797 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %796, i64 1, 4, 1
  %798 = call ptr @llvm.stacksave.p0()
  %799 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %797, ptr %799, align 8
  %800 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %799, 1
  %801 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %378, ptr %801, align 8
  %802 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %801, 1
  %803 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %800, ptr %803, align 8
  %804 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %802, ptr %804, align 8
  call void @memrefCopy(i64 4, ptr %803, ptr %804)
  call void @llvm.stackrestore.p0(ptr %798)
  %805 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %806 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %807 = insertvalue { ptr, ptr, i64 } poison, ptr %805, 0
  %808 = insertvalue { ptr, ptr, i64 } %807, ptr %806, 1
  %809 = insertvalue { ptr, ptr, i64 } %808, i64 0, 2
  %810 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %811 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %812 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %813 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %814 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %815 = extractvalue { ptr, ptr, i64 } %809, 0
  %816 = extractvalue { ptr, ptr, i64 } %809, 1
  %817 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %815, 0
  %818 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %817, ptr %816, 1
  %819 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %818, i64 192, 2
  %820 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %819, i64 16, 3, 0
  %821 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %820, i64 12, 4, 0
  %822 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %821, i64 4, 3, 1
  %823 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %822, i64 1, 4, 1
  %824 = call ptr @llvm.stacksave.p0()
  %825 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %823, ptr %825, align 8
  %826 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %825, 1
  %827 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %416, ptr %827, align 8
  %828 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %827, 1
  %829 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %826, ptr %829, align 8
  %830 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %828, ptr %830, align 8
  call void @memrefCopy(i64 4, ptr %829, ptr %830)
  call void @llvm.stackrestore.p0(ptr %824)
  %831 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %388, ptr %426, ptr %350, i32 3, i32 42, i32 117)
  %832 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %833 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %834 = insertvalue { ptr, ptr, i64 } poison, ptr %832, 0
  %835 = insertvalue { ptr, ptr, i64 } %834, ptr %833, 1
  %836 = insertvalue { ptr, ptr, i64 } %835, i64 0, 2
  %837 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %838 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %839 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %840 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %841 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %842 = extractvalue { ptr, ptr, i64 } %836, 0
  %843 = extractvalue { ptr, ptr, i64 } %836, 1
  %844 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %842, 0
  %845 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %844, ptr %843, 1
  %846 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %845, i64 312, 2
  %847 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %846, i64 4, 3, 0
  %848 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %847, i64 35, 4, 0
  %849 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %848, i64 3, 3, 1
  %850 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %849, i64 1, 4, 1
  %851 = call ptr @llvm.stacksave.p0()
  %852 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %850, ptr %852, align 8
  %853 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %852, 1
  %854 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %508, ptr %854, align 8
  %855 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %854, 1
  %856 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %853, ptr %856, align 8
  %857 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %855, ptr %857, align 8
  call void @memrefCopy(i64 4, ptr %856, ptr %857)
  call void @llvm.stackrestore.p0(ptr %851)
  %858 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %859 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %860 = insertvalue { ptr, ptr, i64 } poison, ptr %858, 0
  %861 = insertvalue { ptr, ptr, i64 } %860, ptr %859, 1
  %862 = insertvalue { ptr, ptr, i64 } %861, i64 0, 2
  %863 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %864 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %865 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %866 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %867 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %868 = extractvalue { ptr, ptr, i64 } %862, 0
  %869 = extractvalue { ptr, ptr, i64 } %862, 1
  %870 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %868, 0
  %871 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %870, ptr %869, 1
  %872 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %871, i64 384, 2
  %873 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %872, i64 3, 3, 0
  %874 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %873, i64 12, 4, 0
  %875 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %874, i64 4, 3, 1
  %876 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %875, i64 1, 4, 1
  %877 = call ptr @llvm.stacksave.p0()
  %878 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %876, ptr %878, align 8
  %879 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %878, 1
  %880 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %546, ptr %880, align 8
  %881 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %880, 1
  %882 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %879, ptr %882, align 8
  %883 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %881, ptr %883, align 8
  call void @memrefCopy(i64 4, ptr %882, ptr %883)
  call void @llvm.stackrestore.p0(ptr %877)
  %884 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 3, i32 4, ptr %518, ptr %556, ptr %350, i32 3, i32 120, i32 104)
  %885 = call ptr @llvm.stacksave.p0()
  %886 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %340, ptr %886, align 8
  %887 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %886, 1
  %888 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %718, ptr %888, align 8
  %889 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %888, 1
  %890 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %887, ptr %890, align 8
  %891 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %889, ptr %891, align 8
  call void @memrefCopy(i64 4, ptr %890, ptr %891)
  call void @llvm.stackrestore.p0(ptr %885)
  %892 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %893 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %894 = insertvalue { ptr, ptr, i64 } poison, ptr %892, 0
  %895 = insertvalue { ptr, ptr, i64 } %894, ptr %893, 1
  %896 = insertvalue { ptr, ptr, i64 } %895, i64 0, 2
  %897 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %898 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %899 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %900 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %901 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %902 = extractvalue { ptr, ptr, i64 } %896, 0
  %903 = extractvalue { ptr, ptr, i64 } %896, 1
  %904 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %902, 0
  %905 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %904, ptr %903, 1
  %906 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %905, i64 100, 2
  %907 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %906, i64 4, 3, 0
  %908 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %907, i64 12, 4, 0
  %909 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %908, i64 4, 3, 1
  %910 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %909, i64 1, 4, 1
  %911 = call ptr @llvm.stacksave.p0()
  %912 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %910, ptr %912, align 8
  %913 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %912, 1
  %914 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %340, ptr %914, align 8
  %915 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %914, 1
  %916 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %913, ptr %916, align 8
  %917 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %915, ptr %917, align 8
  call void @memrefCopy(i64 4, ptr %916, ptr %917)
  call void @llvm.stackrestore.p0(ptr %911)
  %918 = call ptr @llvm.stacksave.p0()
  %919 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %744, ptr %919, align 8
  %920 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %919, 1
  %921 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %378, ptr %921, align 8
  %922 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %921, 1
  %923 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %920, ptr %923, align 8
  %924 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %922, ptr %924, align 8
  call void @memrefCopy(i64 4, ptr %923, ptr %924)
  call void @llvm.stackrestore.p0(ptr %918)
  %925 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %926 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %927 = insertvalue { ptr, ptr, i64 } poison, ptr %925, 0
  %928 = insertvalue { ptr, ptr, i64 } %927, ptr %926, 1
  %929 = insertvalue { ptr, ptr, i64 } %928, i64 0, 2
  %930 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %931 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %932 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %933 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %934 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %935 = extractvalue { ptr, ptr, i64 } %929, 0
  %936 = extractvalue { ptr, ptr, i64 } %929, 1
  %937 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %935, 0
  %938 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %937, ptr %936, 1
  %939 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %938, i64 4, 2
  %940 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %939, i64 16, 3, 0
  %941 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %940, i64 12, 4, 0
  %942 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %941, i64 4, 3, 1
  %943 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %942, i64 1, 4, 1
  %944 = call ptr @llvm.stacksave.p0()
  %945 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %943, ptr %945, align 8
  %946 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %945, 1
  %947 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %416, ptr %947, align 8
  %948 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %947, 1
  %949 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %946, ptr %949, align 8
  %950 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %948, ptr %950, align 8
  call void @memrefCopy(i64 4, ptr %949, ptr %950)
  call void @llvm.stackrestore.p0(ptr %944)
  %951 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %388, ptr %426, ptr %350, i32 1, i32 189, i32 117)
  %952 = call ptr @llvm.stacksave.p0()
  %953 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %797, ptr %953, align 8
  %954 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %953, 1
  %955 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %378, ptr %955, align 8
  %956 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %955, 1
  %957 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %954, ptr %957, align 8
  %958 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %956, ptr %958, align 8
  call void @memrefCopy(i64 4, ptr %957, ptr %958)
  call void @llvm.stackrestore.p0(ptr %952)
  %959 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %960 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %961 = insertvalue { ptr, ptr, i64 } poison, ptr %959, 0
  %962 = insertvalue { ptr, ptr, i64 } %961, ptr %960, 1
  %963 = insertvalue { ptr, ptr, i64 } %962, i64 0, 2
  %964 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %965 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %966 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %967 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %968 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %969 = extractvalue { ptr, ptr, i64 } %963, 0
  %970 = extractvalue { ptr, ptr, i64 } %963, 1
  %971 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %969, 0
  %972 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %971, ptr %970, 1
  %973 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %972, i64 196, 2
  %974 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %973, i64 16, 3, 0
  %975 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %974, i64 12, 4, 0
  %976 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %975, i64 4, 3, 1
  %977 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %976, i64 1, 4, 1
  %978 = call ptr @llvm.stacksave.p0()
  %979 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %977, ptr %979, align 8
  %980 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %979, 1
  %981 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %416, ptr %981, align 8
  %982 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %981, 1
  %983 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %980, ptr %983, align 8
  %984 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %982, ptr %984, align 8
  call void @memrefCopy(i64 4, ptr %983, ptr %984)
  call void @llvm.stackrestore.p0(ptr %978)
  %985 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %388, ptr %426, ptr %350, i32 1, i32 258, i32 117)
  %986 = call ptr @llvm.stacksave.p0()
  %987 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %850, ptr %987, align 8
  %988 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %987, 1
  %989 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %508, ptr %989, align 8
  %990 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %989, 1
  %991 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %988, ptr %991, align 8
  %992 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %990, ptr %992, align 8
  call void @memrefCopy(i64 4, ptr %991, ptr %992)
  call void @llvm.stackrestore.p0(ptr %986)
  %993 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %994 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %995 = insertvalue { ptr, ptr, i64 } poison, ptr %993, 0
  %996 = insertvalue { ptr, ptr, i64 } %995, ptr %994, 1
  %997 = insertvalue { ptr, ptr, i64 } %996, i64 0, 2
  %998 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %999 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %1000 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %1001 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %1002 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %1003 = extractvalue { ptr, ptr, i64 } %997, 0
  %1004 = extractvalue { ptr, ptr, i64 } %997, 1
  %1005 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1003, 0
  %1006 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1005, ptr %1004, 1
  %1007 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1006, i64 388, 2
  %1008 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1007, i64 3, 3, 0
  %1009 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1008, i64 12, 4, 0
  %1010 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1009, i64 4, 3, 1
  %1011 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1010, i64 1, 4, 1
  %1012 = call ptr @llvm.stacksave.p0()
  %1013 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1011, ptr %1013, align 8
  %1014 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1013, 1
  %1015 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %546, ptr %1015, align 8
  %1016 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1015, 1
  %1017 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1014, ptr %1017, align 8
  %1018 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1016, ptr %1018, align 8
  call void @memrefCopy(i64 4, ptr %1017, ptr %1018)
  call void @llvm.stackrestore.p0(ptr %1012)
  %1019 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 3, i32 4, ptr %518, ptr %556, ptr %350, i32 1, i32 327, i32 104)
  %1020 = call ptr @llvm.stacksave.p0()
  %1021 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %340, ptr %1021, align 8
  %1022 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1021, 1
  %1023 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %910, ptr %1023, align 8
  %1024 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1023, 1
  %1025 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1022, ptr %1025, align 8
  %1026 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1024, ptr %1026, align 8
  call void @memrefCopy(i64 4, ptr %1025, ptr %1026)
  call void @llvm.stackrestore.p0(ptr %1020)
  %1027 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %1028 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %1029 = insertvalue { ptr, ptr, i64 } poison, ptr %1027, 0
  %1030 = insertvalue { ptr, ptr, i64 } %1029, ptr %1028, 1
  %1031 = insertvalue { ptr, ptr, i64 } %1030, i64 0, 2
  %1032 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %1033 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %1034 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %1035 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %1036 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %1037 = extractvalue { ptr, ptr, i64 } %1031, 0
  %1038 = extractvalue { ptr, ptr, i64 } %1031, 1
  %1039 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1037, 0
  %1040 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1039, ptr %1038, 1
  %1041 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1040, i64 104, 2
  %1042 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1041, i64 4, 3, 0
  %1043 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1042, i64 12, 4, 0
  %1044 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1043, i64 4, 3, 1
  %1045 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1044, i64 1, 4, 1
  %1046 = call ptr @llvm.stacksave.p0()
  %1047 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1045, ptr %1047, align 8
  %1048 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1047, 1
  %1049 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %340, ptr %1049, align 8
  %1050 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1049, 1
  %1051 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1048, ptr %1051, align 8
  %1052 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1050, ptr %1052, align 8
  call void @memrefCopy(i64 4, ptr %1051, ptr %1052)
  call void @llvm.stackrestore.p0(ptr %1046)
  %1053 = call ptr @llvm.stacksave.p0()
  %1054 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %744, ptr %1054, align 8
  %1055 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1054, 1
  %1056 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %378, ptr %1056, align 8
  %1057 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1056, 1
  %1058 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1055, ptr %1058, align 8
  %1059 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1057, ptr %1059, align 8
  call void @memrefCopy(i64 4, ptr %1058, ptr %1059)
  call void @llvm.stackrestore.p0(ptr %1053)
  %1060 = call ptr @llvm.stacksave.p0()
  %1061 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %407, ptr %1061, align 8
  %1062 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1061, 1
  %1063 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %416, ptr %1063, align 8
  %1064 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1063, 1
  %1065 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1062, ptr %1065, align 8
  %1066 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1064, ptr %1066, align 8
  call void @memrefCopy(i64 4, ptr %1065, ptr %1066)
  call void @llvm.stackrestore.p0(ptr %1060)
  %1067 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %388, ptr %426, ptr %350, i32 2, i32 189, i32 117)
  %1068 = call ptr @llvm.stacksave.p0()
  %1069 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %797, ptr %1069, align 8
  %1070 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1069, 1
  %1071 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %378, ptr %1071, align 8
  %1072 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1071, 1
  %1073 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1070, ptr %1073, align 8
  %1074 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1072, ptr %1074, align 8
  call void @memrefCopy(i64 4, ptr %1073, ptr %1074)
  call void @llvm.stackrestore.p0(ptr %1068)
  %1075 = call ptr @llvm.stacksave.p0()
  %1076 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %472, ptr %1076, align 8
  %1077 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1076, 1
  %1078 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %416, ptr %1078, align 8
  %1079 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1078, 1
  %1080 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1077, ptr %1080, align 8
  %1081 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1079, ptr %1081, align 8
  call void @memrefCopy(i64 4, ptr %1080, ptr %1081)
  call void @llvm.stackrestore.p0(ptr %1075)
  %1082 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %388, ptr %426, ptr %350, i32 2, i32 258, i32 117)
  %1083 = call ptr @llvm.stacksave.p0()
  %1084 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %850, ptr %1084, align 8
  %1085 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1084, 1
  %1086 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %508, ptr %1086, align 8
  %1087 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1086, 1
  %1088 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1085, ptr %1088, align 8
  %1089 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1087, ptr %1089, align 8
  call void @memrefCopy(i64 4, ptr %1088, ptr %1089)
  call void @llvm.stackrestore.p0(ptr %1083)
  %1090 = call ptr @llvm.stacksave.p0()
  %1091 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %537, ptr %1091, align 8
  %1092 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1091, 1
  %1093 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %546, ptr %1093, align 8
  %1094 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1093, 1
  %1095 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1092, ptr %1095, align 8
  %1096 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1094, ptr %1096, align 8
  call void @memrefCopy(i64 4, ptr %1095, ptr %1096)
  call void @llvm.stackrestore.p0(ptr %1090)
  %1097 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 3, i32 4, ptr %518, ptr %556, ptr %350, i32 2, i32 327, i32 104)
  %1098 = call ptr @llvm.stacksave.p0()
  %1099 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %340, ptr %1099, align 8
  %1100 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1099, 1
  %1101 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1045, ptr %1101, align 8
  %1102 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1101, 1
  %1103 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1100, ptr %1103, align 8
  %1104 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1102, ptr %1104, align 8
  call void @memrefCopy(i64 4, ptr %1103, ptr %1104)
  call void @llvm.stackrestore.p0(ptr %1098)
  %1105 = call ptr @malloc(i64 576)
  %1106 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1105, 0
  %1107 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1106, ptr %1105, 1
  %1108 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1107, i64 0, 2
  %1109 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1108, i64 12, 3, 0
  %1110 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1109, i64 12, 3, 1
  %1111 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1110, i64 12, 4, 0
  %1112 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1111, i64 1, 4, 1
  %1113 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %1114 = mul i64 1, %1113
  %1115 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %1116 = mul i64 %1114, %1115
  %1117 = mul i64 %1116, 4
  %1118 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %1119 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %1120 = getelementptr float, ptr %1118, i64 %1119
  %1121 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1112, 1
  %1122 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1112, 2
  %1123 = getelementptr float, ptr %1121, i64 %1122
  call void @llvm.memcpy.p0.p0.i64(ptr %1123, ptr %1120, i64 %1117, i1 false)
  ret { ptr, ptr, i64, [2 x i64], [2 x i64] } %1112
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
