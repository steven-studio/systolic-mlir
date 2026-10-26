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
  %77 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %76, i64 16, 4, 0
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
  %115 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %114, i64 31, 4, 0
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
  %153 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %152, i64 16, 4, 0
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
  %192 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %191, i64 31, 4, 0
  %193 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %192, i64 15, 3, 1
  %194 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %193, i64 1, 4, 1
  %195 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 0
  %196 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %197 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %195, 0
  %198 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %197, ptr %196, 1
  %199 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %198, i64 0, 2
  %200 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %199, i64 8, 3, 0
  %201 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %200, i64 15, 4, 0
  %202 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %201, i64 15, 3, 1
  %203 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %202, i64 1, 4, 1
  %204 = call ptr @llvm.stacksave.p0()
  %205 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %194, ptr %205, align 8
  %206 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %205, 1
  %207 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %203, ptr %207, align 8
  %208 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %207, 1
  %209 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %206, ptr %209, align 8
  %210 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %208, ptr %210, align 8
  call void @memrefCopy(i64 4, ptr %209, ptr %210)
  call void @llvm.stackrestore.p0(ptr %204)
  %211 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %212 = ptrtoint ptr %211 to i64
  %213 = inttoptr i64 %212 to ptr
  %214 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %215 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %216 = insertvalue { ptr, ptr, i64 } poison, ptr %214, 0
  %217 = insertvalue { ptr, ptr, i64 } %216, ptr %215, 1
  %218 = insertvalue { ptr, ptr, i64 } %217, i64 0, 2
  %219 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %220 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %221 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %222 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %223 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %224 = extractvalue { ptr, ptr, i64 } %218, 0
  %225 = extractvalue { ptr, ptr, i64 } %218, 1
  %226 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %224, 0
  %227 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %226, ptr %225, 1
  %228 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %227, i64 256, 2
  %229 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %228, i64 15, 3, 0
  %230 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %229, i64 16, 4, 0
  %231 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %230, i64 8, 3, 1
  %232 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %231, i64 1, 4, 1
  %233 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 0
  %234 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %235 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %233, 0
  %236 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %235, ptr %234, 1
  %237 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %236, i64 0, 2
  %238 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %237, i64 15, 3, 0
  %239 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %238, i64 8, 4, 0
  %240 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %239, i64 8, 3, 1
  %241 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %240, i64 1, 4, 1
  %242 = call ptr @llvm.stacksave.p0()
  %243 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %232, ptr %243, align 8
  %244 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %243, 1
  %245 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %241, ptr %245, align 8
  %246 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %245, 1
  %247 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %244, ptr %247, align 8
  %248 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %246, ptr %248, align 8
  call void @memrefCopy(i64 4, ptr %247, ptr %248)
  call void @llvm.stackrestore.p0(ptr %242)
  %249 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %250 = ptrtoint ptr %249 to i64
  %251 = inttoptr i64 %250 to ptr
  %252 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 15, i32 8, ptr %213, ptr %251, ptr %98, i32 0, i32 42, i32 124)
  %253 = call ptr @llvm.stacksave.p0()
  %254 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %254, align 8
  %255 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %254, 1
  %256 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %79, ptr %256, align 8
  %257 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %256, 1
  %258 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %255, ptr %258, align 8
  %259 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %257, ptr %259, align 8
  call void @memrefCopy(i64 4, ptr %258, ptr %259)
  call void @llvm.stackrestore.p0(ptr %253)
  %260 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %261 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %262 = insertvalue { ptr, ptr, i64 } poison, ptr %260, 0
  %263 = insertvalue { ptr, ptr, i64 } %262, ptr %261, 1
  %264 = insertvalue { ptr, ptr, i64 } %263, i64 0, 2
  %265 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %266 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %267 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %268 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %269 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %270 = extractvalue { ptr, ptr, i64 } %264, 0
  %271 = extractvalue { ptr, ptr, i64 } %264, 1
  %272 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %270, 0
  %273 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %272, ptr %271, 1
  %274 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %273, i64 8, 2
  %275 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %274, i64 8, 3, 0
  %276 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %275, i64 16, 4, 0
  %277 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %276, i64 8, 3, 1
  %278 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %277, i64 1, 4, 1
  %279 = call ptr @llvm.stacksave.p0()
  %280 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %278, ptr %280, align 8
  %281 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %280, 1
  %282 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %282, align 8
  %283 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %282, 1
  %284 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %281, ptr %284, align 8
  %285 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %283, ptr %285, align 8
  call void @memrefCopy(i64 4, ptr %284, ptr %285)
  call void @llvm.stackrestore.p0(ptr %279)
  %286 = call ptr @llvm.stacksave.p0()
  %287 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %117, ptr %287, align 8
  %288 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %287, 1
  %289 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %289, align 8
  %290 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %289, 1
  %291 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %288, ptr %291, align 8
  %292 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %290, ptr %292, align 8
  call void @memrefCopy(i64 4, ptr %291, ptr %292)
  call void @llvm.stackrestore.p0(ptr %286)
  %293 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %294 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %295 = insertvalue { ptr, ptr, i64 } poison, ptr %293, 0
  %296 = insertvalue { ptr, ptr, i64 } %295, ptr %294, 1
  %297 = insertvalue { ptr, ptr, i64 } %296, i64 0, 2
  %298 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %299 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %300 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %301 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %302 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %303 = extractvalue { ptr, ptr, i64 } %297, 0
  %304 = extractvalue { ptr, ptr, i64 } %297, 1
  %305 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %303, 0
  %306 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %305, ptr %304, 1
  %307 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %306, i64 8, 2
  %308 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %307, i64 16, 3, 0
  %309 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %308, i64 16, 4, 0
  %310 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %309, i64 8, 3, 1
  %311 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %310, i64 1, 4, 1
  %312 = call ptr @llvm.stacksave.p0()
  %313 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %311, ptr %313, align 8
  %314 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %313, 1
  %315 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %315, align 8
  %316 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %315, 1
  %317 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %314, ptr %317, align 8
  %318 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %316, ptr %318, align 8
  call void @memrefCopy(i64 4, ptr %317, ptr %318)
  call void @llvm.stackrestore.p0(ptr %312)
  %319 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 128, i32 125)
  %320 = call ptr @llvm.stacksave.p0()
  %321 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %194, ptr %321, align 8
  %322 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %321, 1
  %323 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %203, ptr %323, align 8
  %324 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %323, 1
  %325 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %322, ptr %325, align 8
  %326 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %324, ptr %326, align 8
  call void @memrefCopy(i64 4, ptr %325, ptr %326)
  call void @llvm.stackrestore.p0(ptr %320)
  %327 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %328 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %329 = insertvalue { ptr, ptr, i64 } poison, ptr %327, 0
  %330 = insertvalue { ptr, ptr, i64 } %329, ptr %328, 1
  %331 = insertvalue { ptr, ptr, i64 } %330, i64 0, 2
  %332 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %333 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %334 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %335 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %336 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %337 = extractvalue { ptr, ptr, i64 } %331, 0
  %338 = extractvalue { ptr, ptr, i64 } %331, 1
  %339 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %337, 0
  %340 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %339, ptr %338, 1
  %341 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %340, i64 264, 2
  %342 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %341, i64 15, 3, 0
  %343 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %342, i64 16, 4, 0
  %344 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %343, i64 8, 3, 1
  %345 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %344, i64 1, 4, 1
  %346 = call ptr @llvm.stacksave.p0()
  %347 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %345, ptr %347, align 8
  %348 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %347, 1
  %349 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %241, ptr %349, align 8
  %350 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %349, 1
  %351 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %348, ptr %351, align 8
  %352 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %350, ptr %352, align 8
  call void @memrefCopy(i64 4, ptr %351, ptr %352)
  call void @llvm.stackrestore.p0(ptr %346)
  %353 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 15, i32 8, ptr %213, ptr %251, ptr %98, i32 0, i32 197, i32 124)
  %354 = call ptr @llvm.stacksave.p0()
  %355 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %355, align 8
  %356 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %355, 1
  %357 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %278, ptr %357, align 8
  %358 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %357, 1
  %359 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %356, ptr %359, align 8
  %360 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %358, ptr %360, align 8
  call void @memrefCopy(i64 4, ptr %359, ptr %360)
  call void @llvm.stackrestore.p0(ptr %354)
  %361 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %362 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %363 = insertvalue { ptr, ptr, i64 } poison, ptr %361, 0
  %364 = insertvalue { ptr, ptr, i64 } %363, ptr %362, 1
  %365 = insertvalue { ptr, ptr, i64 } %364, i64 0, 2
  %366 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %367 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %368 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %369 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %370 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %371 = extractvalue { ptr, ptr, i64 } %365, 0
  %372 = extractvalue { ptr, ptr, i64 } %365, 1
  %373 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %371, 0
  %374 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %373, ptr %372, 1
  %375 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %374, i64 128, 2
  %376 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %375, i64 8, 3, 0
  %377 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %376, i64 16, 4, 0
  %378 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %377, i64 8, 3, 1
  %379 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %378, i64 1, 4, 1
  %380 = call ptr @llvm.stacksave.p0()
  %381 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %379, ptr %381, align 8
  %382 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %381, 1
  %383 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %383, align 8
  %384 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %383, 1
  %385 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %382, ptr %385, align 8
  %386 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %384, ptr %386, align 8
  call void @memrefCopy(i64 4, ptr %385, ptr %386)
  call void @llvm.stackrestore.p0(ptr %380)
  %387 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %388 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %389 = insertvalue { ptr, ptr, i64 } poison, ptr %387, 0
  %390 = insertvalue { ptr, ptr, i64 } %389, ptr %388, 1
  %391 = insertvalue { ptr, ptr, i64 } %390, i64 0, 2
  %392 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %393 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %394 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %395 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %396 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %397 = extractvalue { ptr, ptr, i64 } %391, 0
  %398 = extractvalue { ptr, ptr, i64 } %391, 1
  %399 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %397, 0
  %400 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %399, ptr %398, 1
  %401 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %400, i64 248, 2
  %402 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %401, i64 8, 3, 0
  %403 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %402, i64 31, 4, 0
  %404 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %403, i64 16, 3, 1
  %405 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %404, i64 1, 4, 1
  %406 = call ptr @llvm.stacksave.p0()
  %407 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %405, ptr %407, align 8
  %408 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %407, 1
  %409 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, ptr %409, align 8
  %410 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %409, 1
  %411 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %408, ptr %411, align 8
  %412 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %410, ptr %412, align 8
  call void @memrefCopy(i64 4, ptr %411, ptr %412)
  call void @llvm.stackrestore.p0(ptr %406)
  %413 = call ptr @llvm.stacksave.p0()
  %414 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %155, ptr %414, align 8
  %415 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %414, 1
  %416 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %416, align 8
  %417 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %416, 1
  %418 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %415, ptr %418, align 8
  %419 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %417, ptr %419, align 8
  call void @memrefCopy(i64 4, ptr %418, ptr %419)
  call void @llvm.stackrestore.p0(ptr %413)
  %420 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %136, ptr %174, ptr %98, i32 0, i32 266, i32 125)
  %421 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %422 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %423 = insertvalue { ptr, ptr, i64 } poison, ptr %421, 0
  %424 = insertvalue { ptr, ptr, i64 } %423, ptr %422, 1
  %425 = insertvalue { ptr, ptr, i64 } %424, i64 0, 2
  %426 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %427 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %428 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %429 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %430 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %431 = extractvalue { ptr, ptr, i64 } %425, 0
  %432 = extractvalue { ptr, ptr, i64 } %425, 1
  %433 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %431, 0
  %434 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %433, ptr %432, 1
  %435 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %434, i64 264, 2
  %436 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %435, i64 8, 3, 0
  %437 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %436, i64 31, 4, 0
  %438 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %437, i64 15, 3, 1
  %439 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %438, i64 1, 4, 1
  %440 = call ptr @llvm.stacksave.p0()
  %441 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %439, ptr %441, align 8
  %442 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %441, 1
  %443 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %203, ptr %443, align 8
  %444 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %443, 1
  %445 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %442, ptr %445, align 8
  %446 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %444, ptr %446, align 8
  call void @memrefCopy(i64 4, ptr %445, ptr %446)
  call void @llvm.stackrestore.p0(ptr %440)
  %447 = call ptr @llvm.stacksave.p0()
  %448 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %232, ptr %448, align 8
  %449 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %448, 1
  %450 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %241, ptr %450, align 8
  %451 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %450, 1
  %452 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %449, ptr %452, align 8
  %453 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %451, ptr %453, align 8
  call void @memrefCopy(i64 4, ptr %452, ptr %453)
  call void @llvm.stackrestore.p0(ptr %447)
  %454 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 15, i32 8, ptr %213, ptr %251, ptr %98, i32 0, i32 335, i32 124)
  %455 = call ptr @llvm.stacksave.p0()
  %456 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %456, align 8
  %457 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %456, 1
  %458 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %379, ptr %458, align 8
  %459 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %458, 1
  %460 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %457, ptr %460, align 8
  %461 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %459, ptr %461, align 8
  call void @memrefCopy(i64 4, ptr %460, ptr %461)
  call void @llvm.stackrestore.p0(ptr %455)
  %462 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %463 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %464 = insertvalue { ptr, ptr, i64 } poison, ptr %462, 0
  %465 = insertvalue { ptr, ptr, i64 } %464, ptr %463, 1
  %466 = insertvalue { ptr, ptr, i64 } %465, i64 0, 2
  %467 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %468 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %469 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %470 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %471 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %472 = extractvalue { ptr, ptr, i64 } %466, 0
  %473 = extractvalue { ptr, ptr, i64 } %466, 1
  %474 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %472, 0
  %475 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %474, ptr %473, 1
  %476 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %475, i64 136, 2
  %477 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %476, i64 4, 3, 0
  %478 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %477, i64 16, 4, 0
  %479 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %478, i64 4, 3, 1
  %480 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %479, i64 1, 4, 1
  %481 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 0
  %482 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 1
  %483 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %481, 0
  %484 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %483, ptr %482, 1
  %485 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %484, i64 0, 2
  %486 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %485, i64 4, 3, 0
  %487 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %486, i64 4, 4, 0
  %488 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %487, i64 4, 3, 1
  %489 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %488, i64 1, 4, 1
  %490 = call ptr @llvm.stacksave.p0()
  %491 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %480, ptr %491, align 8
  %492 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %491, 1
  %493 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %489, ptr %493, align 8
  %494 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %493, 1
  %495 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %492, ptr %495, align 8
  %496 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %494, ptr %496, align 8
  call void @memrefCopy(i64 4, ptr %495, ptr %496)
  call void @llvm.stackrestore.p0(ptr %490)
  %497 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 1
  %498 = ptrtoint ptr %497 to i64
  %499 = inttoptr i64 %498 to ptr
  %500 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %501 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %502 = insertvalue { ptr, ptr, i64 } poison, ptr %500, 0
  %503 = insertvalue { ptr, ptr, i64 } %502, ptr %501, 1
  %504 = insertvalue { ptr, ptr, i64 } %503, i64 0, 2
  %505 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %506 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %507 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %508 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %509 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %510 = extractvalue { ptr, ptr, i64 } %504, 0
  %511 = extractvalue { ptr, ptr, i64 } %504, 1
  %512 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %510, 0
  %513 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %512, ptr %511, 1
  %514 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %513, i64 248, 2
  %515 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %514, i64 4, 3, 0
  %516 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %515, i64 31, 4, 0
  %517 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %516, i64 16, 3, 1
  %518 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %517, i64 1, 4, 1
  %519 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 0
  %520 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %521 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %519, 0
  %522 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %521, ptr %520, 1
  %523 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %522, i64 0, 2
  %524 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %523, i64 4, 3, 0
  %525 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %524, i64 16, 4, 0
  %526 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %525, i64 16, 3, 1
  %527 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %526, i64 1, 4, 1
  %528 = call ptr @llvm.stacksave.p0()
  %529 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %518, ptr %529, align 8
  %530 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %529, 1
  %531 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %527, ptr %531, align 8
  %532 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %531, 1
  %533 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %530, ptr %533, align 8
  %534 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %532, ptr %534, align 8
  call void @memrefCopy(i64 4, ptr %533, ptr %534)
  call void @llvm.stackrestore.p0(ptr %528)
  %535 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %536 = ptrtoint ptr %535 to i64
  %537 = inttoptr i64 %536 to ptr
  %538 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %539 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %540 = insertvalue { ptr, ptr, i64 } poison, ptr %538, 0
  %541 = insertvalue { ptr, ptr, i64 } %540, ptr %539, 1
  %542 = insertvalue { ptr, ptr, i64 } %541, i64 0, 2
  %543 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %544 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %545 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %546 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %547 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %548 = extractvalue { ptr, ptr, i64 } %542, 0
  %549 = extractvalue { ptr, ptr, i64 } %542, 1
  %550 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %548, 0
  %551 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %550, ptr %549, 1
  %552 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %551, i64 8, 2
  %553 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %552, i64 16, 3, 0
  %554 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %553, i64 16, 4, 0
  %555 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %554, i64 4, 3, 1
  %556 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %555, i64 1, 4, 1
  %557 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 0
  %558 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %559 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %557, 0
  %560 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %559, ptr %558, 1
  %561 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %560, i64 0, 2
  %562 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %561, i64 16, 3, 0
  %563 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %562, i64 4, 4, 0
  %564 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %563, i64 4, 3, 1
  %565 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %564, i64 1, 4, 1
  %566 = call ptr @llvm.stacksave.p0()
  %567 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %556, ptr %567, align 8
  %568 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %567, 1
  %569 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %565, ptr %569, align 8
  %570 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %569, 1
  %571 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %568, ptr %571, align 8
  %572 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %570, ptr %572, align 8
  call void @memrefCopy(i64 4, ptr %571, ptr %572)
  call void @llvm.stackrestore.p0(ptr %566)
  %573 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %574 = ptrtoint ptr %573 to i64
  %575 = inttoptr i64 %574 to ptr
  %576 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %537, ptr %575, ptr %499, i32 1, i32 0, i32 117)
  %577 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %578 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %579 = insertvalue { ptr, ptr, i64 } poison, ptr %577, 0
  %580 = insertvalue { ptr, ptr, i64 } %579, ptr %578, 1
  %581 = insertvalue { ptr, ptr, i64 } %580, i64 0, 2
  %582 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %583 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %584 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %585 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %586 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %587 = extractvalue { ptr, ptr, i64 } %581, 0
  %588 = extractvalue { ptr, ptr, i64 } %581, 1
  %589 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %587, 0
  %590 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %589, ptr %588, 1
  %591 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %590, i64 264, 2
  %592 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %591, i64 4, 3, 0
  %593 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %592, i64 31, 4, 0
  %594 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %593, i64 15, 3, 1
  %595 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %594, i64 1, 4, 1
  %596 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 0
  %597 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %598 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %596, 0
  %599 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %598, ptr %597, 1
  %600 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %599, i64 0, 2
  %601 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %600, i64 4, 3, 0
  %602 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %601, i64 15, 4, 0
  %603 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %602, i64 15, 3, 1
  %604 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %603, i64 1, 4, 1
  %605 = call ptr @llvm.stacksave.p0()
  %606 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %595, ptr %606, align 8
  %607 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %606, 1
  %608 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %604, ptr %608, align 8
  %609 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %608, 1
  %610 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %607, ptr %610, align 8
  %611 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %609, ptr %611, align 8
  call void @memrefCopy(i64 4, ptr %610, ptr %611)
  call void @llvm.stackrestore.p0(ptr %605)
  %612 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %613 = ptrtoint ptr %612 to i64
  %614 = inttoptr i64 %613 to ptr
  %615 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %616 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %617 = insertvalue { ptr, ptr, i64 } poison, ptr %615, 0
  %618 = insertvalue { ptr, ptr, i64 } %617, ptr %616, 1
  %619 = insertvalue { ptr, ptr, i64 } %618, i64 0, 2
  %620 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %621 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %622 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %623 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %624 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %625 = extractvalue { ptr, ptr, i64 } %619, 0
  %626 = extractvalue { ptr, ptr, i64 } %619, 1
  %627 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %625, 0
  %628 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %627, ptr %626, 1
  %629 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %628, i64 264, 2
  %630 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %629, i64 15, 3, 0
  %631 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %630, i64 16, 4, 0
  %632 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %631, i64 4, 3, 1
  %633 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %632, i64 1, 4, 1
  %634 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 0
  %635 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %636 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %634, 0
  %637 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %636, ptr %635, 1
  %638 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %637, i64 0, 2
  %639 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %638, i64 15, 3, 0
  %640 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %639, i64 4, 4, 0
  %641 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %640, i64 4, 3, 1
  %642 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %641, i64 1, 4, 1
  %643 = call ptr @llvm.stacksave.p0()
  %644 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %633, ptr %644, align 8
  %645 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %644, 1
  %646 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %642, ptr %646, align 8
  %647 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %646, 1
  %648 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %645, ptr %648, align 8
  %649 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %647, ptr %649, align 8
  call void @memrefCopy(i64 4, ptr %648, ptr %649)
  call void @llvm.stackrestore.p0(ptr %643)
  %650 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %651 = ptrtoint ptr %650 to i64
  %652 = inttoptr i64 %651 to ptr
  %653 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 15, i32 4, ptr %614, ptr %652, ptr %499, i32 1, i32 42, i32 116)
  %654 = call ptr @llvm.stacksave.p0()
  %655 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %489, ptr %655, align 8
  %656 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %655, 1
  %657 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %480, ptr %657, align 8
  %658 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %657, 1
  %659 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %656, ptr %659, align 8
  %660 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %658, ptr %660, align 8
  call void @memrefCopy(i64 4, ptr %659, ptr %660)
  call void @llvm.stackrestore.p0(ptr %654)
  %661 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %662 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %663 = insertvalue { ptr, ptr, i64 } poison, ptr %661, 0
  %664 = insertvalue { ptr, ptr, i64 } %663, ptr %662, 1
  %665 = insertvalue { ptr, ptr, i64 } %664, i64 0, 2
  %666 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %667 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %668 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %669 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %670 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %671 = extractvalue { ptr, ptr, i64 } %665, 0
  %672 = extractvalue { ptr, ptr, i64 } %665, 1
  %673 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %671, 0
  %674 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %673, ptr %672, 1
  %675 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %674, i64 140, 2
  %676 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %675, i64 4, 3, 0
  %677 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %676, i64 16, 4, 0
  %678 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %677, i64 4, 3, 1
  %679 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %678, i64 1, 4, 1
  %680 = call ptr @llvm.stacksave.p0()
  %681 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %679, ptr %681, align 8
  %682 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %681, 1
  %683 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %489, ptr %683, align 8
  %684 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %683, 1
  %685 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %682, ptr %685, align 8
  %686 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %684, ptr %686, align 8
  call void @memrefCopy(i64 4, ptr %685, ptr %686)
  call void @llvm.stackrestore.p0(ptr %680)
  %687 = call ptr @llvm.stacksave.p0()
  %688 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %518, ptr %688, align 8
  %689 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %688, 1
  %690 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %527, ptr %690, align 8
  %691 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %690, 1
  %692 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %689, ptr %692, align 8
  %693 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %691, ptr %693, align 8
  call void @memrefCopy(i64 4, ptr %692, ptr %693)
  call void @llvm.stackrestore.p0(ptr %687)
  %694 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %695 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %696 = insertvalue { ptr, ptr, i64 } poison, ptr %694, 0
  %697 = insertvalue { ptr, ptr, i64 } %696, ptr %695, 1
  %698 = insertvalue { ptr, ptr, i64 } %697, i64 0, 2
  %699 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %700 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %701 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %702 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %703 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %704 = extractvalue { ptr, ptr, i64 } %698, 0
  %705 = extractvalue { ptr, ptr, i64 } %698, 1
  %706 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %704, 0
  %707 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %706, ptr %705, 1
  %708 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %707, i64 12, 2
  %709 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %708, i64 16, 3, 0
  %710 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %709, i64 16, 4, 0
  %711 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %710, i64 4, 3, 1
  %712 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %711, i64 1, 4, 1
  %713 = call ptr @llvm.stacksave.p0()
  %714 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %712, ptr %714, align 8
  %715 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %714, 1
  %716 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %565, ptr %716, align 8
  %717 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %716, 1
  %718 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %715, ptr %718, align 8
  %719 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %717, ptr %719, align 8
  call void @memrefCopy(i64 4, ptr %718, ptr %719)
  call void @llvm.stackrestore.p0(ptr %713)
  %720 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %537, ptr %575, ptr %499, i32 2, i32 0, i32 117)
  %721 = call ptr @llvm.stacksave.p0()
  %722 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %595, ptr %722, align 8
  %723 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %722, 1
  %724 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %604, ptr %724, align 8
  %725 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %724, 1
  %726 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %723, ptr %726, align 8
  %727 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %725, ptr %727, align 8
  call void @memrefCopy(i64 4, ptr %726, ptr %727)
  call void @llvm.stackrestore.p0(ptr %721)
  %728 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %729 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %730 = insertvalue { ptr, ptr, i64 } poison, ptr %728, 0
  %731 = insertvalue { ptr, ptr, i64 } %730, ptr %729, 1
  %732 = insertvalue { ptr, ptr, i64 } %731, i64 0, 2
  %733 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %734 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %735 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %736 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %737 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %738 = extractvalue { ptr, ptr, i64 } %732, 0
  %739 = extractvalue { ptr, ptr, i64 } %732, 1
  %740 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %738, 0
  %741 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %740, ptr %739, 1
  %742 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %741, i64 268, 2
  %743 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %742, i64 15, 3, 0
  %744 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %743, i64 16, 4, 0
  %745 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %744, i64 4, 3, 1
  %746 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %745, i64 1, 4, 1
  %747 = call ptr @llvm.stacksave.p0()
  %748 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %746, ptr %748, align 8
  %749 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %748, 1
  %750 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %642, ptr %750, align 8
  %751 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %750, 1
  %752 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %749, ptr %752, align 8
  %753 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %751, ptr %753, align 8
  call void @memrefCopy(i64 4, ptr %752, ptr %753)
  call void @llvm.stackrestore.p0(ptr %747)
  %754 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 15, i32 4, ptr %614, ptr %652, ptr %499, i32 2, i32 42, i32 116)
  %755 = call ptr @llvm.stacksave.p0()
  %756 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %489, ptr %756, align 8
  %757 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %756, 1
  %758 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %679, ptr %758, align 8
  %759 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %758, 1
  %760 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %757, ptr %760, align 8
  %761 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %759, ptr %761, align 8
  call void @memrefCopy(i64 4, ptr %760, ptr %761)
  call void @llvm.stackrestore.p0(ptr %755)
  %762 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %763 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %764 = insertvalue { ptr, ptr, i64 } poison, ptr %762, 0
  %765 = insertvalue { ptr, ptr, i64 } %764, ptr %763, 1
  %766 = insertvalue { ptr, ptr, i64 } %765, i64 0, 2
  %767 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %768 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %769 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %770 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %771 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %772 = extractvalue { ptr, ptr, i64 } %766, 0
  %773 = extractvalue { ptr, ptr, i64 } %766, 1
  %774 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %772, 0
  %775 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %774, ptr %773, 1
  %776 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %775, i64 200, 2
  %777 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %776, i64 4, 3, 0
  %778 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %777, i64 16, 4, 0
  %779 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %778, i64 4, 3, 1
  %780 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %779, i64 1, 4, 1
  %781 = call ptr @llvm.stacksave.p0()
  %782 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %780, ptr %782, align 8
  %783 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %782, 1
  %784 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %489, ptr %784, align 8
  %785 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %784, 1
  %786 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %783, ptr %786, align 8
  %787 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %785, ptr %787, align 8
  call void @memrefCopy(i64 4, ptr %786, ptr %787)
  call void @llvm.stackrestore.p0(ptr %781)
  %788 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %789 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %790 = insertvalue { ptr, ptr, i64 } poison, ptr %788, 0
  %791 = insertvalue { ptr, ptr, i64 } %790, ptr %789, 1
  %792 = insertvalue { ptr, ptr, i64 } %791, i64 0, 2
  %793 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %794 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %795 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %796 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %797 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %798 = extractvalue { ptr, ptr, i64 } %792, 0
  %799 = extractvalue { ptr, ptr, i64 } %792, 1
  %800 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %798, 0
  %801 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %800, ptr %799, 1
  %802 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %801, i64 372, 2
  %803 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %802, i64 4, 3, 0
  %804 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %803, i64 31, 4, 0
  %805 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %804, i64 16, 3, 1
  %806 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %805, i64 1, 4, 1
  %807 = call ptr @llvm.stacksave.p0()
  %808 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %806, ptr %808, align 8
  %809 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %808, 1
  %810 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %527, ptr %810, align 8
  %811 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %810, 1
  %812 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %809, ptr %812, align 8
  %813 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %811, ptr %813, align 8
  call void @memrefCopy(i64 4, ptr %812, ptr %813)
  call void @llvm.stackrestore.p0(ptr %807)
  %814 = call ptr @llvm.stacksave.p0()
  %815 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %556, ptr %815, align 8
  %816 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %815, 1
  %817 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %565, ptr %817, align 8
  %818 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %817, 1
  %819 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %816, ptr %819, align 8
  %820 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %818, ptr %820, align 8
  call void @memrefCopy(i64 4, ptr %819, ptr %820)
  call void @llvm.stackrestore.p0(ptr %814)
  %821 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %537, ptr %575, ptr %499, i32 3, i32 0, i32 117)
  %822 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %823 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %824 = insertvalue { ptr, ptr, i64 } poison, ptr %822, 0
  %825 = insertvalue { ptr, ptr, i64 } %824, ptr %823, 1
  %826 = insertvalue { ptr, ptr, i64 } %825, i64 0, 2
  %827 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %828 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %829 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %830 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %831 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %832 = extractvalue { ptr, ptr, i64 } %826, 0
  %833 = extractvalue { ptr, ptr, i64 } %826, 1
  %834 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %832, 0
  %835 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %834, ptr %833, 1
  %836 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %835, i64 388, 2
  %837 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %836, i64 4, 3, 0
  %838 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %837, i64 31, 4, 0
  %839 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %838, i64 15, 3, 1
  %840 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %839, i64 1, 4, 1
  %841 = call ptr @llvm.stacksave.p0()
  %842 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %840, ptr %842, align 8
  %843 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %842, 1
  %844 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %604, ptr %844, align 8
  %845 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %844, 1
  %846 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %843, ptr %846, align 8
  %847 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %845, ptr %847, align 8
  call void @memrefCopy(i64 4, ptr %846, ptr %847)
  call void @llvm.stackrestore.p0(ptr %841)
  %848 = call ptr @llvm.stacksave.p0()
  %849 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %633, ptr %849, align 8
  %850 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %849, 1
  %851 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %642, ptr %851, align 8
  %852 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %851, 1
  %853 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %850, ptr %853, align 8
  %854 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %852, ptr %854, align 8
  call void @memrefCopy(i64 4, ptr %853, ptr %854)
  call void @llvm.stackrestore.p0(ptr %848)
  %855 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 15, i32 4, ptr %614, ptr %652, ptr %499, i32 3, i32 42, i32 116)
  %856 = call ptr @llvm.stacksave.p0()
  %857 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %489, ptr %857, align 8
  %858 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %857, 1
  %859 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %780, ptr %859, align 8
  %860 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %859, 1
  %861 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %858, ptr %861, align 8
  %862 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %860, ptr %862, align 8
  call void @memrefCopy(i64 4, ptr %861, ptr %862)
  call void @llvm.stackrestore.p0(ptr %856)
  %863 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %864 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %865 = insertvalue { ptr, ptr, i64 } poison, ptr %863, 0
  %866 = insertvalue { ptr, ptr, i64 } %865, ptr %864, 1
  %867 = insertvalue { ptr, ptr, i64 } %866, i64 0, 2
  %868 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %869 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %870 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %871 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %872 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %873 = extractvalue { ptr, ptr, i64 } %867, 0
  %874 = extractvalue { ptr, ptr, i64 } %867, 1
  %875 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %873, 0
  %876 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %875, ptr %874, 1
  %877 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %876, i64 204, 2
  %878 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %877, i64 4, 3, 0
  %879 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %878, i64 16, 4, 0
  %880 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %879, i64 4, 3, 1
  %881 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %880, i64 1, 4, 1
  %882 = call ptr @llvm.stacksave.p0()
  %883 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %881, ptr %883, align 8
  %884 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %883, 1
  %885 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %489, ptr %885, align 8
  %886 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %885, 1
  %887 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %884, ptr %887, align 8
  %888 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %886, ptr %888, align 8
  call void @memrefCopy(i64 4, ptr %887, ptr %888)
  call void @llvm.stackrestore.p0(ptr %882)
  %889 = call ptr @llvm.stacksave.p0()
  %890 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %806, ptr %890, align 8
  %891 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %890, 1
  %892 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %527, ptr %892, align 8
  %893 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %892, 1
  %894 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %891, ptr %894, align 8
  %895 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %893, ptr %895, align 8
  call void @memrefCopy(i64 4, ptr %894, ptr %895)
  call void @llvm.stackrestore.p0(ptr %889)
  %896 = call ptr @llvm.stacksave.p0()
  %897 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %712, ptr %897, align 8
  %898 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %897, 1
  %899 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %565, ptr %899, align 8
  %900 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %899, 1
  %901 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %898, ptr %901, align 8
  %902 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %900, ptr %902, align 8
  call void @memrefCopy(i64 4, ptr %901, ptr %902)
  call void @llvm.stackrestore.p0(ptr %896)
  %903 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %537, ptr %575, ptr %499, i32 1, i32 120, i32 117)
  %904 = call ptr @llvm.stacksave.p0()
  %905 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %840, ptr %905, align 8
  %906 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %905, 1
  %907 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %604, ptr %907, align 8
  %908 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %907, 1
  %909 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %906, ptr %909, align 8
  %910 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %908, ptr %910, align 8
  call void @memrefCopy(i64 4, ptr %909, ptr %910)
  call void @llvm.stackrestore.p0(ptr %904)
  %911 = call ptr @llvm.stacksave.p0()
  %912 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %746, ptr %912, align 8
  %913 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %912, 1
  %914 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %642, ptr %914, align 8
  %915 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %914, 1
  %916 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %913, ptr %916, align 8
  %917 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %915, ptr %917, align 8
  call void @memrefCopy(i64 4, ptr %916, ptr %917)
  call void @llvm.stackrestore.p0(ptr %911)
  %918 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 15, i32 4, ptr %614, ptr %652, ptr %499, i32 1, i32 189, i32 116)
  %919 = call ptr @llvm.stacksave.p0()
  %920 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %489, ptr %920, align 8
  %921 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %920, 1
  %922 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %881, ptr %922, align 8
  %923 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %922, 1
  %924 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %921, ptr %924, align 8
  %925 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %923, ptr %925, align 8
  call void @memrefCopy(i64 4, ptr %924, ptr %925)
  call void @llvm.stackrestore.p0(ptr %919)
  %926 = call ptr @malloc(i64 1024)
  %927 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %926, 0
  %928 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %927, ptr %926, 1
  %929 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %928, i64 0, 2
  %930 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %929, i64 16, 3, 0
  %931 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %930, i64 16, 3, 1
  %932 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %931, i64 16, 4, 0
  %933 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %932, i64 1, 4, 1
  %934 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %935 = mul i64 1, %934
  %936 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %937 = mul i64 %935, %936
  %938 = mul i64 %937, 4
  %939 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %940 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %941 = getelementptr float, ptr %939, i64 %940
  %942 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %933, 1
  %943 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %933, 2
  %944 = getelementptr float, ptr %942, i64 %943
  call void @llvm.memcpy.p0.p0.i64(ptr %944, ptr %941, i64 %938, i1 false)
  ret { ptr, ptr, i64, [2 x i64], [2 x i64] } %933
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
