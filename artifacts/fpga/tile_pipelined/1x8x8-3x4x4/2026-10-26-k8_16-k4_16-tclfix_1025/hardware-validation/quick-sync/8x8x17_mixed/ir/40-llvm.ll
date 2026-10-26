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
  %61 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 0
  %62 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 1
  %63 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %61, 0
  %64 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %63, ptr %62, 1
  %65 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %64, i64 0, 2
  %66 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %65, i64 8, 3, 0
  %67 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %66, i64 8, 4, 0
  %68 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %67, i64 8, 3, 1
  %69 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %68, i64 1, 4, 1
  %70 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %71 = mul i64 1, %70
  %72 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %73 = mul i64 %71, %72
  %74 = mul i64 %73, 4
  %75 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %76 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %77 = getelementptr float, ptr %75, i64 %76
  %78 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %69, 1
  %79 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %69, 2
  %80 = getelementptr float, ptr %78, i64 %79
  call void @llvm.memcpy.p0.p0.i64(ptr %80, ptr %77, i64 %74, i1 false)
  %81 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 1
  %82 = ptrtoint ptr %81 to i64
  %83 = inttoptr i64 %82 to ptr
  %84 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %85 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %86 = insertvalue { ptr, ptr, i64 } poison, ptr %84, 0
  %87 = insertvalue { ptr, ptr, i64 } %86, ptr %85, 1
  %88 = insertvalue { ptr, ptr, i64 } %87, i64 0, 2
  %89 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %90 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %91 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %92 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %93 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %94 = extractvalue { ptr, ptr, i64 } %88, 0
  %95 = extractvalue { ptr, ptr, i64 } %88, 1
  %96 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %94, 0
  %97 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %96, ptr %95, 1
  %98 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %97, i64 0, 2
  %99 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %98, i64 8, 3, 0
  %100 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %99, i64 17, 4, 0
  %101 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %100, i64 16, 3, 1
  %102 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %101, i64 1, 4, 1
  %103 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 0
  %104 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %105 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %103, 0
  %106 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %105, ptr %104, 1
  %107 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %106, i64 0, 2
  %108 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %107, i64 8, 3, 0
  %109 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %108, i64 16, 4, 0
  %110 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %109, i64 16, 3, 1
  %111 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %110, i64 1, 4, 1
  %112 = call ptr @llvm.stacksave.p0()
  %113 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %102, ptr %113, align 8
  %114 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %113, 1
  %115 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %111, ptr %115, align 8
  %116 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %115, 1
  %117 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %114, ptr %117, align 8
  %118 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %116, ptr %118, align 8
  call void @memrefCopy(i64 4, ptr %117, ptr %118)
  call void @llvm.stackrestore.p0(ptr %112)
  %119 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %120 = ptrtoint ptr %119 to i64
  %121 = inttoptr i64 %120 to ptr
  %122 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %123 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %124 = insertvalue { ptr, ptr, i64 } poison, ptr %122, 0
  %125 = insertvalue { ptr, ptr, i64 } %124, ptr %123, 1
  %126 = insertvalue { ptr, ptr, i64 } %125, i64 0, 2
  %127 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %128 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %129 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %130 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %131 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %132 = extractvalue { ptr, ptr, i64 } %126, 0
  %133 = extractvalue { ptr, ptr, i64 } %126, 1
  %134 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %132, 0
  %135 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %134, ptr %133, 1
  %136 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %135, i64 0, 2
  %137 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %136, i64 16, 3, 0
  %138 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %137, i64 8, 4, 0
  %139 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %138, i64 8, 3, 1
  %140 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %139, i64 1, 4, 1
  %141 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 0
  %142 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %143 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %141, 0
  %144 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %143, ptr %142, 1
  %145 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %144, i64 0, 2
  %146 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %145, i64 16, 3, 0
  %147 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %146, i64 8, 4, 0
  %148 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %147, i64 8, 3, 1
  %149 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %148, i64 1, 4, 1
  %150 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %140, 3, 0
  %151 = mul i64 1, %150
  %152 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %140, 3, 1
  %153 = mul i64 %151, %152
  %154 = mul i64 %153, 4
  %155 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %140, 1
  %156 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %140, 2
  %157 = getelementptr float, ptr %155, i64 %156
  %158 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %149, 1
  %159 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %149, 2
  %160 = getelementptr float, ptr %158, i64 %159
  call void @llvm.memcpy.p0.p0.i64(ptr %160, ptr %157, i64 %154, i1 false)
  %161 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %162 = ptrtoint ptr %161 to i64
  %163 = inttoptr i64 %162 to ptr
  %164 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %121, ptr %163, ptr %83, i32 0, i32 0, i32 125)
  %165 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %166 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %167 = insertvalue { ptr, ptr, i64 } poison, ptr %165, 0
  %168 = insertvalue { ptr, ptr, i64 } %167, ptr %166, 1
  %169 = insertvalue { ptr, ptr, i64 } %168, i64 0, 2
  %170 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %171 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %172 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %173 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %174 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %175 = extractvalue { ptr, ptr, i64 } %169, 0
  %176 = extractvalue { ptr, ptr, i64 } %169, 1
  %177 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %175, 0
  %178 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %177, ptr %176, 1
  %179 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %178, i64 16, 2
  %180 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %179, i64 8, 3, 0
  %181 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %180, i64 17, 4, 0
  %182 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %181, i64 1, 3, 1
  %183 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %182, i64 1, 4, 1
  %184 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 0
  %185 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %186 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %184, 0
  %187 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %186, ptr %185, 1
  %188 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %187, i64 0, 2
  %189 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %188, i64 8, 3, 0
  %190 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %189, i64 1, 4, 0
  %191 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %190, i64 1, 3, 1
  %192 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %191, i64 1, 4, 1
  %193 = call ptr @llvm.stacksave.p0()
  %194 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %183, ptr %194, align 8
  %195 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %194, 1
  %196 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %192, ptr %196, align 8
  %197 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %196, 1
  %198 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %195, ptr %198, align 8
  %199 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %197, ptr %199, align 8
  call void @memrefCopy(i64 4, ptr %198, ptr %199)
  call void @llvm.stackrestore.p0(ptr %193)
  %200 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %201 = ptrtoint ptr %200 to i64
  %202 = inttoptr i64 %201 to ptr
  %203 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %204 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %205 = insertvalue { ptr, ptr, i64 } poison, ptr %203, 0
  %206 = insertvalue { ptr, ptr, i64 } %205, ptr %204, 1
  %207 = insertvalue { ptr, ptr, i64 } %206, i64 0, 2
  %208 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %209 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %210 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %211 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %212 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %213 = extractvalue { ptr, ptr, i64 } %207, 0
  %214 = extractvalue { ptr, ptr, i64 } %207, 1
  %215 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %213, 0
  %216 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %215, ptr %214, 1
  %217 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %216, i64 128, 2
  %218 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %217, i64 1, 3, 0
  %219 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %218, i64 8, 4, 0
  %220 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %219, i64 8, 3, 1
  %221 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %220, i64 1, 4, 1
  %222 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 0
  %223 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %224 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %222, 0
  %225 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %224, ptr %223, 1
  %226 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %225, i64 0, 2
  %227 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %226, i64 1, 3, 0
  %228 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %227, i64 8, 4, 0
  %229 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %228, i64 8, 3, 1
  %230 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %229, i64 1, 4, 1
  %231 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %221, 3, 0
  %232 = mul i64 1, %231
  %233 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %221, 3, 1
  %234 = mul i64 %232, %233
  %235 = mul i64 %234, 4
  %236 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %221, 1
  %237 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %221, 2
  %238 = getelementptr float, ptr %236, i64 %237
  %239 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %230, 1
  %240 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %230, 2
  %241 = getelementptr float, ptr %239, i64 %240
  call void @llvm.memcpy.p0.p0.i64(ptr %241, ptr %238, i64 %235, i1 false)
  %242 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %243 = ptrtoint ptr %242 to i64
  %244 = inttoptr i64 %243 to ptr
  %245 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 1, i32 8, ptr %202, ptr %244, ptr %83, i32 0, i32 42, i32 110)
  %246 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %69, 3, 0
  %247 = mul i64 1, %246
  %248 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %69, 3, 1
  %249 = mul i64 %247, %248
  %250 = mul i64 %249, 4
  %251 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %69, 1
  %252 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %69, 2
  %253 = getelementptr float, ptr %251, i64 %252
  %254 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %255 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %256 = getelementptr float, ptr %254, i64 %255
  call void @llvm.memcpy.p0.p0.i64(ptr %256, ptr %253, i64 %250, i1 false)
  %257 = call ptr @malloc(i64 256)
  %258 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %257, 0
  %259 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %258, ptr %257, 1
  %260 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %259, i64 0, 2
  %261 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %260, i64 8, 3, 0
  %262 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %261, i64 8, 3, 1
  %263 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %262, i64 8, 4, 0
  %264 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %263, i64 1, 4, 1
  %265 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %266 = mul i64 1, %265
  %267 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %268 = mul i64 %266, %267
  %269 = mul i64 %268, 4
  %270 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %271 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %272 = getelementptr float, ptr %270, i64 %271
  %273 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %264, 1
  %274 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %264, 2
  %275 = getelementptr float, ptr %273, i64 %274
  call void @llvm.memcpy.p0.p0.i64(ptr %275, ptr %272, i64 %269, i1 false)
  ret { ptr, ptr, i64, [2 x i64], [2 x i64] } %264
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

; Function Attrs: nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
declare void @llvm.memcpy.p0.p0.i64(ptr noalias writeonly captures(none), ptr noalias readonly captures(none), i64, i1 immarg) #0

; Function Attrs: nocallback nofree nosync nounwind willreturn
declare ptr @llvm.stacksave.p0() #1

; Function Attrs: nocallback nofree nosync nounwind willreturn
declare void @llvm.stackrestore.p0(ptr) #1

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

attributes #0 = { nocallback nofree nosync nounwind willreturn memory(argmem: readwrite) }
attributes #1 = { nocallback nofree nosync nounwind willreturn }

!llvm.module.flags = !{!0}

!0 = !{i32 2, !"Debug Info Version", i32 3}
