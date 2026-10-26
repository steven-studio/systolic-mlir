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
  %115 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %114, i64 1, 4, 0
  %116 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %115, i64 1, 3, 1
  %117 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %116, i64 1, 4, 1
  %118 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 0
  %119 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %120 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %118, 0
  %121 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %120, ptr %119, 1
  %122 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %121, i64 0, 2
  %123 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %122, i64 8, 3, 0
  %124 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %123, i64 1, 4, 0
  %125 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %124, i64 1, 3, 1
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
  %156 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %155, i64 1, 3, 0
  %157 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %156, i64 12, 4, 0
  %158 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %157, i64 8, 3, 1
  %159 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %158, i64 1, 4, 1
  %160 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 0
  %161 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %162 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %160, 0
  %163 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %162, ptr %161, 1
  %164 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %163, i64 0, 2
  %165 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, i64 1, 3, 0
  %166 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %165, i64 8, 4, 0
  %167 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %166, i64 8, 3, 1
  %168 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %167, i64 1, 4, 1
  %169 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %159, 3, 0
  %170 = mul i64 1, %169
  %171 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %159, 3, 1
  %172 = mul i64 %170, %171
  %173 = mul i64 %172, 4
  %174 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %159, 1
  %175 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %159, 2
  %176 = getelementptr float, ptr %174, i64 %175
  %177 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %168, 1
  %178 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %168, 2
  %179 = getelementptr float, ptr %177, i64 %178
  call void @llvm.memcpy.p0.p0.i64(ptr %179, ptr %176, i64 %173, i1 false)
  %180 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %181 = ptrtoint ptr %180 to i64
  %182 = inttoptr i64 %181 to ptr
  %183 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 1, i32 8, ptr %140, ptr %182, ptr %98, i32 0, i32 0, i32 110)
  %184 = call ptr @llvm.stacksave.p0()
  %185 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %185, align 8
  %186 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %185, 1
  %187 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %79, ptr %187, align 8
  %188 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %187, 1
  %189 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %186, ptr %189, align 8
  %190 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %188, ptr %190, align 8
  call void @memrefCopy(i64 4, ptr %189, ptr %190)
  call void @llvm.stackrestore.p0(ptr %184)
  %191 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %192 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %193 = insertvalue { ptr, ptr, i64 } poison, ptr %191, 0
  %194 = insertvalue { ptr, ptr, i64 } %193, ptr %192, 1
  %195 = insertvalue { ptr, ptr, i64 } %194, i64 0, 2
  %196 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %197 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %198 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %199 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %200 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %201 = extractvalue { ptr, ptr, i64 } %195, 0
  %202 = extractvalue { ptr, ptr, i64 } %195, 1
  %203 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %201, 0
  %204 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %203, ptr %202, 1
  %205 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %204, i64 8, 2
  %206 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %205, i64 4, 3, 0
  %207 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %206, i64 12, 4, 0
  %208 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %207, i64 4, 3, 1
  %209 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %208, i64 1, 4, 1
  %210 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 0
  %211 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 1
  %212 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %210, 0
  %213 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %212, ptr %211, 1
  %214 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %213, i64 0, 2
  %215 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %214, i64 4, 3, 0
  %216 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %215, i64 4, 4, 0
  %217 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %216, i64 4, 3, 1
  %218 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %217, i64 1, 4, 1
  %219 = call ptr @llvm.stacksave.p0()
  %220 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %209, ptr %220, align 8
  %221 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %220, 1
  %222 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %218, ptr %222, align 8
  %223 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %222, 1
  %224 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %221, ptr %224, align 8
  %225 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %223, ptr %225, align 8
  call void @memrefCopy(i64 4, ptr %224, ptr %225)
  call void @llvm.stackrestore.p0(ptr %219)
  %226 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 1
  %227 = ptrtoint ptr %226 to i64
  %228 = inttoptr i64 %227 to ptr
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
  %243 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %242, i64 0, 2
  %244 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %243, i64 4, 3, 0
  %245 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %244, i64 1, 4, 0
  %246 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %245, i64 1, 3, 1
  %247 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %246, i64 1, 4, 1
  %248 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 0
  %249 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %250 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %248, 0
  %251 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %250, ptr %249, 1
  %252 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %251, i64 0, 2
  %253 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %252, i64 4, 3, 0
  %254 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %253, i64 1, 4, 0
  %255 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %254, i64 1, 3, 1
  %256 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %255, i64 1, 4, 1
  %257 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %247, 3, 0
  %258 = mul i64 1, %257
  %259 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %247, 3, 1
  %260 = mul i64 %258, %259
  %261 = mul i64 %260, 4
  %262 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %247, 1
  %263 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %247, 2
  %264 = getelementptr float, ptr %262, i64 %263
  %265 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %256, 1
  %266 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %256, 2
  %267 = getelementptr float, ptr %265, i64 %266
  call void @llvm.memcpy.p0.p0.i64(ptr %267, ptr %264, i64 %261, i1 false)
  %268 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %269 = ptrtoint ptr %268 to i64
  %270 = inttoptr i64 %269 to ptr
  %271 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %272 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %273 = insertvalue { ptr, ptr, i64 } poison, ptr %271, 0
  %274 = insertvalue { ptr, ptr, i64 } %273, ptr %272, 1
  %275 = insertvalue { ptr, ptr, i64 } %274, i64 0, 2
  %276 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %277 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %278 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %279 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %280 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %281 = extractvalue { ptr, ptr, i64 } %275, 0
  %282 = extractvalue { ptr, ptr, i64 } %275, 1
  %283 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %281, 0
  %284 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %283, ptr %282, 1
  %285 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %284, i64 8, 2
  %286 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %285, i64 1, 3, 0
  %287 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %286, i64 12, 4, 0
  %288 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %287, i64 4, 3, 1
  %289 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %288, i64 1, 4, 1
  %290 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 0
  %291 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %292 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %290, 0
  %293 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %292, ptr %291, 1
  %294 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %293, i64 0, 2
  %295 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %294, i64 1, 3, 0
  %296 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %295, i64 4, 4, 0
  %297 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %296, i64 4, 3, 1
  %298 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %297, i64 1, 4, 1
  %299 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %289, 3, 0
  %300 = mul i64 1, %299
  %301 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %289, 3, 1
  %302 = mul i64 %300, %301
  %303 = mul i64 %302, 4
  %304 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %289, 1
  %305 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %289, 2
  %306 = getelementptr float, ptr %304, i64 %305
  %307 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %298, 1
  %308 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %298, 2
  %309 = getelementptr float, ptr %307, i64 %308
  call void @llvm.memcpy.p0.p0.i64(ptr %309, ptr %306, i64 %303, i1 false)
  %310 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %311 = ptrtoint ptr %310 to i64
  %312 = inttoptr i64 %311 to ptr
  %313 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 1, i32 4, ptr %270, ptr %312, ptr %228, i32 1, i32 0, i32 102)
  %314 = call ptr @llvm.stacksave.p0()
  %315 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %218, ptr %315, align 8
  %316 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %315, 1
  %317 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %209, ptr %317, align 8
  %318 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %317, 1
  %319 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %316, ptr %319, align 8
  %320 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %318, ptr %320, align 8
  call void @memrefCopy(i64 4, ptr %319, ptr %320)
  call void @llvm.stackrestore.p0(ptr %314)
  %321 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %322 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %323 = insertvalue { ptr, ptr, i64 } poison, ptr %321, 0
  %324 = insertvalue { ptr, ptr, i64 } %323, ptr %322, 1
  %325 = insertvalue { ptr, ptr, i64 } %324, i64 0, 2
  %326 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %327 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %328 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %329 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %330 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %331 = extractvalue { ptr, ptr, i64 } %325, 0
  %332 = extractvalue { ptr, ptr, i64 } %325, 1
  %333 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %331, 0
  %334 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %333, ptr %332, 1
  %335 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %334, i64 56, 2
  %336 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %335, i64 4, 3, 0
  %337 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %336, i64 12, 4, 0
  %338 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %337, i64 4, 3, 1
  %339 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %338, i64 1, 4, 1
  %340 = call ptr @llvm.stacksave.p0()
  %341 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %339, ptr %341, align 8
  %342 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %341, 1
  %343 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %218, ptr %343, align 8
  %344 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %343, 1
  %345 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %342, ptr %345, align 8
  %346 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %344, ptr %346, align 8
  call void @memrefCopy(i64 4, ptr %345, ptr %346)
  call void @llvm.stackrestore.p0(ptr %340)
  %347 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %348 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %349 = insertvalue { ptr, ptr, i64 } poison, ptr %347, 0
  %350 = insertvalue { ptr, ptr, i64 } %349, ptr %348, 1
  %351 = insertvalue { ptr, ptr, i64 } %350, i64 0, 2
  %352 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %353 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %354 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %355 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %356 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %357 = extractvalue { ptr, ptr, i64 } %351, 0
  %358 = extractvalue { ptr, ptr, i64 } %351, 1
  %359 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %357, 0
  %360 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %359, ptr %358, 1
  %361 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %360, i64 4, 2
  %362 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %361, i64 4, 3, 0
  %363 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %362, i64 1, 4, 0
  %364 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %363, i64 1, 3, 1
  %365 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %364, i64 1, 4, 1
  %366 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %365, 3, 0
  %367 = mul i64 1, %366
  %368 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %365, 3, 1
  %369 = mul i64 %367, %368
  %370 = mul i64 %369, 4
  %371 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %365, 1
  %372 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %365, 2
  %373 = getelementptr float, ptr %371, i64 %372
  %374 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %256, 1
  %375 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %256, 2
  %376 = getelementptr float, ptr %374, i64 %375
  call void @llvm.memcpy.p0.p0.i64(ptr %376, ptr %373, i64 %370, i1 false)
  %377 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %289, 3, 0
  %378 = mul i64 1, %377
  %379 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %289, 3, 1
  %380 = mul i64 %378, %379
  %381 = mul i64 %380, 4
  %382 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %289, 1
  %383 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %289, 2
  %384 = getelementptr float, ptr %382, i64 %383
  %385 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %298, 1
  %386 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %298, 2
  %387 = getelementptr float, ptr %385, i64 %386
  call void @llvm.memcpy.p0.p0.i64(ptr %387, ptr %384, i64 %381, i1 false)
  %388 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 1, i32 4, ptr %270, ptr %312, ptr %228, i32 2, i32 0, i32 102)
  %389 = call ptr @llvm.stacksave.p0()
  %390 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %218, ptr %390, align 8
  %391 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %390, 1
  %392 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %339, ptr %392, align 8
  %393 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %392, 1
  %394 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %391, ptr %394, align 8
  %395 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %393, ptr %395, align 8
  call void @memrefCopy(i64 4, ptr %394, ptr %395)
  call void @llvm.stackrestore.p0(ptr %389)
  %396 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %397 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %398 = insertvalue { ptr, ptr, i64 } poison, ptr %396, 0
  %399 = insertvalue { ptr, ptr, i64 } %398, ptr %397, 1
  %400 = insertvalue { ptr, ptr, i64 } %399, i64 0, 2
  %401 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %402 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %403 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %404 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %405 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %406 = extractvalue { ptr, ptr, i64 } %400, 0
  %407 = extractvalue { ptr, ptr, i64 } %400, 1
  %408 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %406, 0
  %409 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %408, ptr %407, 1
  %410 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %409, i64 96, 2
  %411 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %410, i64 4, 3, 0
  %412 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %411, i64 12, 4, 0
  %413 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %412, i64 4, 3, 1
  %414 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %413, i64 1, 4, 1
  %415 = call ptr @llvm.stacksave.p0()
  %416 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %414, ptr %416, align 8
  %417 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %416, 1
  %418 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %218, ptr %418, align 8
  %419 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %418, 1
  %420 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %417, ptr %420, align 8
  %421 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %419, ptr %421, align 8
  call void @memrefCopy(i64 4, ptr %420, ptr %421)
  call void @llvm.stackrestore.p0(ptr %415)
  %422 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %423 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %424 = insertvalue { ptr, ptr, i64 } poison, ptr %422, 0
  %425 = insertvalue { ptr, ptr, i64 } %424, ptr %423, 1
  %426 = insertvalue { ptr, ptr, i64 } %425, i64 0, 2
  %427 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %428 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %429 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %430 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %431 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %432 = extractvalue { ptr, ptr, i64 } %426, 0
  %433 = extractvalue { ptr, ptr, i64 } %426, 1
  %434 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %432, 0
  %435 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %434, ptr %433, 1
  %436 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %435, i64 8, 2
  %437 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %436, i64 4, 3, 0
  %438 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %437, i64 1, 4, 0
  %439 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %438, i64 1, 3, 1
  %440 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %439, i64 1, 4, 1
  %441 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %440, 3, 0
  %442 = mul i64 1, %441
  %443 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %440, 3, 1
  %444 = mul i64 %442, %443
  %445 = mul i64 %444, 4
  %446 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %440, 1
  %447 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %440, 2
  %448 = getelementptr float, ptr %446, i64 %447
  %449 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %256, 1
  %450 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %256, 2
  %451 = getelementptr float, ptr %449, i64 %450
  call void @llvm.memcpy.p0.p0.i64(ptr %451, ptr %448, i64 %445, i1 false)
  %452 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %453 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %454 = insertvalue { ptr, ptr, i64 } poison, ptr %452, 0
  %455 = insertvalue { ptr, ptr, i64 } %454, ptr %453, 1
  %456 = insertvalue { ptr, ptr, i64 } %455, i64 0, 2
  %457 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %458 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %459 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %460 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %461 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %462 = extractvalue { ptr, ptr, i64 } %456, 0
  %463 = extractvalue { ptr, ptr, i64 } %456, 1
  %464 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %462, 0
  %465 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %464, ptr %463, 1
  %466 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %465, i64 0, 2
  %467 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %466, i64 1, 3, 0
  %468 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %467, i64 12, 4, 0
  %469 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %468, i64 4, 3, 1
  %470 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %469, i64 1, 4, 1
  %471 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %470, 3, 0
  %472 = mul i64 1, %471
  %473 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %470, 3, 1
  %474 = mul i64 %472, %473
  %475 = mul i64 %474, 4
  %476 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %470, 1
  %477 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %470, 2
  %478 = getelementptr float, ptr %476, i64 %477
  %479 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %298, 1
  %480 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %298, 2
  %481 = getelementptr float, ptr %479, i64 %480
  call void @llvm.memcpy.p0.p0.i64(ptr %481, ptr %478, i64 %475, i1 false)
  %482 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 1, i32 4, ptr %270, ptr %312, ptr %228, i32 3, i32 0, i32 102)
  %483 = call ptr @llvm.stacksave.p0()
  %484 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %218, ptr %484, align 8
  %485 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %484, 1
  %486 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %414, ptr %486, align 8
  %487 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %486, 1
  %488 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %485, ptr %488, align 8
  %489 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %487, ptr %489, align 8
  call void @memrefCopy(i64 4, ptr %488, ptr %489)
  call void @llvm.stackrestore.p0(ptr %483)
  %490 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %491 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %492 = insertvalue { ptr, ptr, i64 } poison, ptr %490, 0
  %493 = insertvalue { ptr, ptr, i64 } %492, ptr %491, 1
  %494 = insertvalue { ptr, ptr, i64 } %493, i64 0, 2
  %495 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %496 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %497 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %498 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %499 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %500 = extractvalue { ptr, ptr, i64 } %494, 0
  %501 = extractvalue { ptr, ptr, i64 } %494, 1
  %502 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %500, 0
  %503 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %502, ptr %501, 1
  %504 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %503, i64 100, 2
  %505 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %504, i64 4, 3, 0
  %506 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %505, i64 12, 4, 0
  %507 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %506, i64 4, 3, 1
  %508 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %507, i64 1, 4, 1
  %509 = call ptr @llvm.stacksave.p0()
  %510 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %508, ptr %510, align 8
  %511 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %510, 1
  %512 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %218, ptr %512, align 8
  %513 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %512, 1
  %514 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %511, ptr %514, align 8
  %515 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %513, ptr %515, align 8
  call void @memrefCopy(i64 4, ptr %514, ptr %515)
  call void @llvm.stackrestore.p0(ptr %509)
  %516 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %440, 3, 0
  %517 = mul i64 1, %516
  %518 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %440, 3, 1
  %519 = mul i64 %517, %518
  %520 = mul i64 %519, 4
  %521 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %440, 1
  %522 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %440, 2
  %523 = getelementptr float, ptr %521, i64 %522
  %524 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %256, 1
  %525 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %256, 2
  %526 = getelementptr float, ptr %524, i64 %525
  call void @llvm.memcpy.p0.p0.i64(ptr %526, ptr %523, i64 %520, i1 false)
  %527 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %528 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %529 = insertvalue { ptr, ptr, i64 } poison, ptr %527, 0
  %530 = insertvalue { ptr, ptr, i64 } %529, ptr %528, 1
  %531 = insertvalue { ptr, ptr, i64 } %530, i64 0, 2
  %532 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %533 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %534 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %535 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %536 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %537 = extractvalue { ptr, ptr, i64 } %531, 0
  %538 = extractvalue { ptr, ptr, i64 } %531, 1
  %539 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %537, 0
  %540 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %539, ptr %538, 1
  %541 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %540, i64 4, 2
  %542 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %541, i64 1, 3, 0
  %543 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %542, i64 12, 4, 0
  %544 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %543, i64 4, 3, 1
  %545 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %544, i64 1, 4, 1
  %546 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %545, 3, 0
  %547 = mul i64 1, %546
  %548 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %545, 3, 1
  %549 = mul i64 %547, %548
  %550 = mul i64 %549, 4
  %551 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %545, 1
  %552 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %545, 2
  %553 = getelementptr float, ptr %551, i64 %552
  %554 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %298, 1
  %555 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %298, 2
  %556 = getelementptr float, ptr %554, i64 %555
  call void @llvm.memcpy.p0.p0.i64(ptr %556, ptr %553, i64 %550, i1 false)
  %557 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 1, i32 4, ptr %270, ptr %312, ptr %228, i32 1, i32 27, i32 102)
  %558 = call ptr @llvm.stacksave.p0()
  %559 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %218, ptr %559, align 8
  %560 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %559, 1
  %561 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %508, ptr %561, align 8
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
  %579 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %578, i64 104, 2
  %580 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %579, i64 4, 3, 0
  %581 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %580, i64 12, 4, 0
  %582 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %581, i64 4, 3, 1
  %583 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %582, i64 1, 4, 1
  %584 = call ptr @llvm.stacksave.p0()
  %585 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %583, ptr %585, align 8
  %586 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %585, 1
  %587 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %218, ptr %587, align 8
  %588 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %587, 1
  %589 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %586, ptr %589, align 8
  %590 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %588, ptr %590, align 8
  call void @memrefCopy(i64 4, ptr %589, ptr %590)
  call void @llvm.stackrestore.p0(ptr %584)
  %591 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %440, 3, 0
  %592 = mul i64 1, %591
  %593 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %440, 3, 1
  %594 = mul i64 %592, %593
  %595 = mul i64 %594, 4
  %596 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %440, 1
  %597 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %440, 2
  %598 = getelementptr float, ptr %596, i64 %597
  %599 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %256, 1
  %600 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %256, 2
  %601 = getelementptr float, ptr %599, i64 %600
  call void @llvm.memcpy.p0.p0.i64(ptr %601, ptr %598, i64 %595, i1 false)
  %602 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %289, 3, 0
  %603 = mul i64 1, %602
  %604 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %289, 3, 1
  %605 = mul i64 %603, %604
  %606 = mul i64 %605, 4
  %607 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %289, 1
  %608 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %289, 2
  %609 = getelementptr float, ptr %607, i64 %608
  %610 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %298, 1
  %611 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %298, 2
  %612 = getelementptr float, ptr %610, i64 %611
  call void @llvm.memcpy.p0.p0.i64(ptr %612, ptr %609, i64 %606, i1 false)
  %613 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 1, i32 4, ptr %270, ptr %312, ptr %228, i32 2, i32 27, i32 102)
  %614 = call ptr @llvm.stacksave.p0()
  %615 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %218, ptr %615, align 8
  %616 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %615, 1
  %617 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %583, ptr %617, align 8
  %618 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %617, 1
  %619 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %616, ptr %619, align 8
  %620 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %618, ptr %620, align 8
  call void @memrefCopy(i64 4, ptr %619, ptr %620)
  call void @llvm.stackrestore.p0(ptr %614)
  %621 = call ptr @malloc(i64 576)
  %622 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %621, 0
  %623 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %622, ptr %621, 1
  %624 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %623, i64 0, 2
  %625 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %624, i64 12, 3, 0
  %626 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %625, i64 12, 3, 1
  %627 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %626, i64 12, 4, 0
  %628 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %627, i64 1, 4, 1
  %629 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %630 = mul i64 1, %629
  %631 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %632 = mul i64 %630, %631
  %633 = mul i64 %632, 4
  %634 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %635 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %636 = getelementptr float, ptr %634, i64 %635
  %637 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %628, 1
  %638 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %628, 2
  %639 = getelementptr float, ptr %637, i64 %638
  call void @llvm.memcpy.p0.p0.i64(ptr %639, ptr %636, i64 %633, i1 false)
  ret { ptr, ptr, i64, [2 x i64], [2 x i64] } %628
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
