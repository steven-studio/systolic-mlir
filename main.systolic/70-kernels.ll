; ModuleID = 'LLVMDialectModule'
source_filename = "LLVMDialectModule"

declare void @free(ptr)

declare void @memrefCopy(i64, ptr, ptr)

declare ptr @malloc(i64)

declare i32 @fpga_matmul_tiled_auto_scheduled(i32, i32, i32, ptr, ptr, ptr, i32, i32, i32)

define void @gemm__systolic_kernel(ptr %0, ptr %1, i64 %2, i64 %3, i64 %4, i64 %5, i64 %6, ptr %7, ptr %8, i64 %9, i64 %10, i64 %11, i64 %12, i64 %13, ptr %14, ptr %15, i64 %16, i64 %17, i64 %18, i64 %19, i64 %20) {
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
  %55 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %56 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %57 = insertvalue { ptr, ptr, i64 } poison, ptr %55, 0
  %58 = insertvalue { ptr, ptr, i64 } %57, ptr %56, 1
  %59 = insertvalue { ptr, ptr, i64 } %58, i64 0, 2
  %60 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %61 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %62 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %63 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %64 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %65 = extractvalue { ptr, ptr, i64 } %59, 0
  %66 = extractvalue { ptr, ptr, i64 } %59, 1
  %67 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %65, 0
  %68 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %67, ptr %66, 1
  %69 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %68, i64 0, 2
  %70 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %69, i64 8, 3, 0
  %71 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %70, i64 16, 4, 0
  %72 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %71, i64 8, 3, 1
  %73 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %72, i64 1, 4, 1
  %74 = call ptr @malloc(i64 320)
  %75 = ptrtoint ptr %74 to i64
  %76 = add i64 %75, 63
  %77 = urem i64 %76, 64
  %78 = sub i64 %76, %77
  %79 = inttoptr i64 %78 to ptr
  %80 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %74, 0
  %81 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %80, ptr %79, 1
  %82 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %81, i64 0, 2
  %83 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %82, i64 8, 3, 0
  %84 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %83, i64 8, 3, 1
  %85 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %84, i64 8, 4, 0
  %86 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %85, i64 1, 4, 1
  %87 = call ptr @llvm.stacksave.p0()
  %88 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %73, ptr %88, align 8
  %89 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %88, 1
  %90 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %86, ptr %90, align 8
  %91 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %90, 1
  %92 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %89, ptr %92, align 8
  %93 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %91, ptr %93, align 8
  call void @memrefCopy(i64 4, ptr %92, ptr %93)
  call void @llvm.stackrestore.p0(ptr %87)
  %94 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %86, 1
  %95 = ptrtoint ptr %94 to i64
  %96 = inttoptr i64 %95 to ptr
  %97 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %98 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %99 = insertvalue { ptr, ptr, i64 } poison, ptr %97, 0
  %100 = insertvalue { ptr, ptr, i64 } %99, ptr %98, 1
  %101 = insertvalue { ptr, ptr, i64 } %100, i64 0, 2
  %102 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %103 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %104 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %105 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %106 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %107 = extractvalue { ptr, ptr, i64 } %101, 0
  %108 = extractvalue { ptr, ptr, i64 } %101, 1
  %109 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %107, 0
  %110 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %109, ptr %108, 1
  %111 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %110, i64 0, 2
  %112 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %111, i64 8, 3, 0
  %113 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %112, i64 16, 4, 0
  %114 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %113, i64 16, 3, 1
  %115 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %114, i64 1, 4, 1
  %116 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 0
  %117 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %118 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %116, 0
  %119 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %118, ptr %117, 1
  %120 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %119, i64 0, 2
  %121 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %120, i64 8, 3, 0
  %122 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %121, i64 16, 4, 0
  %123 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %122, i64 16, 3, 1
  %124 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %123, i64 1, 4, 1
  %125 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %115, 3, 0
  %126 = mul i64 1, %125
  %127 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %115, 3, 1
  %128 = mul i64 %126, %127
  %129 = mul i64 %128, 4
  %130 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %115, 1
  %131 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %115, 2
  %132 = getelementptr float, ptr %130, i64 %131
  %133 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %124, 1
  %134 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %124, 2
  %135 = getelementptr float, ptr %133, i64 %134
  call void @llvm.memcpy.p0.p0.i64(ptr %135, ptr %132, i64 %129, i1 false)
  %136 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %137 = ptrtoint ptr %136 to i64
  %138 = inttoptr i64 %137 to ptr
  %139 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %140 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %141 = insertvalue { ptr, ptr, i64 } poison, ptr %139, 0
  %142 = insertvalue { ptr, ptr, i64 } %141, ptr %140, 1
  %143 = insertvalue { ptr, ptr, i64 } %142, i64 0, 2
  %144 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %145 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %146 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %147 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %148 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %149 = extractvalue { ptr, ptr, i64 } %143, 0
  %150 = extractvalue { ptr, ptr, i64 } %143, 1
  %151 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %149, 0
  %152 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %151, ptr %150, 1
  %153 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %152, i64 0, 2
  %154 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %153, i64 16, 3, 0
  %155 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %154, i64 16, 4, 0
  %156 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %155, i64 8, 3, 1
  %157 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %156, i64 1, 4, 1
  %158 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 0
  %159 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %160 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %158, 0
  %161 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %160, ptr %159, 1
  %162 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %161, i64 0, 2
  %163 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %162, i64 16, 3, 0
  %164 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %163, i64 8, 4, 0
  %165 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, i64 8, 3, 1
  %166 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %165, i64 1, 4, 1
  %167 = call ptr @llvm.stacksave.p0()
  %168 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %157, ptr %168, align 8
  %169 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %168, 1
  %170 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %166, ptr %170, align 8
  %171 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %170, 1
  %172 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %169, ptr %172, align 8
  %173 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %171, ptr %173, align 8
  call void @memrefCopy(i64 4, ptr %172, ptr %173)
  call void @llvm.stackrestore.p0(ptr %167)
  %174 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %175 = ptrtoint ptr %174 to i64
  %176 = inttoptr i64 %175 to ptr
  %177 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %138, ptr %176, ptr %96, i32 0, i32 0, i32 125)
  %178 = call ptr @llvm.stacksave.p0()
  %179 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %86, ptr %179, align 8
  %180 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %179, 1
  %181 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %73, ptr %181, align 8
  %182 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %181, 1
  %183 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %180, ptr %183, align 8
  %184 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %182, ptr %184, align 8
  call void @memrefCopy(i64 4, ptr %183, ptr %184)
  call void @llvm.stackrestore.p0(ptr %178)
  %185 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %186 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %187 = insertvalue { ptr, ptr, i64 } poison, ptr %185, 0
  %188 = insertvalue { ptr, ptr, i64 } %187, ptr %186, 1
  %189 = insertvalue { ptr, ptr, i64 } %188, i64 0, 2
  %190 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %191 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %192 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %193 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %194 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %195 = extractvalue { ptr, ptr, i64 } %189, 0
  %196 = extractvalue { ptr, ptr, i64 } %189, 1
  %197 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %195, 0
  %198 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %197, ptr %196, 1
  %199 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %198, i64 8, 2
  %200 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %199, i64 8, 3, 0
  %201 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %200, i64 16, 4, 0
  %202 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %201, i64 8, 3, 1
  %203 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %202, i64 1, 4, 1
  %204 = call ptr @malloc(i64 320)
  %205 = ptrtoint ptr %204 to i64
  %206 = add i64 %205, 63
  %207 = urem i64 %206, 64
  %208 = sub i64 %206, %207
  %209 = inttoptr i64 %208 to ptr
  %210 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %204, 0
  %211 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %210, ptr %209, 1
  %212 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %211, i64 0, 2
  %213 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %212, i64 8, 3, 0
  %214 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %213, i64 8, 3, 1
  %215 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %214, i64 8, 4, 0
  %216 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %215, i64 1, 4, 1
  %217 = call ptr @llvm.stacksave.p0()
  %218 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %203, ptr %218, align 8
  %219 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %218, 1
  %220 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %216, ptr %220, align 8
  %221 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %220, 1
  %222 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %219, ptr %222, align 8
  %223 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %221, ptr %223, align 8
  call void @memrefCopy(i64 4, ptr %222, ptr %223)
  call void @llvm.stackrestore.p0(ptr %217)
  %224 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %216, 1
  %225 = ptrtoint ptr %224 to i64
  %226 = inttoptr i64 %225 to ptr
  %227 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %115, 3, 0
  %228 = mul i64 1, %227
  %229 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %115, 3, 1
  %230 = mul i64 %228, %229
  %231 = mul i64 %230, 4
  %232 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %115, 1
  %233 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %115, 2
  %234 = getelementptr float, ptr %232, i64 %233
  %235 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %124, 1
  %236 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %124, 2
  %237 = getelementptr float, ptr %235, i64 %236
  call void @llvm.memcpy.p0.p0.i64(ptr %237, ptr %234, i64 %231, i1 false)
  %238 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %239 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %240 = insertvalue { ptr, ptr, i64 } poison, ptr %238, 0
  %241 = insertvalue { ptr, ptr, i64 } %240, ptr %239, 1
  %242 = insertvalue { ptr, ptr, i64 } %241, i64 0, 2
  %243 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %244 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %245 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %246 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %247 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %248 = extractvalue { ptr, ptr, i64 } %242, 0
  %249 = extractvalue { ptr, ptr, i64 } %242, 1
  %250 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %248, 0
  %251 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %250, ptr %249, 1
  %252 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %251, i64 8, 2
  %253 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %252, i64 16, 3, 0
  %254 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %253, i64 16, 4, 0
  %255 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %254, i64 8, 3, 1
  %256 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %255, i64 1, 4, 1
  %257 = call ptr @llvm.stacksave.p0()
  %258 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %256, ptr %258, align 8
  %259 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %258, 1
  %260 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %166, ptr %260, align 8
  %261 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %260, 1
  %262 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %259, ptr %262, align 8
  %263 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %261, ptr %263, align 8
  call void @memrefCopy(i64 4, ptr %262, ptr %263)
  call void @llvm.stackrestore.p0(ptr %257)
  %264 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %138, ptr %176, ptr %226, i32 0, i32 125, i32 125)
  %265 = call ptr @llvm.stacksave.p0()
  %266 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %216, ptr %266, align 8
  %267 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %266, 1
  %268 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %203, ptr %268, align 8
  %269 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %268, 1
  %270 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %267, ptr %270, align 8
  %271 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %269, ptr %271, align 8
  call void @memrefCopy(i64 4, ptr %270, ptr %271)
  call void @llvm.stackrestore.p0(ptr %265)
  %272 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %273 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %274 = insertvalue { ptr, ptr, i64 } poison, ptr %272, 0
  %275 = insertvalue { ptr, ptr, i64 } %274, ptr %273, 1
  %276 = insertvalue { ptr, ptr, i64 } %275, i64 0, 2
  %277 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %278 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %279 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %280 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %281 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %282 = extractvalue { ptr, ptr, i64 } %276, 0
  %283 = extractvalue { ptr, ptr, i64 } %276, 1
  %284 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %282, 0
  %285 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %284, ptr %283, 1
  %286 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %285, i64 128, 2
  %287 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %286, i64 4, 3, 0
  %288 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %287, i64 16, 4, 0
  %289 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %288, i64 4, 3, 1
  %290 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %289, i64 1, 4, 1
  %291 = call ptr @malloc(i64 128)
  %292 = ptrtoint ptr %291 to i64
  %293 = add i64 %292, 63
  %294 = urem i64 %293, 64
  %295 = sub i64 %293, %294
  %296 = inttoptr i64 %295 to ptr
  %297 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %291, 0
  %298 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %297, ptr %296, 1
  %299 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %298, i64 0, 2
  %300 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %299, i64 4, 3, 0
  %301 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %300, i64 4, 3, 1
  %302 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %301, i64 4, 4, 0
  %303 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %302, i64 1, 4, 1
  %304 = call ptr @llvm.stacksave.p0()
  %305 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %290, ptr %305, align 8
  %306 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %305, 1
  %307 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %303, ptr %307, align 8
  %308 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %307, 1
  %309 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %306, ptr %309, align 8
  %310 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %308, ptr %310, align 8
  call void @memrefCopy(i64 4, ptr %309, ptr %310)
  call void @llvm.stackrestore.p0(ptr %304)
  %311 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %303, 1
  %312 = ptrtoint ptr %311 to i64
  %313 = inttoptr i64 %312 to ptr
  %314 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %315 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %316 = insertvalue { ptr, ptr, i64 } poison, ptr %314, 0
  %317 = insertvalue { ptr, ptr, i64 } %316, ptr %315, 1
  %318 = insertvalue { ptr, ptr, i64 } %317, i64 0, 2
  %319 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %320 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %321 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %322 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %323 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %324 = extractvalue { ptr, ptr, i64 } %318, 0
  %325 = extractvalue { ptr, ptr, i64 } %318, 1
  %326 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %324, 0
  %327 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %326, ptr %325, 1
  %328 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %327, i64 128, 2
  %329 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %328, i64 4, 3, 0
  %330 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %329, i64 16, 4, 0
  %331 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %330, i64 16, 3, 1
  %332 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %331, i64 1, 4, 1
  %333 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 0
  %334 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %335 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %333, 0
  %336 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %335, ptr %334, 1
  %337 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %336, i64 0, 2
  %338 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %337, i64 4, 3, 0
  %339 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %338, i64 16, 4, 0
  %340 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %339, i64 16, 3, 1
  %341 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %340, i64 1, 4, 1
  %342 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %332, 3, 0
  %343 = mul i64 1, %342
  %344 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %332, 3, 1
  %345 = mul i64 %343, %344
  %346 = mul i64 %345, 4
  %347 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %332, 1
  %348 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %332, 2
  %349 = getelementptr float, ptr %347, i64 %348
  %350 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %341, 1
  %351 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %341, 2
  %352 = getelementptr float, ptr %350, i64 %351
  call void @llvm.memcpy.p0.p0.i64(ptr %352, ptr %349, i64 %346, i1 false)
  %353 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %354 = ptrtoint ptr %353 to i64
  %355 = inttoptr i64 %354 to ptr
  %356 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %357 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %358 = insertvalue { ptr, ptr, i64 } poison, ptr %356, 0
  %359 = insertvalue { ptr, ptr, i64 } %358, ptr %357, 1
  %360 = insertvalue { ptr, ptr, i64 } %359, i64 0, 2
  %361 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %362 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %363 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %364 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %365 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %366 = extractvalue { ptr, ptr, i64 } %360, 0
  %367 = extractvalue { ptr, ptr, i64 } %360, 1
  %368 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %366, 0
  %369 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %368, ptr %367, 1
  %370 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %369, i64 0, 2
  %371 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %370, i64 16, 3, 0
  %372 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %371, i64 16, 4, 0
  %373 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %372, i64 4, 3, 1
  %374 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %373, i64 1, 4, 1
  %375 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 0
  %376 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %377 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %375, 0
  %378 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %377, ptr %376, 1
  %379 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %378, i64 0, 2
  %380 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %379, i64 16, 3, 0
  %381 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %380, i64 4, 4, 0
  %382 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %381, i64 4, 3, 1
  %383 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %382, i64 1, 4, 1
  %384 = call ptr @llvm.stacksave.p0()
  %385 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %374, ptr %385, align 8
  %386 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %385, 1
  %387 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %383, ptr %387, align 8
  %388 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %387, 1
  %389 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %386, ptr %389, align 8
  %390 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %388, ptr %390, align 8
  call void @memrefCopy(i64 4, ptr %389, ptr %390)
  call void @llvm.stackrestore.p0(ptr %384)
  %391 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %392 = ptrtoint ptr %391 to i64
  %393 = inttoptr i64 %392 to ptr
  %394 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %355, ptr %393, ptr %313, i32 1, i32 0, i32 117)
  %395 = call ptr @llvm.stacksave.p0()
  %396 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %303, ptr %396, align 8
  %397 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %396, 1
  %398 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %290, ptr %398, align 8
  %399 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %398, 1
  %400 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %397, ptr %400, align 8
  %401 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %399, ptr %401, align 8
  call void @memrefCopy(i64 4, ptr %400, ptr %401)
  call void @llvm.stackrestore.p0(ptr %395)
  %402 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %403 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %404 = insertvalue { ptr, ptr, i64 } poison, ptr %402, 0
  %405 = insertvalue { ptr, ptr, i64 } %404, ptr %403, 1
  %406 = insertvalue { ptr, ptr, i64 } %405, i64 0, 2
  %407 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %408 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %409 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %410 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %411 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %412 = extractvalue { ptr, ptr, i64 } %406, 0
  %413 = extractvalue { ptr, ptr, i64 } %406, 1
  %414 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %412, 0
  %415 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %414, ptr %413, 1
  %416 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %415, i64 132, 2
  %417 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %416, i64 4, 3, 0
  %418 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %417, i64 16, 4, 0
  %419 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %418, i64 4, 3, 1
  %420 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %419, i64 1, 4, 1
  %421 = call ptr @malloc(i64 128)
  %422 = ptrtoint ptr %421 to i64
  %423 = add i64 %422, 63
  %424 = urem i64 %423, 64
  %425 = sub i64 %423, %424
  %426 = inttoptr i64 %425 to ptr
  %427 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %421, 0
  %428 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %427, ptr %426, 1
  %429 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %428, i64 0, 2
  %430 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %429, i64 4, 3, 0
  %431 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %430, i64 4, 3, 1
  %432 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %431, i64 4, 4, 0
  %433 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %432, i64 1, 4, 1
  %434 = call ptr @llvm.stacksave.p0()
  %435 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %420, ptr %435, align 8
  %436 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %435, 1
  %437 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %433, ptr %437, align 8
  %438 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %437, 1
  %439 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %436, ptr %439, align 8
  %440 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %438, ptr %440, align 8
  call void @memrefCopy(i64 4, ptr %439, ptr %440)
  call void @llvm.stackrestore.p0(ptr %434)
  %441 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %433, 1
  %442 = ptrtoint ptr %441 to i64
  %443 = inttoptr i64 %442 to ptr
  %444 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %332, 3, 0
  %445 = mul i64 1, %444
  %446 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %332, 3, 1
  %447 = mul i64 %445, %446
  %448 = mul i64 %447, 4
  %449 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %332, 1
  %450 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %332, 2
  %451 = getelementptr float, ptr %449, i64 %450
  %452 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %341, 1
  %453 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %341, 2
  %454 = getelementptr float, ptr %452, i64 %453
  call void @llvm.memcpy.p0.p0.i64(ptr %454, ptr %451, i64 %448, i1 false)
  %455 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %456 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %457 = insertvalue { ptr, ptr, i64 } poison, ptr %455, 0
  %458 = insertvalue { ptr, ptr, i64 } %457, ptr %456, 1
  %459 = insertvalue { ptr, ptr, i64 } %458, i64 0, 2
  %460 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %461 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %462 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %463 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %464 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %465 = extractvalue { ptr, ptr, i64 } %459, 0
  %466 = extractvalue { ptr, ptr, i64 } %459, 1
  %467 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %465, 0
  %468 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %467, ptr %466, 1
  %469 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %468, i64 4, 2
  %470 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %469, i64 16, 3, 0
  %471 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %470, i64 16, 4, 0
  %472 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %471, i64 4, 3, 1
  %473 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %472, i64 1, 4, 1
  %474 = call ptr @llvm.stacksave.p0()
  %475 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %473, ptr %475, align 8
  %476 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %475, 1
  %477 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %383, ptr %477, align 8
  %478 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %477, 1
  %479 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %476, ptr %479, align 8
  %480 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %478, ptr %480, align 8
  call void @memrefCopy(i64 4, ptr %479, ptr %480)
  call void @llvm.stackrestore.p0(ptr %474)
  %481 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %355, ptr %393, ptr %443, i32 2, i32 0, i32 117)
  %482 = call ptr @llvm.stacksave.p0()
  %483 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %433, ptr %483, align 8
  %484 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %483, 1
  %485 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %420, ptr %485, align 8
  %486 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %485, 1
  %487 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %484, ptr %487, align 8
  %488 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %486, ptr %488, align 8
  call void @memrefCopy(i64 4, ptr %487, ptr %488)
  call void @llvm.stackrestore.p0(ptr %482)
  %489 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %490 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %491 = insertvalue { ptr, ptr, i64 } poison, ptr %489, 0
  %492 = insertvalue { ptr, ptr, i64 } %491, ptr %490, 1
  %493 = insertvalue { ptr, ptr, i64 } %492, i64 0, 2
  %494 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %495 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %496 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %497 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %498 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %499 = extractvalue { ptr, ptr, i64 } %493, 0
  %500 = extractvalue { ptr, ptr, i64 } %493, 1
  %501 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %499, 0
  %502 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %501, ptr %500, 1
  %503 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %502, i64 136, 2
  %504 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %503, i64 4, 3, 0
  %505 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %504, i64 16, 4, 0
  %506 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %505, i64 4, 3, 1
  %507 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %506, i64 1, 4, 1
  %508 = call ptr @malloc(i64 128)
  %509 = ptrtoint ptr %508 to i64
  %510 = add i64 %509, 63
  %511 = urem i64 %510, 64
  %512 = sub i64 %510, %511
  %513 = inttoptr i64 %512 to ptr
  %514 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %508, 0
  %515 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %514, ptr %513, 1
  %516 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %515, i64 0, 2
  %517 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %516, i64 4, 3, 0
  %518 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %517, i64 4, 3, 1
  %519 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %518, i64 4, 4, 0
  %520 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %519, i64 1, 4, 1
  %521 = call ptr @llvm.stacksave.p0()
  %522 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %507, ptr %522, align 8
  %523 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %522, 1
  %524 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %520, ptr %524, align 8
  %525 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %524, 1
  %526 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %523, ptr %526, align 8
  %527 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %525, ptr %527, align 8
  call void @memrefCopy(i64 4, ptr %526, ptr %527)
  call void @llvm.stackrestore.p0(ptr %521)
  %528 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %520, 1
  %529 = ptrtoint ptr %528 to i64
  %530 = inttoptr i64 %529 to ptr
  %531 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %332, 3, 0
  %532 = mul i64 1, %531
  %533 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %332, 3, 1
  %534 = mul i64 %532, %533
  %535 = mul i64 %534, 4
  %536 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %332, 1
  %537 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %332, 2
  %538 = getelementptr float, ptr %536, i64 %537
  %539 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %341, 1
  %540 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %341, 2
  %541 = getelementptr float, ptr %539, i64 %540
  call void @llvm.memcpy.p0.p0.i64(ptr %541, ptr %538, i64 %535, i1 false)
  %542 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %543 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %544 = insertvalue { ptr, ptr, i64 } poison, ptr %542, 0
  %545 = insertvalue { ptr, ptr, i64 } %544, ptr %543, 1
  %546 = insertvalue { ptr, ptr, i64 } %545, i64 0, 2
  %547 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %548 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %549 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %550 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %551 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %552 = extractvalue { ptr, ptr, i64 } %546, 0
  %553 = extractvalue { ptr, ptr, i64 } %546, 1
  %554 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %552, 0
  %555 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %554, ptr %553, 1
  %556 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %555, i64 8, 2
  %557 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %556, i64 16, 3, 0
  %558 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %557, i64 16, 4, 0
  %559 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %558, i64 4, 3, 1
  %560 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %559, i64 1, 4, 1
  %561 = call ptr @llvm.stacksave.p0()
  %562 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %560, ptr %562, align 8
  %563 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %562, 1
  %564 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %383, ptr %564, align 8
  %565 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %564, 1
  %566 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %563, ptr %566, align 8
  %567 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %565, ptr %567, align 8
  call void @memrefCopy(i64 4, ptr %566, ptr %567)
  call void @llvm.stackrestore.p0(ptr %561)
  %568 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %355, ptr %393, ptr %530, i32 3, i32 0, i32 117)
  %569 = call ptr @llvm.stacksave.p0()
  %570 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %520, ptr %570, align 8
  %571 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %570, 1
  %572 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %507, ptr %572, align 8
  %573 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %572, 1
  %574 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %571, ptr %574, align 8
  %575 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %573, ptr %575, align 8
  call void @memrefCopy(i64 4, ptr %574, ptr %575)
  call void @llvm.stackrestore.p0(ptr %569)
  %576 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %577 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %578 = insertvalue { ptr, ptr, i64 } poison, ptr %576, 0
  %579 = insertvalue { ptr, ptr, i64 } %578, ptr %577, 1
  %580 = insertvalue { ptr, ptr, i64 } %579, i64 0, 2
  %581 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %582 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %583 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %584 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %585 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %586 = extractvalue { ptr, ptr, i64 } %580, 0
  %587 = extractvalue { ptr, ptr, i64 } %580, 1
  %588 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %586, 0
  %589 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %588, ptr %587, 1
  %590 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %589, i64 140, 2
  %591 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %590, i64 4, 3, 0
  %592 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %591, i64 16, 4, 0
  %593 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %592, i64 4, 3, 1
  %594 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %593, i64 1, 4, 1
  %595 = call ptr @malloc(i64 128)
  %596 = ptrtoint ptr %595 to i64
  %597 = add i64 %596, 63
  %598 = urem i64 %597, 64
  %599 = sub i64 %597, %598
  %600 = inttoptr i64 %599 to ptr
  %601 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %595, 0
  %602 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %601, ptr %600, 1
  %603 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %602, i64 0, 2
  %604 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %603, i64 4, 3, 0
  %605 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %604, i64 4, 3, 1
  %606 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %605, i64 4, 4, 0
  %607 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %606, i64 1, 4, 1
  %608 = call ptr @llvm.stacksave.p0()
  %609 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %594, ptr %609, align 8
  %610 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %609, 1
  %611 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %607, ptr %611, align 8
  %612 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %611, 1
  %613 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %610, ptr %613, align 8
  %614 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %612, ptr %614, align 8
  call void @memrefCopy(i64 4, ptr %613, ptr %614)
  call void @llvm.stackrestore.p0(ptr %608)
  %615 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %607, 1
  %616 = ptrtoint ptr %615 to i64
  %617 = inttoptr i64 %616 to ptr
  %618 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %332, 3, 0
  %619 = mul i64 1, %618
  %620 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %332, 3, 1
  %621 = mul i64 %619, %620
  %622 = mul i64 %621, 4
  %623 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %332, 1
  %624 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %332, 2
  %625 = getelementptr float, ptr %623, i64 %624
  %626 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %341, 1
  %627 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %341, 2
  %628 = getelementptr float, ptr %626, i64 %627
  call void @llvm.memcpy.p0.p0.i64(ptr %628, ptr %625, i64 %622, i1 false)
  %629 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %630 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %631 = insertvalue { ptr, ptr, i64 } poison, ptr %629, 0
  %632 = insertvalue { ptr, ptr, i64 } %631, ptr %630, 1
  %633 = insertvalue { ptr, ptr, i64 } %632, i64 0, 2
  %634 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %635 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %636 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %637 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %638 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %639 = extractvalue { ptr, ptr, i64 } %633, 0
  %640 = extractvalue { ptr, ptr, i64 } %633, 1
  %641 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %639, 0
  %642 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %641, ptr %640, 1
  %643 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %642, i64 12, 2
  %644 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %643, i64 16, 3, 0
  %645 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %644, i64 16, 4, 0
  %646 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %645, i64 4, 3, 1
  %647 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %646, i64 1, 4, 1
  %648 = call ptr @llvm.stacksave.p0()
  %649 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %647, ptr %649, align 8
  %650 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %649, 1
  %651 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %383, ptr %651, align 8
  %652 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %651, 1
  %653 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %650, ptr %653, align 8
  %654 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %652, ptr %654, align 8
  call void @memrefCopy(i64 4, ptr %653, ptr %654)
  call void @llvm.stackrestore.p0(ptr %648)
  %655 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %355, ptr %393, ptr %617, i32 1, i32 117, i32 117)
  %656 = call ptr @llvm.stacksave.p0()
  %657 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %607, ptr %657, align 8
  %658 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %657, 1
  %659 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %594, ptr %659, align 8
  %660 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %659, 1
  %661 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %658, ptr %661, align 8
  %662 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %660, ptr %662, align 8
  call void @memrefCopy(i64 4, ptr %661, ptr %662)
  call void @llvm.stackrestore.p0(ptr %656)
  %663 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %664 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %665 = insertvalue { ptr, ptr, i64 } poison, ptr %663, 0
  %666 = insertvalue { ptr, ptr, i64 } %665, ptr %664, 1
  %667 = insertvalue { ptr, ptr, i64 } %666, i64 0, 2
  %668 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %669 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %670 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %671 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %672 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %673 = extractvalue { ptr, ptr, i64 } %667, 0
  %674 = extractvalue { ptr, ptr, i64 } %667, 1
  %675 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %673, 0
  %676 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %675, ptr %674, 1
  %677 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %676, i64 192, 2
  %678 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %677, i64 4, 3, 0
  %679 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %678, i64 16, 4, 0
  %680 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %679, i64 4, 3, 1
  %681 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %680, i64 1, 4, 1
  %682 = call ptr @malloc(i64 128)
  %683 = ptrtoint ptr %682 to i64
  %684 = add i64 %683, 63
  %685 = urem i64 %684, 64
  %686 = sub i64 %684, %685
  %687 = inttoptr i64 %686 to ptr
  %688 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %682, 0
  %689 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %688, ptr %687, 1
  %690 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %689, i64 0, 2
  %691 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %690, i64 4, 3, 0
  %692 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %691, i64 4, 3, 1
  %693 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %692, i64 4, 4, 0
  %694 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %693, i64 1, 4, 1
  %695 = call ptr @llvm.stacksave.p0()
  %696 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %681, ptr %696, align 8
  %697 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %696, 1
  %698 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %694, ptr %698, align 8
  %699 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %698, 1
  %700 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %697, ptr %700, align 8
  %701 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %699, ptr %701, align 8
  call void @memrefCopy(i64 4, ptr %700, ptr %701)
  call void @llvm.stackrestore.p0(ptr %695)
  %702 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %694, 1
  %703 = ptrtoint ptr %702 to i64
  %704 = inttoptr i64 %703 to ptr
  %705 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %706 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %707 = insertvalue { ptr, ptr, i64 } poison, ptr %705, 0
  %708 = insertvalue { ptr, ptr, i64 } %707, ptr %706, 1
  %709 = insertvalue { ptr, ptr, i64 } %708, i64 0, 2
  %710 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %711 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %712 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %713 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %714 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %715 = extractvalue { ptr, ptr, i64 } %709, 0
  %716 = extractvalue { ptr, ptr, i64 } %709, 1
  %717 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %715, 0
  %718 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %717, ptr %716, 1
  %719 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %718, i64 192, 2
  %720 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %719, i64 4, 3, 0
  %721 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %720, i64 16, 4, 0
  %722 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %721, i64 16, 3, 1
  %723 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %722, i64 1, 4, 1
  %724 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %723, 3, 0
  %725 = mul i64 1, %724
  %726 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %723, 3, 1
  %727 = mul i64 %725, %726
  %728 = mul i64 %727, 4
  %729 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %723, 1
  %730 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %723, 2
  %731 = getelementptr float, ptr %729, i64 %730
  %732 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %341, 1
  %733 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %341, 2
  %734 = getelementptr float, ptr %732, i64 %733
  call void @llvm.memcpy.p0.p0.i64(ptr %734, ptr %731, i64 %728, i1 false)
  %735 = call ptr @llvm.stacksave.p0()
  %736 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %374, ptr %736, align 8
  %737 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %736, 1
  %738 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %383, ptr %738, align 8
  %739 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %738, 1
  %740 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %737, ptr %740, align 8
  %741 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %739, ptr %741, align 8
  call void @memrefCopy(i64 4, ptr %740, ptr %741)
  call void @llvm.stackrestore.p0(ptr %735)
  %742 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %355, ptr %393, ptr %704, i32 2, i32 117, i32 117)
  %743 = call ptr @llvm.stacksave.p0()
  %744 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %694, ptr %744, align 8
  %745 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %744, 1
  %746 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %681, ptr %746, align 8
  %747 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %746, 1
  %748 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %745, ptr %748, align 8
  %749 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %747, ptr %749, align 8
  call void @memrefCopy(i64 4, ptr %748, ptr %749)
  call void @llvm.stackrestore.p0(ptr %743)
  %750 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %751 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %752 = insertvalue { ptr, ptr, i64 } poison, ptr %750, 0
  %753 = insertvalue { ptr, ptr, i64 } %752, ptr %751, 1
  %754 = insertvalue { ptr, ptr, i64 } %753, i64 0, 2
  %755 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %756 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %757 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %758 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %759 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %760 = extractvalue { ptr, ptr, i64 } %754, 0
  %761 = extractvalue { ptr, ptr, i64 } %754, 1
  %762 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %760, 0
  %763 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %762, ptr %761, 1
  %764 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %763, i64 196, 2
  %765 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %764, i64 4, 3, 0
  %766 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %765, i64 16, 4, 0
  %767 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %766, i64 4, 3, 1
  %768 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %767, i64 1, 4, 1
  %769 = call ptr @malloc(i64 128)
  %770 = ptrtoint ptr %769 to i64
  %771 = add i64 %770, 63
  %772 = urem i64 %771, 64
  %773 = sub i64 %771, %772
  %774 = inttoptr i64 %773 to ptr
  %775 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %769, 0
  %776 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %775, ptr %774, 1
  %777 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %776, i64 0, 2
  %778 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %777, i64 4, 3, 0
  %779 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %778, i64 4, 3, 1
  %780 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %779, i64 4, 4, 0
  %781 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %780, i64 1, 4, 1
  %782 = call ptr @llvm.stacksave.p0()
  %783 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %768, ptr %783, align 8
  %784 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %783, 1
  %785 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %781, ptr %785, align 8
  %786 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %785, 1
  %787 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %784, ptr %787, align 8
  %788 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %786, ptr %788, align 8
  call void @memrefCopy(i64 4, ptr %787, ptr %788)
  call void @llvm.stackrestore.p0(ptr %782)
  %789 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %781, 1
  %790 = ptrtoint ptr %789 to i64
  %791 = inttoptr i64 %790 to ptr
  %792 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %723, 3, 0
  %793 = mul i64 1, %792
  %794 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %723, 3, 1
  %795 = mul i64 %793, %794
  %796 = mul i64 %795, 4
  %797 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %723, 1
  %798 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %723, 2
  %799 = getelementptr float, ptr %797, i64 %798
  %800 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %341, 1
  %801 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %341, 2
  %802 = getelementptr float, ptr %800, i64 %801
  call void @llvm.memcpy.p0.p0.i64(ptr %802, ptr %799, i64 %796, i1 false)
  %803 = call ptr @llvm.stacksave.p0()
  %804 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %473, ptr %804, align 8
  %805 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %804, 1
  %806 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %383, ptr %806, align 8
  %807 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %806, 1
  %808 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %805, ptr %808, align 8
  %809 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %807, ptr %809, align 8
  call void @memrefCopy(i64 4, ptr %808, ptr %809)
  call void @llvm.stackrestore.p0(ptr %803)
  %810 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %355, ptr %393, ptr %791, i32 3, i32 117, i32 117)
  %811 = call ptr @llvm.stacksave.p0()
  %812 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %781, ptr %812, align 8
  %813 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %812, 1
  %814 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %768, ptr %814, align 8
  %815 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %814, 1
  %816 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %813, ptr %816, align 8
  %817 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %815, ptr %817, align 8
  call void @memrefCopy(i64 4, ptr %816, ptr %817)
  call void @llvm.stackrestore.p0(ptr %811)
  %818 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %819 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %820 = insertvalue { ptr, ptr, i64 } poison, ptr %818, 0
  %821 = insertvalue { ptr, ptr, i64 } %820, ptr %819, 1
  %822 = insertvalue { ptr, ptr, i64 } %821, i64 0, 2
  %823 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %824 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %825 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %826 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %827 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %828 = extractvalue { ptr, ptr, i64 } %822, 0
  %829 = extractvalue { ptr, ptr, i64 } %822, 1
  %830 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %828, 0
  %831 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %830, ptr %829, 1
  %832 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %831, i64 200, 2
  %833 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %832, i64 4, 3, 0
  %834 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %833, i64 16, 4, 0
  %835 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %834, i64 4, 3, 1
  %836 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %835, i64 1, 4, 1
  %837 = call ptr @malloc(i64 128)
  %838 = ptrtoint ptr %837 to i64
  %839 = add i64 %838, 63
  %840 = urem i64 %839, 64
  %841 = sub i64 %839, %840
  %842 = inttoptr i64 %841 to ptr
  %843 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %837, 0
  %844 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %843, ptr %842, 1
  %845 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %844, i64 0, 2
  %846 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %845, i64 4, 3, 0
  %847 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %846, i64 4, 3, 1
  %848 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %847, i64 4, 4, 0
  %849 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %848, i64 1, 4, 1
  %850 = call ptr @llvm.stacksave.p0()
  %851 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %836, ptr %851, align 8
  %852 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %851, 1
  %853 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %849, ptr %853, align 8
  %854 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %853, 1
  %855 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %852, ptr %855, align 8
  %856 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %854, ptr %856, align 8
  call void @memrefCopy(i64 4, ptr %855, ptr %856)
  call void @llvm.stackrestore.p0(ptr %850)
  %857 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %849, 1
  %858 = ptrtoint ptr %857 to i64
  %859 = inttoptr i64 %858 to ptr
  %860 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %723, 3, 0
  %861 = mul i64 1, %860
  %862 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %723, 3, 1
  %863 = mul i64 %861, %862
  %864 = mul i64 %863, 4
  %865 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %723, 1
  %866 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %723, 2
  %867 = getelementptr float, ptr %865, i64 %866
  %868 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %341, 1
  %869 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %341, 2
  %870 = getelementptr float, ptr %868, i64 %869
  call void @llvm.memcpy.p0.p0.i64(ptr %870, ptr %867, i64 %864, i1 false)
  %871 = call ptr @llvm.stacksave.p0()
  %872 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %560, ptr %872, align 8
  %873 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %872, 1
  %874 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %383, ptr %874, align 8
  %875 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %874, 1
  %876 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %873, ptr %876, align 8
  %877 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %875, ptr %877, align 8
  call void @memrefCopy(i64 4, ptr %876, ptr %877)
  call void @llvm.stackrestore.p0(ptr %871)
  %878 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %355, ptr %393, ptr %859, i32 1, i32 234, i32 117)
  %879 = call ptr @llvm.stacksave.p0()
  %880 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %849, ptr %880, align 8
  %881 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %880, 1
  %882 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %836, ptr %882, align 8
  %883 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %882, 1
  %884 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %881, ptr %884, align 8
  %885 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %883, ptr %885, align 8
  call void @memrefCopy(i64 4, ptr %884, ptr %885)
  call void @llvm.stackrestore.p0(ptr %879)
  %886 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %887 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %888 = insertvalue { ptr, ptr, i64 } poison, ptr %886, 0
  %889 = insertvalue { ptr, ptr, i64 } %888, ptr %887, 1
  %890 = insertvalue { ptr, ptr, i64 } %889, i64 0, 2
  %891 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %892 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %893 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %894 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %895 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %896 = extractvalue { ptr, ptr, i64 } %890, 0
  %897 = extractvalue { ptr, ptr, i64 } %890, 1
  %898 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %896, 0
  %899 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %898, ptr %897, 1
  %900 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %899, i64 204, 2
  %901 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %900, i64 4, 3, 0
  %902 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %901, i64 16, 4, 0
  %903 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %902, i64 4, 3, 1
  %904 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %903, i64 1, 4, 1
  %905 = call ptr @malloc(i64 128)
  %906 = ptrtoint ptr %905 to i64
  %907 = add i64 %906, 63
  %908 = urem i64 %907, 64
  %909 = sub i64 %907, %908
  %910 = inttoptr i64 %909 to ptr
  %911 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %905, 0
  %912 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %911, ptr %910, 1
  %913 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %912, i64 0, 2
  %914 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %913, i64 4, 3, 0
  %915 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %914, i64 4, 3, 1
  %916 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %915, i64 4, 4, 0
  %917 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %916, i64 1, 4, 1
  %918 = call ptr @llvm.stacksave.p0()
  %919 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %904, ptr %919, align 8
  %920 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %919, 1
  %921 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %917, ptr %921, align 8
  %922 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %921, 1
  %923 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %920, ptr %923, align 8
  %924 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %922, ptr %924, align 8
  call void @memrefCopy(i64 4, ptr %923, ptr %924)
  call void @llvm.stackrestore.p0(ptr %918)
  %925 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %917, 1
  %926 = ptrtoint ptr %925 to i64
  %927 = inttoptr i64 %926 to ptr
  %928 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %723, 3, 0
  %929 = mul i64 1, %928
  %930 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %723, 3, 1
  %931 = mul i64 %929, %930
  %932 = mul i64 %931, 4
  %933 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %723, 1
  %934 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %723, 2
  %935 = getelementptr float, ptr %933, i64 %934
  %936 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %341, 1
  %937 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %341, 2
  %938 = getelementptr float, ptr %936, i64 %937
  call void @llvm.memcpy.p0.p0.i64(ptr %938, ptr %935, i64 %932, i1 false)
  %939 = call ptr @llvm.stacksave.p0()
  %940 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %647, ptr %940, align 8
  %941 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %940, 1
  %942 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %383, ptr %942, align 8
  %943 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %942, 1
  %944 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %941, ptr %944, align 8
  %945 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %943, ptr %945, align 8
  call void @memrefCopy(i64 4, ptr %944, ptr %945)
  call void @llvm.stackrestore.p0(ptr %939)
  %946 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %355, ptr %393, ptr %927, i32 2, i32 234, i32 117)
  %947 = call ptr @llvm.stacksave.p0()
  %948 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %917, ptr %948, align 8
  %949 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %948, 1
  %950 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %904, ptr %950, align 8
  %951 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %950, 1
  %952 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %949, ptr %952, align 8
  %953 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %951, ptr %953, align 8
  call void @memrefCopy(i64 4, ptr %952, ptr %953)
  call void @llvm.stackrestore.p0(ptr %947)
  %954 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %86, 0
  call void @free(ptr %954)
  %955 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %216, 0
  call void @free(ptr %955)
  %956 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %303, 0
  call void @free(ptr %956)
  %957 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %433, 0
  call void @free(ptr %957)
  %958 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %520, 0
  call void @free(ptr %958)
  %959 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %607, 0
  call void @free(ptr %959)
  %960 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %694, 0
  call void @free(ptr %960)
  %961 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %781, 0
  call void @free(ptr %961)
  %962 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %849, 0
  call void @free(ptr %962)
  %963 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %917, 0
  call void @free(ptr %963)
  ret void
}

; Function Attrs: nocallback nofree nosync nounwind willreturn
declare ptr @llvm.stacksave.p0() #0

; Function Attrs: nocallback nofree nosync nounwind willreturn
declare void @llvm.stackrestore.p0(ptr) #0

; Function Attrs: nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
declare void @llvm.memcpy.p0.p0.i64(ptr noalias writeonly captures(none), ptr noalias readonly captures(none), i64, i1 immarg) #1

attributes #0 = { nocallback nofree nosync nounwind willreturn }
attributes #1 = { nocallback nofree nosync nounwind willreturn memory(argmem: readwrite) }

!llvm.module.flags = !{!0}

!0 = !{i32 2, !"Debug Info Version", i32 3}
