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
  %115 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %114, i64 17, 4, 0
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
  %192 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %191, i64 17, 4, 0
  %193 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %192, i64 1, 3, 1
  %194 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %193, i64 1, 4, 1
  %195 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 0
  %196 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %197 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %195, 0
  %198 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %197, ptr %196, 1
  %199 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %198, i64 0, 2
  %200 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %199, i64 8, 3, 0
  %201 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %200, i64 1, 4, 0
  %202 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %201, i64 1, 3, 1
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
  %228 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %227, i64 192, 2
  %229 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %228, i64 1, 3, 0
  %230 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %229, i64 12, 4, 0
  %231 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %230, i64 8, 3, 1
  %232 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %231, i64 1, 4, 1
  %233 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 0
  %234 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %235 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %233, 0
  %236 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %235, ptr %234, 1
  %237 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %236, i64 0, 2
  %238 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %237, i64 1, 3, 0
  %239 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %238, i64 8, 4, 0
  %240 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %239, i64 8, 3, 1
  %241 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %240, i64 1, 4, 1
  %242 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %232, 3, 0
  %243 = mul i64 1, %242
  %244 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %232, 3, 1
  %245 = mul i64 %243, %244
  %246 = mul i64 %245, 4
  %247 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %232, 1
  %248 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %232, 2
  %249 = getelementptr float, ptr %247, i64 %248
  %250 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %241, 1
  %251 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %241, 2
  %252 = getelementptr float, ptr %250, i64 %251
  call void @llvm.memcpy.p0.p0.i64(ptr %252, ptr %249, i64 %246, i1 false)
  %253 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %254 = ptrtoint ptr %253 to i64
  %255 = inttoptr i64 %254 to ptr
  %256 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 1, i32 8, ptr %213, ptr %255, ptr %98, i32 0, i32 42, i32 110)
  %257 = call ptr @llvm.stacksave.p0()
  %258 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %258, align 8
  %259 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %258, 1
  %260 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %79, ptr %260, align 8
  %261 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %260, 1
  %262 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %259, ptr %262, align 8
  %263 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %261, ptr %263, align 8
  call void @memrefCopy(i64 4, ptr %262, ptr %263)
  call void @llvm.stackrestore.p0(ptr %257)
  %264 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %265 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %266 = insertvalue { ptr, ptr, i64 } poison, ptr %264, 0
  %267 = insertvalue { ptr, ptr, i64 } %266, ptr %265, 1
  %268 = insertvalue { ptr, ptr, i64 } %267, i64 0, 2
  %269 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %270 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %271 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %272 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %273 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %274 = extractvalue { ptr, ptr, i64 } %268, 0
  %275 = extractvalue { ptr, ptr, i64 } %268, 1
  %276 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %274, 0
  %277 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %276, ptr %275, 1
  %278 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %277, i64 8, 2
  %279 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %278, i64 4, 3, 0
  %280 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %279, i64 12, 4, 0
  %281 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %280, i64 4, 3, 1
  %282 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %281, i64 1, 4, 1
  %283 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 0
  %284 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 1
  %285 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %283, 0
  %286 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %285, ptr %284, 1
  %287 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %286, i64 0, 2
  %288 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %287, i64 4, 3, 0
  %289 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %288, i64 4, 4, 0
  %290 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %289, i64 4, 3, 1
  %291 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %290, i64 1, 4, 1
  %292 = call ptr @llvm.stacksave.p0()
  %293 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %282, ptr %293, align 8
  %294 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %293, 1
  %295 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %291, ptr %295, align 8
  %296 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %295, 1
  %297 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %294, ptr %297, align 8
  %298 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %296, ptr %298, align 8
  call void @memrefCopy(i64 4, ptr %297, ptr %298)
  call void @llvm.stackrestore.p0(ptr %292)
  %299 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 1
  %300 = ptrtoint ptr %299 to i64
  %301 = inttoptr i64 %300 to ptr
  %302 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %303 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %304 = insertvalue { ptr, ptr, i64 } poison, ptr %302, 0
  %305 = insertvalue { ptr, ptr, i64 } %304, ptr %303, 1
  %306 = insertvalue { ptr, ptr, i64 } %305, i64 0, 2
  %307 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %308 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %309 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %310 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %311 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %312 = extractvalue { ptr, ptr, i64 } %306, 0
  %313 = extractvalue { ptr, ptr, i64 } %306, 1
  %314 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %312, 0
  %315 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %314, ptr %313, 1
  %316 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %315, i64 0, 2
  %317 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %316, i64 4, 3, 0
  %318 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %317, i64 17, 4, 0
  %319 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %318, i64 16, 3, 1
  %320 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %319, i64 1, 4, 1
  %321 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 0
  %322 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %323 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %321, 0
  %324 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %323, ptr %322, 1
  %325 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %324, i64 0, 2
  %326 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %325, i64 4, 3, 0
  %327 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %326, i64 16, 4, 0
  %328 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %327, i64 16, 3, 1
  %329 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %328, i64 1, 4, 1
  %330 = call ptr @llvm.stacksave.p0()
  %331 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %320, ptr %331, align 8
  %332 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %331, 1
  %333 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %329, ptr %333, align 8
  %334 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %333, 1
  %335 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %332, ptr %335, align 8
  %336 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %334, ptr %336, align 8
  call void @memrefCopy(i64 4, ptr %335, ptr %336)
  call void @llvm.stackrestore.p0(ptr %330)
  %337 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %338 = ptrtoint ptr %337 to i64
  %339 = inttoptr i64 %338 to ptr
  %340 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %341 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %342 = insertvalue { ptr, ptr, i64 } poison, ptr %340, 0
  %343 = insertvalue { ptr, ptr, i64 } %342, ptr %341, 1
  %344 = insertvalue { ptr, ptr, i64 } %343, i64 0, 2
  %345 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %346 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %347 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %348 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %349 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %350 = extractvalue { ptr, ptr, i64 } %344, 0
  %351 = extractvalue { ptr, ptr, i64 } %344, 1
  %352 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %350, 0
  %353 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %352, ptr %351, 1
  %354 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %353, i64 8, 2
  %355 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %354, i64 16, 3, 0
  %356 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %355, i64 12, 4, 0
  %357 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %356, i64 4, 3, 1
  %358 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %357, i64 1, 4, 1
  %359 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 0
  %360 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %361 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %359, 0
  %362 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %361, ptr %360, 1
  %363 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %362, i64 0, 2
  %364 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %363, i64 16, 3, 0
  %365 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %364, i64 4, 4, 0
  %366 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %365, i64 4, 3, 1
  %367 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %366, i64 1, 4, 1
  %368 = call ptr @llvm.stacksave.p0()
  %369 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %358, ptr %369, align 8
  %370 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %369, 1
  %371 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %367, ptr %371, align 8
  %372 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %371, 1
  %373 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %370, ptr %373, align 8
  %374 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %372, ptr %374, align 8
  call void @memrefCopy(i64 4, ptr %373, ptr %374)
  call void @llvm.stackrestore.p0(ptr %368)
  %375 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %376 = ptrtoint ptr %375 to i64
  %377 = inttoptr i64 %376 to ptr
  %378 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %339, ptr %377, ptr %301, i32 1, i32 0, i32 117)
  %379 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %380 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %381 = insertvalue { ptr, ptr, i64 } poison, ptr %379, 0
  %382 = insertvalue { ptr, ptr, i64 } %381, ptr %380, 1
  %383 = insertvalue { ptr, ptr, i64 } %382, i64 0, 2
  %384 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %385 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %386 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %387 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %388 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %389 = extractvalue { ptr, ptr, i64 } %383, 0
  %390 = extractvalue { ptr, ptr, i64 } %383, 1
  %391 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %389, 0
  %392 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %391, ptr %390, 1
  %393 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %392, i64 16, 2
  %394 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %393, i64 4, 3, 0
  %395 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %394, i64 17, 4, 0
  %396 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %395, i64 1, 3, 1
  %397 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %396, i64 1, 4, 1
  %398 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 0
  %399 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %400 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %398, 0
  %401 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %400, ptr %399, 1
  %402 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %401, i64 0, 2
  %403 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %402, i64 4, 3, 0
  %404 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %403, i64 1, 4, 0
  %405 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %404, i64 1, 3, 1
  %406 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %405, i64 1, 4, 1
  %407 = call ptr @llvm.stacksave.p0()
  %408 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %397, ptr %408, align 8
  %409 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %408, 1
  %410 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %406, ptr %410, align 8
  %411 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %410, 1
  %412 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %409, ptr %412, align 8
  %413 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %411, ptr %413, align 8
  call void @memrefCopy(i64 4, ptr %412, ptr %413)
  call void @llvm.stackrestore.p0(ptr %407)
  %414 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %415 = ptrtoint ptr %414 to i64
  %416 = inttoptr i64 %415 to ptr
  %417 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %418 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %419 = insertvalue { ptr, ptr, i64 } poison, ptr %417, 0
  %420 = insertvalue { ptr, ptr, i64 } %419, ptr %418, 1
  %421 = insertvalue { ptr, ptr, i64 } %420, i64 0, 2
  %422 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %423 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %424 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %425 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %426 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %427 = extractvalue { ptr, ptr, i64 } %421, 0
  %428 = extractvalue { ptr, ptr, i64 } %421, 1
  %429 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %427, 0
  %430 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %429, ptr %428, 1
  %431 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %430, i64 200, 2
  %432 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %431, i64 1, 3, 0
  %433 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %432, i64 12, 4, 0
  %434 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %433, i64 4, 3, 1
  %435 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %434, i64 1, 4, 1
  %436 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 0
  %437 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %438 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %436, 0
  %439 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %438, ptr %437, 1
  %440 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %439, i64 0, 2
  %441 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %440, i64 1, 3, 0
  %442 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %441, i64 4, 4, 0
  %443 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %442, i64 4, 3, 1
  %444 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %443, i64 1, 4, 1
  %445 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %435, 3, 0
  %446 = mul i64 1, %445
  %447 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %435, 3, 1
  %448 = mul i64 %446, %447
  %449 = mul i64 %448, 4
  %450 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %435, 1
  %451 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %435, 2
  %452 = getelementptr float, ptr %450, i64 %451
  %453 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %444, 1
  %454 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %444, 2
  %455 = getelementptr float, ptr %453, i64 %454
  call void @llvm.memcpy.p0.p0.i64(ptr %455, ptr %452, i64 %449, i1 false)
  %456 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %457 = ptrtoint ptr %456 to i64
  %458 = inttoptr i64 %457 to ptr
  %459 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 1, i32 4, ptr %416, ptr %458, ptr %301, i32 1, i32 42, i32 102)
  %460 = call ptr @llvm.stacksave.p0()
  %461 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %291, ptr %461, align 8
  %462 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %461, 1
  %463 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %282, ptr %463, align 8
  %464 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %463, 1
  %465 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %462, ptr %465, align 8
  %466 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %464, ptr %466, align 8
  call void @memrefCopy(i64 4, ptr %465, ptr %466)
  call void @llvm.stackrestore.p0(ptr %460)
  %467 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %468 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %469 = insertvalue { ptr, ptr, i64 } poison, ptr %467, 0
  %470 = insertvalue { ptr, ptr, i64 } %469, ptr %468, 1
  %471 = insertvalue { ptr, ptr, i64 } %470, i64 0, 2
  %472 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %473 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %474 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %475 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %476 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %477 = extractvalue { ptr, ptr, i64 } %471, 0
  %478 = extractvalue { ptr, ptr, i64 } %471, 1
  %479 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %477, 0
  %480 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %479, ptr %478, 1
  %481 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %480, i64 56, 2
  %482 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %481, i64 4, 3, 0
  %483 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %482, i64 12, 4, 0
  %484 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %483, i64 4, 3, 1
  %485 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %484, i64 1, 4, 1
  %486 = call ptr @llvm.stacksave.p0()
  %487 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %485, ptr %487, align 8
  %488 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %487, 1
  %489 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %291, ptr %489, align 8
  %490 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %489, 1
  %491 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %488, ptr %491, align 8
  %492 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %490, ptr %492, align 8
  call void @memrefCopy(i64 4, ptr %491, ptr %492)
  call void @llvm.stackrestore.p0(ptr %486)
  %493 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %494 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %495 = insertvalue { ptr, ptr, i64 } poison, ptr %493, 0
  %496 = insertvalue { ptr, ptr, i64 } %495, ptr %494, 1
  %497 = insertvalue { ptr, ptr, i64 } %496, i64 0, 2
  %498 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %499 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %500 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %501 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %502 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %503 = extractvalue { ptr, ptr, i64 } %497, 0
  %504 = extractvalue { ptr, ptr, i64 } %497, 1
  %505 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %503, 0
  %506 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %505, ptr %504, 1
  %507 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %506, i64 68, 2
  %508 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %507, i64 4, 3, 0
  %509 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %508, i64 17, 4, 0
  %510 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %509, i64 16, 3, 1
  %511 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %510, i64 1, 4, 1
  %512 = call ptr @llvm.stacksave.p0()
  %513 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %511, ptr %513, align 8
  %514 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %513, 1
  %515 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %329, ptr %515, align 8
  %516 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %515, 1
  %517 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %514, ptr %517, align 8
  %518 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %516, ptr %518, align 8
  call void @memrefCopy(i64 4, ptr %517, ptr %518)
  call void @llvm.stackrestore.p0(ptr %512)
  %519 = call ptr @llvm.stacksave.p0()
  %520 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %358, ptr %520, align 8
  %521 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %520, 1
  %522 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %367, ptr %522, align 8
  %523 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %522, 1
  %524 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %521, ptr %524, align 8
  %525 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %523, ptr %525, align 8
  call void @memrefCopy(i64 4, ptr %524, ptr %525)
  call void @llvm.stackrestore.p0(ptr %519)
  %526 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %339, ptr %377, ptr %301, i32 2, i32 0, i32 117)
  %527 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %528 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %529 = insertvalue { ptr, ptr, i64 } poison, ptr %527, 0
  %530 = insertvalue { ptr, ptr, i64 } %529, ptr %528, 1
  %531 = insertvalue { ptr, ptr, i64 } %530, i64 0, 2
  %532 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %533 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %534 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %535 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %536 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %537 = extractvalue { ptr, ptr, i64 } %531, 0
  %538 = extractvalue { ptr, ptr, i64 } %531, 1
  %539 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %537, 0
  %540 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %539, ptr %538, 1
  %541 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %540, i64 84, 2
  %542 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %541, i64 4, 3, 0
  %543 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %542, i64 17, 4, 0
  %544 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %543, i64 1, 3, 1
  %545 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %544, i64 1, 4, 1
  %546 = call ptr @llvm.stacksave.p0()
  %547 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %545, ptr %547, align 8
  %548 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %547, 1
  %549 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %406, ptr %549, align 8
  %550 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %549, 1
  %551 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %548, ptr %551, align 8
  %552 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %550, ptr %552, align 8
  call void @memrefCopy(i64 4, ptr %551, ptr %552)
  call void @llvm.stackrestore.p0(ptr %546)
  %553 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %435, 3, 0
  %554 = mul i64 1, %553
  %555 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %435, 3, 1
  %556 = mul i64 %554, %555
  %557 = mul i64 %556, 4
  %558 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %435, 1
  %559 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %435, 2
  %560 = getelementptr float, ptr %558, i64 %559
  %561 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %444, 1
  %562 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %444, 2
  %563 = getelementptr float, ptr %561, i64 %562
  call void @llvm.memcpy.p0.p0.i64(ptr %563, ptr %560, i64 %557, i1 false)
  %564 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 1, i32 4, ptr %416, ptr %458, ptr %301, i32 2, i32 42, i32 102)
  %565 = call ptr @llvm.stacksave.p0()
  %566 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %291, ptr %566, align 8
  %567 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %566, 1
  %568 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %485, ptr %568, align 8
  %569 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %568, 1
  %570 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %567, ptr %570, align 8
  %571 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %569, ptr %571, align 8
  call void @memrefCopy(i64 4, ptr %570, ptr %571)
  call void @llvm.stackrestore.p0(ptr %565)
  %572 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %573 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %574 = insertvalue { ptr, ptr, i64 } poison, ptr %572, 0
  %575 = insertvalue { ptr, ptr, i64 } %574, ptr %573, 1
  %576 = insertvalue { ptr, ptr, i64 } %575, i64 0, 2
  %577 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %578 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %579 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %580 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %581 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %582 = extractvalue { ptr, ptr, i64 } %576, 0
  %583 = extractvalue { ptr, ptr, i64 } %576, 1
  %584 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %582, 0
  %585 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %584, ptr %583, 1
  %586 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %585, i64 96, 2
  %587 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %586, i64 4, 3, 0
  %588 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %587, i64 12, 4, 0
  %589 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %588, i64 4, 3, 1
  %590 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %589, i64 1, 4, 1
  %591 = call ptr @llvm.stacksave.p0()
  %592 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %590, ptr %592, align 8
  %593 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %592, 1
  %594 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %291, ptr %594, align 8
  %595 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %594, 1
  %596 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %593, ptr %596, align 8
  %597 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %595, ptr %597, align 8
  call void @memrefCopy(i64 4, ptr %596, ptr %597)
  call void @llvm.stackrestore.p0(ptr %591)
  %598 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %599 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %600 = insertvalue { ptr, ptr, i64 } poison, ptr %598, 0
  %601 = insertvalue { ptr, ptr, i64 } %600, ptr %599, 1
  %602 = insertvalue { ptr, ptr, i64 } %601, i64 0, 2
  %603 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %604 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %605 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %606 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %607 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %608 = extractvalue { ptr, ptr, i64 } %602, 0
  %609 = extractvalue { ptr, ptr, i64 } %602, 1
  %610 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %608, 0
  %611 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %610, ptr %609, 1
  %612 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %611, i64 136, 2
  %613 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %612, i64 4, 3, 0
  %614 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %613, i64 17, 4, 0
  %615 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %614, i64 16, 3, 1
  %616 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %615, i64 1, 4, 1
  %617 = call ptr @llvm.stacksave.p0()
  %618 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %616, ptr %618, align 8
  %619 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %618, 1
  %620 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %329, ptr %620, align 8
  %621 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %620, 1
  %622 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %619, ptr %622, align 8
  %623 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %621, ptr %623, align 8
  call void @memrefCopy(i64 4, ptr %622, ptr %623)
  call void @llvm.stackrestore.p0(ptr %617)
  %624 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %625 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %626 = insertvalue { ptr, ptr, i64 } poison, ptr %624, 0
  %627 = insertvalue { ptr, ptr, i64 } %626, ptr %625, 1
  %628 = insertvalue { ptr, ptr, i64 } %627, i64 0, 2
  %629 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %630 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %631 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %632 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %633 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %634 = extractvalue { ptr, ptr, i64 } %628, 0
  %635 = extractvalue { ptr, ptr, i64 } %628, 1
  %636 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %634, 0
  %637 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %636, ptr %635, 1
  %638 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %637, i64 0, 2
  %639 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %638, i64 16, 3, 0
  %640 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %639, i64 12, 4, 0
  %641 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %640, i64 4, 3, 1
  %642 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %641, i64 1, 4, 1
  %643 = call ptr @llvm.stacksave.p0()
  %644 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %642, ptr %644, align 8
  %645 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %644, 1
  %646 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %367, ptr %646, align 8
  %647 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %646, 1
  %648 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %645, ptr %648, align 8
  %649 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %647, ptr %649, align 8
  call void @memrefCopy(i64 4, ptr %648, ptr %649)
  call void @llvm.stackrestore.p0(ptr %643)
  %650 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %339, ptr %377, ptr %301, i32 3, i32 0, i32 117)
  %651 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %652 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %653 = insertvalue { ptr, ptr, i64 } poison, ptr %651, 0
  %654 = insertvalue { ptr, ptr, i64 } %653, ptr %652, 1
  %655 = insertvalue { ptr, ptr, i64 } %654, i64 0, 2
  %656 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %657 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %658 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %659 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %660 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %661 = extractvalue { ptr, ptr, i64 } %655, 0
  %662 = extractvalue { ptr, ptr, i64 } %655, 1
  %663 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %661, 0
  %664 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %663, ptr %662, 1
  %665 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %664, i64 152, 2
  %666 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %665, i64 4, 3, 0
  %667 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %666, i64 17, 4, 0
  %668 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %667, i64 1, 3, 1
  %669 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %668, i64 1, 4, 1
  %670 = call ptr @llvm.stacksave.p0()
  %671 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %669, ptr %671, align 8
  %672 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %671, 1
  %673 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %406, ptr %673, align 8
  %674 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %673, 1
  %675 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %672, ptr %675, align 8
  %676 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %674, ptr %676, align 8
  call void @memrefCopy(i64 4, ptr %675, ptr %676)
  call void @llvm.stackrestore.p0(ptr %670)
  %677 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %678 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %679 = insertvalue { ptr, ptr, i64 } poison, ptr %677, 0
  %680 = insertvalue { ptr, ptr, i64 } %679, ptr %678, 1
  %681 = insertvalue { ptr, ptr, i64 } %680, i64 0, 2
  %682 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %683 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %684 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %685 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %686 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %687 = extractvalue { ptr, ptr, i64 } %681, 0
  %688 = extractvalue { ptr, ptr, i64 } %681, 1
  %689 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %687, 0
  %690 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %689, ptr %688, 1
  %691 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %690, i64 192, 2
  %692 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %691, i64 1, 3, 0
  %693 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %692, i64 12, 4, 0
  %694 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %693, i64 4, 3, 1
  %695 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %694, i64 1, 4, 1
  %696 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %695, 3, 0
  %697 = mul i64 1, %696
  %698 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %695, 3, 1
  %699 = mul i64 %697, %698
  %700 = mul i64 %699, 4
  %701 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %695, 1
  %702 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %695, 2
  %703 = getelementptr float, ptr %701, i64 %702
  %704 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %444, 1
  %705 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %444, 2
  %706 = getelementptr float, ptr %704, i64 %705
  call void @llvm.memcpy.p0.p0.i64(ptr %706, ptr %703, i64 %700, i1 false)
  %707 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 1, i32 4, ptr %416, ptr %458, ptr %301, i32 3, i32 42, i32 102)
  %708 = call ptr @llvm.stacksave.p0()
  %709 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %291, ptr %709, align 8
  %710 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %709, 1
  %711 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %590, ptr %711, align 8
  %712 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %711, 1
  %713 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %710, ptr %713, align 8
  %714 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %712, ptr %714, align 8
  call void @memrefCopy(i64 4, ptr %713, ptr %714)
  call void @llvm.stackrestore.p0(ptr %708)
  %715 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %716 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %717 = insertvalue { ptr, ptr, i64 } poison, ptr %715, 0
  %718 = insertvalue { ptr, ptr, i64 } %717, ptr %716, 1
  %719 = insertvalue { ptr, ptr, i64 } %718, i64 0, 2
  %720 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %721 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %722 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %723 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %724 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %725 = extractvalue { ptr, ptr, i64 } %719, 0
  %726 = extractvalue { ptr, ptr, i64 } %719, 1
  %727 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %725, 0
  %728 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %727, ptr %726, 1
  %729 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %728, i64 100, 2
  %730 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %729, i64 4, 3, 0
  %731 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %730, i64 12, 4, 0
  %732 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %731, i64 4, 3, 1
  %733 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %732, i64 1, 4, 1
  %734 = call ptr @llvm.stacksave.p0()
  %735 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %733, ptr %735, align 8
  %736 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %735, 1
  %737 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %291, ptr %737, align 8
  %738 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %737, 1
  %739 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %736, ptr %739, align 8
  %740 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %738, ptr %740, align 8
  call void @memrefCopy(i64 4, ptr %739, ptr %740)
  call void @llvm.stackrestore.p0(ptr %734)
  %741 = call ptr @llvm.stacksave.p0()
  %742 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %616, ptr %742, align 8
  %743 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %742, 1
  %744 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %329, ptr %744, align 8
  %745 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %744, 1
  %746 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %743, ptr %746, align 8
  %747 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %745, ptr %747, align 8
  call void @memrefCopy(i64 4, ptr %746, ptr %747)
  call void @llvm.stackrestore.p0(ptr %741)
  %748 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %749 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %750 = insertvalue { ptr, ptr, i64 } poison, ptr %748, 0
  %751 = insertvalue { ptr, ptr, i64 } %750, ptr %749, 1
  %752 = insertvalue { ptr, ptr, i64 } %751, i64 0, 2
  %753 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %754 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %755 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %756 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %757 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %758 = extractvalue { ptr, ptr, i64 } %752, 0
  %759 = extractvalue { ptr, ptr, i64 } %752, 1
  %760 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %758, 0
  %761 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %760, ptr %759, 1
  %762 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %761, i64 4, 2
  %763 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %762, i64 16, 3, 0
  %764 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %763, i64 12, 4, 0
  %765 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %764, i64 4, 3, 1
  %766 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %765, i64 1, 4, 1
  %767 = call ptr @llvm.stacksave.p0()
  %768 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %766, ptr %768, align 8
  %769 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %768, 1
  %770 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %367, ptr %770, align 8
  %771 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %770, 1
  %772 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %769, ptr %772, align 8
  %773 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %771, ptr %773, align 8
  call void @memrefCopy(i64 4, ptr %772, ptr %773)
  call void @llvm.stackrestore.p0(ptr %767)
  %774 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %339, ptr %377, ptr %301, i32 1, i32 120, i32 117)
  %775 = call ptr @llvm.stacksave.p0()
  %776 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %669, ptr %776, align 8
  %777 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %776, 1
  %778 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %406, ptr %778, align 8
  %779 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %778, 1
  %780 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %777, ptr %780, align 8
  %781 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %779, ptr %781, align 8
  call void @memrefCopy(i64 4, ptr %780, ptr %781)
  call void @llvm.stackrestore.p0(ptr %775)
  %782 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %783 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %784 = insertvalue { ptr, ptr, i64 } poison, ptr %782, 0
  %785 = insertvalue { ptr, ptr, i64 } %784, ptr %783, 1
  %786 = insertvalue { ptr, ptr, i64 } %785, i64 0, 2
  %787 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %788 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %789 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %790 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %791 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %792 = extractvalue { ptr, ptr, i64 } %786, 0
  %793 = extractvalue { ptr, ptr, i64 } %786, 1
  %794 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %792, 0
  %795 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %794, ptr %793, 1
  %796 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %795, i64 196, 2
  %797 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %796, i64 1, 3, 0
  %798 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %797, i64 12, 4, 0
  %799 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %798, i64 4, 3, 1
  %800 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %799, i64 1, 4, 1
  %801 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %800, 3, 0
  %802 = mul i64 1, %801
  %803 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %800, 3, 1
  %804 = mul i64 %802, %803
  %805 = mul i64 %804, 4
  %806 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %800, 1
  %807 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %800, 2
  %808 = getelementptr float, ptr %806, i64 %807
  %809 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %444, 1
  %810 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %444, 2
  %811 = getelementptr float, ptr %809, i64 %810
  call void @llvm.memcpy.p0.p0.i64(ptr %811, ptr %808, i64 %805, i1 false)
  %812 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 1, i32 4, ptr %416, ptr %458, ptr %301, i32 1, i32 189, i32 102)
  %813 = call ptr @llvm.stacksave.p0()
  %814 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %291, ptr %814, align 8
  %815 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %814, 1
  %816 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %733, ptr %816, align 8
  %817 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %816, 1
  %818 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %815, ptr %818, align 8
  %819 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %817, ptr %819, align 8
  call void @memrefCopy(i64 4, ptr %818, ptr %819)
  call void @llvm.stackrestore.p0(ptr %813)
  %820 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %821 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %822 = insertvalue { ptr, ptr, i64 } poison, ptr %820, 0
  %823 = insertvalue { ptr, ptr, i64 } %822, ptr %821, 1
  %824 = insertvalue { ptr, ptr, i64 } %823, i64 0, 2
  %825 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %826 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %827 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %828 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %829 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %830 = extractvalue { ptr, ptr, i64 } %824, 0
  %831 = extractvalue { ptr, ptr, i64 } %824, 1
  %832 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %830, 0
  %833 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %832, ptr %831, 1
  %834 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %833, i64 104, 2
  %835 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %834, i64 4, 3, 0
  %836 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %835, i64 12, 4, 0
  %837 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %836, i64 4, 3, 1
  %838 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %837, i64 1, 4, 1
  %839 = call ptr @llvm.stacksave.p0()
  %840 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %838, ptr %840, align 8
  %841 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %840, 1
  %842 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %291, ptr %842, align 8
  %843 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %842, 1
  %844 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %841, ptr %844, align 8
  %845 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %843, ptr %845, align 8
  call void @memrefCopy(i64 4, ptr %844, ptr %845)
  call void @llvm.stackrestore.p0(ptr %839)
  %846 = call ptr @llvm.stacksave.p0()
  %847 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %616, ptr %847, align 8
  %848 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %847, 1
  %849 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %329, ptr %849, align 8
  %850 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %849, 1
  %851 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %848, ptr %851, align 8
  %852 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %850, ptr %852, align 8
  call void @memrefCopy(i64 4, ptr %851, ptr %852)
  call void @llvm.stackrestore.p0(ptr %846)
  %853 = call ptr @llvm.stacksave.p0()
  %854 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %358, ptr %854, align 8
  %855 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %854, 1
  %856 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %367, ptr %856, align 8
  %857 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %856, 1
  %858 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %855, ptr %858, align 8
  %859 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %857, ptr %859, align 8
  call void @memrefCopy(i64 4, ptr %858, ptr %859)
  call void @llvm.stackrestore.p0(ptr %853)
  %860 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %339, ptr %377, ptr %301, i32 2, i32 120, i32 117)
  %861 = call ptr @llvm.stacksave.p0()
  %862 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %669, ptr %862, align 8
  %863 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %862, 1
  %864 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %406, ptr %864, align 8
  %865 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %864, 1
  %866 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %863, ptr %866, align 8
  %867 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %865, ptr %867, align 8
  call void @memrefCopy(i64 4, ptr %866, ptr %867)
  call void @llvm.stackrestore.p0(ptr %861)
  %868 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %435, 3, 0
  %869 = mul i64 1, %868
  %870 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %435, 3, 1
  %871 = mul i64 %869, %870
  %872 = mul i64 %871, 4
  %873 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %435, 1
  %874 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %435, 2
  %875 = getelementptr float, ptr %873, i64 %874
  %876 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %444, 1
  %877 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %444, 2
  %878 = getelementptr float, ptr %876, i64 %877
  call void @llvm.memcpy.p0.p0.i64(ptr %878, ptr %875, i64 %872, i1 false)
  %879 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 1, i32 4, ptr %416, ptr %458, ptr %301, i32 2, i32 189, i32 102)
  %880 = call ptr @llvm.stacksave.p0()
  %881 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %291, ptr %881, align 8
  %882 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %881, 1
  %883 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %838, ptr %883, align 8
  %884 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %883, 1
  %885 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %882, ptr %885, align 8
  %886 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %884, ptr %886, align 8
  call void @memrefCopy(i64 4, ptr %885, ptr %886)
  call void @llvm.stackrestore.p0(ptr %880)
  %887 = call ptr @malloc(i64 576)
  %888 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %887, 0
  %889 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %888, ptr %887, 1
  %890 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %889, i64 0, 2
  %891 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %890, i64 12, 3, 0
  %892 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %891, i64 12, 3, 1
  %893 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %892, i64 12, 4, 0
  %894 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %893, i64 1, 4, 1
  %895 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %896 = mul i64 1, %895
  %897 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %898 = mul i64 %896, %897
  %899 = mul i64 %898, 4
  %900 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %901 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %902 = getelementptr float, ptr %900, i64 %901
  %903 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %894, 1
  %904 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %894, 2
  %905 = getelementptr float, ptr %903, i64 %904
  call void @llvm.memcpy.p0.p0.i64(ptr %905, ptr %902, i64 %899, i1 false)
  ret { ptr, ptr, i64, [2 x i64], [2 x i64] } %894
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
