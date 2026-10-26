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
  %77 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %76, i64 64, 4, 0
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
  %153 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %152, i64 64, 4, 0
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
  %216 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %215, i64 1024, 2
  %217 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %216, i64 16, 3, 0
  %218 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %217, i64 64, 4, 0
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
  %252 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %251, i64 64, 4, 0
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
  %285 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %284, i64 64, 4, 0
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
  %317 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %316, i64 1032, 2
  %318 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %317, i64 16, 3, 0
  %319 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %318, i64 64, 4, 0
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
  %353 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %352, i64 64, 4, 0
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
  %386 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %385, i64 64, 4, 0
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
  %418 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %417, i64 1040, 2
  %419 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %418, i64 16, 3, 0
  %420 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %419, i64 64, 4, 0
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
  %454 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %453, i64 64, 4, 0
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
  %487 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %486, i64 64, 4, 0
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
  %519 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %518, i64 1048, 2
  %520 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %519, i64 16, 3, 0
  %521 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %520, i64 64, 4, 0
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
  %553 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %552, i64 32, 2
  %554 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %553, i64 8, 3, 0
  %555 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %554, i64 64, 4, 0
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
  %565 = call ptr @llvm.stacksave.p0()
  %566 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %117, ptr %566, align 8
  %567 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %566, 1
  %568 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %568, align 8
  %569 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %568, 1
  %570 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %567, ptr %570, align 8
  %571 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %569, ptr %571, align 8
  call void @memrefCopy(i64 4, ptr %570, ptr %571)
  call void @llvm.stackrestore.p0(ptr %565)
  %572 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %573 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %574 = insertvalue { ptr, ptr, i64 } poison, ptr %572, 0
  %575 = insertvalue { ptr, ptr, i64 } %574, ptr %573, 1
  %576 = insertvalue { ptr, ptr, i64 } %575, i64 0, 2
  %577 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %578 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %579 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %580 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %581 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %582 = extractvalue { ptr, ptr, i64 } %576, 0
  %583 = extractvalue { ptr, ptr, i64 } %576, 1
  %584 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %582, 0
  %585 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %584, ptr %583, 1
  %586 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %585, i64 32, 2
  %587 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %586, i64 16, 3, 0
  %588 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %587, i64 64, 4, 0
  %589 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %588, i64 8, 3, 1
  %590 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %589, i64 1, 4, 1
  %591 = call ptr @llvm.stacksave.p0()
  %592 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %590, ptr %592, align 8
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
  %599 = call ptr @llvm.stacksave.p0()
  %600 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %194, ptr %600, align 8
  %601 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %600, 1
  %602 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %602, align 8
  %603 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %602, 1
  %604 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %601, ptr %604, align 8
  %605 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %603, ptr %605, align 8
  call void @memrefCopy(i64 4, ptr %604, ptr %605)
  call void @llvm.stackrestore.p0(ptr %599)
  %606 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %607 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %608 = insertvalue { ptr, ptr, i64 } poison, ptr %606, 0
  %609 = insertvalue { ptr, ptr, i64 } %608, ptr %607, 1
  %610 = insertvalue { ptr, ptr, i64 } %609, i64 0, 2
  %611 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %612 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %613 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %614 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %615 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %616 = extractvalue { ptr, ptr, i64 } %610, 0
  %617 = extractvalue { ptr, ptr, i64 } %610, 1
  %618 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %616, 0
  %619 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %618, ptr %617, 1
  %620 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %619, i64 1056, 2
  %621 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %620, i64 16, 3, 0
  %622 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %621, i64 64, 4, 0
  %623 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %622, i64 8, 3, 1
  %624 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %623, i64 1, 4, 1
  %625 = call ptr @llvm.stacksave.p0()
  %626 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %624, ptr %626, align 8
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
  %654 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %653, i64 40, 2
  %655 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %654, i64 8, 3, 0
  %656 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %655, i64 64, 4, 0
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
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %117, ptr %667, align 8
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
  %673 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %674 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %675 = insertvalue { ptr, ptr, i64 } poison, ptr %673, 0
  %676 = insertvalue { ptr, ptr, i64 } %675, ptr %674, 1
  %677 = insertvalue { ptr, ptr, i64 } %676, i64 0, 2
  %678 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %679 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %680 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %681 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %682 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %683 = extractvalue { ptr, ptr, i64 } %677, 0
  %684 = extractvalue { ptr, ptr, i64 } %677, 1
  %685 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %683, 0
  %686 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %685, ptr %684, 1
  %687 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %686, i64 40, 2
  %688 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %687, i64 16, 3, 0
  %689 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %688, i64 64, 4, 0
  %690 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %689, i64 8, 3, 1
  %691 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %690, i64 1, 4, 1
  %692 = call ptr @llvm.stacksave.p0()
  %693 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %691, ptr %693, align 8
  %694 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %693, 1
  %695 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %695, align 8
  %696 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %695, 1
  %697 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %694, ptr %697, align 8
  %698 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %696, ptr %698, align 8
  call void @memrefCopy(i64 4, ptr %697, ptr %698)
  call void @llvm.stackrestore.p0(ptr %692)
  %699 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 680, i32 125)
  %700 = call ptr @llvm.stacksave.p0()
  %701 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %194, ptr %701, align 8
  %702 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %701, 1
  %703 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %703, align 8
  %704 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %703, 1
  %705 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %702, ptr %705, align 8
  %706 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %704, ptr %706, align 8
  call void @memrefCopy(i64 4, ptr %705, ptr %706)
  call void @llvm.stackrestore.p0(ptr %700)
  %707 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %708 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %709 = insertvalue { ptr, ptr, i64 } poison, ptr %707, 0
  %710 = insertvalue { ptr, ptr, i64 } %709, ptr %708, 1
  %711 = insertvalue { ptr, ptr, i64 } %710, i64 0, 2
  %712 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %713 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %714 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %715 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %716 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %717 = extractvalue { ptr, ptr, i64 } %711, 0
  %718 = extractvalue { ptr, ptr, i64 } %711, 1
  %719 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %717, 0
  %720 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %719, ptr %718, 1
  %721 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %720, i64 1064, 2
  %722 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %721, i64 16, 3, 0
  %723 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %722, i64 64, 4, 0
  %724 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %723, i64 8, 3, 1
  %725 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %724, i64 1, 4, 1
  %726 = call ptr @llvm.stacksave.p0()
  %727 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %725, ptr %727, align 8
  %728 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %727, 1
  %729 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %729, align 8
  %730 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %729, 1
  %731 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %728, ptr %731, align 8
  %732 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %730, ptr %732, align 8
  call void @memrefCopy(i64 4, ptr %731, ptr %732)
  call void @llvm.stackrestore.p0(ptr %726)
  %733 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 749, i32 125)
  %734 = call ptr @llvm.stacksave.p0()
  %735 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %735, align 8
  %736 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %735, 1
  %737 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %658, ptr %737, align 8
  %738 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %737, 1
  %739 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %736, ptr %739, align 8
  %740 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %738, ptr %740, align 8
  call void @memrefCopy(i64 4, ptr %739, ptr %740)
  call void @llvm.stackrestore.p0(ptr %734)
  %741 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %742 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %743 = insertvalue { ptr, ptr, i64 } poison, ptr %741, 0
  %744 = insertvalue { ptr, ptr, i64 } %743, ptr %742, 1
  %745 = insertvalue { ptr, ptr, i64 } %744, i64 0, 2
  %746 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %747 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %748 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %749 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %750 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %751 = extractvalue { ptr, ptr, i64 } %745, 0
  %752 = extractvalue { ptr, ptr, i64 } %745, 1
  %753 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %751, 0
  %754 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %753, ptr %752, 1
  %755 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %754, i64 48, 2
  %756 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %755, i64 8, 3, 0
  %757 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %756, i64 64, 4, 0
  %758 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %757, i64 8, 3, 1
  %759 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %758, i64 1, 4, 1
  %760 = call ptr @llvm.stacksave.p0()
  %761 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %759, ptr %761, align 8
  %762 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %761, 1
  %763 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %763, align 8
  %764 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %763, 1
  %765 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %762, ptr %765, align 8
  %766 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %764, ptr %766, align 8
  call void @memrefCopy(i64 4, ptr %765, ptr %766)
  call void @llvm.stackrestore.p0(ptr %760)
  %767 = call ptr @llvm.stacksave.p0()
  %768 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %117, ptr %768, align 8
  %769 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %768, 1
  %770 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %770, align 8
  %771 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %770, 1
  %772 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %769, ptr %772, align 8
  %773 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %771, ptr %773, align 8
  call void @memrefCopy(i64 4, ptr %772, ptr %773)
  call void @llvm.stackrestore.p0(ptr %767)
  %774 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %775 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %776 = insertvalue { ptr, ptr, i64 } poison, ptr %774, 0
  %777 = insertvalue { ptr, ptr, i64 } %776, ptr %775, 1
  %778 = insertvalue { ptr, ptr, i64 } %777, i64 0, 2
  %779 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %780 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %781 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %782 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %783 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %784 = extractvalue { ptr, ptr, i64 } %778, 0
  %785 = extractvalue { ptr, ptr, i64 } %778, 1
  %786 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %784, 0
  %787 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %786, ptr %785, 1
  %788 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %787, i64 48, 2
  %789 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %788, i64 16, 3, 0
  %790 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %789, i64 64, 4, 0
  %791 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %790, i64 8, 3, 1
  %792 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %791, i64 1, 4, 1
  %793 = call ptr @llvm.stacksave.p0()
  %794 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %792, ptr %794, align 8
  %795 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %794, 1
  %796 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %796, align 8
  %797 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %796, 1
  %798 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %795, ptr %798, align 8
  %799 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %797, ptr %799, align 8
  call void @memrefCopy(i64 4, ptr %798, ptr %799)
  call void @llvm.stackrestore.p0(ptr %793)
  %800 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 818, i32 125)
  %801 = call ptr @llvm.stacksave.p0()
  %802 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %194, ptr %802, align 8
  %803 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %802, 1
  %804 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %804, align 8
  %805 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %804, 1
  %806 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %803, ptr %806, align 8
  %807 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %805, ptr %807, align 8
  call void @memrefCopy(i64 4, ptr %806, ptr %807)
  call void @llvm.stackrestore.p0(ptr %801)
  %808 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %809 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %810 = insertvalue { ptr, ptr, i64 } poison, ptr %808, 0
  %811 = insertvalue { ptr, ptr, i64 } %810, ptr %809, 1
  %812 = insertvalue { ptr, ptr, i64 } %811, i64 0, 2
  %813 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %814 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %815 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %816 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %817 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %818 = extractvalue { ptr, ptr, i64 } %812, 0
  %819 = extractvalue { ptr, ptr, i64 } %812, 1
  %820 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %818, 0
  %821 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %820, ptr %819, 1
  %822 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %821, i64 1072, 2
  %823 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %822, i64 16, 3, 0
  %824 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %823, i64 64, 4, 0
  %825 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %824, i64 8, 3, 1
  %826 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %825, i64 1, 4, 1
  %827 = call ptr @llvm.stacksave.p0()
  %828 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %826, ptr %828, align 8
  %829 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %828, 1
  %830 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %830, align 8
  %831 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %830, 1
  %832 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %829, ptr %832, align 8
  %833 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %831, ptr %833, align 8
  call void @memrefCopy(i64 4, ptr %832, ptr %833)
  call void @llvm.stackrestore.p0(ptr %827)
  %834 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 887, i32 125)
  %835 = call ptr @llvm.stacksave.p0()
  %836 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %836, align 8
  %837 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %836, 1
  %838 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %759, ptr %838, align 8
  %839 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %838, 1
  %840 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %837, ptr %840, align 8
  %841 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %839, ptr %841, align 8
  call void @memrefCopy(i64 4, ptr %840, ptr %841)
  call void @llvm.stackrestore.p0(ptr %835)
  %842 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %843 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %844 = insertvalue { ptr, ptr, i64 } poison, ptr %842, 0
  %845 = insertvalue { ptr, ptr, i64 } %844, ptr %843, 1
  %846 = insertvalue { ptr, ptr, i64 } %845, i64 0, 2
  %847 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %848 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %849 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %850 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %851 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %852 = extractvalue { ptr, ptr, i64 } %846, 0
  %853 = extractvalue { ptr, ptr, i64 } %846, 1
  %854 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %852, 0
  %855 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %854, ptr %853, 1
  %856 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %855, i64 56, 2
  %857 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %856, i64 8, 3, 0
  %858 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %857, i64 64, 4, 0
  %859 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %858, i64 8, 3, 1
  %860 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %859, i64 1, 4, 1
  %861 = call ptr @llvm.stacksave.p0()
  %862 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %860, ptr %862, align 8
  %863 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %862, 1
  %864 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %864, align 8
  %865 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %864, 1
  %866 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %863, ptr %866, align 8
  %867 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %865, ptr %867, align 8
  call void @memrefCopy(i64 4, ptr %866, ptr %867)
  call void @llvm.stackrestore.p0(ptr %861)
  %868 = call ptr @llvm.stacksave.p0()
  %869 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %117, ptr %869, align 8
  %870 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %869, 1
  %871 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %871, align 8
  %872 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %871, 1
  %873 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %870, ptr %873, align 8
  %874 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %872, ptr %874, align 8
  call void @memrefCopy(i64 4, ptr %873, ptr %874)
  call void @llvm.stackrestore.p0(ptr %868)
  %875 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %876 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %877 = insertvalue { ptr, ptr, i64 } poison, ptr %875, 0
  %878 = insertvalue { ptr, ptr, i64 } %877, ptr %876, 1
  %879 = insertvalue { ptr, ptr, i64 } %878, i64 0, 2
  %880 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %881 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %882 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %883 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %884 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %885 = extractvalue { ptr, ptr, i64 } %879, 0
  %886 = extractvalue { ptr, ptr, i64 } %879, 1
  %887 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %885, 0
  %888 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %887, ptr %886, 1
  %889 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %888, i64 56, 2
  %890 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %889, i64 16, 3, 0
  %891 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %890, i64 64, 4, 0
  %892 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %891, i64 8, 3, 1
  %893 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %892, i64 1, 4, 1
  %894 = call ptr @llvm.stacksave.p0()
  %895 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %893, ptr %895, align 8
  %896 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %895, 1
  %897 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %897, align 8
  %898 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %897, 1
  %899 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %896, ptr %899, align 8
  %900 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %898, ptr %900, align 8
  call void @memrefCopy(i64 4, ptr %899, ptr %900)
  call void @llvm.stackrestore.p0(ptr %894)
  %901 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 956, i32 125)
  %902 = call ptr @llvm.stacksave.p0()
  %903 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %194, ptr %903, align 8
  %904 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %903, 1
  %905 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %905, align 8
  %906 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %905, 1
  %907 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %904, ptr %907, align 8
  %908 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %906, ptr %908, align 8
  call void @memrefCopy(i64 4, ptr %907, ptr %908)
  call void @llvm.stackrestore.p0(ptr %902)
  %909 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %910 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %911 = insertvalue { ptr, ptr, i64 } poison, ptr %909, 0
  %912 = insertvalue { ptr, ptr, i64 } %911, ptr %910, 1
  %913 = insertvalue { ptr, ptr, i64 } %912, i64 0, 2
  %914 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %915 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %916 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %917 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %918 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %919 = extractvalue { ptr, ptr, i64 } %913, 0
  %920 = extractvalue { ptr, ptr, i64 } %913, 1
  %921 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %919, 0
  %922 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %921, ptr %920, 1
  %923 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %922, i64 1080, 2
  %924 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %923, i64 16, 3, 0
  %925 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %924, i64 64, 4, 0
  %926 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %925, i64 8, 3, 1
  %927 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %926, i64 1, 4, 1
  %928 = call ptr @llvm.stacksave.p0()
  %929 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %927, ptr %929, align 8
  %930 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %929, 1
  %931 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %931, align 8
  %932 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %931, 1
  %933 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %930, ptr %933, align 8
  %934 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %932, ptr %934, align 8
  call void @memrefCopy(i64 4, ptr %933, ptr %934)
  call void @llvm.stackrestore.p0(ptr %928)
  %935 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 1025, i32 125)
  %936 = call ptr @llvm.stacksave.p0()
  %937 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %937, align 8
  %938 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %937, 1
  %939 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %860, ptr %939, align 8
  %940 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %939, 1
  %941 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %938, ptr %941, align 8
  %942 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %940, ptr %942, align 8
  call void @memrefCopy(i64 4, ptr %941, ptr %942)
  call void @llvm.stackrestore.p0(ptr %936)
  %943 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %944 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %945 = insertvalue { ptr, ptr, i64 } poison, ptr %943, 0
  %946 = insertvalue { ptr, ptr, i64 } %945, ptr %944, 1
  %947 = insertvalue { ptr, ptr, i64 } %946, i64 0, 2
  %948 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %949 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %950 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %951 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %952 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %953 = extractvalue { ptr, ptr, i64 } %947, 0
  %954 = extractvalue { ptr, ptr, i64 } %947, 1
  %955 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %953, 0
  %956 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %955, ptr %954, 1
  %957 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %956, i64 512, 2
  %958 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %957, i64 8, 3, 0
  %959 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %958, i64 64, 4, 0
  %960 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %959, i64 8, 3, 1
  %961 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %960, i64 1, 4, 1
  %962 = call ptr @llvm.stacksave.p0()
  %963 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %961, ptr %963, align 8
  %964 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %963, 1
  %965 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %965, align 8
  %966 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %965, 1
  %967 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %964, ptr %967, align 8
  %968 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %966, ptr %968, align 8
  call void @memrefCopy(i64 4, ptr %967, ptr %968)
  call void @llvm.stackrestore.p0(ptr %962)
  %969 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %970 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %971 = insertvalue { ptr, ptr, i64 } poison, ptr %969, 0
  %972 = insertvalue { ptr, ptr, i64 } %971, ptr %970, 1
  %973 = insertvalue { ptr, ptr, i64 } %972, i64 0, 2
  %974 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %975 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %976 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %977 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %978 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %979 = extractvalue { ptr, ptr, i64 } %973, 0
  %980 = extractvalue { ptr, ptr, i64 } %973, 1
  %981 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %979, 0
  %982 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %981, ptr %980, 1
  %983 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %982, i64 256, 2
  %984 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %983, i64 8, 3, 0
  %985 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %984, i64 32, 4, 0
  %986 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %985, i64 16, 3, 1
  %987 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %986, i64 1, 4, 1
  %988 = call ptr @llvm.stacksave.p0()
  %989 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %987, ptr %989, align 8
  %990 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %989, 1
  %991 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %991, align 8
  %992 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %991, 1
  %993 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %990, ptr %993, align 8
  %994 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %992, ptr %994, align 8
  call void @memrefCopy(i64 4, ptr %993, ptr %994)
  call void @llvm.stackrestore.p0(ptr %988)
  %995 = call ptr @llvm.stacksave.p0()
  %996 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %155, ptr %996, align 8
  %997 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %996, 1
  %998 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %998, align 8
  %999 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %998, 1
  %1000 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %997, ptr %1000, align 8
  %1001 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %999, ptr %1001, align 8
  call void @memrefCopy(i64 4, ptr %1000, ptr %1001)
  call void @llvm.stackrestore.p0(ptr %995)
  %1002 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 1094, i32 125)
  %1003 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %1004 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %1005 = insertvalue { ptr, ptr, i64 } poison, ptr %1003, 0
  %1006 = insertvalue { ptr, ptr, i64 } %1005, ptr %1004, 1
  %1007 = insertvalue { ptr, ptr, i64 } %1006, i64 0, 2
  %1008 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %1009 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %1010 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %1011 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %1012 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %1013 = extractvalue { ptr, ptr, i64 } %1007, 0
  %1014 = extractvalue { ptr, ptr, i64 } %1007, 1
  %1015 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1013, 0
  %1016 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1015, ptr %1014, 1
  %1017 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1016, i64 272, 2
  %1018 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1017, i64 8, 3, 0
  %1019 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1018, i64 32, 4, 0
  %1020 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1019, i64 16, 3, 1
  %1021 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1020, i64 1, 4, 1
  %1022 = call ptr @llvm.stacksave.p0()
  %1023 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1021, ptr %1023, align 8
  %1024 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1023, 1
  %1025 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %1025, align 8
  %1026 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1025, 1
  %1027 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1024, ptr %1027, align 8
  %1028 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1026, ptr %1028, align 8
  call void @memrefCopy(i64 4, ptr %1027, ptr %1028)
  call void @llvm.stackrestore.p0(ptr %1022)
  %1029 = call ptr @llvm.stacksave.p0()
  %1030 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %220, ptr %1030, align 8
  %1031 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1030, 1
  %1032 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %1032, align 8
  %1033 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1032, 1
  %1034 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1031, ptr %1034, align 8
  %1035 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1033, ptr %1035, align 8
  call void @memrefCopy(i64 4, ptr %1034, ptr %1035)
  call void @llvm.stackrestore.p0(ptr %1029)
  %1036 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 1163, i32 125)
  %1037 = call ptr @llvm.stacksave.p0()
  %1038 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %1038, align 8
  %1039 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1038, 1
  %1040 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %961, ptr %1040, align 8
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
  %1058 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1057, i64 520, 2
  %1059 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1058, i64 8, 3, 0
  %1060 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1059, i64 64, 4, 0
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
  %1070 = call ptr @llvm.stacksave.p0()
  %1071 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %987, ptr %1071, align 8
  %1072 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1071, 1
  %1073 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %1073, align 8
  %1074 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1073, 1
  %1075 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1072, ptr %1075, align 8
  %1076 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1074, ptr %1076, align 8
  call void @memrefCopy(i64 4, ptr %1075, ptr %1076)
  call void @llvm.stackrestore.p0(ptr %1070)
  %1077 = call ptr @llvm.stacksave.p0()
  %1078 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %287, ptr %1078, align 8
  %1079 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1078, 1
  %1080 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %1080, align 8
  %1081 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1080, 1
  %1082 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1079, ptr %1082, align 8
  %1083 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1081, ptr %1083, align 8
  call void @memrefCopy(i64 4, ptr %1082, ptr %1083)
  call void @llvm.stackrestore.p0(ptr %1077)
  %1084 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 1232, i32 125)
  %1085 = call ptr @llvm.stacksave.p0()
  %1086 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1021, ptr %1086, align 8
  %1087 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1086, 1
  %1088 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %1088, align 8
  %1089 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1088, 1
  %1090 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1087, ptr %1090, align 8
  %1091 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1089, ptr %1091, align 8
  call void @memrefCopy(i64 4, ptr %1090, ptr %1091)
  call void @llvm.stackrestore.p0(ptr %1085)
  %1092 = call ptr @llvm.stacksave.p0()
  %1093 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %321, ptr %1093, align 8
  %1094 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1093, 1
  %1095 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %1095, align 8
  %1096 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1095, 1
  %1097 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1094, ptr %1097, align 8
  %1098 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1096, ptr %1098, align 8
  call void @memrefCopy(i64 4, ptr %1097, ptr %1098)
  call void @llvm.stackrestore.p0(ptr %1092)
  %1099 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 1301, i32 125)
  %1100 = call ptr @llvm.stacksave.p0()
  %1101 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %1101, align 8
  %1102 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1101, 1
  %1103 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1062, ptr %1103, align 8
  %1104 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1103, 1
  %1105 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1102, ptr %1105, align 8
  %1106 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1104, ptr %1106, align 8
  call void @memrefCopy(i64 4, ptr %1105, ptr %1106)
  call void @llvm.stackrestore.p0(ptr %1100)
  %1107 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %1108 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %1109 = insertvalue { ptr, ptr, i64 } poison, ptr %1107, 0
  %1110 = insertvalue { ptr, ptr, i64 } %1109, ptr %1108, 1
  %1111 = insertvalue { ptr, ptr, i64 } %1110, i64 0, 2
  %1112 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %1113 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %1114 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %1115 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %1116 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %1117 = extractvalue { ptr, ptr, i64 } %1111, 0
  %1118 = extractvalue { ptr, ptr, i64 } %1111, 1
  %1119 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1117, 0
  %1120 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1119, ptr %1118, 1
  %1121 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1120, i64 528, 2
  %1122 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1121, i64 8, 3, 0
  %1123 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1122, i64 64, 4, 0
  %1124 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1123, i64 8, 3, 1
  %1125 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1124, i64 1, 4, 1
  %1126 = call ptr @llvm.stacksave.p0()
  %1127 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1125, ptr %1127, align 8
  %1128 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1127, 1
  %1129 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %1129, align 8
  %1130 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1129, 1
  %1131 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1128, ptr %1131, align 8
  %1132 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1130, ptr %1132, align 8
  call void @memrefCopy(i64 4, ptr %1131, ptr %1132)
  call void @llvm.stackrestore.p0(ptr %1126)
  %1133 = call ptr @llvm.stacksave.p0()
  %1134 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %987, ptr %1134, align 8
  %1135 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1134, 1
  %1136 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %1136, align 8
  %1137 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1136, 1
  %1138 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1135, ptr %1138, align 8
  %1139 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1137, ptr %1139, align 8
  call void @memrefCopy(i64 4, ptr %1138, ptr %1139)
  call void @llvm.stackrestore.p0(ptr %1133)
  %1140 = call ptr @llvm.stacksave.p0()
  %1141 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %388, ptr %1141, align 8
  %1142 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1141, 1
  %1143 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %1143, align 8
  %1144 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1143, 1
  %1145 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1142, ptr %1145, align 8
  %1146 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1144, ptr %1146, align 8
  call void @memrefCopy(i64 4, ptr %1145, ptr %1146)
  call void @llvm.stackrestore.p0(ptr %1140)
  %1147 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 1370, i32 125)
  %1148 = call ptr @llvm.stacksave.p0()
  %1149 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1021, ptr %1149, align 8
  %1150 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1149, 1
  %1151 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %1151, align 8
  %1152 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1151, 1
  %1153 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1150, ptr %1153, align 8
  %1154 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1152, ptr %1154, align 8
  call void @memrefCopy(i64 4, ptr %1153, ptr %1154)
  call void @llvm.stackrestore.p0(ptr %1148)
  %1155 = call ptr @llvm.stacksave.p0()
  %1156 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %422, ptr %1156, align 8
  %1157 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1156, 1
  %1158 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %1158, align 8
  %1159 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1158, 1
  %1160 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1157, ptr %1160, align 8
  %1161 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1159, ptr %1161, align 8
  call void @memrefCopy(i64 4, ptr %1160, ptr %1161)
  call void @llvm.stackrestore.p0(ptr %1155)
  %1162 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 1439, i32 125)
  %1163 = call ptr @llvm.stacksave.p0()
  %1164 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %1164, align 8
  %1165 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1164, 1
  %1166 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1125, ptr %1166, align 8
  %1167 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1166, 1
  %1168 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1165, ptr %1168, align 8
  %1169 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1167, ptr %1169, align 8
  call void @memrefCopy(i64 4, ptr %1168, ptr %1169)
  call void @llvm.stackrestore.p0(ptr %1163)
  %1170 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %1171 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %1172 = insertvalue { ptr, ptr, i64 } poison, ptr %1170, 0
  %1173 = insertvalue { ptr, ptr, i64 } %1172, ptr %1171, 1
  %1174 = insertvalue { ptr, ptr, i64 } %1173, i64 0, 2
  %1175 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %1176 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %1177 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %1178 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %1179 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %1180 = extractvalue { ptr, ptr, i64 } %1174, 0
  %1181 = extractvalue { ptr, ptr, i64 } %1174, 1
  %1182 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1180, 0
  %1183 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1182, ptr %1181, 1
  %1184 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1183, i64 536, 2
  %1185 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1184, i64 8, 3, 0
  %1186 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1185, i64 64, 4, 0
  %1187 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1186, i64 8, 3, 1
  %1188 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1187, i64 1, 4, 1
  %1189 = call ptr @llvm.stacksave.p0()
  %1190 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1188, ptr %1190, align 8
  %1191 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1190, 1
  %1192 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %1192, align 8
  %1193 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1192, 1
  %1194 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1191, ptr %1194, align 8
  %1195 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1193, ptr %1195, align 8
  call void @memrefCopy(i64 4, ptr %1194, ptr %1195)
  call void @llvm.stackrestore.p0(ptr %1189)
  %1196 = call ptr @llvm.stacksave.p0()
  %1197 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %987, ptr %1197, align 8
  %1198 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1197, 1
  %1199 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %1199, align 8
  %1200 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1199, 1
  %1201 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1198, ptr %1201, align 8
  %1202 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1200, ptr %1202, align 8
  call void @memrefCopy(i64 4, ptr %1201, ptr %1202)
  call void @llvm.stackrestore.p0(ptr %1196)
  %1203 = call ptr @llvm.stacksave.p0()
  %1204 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %489, ptr %1204, align 8
  %1205 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1204, 1
  %1206 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %1206, align 8
  %1207 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1206, 1
  %1208 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1205, ptr %1208, align 8
  %1209 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1207, ptr %1209, align 8
  call void @memrefCopy(i64 4, ptr %1208, ptr %1209)
  call void @llvm.stackrestore.p0(ptr %1203)
  %1210 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 1508, i32 125)
  %1211 = call ptr @llvm.stacksave.p0()
  %1212 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1021, ptr %1212, align 8
  %1213 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1212, 1
  %1214 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %1214, align 8
  %1215 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1214, 1
  %1216 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1213, ptr %1216, align 8
  %1217 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1215, ptr %1217, align 8
  call void @memrefCopy(i64 4, ptr %1216, ptr %1217)
  call void @llvm.stackrestore.p0(ptr %1211)
  %1218 = call ptr @llvm.stacksave.p0()
  %1219 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %523, ptr %1219, align 8
  %1220 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1219, 1
  %1221 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %1221, align 8
  %1222 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1221, 1
  %1223 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1220, ptr %1223, align 8
  %1224 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1222, ptr %1224, align 8
  call void @memrefCopy(i64 4, ptr %1223, ptr %1224)
  call void @llvm.stackrestore.p0(ptr %1218)
  %1225 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 1577, i32 125)
  %1226 = call ptr @llvm.stacksave.p0()
  %1227 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %1227, align 8
  %1228 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1227, 1
  %1229 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1188, ptr %1229, align 8
  %1230 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1229, 1
  %1231 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1228, ptr %1231, align 8
  %1232 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1230, ptr %1232, align 8
  call void @memrefCopy(i64 4, ptr %1231, ptr %1232)
  call void @llvm.stackrestore.p0(ptr %1226)
  %1233 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %1234 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %1235 = insertvalue { ptr, ptr, i64 } poison, ptr %1233, 0
  %1236 = insertvalue { ptr, ptr, i64 } %1235, ptr %1234, 1
  %1237 = insertvalue { ptr, ptr, i64 } %1236, i64 0, 2
  %1238 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %1239 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %1240 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %1241 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %1242 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %1243 = extractvalue { ptr, ptr, i64 } %1237, 0
  %1244 = extractvalue { ptr, ptr, i64 } %1237, 1
  %1245 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1243, 0
  %1246 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1245, ptr %1244, 1
  %1247 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1246, i64 544, 2
  %1248 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1247, i64 8, 3, 0
  %1249 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1248, i64 64, 4, 0
  %1250 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1249, i64 8, 3, 1
  %1251 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1250, i64 1, 4, 1
  %1252 = call ptr @llvm.stacksave.p0()
  %1253 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1251, ptr %1253, align 8
  %1254 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1253, 1
  %1255 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %1255, align 8
  %1256 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1255, 1
  %1257 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1254, ptr %1257, align 8
  %1258 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1256, ptr %1258, align 8
  call void @memrefCopy(i64 4, ptr %1257, ptr %1258)
  call void @llvm.stackrestore.p0(ptr %1252)
  %1259 = call ptr @llvm.stacksave.p0()
  %1260 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %987, ptr %1260, align 8
  %1261 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1260, 1
  %1262 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %1262, align 8
  %1263 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1262, 1
  %1264 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1261, ptr %1264, align 8
  %1265 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1263, ptr %1265, align 8
  call void @memrefCopy(i64 4, ptr %1264, ptr %1265)
  call void @llvm.stackrestore.p0(ptr %1259)
  %1266 = call ptr @llvm.stacksave.p0()
  %1267 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %590, ptr %1267, align 8
  %1268 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1267, 1
  %1269 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %1269, align 8
  %1270 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1269, 1
  %1271 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1268, ptr %1271, align 8
  %1272 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1270, ptr %1272, align 8
  call void @memrefCopy(i64 4, ptr %1271, ptr %1272)
  call void @llvm.stackrestore.p0(ptr %1266)
  %1273 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 1646, i32 125)
  %1274 = call ptr @llvm.stacksave.p0()
  %1275 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1021, ptr %1275, align 8
  %1276 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1275, 1
  %1277 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %1277, align 8
  %1278 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1277, 1
  %1279 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1276, ptr %1279, align 8
  %1280 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1278, ptr %1280, align 8
  call void @memrefCopy(i64 4, ptr %1279, ptr %1280)
  call void @llvm.stackrestore.p0(ptr %1274)
  %1281 = call ptr @llvm.stacksave.p0()
  %1282 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %624, ptr %1282, align 8
  %1283 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1282, 1
  %1284 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %1284, align 8
  %1285 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1284, 1
  %1286 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1283, ptr %1286, align 8
  %1287 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1285, ptr %1287, align 8
  call void @memrefCopy(i64 4, ptr %1286, ptr %1287)
  call void @llvm.stackrestore.p0(ptr %1281)
  %1288 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 1715, i32 125)
  %1289 = call ptr @llvm.stacksave.p0()
  %1290 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %1290, align 8
  %1291 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1290, 1
  %1292 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1251, ptr %1292, align 8
  %1293 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1292, 1
  %1294 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1291, ptr %1294, align 8
  %1295 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1293, ptr %1295, align 8
  call void @memrefCopy(i64 4, ptr %1294, ptr %1295)
  call void @llvm.stackrestore.p0(ptr %1289)
  %1296 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %1297 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %1298 = insertvalue { ptr, ptr, i64 } poison, ptr %1296, 0
  %1299 = insertvalue { ptr, ptr, i64 } %1298, ptr %1297, 1
  %1300 = insertvalue { ptr, ptr, i64 } %1299, i64 0, 2
  %1301 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %1302 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %1303 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %1304 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %1305 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %1306 = extractvalue { ptr, ptr, i64 } %1300, 0
  %1307 = extractvalue { ptr, ptr, i64 } %1300, 1
  %1308 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1306, 0
  %1309 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1308, ptr %1307, 1
  %1310 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1309, i64 552, 2
  %1311 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1310, i64 8, 3, 0
  %1312 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1311, i64 64, 4, 0
  %1313 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1312, i64 8, 3, 1
  %1314 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1313, i64 1, 4, 1
  %1315 = call ptr @llvm.stacksave.p0()
  %1316 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1314, ptr %1316, align 8
  %1317 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1316, 1
  %1318 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %1318, align 8
  %1319 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1318, 1
  %1320 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1317, ptr %1320, align 8
  %1321 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1319, ptr %1321, align 8
  call void @memrefCopy(i64 4, ptr %1320, ptr %1321)
  call void @llvm.stackrestore.p0(ptr %1315)
  %1322 = call ptr @llvm.stacksave.p0()
  %1323 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %987, ptr %1323, align 8
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
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %691, ptr %1330, align 8
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
  %1336 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 1784, i32 125)
  %1337 = call ptr @llvm.stacksave.p0()
  %1338 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1021, ptr %1338, align 8
  %1339 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1338, 1
  %1340 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %1340, align 8
  %1341 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1340, 1
  %1342 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1339, ptr %1342, align 8
  %1343 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1341, ptr %1343, align 8
  call void @memrefCopy(i64 4, ptr %1342, ptr %1343)
  call void @llvm.stackrestore.p0(ptr %1337)
  %1344 = call ptr @llvm.stacksave.p0()
  %1345 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %725, ptr %1345, align 8
  %1346 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1345, 1
  %1347 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %1347, align 8
  %1348 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1347, 1
  %1349 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1346, ptr %1349, align 8
  %1350 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1348, ptr %1350, align 8
  call void @memrefCopy(i64 4, ptr %1349, ptr %1350)
  call void @llvm.stackrestore.p0(ptr %1344)
  %1351 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 1853, i32 125)
  %1352 = call ptr @llvm.stacksave.p0()
  %1353 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %1353, align 8
  %1354 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1353, 1
  %1355 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1314, ptr %1355, align 8
  %1356 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1355, 1
  %1357 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1354, ptr %1357, align 8
  %1358 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1356, ptr %1358, align 8
  call void @memrefCopy(i64 4, ptr %1357, ptr %1358)
  call void @llvm.stackrestore.p0(ptr %1352)
  %1359 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %1360 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %1361 = insertvalue { ptr, ptr, i64 } poison, ptr %1359, 0
  %1362 = insertvalue { ptr, ptr, i64 } %1361, ptr %1360, 1
  %1363 = insertvalue { ptr, ptr, i64 } %1362, i64 0, 2
  %1364 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %1365 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %1366 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %1367 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %1368 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %1369 = extractvalue { ptr, ptr, i64 } %1363, 0
  %1370 = extractvalue { ptr, ptr, i64 } %1363, 1
  %1371 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1369, 0
  %1372 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1371, ptr %1370, 1
  %1373 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1372, i64 560, 2
  %1374 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1373, i64 8, 3, 0
  %1375 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1374, i64 64, 4, 0
  %1376 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1375, i64 8, 3, 1
  %1377 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1376, i64 1, 4, 1
  %1378 = call ptr @llvm.stacksave.p0()
  %1379 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1377, ptr %1379, align 8
  %1380 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1379, 1
  %1381 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %1381, align 8
  %1382 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1381, 1
  %1383 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1380, ptr %1383, align 8
  %1384 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1382, ptr %1384, align 8
  call void @memrefCopy(i64 4, ptr %1383, ptr %1384)
  call void @llvm.stackrestore.p0(ptr %1378)
  %1385 = call ptr @llvm.stacksave.p0()
  %1386 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %987, ptr %1386, align 8
  %1387 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1386, 1
  %1388 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %1388, align 8
  %1389 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1388, 1
  %1390 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1387, ptr %1390, align 8
  %1391 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1389, ptr %1391, align 8
  call void @memrefCopy(i64 4, ptr %1390, ptr %1391)
  call void @llvm.stackrestore.p0(ptr %1385)
  %1392 = call ptr @llvm.stacksave.p0()
  %1393 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %792, ptr %1393, align 8
  %1394 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1393, 1
  %1395 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %1395, align 8
  %1396 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1395, 1
  %1397 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1394, ptr %1397, align 8
  %1398 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1396, ptr %1398, align 8
  call void @memrefCopy(i64 4, ptr %1397, ptr %1398)
  call void @llvm.stackrestore.p0(ptr %1392)
  %1399 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 1922, i32 125)
  %1400 = call ptr @llvm.stacksave.p0()
  %1401 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1021, ptr %1401, align 8
  %1402 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1401, 1
  %1403 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %1403, align 8
  %1404 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1403, 1
  %1405 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1402, ptr %1405, align 8
  %1406 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1404, ptr %1406, align 8
  call void @memrefCopy(i64 4, ptr %1405, ptr %1406)
  call void @llvm.stackrestore.p0(ptr %1400)
  %1407 = call ptr @llvm.stacksave.p0()
  %1408 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %826, ptr %1408, align 8
  %1409 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1408, 1
  %1410 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %1410, align 8
  %1411 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1410, 1
  %1412 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1409, ptr %1412, align 8
  %1413 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1411, ptr %1413, align 8
  call void @memrefCopy(i64 4, ptr %1412, ptr %1413)
  call void @llvm.stackrestore.p0(ptr %1407)
  %1414 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 1991, i32 125)
  %1415 = call ptr @llvm.stacksave.p0()
  %1416 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %1416, align 8
  %1417 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1416, 1
  %1418 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1377, ptr %1418, align 8
  %1419 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1418, 1
  %1420 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1417, ptr %1420, align 8
  %1421 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1419, ptr %1421, align 8
  call void @memrefCopy(i64 4, ptr %1420, ptr %1421)
  call void @llvm.stackrestore.p0(ptr %1415)
  %1422 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %1423 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %1424 = insertvalue { ptr, ptr, i64 } poison, ptr %1422, 0
  %1425 = insertvalue { ptr, ptr, i64 } %1424, ptr %1423, 1
  %1426 = insertvalue { ptr, ptr, i64 } %1425, i64 0, 2
  %1427 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %1428 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %1429 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %1430 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %1431 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %1432 = extractvalue { ptr, ptr, i64 } %1426, 0
  %1433 = extractvalue { ptr, ptr, i64 } %1426, 1
  %1434 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1432, 0
  %1435 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1434, ptr %1433, 1
  %1436 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1435, i64 568, 2
  %1437 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1436, i64 8, 3, 0
  %1438 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1437, i64 64, 4, 0
  %1439 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1438, i64 8, 3, 1
  %1440 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1439, i64 1, 4, 1
  %1441 = call ptr @llvm.stacksave.p0()
  %1442 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1440, ptr %1442, align 8
  %1443 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1442, 1
  %1444 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %1444, align 8
  %1445 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1444, 1
  %1446 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1443, ptr %1446, align 8
  %1447 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1445, ptr %1447, align 8
  call void @memrefCopy(i64 4, ptr %1446, ptr %1447)
  call void @llvm.stackrestore.p0(ptr %1441)
  %1448 = call ptr @llvm.stacksave.p0()
  %1449 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %987, ptr %1449, align 8
  %1450 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1449, 1
  %1451 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %1451, align 8
  %1452 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1451, 1
  %1453 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1450, ptr %1453, align 8
  %1454 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1452, ptr %1454, align 8
  call void @memrefCopy(i64 4, ptr %1453, ptr %1454)
  call void @llvm.stackrestore.p0(ptr %1448)
  %1455 = call ptr @llvm.stacksave.p0()
  %1456 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %893, ptr %1456, align 8
  %1457 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1456, 1
  %1458 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %1458, align 8
  %1459 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1458, 1
  %1460 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1457, ptr %1460, align 8
  %1461 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1459, ptr %1461, align 8
  call void @memrefCopy(i64 4, ptr %1460, ptr %1461)
  call void @llvm.stackrestore.p0(ptr %1455)
  %1462 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 2060, i32 125)
  %1463 = call ptr @llvm.stacksave.p0()
  %1464 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1021, ptr %1464, align 8
  %1465 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1464, 1
  %1466 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %1466, align 8
  %1467 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1466, 1
  %1468 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1465, ptr %1468, align 8
  %1469 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1467, ptr %1469, align 8
  call void @memrefCopy(i64 4, ptr %1468, ptr %1469)
  call void @llvm.stackrestore.p0(ptr %1463)
  %1470 = call ptr @llvm.stacksave.p0()
  %1471 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %927, ptr %1471, align 8
  %1472 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1471, 1
  %1473 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %1473, align 8
  %1474 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1473, 1
  %1475 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1472, ptr %1475, align 8
  %1476 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1474, ptr %1476, align 8
  call void @memrefCopy(i64 4, ptr %1475, ptr %1476)
  call void @llvm.stackrestore.p0(ptr %1470)
  %1477 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 2129, i32 125)
  %1478 = call ptr @llvm.stacksave.p0()
  %1479 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %1479, align 8
  %1480 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1479, 1
  %1481 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1440, ptr %1481, align 8
  %1482 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1481, 1
  %1483 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1480, ptr %1483, align 8
  %1484 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1482, ptr %1484, align 8
  call void @memrefCopy(i64 4, ptr %1483, ptr %1484)
  call void @llvm.stackrestore.p0(ptr %1478)
  %1485 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %1486 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %1487 = insertvalue { ptr, ptr, i64 } poison, ptr %1485, 0
  %1488 = insertvalue { ptr, ptr, i64 } %1487, ptr %1486, 1
  %1489 = insertvalue { ptr, ptr, i64 } %1488, i64 0, 2
  %1490 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %1491 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %1492 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %1493 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %1494 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %1495 = extractvalue { ptr, ptr, i64 } %1489, 0
  %1496 = extractvalue { ptr, ptr, i64 } %1489, 1
  %1497 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1495, 0
  %1498 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1497, ptr %1496, 1
  %1499 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1498, i64 1024, 2
  %1500 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1499, i64 8, 3, 0
  %1501 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1500, i64 64, 4, 0
  %1502 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1501, i64 8, 3, 1
  %1503 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1502, i64 1, 4, 1
  %1504 = call ptr @llvm.stacksave.p0()
  %1505 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1503, ptr %1505, align 8
  %1506 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1505, 1
  %1507 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %1507, align 8
  %1508 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1507, 1
  %1509 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1506, ptr %1509, align 8
  %1510 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1508, ptr %1510, align 8
  call void @memrefCopy(i64 4, ptr %1509, ptr %1510)
  call void @llvm.stackrestore.p0(ptr %1504)
  %1511 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %1512 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %1513 = insertvalue { ptr, ptr, i64 } poison, ptr %1511, 0
  %1514 = insertvalue { ptr, ptr, i64 } %1513, ptr %1512, 1
  %1515 = insertvalue { ptr, ptr, i64 } %1514, i64 0, 2
  %1516 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %1517 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %1518 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %1519 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %1520 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %1521 = extractvalue { ptr, ptr, i64 } %1515, 0
  %1522 = extractvalue { ptr, ptr, i64 } %1515, 1
  %1523 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1521, 0
  %1524 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1523, ptr %1522, 1
  %1525 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1524, i64 512, 2
  %1526 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1525, i64 8, 3, 0
  %1527 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1526, i64 32, 4, 0
  %1528 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1527, i64 16, 3, 1
  %1529 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1528, i64 1, 4, 1
  %1530 = call ptr @llvm.stacksave.p0()
  %1531 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1529, ptr %1531, align 8
  %1532 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1531, 1
  %1533 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %1533, align 8
  %1534 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1533, 1
  %1535 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1532, ptr %1535, align 8
  %1536 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1534, ptr %1536, align 8
  call void @memrefCopy(i64 4, ptr %1535, ptr %1536)
  call void @llvm.stackrestore.p0(ptr %1530)
  %1537 = call ptr @llvm.stacksave.p0()
  %1538 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %155, ptr %1538, align 8
  %1539 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1538, 1
  %1540 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %1540, align 8
  %1541 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1540, 1
  %1542 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1539, ptr %1542, align 8
  %1543 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1541, ptr %1543, align 8
  call void @memrefCopy(i64 4, ptr %1542, ptr %1543)
  call void @llvm.stackrestore.p0(ptr %1537)
  %1544 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 2198, i32 125)
  %1545 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %1546 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %1547 = insertvalue { ptr, ptr, i64 } poison, ptr %1545, 0
  %1548 = insertvalue { ptr, ptr, i64 } %1547, ptr %1546, 1
  %1549 = insertvalue { ptr, ptr, i64 } %1548, i64 0, 2
  %1550 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %1551 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %1552 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %1553 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %1554 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %1555 = extractvalue { ptr, ptr, i64 } %1549, 0
  %1556 = extractvalue { ptr, ptr, i64 } %1549, 1
  %1557 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1555, 0
  %1558 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1557, ptr %1556, 1
  %1559 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1558, i64 528, 2
  %1560 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1559, i64 8, 3, 0
  %1561 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1560, i64 32, 4, 0
  %1562 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1561, i64 16, 3, 1
  %1563 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1562, i64 1, 4, 1
  %1564 = call ptr @llvm.stacksave.p0()
  %1565 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1563, ptr %1565, align 8
  %1566 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1565, 1
  %1567 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %1567, align 8
  %1568 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1567, 1
  %1569 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1566, ptr %1569, align 8
  %1570 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1568, ptr %1570, align 8
  call void @memrefCopy(i64 4, ptr %1569, ptr %1570)
  call void @llvm.stackrestore.p0(ptr %1564)
  %1571 = call ptr @llvm.stacksave.p0()
  %1572 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %220, ptr %1572, align 8
  %1573 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1572, 1
  %1574 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %1574, align 8
  %1575 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1574, 1
  %1576 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1573, ptr %1576, align 8
  %1577 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1575, ptr %1577, align 8
  call void @memrefCopy(i64 4, ptr %1576, ptr %1577)
  call void @llvm.stackrestore.p0(ptr %1571)
  %1578 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 2267, i32 125)
  %1579 = call ptr @llvm.stacksave.p0()
  %1580 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %1580, align 8
  %1581 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1580, 1
  %1582 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1503, ptr %1582, align 8
  %1583 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1582, 1
  %1584 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1581, ptr %1584, align 8
  %1585 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1583, ptr %1585, align 8
  call void @memrefCopy(i64 4, ptr %1584, ptr %1585)
  call void @llvm.stackrestore.p0(ptr %1579)
  %1586 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %1587 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %1588 = insertvalue { ptr, ptr, i64 } poison, ptr %1586, 0
  %1589 = insertvalue { ptr, ptr, i64 } %1588, ptr %1587, 1
  %1590 = insertvalue { ptr, ptr, i64 } %1589, i64 0, 2
  %1591 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %1592 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %1593 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %1594 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %1595 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %1596 = extractvalue { ptr, ptr, i64 } %1590, 0
  %1597 = extractvalue { ptr, ptr, i64 } %1590, 1
  %1598 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1596, 0
  %1599 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1598, ptr %1597, 1
  %1600 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1599, i64 1032, 2
  %1601 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1600, i64 8, 3, 0
  %1602 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1601, i64 64, 4, 0
  %1603 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1602, i64 8, 3, 1
  %1604 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1603, i64 1, 4, 1
  %1605 = call ptr @llvm.stacksave.p0()
  %1606 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1604, ptr %1606, align 8
  %1607 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1606, 1
  %1608 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %1608, align 8
  %1609 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1608, 1
  %1610 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1607, ptr %1610, align 8
  %1611 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1609, ptr %1611, align 8
  call void @memrefCopy(i64 4, ptr %1610, ptr %1611)
  call void @llvm.stackrestore.p0(ptr %1605)
  %1612 = call ptr @llvm.stacksave.p0()
  %1613 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1529, ptr %1613, align 8
  %1614 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1613, 1
  %1615 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %1615, align 8
  %1616 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1615, 1
  %1617 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1614, ptr %1617, align 8
  %1618 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1616, ptr %1618, align 8
  call void @memrefCopy(i64 4, ptr %1617, ptr %1618)
  call void @llvm.stackrestore.p0(ptr %1612)
  %1619 = call ptr @llvm.stacksave.p0()
  %1620 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %287, ptr %1620, align 8
  %1621 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1620, 1
  %1622 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %1622, align 8
  %1623 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1622, 1
  %1624 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1621, ptr %1624, align 8
  %1625 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1623, ptr %1625, align 8
  call void @memrefCopy(i64 4, ptr %1624, ptr %1625)
  call void @llvm.stackrestore.p0(ptr %1619)
  %1626 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 2336, i32 125)
  %1627 = call ptr @llvm.stacksave.p0()
  %1628 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1563, ptr %1628, align 8
  %1629 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1628, 1
  %1630 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %1630, align 8
  %1631 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1630, 1
  %1632 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1629, ptr %1632, align 8
  %1633 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1631, ptr %1633, align 8
  call void @memrefCopy(i64 4, ptr %1632, ptr %1633)
  call void @llvm.stackrestore.p0(ptr %1627)
  %1634 = call ptr @llvm.stacksave.p0()
  %1635 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %321, ptr %1635, align 8
  %1636 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1635, 1
  %1637 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %1637, align 8
  %1638 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1637, 1
  %1639 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1636, ptr %1639, align 8
  %1640 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1638, ptr %1640, align 8
  call void @memrefCopy(i64 4, ptr %1639, ptr %1640)
  call void @llvm.stackrestore.p0(ptr %1634)
  %1641 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 2405, i32 125)
  %1642 = call ptr @llvm.stacksave.p0()
  %1643 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %1643, align 8
  %1644 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1643, 1
  %1645 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1604, ptr %1645, align 8
  %1646 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1645, 1
  %1647 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1644, ptr %1647, align 8
  %1648 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1646, ptr %1648, align 8
  call void @memrefCopy(i64 4, ptr %1647, ptr %1648)
  call void @llvm.stackrestore.p0(ptr %1642)
  %1649 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %1650 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %1651 = insertvalue { ptr, ptr, i64 } poison, ptr %1649, 0
  %1652 = insertvalue { ptr, ptr, i64 } %1651, ptr %1650, 1
  %1653 = insertvalue { ptr, ptr, i64 } %1652, i64 0, 2
  %1654 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %1655 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %1656 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %1657 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %1658 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %1659 = extractvalue { ptr, ptr, i64 } %1653, 0
  %1660 = extractvalue { ptr, ptr, i64 } %1653, 1
  %1661 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1659, 0
  %1662 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1661, ptr %1660, 1
  %1663 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1662, i64 1040, 2
  %1664 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1663, i64 8, 3, 0
  %1665 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1664, i64 64, 4, 0
  %1666 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1665, i64 8, 3, 1
  %1667 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1666, i64 1, 4, 1
  %1668 = call ptr @llvm.stacksave.p0()
  %1669 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1667, ptr %1669, align 8
  %1670 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1669, 1
  %1671 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %1671, align 8
  %1672 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1671, 1
  %1673 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1670, ptr %1673, align 8
  %1674 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1672, ptr %1674, align 8
  call void @memrefCopy(i64 4, ptr %1673, ptr %1674)
  call void @llvm.stackrestore.p0(ptr %1668)
  %1675 = call ptr @llvm.stacksave.p0()
  %1676 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1529, ptr %1676, align 8
  %1677 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1676, 1
  %1678 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %1678, align 8
  %1679 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1678, 1
  %1680 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1677, ptr %1680, align 8
  %1681 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1679, ptr %1681, align 8
  call void @memrefCopy(i64 4, ptr %1680, ptr %1681)
  call void @llvm.stackrestore.p0(ptr %1675)
  %1682 = call ptr @llvm.stacksave.p0()
  %1683 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %388, ptr %1683, align 8
  %1684 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1683, 1
  %1685 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %1685, align 8
  %1686 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1685, 1
  %1687 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1684, ptr %1687, align 8
  %1688 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1686, ptr %1688, align 8
  call void @memrefCopy(i64 4, ptr %1687, ptr %1688)
  call void @llvm.stackrestore.p0(ptr %1682)
  %1689 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 2474, i32 125)
  %1690 = call ptr @llvm.stacksave.p0()
  %1691 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1563, ptr %1691, align 8
  %1692 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1691, 1
  %1693 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %1693, align 8
  %1694 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1693, 1
  %1695 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1692, ptr %1695, align 8
  %1696 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1694, ptr %1696, align 8
  call void @memrefCopy(i64 4, ptr %1695, ptr %1696)
  call void @llvm.stackrestore.p0(ptr %1690)
  %1697 = call ptr @llvm.stacksave.p0()
  %1698 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %422, ptr %1698, align 8
  %1699 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1698, 1
  %1700 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %1700, align 8
  %1701 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1700, 1
  %1702 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1699, ptr %1702, align 8
  %1703 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1701, ptr %1703, align 8
  call void @memrefCopy(i64 4, ptr %1702, ptr %1703)
  call void @llvm.stackrestore.p0(ptr %1697)
  %1704 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 2543, i32 125)
  %1705 = call ptr @llvm.stacksave.p0()
  %1706 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %1706, align 8
  %1707 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1706, 1
  %1708 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1667, ptr %1708, align 8
  %1709 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1708, 1
  %1710 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1707, ptr %1710, align 8
  %1711 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1709, ptr %1711, align 8
  call void @memrefCopy(i64 4, ptr %1710, ptr %1711)
  call void @llvm.stackrestore.p0(ptr %1705)
  %1712 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %1713 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %1714 = insertvalue { ptr, ptr, i64 } poison, ptr %1712, 0
  %1715 = insertvalue { ptr, ptr, i64 } %1714, ptr %1713, 1
  %1716 = insertvalue { ptr, ptr, i64 } %1715, i64 0, 2
  %1717 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %1718 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %1719 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %1720 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %1721 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %1722 = extractvalue { ptr, ptr, i64 } %1716, 0
  %1723 = extractvalue { ptr, ptr, i64 } %1716, 1
  %1724 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1722, 0
  %1725 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1724, ptr %1723, 1
  %1726 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1725, i64 1048, 2
  %1727 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1726, i64 4, 3, 0
  %1728 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1727, i64 64, 4, 0
  %1729 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1728, i64 4, 3, 1
  %1730 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1729, i64 1, 4, 1
  %1731 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 0
  %1732 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 1
  %1733 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1731, 0
  %1734 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1733, ptr %1732, 1
  %1735 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1734, i64 0, 2
  %1736 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1735, i64 4, 3, 0
  %1737 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1736, i64 4, 4, 0
  %1738 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1737, i64 4, 3, 1
  %1739 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1738, i64 1, 4, 1
  %1740 = call ptr @llvm.stacksave.p0()
  %1741 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1730, ptr %1741, align 8
  %1742 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1741, 1
  %1743 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %1743, align 8
  %1744 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1743, 1
  %1745 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1742, ptr %1745, align 8
  %1746 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1744, ptr %1746, align 8
  call void @memrefCopy(i64 4, ptr %1745, ptr %1746)
  call void @llvm.stackrestore.p0(ptr %1740)
  %1747 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 1
  %1748 = ptrtoint ptr %1747 to i64
  %1749 = inttoptr i64 %1748 to ptr
  %1750 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %1751 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %1752 = insertvalue { ptr, ptr, i64 } poison, ptr %1750, 0
  %1753 = insertvalue { ptr, ptr, i64 } %1752, ptr %1751, 1
  %1754 = insertvalue { ptr, ptr, i64 } %1753, i64 0, 2
  %1755 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %1756 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %1757 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %1758 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %1759 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %1760 = extractvalue { ptr, ptr, i64 } %1754, 0
  %1761 = extractvalue { ptr, ptr, i64 } %1754, 1
  %1762 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1760, 0
  %1763 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1762, ptr %1761, 1
  %1764 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1763, i64 512, 2
  %1765 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1764, i64 4, 3, 0
  %1766 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1765, i64 32, 4, 0
  %1767 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1766, i64 16, 3, 1
  %1768 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1767, i64 1, 4, 1
  %1769 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 0
  %1770 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %1771 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1769, 0
  %1772 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1771, ptr %1770, 1
  %1773 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1772, i64 0, 2
  %1774 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1773, i64 4, 3, 0
  %1775 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1774, i64 16, 4, 0
  %1776 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1775, i64 16, 3, 1
  %1777 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1776, i64 1, 4, 1
  %1778 = call ptr @llvm.stacksave.p0()
  %1779 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1768, ptr %1779, align 8
  %1780 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1779, 1
  %1781 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %1781, align 8
  %1782 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1781, 1
  %1783 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1780, ptr %1783, align 8
  %1784 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1782, ptr %1784, align 8
  call void @memrefCopy(i64 4, ptr %1783, ptr %1784)
  call void @llvm.stackrestore.p0(ptr %1778)
  %1785 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %1786 = ptrtoint ptr %1785 to i64
  %1787 = inttoptr i64 %1786 to ptr
  %1788 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %1789 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %1790 = insertvalue { ptr, ptr, i64 } poison, ptr %1788, 0
  %1791 = insertvalue { ptr, ptr, i64 } %1790, ptr %1789, 1
  %1792 = insertvalue { ptr, ptr, i64 } %1791, i64 0, 2
  %1793 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %1794 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %1795 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %1796 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %1797 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %1798 = extractvalue { ptr, ptr, i64 } %1792, 0
  %1799 = extractvalue { ptr, ptr, i64 } %1792, 1
  %1800 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1798, 0
  %1801 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1800, ptr %1799, 1
  %1802 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1801, i64 24, 2
  %1803 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1802, i64 16, 3, 0
  %1804 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1803, i64 64, 4, 0
  %1805 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1804, i64 4, 3, 1
  %1806 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1805, i64 1, 4, 1
  %1807 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 0
  %1808 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %1809 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1807, 0
  %1810 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1809, ptr %1808, 1
  %1811 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1810, i64 0, 2
  %1812 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1811, i64 16, 3, 0
  %1813 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1812, i64 4, 4, 0
  %1814 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1813, i64 4, 3, 1
  %1815 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1814, i64 1, 4, 1
  %1816 = call ptr @llvm.stacksave.p0()
  %1817 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1806, ptr %1817, align 8
  %1818 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1817, 1
  %1819 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %1819, align 8
  %1820 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1819, 1
  %1821 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1818, ptr %1821, align 8
  %1822 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1820, ptr %1822, align 8
  call void @memrefCopy(i64 4, ptr %1821, ptr %1822)
  call void @llvm.stackrestore.p0(ptr %1816)
  %1823 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %1824 = ptrtoint ptr %1823 to i64
  %1825 = inttoptr i64 %1824 to ptr
  %1826 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 1, i32 0, i32 117)
  %1827 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %1828 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %1829 = insertvalue { ptr, ptr, i64 } poison, ptr %1827, 0
  %1830 = insertvalue { ptr, ptr, i64 } %1829, ptr %1828, 1
  %1831 = insertvalue { ptr, ptr, i64 } %1830, i64 0, 2
  %1832 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %1833 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %1834 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %1835 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %1836 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %1837 = extractvalue { ptr, ptr, i64 } %1831, 0
  %1838 = extractvalue { ptr, ptr, i64 } %1831, 1
  %1839 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1837, 0
  %1840 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1839, ptr %1838, 1
  %1841 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1840, i64 528, 2
  %1842 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1841, i64 4, 3, 0
  %1843 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1842, i64 32, 4, 0
  %1844 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1843, i64 16, 3, 1
  %1845 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1844, i64 1, 4, 1
  %1846 = call ptr @llvm.stacksave.p0()
  %1847 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1845, ptr %1847, align 8
  %1848 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1847, 1
  %1849 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %1849, align 8
  %1850 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1849, 1
  %1851 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1848, ptr %1851, align 8
  %1852 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1850, ptr %1852, align 8
  call void @memrefCopy(i64 4, ptr %1851, ptr %1852)
  call void @llvm.stackrestore.p0(ptr %1846)
  %1853 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %1854 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %1855 = insertvalue { ptr, ptr, i64 } poison, ptr %1853, 0
  %1856 = insertvalue { ptr, ptr, i64 } %1855, ptr %1854, 1
  %1857 = insertvalue { ptr, ptr, i64 } %1856, i64 0, 2
  %1858 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %1859 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %1860 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %1861 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %1862 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %1863 = extractvalue { ptr, ptr, i64 } %1857, 0
  %1864 = extractvalue { ptr, ptr, i64 } %1857, 1
  %1865 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1863, 0
  %1866 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1865, ptr %1864, 1
  %1867 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1866, i64 1048, 2
  %1868 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1867, i64 16, 3, 0
  %1869 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1868, i64 64, 4, 0
  %1870 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1869, i64 4, 3, 1
  %1871 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1870, i64 1, 4, 1
  %1872 = call ptr @llvm.stacksave.p0()
  %1873 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1871, ptr %1873, align 8
  %1874 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1873, 1
  %1875 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %1875, align 8
  %1876 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1875, 1
  %1877 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1874, ptr %1877, align 8
  %1878 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1876, ptr %1878, align 8
  call void @memrefCopy(i64 4, ptr %1877, ptr %1878)
  call void @llvm.stackrestore.p0(ptr %1872)
  %1879 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 1, i32 42, i32 117)
  %1880 = call ptr @llvm.stacksave.p0()
  %1881 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %1881, align 8
  %1882 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1881, 1
  %1883 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1730, ptr %1883, align 8
  %1884 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1883, 1
  %1885 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1882, ptr %1885, align 8
  %1886 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1884, ptr %1886, align 8
  call void @memrefCopy(i64 4, ptr %1885, ptr %1886)
  call void @llvm.stackrestore.p0(ptr %1880)
  %1887 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %1888 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %1889 = insertvalue { ptr, ptr, i64 } poison, ptr %1887, 0
  %1890 = insertvalue { ptr, ptr, i64 } %1889, ptr %1888, 1
  %1891 = insertvalue { ptr, ptr, i64 } %1890, i64 0, 2
  %1892 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %1893 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %1894 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %1895 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %1896 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %1897 = extractvalue { ptr, ptr, i64 } %1891, 0
  %1898 = extractvalue { ptr, ptr, i64 } %1891, 1
  %1899 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1897, 0
  %1900 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1899, ptr %1898, 1
  %1901 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1900, i64 1052, 2
  %1902 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1901, i64 4, 3, 0
  %1903 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1902, i64 64, 4, 0
  %1904 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1903, i64 4, 3, 1
  %1905 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1904, i64 1, 4, 1
  %1906 = call ptr @llvm.stacksave.p0()
  %1907 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1905, ptr %1907, align 8
  %1908 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1907, 1
  %1909 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %1909, align 8
  %1910 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1909, 1
  %1911 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1908, ptr %1911, align 8
  %1912 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1910, ptr %1912, align 8
  call void @memrefCopy(i64 4, ptr %1911, ptr %1912)
  call void @llvm.stackrestore.p0(ptr %1906)
  %1913 = call ptr @llvm.stacksave.p0()
  %1914 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1768, ptr %1914, align 8
  %1915 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1914, 1
  %1916 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %1916, align 8
  %1917 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1916, 1
  %1918 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1915, ptr %1918, align 8
  %1919 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1917, ptr %1919, align 8
  call void @memrefCopy(i64 4, ptr %1918, ptr %1919)
  call void @llvm.stackrestore.p0(ptr %1913)
  %1920 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %1921 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %1922 = insertvalue { ptr, ptr, i64 } poison, ptr %1920, 0
  %1923 = insertvalue { ptr, ptr, i64 } %1922, ptr %1921, 1
  %1924 = insertvalue { ptr, ptr, i64 } %1923, i64 0, 2
  %1925 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %1926 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %1927 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %1928 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %1929 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %1930 = extractvalue { ptr, ptr, i64 } %1924, 0
  %1931 = extractvalue { ptr, ptr, i64 } %1924, 1
  %1932 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1930, 0
  %1933 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1932, ptr %1931, 1
  %1934 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1933, i64 28, 2
  %1935 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1934, i64 16, 3, 0
  %1936 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1935, i64 64, 4, 0
  %1937 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1936, i64 4, 3, 1
  %1938 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1937, i64 1, 4, 1
  %1939 = call ptr @llvm.stacksave.p0()
  %1940 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1938, ptr %1940, align 8
  %1941 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1940, 1
  %1942 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %1942, align 8
  %1943 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1942, 1
  %1944 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1941, ptr %1944, align 8
  %1945 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1943, ptr %1945, align 8
  call void @memrefCopy(i64 4, ptr %1944, ptr %1945)
  call void @llvm.stackrestore.p0(ptr %1939)
  %1946 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 2, i32 0, i32 117)
  %1947 = call ptr @llvm.stacksave.p0()
  %1948 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1845, ptr %1948, align 8
  %1949 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1948, 1
  %1950 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %1950, align 8
  %1951 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1950, 1
  %1952 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1949, ptr %1952, align 8
  %1953 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1951, ptr %1953, align 8
  call void @memrefCopy(i64 4, ptr %1952, ptr %1953)
  call void @llvm.stackrestore.p0(ptr %1947)
  %1954 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %1955 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %1956 = insertvalue { ptr, ptr, i64 } poison, ptr %1954, 0
  %1957 = insertvalue { ptr, ptr, i64 } %1956, ptr %1955, 1
  %1958 = insertvalue { ptr, ptr, i64 } %1957, i64 0, 2
  %1959 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %1960 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %1961 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %1962 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %1963 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %1964 = extractvalue { ptr, ptr, i64 } %1958, 0
  %1965 = extractvalue { ptr, ptr, i64 } %1958, 1
  %1966 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1964, 0
  %1967 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1966, ptr %1965, 1
  %1968 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1967, i64 1052, 2
  %1969 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1968, i64 16, 3, 0
  %1970 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1969, i64 64, 4, 0
  %1971 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1970, i64 4, 3, 1
  %1972 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1971, i64 1, 4, 1
  %1973 = call ptr @llvm.stacksave.p0()
  %1974 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1972, ptr %1974, align 8
  %1975 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1974, 1
  %1976 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %1976, align 8
  %1977 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1976, 1
  %1978 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1975, ptr %1978, align 8
  %1979 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1977, ptr %1979, align 8
  call void @memrefCopy(i64 4, ptr %1978, ptr %1979)
  call void @llvm.stackrestore.p0(ptr %1973)
  %1980 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 2, i32 42, i32 117)
  %1981 = call ptr @llvm.stacksave.p0()
  %1982 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %1982, align 8
  %1983 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1982, 1
  %1984 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1905, ptr %1984, align 8
  %1985 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1984, 1
  %1986 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1983, ptr %1986, align 8
  %1987 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1985, ptr %1987, align 8
  call void @memrefCopy(i64 4, ptr %1986, ptr %1987)
  call void @llvm.stackrestore.p0(ptr %1981)
  %1988 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %1989 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %1990 = insertvalue { ptr, ptr, i64 } poison, ptr %1988, 0
  %1991 = insertvalue { ptr, ptr, i64 } %1990, ptr %1989, 1
  %1992 = insertvalue { ptr, ptr, i64 } %1991, i64 0, 2
  %1993 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %1994 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %1995 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %1996 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %1997 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %1998 = extractvalue { ptr, ptr, i64 } %1992, 0
  %1999 = extractvalue { ptr, ptr, i64 } %1992, 1
  %2000 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1998, 0
  %2001 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2000, ptr %1999, 1
  %2002 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2001, i64 1056, 2
  %2003 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2002, i64 4, 3, 0
  %2004 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2003, i64 64, 4, 0
  %2005 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2004, i64 4, 3, 1
  %2006 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2005, i64 1, 4, 1
  %2007 = call ptr @llvm.stacksave.p0()
  %2008 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2006, ptr %2008, align 8
  %2009 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2008, 1
  %2010 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %2010, align 8
  %2011 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2010, 1
  %2012 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2009, ptr %2012, align 8
  %2013 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2011, ptr %2013, align 8
  call void @memrefCopy(i64 4, ptr %2012, ptr %2013)
  call void @llvm.stackrestore.p0(ptr %2007)
  %2014 = call ptr @llvm.stacksave.p0()
  %2015 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1768, ptr %2015, align 8
  %2016 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2015, 1
  %2017 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %2017, align 8
  %2018 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2017, 1
  %2019 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2016, ptr %2019, align 8
  %2020 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2018, ptr %2020, align 8
  call void @memrefCopy(i64 4, ptr %2019, ptr %2020)
  call void @llvm.stackrestore.p0(ptr %2014)
  %2021 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %2022 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %2023 = insertvalue { ptr, ptr, i64 } poison, ptr %2021, 0
  %2024 = insertvalue { ptr, ptr, i64 } %2023, ptr %2022, 1
  %2025 = insertvalue { ptr, ptr, i64 } %2024, i64 0, 2
  %2026 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %2027 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %2028 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %2029 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %2030 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %2031 = extractvalue { ptr, ptr, i64 } %2025, 0
  %2032 = extractvalue { ptr, ptr, i64 } %2025, 1
  %2033 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2031, 0
  %2034 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2033, ptr %2032, 1
  %2035 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2034, i64 32, 2
  %2036 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2035, i64 16, 3, 0
  %2037 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2036, i64 64, 4, 0
  %2038 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2037, i64 4, 3, 1
  %2039 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2038, i64 1, 4, 1
  %2040 = call ptr @llvm.stacksave.p0()
  %2041 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2039, ptr %2041, align 8
  %2042 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2041, 1
  %2043 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %2043, align 8
  %2044 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2043, 1
  %2045 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2042, ptr %2045, align 8
  %2046 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2044, ptr %2046, align 8
  call void @memrefCopy(i64 4, ptr %2045, ptr %2046)
  call void @llvm.stackrestore.p0(ptr %2040)
  %2047 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 3, i32 0, i32 117)
  %2048 = call ptr @llvm.stacksave.p0()
  %2049 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1845, ptr %2049, align 8
  %2050 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2049, 1
  %2051 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %2051, align 8
  %2052 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2051, 1
  %2053 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2050, ptr %2053, align 8
  %2054 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2052, ptr %2054, align 8
  call void @memrefCopy(i64 4, ptr %2053, ptr %2054)
  call void @llvm.stackrestore.p0(ptr %2048)
  %2055 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %2056 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %2057 = insertvalue { ptr, ptr, i64 } poison, ptr %2055, 0
  %2058 = insertvalue { ptr, ptr, i64 } %2057, ptr %2056, 1
  %2059 = insertvalue { ptr, ptr, i64 } %2058, i64 0, 2
  %2060 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %2061 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %2062 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %2063 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %2064 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %2065 = extractvalue { ptr, ptr, i64 } %2059, 0
  %2066 = extractvalue { ptr, ptr, i64 } %2059, 1
  %2067 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2065, 0
  %2068 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2067, ptr %2066, 1
  %2069 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2068, i64 1056, 2
  %2070 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2069, i64 16, 3, 0
  %2071 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2070, i64 64, 4, 0
  %2072 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2071, i64 4, 3, 1
  %2073 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2072, i64 1, 4, 1
  %2074 = call ptr @llvm.stacksave.p0()
  %2075 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2073, ptr %2075, align 8
  %2076 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2075, 1
  %2077 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %2077, align 8
  %2078 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2077, 1
  %2079 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2076, ptr %2079, align 8
  %2080 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2078, ptr %2080, align 8
  call void @memrefCopy(i64 4, ptr %2079, ptr %2080)
  call void @llvm.stackrestore.p0(ptr %2074)
  %2081 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 3, i32 42, i32 117)
  %2082 = call ptr @llvm.stacksave.p0()
  %2083 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %2083, align 8
  %2084 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2083, 1
  %2085 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2006, ptr %2085, align 8
  %2086 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2085, 1
  %2087 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2084, ptr %2087, align 8
  %2088 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2086, ptr %2088, align 8
  call void @memrefCopy(i64 4, ptr %2087, ptr %2088)
  call void @llvm.stackrestore.p0(ptr %2082)
  %2089 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %2090 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %2091 = insertvalue { ptr, ptr, i64 } poison, ptr %2089, 0
  %2092 = insertvalue { ptr, ptr, i64 } %2091, ptr %2090, 1
  %2093 = insertvalue { ptr, ptr, i64 } %2092, i64 0, 2
  %2094 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %2095 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %2096 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %2097 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %2098 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %2099 = extractvalue { ptr, ptr, i64 } %2093, 0
  %2100 = extractvalue { ptr, ptr, i64 } %2093, 1
  %2101 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2099, 0
  %2102 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2101, ptr %2100, 1
  %2103 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2102, i64 1060, 2
  %2104 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2103, i64 4, 3, 0
  %2105 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2104, i64 64, 4, 0
  %2106 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2105, i64 4, 3, 1
  %2107 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2106, i64 1, 4, 1
  %2108 = call ptr @llvm.stacksave.p0()
  %2109 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2107, ptr %2109, align 8
  %2110 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2109, 1
  %2111 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %2111, align 8
  %2112 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2111, 1
  %2113 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2110, ptr %2113, align 8
  %2114 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2112, ptr %2114, align 8
  call void @memrefCopy(i64 4, ptr %2113, ptr %2114)
  call void @llvm.stackrestore.p0(ptr %2108)
  %2115 = call ptr @llvm.stacksave.p0()
  %2116 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1768, ptr %2116, align 8
  %2117 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2116, 1
  %2118 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %2118, align 8
  %2119 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2118, 1
  %2120 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2117, ptr %2120, align 8
  %2121 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2119, ptr %2121, align 8
  call void @memrefCopy(i64 4, ptr %2120, ptr %2121)
  call void @llvm.stackrestore.p0(ptr %2115)
  %2122 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %2123 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %2124 = insertvalue { ptr, ptr, i64 } poison, ptr %2122, 0
  %2125 = insertvalue { ptr, ptr, i64 } %2124, ptr %2123, 1
  %2126 = insertvalue { ptr, ptr, i64 } %2125, i64 0, 2
  %2127 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %2128 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %2129 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %2130 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %2131 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %2132 = extractvalue { ptr, ptr, i64 } %2126, 0
  %2133 = extractvalue { ptr, ptr, i64 } %2126, 1
  %2134 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2132, 0
  %2135 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2134, ptr %2133, 1
  %2136 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2135, i64 36, 2
  %2137 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2136, i64 16, 3, 0
  %2138 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2137, i64 64, 4, 0
  %2139 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2138, i64 4, 3, 1
  %2140 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2139, i64 1, 4, 1
  %2141 = call ptr @llvm.stacksave.p0()
  %2142 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2140, ptr %2142, align 8
  %2143 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2142, 1
  %2144 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %2144, align 8
  %2145 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2144, 1
  %2146 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2143, ptr %2146, align 8
  %2147 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2145, ptr %2147, align 8
  call void @memrefCopy(i64 4, ptr %2146, ptr %2147)
  call void @llvm.stackrestore.p0(ptr %2141)
  %2148 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 1, i32 120, i32 117)
  %2149 = call ptr @llvm.stacksave.p0()
  %2150 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1845, ptr %2150, align 8
  %2151 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2150, 1
  %2152 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %2152, align 8
  %2153 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2152, 1
  %2154 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2151, ptr %2154, align 8
  %2155 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2153, ptr %2155, align 8
  call void @memrefCopy(i64 4, ptr %2154, ptr %2155)
  call void @llvm.stackrestore.p0(ptr %2149)
  %2156 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %2157 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %2158 = insertvalue { ptr, ptr, i64 } poison, ptr %2156, 0
  %2159 = insertvalue { ptr, ptr, i64 } %2158, ptr %2157, 1
  %2160 = insertvalue { ptr, ptr, i64 } %2159, i64 0, 2
  %2161 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %2162 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %2163 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %2164 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %2165 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %2166 = extractvalue { ptr, ptr, i64 } %2160, 0
  %2167 = extractvalue { ptr, ptr, i64 } %2160, 1
  %2168 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2166, 0
  %2169 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2168, ptr %2167, 1
  %2170 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2169, i64 1060, 2
  %2171 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2170, i64 16, 3, 0
  %2172 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2171, i64 64, 4, 0
  %2173 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2172, i64 4, 3, 1
  %2174 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2173, i64 1, 4, 1
  %2175 = call ptr @llvm.stacksave.p0()
  %2176 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2174, ptr %2176, align 8
  %2177 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2176, 1
  %2178 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %2178, align 8
  %2179 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2178, 1
  %2180 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2177, ptr %2180, align 8
  %2181 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2179, ptr %2181, align 8
  call void @memrefCopy(i64 4, ptr %2180, ptr %2181)
  call void @llvm.stackrestore.p0(ptr %2175)
  %2182 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 1, i32 189, i32 117)
  %2183 = call ptr @llvm.stacksave.p0()
  %2184 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %2184, align 8
  %2185 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2184, 1
  %2186 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2107, ptr %2186, align 8
  %2187 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2186, 1
  %2188 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2185, ptr %2188, align 8
  %2189 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2187, ptr %2189, align 8
  call void @memrefCopy(i64 4, ptr %2188, ptr %2189)
  call void @llvm.stackrestore.p0(ptr %2183)
  %2190 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %2191 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %2192 = insertvalue { ptr, ptr, i64 } poison, ptr %2190, 0
  %2193 = insertvalue { ptr, ptr, i64 } %2192, ptr %2191, 1
  %2194 = insertvalue { ptr, ptr, i64 } %2193, i64 0, 2
  %2195 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %2196 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %2197 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %2198 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %2199 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %2200 = extractvalue { ptr, ptr, i64 } %2194, 0
  %2201 = extractvalue { ptr, ptr, i64 } %2194, 1
  %2202 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2200, 0
  %2203 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2202, ptr %2201, 1
  %2204 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2203, i64 1064, 2
  %2205 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2204, i64 4, 3, 0
  %2206 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2205, i64 64, 4, 0
  %2207 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2206, i64 4, 3, 1
  %2208 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2207, i64 1, 4, 1
  %2209 = call ptr @llvm.stacksave.p0()
  %2210 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2208, ptr %2210, align 8
  %2211 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2210, 1
  %2212 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %2212, align 8
  %2213 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2212, 1
  %2214 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2211, ptr %2214, align 8
  %2215 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2213, ptr %2215, align 8
  call void @memrefCopy(i64 4, ptr %2214, ptr %2215)
  call void @llvm.stackrestore.p0(ptr %2209)
  %2216 = call ptr @llvm.stacksave.p0()
  %2217 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1768, ptr %2217, align 8
  %2218 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2217, 1
  %2219 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %2219, align 8
  %2220 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2219, 1
  %2221 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2218, ptr %2221, align 8
  %2222 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2220, ptr %2222, align 8
  call void @memrefCopy(i64 4, ptr %2221, ptr %2222)
  call void @llvm.stackrestore.p0(ptr %2216)
  %2223 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %2224 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %2225 = insertvalue { ptr, ptr, i64 } poison, ptr %2223, 0
  %2226 = insertvalue { ptr, ptr, i64 } %2225, ptr %2224, 1
  %2227 = insertvalue { ptr, ptr, i64 } %2226, i64 0, 2
  %2228 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %2229 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %2230 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %2231 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %2232 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %2233 = extractvalue { ptr, ptr, i64 } %2227, 0
  %2234 = extractvalue { ptr, ptr, i64 } %2227, 1
  %2235 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2233, 0
  %2236 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2235, ptr %2234, 1
  %2237 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2236, i64 40, 2
  %2238 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2237, i64 16, 3, 0
  %2239 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2238, i64 64, 4, 0
  %2240 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2239, i64 4, 3, 1
  %2241 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2240, i64 1, 4, 1
  %2242 = call ptr @llvm.stacksave.p0()
  %2243 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2241, ptr %2243, align 8
  %2244 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2243, 1
  %2245 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %2245, align 8
  %2246 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2245, 1
  %2247 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2244, ptr %2247, align 8
  %2248 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2246, ptr %2248, align 8
  call void @memrefCopy(i64 4, ptr %2247, ptr %2248)
  call void @llvm.stackrestore.p0(ptr %2242)
  %2249 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 2, i32 120, i32 117)
  %2250 = call ptr @llvm.stacksave.p0()
  %2251 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1845, ptr %2251, align 8
  %2252 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2251, 1
  %2253 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %2253, align 8
  %2254 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2253, 1
  %2255 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2252, ptr %2255, align 8
  %2256 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2254, ptr %2256, align 8
  call void @memrefCopy(i64 4, ptr %2255, ptr %2256)
  call void @llvm.stackrestore.p0(ptr %2250)
  %2257 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %2258 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %2259 = insertvalue { ptr, ptr, i64 } poison, ptr %2257, 0
  %2260 = insertvalue { ptr, ptr, i64 } %2259, ptr %2258, 1
  %2261 = insertvalue { ptr, ptr, i64 } %2260, i64 0, 2
  %2262 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %2263 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %2264 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %2265 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %2266 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %2267 = extractvalue { ptr, ptr, i64 } %2261, 0
  %2268 = extractvalue { ptr, ptr, i64 } %2261, 1
  %2269 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2267, 0
  %2270 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2269, ptr %2268, 1
  %2271 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2270, i64 1064, 2
  %2272 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2271, i64 16, 3, 0
  %2273 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2272, i64 64, 4, 0
  %2274 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2273, i64 4, 3, 1
  %2275 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2274, i64 1, 4, 1
  %2276 = call ptr @llvm.stacksave.p0()
  %2277 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2275, ptr %2277, align 8
  %2278 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2277, 1
  %2279 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %2279, align 8
  %2280 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2279, 1
  %2281 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2278, ptr %2281, align 8
  %2282 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2280, ptr %2282, align 8
  call void @memrefCopy(i64 4, ptr %2281, ptr %2282)
  call void @llvm.stackrestore.p0(ptr %2276)
  %2283 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 2, i32 189, i32 117)
  %2284 = call ptr @llvm.stacksave.p0()
  %2285 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %2285, align 8
  %2286 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2285, 1
  %2287 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2208, ptr %2287, align 8
  %2288 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2287, 1
  %2289 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2286, ptr %2289, align 8
  %2290 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2288, ptr %2290, align 8
  call void @memrefCopy(i64 4, ptr %2289, ptr %2290)
  call void @llvm.stackrestore.p0(ptr %2284)
  %2291 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %2292 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %2293 = insertvalue { ptr, ptr, i64 } poison, ptr %2291, 0
  %2294 = insertvalue { ptr, ptr, i64 } %2293, ptr %2292, 1
  %2295 = insertvalue { ptr, ptr, i64 } %2294, i64 0, 2
  %2296 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %2297 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %2298 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %2299 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %2300 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %2301 = extractvalue { ptr, ptr, i64 } %2295, 0
  %2302 = extractvalue { ptr, ptr, i64 } %2295, 1
  %2303 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2301, 0
  %2304 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2303, ptr %2302, 1
  %2305 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2304, i64 1068, 2
  %2306 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2305, i64 4, 3, 0
  %2307 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2306, i64 64, 4, 0
  %2308 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2307, i64 4, 3, 1
  %2309 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2308, i64 1, 4, 1
  %2310 = call ptr @llvm.stacksave.p0()
  %2311 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2309, ptr %2311, align 8
  %2312 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2311, 1
  %2313 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %2313, align 8
  %2314 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2313, 1
  %2315 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2312, ptr %2315, align 8
  %2316 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2314, ptr %2316, align 8
  call void @memrefCopy(i64 4, ptr %2315, ptr %2316)
  call void @llvm.stackrestore.p0(ptr %2310)
  %2317 = call ptr @llvm.stacksave.p0()
  %2318 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1768, ptr %2318, align 8
  %2319 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2318, 1
  %2320 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %2320, align 8
  %2321 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2320, 1
  %2322 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2319, ptr %2322, align 8
  %2323 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2321, ptr %2323, align 8
  call void @memrefCopy(i64 4, ptr %2322, ptr %2323)
  call void @llvm.stackrestore.p0(ptr %2317)
  %2324 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %2325 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %2326 = insertvalue { ptr, ptr, i64 } poison, ptr %2324, 0
  %2327 = insertvalue { ptr, ptr, i64 } %2326, ptr %2325, 1
  %2328 = insertvalue { ptr, ptr, i64 } %2327, i64 0, 2
  %2329 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %2330 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %2331 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %2332 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %2333 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %2334 = extractvalue { ptr, ptr, i64 } %2328, 0
  %2335 = extractvalue { ptr, ptr, i64 } %2328, 1
  %2336 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2334, 0
  %2337 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2336, ptr %2335, 1
  %2338 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2337, i64 44, 2
  %2339 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2338, i64 16, 3, 0
  %2340 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2339, i64 64, 4, 0
  %2341 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2340, i64 4, 3, 1
  %2342 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2341, i64 1, 4, 1
  %2343 = call ptr @llvm.stacksave.p0()
  %2344 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2342, ptr %2344, align 8
  %2345 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2344, 1
  %2346 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %2346, align 8
  %2347 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2346, 1
  %2348 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2345, ptr %2348, align 8
  %2349 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2347, ptr %2349, align 8
  call void @memrefCopy(i64 4, ptr %2348, ptr %2349)
  call void @llvm.stackrestore.p0(ptr %2343)
  %2350 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 3, i32 120, i32 117)
  %2351 = call ptr @llvm.stacksave.p0()
  %2352 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1845, ptr %2352, align 8
  %2353 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2352, 1
  %2354 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %2354, align 8
  %2355 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2354, 1
  %2356 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2353, ptr %2356, align 8
  %2357 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2355, ptr %2357, align 8
  call void @memrefCopy(i64 4, ptr %2356, ptr %2357)
  call void @llvm.stackrestore.p0(ptr %2351)
  %2358 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %2359 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %2360 = insertvalue { ptr, ptr, i64 } poison, ptr %2358, 0
  %2361 = insertvalue { ptr, ptr, i64 } %2360, ptr %2359, 1
  %2362 = insertvalue { ptr, ptr, i64 } %2361, i64 0, 2
  %2363 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %2364 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %2365 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %2366 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %2367 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %2368 = extractvalue { ptr, ptr, i64 } %2362, 0
  %2369 = extractvalue { ptr, ptr, i64 } %2362, 1
  %2370 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2368, 0
  %2371 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2370, ptr %2369, 1
  %2372 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2371, i64 1068, 2
  %2373 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2372, i64 16, 3, 0
  %2374 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2373, i64 64, 4, 0
  %2375 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2374, i64 4, 3, 1
  %2376 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2375, i64 1, 4, 1
  %2377 = call ptr @llvm.stacksave.p0()
  %2378 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2376, ptr %2378, align 8
  %2379 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2378, 1
  %2380 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %2380, align 8
  %2381 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2380, 1
  %2382 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2379, ptr %2382, align 8
  %2383 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2381, ptr %2383, align 8
  call void @memrefCopy(i64 4, ptr %2382, ptr %2383)
  call void @llvm.stackrestore.p0(ptr %2377)
  %2384 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 3, i32 189, i32 117)
  %2385 = call ptr @llvm.stacksave.p0()
  %2386 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %2386, align 8
  %2387 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2386, 1
  %2388 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2309, ptr %2388, align 8
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
  %2406 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2405, i64 1072, 2
  %2407 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2406, i64 4, 3, 0
  %2408 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2407, i64 64, 4, 0
  %2409 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2408, i64 4, 3, 1
  %2410 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2409, i64 1, 4, 1
  %2411 = call ptr @llvm.stacksave.p0()
  %2412 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2410, ptr %2412, align 8
  %2413 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2412, 1
  %2414 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %2414, align 8
  %2415 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2414, 1
  %2416 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2413, ptr %2416, align 8
  %2417 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2415, ptr %2417, align 8
  call void @memrefCopy(i64 4, ptr %2416, ptr %2417)
  call void @llvm.stackrestore.p0(ptr %2411)
  %2418 = call ptr @llvm.stacksave.p0()
  %2419 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1768, ptr %2419, align 8
  %2420 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2419, 1
  %2421 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %2421, align 8
  %2422 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2421, 1
  %2423 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2420, ptr %2423, align 8
  %2424 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2422, ptr %2424, align 8
  call void @memrefCopy(i64 4, ptr %2423, ptr %2424)
  call void @llvm.stackrestore.p0(ptr %2418)
  %2425 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %2426 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %2427 = insertvalue { ptr, ptr, i64 } poison, ptr %2425, 0
  %2428 = insertvalue { ptr, ptr, i64 } %2427, ptr %2426, 1
  %2429 = insertvalue { ptr, ptr, i64 } %2428, i64 0, 2
  %2430 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %2431 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %2432 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %2433 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %2434 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %2435 = extractvalue { ptr, ptr, i64 } %2429, 0
  %2436 = extractvalue { ptr, ptr, i64 } %2429, 1
  %2437 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2435, 0
  %2438 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2437, ptr %2436, 1
  %2439 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2438, i64 48, 2
  %2440 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2439, i64 16, 3, 0
  %2441 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2440, i64 64, 4, 0
  %2442 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2441, i64 4, 3, 1
  %2443 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2442, i64 1, 4, 1
  %2444 = call ptr @llvm.stacksave.p0()
  %2445 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2443, ptr %2445, align 8
  %2446 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2445, 1
  %2447 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %2447, align 8
  %2448 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2447, 1
  %2449 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2446, ptr %2449, align 8
  %2450 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2448, ptr %2450, align 8
  call void @memrefCopy(i64 4, ptr %2449, ptr %2450)
  call void @llvm.stackrestore.p0(ptr %2444)
  %2451 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 1, i32 258, i32 117)
  %2452 = call ptr @llvm.stacksave.p0()
  %2453 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1845, ptr %2453, align 8
  %2454 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2453, 1
  %2455 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %2455, align 8
  %2456 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2455, 1
  %2457 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2454, ptr %2457, align 8
  %2458 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2456, ptr %2458, align 8
  call void @memrefCopy(i64 4, ptr %2457, ptr %2458)
  call void @llvm.stackrestore.p0(ptr %2452)
  %2459 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %2460 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %2461 = insertvalue { ptr, ptr, i64 } poison, ptr %2459, 0
  %2462 = insertvalue { ptr, ptr, i64 } %2461, ptr %2460, 1
  %2463 = insertvalue { ptr, ptr, i64 } %2462, i64 0, 2
  %2464 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %2465 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %2466 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %2467 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %2468 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %2469 = extractvalue { ptr, ptr, i64 } %2463, 0
  %2470 = extractvalue { ptr, ptr, i64 } %2463, 1
  %2471 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2469, 0
  %2472 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2471, ptr %2470, 1
  %2473 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2472, i64 1072, 2
  %2474 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2473, i64 16, 3, 0
  %2475 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2474, i64 64, 4, 0
  %2476 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2475, i64 4, 3, 1
  %2477 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2476, i64 1, 4, 1
  %2478 = call ptr @llvm.stacksave.p0()
  %2479 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2477, ptr %2479, align 8
  %2480 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2479, 1
  %2481 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %2481, align 8
  %2482 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2481, 1
  %2483 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2480, ptr %2483, align 8
  %2484 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2482, ptr %2484, align 8
  call void @memrefCopy(i64 4, ptr %2483, ptr %2484)
  call void @llvm.stackrestore.p0(ptr %2478)
  %2485 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 1, i32 327, i32 117)
  %2486 = call ptr @llvm.stacksave.p0()
  %2487 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %2487, align 8
  %2488 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2487, 1
  %2489 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2410, ptr %2489, align 8
  %2490 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2489, 1
  %2491 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2488, ptr %2491, align 8
  %2492 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2490, ptr %2492, align 8
  call void @memrefCopy(i64 4, ptr %2491, ptr %2492)
  call void @llvm.stackrestore.p0(ptr %2486)
  %2493 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %2494 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %2495 = insertvalue { ptr, ptr, i64 } poison, ptr %2493, 0
  %2496 = insertvalue { ptr, ptr, i64 } %2495, ptr %2494, 1
  %2497 = insertvalue { ptr, ptr, i64 } %2496, i64 0, 2
  %2498 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %2499 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %2500 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %2501 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %2502 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %2503 = extractvalue { ptr, ptr, i64 } %2497, 0
  %2504 = extractvalue { ptr, ptr, i64 } %2497, 1
  %2505 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2503, 0
  %2506 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2505, ptr %2504, 1
  %2507 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2506, i64 1076, 2
  %2508 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2507, i64 4, 3, 0
  %2509 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2508, i64 64, 4, 0
  %2510 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2509, i64 4, 3, 1
  %2511 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2510, i64 1, 4, 1
  %2512 = call ptr @llvm.stacksave.p0()
  %2513 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2511, ptr %2513, align 8
  %2514 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2513, 1
  %2515 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %2515, align 8
  %2516 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2515, 1
  %2517 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2514, ptr %2517, align 8
  %2518 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2516, ptr %2518, align 8
  call void @memrefCopy(i64 4, ptr %2517, ptr %2518)
  call void @llvm.stackrestore.p0(ptr %2512)
  %2519 = call ptr @llvm.stacksave.p0()
  %2520 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1768, ptr %2520, align 8
  %2521 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2520, 1
  %2522 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %2522, align 8
  %2523 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2522, 1
  %2524 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2521, ptr %2524, align 8
  %2525 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2523, ptr %2525, align 8
  call void @memrefCopy(i64 4, ptr %2524, ptr %2525)
  call void @llvm.stackrestore.p0(ptr %2519)
  %2526 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %2527 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %2528 = insertvalue { ptr, ptr, i64 } poison, ptr %2526, 0
  %2529 = insertvalue { ptr, ptr, i64 } %2528, ptr %2527, 1
  %2530 = insertvalue { ptr, ptr, i64 } %2529, i64 0, 2
  %2531 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %2532 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %2533 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %2534 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %2535 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %2536 = extractvalue { ptr, ptr, i64 } %2530, 0
  %2537 = extractvalue { ptr, ptr, i64 } %2530, 1
  %2538 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2536, 0
  %2539 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2538, ptr %2537, 1
  %2540 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2539, i64 52, 2
  %2541 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2540, i64 16, 3, 0
  %2542 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2541, i64 64, 4, 0
  %2543 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2542, i64 4, 3, 1
  %2544 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2543, i64 1, 4, 1
  %2545 = call ptr @llvm.stacksave.p0()
  %2546 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2544, ptr %2546, align 8
  %2547 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2546, 1
  %2548 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %2548, align 8
  %2549 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2548, 1
  %2550 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2547, ptr %2550, align 8
  %2551 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2549, ptr %2551, align 8
  call void @memrefCopy(i64 4, ptr %2550, ptr %2551)
  call void @llvm.stackrestore.p0(ptr %2545)
  %2552 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 2, i32 258, i32 117)
  %2553 = call ptr @llvm.stacksave.p0()
  %2554 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1845, ptr %2554, align 8
  %2555 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2554, 1
  %2556 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %2556, align 8
  %2557 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2556, 1
  %2558 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2555, ptr %2558, align 8
  %2559 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2557, ptr %2559, align 8
  call void @memrefCopy(i64 4, ptr %2558, ptr %2559)
  call void @llvm.stackrestore.p0(ptr %2553)
  %2560 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %2561 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %2562 = insertvalue { ptr, ptr, i64 } poison, ptr %2560, 0
  %2563 = insertvalue { ptr, ptr, i64 } %2562, ptr %2561, 1
  %2564 = insertvalue { ptr, ptr, i64 } %2563, i64 0, 2
  %2565 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %2566 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %2567 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %2568 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %2569 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %2570 = extractvalue { ptr, ptr, i64 } %2564, 0
  %2571 = extractvalue { ptr, ptr, i64 } %2564, 1
  %2572 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2570, 0
  %2573 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2572, ptr %2571, 1
  %2574 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2573, i64 1076, 2
  %2575 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2574, i64 16, 3, 0
  %2576 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2575, i64 64, 4, 0
  %2577 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2576, i64 4, 3, 1
  %2578 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2577, i64 1, 4, 1
  %2579 = call ptr @llvm.stacksave.p0()
  %2580 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2578, ptr %2580, align 8
  %2581 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2580, 1
  %2582 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %2582, align 8
  %2583 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2582, 1
  %2584 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2581, ptr %2584, align 8
  %2585 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2583, ptr %2585, align 8
  call void @memrefCopy(i64 4, ptr %2584, ptr %2585)
  call void @llvm.stackrestore.p0(ptr %2579)
  %2586 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 2, i32 327, i32 117)
  %2587 = call ptr @llvm.stacksave.p0()
  %2588 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %2588, align 8
  %2589 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2588, 1
  %2590 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2511, ptr %2590, align 8
  %2591 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2590, 1
  %2592 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2589, ptr %2592, align 8
  %2593 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2591, ptr %2593, align 8
  call void @memrefCopy(i64 4, ptr %2592, ptr %2593)
  call void @llvm.stackrestore.p0(ptr %2587)
  %2594 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %2595 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %2596 = insertvalue { ptr, ptr, i64 } poison, ptr %2594, 0
  %2597 = insertvalue { ptr, ptr, i64 } %2596, ptr %2595, 1
  %2598 = insertvalue { ptr, ptr, i64 } %2597, i64 0, 2
  %2599 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %2600 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %2601 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %2602 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %2603 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %2604 = extractvalue { ptr, ptr, i64 } %2598, 0
  %2605 = extractvalue { ptr, ptr, i64 } %2598, 1
  %2606 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2604, 0
  %2607 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2606, ptr %2605, 1
  %2608 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2607, i64 1080, 2
  %2609 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2608, i64 4, 3, 0
  %2610 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2609, i64 64, 4, 0
  %2611 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2610, i64 4, 3, 1
  %2612 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2611, i64 1, 4, 1
  %2613 = call ptr @llvm.stacksave.p0()
  %2614 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2612, ptr %2614, align 8
  %2615 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2614, 1
  %2616 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %2616, align 8
  %2617 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2616, 1
  %2618 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2615, ptr %2618, align 8
  %2619 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2617, ptr %2619, align 8
  call void @memrefCopy(i64 4, ptr %2618, ptr %2619)
  call void @llvm.stackrestore.p0(ptr %2613)
  %2620 = call ptr @llvm.stacksave.p0()
  %2621 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1768, ptr %2621, align 8
  %2622 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2621, 1
  %2623 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %2623, align 8
  %2624 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2623, 1
  %2625 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2622, ptr %2625, align 8
  %2626 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2624, ptr %2626, align 8
  call void @memrefCopy(i64 4, ptr %2625, ptr %2626)
  call void @llvm.stackrestore.p0(ptr %2620)
  %2627 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %2628 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %2629 = insertvalue { ptr, ptr, i64 } poison, ptr %2627, 0
  %2630 = insertvalue { ptr, ptr, i64 } %2629, ptr %2628, 1
  %2631 = insertvalue { ptr, ptr, i64 } %2630, i64 0, 2
  %2632 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %2633 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %2634 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %2635 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %2636 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %2637 = extractvalue { ptr, ptr, i64 } %2631, 0
  %2638 = extractvalue { ptr, ptr, i64 } %2631, 1
  %2639 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2637, 0
  %2640 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2639, ptr %2638, 1
  %2641 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2640, i64 56, 2
  %2642 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2641, i64 16, 3, 0
  %2643 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2642, i64 64, 4, 0
  %2644 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2643, i64 4, 3, 1
  %2645 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2644, i64 1, 4, 1
  %2646 = call ptr @llvm.stacksave.p0()
  %2647 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2645, ptr %2647, align 8
  %2648 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2647, 1
  %2649 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %2649, align 8
  %2650 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2649, 1
  %2651 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2648, ptr %2651, align 8
  %2652 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2650, ptr %2652, align 8
  call void @memrefCopy(i64 4, ptr %2651, ptr %2652)
  call void @llvm.stackrestore.p0(ptr %2646)
  %2653 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 3, i32 258, i32 117)
  %2654 = call ptr @llvm.stacksave.p0()
  %2655 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1845, ptr %2655, align 8
  %2656 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2655, 1
  %2657 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %2657, align 8
  %2658 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2657, 1
  %2659 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2656, ptr %2659, align 8
  %2660 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2658, ptr %2660, align 8
  call void @memrefCopy(i64 4, ptr %2659, ptr %2660)
  call void @llvm.stackrestore.p0(ptr %2654)
  %2661 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %2662 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %2663 = insertvalue { ptr, ptr, i64 } poison, ptr %2661, 0
  %2664 = insertvalue { ptr, ptr, i64 } %2663, ptr %2662, 1
  %2665 = insertvalue { ptr, ptr, i64 } %2664, i64 0, 2
  %2666 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %2667 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %2668 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %2669 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %2670 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %2671 = extractvalue { ptr, ptr, i64 } %2665, 0
  %2672 = extractvalue { ptr, ptr, i64 } %2665, 1
  %2673 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2671, 0
  %2674 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2673, ptr %2672, 1
  %2675 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2674, i64 1080, 2
  %2676 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2675, i64 16, 3, 0
  %2677 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2676, i64 64, 4, 0
  %2678 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2677, i64 4, 3, 1
  %2679 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2678, i64 1, 4, 1
  %2680 = call ptr @llvm.stacksave.p0()
  %2681 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2679, ptr %2681, align 8
  %2682 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2681, 1
  %2683 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %2683, align 8
  %2684 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2683, 1
  %2685 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2682, ptr %2685, align 8
  %2686 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2684, ptr %2686, align 8
  call void @memrefCopy(i64 4, ptr %2685, ptr %2686)
  call void @llvm.stackrestore.p0(ptr %2680)
  %2687 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 3, i32 327, i32 117)
  %2688 = call ptr @llvm.stacksave.p0()
  %2689 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %2689, align 8
  %2690 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2689, 1
  %2691 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2612, ptr %2691, align 8
  %2692 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2691, 1
  %2693 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2690, ptr %2693, align 8
  %2694 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2692, ptr %2694, align 8
  call void @memrefCopy(i64 4, ptr %2693, ptr %2694)
  call void @llvm.stackrestore.p0(ptr %2688)
  %2695 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %2696 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %2697 = insertvalue { ptr, ptr, i64 } poison, ptr %2695, 0
  %2698 = insertvalue { ptr, ptr, i64 } %2697, ptr %2696, 1
  %2699 = insertvalue { ptr, ptr, i64 } %2698, i64 0, 2
  %2700 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %2701 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %2702 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %2703 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %2704 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %2705 = extractvalue { ptr, ptr, i64 } %2699, 0
  %2706 = extractvalue { ptr, ptr, i64 } %2699, 1
  %2707 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2705, 0
  %2708 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2707, ptr %2706, 1
  %2709 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2708, i64 1084, 2
  %2710 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2709, i64 4, 3, 0
  %2711 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2710, i64 64, 4, 0
  %2712 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2711, i64 4, 3, 1
  %2713 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2712, i64 1, 4, 1
  %2714 = call ptr @llvm.stacksave.p0()
  %2715 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2713, ptr %2715, align 8
  %2716 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2715, 1
  %2717 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %2717, align 8
  %2718 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2717, 1
  %2719 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2716, ptr %2719, align 8
  %2720 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2718, ptr %2720, align 8
  call void @memrefCopy(i64 4, ptr %2719, ptr %2720)
  call void @llvm.stackrestore.p0(ptr %2714)
  %2721 = call ptr @llvm.stacksave.p0()
  %2722 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1768, ptr %2722, align 8
  %2723 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2722, 1
  %2724 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %2724, align 8
  %2725 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2724, 1
  %2726 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2723, ptr %2726, align 8
  %2727 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2725, ptr %2727, align 8
  call void @memrefCopy(i64 4, ptr %2726, ptr %2727)
  call void @llvm.stackrestore.p0(ptr %2721)
  %2728 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %2729 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %2730 = insertvalue { ptr, ptr, i64 } poison, ptr %2728, 0
  %2731 = insertvalue { ptr, ptr, i64 } %2730, ptr %2729, 1
  %2732 = insertvalue { ptr, ptr, i64 } %2731, i64 0, 2
  %2733 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %2734 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %2735 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %2736 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %2737 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %2738 = extractvalue { ptr, ptr, i64 } %2732, 0
  %2739 = extractvalue { ptr, ptr, i64 } %2732, 1
  %2740 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2738, 0
  %2741 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2740, ptr %2739, 1
  %2742 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2741, i64 60, 2
  %2743 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2742, i64 16, 3, 0
  %2744 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2743, i64 64, 4, 0
  %2745 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2744, i64 4, 3, 1
  %2746 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2745, i64 1, 4, 1
  %2747 = call ptr @llvm.stacksave.p0()
  %2748 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2746, ptr %2748, align 8
  %2749 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2748, 1
  %2750 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %2750, align 8
  %2751 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2750, 1
  %2752 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2749, ptr %2752, align 8
  %2753 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2751, ptr %2753, align 8
  call void @memrefCopy(i64 4, ptr %2752, ptr %2753)
  call void @llvm.stackrestore.p0(ptr %2747)
  %2754 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 1, i32 396, i32 117)
  %2755 = call ptr @llvm.stacksave.p0()
  %2756 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1845, ptr %2756, align 8
  %2757 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2756, 1
  %2758 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %2758, align 8
  %2759 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2758, 1
  %2760 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2757, ptr %2760, align 8
  %2761 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2759, ptr %2761, align 8
  call void @memrefCopy(i64 4, ptr %2760, ptr %2761)
  call void @llvm.stackrestore.p0(ptr %2755)
  %2762 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %2763 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %2764 = insertvalue { ptr, ptr, i64 } poison, ptr %2762, 0
  %2765 = insertvalue { ptr, ptr, i64 } %2764, ptr %2763, 1
  %2766 = insertvalue { ptr, ptr, i64 } %2765, i64 0, 2
  %2767 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %2768 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %2769 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %2770 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %2771 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %2772 = extractvalue { ptr, ptr, i64 } %2766, 0
  %2773 = extractvalue { ptr, ptr, i64 } %2766, 1
  %2774 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2772, 0
  %2775 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2774, ptr %2773, 1
  %2776 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2775, i64 1084, 2
  %2777 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2776, i64 16, 3, 0
  %2778 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2777, i64 64, 4, 0
  %2779 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2778, i64 4, 3, 1
  %2780 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2779, i64 1, 4, 1
  %2781 = call ptr @llvm.stacksave.p0()
  %2782 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2780, ptr %2782, align 8
  %2783 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2782, 1
  %2784 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %2784, align 8
  %2785 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2784, 1
  %2786 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2783, ptr %2786, align 8
  %2787 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2785, ptr %2787, align 8
  call void @memrefCopy(i64 4, ptr %2786, ptr %2787)
  call void @llvm.stackrestore.p0(ptr %2781)
  %2788 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 1, i32 465, i32 117)
  %2789 = call ptr @llvm.stacksave.p0()
  %2790 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %2790, align 8
  %2791 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2790, 1
  %2792 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2713, ptr %2792, align 8
  %2793 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2792, 1
  %2794 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2791, ptr %2794, align 8
  %2795 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2793, ptr %2795, align 8
  call void @memrefCopy(i64 4, ptr %2794, ptr %2795)
  call void @llvm.stackrestore.p0(ptr %2789)
  %2796 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %2797 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %2798 = insertvalue { ptr, ptr, i64 } poison, ptr %2796, 0
  %2799 = insertvalue { ptr, ptr, i64 } %2798, ptr %2797, 1
  %2800 = insertvalue { ptr, ptr, i64 } %2799, i64 0, 2
  %2801 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %2802 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %2803 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %2804 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %2805 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %2806 = extractvalue { ptr, ptr, i64 } %2800, 0
  %2807 = extractvalue { ptr, ptr, i64 } %2800, 1
  %2808 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2806, 0
  %2809 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2808, ptr %2807, 1
  %2810 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2809, i64 1304, 2
  %2811 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2810, i64 4, 3, 0
  %2812 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2811, i64 64, 4, 0
  %2813 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2812, i64 4, 3, 1
  %2814 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2813, i64 1, 4, 1
  %2815 = call ptr @llvm.stacksave.p0()
  %2816 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2814, ptr %2816, align 8
  %2817 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2816, 1
  %2818 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %2818, align 8
  %2819 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2818, 1
  %2820 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2817, ptr %2820, align 8
  %2821 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2819, ptr %2821, align 8
  call void @memrefCopy(i64 4, ptr %2820, ptr %2821)
  call void @llvm.stackrestore.p0(ptr %2815)
  %2822 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %2823 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %2824 = insertvalue { ptr, ptr, i64 } poison, ptr %2822, 0
  %2825 = insertvalue { ptr, ptr, i64 } %2824, ptr %2823, 1
  %2826 = insertvalue { ptr, ptr, i64 } %2825, i64 0, 2
  %2827 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %2828 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %2829 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %2830 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %2831 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %2832 = extractvalue { ptr, ptr, i64 } %2826, 0
  %2833 = extractvalue { ptr, ptr, i64 } %2826, 1
  %2834 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2832, 0
  %2835 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2834, ptr %2833, 1
  %2836 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2835, i64 640, 2
  %2837 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2836, i64 4, 3, 0
  %2838 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2837, i64 32, 4, 0
  %2839 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2838, i64 16, 3, 1
  %2840 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2839, i64 1, 4, 1
  %2841 = call ptr @llvm.stacksave.p0()
  %2842 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2840, ptr %2842, align 8
  %2843 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2842, 1
  %2844 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %2844, align 8
  %2845 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2844, 1
  %2846 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2843, ptr %2846, align 8
  %2847 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2845, ptr %2847, align 8
  call void @memrefCopy(i64 4, ptr %2846, ptr %2847)
  call void @llvm.stackrestore.p0(ptr %2841)
  %2848 = call ptr @llvm.stacksave.p0()
  %2849 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1806, ptr %2849, align 8
  %2850 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2849, 1
  %2851 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %2851, align 8
  %2852 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2851, 1
  %2853 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2850, ptr %2853, align 8
  %2854 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2852, ptr %2854, align 8
  call void @memrefCopy(i64 4, ptr %2853, ptr %2854)
  call void @llvm.stackrestore.p0(ptr %2848)
  %2855 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 2, i32 396, i32 117)
  %2856 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %2857 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %2858 = insertvalue { ptr, ptr, i64 } poison, ptr %2856, 0
  %2859 = insertvalue { ptr, ptr, i64 } %2858, ptr %2857, 1
  %2860 = insertvalue { ptr, ptr, i64 } %2859, i64 0, 2
  %2861 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %2862 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %2863 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %2864 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %2865 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %2866 = extractvalue { ptr, ptr, i64 } %2860, 0
  %2867 = extractvalue { ptr, ptr, i64 } %2860, 1
  %2868 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2866, 0
  %2869 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2868, ptr %2867, 1
  %2870 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2869, i64 656, 2
  %2871 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2870, i64 4, 3, 0
  %2872 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2871, i64 32, 4, 0
  %2873 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2872, i64 16, 3, 1
  %2874 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2873, i64 1, 4, 1
  %2875 = call ptr @llvm.stacksave.p0()
  %2876 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2874, ptr %2876, align 8
  %2877 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2876, 1
  %2878 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %2878, align 8
  %2879 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2878, 1
  %2880 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2877, ptr %2880, align 8
  %2881 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2879, ptr %2881, align 8
  call void @memrefCopy(i64 4, ptr %2880, ptr %2881)
  call void @llvm.stackrestore.p0(ptr %2875)
  %2882 = call ptr @llvm.stacksave.p0()
  %2883 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1871, ptr %2883, align 8
  %2884 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2883, 1
  %2885 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %2885, align 8
  %2886 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2885, 1
  %2887 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2884, ptr %2887, align 8
  %2888 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2886, ptr %2888, align 8
  call void @memrefCopy(i64 4, ptr %2887, ptr %2888)
  call void @llvm.stackrestore.p0(ptr %2882)
  %2889 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 2, i32 465, i32 117)
  %2890 = call ptr @llvm.stacksave.p0()
  %2891 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %2891, align 8
  %2892 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2891, 1
  %2893 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2814, ptr %2893, align 8
  %2894 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2893, 1
  %2895 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2892, ptr %2895, align 8
  %2896 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2894, ptr %2896, align 8
  call void @memrefCopy(i64 4, ptr %2895, ptr %2896)
  call void @llvm.stackrestore.p0(ptr %2890)
  %2897 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %2898 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %2899 = insertvalue { ptr, ptr, i64 } poison, ptr %2897, 0
  %2900 = insertvalue { ptr, ptr, i64 } %2899, ptr %2898, 1
  %2901 = insertvalue { ptr, ptr, i64 } %2900, i64 0, 2
  %2902 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %2903 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %2904 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %2905 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %2906 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %2907 = extractvalue { ptr, ptr, i64 } %2901, 0
  %2908 = extractvalue { ptr, ptr, i64 } %2901, 1
  %2909 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2907, 0
  %2910 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2909, ptr %2908, 1
  %2911 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2910, i64 1308, 2
  %2912 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2911, i64 4, 3, 0
  %2913 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2912, i64 64, 4, 0
  %2914 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2913, i64 4, 3, 1
  %2915 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2914, i64 1, 4, 1
  %2916 = call ptr @llvm.stacksave.p0()
  %2917 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2915, ptr %2917, align 8
  %2918 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2917, 1
  %2919 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %2919, align 8
  %2920 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2919, 1
  %2921 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2918, ptr %2921, align 8
  %2922 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2920, ptr %2922, align 8
  call void @memrefCopy(i64 4, ptr %2921, ptr %2922)
  call void @llvm.stackrestore.p0(ptr %2916)
  %2923 = call ptr @llvm.stacksave.p0()
  %2924 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2840, ptr %2924, align 8
  %2925 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2924, 1
  %2926 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %2926, align 8
  %2927 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2926, 1
  %2928 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2925, ptr %2928, align 8
  %2929 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2927, ptr %2929, align 8
  call void @memrefCopy(i64 4, ptr %2928, ptr %2929)
  call void @llvm.stackrestore.p0(ptr %2923)
  %2930 = call ptr @llvm.stacksave.p0()
  %2931 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1938, ptr %2931, align 8
  %2932 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2931, 1
  %2933 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %2933, align 8
  %2934 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2933, 1
  %2935 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2932, ptr %2935, align 8
  %2936 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2934, ptr %2936, align 8
  call void @memrefCopy(i64 4, ptr %2935, ptr %2936)
  call void @llvm.stackrestore.p0(ptr %2930)
  %2937 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 3, i32 396, i32 117)
  %2938 = call ptr @llvm.stacksave.p0()
  %2939 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2874, ptr %2939, align 8
  %2940 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2939, 1
  %2941 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %2941, align 8
  %2942 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2941, 1
  %2943 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2940, ptr %2943, align 8
  %2944 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2942, ptr %2944, align 8
  call void @memrefCopy(i64 4, ptr %2943, ptr %2944)
  call void @llvm.stackrestore.p0(ptr %2938)
  %2945 = call ptr @llvm.stacksave.p0()
  %2946 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1972, ptr %2946, align 8
  %2947 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2946, 1
  %2948 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %2948, align 8
  %2949 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2948, 1
  %2950 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2947, ptr %2950, align 8
  %2951 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2949, ptr %2951, align 8
  call void @memrefCopy(i64 4, ptr %2950, ptr %2951)
  call void @llvm.stackrestore.p0(ptr %2945)
  %2952 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 3, i32 465, i32 117)
  %2953 = call ptr @llvm.stacksave.p0()
  %2954 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %2954, align 8
  %2955 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2954, 1
  %2956 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2915, ptr %2956, align 8
  %2957 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2956, 1
  %2958 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2955, ptr %2958, align 8
  %2959 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2957, ptr %2959, align 8
  call void @memrefCopy(i64 4, ptr %2958, ptr %2959)
  call void @llvm.stackrestore.p0(ptr %2953)
  %2960 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %2961 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %2962 = insertvalue { ptr, ptr, i64 } poison, ptr %2960, 0
  %2963 = insertvalue { ptr, ptr, i64 } %2962, ptr %2961, 1
  %2964 = insertvalue { ptr, ptr, i64 } %2963, i64 0, 2
  %2965 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %2966 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %2967 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %2968 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %2969 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %2970 = extractvalue { ptr, ptr, i64 } %2964, 0
  %2971 = extractvalue { ptr, ptr, i64 } %2964, 1
  %2972 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2970, 0
  %2973 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2972, ptr %2971, 1
  %2974 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2973, i64 1312, 2
  %2975 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2974, i64 4, 3, 0
  %2976 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2975, i64 64, 4, 0
  %2977 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2976, i64 4, 3, 1
  %2978 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2977, i64 1, 4, 1
  %2979 = call ptr @llvm.stacksave.p0()
  %2980 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2978, ptr %2980, align 8
  %2981 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2980, 1
  %2982 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %2982, align 8
  %2983 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2982, 1
  %2984 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2981, ptr %2984, align 8
  %2985 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2983, ptr %2985, align 8
  call void @memrefCopy(i64 4, ptr %2984, ptr %2985)
  call void @llvm.stackrestore.p0(ptr %2979)
  %2986 = call ptr @llvm.stacksave.p0()
  %2987 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2840, ptr %2987, align 8
  %2988 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2987, 1
  %2989 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %2989, align 8
  %2990 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2989, 1
  %2991 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2988, ptr %2991, align 8
  %2992 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2990, ptr %2992, align 8
  call void @memrefCopy(i64 4, ptr %2991, ptr %2992)
  call void @llvm.stackrestore.p0(ptr %2986)
  %2993 = call ptr @llvm.stacksave.p0()
  %2994 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2039, ptr %2994, align 8
  %2995 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2994, 1
  %2996 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %2996, align 8
  %2997 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2996, 1
  %2998 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2995, ptr %2998, align 8
  %2999 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2997, ptr %2999, align 8
  call void @memrefCopy(i64 4, ptr %2998, ptr %2999)
  call void @llvm.stackrestore.p0(ptr %2993)
  %3000 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 1, i32 534, i32 117)
  %3001 = call ptr @llvm.stacksave.p0()
  %3002 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2874, ptr %3002, align 8
  %3003 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3002, 1
  %3004 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %3004, align 8
  %3005 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3004, 1
  %3006 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3003, ptr %3006, align 8
  %3007 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3005, ptr %3007, align 8
  call void @memrefCopy(i64 4, ptr %3006, ptr %3007)
  call void @llvm.stackrestore.p0(ptr %3001)
  %3008 = call ptr @llvm.stacksave.p0()
  %3009 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2073, ptr %3009, align 8
  %3010 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3009, 1
  %3011 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %3011, align 8
  %3012 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3011, 1
  %3013 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3010, ptr %3013, align 8
  %3014 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3012, ptr %3014, align 8
  call void @memrefCopy(i64 4, ptr %3013, ptr %3014)
  call void @llvm.stackrestore.p0(ptr %3008)
  %3015 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 1, i32 603, i32 117)
  %3016 = call ptr @llvm.stacksave.p0()
  %3017 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %3017, align 8
  %3018 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3017, 1
  %3019 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2978, ptr %3019, align 8
  %3020 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3019, 1
  %3021 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3018, ptr %3021, align 8
  %3022 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3020, ptr %3022, align 8
  call void @memrefCopy(i64 4, ptr %3021, ptr %3022)
  call void @llvm.stackrestore.p0(ptr %3016)
  %3023 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %3024 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %3025 = insertvalue { ptr, ptr, i64 } poison, ptr %3023, 0
  %3026 = insertvalue { ptr, ptr, i64 } %3025, ptr %3024, 1
  %3027 = insertvalue { ptr, ptr, i64 } %3026, i64 0, 2
  %3028 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %3029 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %3030 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %3031 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %3032 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %3033 = extractvalue { ptr, ptr, i64 } %3027, 0
  %3034 = extractvalue { ptr, ptr, i64 } %3027, 1
  %3035 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %3033, 0
  %3036 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3035, ptr %3034, 1
  %3037 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3036, i64 1316, 2
  %3038 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3037, i64 4, 3, 0
  %3039 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3038, i64 64, 4, 0
  %3040 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3039, i64 4, 3, 1
  %3041 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3040, i64 1, 4, 1
  %3042 = call ptr @llvm.stacksave.p0()
  %3043 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3041, ptr %3043, align 8
  %3044 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3043, 1
  %3045 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %3045, align 8
  %3046 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3045, 1
  %3047 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3044, ptr %3047, align 8
  %3048 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3046, ptr %3048, align 8
  call void @memrefCopy(i64 4, ptr %3047, ptr %3048)
  call void @llvm.stackrestore.p0(ptr %3042)
  %3049 = call ptr @llvm.stacksave.p0()
  %3050 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2840, ptr %3050, align 8
  %3051 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3050, 1
  %3052 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %3052, align 8
  %3053 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3052, 1
  %3054 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3051, ptr %3054, align 8
  %3055 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3053, ptr %3055, align 8
  call void @memrefCopy(i64 4, ptr %3054, ptr %3055)
  call void @llvm.stackrestore.p0(ptr %3049)
  %3056 = call ptr @llvm.stacksave.p0()
  %3057 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2140, ptr %3057, align 8
  %3058 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3057, 1
  %3059 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %3059, align 8
  %3060 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3059, 1
  %3061 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3058, ptr %3061, align 8
  %3062 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3060, ptr %3062, align 8
  call void @memrefCopy(i64 4, ptr %3061, ptr %3062)
  call void @llvm.stackrestore.p0(ptr %3056)
  %3063 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 2, i32 534, i32 117)
  %3064 = call ptr @llvm.stacksave.p0()
  %3065 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2874, ptr %3065, align 8
  %3066 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3065, 1
  %3067 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %3067, align 8
  %3068 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3067, 1
  %3069 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3066, ptr %3069, align 8
  %3070 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3068, ptr %3070, align 8
  call void @memrefCopy(i64 4, ptr %3069, ptr %3070)
  call void @llvm.stackrestore.p0(ptr %3064)
  %3071 = call ptr @llvm.stacksave.p0()
  %3072 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2174, ptr %3072, align 8
  %3073 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3072, 1
  %3074 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %3074, align 8
  %3075 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3074, 1
  %3076 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3073, ptr %3076, align 8
  %3077 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3075, ptr %3077, align 8
  call void @memrefCopy(i64 4, ptr %3076, ptr %3077)
  call void @llvm.stackrestore.p0(ptr %3071)
  %3078 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 2, i32 603, i32 117)
  %3079 = call ptr @llvm.stacksave.p0()
  %3080 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %3080, align 8
  %3081 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3080, 1
  %3082 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3041, ptr %3082, align 8
  %3083 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3082, 1
  %3084 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3081, ptr %3084, align 8
  %3085 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3083, ptr %3085, align 8
  call void @memrefCopy(i64 4, ptr %3084, ptr %3085)
  call void @llvm.stackrestore.p0(ptr %3079)
  %3086 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %3087 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %3088 = insertvalue { ptr, ptr, i64 } poison, ptr %3086, 0
  %3089 = insertvalue { ptr, ptr, i64 } %3088, ptr %3087, 1
  %3090 = insertvalue { ptr, ptr, i64 } %3089, i64 0, 2
  %3091 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %3092 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %3093 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %3094 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %3095 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %3096 = extractvalue { ptr, ptr, i64 } %3090, 0
  %3097 = extractvalue { ptr, ptr, i64 } %3090, 1
  %3098 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %3096, 0
  %3099 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3098, ptr %3097, 1
  %3100 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3099, i64 1320, 2
  %3101 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3100, i64 4, 3, 0
  %3102 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3101, i64 64, 4, 0
  %3103 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3102, i64 4, 3, 1
  %3104 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3103, i64 1, 4, 1
  %3105 = call ptr @llvm.stacksave.p0()
  %3106 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3104, ptr %3106, align 8
  %3107 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3106, 1
  %3108 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %3108, align 8
  %3109 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3108, 1
  %3110 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3107, ptr %3110, align 8
  %3111 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3109, ptr %3111, align 8
  call void @memrefCopy(i64 4, ptr %3110, ptr %3111)
  call void @llvm.stackrestore.p0(ptr %3105)
  %3112 = call ptr @llvm.stacksave.p0()
  %3113 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2840, ptr %3113, align 8
  %3114 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3113, 1
  %3115 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %3115, align 8
  %3116 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3115, 1
  %3117 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3114, ptr %3117, align 8
  %3118 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3116, ptr %3118, align 8
  call void @memrefCopy(i64 4, ptr %3117, ptr %3118)
  call void @llvm.stackrestore.p0(ptr %3112)
  %3119 = call ptr @llvm.stacksave.p0()
  %3120 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2241, ptr %3120, align 8
  %3121 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3120, 1
  %3122 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %3122, align 8
  %3123 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3122, 1
  %3124 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3121, ptr %3124, align 8
  %3125 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3123, ptr %3125, align 8
  call void @memrefCopy(i64 4, ptr %3124, ptr %3125)
  call void @llvm.stackrestore.p0(ptr %3119)
  %3126 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 3, i32 534, i32 117)
  %3127 = call ptr @llvm.stacksave.p0()
  %3128 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2874, ptr %3128, align 8
  %3129 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3128, 1
  %3130 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %3130, align 8
  %3131 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3130, 1
  %3132 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3129, ptr %3132, align 8
  %3133 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3131, ptr %3133, align 8
  call void @memrefCopy(i64 4, ptr %3132, ptr %3133)
  call void @llvm.stackrestore.p0(ptr %3127)
  %3134 = call ptr @llvm.stacksave.p0()
  %3135 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2275, ptr %3135, align 8
  %3136 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3135, 1
  %3137 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %3137, align 8
  %3138 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3137, 1
  %3139 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3136, ptr %3139, align 8
  %3140 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3138, ptr %3140, align 8
  call void @memrefCopy(i64 4, ptr %3139, ptr %3140)
  call void @llvm.stackrestore.p0(ptr %3134)
  %3141 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 3, i32 603, i32 117)
  %3142 = call ptr @llvm.stacksave.p0()
  %3143 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %3143, align 8
  %3144 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3143, 1
  %3145 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3104, ptr %3145, align 8
  %3146 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3145, 1
  %3147 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3144, ptr %3147, align 8
  %3148 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3146, ptr %3148, align 8
  call void @memrefCopy(i64 4, ptr %3147, ptr %3148)
  call void @llvm.stackrestore.p0(ptr %3142)
  %3149 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %3150 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %3151 = insertvalue { ptr, ptr, i64 } poison, ptr %3149, 0
  %3152 = insertvalue { ptr, ptr, i64 } %3151, ptr %3150, 1
  %3153 = insertvalue { ptr, ptr, i64 } %3152, i64 0, 2
  %3154 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %3155 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %3156 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %3157 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %3158 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %3159 = extractvalue { ptr, ptr, i64 } %3153, 0
  %3160 = extractvalue { ptr, ptr, i64 } %3153, 1
  %3161 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %3159, 0
  %3162 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3161, ptr %3160, 1
  %3163 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3162, i64 1324, 2
  %3164 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3163, i64 4, 3, 0
  %3165 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3164, i64 64, 4, 0
  %3166 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3165, i64 4, 3, 1
  %3167 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3166, i64 1, 4, 1
  %3168 = call ptr @llvm.stacksave.p0()
  %3169 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3167, ptr %3169, align 8
  %3170 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3169, 1
  %3171 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %3171, align 8
  %3172 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3171, 1
  %3173 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3170, ptr %3173, align 8
  %3174 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3172, ptr %3174, align 8
  call void @memrefCopy(i64 4, ptr %3173, ptr %3174)
  call void @llvm.stackrestore.p0(ptr %3168)
  %3175 = call ptr @llvm.stacksave.p0()
  %3176 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2840, ptr %3176, align 8
  %3177 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3176, 1
  %3178 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %3178, align 8
  %3179 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3178, 1
  %3180 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3177, ptr %3180, align 8
  %3181 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3179, ptr %3181, align 8
  call void @memrefCopy(i64 4, ptr %3180, ptr %3181)
  call void @llvm.stackrestore.p0(ptr %3175)
  %3182 = call ptr @llvm.stacksave.p0()
  %3183 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2342, ptr %3183, align 8
  %3184 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3183, 1
  %3185 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %3185, align 8
  %3186 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3185, 1
  %3187 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3184, ptr %3187, align 8
  %3188 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3186, ptr %3188, align 8
  call void @memrefCopy(i64 4, ptr %3187, ptr %3188)
  call void @llvm.stackrestore.p0(ptr %3182)
  %3189 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 1, i32 672, i32 117)
  %3190 = call ptr @llvm.stacksave.p0()
  %3191 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2874, ptr %3191, align 8
  %3192 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3191, 1
  %3193 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %3193, align 8
  %3194 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3193, 1
  %3195 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3192, ptr %3195, align 8
  %3196 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3194, ptr %3196, align 8
  call void @memrefCopy(i64 4, ptr %3195, ptr %3196)
  call void @llvm.stackrestore.p0(ptr %3190)
  %3197 = call ptr @llvm.stacksave.p0()
  %3198 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2376, ptr %3198, align 8
  %3199 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3198, 1
  %3200 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %3200, align 8
  %3201 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3200, 1
  %3202 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3199, ptr %3202, align 8
  %3203 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3201, ptr %3203, align 8
  call void @memrefCopy(i64 4, ptr %3202, ptr %3203)
  call void @llvm.stackrestore.p0(ptr %3197)
  %3204 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 1, i32 741, i32 117)
  %3205 = call ptr @llvm.stacksave.p0()
  %3206 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %3206, align 8
  %3207 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3206, 1
  %3208 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3167, ptr %3208, align 8
  %3209 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3208, 1
  %3210 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3207, ptr %3210, align 8
  %3211 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3209, ptr %3211, align 8
  call void @memrefCopy(i64 4, ptr %3210, ptr %3211)
  call void @llvm.stackrestore.p0(ptr %3205)
  %3212 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %3213 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %3214 = insertvalue { ptr, ptr, i64 } poison, ptr %3212, 0
  %3215 = insertvalue { ptr, ptr, i64 } %3214, ptr %3213, 1
  %3216 = insertvalue { ptr, ptr, i64 } %3215, i64 0, 2
  %3217 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %3218 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %3219 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %3220 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %3221 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %3222 = extractvalue { ptr, ptr, i64 } %3216, 0
  %3223 = extractvalue { ptr, ptr, i64 } %3216, 1
  %3224 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %3222, 0
  %3225 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3224, ptr %3223, 1
  %3226 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3225, i64 1328, 2
  %3227 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3226, i64 4, 3, 0
  %3228 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3227, i64 64, 4, 0
  %3229 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3228, i64 4, 3, 1
  %3230 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3229, i64 1, 4, 1
  %3231 = call ptr @llvm.stacksave.p0()
  %3232 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3230, ptr %3232, align 8
  %3233 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3232, 1
  %3234 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %3234, align 8
  %3235 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3234, 1
  %3236 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3233, ptr %3236, align 8
  %3237 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3235, ptr %3237, align 8
  call void @memrefCopy(i64 4, ptr %3236, ptr %3237)
  call void @llvm.stackrestore.p0(ptr %3231)
  %3238 = call ptr @llvm.stacksave.p0()
  %3239 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2840, ptr %3239, align 8
  %3240 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3239, 1
  %3241 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %3241, align 8
  %3242 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3241, 1
  %3243 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3240, ptr %3243, align 8
  %3244 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3242, ptr %3244, align 8
  call void @memrefCopy(i64 4, ptr %3243, ptr %3244)
  call void @llvm.stackrestore.p0(ptr %3238)
  %3245 = call ptr @llvm.stacksave.p0()
  %3246 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2443, ptr %3246, align 8
  %3247 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3246, 1
  %3248 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %3248, align 8
  %3249 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3248, 1
  %3250 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3247, ptr %3250, align 8
  %3251 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3249, ptr %3251, align 8
  call void @memrefCopy(i64 4, ptr %3250, ptr %3251)
  call void @llvm.stackrestore.p0(ptr %3245)
  %3252 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 2, i32 672, i32 117)
  %3253 = call ptr @llvm.stacksave.p0()
  %3254 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2874, ptr %3254, align 8
  %3255 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3254, 1
  %3256 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %3256, align 8
  %3257 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3256, 1
  %3258 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3255, ptr %3258, align 8
  %3259 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3257, ptr %3259, align 8
  call void @memrefCopy(i64 4, ptr %3258, ptr %3259)
  call void @llvm.stackrestore.p0(ptr %3253)
  %3260 = call ptr @llvm.stacksave.p0()
  %3261 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2477, ptr %3261, align 8
  %3262 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3261, 1
  %3263 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %3263, align 8
  %3264 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3263, 1
  %3265 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3262, ptr %3265, align 8
  %3266 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3264, ptr %3266, align 8
  call void @memrefCopy(i64 4, ptr %3265, ptr %3266)
  call void @llvm.stackrestore.p0(ptr %3260)
  %3267 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 2, i32 741, i32 117)
  %3268 = call ptr @llvm.stacksave.p0()
  %3269 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %3269, align 8
  %3270 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3269, 1
  %3271 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3230, ptr %3271, align 8
  %3272 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3271, 1
  %3273 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3270, ptr %3273, align 8
  %3274 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3272, ptr %3274, align 8
  call void @memrefCopy(i64 4, ptr %3273, ptr %3274)
  call void @llvm.stackrestore.p0(ptr %3268)
  %3275 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %3276 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %3277 = insertvalue { ptr, ptr, i64 } poison, ptr %3275, 0
  %3278 = insertvalue { ptr, ptr, i64 } %3277, ptr %3276, 1
  %3279 = insertvalue { ptr, ptr, i64 } %3278, i64 0, 2
  %3280 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %3281 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %3282 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %3283 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %3284 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %3285 = extractvalue { ptr, ptr, i64 } %3279, 0
  %3286 = extractvalue { ptr, ptr, i64 } %3279, 1
  %3287 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %3285, 0
  %3288 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3287, ptr %3286, 1
  %3289 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3288, i64 1332, 2
  %3290 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3289, i64 4, 3, 0
  %3291 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3290, i64 64, 4, 0
  %3292 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3291, i64 4, 3, 1
  %3293 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3292, i64 1, 4, 1
  %3294 = call ptr @llvm.stacksave.p0()
  %3295 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3293, ptr %3295, align 8
  %3296 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3295, 1
  %3297 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %3297, align 8
  %3298 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3297, 1
  %3299 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3296, ptr %3299, align 8
  %3300 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3298, ptr %3300, align 8
  call void @memrefCopy(i64 4, ptr %3299, ptr %3300)
  call void @llvm.stackrestore.p0(ptr %3294)
  %3301 = call ptr @llvm.stacksave.p0()
  %3302 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2840, ptr %3302, align 8
  %3303 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3302, 1
  %3304 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %3304, align 8
  %3305 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3304, 1
  %3306 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3303, ptr %3306, align 8
  %3307 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3305, ptr %3307, align 8
  call void @memrefCopy(i64 4, ptr %3306, ptr %3307)
  call void @llvm.stackrestore.p0(ptr %3301)
  %3308 = call ptr @llvm.stacksave.p0()
  %3309 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2544, ptr %3309, align 8
  %3310 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3309, 1
  %3311 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %3311, align 8
  %3312 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3311, 1
  %3313 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3310, ptr %3313, align 8
  %3314 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3312, ptr %3314, align 8
  call void @memrefCopy(i64 4, ptr %3313, ptr %3314)
  call void @llvm.stackrestore.p0(ptr %3308)
  %3315 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 3, i32 672, i32 117)
  %3316 = call ptr @llvm.stacksave.p0()
  %3317 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2874, ptr %3317, align 8
  %3318 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3317, 1
  %3319 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %3319, align 8
  %3320 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3319, 1
  %3321 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3318, ptr %3321, align 8
  %3322 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3320, ptr %3322, align 8
  call void @memrefCopy(i64 4, ptr %3321, ptr %3322)
  call void @llvm.stackrestore.p0(ptr %3316)
  %3323 = call ptr @llvm.stacksave.p0()
  %3324 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2578, ptr %3324, align 8
  %3325 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3324, 1
  %3326 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %3326, align 8
  %3327 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3326, 1
  %3328 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3325, ptr %3328, align 8
  %3329 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3327, ptr %3329, align 8
  call void @memrefCopy(i64 4, ptr %3328, ptr %3329)
  call void @llvm.stackrestore.p0(ptr %3323)
  %3330 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 3, i32 741, i32 117)
  %3331 = call ptr @llvm.stacksave.p0()
  %3332 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %3332, align 8
  %3333 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3332, 1
  %3334 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3293, ptr %3334, align 8
  %3335 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3334, 1
  %3336 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3333, ptr %3336, align 8
  %3337 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3335, ptr %3337, align 8
  call void @memrefCopy(i64 4, ptr %3336, ptr %3337)
  call void @llvm.stackrestore.p0(ptr %3331)
  %3338 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %3339 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %3340 = insertvalue { ptr, ptr, i64 } poison, ptr %3338, 0
  %3341 = insertvalue { ptr, ptr, i64 } %3340, ptr %3339, 1
  %3342 = insertvalue { ptr, ptr, i64 } %3341, i64 0, 2
  %3343 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %3344 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %3345 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %3346 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %3347 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %3348 = extractvalue { ptr, ptr, i64 } %3342, 0
  %3349 = extractvalue { ptr, ptr, i64 } %3342, 1
  %3350 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %3348, 0
  %3351 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3350, ptr %3349, 1
  %3352 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3351, i64 1336, 2
  %3353 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3352, i64 4, 3, 0
  %3354 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3353, i64 64, 4, 0
  %3355 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3354, i64 4, 3, 1
  %3356 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3355, i64 1, 4, 1
  %3357 = call ptr @llvm.stacksave.p0()
  %3358 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3356, ptr %3358, align 8
  %3359 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3358, 1
  %3360 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %3360, align 8
  %3361 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3360, 1
  %3362 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3359, ptr %3362, align 8
  %3363 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3361, ptr %3363, align 8
  call void @memrefCopy(i64 4, ptr %3362, ptr %3363)
  call void @llvm.stackrestore.p0(ptr %3357)
  %3364 = call ptr @llvm.stacksave.p0()
  %3365 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2840, ptr %3365, align 8
  %3366 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3365, 1
  %3367 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %3367, align 8
  %3368 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3367, 1
  %3369 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3366, ptr %3369, align 8
  %3370 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3368, ptr %3370, align 8
  call void @memrefCopy(i64 4, ptr %3369, ptr %3370)
  call void @llvm.stackrestore.p0(ptr %3364)
  %3371 = call ptr @llvm.stacksave.p0()
  %3372 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2645, ptr %3372, align 8
  %3373 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3372, 1
  %3374 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %3374, align 8
  %3375 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3374, 1
  %3376 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3373, ptr %3376, align 8
  %3377 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3375, ptr %3377, align 8
  call void @memrefCopy(i64 4, ptr %3376, ptr %3377)
  call void @llvm.stackrestore.p0(ptr %3371)
  %3378 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 1, i32 810, i32 117)
  %3379 = call ptr @llvm.stacksave.p0()
  %3380 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2874, ptr %3380, align 8
  %3381 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3380, 1
  %3382 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %3382, align 8
  %3383 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3382, 1
  %3384 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3381, ptr %3384, align 8
  %3385 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3383, ptr %3385, align 8
  call void @memrefCopy(i64 4, ptr %3384, ptr %3385)
  call void @llvm.stackrestore.p0(ptr %3379)
  %3386 = call ptr @llvm.stacksave.p0()
  %3387 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2679, ptr %3387, align 8
  %3388 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3387, 1
  %3389 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %3389, align 8
  %3390 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3389, 1
  %3391 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3388, ptr %3391, align 8
  %3392 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3390, ptr %3392, align 8
  call void @memrefCopy(i64 4, ptr %3391, ptr %3392)
  call void @llvm.stackrestore.p0(ptr %3386)
  %3393 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 1, i32 879, i32 117)
  %3394 = call ptr @llvm.stacksave.p0()
  %3395 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %3395, align 8
  %3396 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3395, 1
  %3397 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3356, ptr %3397, align 8
  %3398 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3397, 1
  %3399 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3396, ptr %3399, align 8
  %3400 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3398, ptr %3400, align 8
  call void @memrefCopy(i64 4, ptr %3399, ptr %3400)
  call void @llvm.stackrestore.p0(ptr %3394)
  %3401 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %3402 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %3403 = insertvalue { ptr, ptr, i64 } poison, ptr %3401, 0
  %3404 = insertvalue { ptr, ptr, i64 } %3403, ptr %3402, 1
  %3405 = insertvalue { ptr, ptr, i64 } %3404, i64 0, 2
  %3406 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %3407 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %3408 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %3409 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %3410 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %3411 = extractvalue { ptr, ptr, i64 } %3405, 0
  %3412 = extractvalue { ptr, ptr, i64 } %3405, 1
  %3413 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %3411, 0
  %3414 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3413, ptr %3412, 1
  %3415 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3414, i64 1340, 2
  %3416 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3415, i64 4, 3, 0
  %3417 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3416, i64 64, 4, 0
  %3418 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3417, i64 4, 3, 1
  %3419 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3418, i64 1, 4, 1
  %3420 = call ptr @llvm.stacksave.p0()
  %3421 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3419, ptr %3421, align 8
  %3422 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3421, 1
  %3423 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %3423, align 8
  %3424 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3423, 1
  %3425 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3422, ptr %3425, align 8
  %3426 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3424, ptr %3426, align 8
  call void @memrefCopy(i64 4, ptr %3425, ptr %3426)
  call void @llvm.stackrestore.p0(ptr %3420)
  %3427 = call ptr @llvm.stacksave.p0()
  %3428 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2840, ptr %3428, align 8
  %3429 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3428, 1
  %3430 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %3430, align 8
  %3431 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3430, 1
  %3432 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3429, ptr %3432, align 8
  %3433 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3431, ptr %3433, align 8
  call void @memrefCopy(i64 4, ptr %3432, ptr %3433)
  call void @llvm.stackrestore.p0(ptr %3427)
  %3434 = call ptr @llvm.stacksave.p0()
  %3435 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2746, ptr %3435, align 8
  %3436 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3435, 1
  %3437 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %3437, align 8
  %3438 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3437, 1
  %3439 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3436, ptr %3439, align 8
  %3440 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3438, ptr %3440, align 8
  call void @memrefCopy(i64 4, ptr %3439, ptr %3440)
  call void @llvm.stackrestore.p0(ptr %3434)
  %3441 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 2, i32 810, i32 117)
  %3442 = call ptr @llvm.stacksave.p0()
  %3443 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2874, ptr %3443, align 8
  %3444 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3443, 1
  %3445 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %3445, align 8
  %3446 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3445, 1
  %3447 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3444, ptr %3447, align 8
  %3448 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3446, ptr %3448, align 8
  call void @memrefCopy(i64 4, ptr %3447, ptr %3448)
  call void @llvm.stackrestore.p0(ptr %3442)
  %3449 = call ptr @llvm.stacksave.p0()
  %3450 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2780, ptr %3450, align 8
  %3451 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3450, 1
  %3452 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %3452, align 8
  %3453 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3452, 1
  %3454 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3451, ptr %3454, align 8
  %3455 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3453, ptr %3455, align 8
  call void @memrefCopy(i64 4, ptr %3454, ptr %3455)
  call void @llvm.stackrestore.p0(ptr %3449)
  %3456 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 2, i32 879, i32 117)
  %3457 = call ptr @llvm.stacksave.p0()
  %3458 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %3458, align 8
  %3459 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3458, 1
  %3460 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3419, ptr %3460, align 8
  %3461 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3460, 1
  %3462 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3459, ptr %3462, align 8
  %3463 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3461, ptr %3463, align 8
  call void @memrefCopy(i64 4, ptr %3462, ptr %3463)
  call void @llvm.stackrestore.p0(ptr %3457)
  %3464 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %3465 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %3466 = insertvalue { ptr, ptr, i64 } poison, ptr %3464, 0
  %3467 = insertvalue { ptr, ptr, i64 } %3466, ptr %3465, 1
  %3468 = insertvalue { ptr, ptr, i64 } %3467, i64 0, 2
  %3469 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %3470 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %3471 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %3472 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %3473 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %3474 = extractvalue { ptr, ptr, i64 } %3468, 0
  %3475 = extractvalue { ptr, ptr, i64 } %3468, 1
  %3476 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %3474, 0
  %3477 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3476, ptr %3475, 1
  %3478 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3477, i64 1536, 2
  %3479 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3478, i64 4, 3, 0
  %3480 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3479, i64 64, 4, 0
  %3481 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3480, i64 4, 3, 1
  %3482 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3481, i64 1, 4, 1
  %3483 = call ptr @llvm.stacksave.p0()
  %3484 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3482, ptr %3484, align 8
  %3485 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3484, 1
  %3486 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %3486, align 8
  %3487 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3486, 1
  %3488 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3485, ptr %3488, align 8
  %3489 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3487, ptr %3489, align 8
  call void @memrefCopy(i64 4, ptr %3488, ptr %3489)
  call void @llvm.stackrestore.p0(ptr %3483)
  %3490 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %3491 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %3492 = insertvalue { ptr, ptr, i64 } poison, ptr %3490, 0
  %3493 = insertvalue { ptr, ptr, i64 } %3492, ptr %3491, 1
  %3494 = insertvalue { ptr, ptr, i64 } %3493, i64 0, 2
  %3495 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %3496 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %3497 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %3498 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %3499 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %3500 = extractvalue { ptr, ptr, i64 } %3494, 0
  %3501 = extractvalue { ptr, ptr, i64 } %3494, 1
  %3502 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %3500, 0
  %3503 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3502, ptr %3501, 1
  %3504 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3503, i64 768, 2
  %3505 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3504, i64 4, 3, 0
  %3506 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3505, i64 32, 4, 0
  %3507 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3506, i64 16, 3, 1
  %3508 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3507, i64 1, 4, 1
  %3509 = call ptr @llvm.stacksave.p0()
  %3510 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3508, ptr %3510, align 8
  %3511 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3510, 1
  %3512 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %3512, align 8
  %3513 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3512, 1
  %3514 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3511, ptr %3514, align 8
  %3515 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3513, ptr %3515, align 8
  call void @memrefCopy(i64 4, ptr %3514, ptr %3515)
  call void @llvm.stackrestore.p0(ptr %3509)
  %3516 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %3517 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %3518 = insertvalue { ptr, ptr, i64 } poison, ptr %3516, 0
  %3519 = insertvalue { ptr, ptr, i64 } %3518, ptr %3517, 1
  %3520 = insertvalue { ptr, ptr, i64 } %3519, i64 0, 2
  %3521 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %3522 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %3523 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %3524 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %3525 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %3526 = extractvalue { ptr, ptr, i64 } %3520, 0
  %3527 = extractvalue { ptr, ptr, i64 } %3520, 1
  %3528 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %3526, 0
  %3529 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3528, ptr %3527, 1
  %3530 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3529, i64 0, 2
  %3531 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3530, i64 16, 3, 0
  %3532 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3531, i64 64, 4, 0
  %3533 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3532, i64 4, 3, 1
  %3534 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3533, i64 1, 4, 1
  %3535 = call ptr @llvm.stacksave.p0()
  %3536 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3534, ptr %3536, align 8
  %3537 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3536, 1
  %3538 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %3538, align 8
  %3539 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3538, 1
  %3540 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3537, ptr %3540, align 8
  %3541 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3539, ptr %3541, align 8
  call void @memrefCopy(i64 4, ptr %3540, ptr %3541)
  call void @llvm.stackrestore.p0(ptr %3535)
  %3542 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 3, i32 810, i32 117)
  %3543 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %3544 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %3545 = insertvalue { ptr, ptr, i64 } poison, ptr %3543, 0
  %3546 = insertvalue { ptr, ptr, i64 } %3545, ptr %3544, 1
  %3547 = insertvalue { ptr, ptr, i64 } %3546, i64 0, 2
  %3548 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %3549 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %3550 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %3551 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %3552 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %3553 = extractvalue { ptr, ptr, i64 } %3547, 0
  %3554 = extractvalue { ptr, ptr, i64 } %3547, 1
  %3555 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %3553, 0
  %3556 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3555, ptr %3554, 1
  %3557 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3556, i64 784, 2
  %3558 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3557, i64 4, 3, 0
  %3559 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3558, i64 32, 4, 0
  %3560 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3559, i64 16, 3, 1
  %3561 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3560, i64 1, 4, 1
  %3562 = call ptr @llvm.stacksave.p0()
  %3563 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3561, ptr %3563, align 8
  %3564 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3563, 1
  %3565 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %3565, align 8
  %3566 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3565, 1
  %3567 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3564, ptr %3567, align 8
  %3568 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3566, ptr %3568, align 8
  call void @memrefCopy(i64 4, ptr %3567, ptr %3568)
  call void @llvm.stackrestore.p0(ptr %3562)
  %3569 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %3570 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %3571 = insertvalue { ptr, ptr, i64 } poison, ptr %3569, 0
  %3572 = insertvalue { ptr, ptr, i64 } %3571, ptr %3570, 1
  %3573 = insertvalue { ptr, ptr, i64 } %3572, i64 0, 2
  %3574 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %3575 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %3576 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %3577 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %3578 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %3579 = extractvalue { ptr, ptr, i64 } %3573, 0
  %3580 = extractvalue { ptr, ptr, i64 } %3573, 1
  %3581 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %3579, 0
  %3582 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3581, ptr %3580, 1
  %3583 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3582, i64 1024, 2
  %3584 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3583, i64 16, 3, 0
  %3585 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3584, i64 64, 4, 0
  %3586 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3585, i64 4, 3, 1
  %3587 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3586, i64 1, 4, 1
  %3588 = call ptr @llvm.stacksave.p0()
  %3589 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3587, ptr %3589, align 8
  %3590 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3589, 1
  %3591 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %3591, align 8
  %3592 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3591, 1
  %3593 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3590, ptr %3593, align 8
  %3594 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3592, ptr %3594, align 8
  call void @memrefCopy(i64 4, ptr %3593, ptr %3594)
  call void @llvm.stackrestore.p0(ptr %3588)
  %3595 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 3, i32 879, i32 117)
  %3596 = call ptr @llvm.stacksave.p0()
  %3597 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %3597, align 8
  %3598 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3597, 1
  %3599 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3482, ptr %3599, align 8
  %3600 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3599, 1
  %3601 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3598, ptr %3601, align 8
  %3602 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3600, ptr %3602, align 8
  call void @memrefCopy(i64 4, ptr %3601, ptr %3602)
  call void @llvm.stackrestore.p0(ptr %3596)
  %3603 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %3604 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %3605 = insertvalue { ptr, ptr, i64 } poison, ptr %3603, 0
  %3606 = insertvalue { ptr, ptr, i64 } %3605, ptr %3604, 1
  %3607 = insertvalue { ptr, ptr, i64 } %3606, i64 0, 2
  %3608 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %3609 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %3610 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %3611 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %3612 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %3613 = extractvalue { ptr, ptr, i64 } %3607, 0
  %3614 = extractvalue { ptr, ptr, i64 } %3607, 1
  %3615 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %3613, 0
  %3616 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3615, ptr %3614, 1
  %3617 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3616, i64 1540, 2
  %3618 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3617, i64 4, 3, 0
  %3619 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3618, i64 64, 4, 0
  %3620 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3619, i64 4, 3, 1
  %3621 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3620, i64 1, 4, 1
  %3622 = call ptr @llvm.stacksave.p0()
  %3623 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3621, ptr %3623, align 8
  %3624 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3623, 1
  %3625 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %3625, align 8
  %3626 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3625, 1
  %3627 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3624, ptr %3627, align 8
  %3628 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3626, ptr %3628, align 8
  call void @memrefCopy(i64 4, ptr %3627, ptr %3628)
  call void @llvm.stackrestore.p0(ptr %3622)
  %3629 = call ptr @llvm.stacksave.p0()
  %3630 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3508, ptr %3630, align 8
  %3631 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3630, 1
  %3632 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %3632, align 8
  %3633 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3632, 1
  %3634 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3631, ptr %3634, align 8
  %3635 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3633, ptr %3635, align 8
  call void @memrefCopy(i64 4, ptr %3634, ptr %3635)
  call void @llvm.stackrestore.p0(ptr %3629)
  %3636 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %3637 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %3638 = insertvalue { ptr, ptr, i64 } poison, ptr %3636, 0
  %3639 = insertvalue { ptr, ptr, i64 } %3638, ptr %3637, 1
  %3640 = insertvalue { ptr, ptr, i64 } %3639, i64 0, 2
  %3641 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %3642 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %3643 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %3644 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %3645 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %3646 = extractvalue { ptr, ptr, i64 } %3640, 0
  %3647 = extractvalue { ptr, ptr, i64 } %3640, 1
  %3648 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %3646, 0
  %3649 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3648, ptr %3647, 1
  %3650 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3649, i64 4, 2
  %3651 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3650, i64 16, 3, 0
  %3652 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3651, i64 64, 4, 0
  %3653 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3652, i64 4, 3, 1
  %3654 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3653, i64 1, 4, 1
  %3655 = call ptr @llvm.stacksave.p0()
  %3656 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3654, ptr %3656, align 8
  %3657 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3656, 1
  %3658 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %3658, align 8
  %3659 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3658, 1
  %3660 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3657, ptr %3660, align 8
  %3661 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3659, ptr %3661, align 8
  call void @memrefCopy(i64 4, ptr %3660, ptr %3661)
  call void @llvm.stackrestore.p0(ptr %3655)
  %3662 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 1, i32 948, i32 117)
  %3663 = call ptr @llvm.stacksave.p0()
  %3664 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3561, ptr %3664, align 8
  %3665 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3664, 1
  %3666 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %3666, align 8
  %3667 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3666, 1
  %3668 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3665, ptr %3668, align 8
  %3669 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3667, ptr %3669, align 8
  call void @memrefCopy(i64 4, ptr %3668, ptr %3669)
  call void @llvm.stackrestore.p0(ptr %3663)
  %3670 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %3671 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %3672 = insertvalue { ptr, ptr, i64 } poison, ptr %3670, 0
  %3673 = insertvalue { ptr, ptr, i64 } %3672, ptr %3671, 1
  %3674 = insertvalue { ptr, ptr, i64 } %3673, i64 0, 2
  %3675 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %3676 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %3677 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %3678 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %3679 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %3680 = extractvalue { ptr, ptr, i64 } %3674, 0
  %3681 = extractvalue { ptr, ptr, i64 } %3674, 1
  %3682 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %3680, 0
  %3683 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3682, ptr %3681, 1
  %3684 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3683, i64 1028, 2
  %3685 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3684, i64 16, 3, 0
  %3686 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3685, i64 64, 4, 0
  %3687 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3686, i64 4, 3, 1
  %3688 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3687, i64 1, 4, 1
  %3689 = call ptr @llvm.stacksave.p0()
  %3690 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3688, ptr %3690, align 8
  %3691 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3690, 1
  %3692 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %3692, align 8
  %3693 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3692, 1
  %3694 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3691, ptr %3694, align 8
  %3695 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3693, ptr %3695, align 8
  call void @memrefCopy(i64 4, ptr %3694, ptr %3695)
  call void @llvm.stackrestore.p0(ptr %3689)
  %3696 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 1, i32 1017, i32 117)
  %3697 = call ptr @llvm.stacksave.p0()
  %3698 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %3698, align 8
  %3699 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3698, 1
  %3700 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3621, ptr %3700, align 8
  %3701 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3700, 1
  %3702 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3699, ptr %3702, align 8
  %3703 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3701, ptr %3703, align 8
  call void @memrefCopy(i64 4, ptr %3702, ptr %3703)
  call void @llvm.stackrestore.p0(ptr %3697)
  %3704 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %3705 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %3706 = insertvalue { ptr, ptr, i64 } poison, ptr %3704, 0
  %3707 = insertvalue { ptr, ptr, i64 } %3706, ptr %3705, 1
  %3708 = insertvalue { ptr, ptr, i64 } %3707, i64 0, 2
  %3709 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %3710 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %3711 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %3712 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %3713 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %3714 = extractvalue { ptr, ptr, i64 } %3708, 0
  %3715 = extractvalue { ptr, ptr, i64 } %3708, 1
  %3716 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %3714, 0
  %3717 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3716, ptr %3715, 1
  %3718 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3717, i64 1544, 2
  %3719 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3718, i64 4, 3, 0
  %3720 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3719, i64 64, 4, 0
  %3721 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3720, i64 4, 3, 1
  %3722 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3721, i64 1, 4, 1
  %3723 = call ptr @llvm.stacksave.p0()
  %3724 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3722, ptr %3724, align 8
  %3725 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3724, 1
  %3726 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %3726, align 8
  %3727 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3726, 1
  %3728 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3725, ptr %3728, align 8
  %3729 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3727, ptr %3729, align 8
  call void @memrefCopy(i64 4, ptr %3728, ptr %3729)
  call void @llvm.stackrestore.p0(ptr %3723)
  %3730 = call ptr @llvm.stacksave.p0()
  %3731 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3508, ptr %3731, align 8
  %3732 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3731, 1
  %3733 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %3733, align 8
  %3734 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3733, 1
  %3735 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3732, ptr %3735, align 8
  %3736 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3734, ptr %3736, align 8
  call void @memrefCopy(i64 4, ptr %3735, ptr %3736)
  call void @llvm.stackrestore.p0(ptr %3730)
  %3737 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %3738 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %3739 = insertvalue { ptr, ptr, i64 } poison, ptr %3737, 0
  %3740 = insertvalue { ptr, ptr, i64 } %3739, ptr %3738, 1
  %3741 = insertvalue { ptr, ptr, i64 } %3740, i64 0, 2
  %3742 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %3743 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %3744 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %3745 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %3746 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %3747 = extractvalue { ptr, ptr, i64 } %3741, 0
  %3748 = extractvalue { ptr, ptr, i64 } %3741, 1
  %3749 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %3747, 0
  %3750 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3749, ptr %3748, 1
  %3751 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3750, i64 8, 2
  %3752 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3751, i64 16, 3, 0
  %3753 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3752, i64 64, 4, 0
  %3754 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3753, i64 4, 3, 1
  %3755 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3754, i64 1, 4, 1
  %3756 = call ptr @llvm.stacksave.p0()
  %3757 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3755, ptr %3757, align 8
  %3758 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3757, 1
  %3759 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %3759, align 8
  %3760 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3759, 1
  %3761 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3758, ptr %3761, align 8
  %3762 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3760, ptr %3762, align 8
  call void @memrefCopy(i64 4, ptr %3761, ptr %3762)
  call void @llvm.stackrestore.p0(ptr %3756)
  %3763 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 2, i32 948, i32 117)
  %3764 = call ptr @llvm.stacksave.p0()
  %3765 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3561, ptr %3765, align 8
  %3766 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3765, 1
  %3767 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %3767, align 8
  %3768 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3767, 1
  %3769 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3766, ptr %3769, align 8
  %3770 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3768, ptr %3770, align 8
  call void @memrefCopy(i64 4, ptr %3769, ptr %3770)
  call void @llvm.stackrestore.p0(ptr %3764)
  %3771 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %3772 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %3773 = insertvalue { ptr, ptr, i64 } poison, ptr %3771, 0
  %3774 = insertvalue { ptr, ptr, i64 } %3773, ptr %3772, 1
  %3775 = insertvalue { ptr, ptr, i64 } %3774, i64 0, 2
  %3776 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %3777 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %3778 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %3779 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %3780 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %3781 = extractvalue { ptr, ptr, i64 } %3775, 0
  %3782 = extractvalue { ptr, ptr, i64 } %3775, 1
  %3783 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %3781, 0
  %3784 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3783, ptr %3782, 1
  %3785 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3784, i64 1032, 2
  %3786 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3785, i64 16, 3, 0
  %3787 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3786, i64 64, 4, 0
  %3788 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3787, i64 4, 3, 1
  %3789 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3788, i64 1, 4, 1
  %3790 = call ptr @llvm.stacksave.p0()
  %3791 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3789, ptr %3791, align 8
  %3792 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3791, 1
  %3793 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %3793, align 8
  %3794 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3793, 1
  %3795 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3792, ptr %3795, align 8
  %3796 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3794, ptr %3796, align 8
  call void @memrefCopy(i64 4, ptr %3795, ptr %3796)
  call void @llvm.stackrestore.p0(ptr %3790)
  %3797 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 2, i32 1017, i32 117)
  %3798 = call ptr @llvm.stacksave.p0()
  %3799 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %3799, align 8
  %3800 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3799, 1
  %3801 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3722, ptr %3801, align 8
  %3802 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3801, 1
  %3803 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3800, ptr %3803, align 8
  %3804 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3802, ptr %3804, align 8
  call void @memrefCopy(i64 4, ptr %3803, ptr %3804)
  call void @llvm.stackrestore.p0(ptr %3798)
  %3805 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %3806 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %3807 = insertvalue { ptr, ptr, i64 } poison, ptr %3805, 0
  %3808 = insertvalue { ptr, ptr, i64 } %3807, ptr %3806, 1
  %3809 = insertvalue { ptr, ptr, i64 } %3808, i64 0, 2
  %3810 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %3811 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %3812 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %3813 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %3814 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %3815 = extractvalue { ptr, ptr, i64 } %3809, 0
  %3816 = extractvalue { ptr, ptr, i64 } %3809, 1
  %3817 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %3815, 0
  %3818 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3817, ptr %3816, 1
  %3819 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3818, i64 1548, 2
  %3820 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3819, i64 4, 3, 0
  %3821 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3820, i64 64, 4, 0
  %3822 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3821, i64 4, 3, 1
  %3823 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3822, i64 1, 4, 1
  %3824 = call ptr @llvm.stacksave.p0()
  %3825 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3823, ptr %3825, align 8
  %3826 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3825, 1
  %3827 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %3827, align 8
  %3828 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3827, 1
  %3829 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3826, ptr %3829, align 8
  %3830 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3828, ptr %3830, align 8
  call void @memrefCopy(i64 4, ptr %3829, ptr %3830)
  call void @llvm.stackrestore.p0(ptr %3824)
  %3831 = call ptr @llvm.stacksave.p0()
  %3832 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3508, ptr %3832, align 8
  %3833 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3832, 1
  %3834 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %3834, align 8
  %3835 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3834, 1
  %3836 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3833, ptr %3836, align 8
  %3837 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3835, ptr %3837, align 8
  call void @memrefCopy(i64 4, ptr %3836, ptr %3837)
  call void @llvm.stackrestore.p0(ptr %3831)
  %3838 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %3839 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %3840 = insertvalue { ptr, ptr, i64 } poison, ptr %3838, 0
  %3841 = insertvalue { ptr, ptr, i64 } %3840, ptr %3839, 1
  %3842 = insertvalue { ptr, ptr, i64 } %3841, i64 0, 2
  %3843 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %3844 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %3845 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %3846 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %3847 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %3848 = extractvalue { ptr, ptr, i64 } %3842, 0
  %3849 = extractvalue { ptr, ptr, i64 } %3842, 1
  %3850 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %3848, 0
  %3851 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3850, ptr %3849, 1
  %3852 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3851, i64 12, 2
  %3853 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3852, i64 16, 3, 0
  %3854 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3853, i64 64, 4, 0
  %3855 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3854, i64 4, 3, 1
  %3856 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3855, i64 1, 4, 1
  %3857 = call ptr @llvm.stacksave.p0()
  %3858 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3856, ptr %3858, align 8
  %3859 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3858, 1
  %3860 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %3860, align 8
  %3861 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3860, 1
  %3862 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3859, ptr %3862, align 8
  %3863 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3861, ptr %3863, align 8
  call void @memrefCopy(i64 4, ptr %3862, ptr %3863)
  call void @llvm.stackrestore.p0(ptr %3857)
  %3864 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 3, i32 948, i32 117)
  %3865 = call ptr @llvm.stacksave.p0()
  %3866 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3561, ptr %3866, align 8
  %3867 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3866, 1
  %3868 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %3868, align 8
  %3869 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3868, 1
  %3870 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3867, ptr %3870, align 8
  %3871 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3869, ptr %3871, align 8
  call void @memrefCopy(i64 4, ptr %3870, ptr %3871)
  call void @llvm.stackrestore.p0(ptr %3865)
  %3872 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %3873 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %3874 = insertvalue { ptr, ptr, i64 } poison, ptr %3872, 0
  %3875 = insertvalue { ptr, ptr, i64 } %3874, ptr %3873, 1
  %3876 = insertvalue { ptr, ptr, i64 } %3875, i64 0, 2
  %3877 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %3878 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %3879 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %3880 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %3881 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %3882 = extractvalue { ptr, ptr, i64 } %3876, 0
  %3883 = extractvalue { ptr, ptr, i64 } %3876, 1
  %3884 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %3882, 0
  %3885 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3884, ptr %3883, 1
  %3886 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3885, i64 1036, 2
  %3887 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3886, i64 16, 3, 0
  %3888 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3887, i64 64, 4, 0
  %3889 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3888, i64 4, 3, 1
  %3890 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3889, i64 1, 4, 1
  %3891 = call ptr @llvm.stacksave.p0()
  %3892 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3890, ptr %3892, align 8
  %3893 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3892, 1
  %3894 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %3894, align 8
  %3895 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3894, 1
  %3896 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3893, ptr %3896, align 8
  %3897 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3895, ptr %3897, align 8
  call void @memrefCopy(i64 4, ptr %3896, ptr %3897)
  call void @llvm.stackrestore.p0(ptr %3891)
  %3898 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 3, i32 1017, i32 117)
  %3899 = call ptr @llvm.stacksave.p0()
  %3900 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %3900, align 8
  %3901 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3900, 1
  %3902 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3823, ptr %3902, align 8
  %3903 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3902, 1
  %3904 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3901, ptr %3904, align 8
  %3905 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3903, ptr %3905, align 8
  call void @memrefCopy(i64 4, ptr %3904, ptr %3905)
  call void @llvm.stackrestore.p0(ptr %3899)
  %3906 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %3907 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %3908 = insertvalue { ptr, ptr, i64 } poison, ptr %3906, 0
  %3909 = insertvalue { ptr, ptr, i64 } %3908, ptr %3907, 1
  %3910 = insertvalue { ptr, ptr, i64 } %3909, i64 0, 2
  %3911 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %3912 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %3913 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %3914 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %3915 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %3916 = extractvalue { ptr, ptr, i64 } %3910, 0
  %3917 = extractvalue { ptr, ptr, i64 } %3910, 1
  %3918 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %3916, 0
  %3919 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3918, ptr %3917, 1
  %3920 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3919, i64 1552, 2
  %3921 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3920, i64 4, 3, 0
  %3922 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3921, i64 64, 4, 0
  %3923 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3922, i64 4, 3, 1
  %3924 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3923, i64 1, 4, 1
  %3925 = call ptr @llvm.stacksave.p0()
  %3926 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3924, ptr %3926, align 8
  %3927 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3926, 1
  %3928 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %3928, align 8
  %3929 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3928, 1
  %3930 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3927, ptr %3930, align 8
  %3931 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3929, ptr %3931, align 8
  call void @memrefCopy(i64 4, ptr %3930, ptr %3931)
  call void @llvm.stackrestore.p0(ptr %3925)
  %3932 = call ptr @llvm.stacksave.p0()
  %3933 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3508, ptr %3933, align 8
  %3934 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3933, 1
  %3935 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %3935, align 8
  %3936 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3935, 1
  %3937 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3934, ptr %3937, align 8
  %3938 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3936, ptr %3938, align 8
  call void @memrefCopy(i64 4, ptr %3937, ptr %3938)
  call void @llvm.stackrestore.p0(ptr %3932)
  %3939 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %3940 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %3941 = insertvalue { ptr, ptr, i64 } poison, ptr %3939, 0
  %3942 = insertvalue { ptr, ptr, i64 } %3941, ptr %3940, 1
  %3943 = insertvalue { ptr, ptr, i64 } %3942, i64 0, 2
  %3944 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %3945 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %3946 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %3947 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %3948 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %3949 = extractvalue { ptr, ptr, i64 } %3943, 0
  %3950 = extractvalue { ptr, ptr, i64 } %3943, 1
  %3951 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %3949, 0
  %3952 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3951, ptr %3950, 1
  %3953 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3952, i64 16, 2
  %3954 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3953, i64 16, 3, 0
  %3955 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3954, i64 64, 4, 0
  %3956 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3955, i64 4, 3, 1
  %3957 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3956, i64 1, 4, 1
  %3958 = call ptr @llvm.stacksave.p0()
  %3959 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3957, ptr %3959, align 8
  %3960 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3959, 1
  %3961 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %3961, align 8
  %3962 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3961, 1
  %3963 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3960, ptr %3963, align 8
  %3964 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3962, ptr %3964, align 8
  call void @memrefCopy(i64 4, ptr %3963, ptr %3964)
  call void @llvm.stackrestore.p0(ptr %3958)
  %3965 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 1, i32 1086, i32 117)
  %3966 = call ptr @llvm.stacksave.p0()
  %3967 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3561, ptr %3967, align 8
  %3968 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3967, 1
  %3969 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %3969, align 8
  %3970 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3969, 1
  %3971 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3968, ptr %3971, align 8
  %3972 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3970, ptr %3972, align 8
  call void @memrefCopy(i64 4, ptr %3971, ptr %3972)
  call void @llvm.stackrestore.p0(ptr %3966)
  %3973 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %3974 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %3975 = insertvalue { ptr, ptr, i64 } poison, ptr %3973, 0
  %3976 = insertvalue { ptr, ptr, i64 } %3975, ptr %3974, 1
  %3977 = insertvalue { ptr, ptr, i64 } %3976, i64 0, 2
  %3978 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %3979 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %3980 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %3981 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %3982 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %3983 = extractvalue { ptr, ptr, i64 } %3977, 0
  %3984 = extractvalue { ptr, ptr, i64 } %3977, 1
  %3985 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %3983, 0
  %3986 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3985, ptr %3984, 1
  %3987 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3986, i64 1040, 2
  %3988 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3987, i64 16, 3, 0
  %3989 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3988, i64 64, 4, 0
  %3990 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3989, i64 4, 3, 1
  %3991 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %3990, i64 1, 4, 1
  %3992 = call ptr @llvm.stacksave.p0()
  %3993 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3991, ptr %3993, align 8
  %3994 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3993, 1
  %3995 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %3995, align 8
  %3996 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %3995, 1
  %3997 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3994, ptr %3997, align 8
  %3998 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %3996, ptr %3998, align 8
  call void @memrefCopy(i64 4, ptr %3997, ptr %3998)
  call void @llvm.stackrestore.p0(ptr %3992)
  %3999 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 1, i32 1155, i32 117)
  %4000 = call ptr @llvm.stacksave.p0()
  %4001 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %4001, align 8
  %4002 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4001, 1
  %4003 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3924, ptr %4003, align 8
  %4004 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4003, 1
  %4005 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4002, ptr %4005, align 8
  %4006 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4004, ptr %4006, align 8
  call void @memrefCopy(i64 4, ptr %4005, ptr %4006)
  call void @llvm.stackrestore.p0(ptr %4000)
  %4007 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %4008 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %4009 = insertvalue { ptr, ptr, i64 } poison, ptr %4007, 0
  %4010 = insertvalue { ptr, ptr, i64 } %4009, ptr %4008, 1
  %4011 = insertvalue { ptr, ptr, i64 } %4010, i64 0, 2
  %4012 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %4013 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %4014 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %4015 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %4016 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %4017 = extractvalue { ptr, ptr, i64 } %4011, 0
  %4018 = extractvalue { ptr, ptr, i64 } %4011, 1
  %4019 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %4017, 0
  %4020 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4019, ptr %4018, 1
  %4021 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4020, i64 1556, 2
  %4022 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4021, i64 4, 3, 0
  %4023 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4022, i64 64, 4, 0
  %4024 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4023, i64 4, 3, 1
  %4025 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4024, i64 1, 4, 1
  %4026 = call ptr @llvm.stacksave.p0()
  %4027 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4025, ptr %4027, align 8
  %4028 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4027, 1
  %4029 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %4029, align 8
  %4030 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4029, 1
  %4031 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4028, ptr %4031, align 8
  %4032 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4030, ptr %4032, align 8
  call void @memrefCopy(i64 4, ptr %4031, ptr %4032)
  call void @llvm.stackrestore.p0(ptr %4026)
  %4033 = call ptr @llvm.stacksave.p0()
  %4034 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3508, ptr %4034, align 8
  %4035 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4034, 1
  %4036 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %4036, align 8
  %4037 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4036, 1
  %4038 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4035, ptr %4038, align 8
  %4039 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4037, ptr %4039, align 8
  call void @memrefCopy(i64 4, ptr %4038, ptr %4039)
  call void @llvm.stackrestore.p0(ptr %4033)
  %4040 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %4041 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %4042 = insertvalue { ptr, ptr, i64 } poison, ptr %4040, 0
  %4043 = insertvalue { ptr, ptr, i64 } %4042, ptr %4041, 1
  %4044 = insertvalue { ptr, ptr, i64 } %4043, i64 0, 2
  %4045 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %4046 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %4047 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %4048 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %4049 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %4050 = extractvalue { ptr, ptr, i64 } %4044, 0
  %4051 = extractvalue { ptr, ptr, i64 } %4044, 1
  %4052 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %4050, 0
  %4053 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4052, ptr %4051, 1
  %4054 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4053, i64 20, 2
  %4055 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4054, i64 16, 3, 0
  %4056 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4055, i64 64, 4, 0
  %4057 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4056, i64 4, 3, 1
  %4058 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4057, i64 1, 4, 1
  %4059 = call ptr @llvm.stacksave.p0()
  %4060 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4058, ptr %4060, align 8
  %4061 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4060, 1
  %4062 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %4062, align 8
  %4063 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4062, 1
  %4064 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4061, ptr %4064, align 8
  %4065 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4063, ptr %4065, align 8
  call void @memrefCopy(i64 4, ptr %4064, ptr %4065)
  call void @llvm.stackrestore.p0(ptr %4059)
  %4066 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 2, i32 1086, i32 117)
  %4067 = call ptr @llvm.stacksave.p0()
  %4068 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3561, ptr %4068, align 8
  %4069 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4068, 1
  %4070 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %4070, align 8
  %4071 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4070, 1
  %4072 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4069, ptr %4072, align 8
  %4073 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4071, ptr %4073, align 8
  call void @memrefCopy(i64 4, ptr %4072, ptr %4073)
  call void @llvm.stackrestore.p0(ptr %4067)
  %4074 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %4075 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %4076 = insertvalue { ptr, ptr, i64 } poison, ptr %4074, 0
  %4077 = insertvalue { ptr, ptr, i64 } %4076, ptr %4075, 1
  %4078 = insertvalue { ptr, ptr, i64 } %4077, i64 0, 2
  %4079 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %4080 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %4081 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %4082 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %4083 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %4084 = extractvalue { ptr, ptr, i64 } %4078, 0
  %4085 = extractvalue { ptr, ptr, i64 } %4078, 1
  %4086 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %4084, 0
  %4087 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4086, ptr %4085, 1
  %4088 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4087, i64 1044, 2
  %4089 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4088, i64 16, 3, 0
  %4090 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4089, i64 64, 4, 0
  %4091 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4090, i64 4, 3, 1
  %4092 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4091, i64 1, 4, 1
  %4093 = call ptr @llvm.stacksave.p0()
  %4094 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4092, ptr %4094, align 8
  %4095 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4094, 1
  %4096 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %4096, align 8
  %4097 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4096, 1
  %4098 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4095, ptr %4098, align 8
  %4099 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4097, ptr %4099, align 8
  call void @memrefCopy(i64 4, ptr %4098, ptr %4099)
  call void @llvm.stackrestore.p0(ptr %4093)
  %4100 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 2, i32 1155, i32 117)
  %4101 = call ptr @llvm.stacksave.p0()
  %4102 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %4102, align 8
  %4103 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4102, 1
  %4104 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4025, ptr %4104, align 8
  %4105 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4104, 1
  %4106 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4103, ptr %4106, align 8
  %4107 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4105, ptr %4107, align 8
  call void @memrefCopy(i64 4, ptr %4106, ptr %4107)
  call void @llvm.stackrestore.p0(ptr %4101)
  %4108 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %4109 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %4110 = insertvalue { ptr, ptr, i64 } poison, ptr %4108, 0
  %4111 = insertvalue { ptr, ptr, i64 } %4110, ptr %4109, 1
  %4112 = insertvalue { ptr, ptr, i64 } %4111, i64 0, 2
  %4113 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %4114 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %4115 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %4116 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %4117 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %4118 = extractvalue { ptr, ptr, i64 } %4112, 0
  %4119 = extractvalue { ptr, ptr, i64 } %4112, 1
  %4120 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %4118, 0
  %4121 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4120, ptr %4119, 1
  %4122 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4121, i64 1560, 2
  %4123 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4122, i64 4, 3, 0
  %4124 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4123, i64 64, 4, 0
  %4125 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4124, i64 4, 3, 1
  %4126 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4125, i64 1, 4, 1
  %4127 = call ptr @llvm.stacksave.p0()
  %4128 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4126, ptr %4128, align 8
  %4129 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4128, 1
  %4130 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %4130, align 8
  %4131 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4130, 1
  %4132 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4129, ptr %4132, align 8
  %4133 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4131, ptr %4133, align 8
  call void @memrefCopy(i64 4, ptr %4132, ptr %4133)
  call void @llvm.stackrestore.p0(ptr %4127)
  %4134 = call ptr @llvm.stacksave.p0()
  %4135 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3508, ptr %4135, align 8
  %4136 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4135, 1
  %4137 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %4137, align 8
  %4138 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4137, 1
  %4139 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4136, ptr %4139, align 8
  %4140 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4138, ptr %4140, align 8
  call void @memrefCopy(i64 4, ptr %4139, ptr %4140)
  call void @llvm.stackrestore.p0(ptr %4134)
  %4141 = call ptr @llvm.stacksave.p0()
  %4142 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1806, ptr %4142, align 8
  %4143 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4142, 1
  %4144 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %4144, align 8
  %4145 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4144, 1
  %4146 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4143, ptr %4146, align 8
  %4147 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4145, ptr %4147, align 8
  call void @memrefCopy(i64 4, ptr %4146, ptr %4147)
  call void @llvm.stackrestore.p0(ptr %4141)
  %4148 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 3, i32 1086, i32 117)
  %4149 = call ptr @llvm.stacksave.p0()
  %4150 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3561, ptr %4150, align 8
  %4151 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4150, 1
  %4152 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %4152, align 8
  %4153 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4152, 1
  %4154 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4151, ptr %4154, align 8
  %4155 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4153, ptr %4155, align 8
  call void @memrefCopy(i64 4, ptr %4154, ptr %4155)
  call void @llvm.stackrestore.p0(ptr %4149)
  %4156 = call ptr @llvm.stacksave.p0()
  %4157 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1871, ptr %4157, align 8
  %4158 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4157, 1
  %4159 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %4159, align 8
  %4160 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4159, 1
  %4161 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4158, ptr %4161, align 8
  %4162 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4160, ptr %4162, align 8
  call void @memrefCopy(i64 4, ptr %4161, ptr %4162)
  call void @llvm.stackrestore.p0(ptr %4156)
  %4163 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 3, i32 1155, i32 117)
  %4164 = call ptr @llvm.stacksave.p0()
  %4165 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %4165, align 8
  %4166 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4165, 1
  %4167 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4126, ptr %4167, align 8
  %4168 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4167, 1
  %4169 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4166, ptr %4169, align 8
  %4170 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4168, ptr %4170, align 8
  call void @memrefCopy(i64 4, ptr %4169, ptr %4170)
  call void @llvm.stackrestore.p0(ptr %4164)
  %4171 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %4172 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %4173 = insertvalue { ptr, ptr, i64 } poison, ptr %4171, 0
  %4174 = insertvalue { ptr, ptr, i64 } %4173, ptr %4172, 1
  %4175 = insertvalue { ptr, ptr, i64 } %4174, i64 0, 2
  %4176 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %4177 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %4178 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %4179 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %4180 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %4181 = extractvalue { ptr, ptr, i64 } %4175, 0
  %4182 = extractvalue { ptr, ptr, i64 } %4175, 1
  %4183 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %4181, 0
  %4184 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4183, ptr %4182, 1
  %4185 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4184, i64 1564, 2
  %4186 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4185, i64 4, 3, 0
  %4187 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4186, i64 64, 4, 0
  %4188 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4187, i64 4, 3, 1
  %4189 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4188, i64 1, 4, 1
  %4190 = call ptr @llvm.stacksave.p0()
  %4191 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4189, ptr %4191, align 8
  %4192 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4191, 1
  %4193 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %4193, align 8
  %4194 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4193, 1
  %4195 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4192, ptr %4195, align 8
  %4196 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4194, ptr %4196, align 8
  call void @memrefCopy(i64 4, ptr %4195, ptr %4196)
  call void @llvm.stackrestore.p0(ptr %4190)
  %4197 = call ptr @llvm.stacksave.p0()
  %4198 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3508, ptr %4198, align 8
  %4199 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4198, 1
  %4200 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %4200, align 8
  %4201 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4200, 1
  %4202 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4199, ptr %4202, align 8
  %4203 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4201, ptr %4203, align 8
  call void @memrefCopy(i64 4, ptr %4202, ptr %4203)
  call void @llvm.stackrestore.p0(ptr %4197)
  %4204 = call ptr @llvm.stacksave.p0()
  %4205 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1938, ptr %4205, align 8
  %4206 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4205, 1
  %4207 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %4207, align 8
  %4208 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4207, 1
  %4209 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4206, ptr %4209, align 8
  %4210 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4208, ptr %4210, align 8
  call void @memrefCopy(i64 4, ptr %4209, ptr %4210)
  call void @llvm.stackrestore.p0(ptr %4204)
  %4211 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 1, i32 1224, i32 117)
  %4212 = call ptr @llvm.stacksave.p0()
  %4213 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3561, ptr %4213, align 8
  %4214 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4213, 1
  %4215 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %4215, align 8
  %4216 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4215, 1
  %4217 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4214, ptr %4217, align 8
  %4218 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4216, ptr %4218, align 8
  call void @memrefCopy(i64 4, ptr %4217, ptr %4218)
  call void @llvm.stackrestore.p0(ptr %4212)
  %4219 = call ptr @llvm.stacksave.p0()
  %4220 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1972, ptr %4220, align 8
  %4221 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4220, 1
  %4222 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %4222, align 8
  %4223 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4222, 1
  %4224 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4221, ptr %4224, align 8
  %4225 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4223, ptr %4225, align 8
  call void @memrefCopy(i64 4, ptr %4224, ptr %4225)
  call void @llvm.stackrestore.p0(ptr %4219)
  %4226 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 1, i32 1293, i32 117)
  %4227 = call ptr @llvm.stacksave.p0()
  %4228 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %4228, align 8
  %4229 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4228, 1
  %4230 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4189, ptr %4230, align 8
  %4231 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4230, 1
  %4232 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4229, ptr %4232, align 8
  %4233 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4231, ptr %4233, align 8
  call void @memrefCopy(i64 4, ptr %4232, ptr %4233)
  call void @llvm.stackrestore.p0(ptr %4227)
  %4234 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %4235 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %4236 = insertvalue { ptr, ptr, i64 } poison, ptr %4234, 0
  %4237 = insertvalue { ptr, ptr, i64 } %4236, ptr %4235, 1
  %4238 = insertvalue { ptr, ptr, i64 } %4237, i64 0, 2
  %4239 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %4240 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %4241 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %4242 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %4243 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %4244 = extractvalue { ptr, ptr, i64 } %4238, 0
  %4245 = extractvalue { ptr, ptr, i64 } %4238, 1
  %4246 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %4244, 0
  %4247 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4246, ptr %4245, 1
  %4248 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4247, i64 1568, 2
  %4249 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4248, i64 4, 3, 0
  %4250 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4249, i64 64, 4, 0
  %4251 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4250, i64 4, 3, 1
  %4252 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4251, i64 1, 4, 1
  %4253 = call ptr @llvm.stacksave.p0()
  %4254 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4252, ptr %4254, align 8
  %4255 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4254, 1
  %4256 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %4256, align 8
  %4257 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4256, 1
  %4258 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4255, ptr %4258, align 8
  %4259 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4257, ptr %4259, align 8
  call void @memrefCopy(i64 4, ptr %4258, ptr %4259)
  call void @llvm.stackrestore.p0(ptr %4253)
  %4260 = call ptr @llvm.stacksave.p0()
  %4261 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3508, ptr %4261, align 8
  %4262 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4261, 1
  %4263 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %4263, align 8
  %4264 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4263, 1
  %4265 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4262, ptr %4265, align 8
  %4266 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4264, ptr %4266, align 8
  call void @memrefCopy(i64 4, ptr %4265, ptr %4266)
  call void @llvm.stackrestore.p0(ptr %4260)
  %4267 = call ptr @llvm.stacksave.p0()
  %4268 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2039, ptr %4268, align 8
  %4269 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4268, 1
  %4270 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %4270, align 8
  %4271 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4270, 1
  %4272 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4269, ptr %4272, align 8
  %4273 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4271, ptr %4273, align 8
  call void @memrefCopy(i64 4, ptr %4272, ptr %4273)
  call void @llvm.stackrestore.p0(ptr %4267)
  %4274 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 2, i32 1224, i32 117)
  %4275 = call ptr @llvm.stacksave.p0()
  %4276 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3561, ptr %4276, align 8
  %4277 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4276, 1
  %4278 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %4278, align 8
  %4279 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4278, 1
  %4280 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4277, ptr %4280, align 8
  %4281 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4279, ptr %4281, align 8
  call void @memrefCopy(i64 4, ptr %4280, ptr %4281)
  call void @llvm.stackrestore.p0(ptr %4275)
  %4282 = call ptr @llvm.stacksave.p0()
  %4283 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2073, ptr %4283, align 8
  %4284 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4283, 1
  %4285 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %4285, align 8
  %4286 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4285, 1
  %4287 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4284, ptr %4287, align 8
  %4288 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4286, ptr %4288, align 8
  call void @memrefCopy(i64 4, ptr %4287, ptr %4288)
  call void @llvm.stackrestore.p0(ptr %4282)
  %4289 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 2, i32 1293, i32 117)
  %4290 = call ptr @llvm.stacksave.p0()
  %4291 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %4291, align 8
  %4292 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4291, 1
  %4293 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4252, ptr %4293, align 8
  %4294 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4293, 1
  %4295 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4292, ptr %4295, align 8
  %4296 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4294, ptr %4296, align 8
  call void @memrefCopy(i64 4, ptr %4295, ptr %4296)
  call void @llvm.stackrestore.p0(ptr %4290)
  %4297 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %4298 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %4299 = insertvalue { ptr, ptr, i64 } poison, ptr %4297, 0
  %4300 = insertvalue { ptr, ptr, i64 } %4299, ptr %4298, 1
  %4301 = insertvalue { ptr, ptr, i64 } %4300, i64 0, 2
  %4302 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %4303 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %4304 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %4305 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %4306 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %4307 = extractvalue { ptr, ptr, i64 } %4301, 0
  %4308 = extractvalue { ptr, ptr, i64 } %4301, 1
  %4309 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %4307, 0
  %4310 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4309, ptr %4308, 1
  %4311 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4310, i64 1572, 2
  %4312 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4311, i64 4, 3, 0
  %4313 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4312, i64 64, 4, 0
  %4314 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4313, i64 4, 3, 1
  %4315 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4314, i64 1, 4, 1
  %4316 = call ptr @llvm.stacksave.p0()
  %4317 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4315, ptr %4317, align 8
  %4318 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4317, 1
  %4319 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %4319, align 8
  %4320 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4319, 1
  %4321 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4318, ptr %4321, align 8
  %4322 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4320, ptr %4322, align 8
  call void @memrefCopy(i64 4, ptr %4321, ptr %4322)
  call void @llvm.stackrestore.p0(ptr %4316)
  %4323 = call ptr @llvm.stacksave.p0()
  %4324 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3508, ptr %4324, align 8
  %4325 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4324, 1
  %4326 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %4326, align 8
  %4327 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4326, 1
  %4328 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4325, ptr %4328, align 8
  %4329 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4327, ptr %4329, align 8
  call void @memrefCopy(i64 4, ptr %4328, ptr %4329)
  call void @llvm.stackrestore.p0(ptr %4323)
  %4330 = call ptr @llvm.stacksave.p0()
  %4331 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2140, ptr %4331, align 8
  %4332 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4331, 1
  %4333 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %4333, align 8
  %4334 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4333, 1
  %4335 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4332, ptr %4335, align 8
  %4336 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4334, ptr %4336, align 8
  call void @memrefCopy(i64 4, ptr %4335, ptr %4336)
  call void @llvm.stackrestore.p0(ptr %4330)
  %4337 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 3, i32 1224, i32 117)
  %4338 = call ptr @llvm.stacksave.p0()
  %4339 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3561, ptr %4339, align 8
  %4340 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4339, 1
  %4341 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %4341, align 8
  %4342 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4341, 1
  %4343 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4340, ptr %4343, align 8
  %4344 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4342, ptr %4344, align 8
  call void @memrefCopy(i64 4, ptr %4343, ptr %4344)
  call void @llvm.stackrestore.p0(ptr %4338)
  %4345 = call ptr @llvm.stacksave.p0()
  %4346 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2174, ptr %4346, align 8
  %4347 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4346, 1
  %4348 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %4348, align 8
  %4349 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4348, 1
  %4350 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4347, ptr %4350, align 8
  %4351 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4349, ptr %4351, align 8
  call void @memrefCopy(i64 4, ptr %4350, ptr %4351)
  call void @llvm.stackrestore.p0(ptr %4345)
  %4352 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 3, i32 1293, i32 117)
  %4353 = call ptr @llvm.stacksave.p0()
  %4354 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %4354, align 8
  %4355 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4354, 1
  %4356 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4315, ptr %4356, align 8
  %4357 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4356, 1
  %4358 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4355, ptr %4358, align 8
  %4359 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4357, ptr %4359, align 8
  call void @memrefCopy(i64 4, ptr %4358, ptr %4359)
  call void @llvm.stackrestore.p0(ptr %4353)
  %4360 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %4361 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %4362 = insertvalue { ptr, ptr, i64 } poison, ptr %4360, 0
  %4363 = insertvalue { ptr, ptr, i64 } %4362, ptr %4361, 1
  %4364 = insertvalue { ptr, ptr, i64 } %4363, i64 0, 2
  %4365 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %4366 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %4367 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %4368 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %4369 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %4370 = extractvalue { ptr, ptr, i64 } %4364, 0
  %4371 = extractvalue { ptr, ptr, i64 } %4364, 1
  %4372 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %4370, 0
  %4373 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4372, ptr %4371, 1
  %4374 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4373, i64 1576, 2
  %4375 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4374, i64 4, 3, 0
  %4376 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4375, i64 64, 4, 0
  %4377 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4376, i64 4, 3, 1
  %4378 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4377, i64 1, 4, 1
  %4379 = call ptr @llvm.stacksave.p0()
  %4380 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4378, ptr %4380, align 8
  %4381 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4380, 1
  %4382 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %4382, align 8
  %4383 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4382, 1
  %4384 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4381, ptr %4384, align 8
  %4385 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4383, ptr %4385, align 8
  call void @memrefCopy(i64 4, ptr %4384, ptr %4385)
  call void @llvm.stackrestore.p0(ptr %4379)
  %4386 = call ptr @llvm.stacksave.p0()
  %4387 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3508, ptr %4387, align 8
  %4388 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4387, 1
  %4389 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %4389, align 8
  %4390 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4389, 1
  %4391 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4388, ptr %4391, align 8
  %4392 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4390, ptr %4392, align 8
  call void @memrefCopy(i64 4, ptr %4391, ptr %4392)
  call void @llvm.stackrestore.p0(ptr %4386)
  %4393 = call ptr @llvm.stacksave.p0()
  %4394 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2241, ptr %4394, align 8
  %4395 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4394, 1
  %4396 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %4396, align 8
  %4397 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4396, 1
  %4398 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4395, ptr %4398, align 8
  %4399 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4397, ptr %4399, align 8
  call void @memrefCopy(i64 4, ptr %4398, ptr %4399)
  call void @llvm.stackrestore.p0(ptr %4393)
  %4400 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 1, i32 1362, i32 117)
  %4401 = call ptr @llvm.stacksave.p0()
  %4402 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3561, ptr %4402, align 8
  %4403 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4402, 1
  %4404 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %4404, align 8
  %4405 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4404, 1
  %4406 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4403, ptr %4406, align 8
  %4407 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4405, ptr %4407, align 8
  call void @memrefCopy(i64 4, ptr %4406, ptr %4407)
  call void @llvm.stackrestore.p0(ptr %4401)
  %4408 = call ptr @llvm.stacksave.p0()
  %4409 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2275, ptr %4409, align 8
  %4410 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4409, 1
  %4411 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %4411, align 8
  %4412 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4411, 1
  %4413 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4410, ptr %4413, align 8
  %4414 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4412, ptr %4414, align 8
  call void @memrefCopy(i64 4, ptr %4413, ptr %4414)
  call void @llvm.stackrestore.p0(ptr %4408)
  %4415 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 1, i32 1431, i32 117)
  %4416 = call ptr @llvm.stacksave.p0()
  %4417 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %4417, align 8
  %4418 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4417, 1
  %4419 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4378, ptr %4419, align 8
  %4420 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4419, 1
  %4421 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4418, ptr %4421, align 8
  %4422 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4420, ptr %4422, align 8
  call void @memrefCopy(i64 4, ptr %4421, ptr %4422)
  call void @llvm.stackrestore.p0(ptr %4416)
  %4423 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %4424 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %4425 = insertvalue { ptr, ptr, i64 } poison, ptr %4423, 0
  %4426 = insertvalue { ptr, ptr, i64 } %4425, ptr %4424, 1
  %4427 = insertvalue { ptr, ptr, i64 } %4426, i64 0, 2
  %4428 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %4429 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %4430 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %4431 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %4432 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %4433 = extractvalue { ptr, ptr, i64 } %4427, 0
  %4434 = extractvalue { ptr, ptr, i64 } %4427, 1
  %4435 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %4433, 0
  %4436 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4435, ptr %4434, 1
  %4437 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4436, i64 1580, 2
  %4438 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4437, i64 4, 3, 0
  %4439 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4438, i64 64, 4, 0
  %4440 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4439, i64 4, 3, 1
  %4441 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4440, i64 1, 4, 1
  %4442 = call ptr @llvm.stacksave.p0()
  %4443 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4441, ptr %4443, align 8
  %4444 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4443, 1
  %4445 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %4445, align 8
  %4446 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4445, 1
  %4447 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4444, ptr %4447, align 8
  %4448 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4446, ptr %4448, align 8
  call void @memrefCopy(i64 4, ptr %4447, ptr %4448)
  call void @llvm.stackrestore.p0(ptr %4442)
  %4449 = call ptr @llvm.stacksave.p0()
  %4450 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3508, ptr %4450, align 8
  %4451 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4450, 1
  %4452 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %4452, align 8
  %4453 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4452, 1
  %4454 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4451, ptr %4454, align 8
  %4455 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4453, ptr %4455, align 8
  call void @memrefCopy(i64 4, ptr %4454, ptr %4455)
  call void @llvm.stackrestore.p0(ptr %4449)
  %4456 = call ptr @llvm.stacksave.p0()
  %4457 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2342, ptr %4457, align 8
  %4458 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4457, 1
  %4459 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %4459, align 8
  %4460 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4459, 1
  %4461 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4458, ptr %4461, align 8
  %4462 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4460, ptr %4462, align 8
  call void @memrefCopy(i64 4, ptr %4461, ptr %4462)
  call void @llvm.stackrestore.p0(ptr %4456)
  %4463 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 2, i32 1362, i32 117)
  %4464 = call ptr @llvm.stacksave.p0()
  %4465 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3561, ptr %4465, align 8
  %4466 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4465, 1
  %4467 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %4467, align 8
  %4468 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4467, 1
  %4469 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4466, ptr %4469, align 8
  %4470 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4468, ptr %4470, align 8
  call void @memrefCopy(i64 4, ptr %4469, ptr %4470)
  call void @llvm.stackrestore.p0(ptr %4464)
  %4471 = call ptr @llvm.stacksave.p0()
  %4472 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2376, ptr %4472, align 8
  %4473 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4472, 1
  %4474 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %4474, align 8
  %4475 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4474, 1
  %4476 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4473, ptr %4476, align 8
  %4477 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4475, ptr %4477, align 8
  call void @memrefCopy(i64 4, ptr %4476, ptr %4477)
  call void @llvm.stackrestore.p0(ptr %4471)
  %4478 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 2, i32 1431, i32 117)
  %4479 = call ptr @llvm.stacksave.p0()
  %4480 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %4480, align 8
  %4481 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4480, 1
  %4482 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4441, ptr %4482, align 8
  %4483 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4482, 1
  %4484 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4481, ptr %4484, align 8
  %4485 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4483, ptr %4485, align 8
  call void @memrefCopy(i64 4, ptr %4484, ptr %4485)
  call void @llvm.stackrestore.p0(ptr %4479)
  %4486 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %4487 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %4488 = insertvalue { ptr, ptr, i64 } poison, ptr %4486, 0
  %4489 = insertvalue { ptr, ptr, i64 } %4488, ptr %4487, 1
  %4490 = insertvalue { ptr, ptr, i64 } %4489, i64 0, 2
  %4491 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %4492 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %4493 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %4494 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %4495 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %4496 = extractvalue { ptr, ptr, i64 } %4490, 0
  %4497 = extractvalue { ptr, ptr, i64 } %4490, 1
  %4498 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %4496, 0
  %4499 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4498, ptr %4497, 1
  %4500 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4499, i64 1584, 2
  %4501 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4500, i64 4, 3, 0
  %4502 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4501, i64 64, 4, 0
  %4503 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4502, i64 4, 3, 1
  %4504 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4503, i64 1, 4, 1
  %4505 = call ptr @llvm.stacksave.p0()
  %4506 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4504, ptr %4506, align 8
  %4507 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4506, 1
  %4508 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %4508, align 8
  %4509 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4508, 1
  %4510 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4507, ptr %4510, align 8
  %4511 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4509, ptr %4511, align 8
  call void @memrefCopy(i64 4, ptr %4510, ptr %4511)
  call void @llvm.stackrestore.p0(ptr %4505)
  %4512 = call ptr @llvm.stacksave.p0()
  %4513 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3508, ptr %4513, align 8
  %4514 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4513, 1
  %4515 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %4515, align 8
  %4516 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4515, 1
  %4517 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4514, ptr %4517, align 8
  %4518 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4516, ptr %4518, align 8
  call void @memrefCopy(i64 4, ptr %4517, ptr %4518)
  call void @llvm.stackrestore.p0(ptr %4512)
  %4519 = call ptr @llvm.stacksave.p0()
  %4520 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2443, ptr %4520, align 8
  %4521 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4520, 1
  %4522 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %4522, align 8
  %4523 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4522, 1
  %4524 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4521, ptr %4524, align 8
  %4525 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4523, ptr %4525, align 8
  call void @memrefCopy(i64 4, ptr %4524, ptr %4525)
  call void @llvm.stackrestore.p0(ptr %4519)
  %4526 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 3, i32 1362, i32 117)
  %4527 = call ptr @llvm.stacksave.p0()
  %4528 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3561, ptr %4528, align 8
  %4529 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4528, 1
  %4530 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %4530, align 8
  %4531 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4530, 1
  %4532 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4529, ptr %4532, align 8
  %4533 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4531, ptr %4533, align 8
  call void @memrefCopy(i64 4, ptr %4532, ptr %4533)
  call void @llvm.stackrestore.p0(ptr %4527)
  %4534 = call ptr @llvm.stacksave.p0()
  %4535 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2477, ptr %4535, align 8
  %4536 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4535, 1
  %4537 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %4537, align 8
  %4538 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4537, 1
  %4539 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4536, ptr %4539, align 8
  %4540 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4538, ptr %4540, align 8
  call void @memrefCopy(i64 4, ptr %4539, ptr %4540)
  call void @llvm.stackrestore.p0(ptr %4534)
  %4541 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 3, i32 1431, i32 117)
  %4542 = call ptr @llvm.stacksave.p0()
  %4543 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %4543, align 8
  %4544 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4543, 1
  %4545 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4504, ptr %4545, align 8
  %4546 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4545, 1
  %4547 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4544, ptr %4547, align 8
  %4548 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4546, ptr %4548, align 8
  call void @memrefCopy(i64 4, ptr %4547, ptr %4548)
  call void @llvm.stackrestore.p0(ptr %4542)
  %4549 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %4550 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %4551 = insertvalue { ptr, ptr, i64 } poison, ptr %4549, 0
  %4552 = insertvalue { ptr, ptr, i64 } %4551, ptr %4550, 1
  %4553 = insertvalue { ptr, ptr, i64 } %4552, i64 0, 2
  %4554 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %4555 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %4556 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %4557 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %4558 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %4559 = extractvalue { ptr, ptr, i64 } %4553, 0
  %4560 = extractvalue { ptr, ptr, i64 } %4553, 1
  %4561 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %4559, 0
  %4562 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4561, ptr %4560, 1
  %4563 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4562, i64 1588, 2
  %4564 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4563, i64 4, 3, 0
  %4565 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4564, i64 64, 4, 0
  %4566 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4565, i64 4, 3, 1
  %4567 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4566, i64 1, 4, 1
  %4568 = call ptr @llvm.stacksave.p0()
  %4569 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4567, ptr %4569, align 8
  %4570 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4569, 1
  %4571 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %4571, align 8
  %4572 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4571, 1
  %4573 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4570, ptr %4573, align 8
  %4574 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4572, ptr %4574, align 8
  call void @memrefCopy(i64 4, ptr %4573, ptr %4574)
  call void @llvm.stackrestore.p0(ptr %4568)
  %4575 = call ptr @llvm.stacksave.p0()
  %4576 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3508, ptr %4576, align 8
  %4577 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4576, 1
  %4578 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %4578, align 8
  %4579 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4578, 1
  %4580 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4577, ptr %4580, align 8
  %4581 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4579, ptr %4581, align 8
  call void @memrefCopy(i64 4, ptr %4580, ptr %4581)
  call void @llvm.stackrestore.p0(ptr %4575)
  %4582 = call ptr @llvm.stacksave.p0()
  %4583 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2544, ptr %4583, align 8
  %4584 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4583, 1
  %4585 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %4585, align 8
  %4586 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4585, 1
  %4587 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4584, ptr %4587, align 8
  %4588 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4586, ptr %4588, align 8
  call void @memrefCopy(i64 4, ptr %4587, ptr %4588)
  call void @llvm.stackrestore.p0(ptr %4582)
  %4589 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 1, i32 1500, i32 117)
  %4590 = call ptr @llvm.stacksave.p0()
  %4591 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3561, ptr %4591, align 8
  %4592 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4591, 1
  %4593 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %4593, align 8
  %4594 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4593, 1
  %4595 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4592, ptr %4595, align 8
  %4596 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4594, ptr %4596, align 8
  call void @memrefCopy(i64 4, ptr %4595, ptr %4596)
  call void @llvm.stackrestore.p0(ptr %4590)
  %4597 = call ptr @llvm.stacksave.p0()
  %4598 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2578, ptr %4598, align 8
  %4599 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4598, 1
  %4600 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %4600, align 8
  %4601 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4600, 1
  %4602 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4599, ptr %4602, align 8
  %4603 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4601, ptr %4603, align 8
  call void @memrefCopy(i64 4, ptr %4602, ptr %4603)
  call void @llvm.stackrestore.p0(ptr %4597)
  %4604 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 1, i32 1569, i32 117)
  %4605 = call ptr @llvm.stacksave.p0()
  %4606 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %4606, align 8
  %4607 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4606, 1
  %4608 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4567, ptr %4608, align 8
  %4609 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4608, 1
  %4610 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4607, ptr %4610, align 8
  %4611 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4609, ptr %4611, align 8
  call void @memrefCopy(i64 4, ptr %4610, ptr %4611)
  call void @llvm.stackrestore.p0(ptr %4605)
  %4612 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %4613 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %4614 = insertvalue { ptr, ptr, i64 } poison, ptr %4612, 0
  %4615 = insertvalue { ptr, ptr, i64 } %4614, ptr %4613, 1
  %4616 = insertvalue { ptr, ptr, i64 } %4615, i64 0, 2
  %4617 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %4618 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %4619 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %4620 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %4621 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %4622 = extractvalue { ptr, ptr, i64 } %4616, 0
  %4623 = extractvalue { ptr, ptr, i64 } %4616, 1
  %4624 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %4622, 0
  %4625 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4624, ptr %4623, 1
  %4626 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4625, i64 1592, 2
  %4627 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4626, i64 4, 3, 0
  %4628 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4627, i64 64, 4, 0
  %4629 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4628, i64 4, 3, 1
  %4630 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4629, i64 1, 4, 1
  %4631 = call ptr @llvm.stacksave.p0()
  %4632 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4630, ptr %4632, align 8
  %4633 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4632, 1
  %4634 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %4634, align 8
  %4635 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4634, 1
  %4636 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4633, ptr %4636, align 8
  %4637 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4635, ptr %4637, align 8
  call void @memrefCopy(i64 4, ptr %4636, ptr %4637)
  call void @llvm.stackrestore.p0(ptr %4631)
  %4638 = call ptr @llvm.stacksave.p0()
  %4639 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3508, ptr %4639, align 8
  %4640 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4639, 1
  %4641 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %4641, align 8
  %4642 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4641, 1
  %4643 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4640, ptr %4643, align 8
  %4644 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4642, ptr %4644, align 8
  call void @memrefCopy(i64 4, ptr %4643, ptr %4644)
  call void @llvm.stackrestore.p0(ptr %4638)
  %4645 = call ptr @llvm.stacksave.p0()
  %4646 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2645, ptr %4646, align 8
  %4647 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4646, 1
  %4648 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %4648, align 8
  %4649 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4648, 1
  %4650 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4647, ptr %4650, align 8
  %4651 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4649, ptr %4651, align 8
  call void @memrefCopy(i64 4, ptr %4650, ptr %4651)
  call void @llvm.stackrestore.p0(ptr %4645)
  %4652 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 2, i32 1500, i32 117)
  %4653 = call ptr @llvm.stacksave.p0()
  %4654 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3561, ptr %4654, align 8
  %4655 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4654, 1
  %4656 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %4656, align 8
  %4657 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4656, 1
  %4658 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4655, ptr %4658, align 8
  %4659 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4657, ptr %4659, align 8
  call void @memrefCopy(i64 4, ptr %4658, ptr %4659)
  call void @llvm.stackrestore.p0(ptr %4653)
  %4660 = call ptr @llvm.stacksave.p0()
  %4661 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2679, ptr %4661, align 8
  %4662 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4661, 1
  %4663 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %4663, align 8
  %4664 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4663, 1
  %4665 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4662, ptr %4665, align 8
  %4666 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4664, ptr %4666, align 8
  call void @memrefCopy(i64 4, ptr %4665, ptr %4666)
  call void @llvm.stackrestore.p0(ptr %4660)
  %4667 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 2, i32 1569, i32 117)
  %4668 = call ptr @llvm.stacksave.p0()
  %4669 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %4669, align 8
  %4670 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4669, 1
  %4671 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4630, ptr %4671, align 8
  %4672 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4671, 1
  %4673 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4670, ptr %4673, align 8
  %4674 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4672, ptr %4674, align 8
  call void @memrefCopy(i64 4, ptr %4673, ptr %4674)
  call void @llvm.stackrestore.p0(ptr %4668)
  %4675 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %4676 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %4677 = insertvalue { ptr, ptr, i64 } poison, ptr %4675, 0
  %4678 = insertvalue { ptr, ptr, i64 } %4677, ptr %4676, 1
  %4679 = insertvalue { ptr, ptr, i64 } %4678, i64 0, 2
  %4680 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %4681 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %4682 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %4683 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %4684 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %4685 = extractvalue { ptr, ptr, i64 } %4679, 0
  %4686 = extractvalue { ptr, ptr, i64 } %4679, 1
  %4687 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %4685, 0
  %4688 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4687, ptr %4686, 1
  %4689 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4688, i64 1596, 2
  %4690 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4689, i64 4, 3, 0
  %4691 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4690, i64 64, 4, 0
  %4692 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4691, i64 4, 3, 1
  %4693 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4692, i64 1, 4, 1
  %4694 = call ptr @llvm.stacksave.p0()
  %4695 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4693, ptr %4695, align 8
  %4696 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4695, 1
  %4697 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %4697, align 8
  %4698 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4697, 1
  %4699 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4696, ptr %4699, align 8
  %4700 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4698, ptr %4700, align 8
  call void @memrefCopy(i64 4, ptr %4699, ptr %4700)
  call void @llvm.stackrestore.p0(ptr %4694)
  %4701 = call ptr @llvm.stacksave.p0()
  %4702 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3508, ptr %4702, align 8
  %4703 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4702, 1
  %4704 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %4704, align 8
  %4705 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4704, 1
  %4706 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4703, ptr %4706, align 8
  %4707 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4705, ptr %4707, align 8
  call void @memrefCopy(i64 4, ptr %4706, ptr %4707)
  call void @llvm.stackrestore.p0(ptr %4701)
  %4708 = call ptr @llvm.stacksave.p0()
  %4709 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2746, ptr %4709, align 8
  %4710 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4709, 1
  %4711 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %4711, align 8
  %4712 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4711, 1
  %4713 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4710, ptr %4713, align 8
  %4714 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4712, ptr %4714, align 8
  call void @memrefCopy(i64 4, ptr %4713, ptr %4714)
  call void @llvm.stackrestore.p0(ptr %4708)
  %4715 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 3, i32 1500, i32 117)
  %4716 = call ptr @llvm.stacksave.p0()
  %4717 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3561, ptr %4717, align 8
  %4718 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4717, 1
  %4719 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %4719, align 8
  %4720 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4719, 1
  %4721 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4718, ptr %4721, align 8
  %4722 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4720, ptr %4722, align 8
  call void @memrefCopy(i64 4, ptr %4721, ptr %4722)
  call void @llvm.stackrestore.p0(ptr %4716)
  %4723 = call ptr @llvm.stacksave.p0()
  %4724 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2780, ptr %4724, align 8
  %4725 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4724, 1
  %4726 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %4726, align 8
  %4727 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4726, 1
  %4728 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4725, ptr %4728, align 8
  %4729 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4727, ptr %4729, align 8
  call void @memrefCopy(i64 4, ptr %4728, ptr %4729)
  call void @llvm.stackrestore.p0(ptr %4723)
  %4730 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 3, i32 1569, i32 117)
  %4731 = call ptr @llvm.stacksave.p0()
  %4732 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %4732, align 8
  %4733 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4732, 1
  %4734 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4693, ptr %4734, align 8
  %4735 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4734, 1
  %4736 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4733, ptr %4736, align 8
  %4737 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4735, ptr %4737, align 8
  call void @memrefCopy(i64 4, ptr %4736, ptr %4737)
  call void @llvm.stackrestore.p0(ptr %4731)
  %4738 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %4739 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %4740 = insertvalue { ptr, ptr, i64 } poison, ptr %4738, 0
  %4741 = insertvalue { ptr, ptr, i64 } %4740, ptr %4739, 1
  %4742 = insertvalue { ptr, ptr, i64 } %4741, i64 0, 2
  %4743 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %4744 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %4745 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %4746 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %4747 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %4748 = extractvalue { ptr, ptr, i64 } %4742, 0
  %4749 = extractvalue { ptr, ptr, i64 } %4742, 1
  %4750 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %4748, 0
  %4751 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4750, ptr %4749, 1
  %4752 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4751, i64 1792, 2
  %4753 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4752, i64 4, 3, 0
  %4754 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4753, i64 64, 4, 0
  %4755 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4754, i64 4, 3, 1
  %4756 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4755, i64 1, 4, 1
  %4757 = call ptr @llvm.stacksave.p0()
  %4758 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4756, ptr %4758, align 8
  %4759 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4758, 1
  %4760 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %4760, align 8
  %4761 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4760, 1
  %4762 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4759, ptr %4762, align 8
  %4763 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4761, ptr %4763, align 8
  call void @memrefCopy(i64 4, ptr %4762, ptr %4763)
  call void @llvm.stackrestore.p0(ptr %4757)
  %4764 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %4765 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %4766 = insertvalue { ptr, ptr, i64 } poison, ptr %4764, 0
  %4767 = insertvalue { ptr, ptr, i64 } %4766, ptr %4765, 1
  %4768 = insertvalue { ptr, ptr, i64 } %4767, i64 0, 2
  %4769 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %4770 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %4771 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %4772 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %4773 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %4774 = extractvalue { ptr, ptr, i64 } %4768, 0
  %4775 = extractvalue { ptr, ptr, i64 } %4768, 1
  %4776 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %4774, 0
  %4777 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4776, ptr %4775, 1
  %4778 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4777, i64 896, 2
  %4779 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4778, i64 4, 3, 0
  %4780 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4779, i64 32, 4, 0
  %4781 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4780, i64 16, 3, 1
  %4782 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4781, i64 1, 4, 1
  %4783 = call ptr @llvm.stacksave.p0()
  %4784 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4782, ptr %4784, align 8
  %4785 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4784, 1
  %4786 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %4786, align 8
  %4787 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4786, 1
  %4788 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4785, ptr %4788, align 8
  %4789 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4787, ptr %4789, align 8
  call void @memrefCopy(i64 4, ptr %4788, ptr %4789)
  call void @llvm.stackrestore.p0(ptr %4783)
  %4790 = call ptr @llvm.stacksave.p0()
  %4791 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3534, ptr %4791, align 8
  %4792 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4791, 1
  %4793 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %4793, align 8
  %4794 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4793, 1
  %4795 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4792, ptr %4795, align 8
  %4796 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4794, ptr %4796, align 8
  call void @memrefCopy(i64 4, ptr %4795, ptr %4796)
  call void @llvm.stackrestore.p0(ptr %4790)
  %4797 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 1, i32 1638, i32 117)
  %4798 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %4799 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %4800 = insertvalue { ptr, ptr, i64 } poison, ptr %4798, 0
  %4801 = insertvalue { ptr, ptr, i64 } %4800, ptr %4799, 1
  %4802 = insertvalue { ptr, ptr, i64 } %4801, i64 0, 2
  %4803 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %4804 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %4805 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %4806 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %4807 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %4808 = extractvalue { ptr, ptr, i64 } %4802, 0
  %4809 = extractvalue { ptr, ptr, i64 } %4802, 1
  %4810 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %4808, 0
  %4811 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4810, ptr %4809, 1
  %4812 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4811, i64 912, 2
  %4813 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4812, i64 4, 3, 0
  %4814 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4813, i64 32, 4, 0
  %4815 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4814, i64 16, 3, 1
  %4816 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4815, i64 1, 4, 1
  %4817 = call ptr @llvm.stacksave.p0()
  %4818 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4816, ptr %4818, align 8
  %4819 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4818, 1
  %4820 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %4820, align 8
  %4821 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4820, 1
  %4822 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4819, ptr %4822, align 8
  %4823 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4821, ptr %4823, align 8
  call void @memrefCopy(i64 4, ptr %4822, ptr %4823)
  call void @llvm.stackrestore.p0(ptr %4817)
  %4824 = call ptr @llvm.stacksave.p0()
  %4825 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3587, ptr %4825, align 8
  %4826 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4825, 1
  %4827 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %4827, align 8
  %4828 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4827, 1
  %4829 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4826, ptr %4829, align 8
  %4830 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4828, ptr %4830, align 8
  call void @memrefCopy(i64 4, ptr %4829, ptr %4830)
  call void @llvm.stackrestore.p0(ptr %4824)
  %4831 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 1, i32 1707, i32 117)
  %4832 = call ptr @llvm.stacksave.p0()
  %4833 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %4833, align 8
  %4834 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4833, 1
  %4835 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4756, ptr %4835, align 8
  %4836 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4835, 1
  %4837 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4834, ptr %4837, align 8
  %4838 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4836, ptr %4838, align 8
  call void @memrefCopy(i64 4, ptr %4837, ptr %4838)
  call void @llvm.stackrestore.p0(ptr %4832)
  %4839 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %4840 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %4841 = insertvalue { ptr, ptr, i64 } poison, ptr %4839, 0
  %4842 = insertvalue { ptr, ptr, i64 } %4841, ptr %4840, 1
  %4843 = insertvalue { ptr, ptr, i64 } %4842, i64 0, 2
  %4844 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %4845 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %4846 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %4847 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %4848 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %4849 = extractvalue { ptr, ptr, i64 } %4843, 0
  %4850 = extractvalue { ptr, ptr, i64 } %4843, 1
  %4851 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %4849, 0
  %4852 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4851, ptr %4850, 1
  %4853 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4852, i64 1796, 2
  %4854 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4853, i64 4, 3, 0
  %4855 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4854, i64 64, 4, 0
  %4856 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4855, i64 4, 3, 1
  %4857 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4856, i64 1, 4, 1
  %4858 = call ptr @llvm.stacksave.p0()
  %4859 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4857, ptr %4859, align 8
  %4860 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4859, 1
  %4861 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %4861, align 8
  %4862 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4861, 1
  %4863 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4860, ptr %4863, align 8
  %4864 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4862, ptr %4864, align 8
  call void @memrefCopy(i64 4, ptr %4863, ptr %4864)
  call void @llvm.stackrestore.p0(ptr %4858)
  %4865 = call ptr @llvm.stacksave.p0()
  %4866 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4782, ptr %4866, align 8
  %4867 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4866, 1
  %4868 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %4868, align 8
  %4869 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4868, 1
  %4870 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4867, ptr %4870, align 8
  %4871 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4869, ptr %4871, align 8
  call void @memrefCopy(i64 4, ptr %4870, ptr %4871)
  call void @llvm.stackrestore.p0(ptr %4865)
  %4872 = call ptr @llvm.stacksave.p0()
  %4873 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3654, ptr %4873, align 8
  %4874 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4873, 1
  %4875 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %4875, align 8
  %4876 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4875, 1
  %4877 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4874, ptr %4877, align 8
  %4878 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4876, ptr %4878, align 8
  call void @memrefCopy(i64 4, ptr %4877, ptr %4878)
  call void @llvm.stackrestore.p0(ptr %4872)
  %4879 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 2, i32 1638, i32 117)
  %4880 = call ptr @llvm.stacksave.p0()
  %4881 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4816, ptr %4881, align 8
  %4882 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4881, 1
  %4883 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %4883, align 8
  %4884 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4883, 1
  %4885 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4882, ptr %4885, align 8
  %4886 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4884, ptr %4886, align 8
  call void @memrefCopy(i64 4, ptr %4885, ptr %4886)
  call void @llvm.stackrestore.p0(ptr %4880)
  %4887 = call ptr @llvm.stacksave.p0()
  %4888 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3688, ptr %4888, align 8
  %4889 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4888, 1
  %4890 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %4890, align 8
  %4891 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4890, 1
  %4892 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4889, ptr %4892, align 8
  %4893 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4891, ptr %4893, align 8
  call void @memrefCopy(i64 4, ptr %4892, ptr %4893)
  call void @llvm.stackrestore.p0(ptr %4887)
  %4894 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 2, i32 1707, i32 117)
  %4895 = call ptr @llvm.stacksave.p0()
  %4896 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %4896, align 8
  %4897 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4896, 1
  %4898 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4857, ptr %4898, align 8
  %4899 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4898, 1
  %4900 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4897, ptr %4900, align 8
  %4901 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4899, ptr %4901, align 8
  call void @memrefCopy(i64 4, ptr %4900, ptr %4901)
  call void @llvm.stackrestore.p0(ptr %4895)
  %4902 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %4903 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %4904 = insertvalue { ptr, ptr, i64 } poison, ptr %4902, 0
  %4905 = insertvalue { ptr, ptr, i64 } %4904, ptr %4903, 1
  %4906 = insertvalue { ptr, ptr, i64 } %4905, i64 0, 2
  %4907 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %4908 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %4909 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %4910 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %4911 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %4912 = extractvalue { ptr, ptr, i64 } %4906, 0
  %4913 = extractvalue { ptr, ptr, i64 } %4906, 1
  %4914 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %4912, 0
  %4915 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4914, ptr %4913, 1
  %4916 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4915, i64 1800, 2
  %4917 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4916, i64 4, 3, 0
  %4918 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4917, i64 64, 4, 0
  %4919 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4918, i64 4, 3, 1
  %4920 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4919, i64 1, 4, 1
  %4921 = call ptr @llvm.stacksave.p0()
  %4922 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4920, ptr %4922, align 8
  %4923 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4922, 1
  %4924 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %4924, align 8
  %4925 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4924, 1
  %4926 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4923, ptr %4926, align 8
  %4927 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4925, ptr %4927, align 8
  call void @memrefCopy(i64 4, ptr %4926, ptr %4927)
  call void @llvm.stackrestore.p0(ptr %4921)
  %4928 = call ptr @llvm.stacksave.p0()
  %4929 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4782, ptr %4929, align 8
  %4930 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4929, 1
  %4931 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %4931, align 8
  %4932 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4931, 1
  %4933 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4930, ptr %4933, align 8
  %4934 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4932, ptr %4934, align 8
  call void @memrefCopy(i64 4, ptr %4933, ptr %4934)
  call void @llvm.stackrestore.p0(ptr %4928)
  %4935 = call ptr @llvm.stacksave.p0()
  %4936 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3755, ptr %4936, align 8
  %4937 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4936, 1
  %4938 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %4938, align 8
  %4939 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4938, 1
  %4940 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4937, ptr %4940, align 8
  %4941 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4939, ptr %4941, align 8
  call void @memrefCopy(i64 4, ptr %4940, ptr %4941)
  call void @llvm.stackrestore.p0(ptr %4935)
  %4942 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 3, i32 1638, i32 117)
  %4943 = call ptr @llvm.stacksave.p0()
  %4944 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4816, ptr %4944, align 8
  %4945 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4944, 1
  %4946 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %4946, align 8
  %4947 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4946, 1
  %4948 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4945, ptr %4948, align 8
  %4949 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4947, ptr %4949, align 8
  call void @memrefCopy(i64 4, ptr %4948, ptr %4949)
  call void @llvm.stackrestore.p0(ptr %4943)
  %4950 = call ptr @llvm.stacksave.p0()
  %4951 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3789, ptr %4951, align 8
  %4952 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4951, 1
  %4953 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %4953, align 8
  %4954 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4953, 1
  %4955 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4952, ptr %4955, align 8
  %4956 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4954, ptr %4956, align 8
  call void @memrefCopy(i64 4, ptr %4955, ptr %4956)
  call void @llvm.stackrestore.p0(ptr %4950)
  %4957 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 3, i32 1707, i32 117)
  %4958 = call ptr @llvm.stacksave.p0()
  %4959 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %4959, align 8
  %4960 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4959, 1
  %4961 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4920, ptr %4961, align 8
  %4962 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4961, 1
  %4963 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4960, ptr %4963, align 8
  %4964 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4962, ptr %4964, align 8
  call void @memrefCopy(i64 4, ptr %4963, ptr %4964)
  call void @llvm.stackrestore.p0(ptr %4958)
  %4965 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %4966 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %4967 = insertvalue { ptr, ptr, i64 } poison, ptr %4965, 0
  %4968 = insertvalue { ptr, ptr, i64 } %4967, ptr %4966, 1
  %4969 = insertvalue { ptr, ptr, i64 } %4968, i64 0, 2
  %4970 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %4971 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %4972 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %4973 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %4974 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %4975 = extractvalue { ptr, ptr, i64 } %4969, 0
  %4976 = extractvalue { ptr, ptr, i64 } %4969, 1
  %4977 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %4975, 0
  %4978 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4977, ptr %4976, 1
  %4979 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4978, i64 1804, 2
  %4980 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4979, i64 4, 3, 0
  %4981 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4980, i64 64, 4, 0
  %4982 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4981, i64 4, 3, 1
  %4983 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %4982, i64 1, 4, 1
  %4984 = call ptr @llvm.stacksave.p0()
  %4985 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4983, ptr %4985, align 8
  %4986 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4985, 1
  %4987 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %4987, align 8
  %4988 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4987, 1
  %4989 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4986, ptr %4989, align 8
  %4990 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4988, ptr %4990, align 8
  call void @memrefCopy(i64 4, ptr %4989, ptr %4990)
  call void @llvm.stackrestore.p0(ptr %4984)
  %4991 = call ptr @llvm.stacksave.p0()
  %4992 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4782, ptr %4992, align 8
  %4993 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4992, 1
  %4994 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %4994, align 8
  %4995 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4994, 1
  %4996 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4993, ptr %4996, align 8
  %4997 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %4995, ptr %4997, align 8
  call void @memrefCopy(i64 4, ptr %4996, ptr %4997)
  call void @llvm.stackrestore.p0(ptr %4991)
  %4998 = call ptr @llvm.stacksave.p0()
  %4999 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3856, ptr %4999, align 8
  %5000 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %4999, 1
  %5001 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %5001, align 8
  %5002 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5001, 1
  %5003 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5000, ptr %5003, align 8
  %5004 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5002, ptr %5004, align 8
  call void @memrefCopy(i64 4, ptr %5003, ptr %5004)
  call void @llvm.stackrestore.p0(ptr %4998)
  %5005 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 1, i32 1776, i32 117)
  %5006 = call ptr @llvm.stacksave.p0()
  %5007 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4816, ptr %5007, align 8
  %5008 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5007, 1
  %5009 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %5009, align 8
  %5010 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5009, 1
  %5011 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5008, ptr %5011, align 8
  %5012 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5010, ptr %5012, align 8
  call void @memrefCopy(i64 4, ptr %5011, ptr %5012)
  call void @llvm.stackrestore.p0(ptr %5006)
  %5013 = call ptr @llvm.stacksave.p0()
  %5014 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3890, ptr %5014, align 8
  %5015 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5014, 1
  %5016 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %5016, align 8
  %5017 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5016, 1
  %5018 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5015, ptr %5018, align 8
  %5019 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5017, ptr %5019, align 8
  call void @memrefCopy(i64 4, ptr %5018, ptr %5019)
  call void @llvm.stackrestore.p0(ptr %5013)
  %5020 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 1, i32 1845, i32 117)
  %5021 = call ptr @llvm.stacksave.p0()
  %5022 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %5022, align 8
  %5023 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5022, 1
  %5024 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4983, ptr %5024, align 8
  %5025 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5024, 1
  %5026 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5023, ptr %5026, align 8
  %5027 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5025, ptr %5027, align 8
  call void @memrefCopy(i64 4, ptr %5026, ptr %5027)
  call void @llvm.stackrestore.p0(ptr %5021)
  %5028 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %5029 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %5030 = insertvalue { ptr, ptr, i64 } poison, ptr %5028, 0
  %5031 = insertvalue { ptr, ptr, i64 } %5030, ptr %5029, 1
  %5032 = insertvalue { ptr, ptr, i64 } %5031, i64 0, 2
  %5033 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %5034 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %5035 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %5036 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %5037 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %5038 = extractvalue { ptr, ptr, i64 } %5032, 0
  %5039 = extractvalue { ptr, ptr, i64 } %5032, 1
  %5040 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %5038, 0
  %5041 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5040, ptr %5039, 1
  %5042 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5041, i64 1808, 2
  %5043 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5042, i64 4, 3, 0
  %5044 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5043, i64 64, 4, 0
  %5045 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5044, i64 4, 3, 1
  %5046 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5045, i64 1, 4, 1
  %5047 = call ptr @llvm.stacksave.p0()
  %5048 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %5046, ptr %5048, align 8
  %5049 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5048, 1
  %5050 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %5050, align 8
  %5051 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5050, 1
  %5052 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5049, ptr %5052, align 8
  %5053 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5051, ptr %5053, align 8
  call void @memrefCopy(i64 4, ptr %5052, ptr %5053)
  call void @llvm.stackrestore.p0(ptr %5047)
  %5054 = call ptr @llvm.stacksave.p0()
  %5055 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4782, ptr %5055, align 8
  %5056 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5055, 1
  %5057 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %5057, align 8
  %5058 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5057, 1
  %5059 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5056, ptr %5059, align 8
  %5060 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5058, ptr %5060, align 8
  call void @memrefCopy(i64 4, ptr %5059, ptr %5060)
  call void @llvm.stackrestore.p0(ptr %5054)
  %5061 = call ptr @llvm.stacksave.p0()
  %5062 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3957, ptr %5062, align 8
  %5063 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5062, 1
  %5064 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %5064, align 8
  %5065 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5064, 1
  %5066 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5063, ptr %5066, align 8
  %5067 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5065, ptr %5067, align 8
  call void @memrefCopy(i64 4, ptr %5066, ptr %5067)
  call void @llvm.stackrestore.p0(ptr %5061)
  %5068 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 2, i32 1776, i32 117)
  %5069 = call ptr @llvm.stacksave.p0()
  %5070 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4816, ptr %5070, align 8
  %5071 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5070, 1
  %5072 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %5072, align 8
  %5073 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5072, 1
  %5074 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5071, ptr %5074, align 8
  %5075 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5073, ptr %5075, align 8
  call void @memrefCopy(i64 4, ptr %5074, ptr %5075)
  call void @llvm.stackrestore.p0(ptr %5069)
  %5076 = call ptr @llvm.stacksave.p0()
  %5077 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %3991, ptr %5077, align 8
  %5078 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5077, 1
  %5079 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %5079, align 8
  %5080 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5079, 1
  %5081 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5078, ptr %5081, align 8
  %5082 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5080, ptr %5082, align 8
  call void @memrefCopy(i64 4, ptr %5081, ptr %5082)
  call void @llvm.stackrestore.p0(ptr %5076)
  %5083 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 2, i32 1845, i32 117)
  %5084 = call ptr @llvm.stacksave.p0()
  %5085 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %5085, align 8
  %5086 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5085, 1
  %5087 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %5046, ptr %5087, align 8
  %5088 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5087, 1
  %5089 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5086, ptr %5089, align 8
  %5090 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5088, ptr %5090, align 8
  call void @memrefCopy(i64 4, ptr %5089, ptr %5090)
  call void @llvm.stackrestore.p0(ptr %5084)
  %5091 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %5092 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %5093 = insertvalue { ptr, ptr, i64 } poison, ptr %5091, 0
  %5094 = insertvalue { ptr, ptr, i64 } %5093, ptr %5092, 1
  %5095 = insertvalue { ptr, ptr, i64 } %5094, i64 0, 2
  %5096 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %5097 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %5098 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %5099 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %5100 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %5101 = extractvalue { ptr, ptr, i64 } %5095, 0
  %5102 = extractvalue { ptr, ptr, i64 } %5095, 1
  %5103 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %5101, 0
  %5104 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5103, ptr %5102, 1
  %5105 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5104, i64 1812, 2
  %5106 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5105, i64 4, 3, 0
  %5107 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5106, i64 64, 4, 0
  %5108 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5107, i64 4, 3, 1
  %5109 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5108, i64 1, 4, 1
  %5110 = call ptr @llvm.stacksave.p0()
  %5111 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %5109, ptr %5111, align 8
  %5112 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5111, 1
  %5113 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %5113, align 8
  %5114 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5113, 1
  %5115 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5112, ptr %5115, align 8
  %5116 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5114, ptr %5116, align 8
  call void @memrefCopy(i64 4, ptr %5115, ptr %5116)
  call void @llvm.stackrestore.p0(ptr %5110)
  %5117 = call ptr @llvm.stacksave.p0()
  %5118 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4782, ptr %5118, align 8
  %5119 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5118, 1
  %5120 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %5120, align 8
  %5121 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5120, 1
  %5122 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5119, ptr %5122, align 8
  %5123 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5121, ptr %5123, align 8
  call void @memrefCopy(i64 4, ptr %5122, ptr %5123)
  call void @llvm.stackrestore.p0(ptr %5117)
  %5124 = call ptr @llvm.stacksave.p0()
  %5125 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4058, ptr %5125, align 8
  %5126 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5125, 1
  %5127 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %5127, align 8
  %5128 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5127, 1
  %5129 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5126, ptr %5129, align 8
  %5130 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5128, ptr %5130, align 8
  call void @memrefCopy(i64 4, ptr %5129, ptr %5130)
  call void @llvm.stackrestore.p0(ptr %5124)
  %5131 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 3, i32 1776, i32 117)
  %5132 = call ptr @llvm.stacksave.p0()
  %5133 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4816, ptr %5133, align 8
  %5134 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5133, 1
  %5135 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %5135, align 8
  %5136 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5135, 1
  %5137 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5134, ptr %5137, align 8
  %5138 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5136, ptr %5138, align 8
  call void @memrefCopy(i64 4, ptr %5137, ptr %5138)
  call void @llvm.stackrestore.p0(ptr %5132)
  %5139 = call ptr @llvm.stacksave.p0()
  %5140 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4092, ptr %5140, align 8
  %5141 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5140, 1
  %5142 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %5142, align 8
  %5143 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5142, 1
  %5144 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5141, ptr %5144, align 8
  %5145 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5143, ptr %5145, align 8
  call void @memrefCopy(i64 4, ptr %5144, ptr %5145)
  call void @llvm.stackrestore.p0(ptr %5139)
  %5146 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 3, i32 1845, i32 117)
  %5147 = call ptr @llvm.stacksave.p0()
  %5148 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %5148, align 8
  %5149 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5148, 1
  %5150 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %5109, ptr %5150, align 8
  %5151 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5150, 1
  %5152 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5149, ptr %5152, align 8
  %5153 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5151, ptr %5153, align 8
  call void @memrefCopy(i64 4, ptr %5152, ptr %5153)
  call void @llvm.stackrestore.p0(ptr %5147)
  %5154 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %5155 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %5156 = insertvalue { ptr, ptr, i64 } poison, ptr %5154, 0
  %5157 = insertvalue { ptr, ptr, i64 } %5156, ptr %5155, 1
  %5158 = insertvalue { ptr, ptr, i64 } %5157, i64 0, 2
  %5159 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %5160 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %5161 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %5162 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %5163 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %5164 = extractvalue { ptr, ptr, i64 } %5158, 0
  %5165 = extractvalue { ptr, ptr, i64 } %5158, 1
  %5166 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %5164, 0
  %5167 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5166, ptr %5165, 1
  %5168 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5167, i64 1816, 2
  %5169 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5168, i64 4, 3, 0
  %5170 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5169, i64 64, 4, 0
  %5171 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5170, i64 4, 3, 1
  %5172 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5171, i64 1, 4, 1
  %5173 = call ptr @llvm.stacksave.p0()
  %5174 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %5172, ptr %5174, align 8
  %5175 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5174, 1
  %5176 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %5176, align 8
  %5177 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5176, 1
  %5178 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5175, ptr %5178, align 8
  %5179 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5177, ptr %5179, align 8
  call void @memrefCopy(i64 4, ptr %5178, ptr %5179)
  call void @llvm.stackrestore.p0(ptr %5173)
  %5180 = call ptr @llvm.stacksave.p0()
  %5181 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4782, ptr %5181, align 8
  %5182 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5181, 1
  %5183 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %5183, align 8
  %5184 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5183, 1
  %5185 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5182, ptr %5185, align 8
  %5186 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5184, ptr %5186, align 8
  call void @memrefCopy(i64 4, ptr %5185, ptr %5186)
  call void @llvm.stackrestore.p0(ptr %5180)
  %5187 = call ptr @llvm.stacksave.p0()
  %5188 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1806, ptr %5188, align 8
  %5189 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5188, 1
  %5190 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %5190, align 8
  %5191 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5190, 1
  %5192 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5189, ptr %5192, align 8
  %5193 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5191, ptr %5193, align 8
  call void @memrefCopy(i64 4, ptr %5192, ptr %5193)
  call void @llvm.stackrestore.p0(ptr %5187)
  %5194 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 1, i32 1914, i32 117)
  %5195 = call ptr @llvm.stacksave.p0()
  %5196 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4816, ptr %5196, align 8
  %5197 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5196, 1
  %5198 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %5198, align 8
  %5199 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5198, 1
  %5200 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5197, ptr %5200, align 8
  %5201 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5199, ptr %5201, align 8
  call void @memrefCopy(i64 4, ptr %5200, ptr %5201)
  call void @llvm.stackrestore.p0(ptr %5195)
  %5202 = call ptr @llvm.stacksave.p0()
  %5203 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1871, ptr %5203, align 8
  %5204 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5203, 1
  %5205 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %5205, align 8
  %5206 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5205, 1
  %5207 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5204, ptr %5207, align 8
  %5208 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5206, ptr %5208, align 8
  call void @memrefCopy(i64 4, ptr %5207, ptr %5208)
  call void @llvm.stackrestore.p0(ptr %5202)
  %5209 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 1, i32 1983, i32 117)
  %5210 = call ptr @llvm.stacksave.p0()
  %5211 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %5211, align 8
  %5212 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5211, 1
  %5213 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %5172, ptr %5213, align 8
  %5214 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5213, 1
  %5215 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5212, ptr %5215, align 8
  %5216 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5214, ptr %5216, align 8
  call void @memrefCopy(i64 4, ptr %5215, ptr %5216)
  call void @llvm.stackrestore.p0(ptr %5210)
  %5217 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %5218 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %5219 = insertvalue { ptr, ptr, i64 } poison, ptr %5217, 0
  %5220 = insertvalue { ptr, ptr, i64 } %5219, ptr %5218, 1
  %5221 = insertvalue { ptr, ptr, i64 } %5220, i64 0, 2
  %5222 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %5223 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %5224 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %5225 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %5226 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %5227 = extractvalue { ptr, ptr, i64 } %5221, 0
  %5228 = extractvalue { ptr, ptr, i64 } %5221, 1
  %5229 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %5227, 0
  %5230 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5229, ptr %5228, 1
  %5231 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5230, i64 1820, 2
  %5232 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5231, i64 4, 3, 0
  %5233 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5232, i64 64, 4, 0
  %5234 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5233, i64 4, 3, 1
  %5235 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5234, i64 1, 4, 1
  %5236 = call ptr @llvm.stacksave.p0()
  %5237 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %5235, ptr %5237, align 8
  %5238 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5237, 1
  %5239 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %5239, align 8
  %5240 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5239, 1
  %5241 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5238, ptr %5241, align 8
  %5242 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5240, ptr %5242, align 8
  call void @memrefCopy(i64 4, ptr %5241, ptr %5242)
  call void @llvm.stackrestore.p0(ptr %5236)
  %5243 = call ptr @llvm.stacksave.p0()
  %5244 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4782, ptr %5244, align 8
  %5245 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5244, 1
  %5246 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %5246, align 8
  %5247 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5246, 1
  %5248 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5245, ptr %5248, align 8
  %5249 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5247, ptr %5249, align 8
  call void @memrefCopy(i64 4, ptr %5248, ptr %5249)
  call void @llvm.stackrestore.p0(ptr %5243)
  %5250 = call ptr @llvm.stacksave.p0()
  %5251 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1938, ptr %5251, align 8
  %5252 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5251, 1
  %5253 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %5253, align 8
  %5254 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5253, 1
  %5255 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5252, ptr %5255, align 8
  %5256 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5254, ptr %5256, align 8
  call void @memrefCopy(i64 4, ptr %5255, ptr %5256)
  call void @llvm.stackrestore.p0(ptr %5250)
  %5257 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 2, i32 1914, i32 117)
  %5258 = call ptr @llvm.stacksave.p0()
  %5259 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4816, ptr %5259, align 8
  %5260 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5259, 1
  %5261 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %5261, align 8
  %5262 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5261, 1
  %5263 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5260, ptr %5263, align 8
  %5264 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5262, ptr %5264, align 8
  call void @memrefCopy(i64 4, ptr %5263, ptr %5264)
  call void @llvm.stackrestore.p0(ptr %5258)
  %5265 = call ptr @llvm.stacksave.p0()
  %5266 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1972, ptr %5266, align 8
  %5267 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5266, 1
  %5268 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %5268, align 8
  %5269 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5268, 1
  %5270 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5267, ptr %5270, align 8
  %5271 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5269, ptr %5271, align 8
  call void @memrefCopy(i64 4, ptr %5270, ptr %5271)
  call void @llvm.stackrestore.p0(ptr %5265)
  %5272 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 2, i32 1983, i32 117)
  %5273 = call ptr @llvm.stacksave.p0()
  %5274 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %5274, align 8
  %5275 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5274, 1
  %5276 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %5235, ptr %5276, align 8
  %5277 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5276, 1
  %5278 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5275, ptr %5278, align 8
  %5279 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5277, ptr %5279, align 8
  call void @memrefCopy(i64 4, ptr %5278, ptr %5279)
  call void @llvm.stackrestore.p0(ptr %5273)
  %5280 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %5281 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %5282 = insertvalue { ptr, ptr, i64 } poison, ptr %5280, 0
  %5283 = insertvalue { ptr, ptr, i64 } %5282, ptr %5281, 1
  %5284 = insertvalue { ptr, ptr, i64 } %5283, i64 0, 2
  %5285 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %5286 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %5287 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %5288 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %5289 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %5290 = extractvalue { ptr, ptr, i64 } %5284, 0
  %5291 = extractvalue { ptr, ptr, i64 } %5284, 1
  %5292 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %5290, 0
  %5293 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5292, ptr %5291, 1
  %5294 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5293, i64 1824, 2
  %5295 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5294, i64 4, 3, 0
  %5296 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5295, i64 64, 4, 0
  %5297 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5296, i64 4, 3, 1
  %5298 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5297, i64 1, 4, 1
  %5299 = call ptr @llvm.stacksave.p0()
  %5300 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %5298, ptr %5300, align 8
  %5301 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5300, 1
  %5302 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %5302, align 8
  %5303 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5302, 1
  %5304 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5301, ptr %5304, align 8
  %5305 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5303, ptr %5305, align 8
  call void @memrefCopy(i64 4, ptr %5304, ptr %5305)
  call void @llvm.stackrestore.p0(ptr %5299)
  %5306 = call ptr @llvm.stacksave.p0()
  %5307 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4782, ptr %5307, align 8
  %5308 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5307, 1
  %5309 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %5309, align 8
  %5310 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5309, 1
  %5311 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5308, ptr %5311, align 8
  %5312 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5310, ptr %5312, align 8
  call void @memrefCopy(i64 4, ptr %5311, ptr %5312)
  call void @llvm.stackrestore.p0(ptr %5306)
  %5313 = call ptr @llvm.stacksave.p0()
  %5314 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2039, ptr %5314, align 8
  %5315 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5314, 1
  %5316 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %5316, align 8
  %5317 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5316, 1
  %5318 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5315, ptr %5318, align 8
  %5319 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5317, ptr %5319, align 8
  call void @memrefCopy(i64 4, ptr %5318, ptr %5319)
  call void @llvm.stackrestore.p0(ptr %5313)
  %5320 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 3, i32 1914, i32 117)
  %5321 = call ptr @llvm.stacksave.p0()
  %5322 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4816, ptr %5322, align 8
  %5323 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5322, 1
  %5324 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %5324, align 8
  %5325 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5324, 1
  %5326 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5323, ptr %5326, align 8
  %5327 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5325, ptr %5327, align 8
  call void @memrefCopy(i64 4, ptr %5326, ptr %5327)
  call void @llvm.stackrestore.p0(ptr %5321)
  %5328 = call ptr @llvm.stacksave.p0()
  %5329 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2073, ptr %5329, align 8
  %5330 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5329, 1
  %5331 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %5331, align 8
  %5332 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5331, 1
  %5333 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5330, ptr %5333, align 8
  %5334 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5332, ptr %5334, align 8
  call void @memrefCopy(i64 4, ptr %5333, ptr %5334)
  call void @llvm.stackrestore.p0(ptr %5328)
  %5335 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 3, i32 1983, i32 117)
  %5336 = call ptr @llvm.stacksave.p0()
  %5337 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %5337, align 8
  %5338 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5337, 1
  %5339 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %5298, ptr %5339, align 8
  %5340 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5339, 1
  %5341 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5338, ptr %5341, align 8
  %5342 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5340, ptr %5342, align 8
  call void @memrefCopy(i64 4, ptr %5341, ptr %5342)
  call void @llvm.stackrestore.p0(ptr %5336)
  %5343 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %5344 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %5345 = insertvalue { ptr, ptr, i64 } poison, ptr %5343, 0
  %5346 = insertvalue { ptr, ptr, i64 } %5345, ptr %5344, 1
  %5347 = insertvalue { ptr, ptr, i64 } %5346, i64 0, 2
  %5348 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %5349 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %5350 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %5351 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %5352 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %5353 = extractvalue { ptr, ptr, i64 } %5347, 0
  %5354 = extractvalue { ptr, ptr, i64 } %5347, 1
  %5355 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %5353, 0
  %5356 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5355, ptr %5354, 1
  %5357 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5356, i64 1828, 2
  %5358 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5357, i64 4, 3, 0
  %5359 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5358, i64 64, 4, 0
  %5360 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5359, i64 4, 3, 1
  %5361 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5360, i64 1, 4, 1
  %5362 = call ptr @llvm.stacksave.p0()
  %5363 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %5361, ptr %5363, align 8
  %5364 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5363, 1
  %5365 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %5365, align 8
  %5366 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5365, 1
  %5367 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5364, ptr %5367, align 8
  %5368 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5366, ptr %5368, align 8
  call void @memrefCopy(i64 4, ptr %5367, ptr %5368)
  call void @llvm.stackrestore.p0(ptr %5362)
  %5369 = call ptr @llvm.stacksave.p0()
  %5370 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4782, ptr %5370, align 8
  %5371 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5370, 1
  %5372 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %5372, align 8
  %5373 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5372, 1
  %5374 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5371, ptr %5374, align 8
  %5375 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5373, ptr %5375, align 8
  call void @memrefCopy(i64 4, ptr %5374, ptr %5375)
  call void @llvm.stackrestore.p0(ptr %5369)
  %5376 = call ptr @llvm.stacksave.p0()
  %5377 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2140, ptr %5377, align 8
  %5378 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5377, 1
  %5379 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %5379, align 8
  %5380 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5379, 1
  %5381 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5378, ptr %5381, align 8
  %5382 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5380, ptr %5382, align 8
  call void @memrefCopy(i64 4, ptr %5381, ptr %5382)
  call void @llvm.stackrestore.p0(ptr %5376)
  %5383 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 1, i32 2052, i32 117)
  %5384 = call ptr @llvm.stacksave.p0()
  %5385 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4816, ptr %5385, align 8
  %5386 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5385, 1
  %5387 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %5387, align 8
  %5388 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5387, 1
  %5389 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5386, ptr %5389, align 8
  %5390 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5388, ptr %5390, align 8
  call void @memrefCopy(i64 4, ptr %5389, ptr %5390)
  call void @llvm.stackrestore.p0(ptr %5384)
  %5391 = call ptr @llvm.stacksave.p0()
  %5392 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2174, ptr %5392, align 8
  %5393 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5392, 1
  %5394 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %5394, align 8
  %5395 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5394, 1
  %5396 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5393, ptr %5396, align 8
  %5397 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5395, ptr %5397, align 8
  call void @memrefCopy(i64 4, ptr %5396, ptr %5397)
  call void @llvm.stackrestore.p0(ptr %5391)
  %5398 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 1, i32 2121, i32 117)
  %5399 = call ptr @llvm.stacksave.p0()
  %5400 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %5400, align 8
  %5401 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5400, 1
  %5402 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %5361, ptr %5402, align 8
  %5403 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5402, 1
  %5404 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5401, ptr %5404, align 8
  %5405 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5403, ptr %5405, align 8
  call void @memrefCopy(i64 4, ptr %5404, ptr %5405)
  call void @llvm.stackrestore.p0(ptr %5399)
  %5406 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %5407 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %5408 = insertvalue { ptr, ptr, i64 } poison, ptr %5406, 0
  %5409 = insertvalue { ptr, ptr, i64 } %5408, ptr %5407, 1
  %5410 = insertvalue { ptr, ptr, i64 } %5409, i64 0, 2
  %5411 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %5412 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %5413 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %5414 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %5415 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %5416 = extractvalue { ptr, ptr, i64 } %5410, 0
  %5417 = extractvalue { ptr, ptr, i64 } %5410, 1
  %5418 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %5416, 0
  %5419 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5418, ptr %5417, 1
  %5420 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5419, i64 1832, 2
  %5421 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5420, i64 4, 3, 0
  %5422 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5421, i64 64, 4, 0
  %5423 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5422, i64 4, 3, 1
  %5424 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5423, i64 1, 4, 1
  %5425 = call ptr @llvm.stacksave.p0()
  %5426 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %5424, ptr %5426, align 8
  %5427 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5426, 1
  %5428 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %5428, align 8
  %5429 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5428, 1
  %5430 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5427, ptr %5430, align 8
  %5431 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5429, ptr %5431, align 8
  call void @memrefCopy(i64 4, ptr %5430, ptr %5431)
  call void @llvm.stackrestore.p0(ptr %5425)
  %5432 = call ptr @llvm.stacksave.p0()
  %5433 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4782, ptr %5433, align 8
  %5434 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5433, 1
  %5435 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %5435, align 8
  %5436 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5435, 1
  %5437 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5434, ptr %5437, align 8
  %5438 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5436, ptr %5438, align 8
  call void @memrefCopy(i64 4, ptr %5437, ptr %5438)
  call void @llvm.stackrestore.p0(ptr %5432)
  %5439 = call ptr @llvm.stacksave.p0()
  %5440 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2241, ptr %5440, align 8
  %5441 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5440, 1
  %5442 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %5442, align 8
  %5443 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5442, 1
  %5444 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5441, ptr %5444, align 8
  %5445 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5443, ptr %5445, align 8
  call void @memrefCopy(i64 4, ptr %5444, ptr %5445)
  call void @llvm.stackrestore.p0(ptr %5439)
  %5446 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 2, i32 2052, i32 117)
  %5447 = call ptr @llvm.stacksave.p0()
  %5448 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4816, ptr %5448, align 8
  %5449 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5448, 1
  %5450 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %5450, align 8
  %5451 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5450, 1
  %5452 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5449, ptr %5452, align 8
  %5453 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5451, ptr %5453, align 8
  call void @memrefCopy(i64 4, ptr %5452, ptr %5453)
  call void @llvm.stackrestore.p0(ptr %5447)
  %5454 = call ptr @llvm.stacksave.p0()
  %5455 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2275, ptr %5455, align 8
  %5456 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5455, 1
  %5457 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %5457, align 8
  %5458 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5457, 1
  %5459 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5456, ptr %5459, align 8
  %5460 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5458, ptr %5460, align 8
  call void @memrefCopy(i64 4, ptr %5459, ptr %5460)
  call void @llvm.stackrestore.p0(ptr %5454)
  %5461 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 2, i32 2121, i32 117)
  %5462 = call ptr @llvm.stacksave.p0()
  %5463 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %5463, align 8
  %5464 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5463, 1
  %5465 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %5424, ptr %5465, align 8
  %5466 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5465, 1
  %5467 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5464, ptr %5467, align 8
  %5468 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5466, ptr %5468, align 8
  call void @memrefCopy(i64 4, ptr %5467, ptr %5468)
  call void @llvm.stackrestore.p0(ptr %5462)
  %5469 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %5470 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %5471 = insertvalue { ptr, ptr, i64 } poison, ptr %5469, 0
  %5472 = insertvalue { ptr, ptr, i64 } %5471, ptr %5470, 1
  %5473 = insertvalue { ptr, ptr, i64 } %5472, i64 0, 2
  %5474 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %5475 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %5476 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %5477 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %5478 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %5479 = extractvalue { ptr, ptr, i64 } %5473, 0
  %5480 = extractvalue { ptr, ptr, i64 } %5473, 1
  %5481 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %5479, 0
  %5482 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5481, ptr %5480, 1
  %5483 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5482, i64 1836, 2
  %5484 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5483, i64 4, 3, 0
  %5485 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5484, i64 64, 4, 0
  %5486 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5485, i64 4, 3, 1
  %5487 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5486, i64 1, 4, 1
  %5488 = call ptr @llvm.stacksave.p0()
  %5489 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %5487, ptr %5489, align 8
  %5490 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5489, 1
  %5491 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %5491, align 8
  %5492 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5491, 1
  %5493 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5490, ptr %5493, align 8
  %5494 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5492, ptr %5494, align 8
  call void @memrefCopy(i64 4, ptr %5493, ptr %5494)
  call void @llvm.stackrestore.p0(ptr %5488)
  %5495 = call ptr @llvm.stacksave.p0()
  %5496 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4782, ptr %5496, align 8
  %5497 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5496, 1
  %5498 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %5498, align 8
  %5499 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5498, 1
  %5500 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5497, ptr %5500, align 8
  %5501 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5499, ptr %5501, align 8
  call void @memrefCopy(i64 4, ptr %5500, ptr %5501)
  call void @llvm.stackrestore.p0(ptr %5495)
  %5502 = call ptr @llvm.stacksave.p0()
  %5503 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2342, ptr %5503, align 8
  %5504 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5503, 1
  %5505 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %5505, align 8
  %5506 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5505, 1
  %5507 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5504, ptr %5507, align 8
  %5508 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5506, ptr %5508, align 8
  call void @memrefCopy(i64 4, ptr %5507, ptr %5508)
  call void @llvm.stackrestore.p0(ptr %5502)
  %5509 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 3, i32 2052, i32 117)
  %5510 = call ptr @llvm.stacksave.p0()
  %5511 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4816, ptr %5511, align 8
  %5512 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5511, 1
  %5513 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %5513, align 8
  %5514 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5513, 1
  %5515 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5512, ptr %5515, align 8
  %5516 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5514, ptr %5516, align 8
  call void @memrefCopy(i64 4, ptr %5515, ptr %5516)
  call void @llvm.stackrestore.p0(ptr %5510)
  %5517 = call ptr @llvm.stacksave.p0()
  %5518 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2376, ptr %5518, align 8
  %5519 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5518, 1
  %5520 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %5520, align 8
  %5521 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5520, 1
  %5522 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5519, ptr %5522, align 8
  %5523 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5521, ptr %5523, align 8
  call void @memrefCopy(i64 4, ptr %5522, ptr %5523)
  call void @llvm.stackrestore.p0(ptr %5517)
  %5524 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 3, i32 2121, i32 117)
  %5525 = call ptr @llvm.stacksave.p0()
  %5526 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %5526, align 8
  %5527 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5526, 1
  %5528 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %5487, ptr %5528, align 8
  %5529 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5528, 1
  %5530 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5527, ptr %5530, align 8
  %5531 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5529, ptr %5531, align 8
  call void @memrefCopy(i64 4, ptr %5530, ptr %5531)
  call void @llvm.stackrestore.p0(ptr %5525)
  %5532 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %5533 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %5534 = insertvalue { ptr, ptr, i64 } poison, ptr %5532, 0
  %5535 = insertvalue { ptr, ptr, i64 } %5534, ptr %5533, 1
  %5536 = insertvalue { ptr, ptr, i64 } %5535, i64 0, 2
  %5537 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %5538 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %5539 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %5540 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %5541 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %5542 = extractvalue { ptr, ptr, i64 } %5536, 0
  %5543 = extractvalue { ptr, ptr, i64 } %5536, 1
  %5544 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %5542, 0
  %5545 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5544, ptr %5543, 1
  %5546 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5545, i64 1840, 2
  %5547 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5546, i64 4, 3, 0
  %5548 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5547, i64 64, 4, 0
  %5549 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5548, i64 4, 3, 1
  %5550 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5549, i64 1, 4, 1
  %5551 = call ptr @llvm.stacksave.p0()
  %5552 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %5550, ptr %5552, align 8
  %5553 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5552, 1
  %5554 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %5554, align 8
  %5555 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5554, 1
  %5556 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5553, ptr %5556, align 8
  %5557 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5555, ptr %5557, align 8
  call void @memrefCopy(i64 4, ptr %5556, ptr %5557)
  call void @llvm.stackrestore.p0(ptr %5551)
  %5558 = call ptr @llvm.stacksave.p0()
  %5559 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4782, ptr %5559, align 8
  %5560 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5559, 1
  %5561 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %5561, align 8
  %5562 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5561, 1
  %5563 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5560, ptr %5563, align 8
  %5564 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5562, ptr %5564, align 8
  call void @memrefCopy(i64 4, ptr %5563, ptr %5564)
  call void @llvm.stackrestore.p0(ptr %5558)
  %5565 = call ptr @llvm.stacksave.p0()
  %5566 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2443, ptr %5566, align 8
  %5567 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5566, 1
  %5568 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %5568, align 8
  %5569 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5568, 1
  %5570 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5567, ptr %5570, align 8
  %5571 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5569, ptr %5571, align 8
  call void @memrefCopy(i64 4, ptr %5570, ptr %5571)
  call void @llvm.stackrestore.p0(ptr %5565)
  %5572 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 1, i32 2190, i32 117)
  %5573 = call ptr @llvm.stacksave.p0()
  %5574 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4816, ptr %5574, align 8
  %5575 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5574, 1
  %5576 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %5576, align 8
  %5577 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5576, 1
  %5578 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5575, ptr %5578, align 8
  %5579 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5577, ptr %5579, align 8
  call void @memrefCopy(i64 4, ptr %5578, ptr %5579)
  call void @llvm.stackrestore.p0(ptr %5573)
  %5580 = call ptr @llvm.stacksave.p0()
  %5581 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2477, ptr %5581, align 8
  %5582 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5581, 1
  %5583 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %5583, align 8
  %5584 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5583, 1
  %5585 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5582, ptr %5585, align 8
  %5586 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5584, ptr %5586, align 8
  call void @memrefCopy(i64 4, ptr %5585, ptr %5586)
  call void @llvm.stackrestore.p0(ptr %5580)
  %5587 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 1, i32 2259, i32 117)
  %5588 = call ptr @llvm.stacksave.p0()
  %5589 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %5589, align 8
  %5590 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5589, 1
  %5591 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %5550, ptr %5591, align 8
  %5592 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5591, 1
  %5593 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5590, ptr %5593, align 8
  %5594 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5592, ptr %5594, align 8
  call void @memrefCopy(i64 4, ptr %5593, ptr %5594)
  call void @llvm.stackrestore.p0(ptr %5588)
  %5595 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %5596 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %5597 = insertvalue { ptr, ptr, i64 } poison, ptr %5595, 0
  %5598 = insertvalue { ptr, ptr, i64 } %5597, ptr %5596, 1
  %5599 = insertvalue { ptr, ptr, i64 } %5598, i64 0, 2
  %5600 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %5601 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %5602 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %5603 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %5604 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %5605 = extractvalue { ptr, ptr, i64 } %5599, 0
  %5606 = extractvalue { ptr, ptr, i64 } %5599, 1
  %5607 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %5605, 0
  %5608 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5607, ptr %5606, 1
  %5609 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5608, i64 1844, 2
  %5610 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5609, i64 4, 3, 0
  %5611 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5610, i64 64, 4, 0
  %5612 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5611, i64 4, 3, 1
  %5613 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5612, i64 1, 4, 1
  %5614 = call ptr @llvm.stacksave.p0()
  %5615 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %5613, ptr %5615, align 8
  %5616 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5615, 1
  %5617 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %5617, align 8
  %5618 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5617, 1
  %5619 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5616, ptr %5619, align 8
  %5620 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5618, ptr %5620, align 8
  call void @memrefCopy(i64 4, ptr %5619, ptr %5620)
  call void @llvm.stackrestore.p0(ptr %5614)
  %5621 = call ptr @llvm.stacksave.p0()
  %5622 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4782, ptr %5622, align 8
  %5623 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5622, 1
  %5624 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %5624, align 8
  %5625 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5624, 1
  %5626 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5623, ptr %5626, align 8
  %5627 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5625, ptr %5627, align 8
  call void @memrefCopy(i64 4, ptr %5626, ptr %5627)
  call void @llvm.stackrestore.p0(ptr %5621)
  %5628 = call ptr @llvm.stacksave.p0()
  %5629 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2544, ptr %5629, align 8
  %5630 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5629, 1
  %5631 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %5631, align 8
  %5632 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5631, 1
  %5633 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5630, ptr %5633, align 8
  %5634 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5632, ptr %5634, align 8
  call void @memrefCopy(i64 4, ptr %5633, ptr %5634)
  call void @llvm.stackrestore.p0(ptr %5628)
  %5635 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 2, i32 2190, i32 117)
  %5636 = call ptr @llvm.stacksave.p0()
  %5637 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4816, ptr %5637, align 8
  %5638 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5637, 1
  %5639 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %5639, align 8
  %5640 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5639, 1
  %5641 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5638, ptr %5641, align 8
  %5642 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5640, ptr %5642, align 8
  call void @memrefCopy(i64 4, ptr %5641, ptr %5642)
  call void @llvm.stackrestore.p0(ptr %5636)
  %5643 = call ptr @llvm.stacksave.p0()
  %5644 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2578, ptr %5644, align 8
  %5645 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5644, 1
  %5646 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %5646, align 8
  %5647 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5646, 1
  %5648 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5645, ptr %5648, align 8
  %5649 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5647, ptr %5649, align 8
  call void @memrefCopy(i64 4, ptr %5648, ptr %5649)
  call void @llvm.stackrestore.p0(ptr %5643)
  %5650 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 2, i32 2259, i32 117)
  %5651 = call ptr @llvm.stacksave.p0()
  %5652 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %5652, align 8
  %5653 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5652, 1
  %5654 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %5613, ptr %5654, align 8
  %5655 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5654, 1
  %5656 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5653, ptr %5656, align 8
  %5657 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5655, ptr %5657, align 8
  call void @memrefCopy(i64 4, ptr %5656, ptr %5657)
  call void @llvm.stackrestore.p0(ptr %5651)
  %5658 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %5659 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %5660 = insertvalue { ptr, ptr, i64 } poison, ptr %5658, 0
  %5661 = insertvalue { ptr, ptr, i64 } %5660, ptr %5659, 1
  %5662 = insertvalue { ptr, ptr, i64 } %5661, i64 0, 2
  %5663 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %5664 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %5665 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %5666 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %5667 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %5668 = extractvalue { ptr, ptr, i64 } %5662, 0
  %5669 = extractvalue { ptr, ptr, i64 } %5662, 1
  %5670 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %5668, 0
  %5671 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5670, ptr %5669, 1
  %5672 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5671, i64 1848, 2
  %5673 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5672, i64 4, 3, 0
  %5674 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5673, i64 64, 4, 0
  %5675 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5674, i64 4, 3, 1
  %5676 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5675, i64 1, 4, 1
  %5677 = call ptr @llvm.stacksave.p0()
  %5678 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %5676, ptr %5678, align 8
  %5679 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5678, 1
  %5680 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %5680, align 8
  %5681 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5680, 1
  %5682 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5679, ptr %5682, align 8
  %5683 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5681, ptr %5683, align 8
  call void @memrefCopy(i64 4, ptr %5682, ptr %5683)
  call void @llvm.stackrestore.p0(ptr %5677)
  %5684 = call ptr @llvm.stacksave.p0()
  %5685 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4782, ptr %5685, align 8
  %5686 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5685, 1
  %5687 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %5687, align 8
  %5688 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5687, 1
  %5689 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5686, ptr %5689, align 8
  %5690 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5688, ptr %5690, align 8
  call void @memrefCopy(i64 4, ptr %5689, ptr %5690)
  call void @llvm.stackrestore.p0(ptr %5684)
  %5691 = call ptr @llvm.stacksave.p0()
  %5692 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2645, ptr %5692, align 8
  %5693 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5692, 1
  %5694 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %5694, align 8
  %5695 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5694, 1
  %5696 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5693, ptr %5696, align 8
  %5697 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5695, ptr %5697, align 8
  call void @memrefCopy(i64 4, ptr %5696, ptr %5697)
  call void @llvm.stackrestore.p0(ptr %5691)
  %5698 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 3, i32 2190, i32 117)
  %5699 = call ptr @llvm.stacksave.p0()
  %5700 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4816, ptr %5700, align 8
  %5701 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5700, 1
  %5702 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %5702, align 8
  %5703 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5702, 1
  %5704 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5701, ptr %5704, align 8
  %5705 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5703, ptr %5705, align 8
  call void @memrefCopy(i64 4, ptr %5704, ptr %5705)
  call void @llvm.stackrestore.p0(ptr %5699)
  %5706 = call ptr @llvm.stacksave.p0()
  %5707 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2679, ptr %5707, align 8
  %5708 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5707, 1
  %5709 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %5709, align 8
  %5710 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5709, 1
  %5711 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5708, ptr %5711, align 8
  %5712 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5710, ptr %5712, align 8
  call void @memrefCopy(i64 4, ptr %5711, ptr %5712)
  call void @llvm.stackrestore.p0(ptr %5706)
  %5713 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 3, i32 2259, i32 117)
  %5714 = call ptr @llvm.stacksave.p0()
  %5715 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %5715, align 8
  %5716 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5715, 1
  %5717 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %5676, ptr %5717, align 8
  %5718 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5717, 1
  %5719 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5716, ptr %5719, align 8
  %5720 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5718, ptr %5720, align 8
  call void @memrefCopy(i64 4, ptr %5719, ptr %5720)
  call void @llvm.stackrestore.p0(ptr %5714)
  %5721 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %5722 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %5723 = insertvalue { ptr, ptr, i64 } poison, ptr %5721, 0
  %5724 = insertvalue { ptr, ptr, i64 } %5723, ptr %5722, 1
  %5725 = insertvalue { ptr, ptr, i64 } %5724, i64 0, 2
  %5726 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %5727 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %5728 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %5729 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %5730 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %5731 = extractvalue { ptr, ptr, i64 } %5725, 0
  %5732 = extractvalue { ptr, ptr, i64 } %5725, 1
  %5733 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %5731, 0
  %5734 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5733, ptr %5732, 1
  %5735 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5734, i64 1852, 2
  %5736 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5735, i64 4, 3, 0
  %5737 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5736, i64 64, 4, 0
  %5738 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5737, i64 4, 3, 1
  %5739 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5738, i64 1, 4, 1
  %5740 = call ptr @llvm.stacksave.p0()
  %5741 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %5739, ptr %5741, align 8
  %5742 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5741, 1
  %5743 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %5743, align 8
  %5744 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5743, 1
  %5745 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5742, ptr %5745, align 8
  %5746 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5744, ptr %5746, align 8
  call void @memrefCopy(i64 4, ptr %5745, ptr %5746)
  call void @llvm.stackrestore.p0(ptr %5740)
  %5747 = call ptr @llvm.stacksave.p0()
  %5748 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4782, ptr %5748, align 8
  %5749 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5748, 1
  %5750 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %5750, align 8
  %5751 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5750, 1
  %5752 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5749, ptr %5752, align 8
  %5753 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5751, ptr %5753, align 8
  call void @memrefCopy(i64 4, ptr %5752, ptr %5753)
  call void @llvm.stackrestore.p0(ptr %5747)
  %5754 = call ptr @llvm.stacksave.p0()
  %5755 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2746, ptr %5755, align 8
  %5756 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5755, 1
  %5757 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %5757, align 8
  %5758 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5757, 1
  %5759 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5756, ptr %5759, align 8
  %5760 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5758, ptr %5760, align 8
  call void @memrefCopy(i64 4, ptr %5759, ptr %5760)
  call void @llvm.stackrestore.p0(ptr %5754)
  %5761 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 1, i32 2328, i32 117)
  %5762 = call ptr @llvm.stacksave.p0()
  %5763 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %4816, ptr %5763, align 8
  %5764 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5763, 1
  %5765 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, ptr %5765, align 8
  %5766 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5765, 1
  %5767 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5764, ptr %5767, align 8
  %5768 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5766, ptr %5768, align 8
  call void @memrefCopy(i64 4, ptr %5767, ptr %5768)
  call void @llvm.stackrestore.p0(ptr %5762)
  %5769 = call ptr @llvm.stacksave.p0()
  %5770 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2780, ptr %5770, align 8
  %5771 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5770, 1
  %5772 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1815, ptr %5772, align 8
  %5773 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5772, 1
  %5774 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5771, ptr %5774, align 8
  %5775 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5773, ptr %5775, align 8
  call void @memrefCopy(i64 4, ptr %5774, ptr %5775)
  call void @llvm.stackrestore.p0(ptr %5769)
  %5776 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %1787, ptr %1825, ptr %1749, i32 1, i32 2397, i32 117)
  %5777 = call ptr @llvm.stacksave.p0()
  %5778 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1739, ptr %5778, align 8
  %5779 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5778, 1
  %5780 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %5739, ptr %5780, align 8
  %5781 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %5780, 1
  %5782 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5779, ptr %5782, align 8
  %5783 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %5781, ptr %5783, align 8
  call void @memrefCopy(i64 4, ptr %5782, ptr %5783)
  call void @llvm.stackrestore.p0(ptr %5777)
  %5784 = call ptr @malloc(i64 8192)
  %5785 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %5784, 0
  %5786 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5785, ptr %5784, 1
  %5787 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5786, i64 0, 2
  %5788 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5787, i64 32, 3, 0
  %5789 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5788, i64 64, 3, 1
  %5790 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5789, i64 64, 4, 0
  %5791 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5790, i64 1, 4, 1
  %5792 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %5793 = mul i64 1, %5792
  %5794 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %5795 = mul i64 %5793, %5794
  %5796 = mul i64 %5795, 4
  %5797 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %5798 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %5799 = getelementptr float, ptr %5797, i64 %5798
  %5800 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5791, 1
  %5801 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %5791, 2
  %5802 = getelementptr float, ptr %5800, i64 %5801
  call void @llvm.memcpy.p0.p0.i64(ptr %5802, ptr %5799, i64 %5796, i1 false)
  ret { ptr, ptr, i64, [2 x i64], [2 x i64] } %5791
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
