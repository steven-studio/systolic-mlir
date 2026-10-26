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
  %77 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %76, i64 8, 4, 0
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
  %89 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %79, 3, 0
  %90 = mul i64 1, %89
  %91 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %79, 3, 1
  %92 = mul i64 %90, %91
  %93 = mul i64 %92, 4
  %94 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %79, 1
  %95 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %79, 2
  %96 = getelementptr float, ptr %94, i64 %95
  %97 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, 1
  %98 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, 2
  %99 = getelementptr float, ptr %97, i64 %98
  call void @llvm.memcpy.p0.p0.i64(ptr %99, ptr %96, i64 %93, i1 false)
  %100 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 1
  %101 = ptrtoint ptr %100 to i64
  %102 = inttoptr i64 %101 to ptr
  %103 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %104 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %105 = insertvalue { ptr, ptr, i64 } poison, ptr %103, 0
  %106 = insertvalue { ptr, ptr, i64 } %105, ptr %104, 1
  %107 = insertvalue { ptr, ptr, i64 } %106, i64 0, 2
  %108 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %109 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %110 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %111 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %112 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %113 = extractvalue { ptr, ptr, i64 } %107, 0
  %114 = extractvalue { ptr, ptr, i64 } %107, 1
  %115 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %113, 0
  %116 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %115, ptr %114, 1
  %117 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %116, i64 0, 2
  %118 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %117, i64 8, 3, 0
  %119 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %118, i64 40, 4, 0
  %120 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %119, i64 16, 3, 1
  %121 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %120, i64 1, 4, 1
  %122 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 0
  %123 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %124 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %122, 0
  %125 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %124, ptr %123, 1
  %126 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %125, i64 0, 2
  %127 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, i64 8, 3, 0
  %128 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %127, i64 16, 4, 0
  %129 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %128, i64 16, 3, 1
  %130 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %129, i64 1, 4, 1
  %131 = call ptr @llvm.stacksave.p0()
  %132 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %121, ptr %132, align 8
  %133 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %132, 1
  %134 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %130, ptr %134, align 8
  %135 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %134, 1
  %136 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %133, ptr %136, align 8
  %137 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %135, ptr %137, align 8
  call void @memrefCopy(i64 4, ptr %136, ptr %137)
  call void @llvm.stackrestore.p0(ptr %131)
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
  %157 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %156, i64 8, 4, 0
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
  %183 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %140, ptr %182, ptr %102, i32 0, i32 0, i32 125)
  %184 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %185 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %186 = insertvalue { ptr, ptr, i64 } poison, ptr %184, 0
  %187 = insertvalue { ptr, ptr, i64 } %186, ptr %185, 1
  %188 = insertvalue { ptr, ptr, i64 } %187, i64 0, 2
  %189 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %190 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %191 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %192 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %193 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %194 = extractvalue { ptr, ptr, i64 } %188, 0
  %195 = extractvalue { ptr, ptr, i64 } %188, 1
  %196 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %194, 0
  %197 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %196, ptr %195, 1
  %198 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %197, i64 16, 2
  %199 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %198, i64 8, 3, 0
  %200 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %199, i64 40, 4, 0
  %201 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %200, i64 16, 3, 1
  %202 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %201, i64 1, 4, 1
  %203 = call ptr @llvm.stacksave.p0()
  %204 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %202, ptr %204, align 8
  %205 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %204, 1
  %206 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %130, ptr %206, align 8
  %207 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %206, 1
  %208 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %205, ptr %208, align 8
  %209 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %207, ptr %209, align 8
  call void @memrefCopy(i64 4, ptr %208, ptr %209)
  call void @llvm.stackrestore.p0(ptr %203)
  %210 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %211 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %212 = insertvalue { ptr, ptr, i64 } poison, ptr %210, 0
  %213 = insertvalue { ptr, ptr, i64 } %212, ptr %211, 1
  %214 = insertvalue { ptr, ptr, i64 } %213, i64 0, 2
  %215 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %216 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %217 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %218 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %219 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %220 = extractvalue { ptr, ptr, i64 } %214, 0
  %221 = extractvalue { ptr, ptr, i64 } %214, 1
  %222 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %220, 0
  %223 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %222, ptr %221, 1
  %224 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %223, i64 128, 2
  %225 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %224, i64 16, 3, 0
  %226 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %225, i64 8, 4, 0
  %227 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %226, i64 8, 3, 1
  %228 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %227, i64 1, 4, 1
  %229 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %228, 3, 0
  %230 = mul i64 1, %229
  %231 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %228, 3, 1
  %232 = mul i64 %230, %231
  %233 = mul i64 %232, 4
  %234 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %228, 1
  %235 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %228, 2
  %236 = getelementptr float, ptr %234, i64 %235
  %237 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %168, 1
  %238 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %168, 2
  %239 = getelementptr float, ptr %237, i64 %238
  call void @llvm.memcpy.p0.p0.i64(ptr %239, ptr %236, i64 %233, i1 false)
  %240 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %140, ptr %182, ptr %102, i32 0, i32 42, i32 125)
  %241 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %242 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %243 = insertvalue { ptr, ptr, i64 } poison, ptr %241, 0
  %244 = insertvalue { ptr, ptr, i64 } %243, ptr %242, 1
  %245 = insertvalue { ptr, ptr, i64 } %244, i64 0, 2
  %246 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %247 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %248 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %249 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %250 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %251 = extractvalue { ptr, ptr, i64 } %245, 0
  %252 = extractvalue { ptr, ptr, i64 } %245, 1
  %253 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %251, 0
  %254 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %253, ptr %252, 1
  %255 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %254, i64 32, 2
  %256 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %255, i64 8, 3, 0
  %257 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %256, i64 40, 4, 0
  %258 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %257, i64 8, 3, 1
  %259 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %258, i64 1, 4, 1
  %260 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 0
  %261 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %262 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %260, 0
  %263 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %262, ptr %261, 1
  %264 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %263, i64 0, 2
  %265 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %264, i64 8, 3, 0
  %266 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %265, i64 8, 4, 0
  %267 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %266, i64 8, 3, 1
  %268 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %267, i64 1, 4, 1
  %269 = call ptr @llvm.stacksave.p0()
  %270 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %259, ptr %270, align 8
  %271 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %270, 1
  %272 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %268, ptr %272, align 8
  %273 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %272, 1
  %274 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %271, ptr %274, align 8
  %275 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %273, ptr %275, align 8
  call void @memrefCopy(i64 4, ptr %274, ptr %275)
  call void @llvm.stackrestore.p0(ptr %269)
  %276 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %277 = ptrtoint ptr %276 to i64
  %278 = inttoptr i64 %277 to ptr
  %279 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %280 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %281 = insertvalue { ptr, ptr, i64 } poison, ptr %279, 0
  %282 = insertvalue { ptr, ptr, i64 } %281, ptr %280, 1
  %283 = insertvalue { ptr, ptr, i64 } %282, i64 0, 2
  %284 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %285 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %286 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %287 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %288 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %289 = extractvalue { ptr, ptr, i64 } %283, 0
  %290 = extractvalue { ptr, ptr, i64 } %283, 1
  %291 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %289, 0
  %292 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %291, ptr %290, 1
  %293 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %292, i64 256, 2
  %294 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %293, i64 8, 3, 0
  %295 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %294, i64 8, 4, 0
  %296 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %295, i64 8, 3, 1
  %297 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %296, i64 1, 4, 1
  %298 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 0
  %299 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %300 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %298, 0
  %301 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %300, ptr %299, 1
  %302 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %301, i64 0, 2
  %303 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %302, i64 8, 3, 0
  %304 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %303, i64 8, 4, 0
  %305 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %304, i64 8, 3, 1
  %306 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %305, i64 1, 4, 1
  %307 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %297, 3, 0
  %308 = mul i64 1, %307
  %309 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %297, 3, 1
  %310 = mul i64 %308, %309
  %311 = mul i64 %310, 4
  %312 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %297, 1
  %313 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %297, 2
  %314 = getelementptr float, ptr %312, i64 %313
  %315 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %306, 1
  %316 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %306, 2
  %317 = getelementptr float, ptr %315, i64 %316
  call void @llvm.memcpy.p0.p0.i64(ptr %317, ptr %314, i64 %311, i1 false)
  %318 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %319 = ptrtoint ptr %318 to i64
  %320 = inttoptr i64 %319 to ptr
  %321 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 8, i32 8, ptr %278, ptr %320, ptr %102, i32 0, i32 128, i32 117)
  %322 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, 3, 0
  %323 = mul i64 1, %322
  %324 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, 3, 1
  %325 = mul i64 %323, %324
  %326 = mul i64 %325, 4
  %327 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, 1
  %328 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, 2
  %329 = getelementptr float, ptr %327, i64 %328
  %330 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %79, 1
  %331 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %79, 2
  %332 = getelementptr float, ptr %330, i64 %331
  call void @llvm.memcpy.p0.p0.i64(ptr %332, ptr %329, i64 %326, i1 false)
  %333 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %334 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %335 = insertvalue { ptr, ptr, i64 } poison, ptr %333, 0
  %336 = insertvalue { ptr, ptr, i64 } %335, ptr %334, 1
  %337 = insertvalue { ptr, ptr, i64 } %336, i64 0, 2
  %338 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %339 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %340 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %341 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %342 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %343 = extractvalue { ptr, ptr, i64 } %337, 0
  %344 = extractvalue { ptr, ptr, i64 } %337, 1
  %345 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %343, 0
  %346 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %345, ptr %344, 1
  %347 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %346, i64 64, 2
  %348 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %347, i64 8, 3, 0
  %349 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %348, i64 8, 4, 0
  %350 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %349, i64 8, 3, 1
  %351 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %350, i64 1, 4, 1
  %352 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %351, 3, 0
  %353 = mul i64 1, %352
  %354 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %351, 3, 1
  %355 = mul i64 %353, %354
  %356 = mul i64 %355, 4
  %357 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %351, 1
  %358 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %351, 2
  %359 = getelementptr float, ptr %357, i64 %358
  %360 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, 1
  %361 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, 2
  %362 = getelementptr float, ptr %360, i64 %361
  call void @llvm.memcpy.p0.p0.i64(ptr %362, ptr %359, i64 %356, i1 false)
  %363 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %364 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %365 = insertvalue { ptr, ptr, i64 } poison, ptr %363, 0
  %366 = insertvalue { ptr, ptr, i64 } %365, ptr %364, 1
  %367 = insertvalue { ptr, ptr, i64 } %366, i64 0, 2
  %368 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %369 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %370 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %371 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %372 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %373 = extractvalue { ptr, ptr, i64 } %367, 0
  %374 = extractvalue { ptr, ptr, i64 } %367, 1
  %375 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %373, 0
  %376 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %375, ptr %374, 1
  %377 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %376, i64 320, 2
  %378 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %377, i64 8, 3, 0
  %379 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %378, i64 40, 4, 0
  %380 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %379, i64 16, 3, 1
  %381 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %380, i64 1, 4, 1
  %382 = call ptr @llvm.stacksave.p0()
  %383 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %381, ptr %383, align 8
  %384 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %383, 1
  %385 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %130, ptr %385, align 8
  %386 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %385, 1
  %387 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %384, ptr %387, align 8
  %388 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %386, ptr %388, align 8
  call void @memrefCopy(i64 4, ptr %387, ptr %388)
  call void @llvm.stackrestore.p0(ptr %382)
  %389 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %159, 3, 0
  %390 = mul i64 1, %389
  %391 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %159, 3, 1
  %392 = mul i64 %390, %391
  %393 = mul i64 %392, 4
  %394 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %159, 1
  %395 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %159, 2
  %396 = getelementptr float, ptr %394, i64 %395
  %397 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %168, 1
  %398 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %168, 2
  %399 = getelementptr float, ptr %397, i64 %398
  call void @llvm.memcpy.p0.p0.i64(ptr %399, ptr %396, i64 %393, i1 false)
  %400 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %140, ptr %182, ptr %102, i32 0, i32 197, i32 125)
  %401 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %402 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %403 = insertvalue { ptr, ptr, i64 } poison, ptr %401, 0
  %404 = insertvalue { ptr, ptr, i64 } %403, ptr %402, 1
  %405 = insertvalue { ptr, ptr, i64 } %404, i64 0, 2
  %406 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %407 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %408 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %409 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %410 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %411 = extractvalue { ptr, ptr, i64 } %405, 0
  %412 = extractvalue { ptr, ptr, i64 } %405, 1
  %413 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %411, 0
  %414 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %413, ptr %412, 1
  %415 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %414, i64 336, 2
  %416 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %415, i64 8, 3, 0
  %417 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %416, i64 40, 4, 0
  %418 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %417, i64 16, 3, 1
  %419 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %418, i64 1, 4, 1
  %420 = call ptr @llvm.stacksave.p0()
  %421 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %419, ptr %421, align 8
  %422 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %421, 1
  %423 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %130, ptr %423, align 8
  %424 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %423, 1
  %425 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %422, ptr %425, align 8
  %426 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %424, ptr %426, align 8
  call void @memrefCopy(i64 4, ptr %425, ptr %426)
  call void @llvm.stackrestore.p0(ptr %420)
  %427 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %228, 3, 0
  %428 = mul i64 1, %427
  %429 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %228, 3, 1
  %430 = mul i64 %428, %429
  %431 = mul i64 %430, 4
  %432 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %228, 1
  %433 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %228, 2
  %434 = getelementptr float, ptr %432, i64 %433
  %435 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %168, 1
  %436 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %168, 2
  %437 = getelementptr float, ptr %435, i64 %436
  call void @llvm.memcpy.p0.p0.i64(ptr %437, ptr %434, i64 %431, i1 false)
  %438 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %140, ptr %182, ptr %102, i32 0, i32 266, i32 125)
  %439 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %440 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %441 = insertvalue { ptr, ptr, i64 } poison, ptr %439, 0
  %442 = insertvalue { ptr, ptr, i64 } %441, ptr %440, 1
  %443 = insertvalue { ptr, ptr, i64 } %442, i64 0, 2
  %444 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %445 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %446 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %447 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %448 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %449 = extractvalue { ptr, ptr, i64 } %443, 0
  %450 = extractvalue { ptr, ptr, i64 } %443, 1
  %451 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %449, 0
  %452 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %451, ptr %450, 1
  %453 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %452, i64 352, 2
  %454 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %453, i64 8, 3, 0
  %455 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %454, i64 40, 4, 0
  %456 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %455, i64 8, 3, 1
  %457 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %456, i64 1, 4, 1
  %458 = call ptr @llvm.stacksave.p0()
  %459 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %457, ptr %459, align 8
  %460 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %459, 1
  %461 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %268, ptr %461, align 8
  %462 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %461, 1
  %463 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %460, ptr %463, align 8
  %464 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %462, ptr %464, align 8
  call void @memrefCopy(i64 4, ptr %463, ptr %464)
  call void @llvm.stackrestore.p0(ptr %458)
  %465 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %297, 3, 0
  %466 = mul i64 1, %465
  %467 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %297, 3, 1
  %468 = mul i64 %466, %467
  %469 = mul i64 %468, 4
  %470 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %297, 1
  %471 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %297, 2
  %472 = getelementptr float, ptr %470, i64 %471
  %473 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %306, 1
  %474 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %306, 2
  %475 = getelementptr float, ptr %473, i64 %474
  call void @llvm.memcpy.p0.p0.i64(ptr %475, ptr %472, i64 %469, i1 false)
  %476 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 8, i32 8, ptr %278, ptr %320, ptr %102, i32 0, i32 335, i32 117)
  %477 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, 3, 0
  %478 = mul i64 1, %477
  %479 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, 3, 1
  %480 = mul i64 %478, %479
  %481 = mul i64 %480, 4
  %482 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, 1
  %483 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, 2
  %484 = getelementptr float, ptr %482, i64 %483
  %485 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %351, 1
  %486 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %351, 2
  %487 = getelementptr float, ptr %485, i64 %486
  call void @llvm.memcpy.p0.p0.i64(ptr %487, ptr %484, i64 %481, i1 false)
  %488 = call ptr @malloc(i64 512)
  %489 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %488, 0
  %490 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %489, ptr %488, 1
  %491 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %490, i64 0, 2
  %492 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %491, i64 16, 3, 0
  %493 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %492, i64 8, 3, 1
  %494 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %493, i64 8, 4, 0
  %495 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %494, i64 1, 4, 1
  %496 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %497 = mul i64 1, %496
  %498 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %499 = mul i64 %497, %498
  %500 = mul i64 %499, 4
  %501 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %502 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %503 = getelementptr float, ptr %501, i64 %502
  %504 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %495, 1
  %505 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %495, 2
  %506 = getelementptr float, ptr %504, i64 %505
  call void @llvm.memcpy.p0.p0.i64(ptr %506, ptr %503, i64 %500, i1 false)
  ret { ptr, ptr, i64, [2 x i64], [2 x i64] } %495
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
