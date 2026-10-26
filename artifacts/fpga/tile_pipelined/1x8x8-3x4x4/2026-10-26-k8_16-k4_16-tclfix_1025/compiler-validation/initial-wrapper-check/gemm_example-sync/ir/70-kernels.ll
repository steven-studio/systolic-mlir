; ModuleID = 'LLVMDialectModule'
source_filename = "LLVMDialectModule"

declare void @memrefCopy(i64, ptr, ptr)

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
  %55 = alloca float, i64 64, align 4
  %56 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } poison, ptr %55, 0
  %57 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %56, ptr %55, 1
  %58 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %57, i64 0, 2
  %59 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %58, i64 64, 3, 0
  %60 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %59, i64 1, 4, 0
  br label %61

61:                                               ; preds = %74, %21
  %62 = phi i64 [ %75, %74 ], [ 0, %21 ]
  %63 = icmp slt i64 %62, 24
  br i1 %63, label %64, label %76

64:                                               ; preds = %61
  br label %65

65:                                               ; preds = %68, %64
  %66 = phi i64 [ %73, %68 ], [ 0, %64 ]
  %67 = icmp slt i64 %66, 20
  br i1 %67, label %68, label %74

68:                                               ; preds = %65
  %69 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %70 = mul nuw nsw i64 %62, 20
  %71 = add nuw nsw i64 %70, %66
  %72 = getelementptr inbounds nuw float, ptr %69, i64 %71
  store float 0.000000e+00, ptr %72, align 4
  %73 = add i64 %66, 1
  br label %65

74:                                               ; preds = %65
  %75 = add i64 %62, 1
  br label %61

76:                                               ; preds = %61
  %77 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %78 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %79 = insertvalue { ptr, ptr, i64 } poison, ptr %77, 0
  %80 = insertvalue { ptr, ptr, i64 } %79, ptr %78, 1
  %81 = insertvalue { ptr, ptr, i64 } %80, i64 0, 2
  %82 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %83 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %84 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %85 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %86 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %87 = extractvalue { ptr, ptr, i64 } %81, 0
  %88 = extractvalue { ptr, ptr, i64 } %81, 1
  %89 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %87, 0
  %90 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %89, ptr %88, 1
  %91 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %90, i64 0, 2
  %92 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %91, i64 8, 3, 0
  %93 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %92, i64 20, 4, 0
  %94 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %93, i64 8, 3, 1
  %95 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %94, i64 1, 4, 1
  %96 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 0
  %97 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 1
  %98 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %96, 0
  %99 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %98, ptr %97, 1
  %100 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %99, i64 0, 2
  %101 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %100, i64 8, 3, 0
  %102 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %101, i64 8, 4, 0
  %103 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %102, i64 8, 3, 1
  %104 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %103, i64 1, 4, 1
  %105 = call ptr @llvm.stacksave.p0()
  %106 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %95, ptr %106, align 8
  %107 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %106, 1
  %108 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %104, ptr %108, align 8
  %109 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %108, 1
  %110 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %107, ptr %110, align 8
  %111 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %109, ptr %111, align 8
  call void @memrefCopy(i64 4, ptr %110, ptr %111)
  call void @llvm.stackrestore.p0(ptr %105)
  %112 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 1
  %113 = ptrtoint ptr %112 to i64
  %114 = inttoptr i64 %113 to ptr
  %115 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %116 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %117 = insertvalue { ptr, ptr, i64 } poison, ptr %115, 0
  %118 = insertvalue { ptr, ptr, i64 } %117, ptr %116, 1
  %119 = insertvalue { ptr, ptr, i64 } %118, i64 0, 2
  %120 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %121 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %122 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %123 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %124 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %125 = extractvalue { ptr, ptr, i64 } %119, 0
  %126 = extractvalue { ptr, ptr, i64 } %119, 1
  %127 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %125, 0
  %128 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %127, ptr %126, 1
  %129 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %128, i64 0, 2
  %130 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %129, i64 8, 3, 0
  %131 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %130, i64 40, 4, 0
  %132 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %131, i64 16, 3, 1
  %133 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %132, i64 1, 4, 1
  %134 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 0
  %135 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %136 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %134, 0
  %137 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %136, ptr %135, 1
  %138 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %137, i64 0, 2
  %139 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %138, i64 8, 3, 0
  %140 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %139, i64 16, 4, 0
  %141 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %140, i64 16, 3, 1
  %142 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %141, i64 1, 4, 1
  %143 = call ptr @llvm.stacksave.p0()
  %144 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %133, ptr %144, align 8
  %145 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %144, 1
  %146 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %142, ptr %146, align 8
  %147 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %146, 1
  %148 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %145, ptr %148, align 8
  %149 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %147, ptr %149, align 8
  call void @memrefCopy(i64 4, ptr %148, ptr %149)
  call void @llvm.stackrestore.p0(ptr %143)
  %150 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %151 = ptrtoint ptr %150 to i64
  %152 = inttoptr i64 %151 to ptr
  %153 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %154 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %155 = insertvalue { ptr, ptr, i64 } poison, ptr %153, 0
  %156 = insertvalue { ptr, ptr, i64 } %155, ptr %154, 1
  %157 = insertvalue { ptr, ptr, i64 } %156, i64 0, 2
  %158 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %159 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %160 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %161 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %162 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %163 = extractvalue { ptr, ptr, i64 } %157, 0
  %164 = extractvalue { ptr, ptr, i64 } %157, 1
  %165 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %163, 0
  %166 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %165, ptr %164, 1
  %167 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %166, i64 0, 2
  %168 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %167, i64 16, 3, 0
  %169 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %168, i64 20, 4, 0
  %170 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %169, i64 8, 3, 1
  %171 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %170, i64 1, 4, 1
  %172 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 0
  %173 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %174 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %172, 0
  %175 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %174, ptr %173, 1
  %176 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %175, i64 0, 2
  %177 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %176, i64 16, 3, 0
  %178 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %177, i64 8, 4, 0
  %179 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %178, i64 8, 3, 1
  %180 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %179, i64 1, 4, 1
  %181 = call ptr @llvm.stacksave.p0()
  %182 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %171, ptr %182, align 8
  %183 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %182, 1
  %184 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %180, ptr %184, align 8
  %185 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %184, 1
  %186 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %183, ptr %186, align 8
  %187 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %185, ptr %187, align 8
  call void @memrefCopy(i64 4, ptr %186, ptr %187)
  call void @llvm.stackrestore.p0(ptr %181)
  %188 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %189 = ptrtoint ptr %188 to i64
  %190 = inttoptr i64 %189 to ptr
  %191 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %152, ptr %190, ptr %114, i32 0, i32 0, i32 125)
  %192 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %193 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %194 = insertvalue { ptr, ptr, i64 } poison, ptr %192, 0
  %195 = insertvalue { ptr, ptr, i64 } %194, ptr %193, 1
  %196 = insertvalue { ptr, ptr, i64 } %195, i64 0, 2
  %197 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %198 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %199 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %200 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %201 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %202 = extractvalue { ptr, ptr, i64 } %196, 0
  %203 = extractvalue { ptr, ptr, i64 } %196, 1
  %204 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %202, 0
  %205 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %204, ptr %203, 1
  %206 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %205, i64 16, 2
  %207 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %206, i64 8, 3, 0
  %208 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %207, i64 40, 4, 0
  %209 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %208, i64 16, 3, 1
  %210 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %209, i64 1, 4, 1
  %211 = call ptr @llvm.stacksave.p0()
  %212 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %210, ptr %212, align 8
  %213 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %212, 1
  %214 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %142, ptr %214, align 8
  %215 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %214, 1
  %216 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %213, ptr %216, align 8
  %217 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %215, ptr %217, align 8
  call void @memrefCopy(i64 4, ptr %216, ptr %217)
  call void @llvm.stackrestore.p0(ptr %211)
  %218 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %219 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %220 = insertvalue { ptr, ptr, i64 } poison, ptr %218, 0
  %221 = insertvalue { ptr, ptr, i64 } %220, ptr %219, 1
  %222 = insertvalue { ptr, ptr, i64 } %221, i64 0, 2
  %223 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %224 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %225 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %226 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %227 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %228 = extractvalue { ptr, ptr, i64 } %222, 0
  %229 = extractvalue { ptr, ptr, i64 } %222, 1
  %230 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %228, 0
  %231 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %230, ptr %229, 1
  %232 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %231, i64 320, 2
  %233 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %232, i64 16, 3, 0
  %234 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %233, i64 20, 4, 0
  %235 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %234, i64 8, 3, 1
  %236 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %235, i64 1, 4, 1
  %237 = call ptr @llvm.stacksave.p0()
  %238 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %236, ptr %238, align 8
  %239 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %238, 1
  %240 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %180, ptr %240, align 8
  %241 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %240, 1
  %242 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %239, ptr %242, align 8
  %243 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %241, ptr %243, align 8
  call void @memrefCopy(i64 4, ptr %242, ptr %243)
  call void @llvm.stackrestore.p0(ptr %237)
  %244 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %152, ptr %190, ptr %114, i32 0, i32 42, i32 125)
  %245 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %246 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %247 = insertvalue { ptr, ptr, i64 } poison, ptr %245, 0
  %248 = insertvalue { ptr, ptr, i64 } %247, ptr %246, 1
  %249 = insertvalue { ptr, ptr, i64 } %248, i64 0, 2
  %250 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %251 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %252 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %253 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %254 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %255 = extractvalue { ptr, ptr, i64 } %249, 0
  %256 = extractvalue { ptr, ptr, i64 } %249, 1
  %257 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %255, 0
  %258 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %257, ptr %256, 1
  %259 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %258, i64 32, 2
  %260 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %259, i64 8, 3, 0
  %261 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %260, i64 40, 4, 0
  %262 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %261, i64 8, 3, 1
  %263 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %262, i64 1, 4, 1
  %264 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 0
  %265 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %266 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %264, 0
  %267 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %266, ptr %265, 1
  %268 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %267, i64 0, 2
  %269 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %268, i64 8, 3, 0
  %270 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %269, i64 8, 4, 0
  %271 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %270, i64 8, 3, 1
  %272 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %271, i64 1, 4, 1
  %273 = call ptr @llvm.stacksave.p0()
  %274 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %263, ptr %274, align 8
  %275 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %274, 1
  %276 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %272, ptr %276, align 8
  %277 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %276, 1
  %278 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %275, ptr %278, align 8
  %279 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %277, ptr %279, align 8
  call void @memrefCopy(i64 4, ptr %278, ptr %279)
  call void @llvm.stackrestore.p0(ptr %273)
  %280 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %281 = ptrtoint ptr %280 to i64
  %282 = inttoptr i64 %281 to ptr
  %283 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %284 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %285 = insertvalue { ptr, ptr, i64 } poison, ptr %283, 0
  %286 = insertvalue { ptr, ptr, i64 } %285, ptr %284, 1
  %287 = insertvalue { ptr, ptr, i64 } %286, i64 0, 2
  %288 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %289 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %290 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %291 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %292 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %293 = extractvalue { ptr, ptr, i64 } %287, 0
  %294 = extractvalue { ptr, ptr, i64 } %287, 1
  %295 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %293, 0
  %296 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %295, ptr %294, 1
  %297 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %296, i64 640, 2
  %298 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %297, i64 8, 3, 0
  %299 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %298, i64 20, 4, 0
  %300 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %299, i64 8, 3, 1
  %301 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %300, i64 1, 4, 1
  %302 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 0
  %303 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %304 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %302, 0
  %305 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %304, ptr %303, 1
  %306 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %305, i64 0, 2
  %307 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %306, i64 8, 3, 0
  %308 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %307, i64 8, 4, 0
  %309 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %308, i64 8, 3, 1
  %310 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %309, i64 1, 4, 1
  %311 = call ptr @llvm.stacksave.p0()
  %312 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %301, ptr %312, align 8
  %313 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %312, 1
  %314 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %310, ptr %314, align 8
  %315 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %314, 1
  %316 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %313, ptr %316, align 8
  %317 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %315, ptr %317, align 8
  call void @memrefCopy(i64 4, ptr %316, ptr %317)
  call void @llvm.stackrestore.p0(ptr %311)
  %318 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %319 = ptrtoint ptr %318 to i64
  %320 = inttoptr i64 %319 to ptr
  %321 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 8, i32 8, ptr %282, ptr %320, ptr %114, i32 0, i32 128, i32 117)
  %322 = call ptr @llvm.stacksave.p0()
  %323 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %104, ptr %323, align 8
  %324 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %323, 1
  %325 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %95, ptr %325, align 8
  %326 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %325, 1
  %327 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %324, ptr %327, align 8
  %328 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %326, ptr %328, align 8
  call void @memrefCopy(i64 4, ptr %327, ptr %328)
  call void @llvm.stackrestore.p0(ptr %322)
  %329 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %330 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %331 = insertvalue { ptr, ptr, i64 } poison, ptr %329, 0
  %332 = insertvalue { ptr, ptr, i64 } %331, ptr %330, 1
  %333 = insertvalue { ptr, ptr, i64 } %332, i64 0, 2
  %334 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %335 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %336 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %337 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %338 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %339 = extractvalue { ptr, ptr, i64 } %333, 0
  %340 = extractvalue { ptr, ptr, i64 } %333, 1
  %341 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %339, 0
  %342 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %341, ptr %340, 1
  %343 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %342, i64 8, 2
  %344 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %343, i64 8, 3, 0
  %345 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %344, i64 20, 4, 0
  %346 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %345, i64 8, 3, 1
  %347 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %346, i64 1, 4, 1
  %348 = call ptr @llvm.stacksave.p0()
  %349 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %347, ptr %349, align 8
  %350 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %349, 1
  %351 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %104, ptr %351, align 8
  %352 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %351, 1
  %353 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %350, ptr %353, align 8
  %354 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %352, ptr %354, align 8
  call void @memrefCopy(i64 4, ptr %353, ptr %354)
  call void @llvm.stackrestore.p0(ptr %348)
  %355 = call ptr @llvm.stacksave.p0()
  %356 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %133, ptr %356, align 8
  %357 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %356, 1
  %358 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %142, ptr %358, align 8
  %359 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %358, 1
  %360 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %357, ptr %360, align 8
  %361 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %359, ptr %361, align 8
  call void @memrefCopy(i64 4, ptr %360, ptr %361)
  call void @llvm.stackrestore.p0(ptr %355)
  %362 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %363 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %364 = insertvalue { ptr, ptr, i64 } poison, ptr %362, 0
  %365 = insertvalue { ptr, ptr, i64 } %364, ptr %363, 1
  %366 = insertvalue { ptr, ptr, i64 } %365, i64 0, 2
  %367 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %368 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %369 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %370 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %371 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %372 = extractvalue { ptr, ptr, i64 } %366, 0
  %373 = extractvalue { ptr, ptr, i64 } %366, 1
  %374 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %372, 0
  %375 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %374, ptr %373, 1
  %376 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %375, i64 8, 2
  %377 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %376, i64 16, 3, 0
  %378 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %377, i64 20, 4, 0
  %379 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %378, i64 8, 3, 1
  %380 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %379, i64 1, 4, 1
  %381 = call ptr @llvm.stacksave.p0()
  %382 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %380, ptr %382, align 8
  %383 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %382, 1
  %384 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %180, ptr %384, align 8
  %385 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %384, 1
  %386 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %383, ptr %386, align 8
  %387 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %385, ptr %387, align 8
  call void @memrefCopy(i64 4, ptr %386, ptr %387)
  call void @llvm.stackrestore.p0(ptr %381)
  %388 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %152, ptr %190, ptr %114, i32 0, i32 197, i32 125)
  %389 = call ptr @llvm.stacksave.p0()
  %390 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %210, ptr %390, align 8
  %391 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %390, 1
  %392 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %142, ptr %392, align 8
  %393 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %392, 1
  %394 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %391, ptr %394, align 8
  %395 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %393, ptr %395, align 8
  call void @memrefCopy(i64 4, ptr %394, ptr %395)
  call void @llvm.stackrestore.p0(ptr %389)
  %396 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %397 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %398 = insertvalue { ptr, ptr, i64 } poison, ptr %396, 0
  %399 = insertvalue { ptr, ptr, i64 } %398, ptr %397, 1
  %400 = insertvalue { ptr, ptr, i64 } %399, i64 0, 2
  %401 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %402 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %403 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %404 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %405 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %406 = extractvalue { ptr, ptr, i64 } %400, 0
  %407 = extractvalue { ptr, ptr, i64 } %400, 1
  %408 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %406, 0
  %409 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %408, ptr %407, 1
  %410 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %409, i64 328, 2
  %411 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %410, i64 16, 3, 0
  %412 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %411, i64 20, 4, 0
  %413 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %412, i64 8, 3, 1
  %414 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %413, i64 1, 4, 1
  %415 = call ptr @llvm.stacksave.p0()
  %416 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %414, ptr %416, align 8
  %417 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %416, 1
  %418 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %180, ptr %418, align 8
  %419 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %418, 1
  %420 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %417, ptr %420, align 8
  %421 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %419, ptr %421, align 8
  call void @memrefCopy(i64 4, ptr %420, ptr %421)
  call void @llvm.stackrestore.p0(ptr %415)
  %422 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %152, ptr %190, ptr %114, i32 0, i32 266, i32 125)
  %423 = call ptr @llvm.stacksave.p0()
  %424 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %263, ptr %424, align 8
  %425 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %424, 1
  %426 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %272, ptr %426, align 8
  %427 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %426, 1
  %428 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %425, ptr %428, align 8
  %429 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %427, ptr %429, align 8
  call void @memrefCopy(i64 4, ptr %428, ptr %429)
  call void @llvm.stackrestore.p0(ptr %423)
  %430 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %431 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %432 = insertvalue { ptr, ptr, i64 } poison, ptr %430, 0
  %433 = insertvalue { ptr, ptr, i64 } %432, ptr %431, 1
  %434 = insertvalue { ptr, ptr, i64 } %433, i64 0, 2
  %435 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %436 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %437 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %438 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %439 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %440 = extractvalue { ptr, ptr, i64 } %434, 0
  %441 = extractvalue { ptr, ptr, i64 } %434, 1
  %442 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %440, 0
  %443 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %442, ptr %441, 1
  %444 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %443, i64 648, 2
  %445 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %444, i64 8, 3, 0
  %446 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %445, i64 20, 4, 0
  %447 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %446, i64 8, 3, 1
  %448 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %447, i64 1, 4, 1
  %449 = call ptr @llvm.stacksave.p0()
  %450 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %448, ptr %450, align 8
  %451 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %450, 1
  %452 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %310, ptr %452, align 8
  %453 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %452, 1
  %454 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %451, ptr %454, align 8
  %455 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %453, ptr %455, align 8
  call void @memrefCopy(i64 4, ptr %454, ptr %455)
  call void @llvm.stackrestore.p0(ptr %449)
  %456 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 8, i32 8, ptr %282, ptr %320, ptr %114, i32 0, i32 335, i32 117)
  %457 = call ptr @llvm.stacksave.p0()
  %458 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %104, ptr %458, align 8
  %459 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %458, 1
  %460 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %347, ptr %460, align 8
  %461 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %460, 1
  %462 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %459, ptr %462, align 8
  %463 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %461, ptr %463, align 8
  call void @memrefCopy(i64 4, ptr %462, ptr %463)
  call void @llvm.stackrestore.p0(ptr %457)
  %464 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %465 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %466 = insertvalue { ptr, ptr, i64 } poison, ptr %464, 0
  %467 = insertvalue { ptr, ptr, i64 } %466, ptr %465, 1
  %468 = insertvalue { ptr, ptr, i64 } %467, i64 0, 2
  %469 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %470 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %471 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %472 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %473 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %474 = extractvalue { ptr, ptr, i64 } %468, 0
  %475 = extractvalue { ptr, ptr, i64 } %468, 1
  %476 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %474, 0
  %477 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %476, ptr %475, 1
  %478 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %477, i64 16, 2
  %479 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %478, i64 4, 3, 0
  %480 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %479, i64 20, 4, 0
  %481 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %480, i64 4, 3, 1
  %482 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %481, i64 1, 4, 1
  %483 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 0
  %484 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 1
  %485 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %483, 0
  %486 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %485, ptr %484, 1
  %487 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %486, i64 0, 2
  %488 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %487, i64 4, 3, 0
  %489 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %488, i64 4, 4, 0
  %490 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %489, i64 4, 3, 1
  %491 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %490, i64 1, 4, 1
  %492 = call ptr @llvm.stacksave.p0()
  %493 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %482, ptr %493, align 8
  %494 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %493, 1
  %495 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %491, ptr %495, align 8
  %496 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %495, 1
  %497 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %494, ptr %497, align 8
  %498 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %496, ptr %498, align 8
  call void @memrefCopy(i64 4, ptr %497, ptr %498)
  call void @llvm.stackrestore.p0(ptr %492)
  %499 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 1
  %500 = ptrtoint ptr %499 to i64
  %501 = inttoptr i64 %500 to ptr
  %502 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %503 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %504 = insertvalue { ptr, ptr, i64 } poison, ptr %502, 0
  %505 = insertvalue { ptr, ptr, i64 } %504, ptr %503, 1
  %506 = insertvalue { ptr, ptr, i64 } %505, i64 0, 2
  %507 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %508 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %509 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %510 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %511 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %512 = extractvalue { ptr, ptr, i64 } %506, 0
  %513 = extractvalue { ptr, ptr, i64 } %506, 1
  %514 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %512, 0
  %515 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %514, ptr %513, 1
  %516 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %515, i64 0, 2
  %517 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %516, i64 4, 3, 0
  %518 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %517, i64 40, 4, 0
  %519 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %518, i64 16, 3, 1
  %520 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %519, i64 1, 4, 1
  %521 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 0
  %522 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %523 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %521, 0
  %524 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %523, ptr %522, 1
  %525 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %524, i64 0, 2
  %526 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %525, i64 4, 3, 0
  %527 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %526, i64 16, 4, 0
  %528 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %527, i64 16, 3, 1
  %529 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %528, i64 1, 4, 1
  %530 = call ptr @llvm.stacksave.p0()
  %531 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %520, ptr %531, align 8
  %532 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %531, 1
  %533 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %529, ptr %533, align 8
  %534 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %533, 1
  %535 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %532, ptr %535, align 8
  %536 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %534, ptr %536, align 8
  call void @memrefCopy(i64 4, ptr %535, ptr %536)
  call void @llvm.stackrestore.p0(ptr %530)
  %537 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %538 = ptrtoint ptr %537 to i64
  %539 = inttoptr i64 %538 to ptr
  %540 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %541 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %542 = insertvalue { ptr, ptr, i64 } poison, ptr %540, 0
  %543 = insertvalue { ptr, ptr, i64 } %542, ptr %541, 1
  %544 = insertvalue { ptr, ptr, i64 } %543, i64 0, 2
  %545 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %546 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %547 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %548 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %549 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %550 = extractvalue { ptr, ptr, i64 } %544, 0
  %551 = extractvalue { ptr, ptr, i64 } %544, 1
  %552 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %550, 0
  %553 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %552, ptr %551, 1
  %554 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %553, i64 16, 2
  %555 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %554, i64 16, 3, 0
  %556 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %555, i64 20, 4, 0
  %557 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %556, i64 4, 3, 1
  %558 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %557, i64 1, 4, 1
  %559 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 0
  %560 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %561 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %559, 0
  %562 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %561, ptr %560, 1
  %563 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %562, i64 0, 2
  %564 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %563, i64 16, 3, 0
  %565 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %564, i64 4, 4, 0
  %566 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %565, i64 4, 3, 1
  %567 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %566, i64 1, 4, 1
  %568 = call ptr @llvm.stacksave.p0()
  %569 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %558, ptr %569, align 8
  %570 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %569, 1
  %571 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %567, ptr %571, align 8
  %572 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %571, 1
  %573 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %570, ptr %573, align 8
  %574 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %572, ptr %574, align 8
  call void @memrefCopy(i64 4, ptr %573, ptr %574)
  call void @llvm.stackrestore.p0(ptr %568)
  %575 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %576 = ptrtoint ptr %575 to i64
  %577 = inttoptr i64 %576 to ptr
  %578 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %539, ptr %577, ptr %501, i32 1, i32 0, i32 117)
  %579 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %580 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %581 = insertvalue { ptr, ptr, i64 } poison, ptr %579, 0
  %582 = insertvalue { ptr, ptr, i64 } %581, ptr %580, 1
  %583 = insertvalue { ptr, ptr, i64 } %582, i64 0, 2
  %584 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %585 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %586 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %587 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %588 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %589 = extractvalue { ptr, ptr, i64 } %583, 0
  %590 = extractvalue { ptr, ptr, i64 } %583, 1
  %591 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %589, 0
  %592 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %591, ptr %590, 1
  %593 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %592, i64 16, 2
  %594 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %593, i64 4, 3, 0
  %595 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %594, i64 40, 4, 0
  %596 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %595, i64 16, 3, 1
  %597 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %596, i64 1, 4, 1
  %598 = call ptr @llvm.stacksave.p0()
  %599 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %597, ptr %599, align 8
  %600 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %599, 1
  %601 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %529, ptr %601, align 8
  %602 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %601, 1
  %603 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %600, ptr %603, align 8
  %604 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %602, ptr %604, align 8
  call void @memrefCopy(i64 4, ptr %603, ptr %604)
  call void @llvm.stackrestore.p0(ptr %598)
  %605 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %606 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %607 = insertvalue { ptr, ptr, i64 } poison, ptr %605, 0
  %608 = insertvalue { ptr, ptr, i64 } %607, ptr %606, 1
  %609 = insertvalue { ptr, ptr, i64 } %608, i64 0, 2
  %610 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %611 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %612 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %613 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %614 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %615 = extractvalue { ptr, ptr, i64 } %609, 0
  %616 = extractvalue { ptr, ptr, i64 } %609, 1
  %617 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %615, 0
  %618 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %617, ptr %616, 1
  %619 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %618, i64 336, 2
  %620 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %619, i64 16, 3, 0
  %621 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %620, i64 20, 4, 0
  %622 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %621, i64 4, 3, 1
  %623 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %622, i64 1, 4, 1
  %624 = call ptr @llvm.stacksave.p0()
  %625 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %623, ptr %625, align 8
  %626 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %625, 1
  %627 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %567, ptr %627, align 8
  %628 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %627, 1
  %629 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %626, ptr %629, align 8
  %630 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %628, ptr %630, align 8
  call void @memrefCopy(i64 4, ptr %629, ptr %630)
  call void @llvm.stackrestore.p0(ptr %624)
  %631 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %539, ptr %577, ptr %501, i32 1, i32 42, i32 117)
  %632 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %633 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %634 = insertvalue { ptr, ptr, i64 } poison, ptr %632, 0
  %635 = insertvalue { ptr, ptr, i64 } %634, ptr %633, 1
  %636 = insertvalue { ptr, ptr, i64 } %635, i64 0, 2
  %637 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %638 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %639 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %640 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %641 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %642 = extractvalue { ptr, ptr, i64 } %636, 0
  %643 = extractvalue { ptr, ptr, i64 } %636, 1
  %644 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %642, 0
  %645 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %644, ptr %643, 1
  %646 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %645, i64 32, 2
  %647 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %646, i64 4, 3, 0
  %648 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %647, i64 40, 4, 0
  %649 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %648, i64 8, 3, 1
  %650 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %649, i64 1, 4, 1
  %651 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 0
  %652 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %653 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %651, 0
  %654 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %653, ptr %652, 1
  %655 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %654, i64 0, 2
  %656 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %655, i64 4, 3, 0
  %657 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %656, i64 8, 4, 0
  %658 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %657, i64 8, 3, 1
  %659 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %658, i64 1, 4, 1
  %660 = call ptr @llvm.stacksave.p0()
  %661 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %650, ptr %661, align 8
  %662 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %661, 1
  %663 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %659, ptr %663, align 8
  %664 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %663, 1
  %665 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %662, ptr %665, align 8
  %666 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %664, ptr %666, align 8
  call void @memrefCopy(i64 4, ptr %665, ptr %666)
  call void @llvm.stackrestore.p0(ptr %660)
  %667 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %668 = ptrtoint ptr %667 to i64
  %669 = inttoptr i64 %668 to ptr
  %670 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %671 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %672 = insertvalue { ptr, ptr, i64 } poison, ptr %670, 0
  %673 = insertvalue { ptr, ptr, i64 } %672, ptr %671, 1
  %674 = insertvalue { ptr, ptr, i64 } %673, i64 0, 2
  %675 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %676 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %677 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %678 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %679 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %680 = extractvalue { ptr, ptr, i64 } %674, 0
  %681 = extractvalue { ptr, ptr, i64 } %674, 1
  %682 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %680, 0
  %683 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %682, ptr %681, 1
  %684 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %683, i64 656, 2
  %685 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %684, i64 8, 3, 0
  %686 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %685, i64 20, 4, 0
  %687 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %686, i64 4, 3, 1
  %688 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %687, i64 1, 4, 1
  %689 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 0
  %690 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %691 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %689, 0
  %692 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %691, ptr %690, 1
  %693 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %692, i64 0, 2
  %694 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %693, i64 8, 3, 0
  %695 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %694, i64 4, 4, 0
  %696 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %695, i64 4, 3, 1
  %697 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %696, i64 1, 4, 1
  %698 = call ptr @llvm.stacksave.p0()
  %699 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %688, ptr %699, align 8
  %700 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %699, 1
  %701 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %697, ptr %701, align 8
  %702 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %701, 1
  %703 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %700, ptr %703, align 8
  %704 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %702, ptr %704, align 8
  call void @memrefCopy(i64 4, ptr %703, ptr %704)
  call void @llvm.stackrestore.p0(ptr %698)
  %705 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %706 = ptrtoint ptr %705 to i64
  %707 = inttoptr i64 %706 to ptr
  %708 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 8, i32 4, ptr %669, ptr %707, ptr %501, i32 1, i32 120, i32 109)
  %709 = call ptr @llvm.stacksave.p0()
  %710 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %491, ptr %710, align 8
  %711 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %710, 1
  %712 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %482, ptr %712, align 8
  %713 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %712, 1
  %714 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %711, ptr %714, align 8
  %715 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %713, ptr %715, align 8
  call void @memrefCopy(i64 4, ptr %714, ptr %715)
  call void @llvm.stackrestore.p0(ptr %709)
  %716 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %717 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %718 = insertvalue { ptr, ptr, i64 } poison, ptr %716, 0
  %719 = insertvalue { ptr, ptr, i64 } %718, ptr %717, 1
  %720 = insertvalue { ptr, ptr, i64 } %719, i64 0, 2
  %721 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %722 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %723 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %724 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %725 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %726 = extractvalue { ptr, ptr, i64 } %720, 0
  %727 = extractvalue { ptr, ptr, i64 } %720, 1
  %728 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %726, 0
  %729 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %728, ptr %727, 1
  %730 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %729, i64 96, 2
  %731 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %730, i64 4, 3, 0
  %732 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %731, i64 20, 4, 0
  %733 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %732, i64 4, 3, 1
  %734 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %733, i64 1, 4, 1
  %735 = call ptr @llvm.stacksave.p0()
  %736 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %734, ptr %736, align 8
  %737 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %736, 1
  %738 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %491, ptr %738, align 8
  %739 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %738, 1
  %740 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %737, ptr %740, align 8
  %741 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %739, ptr %741, align 8
  call void @memrefCopy(i64 4, ptr %740, ptr %741)
  call void @llvm.stackrestore.p0(ptr %735)
  %742 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %743 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %744 = insertvalue { ptr, ptr, i64 } poison, ptr %742, 0
  %745 = insertvalue { ptr, ptr, i64 } %744, ptr %743, 1
  %746 = insertvalue { ptr, ptr, i64 } %745, i64 0, 2
  %747 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %748 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %749 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %750 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %751 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %752 = extractvalue { ptr, ptr, i64 } %746, 0
  %753 = extractvalue { ptr, ptr, i64 } %746, 1
  %754 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %752, 0
  %755 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %754, ptr %753, 1
  %756 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %755, i64 160, 2
  %757 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %756, i64 4, 3, 0
  %758 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %757, i64 40, 4, 0
  %759 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %758, i64 16, 3, 1
  %760 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %759, i64 1, 4, 1
  %761 = call ptr @llvm.stacksave.p0()
  %762 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %760, ptr %762, align 8
  %763 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %762, 1
  %764 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %529, ptr %764, align 8
  %765 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %764, 1
  %766 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %763, ptr %766, align 8
  %767 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %765, ptr %767, align 8
  call void @memrefCopy(i64 4, ptr %766, ptr %767)
  call void @llvm.stackrestore.p0(ptr %761)
  %768 = call ptr @llvm.stacksave.p0()
  %769 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %558, ptr %769, align 8
  %770 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %769, 1
  %771 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %567, ptr %771, align 8
  %772 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %771, 1
  %773 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %770, ptr %773, align 8
  %774 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %772, ptr %774, align 8
  call void @memrefCopy(i64 4, ptr %773, ptr %774)
  call void @llvm.stackrestore.p0(ptr %768)
  %775 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %539, ptr %577, ptr %501, i32 2, i32 0, i32 117)
  %776 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %777 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %778 = insertvalue { ptr, ptr, i64 } poison, ptr %776, 0
  %779 = insertvalue { ptr, ptr, i64 } %778, ptr %777, 1
  %780 = insertvalue { ptr, ptr, i64 } %779, i64 0, 2
  %781 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %782 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %783 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %784 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %785 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %786 = extractvalue { ptr, ptr, i64 } %780, 0
  %787 = extractvalue { ptr, ptr, i64 } %780, 1
  %788 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %786, 0
  %789 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %788, ptr %787, 1
  %790 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %789, i64 176, 2
  %791 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %790, i64 4, 3, 0
  %792 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %791, i64 40, 4, 0
  %793 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %792, i64 16, 3, 1
  %794 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %793, i64 1, 4, 1
  %795 = call ptr @llvm.stacksave.p0()
  %796 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %794, ptr %796, align 8
  %797 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %796, 1
  %798 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %529, ptr %798, align 8
  %799 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %798, 1
  %800 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %797, ptr %800, align 8
  %801 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %799, ptr %801, align 8
  call void @memrefCopy(i64 4, ptr %800, ptr %801)
  call void @llvm.stackrestore.p0(ptr %795)
  %802 = call ptr @llvm.stacksave.p0()
  %803 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %623, ptr %803, align 8
  %804 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %803, 1
  %805 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %567, ptr %805, align 8
  %806 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %805, 1
  %807 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %804, ptr %807, align 8
  %808 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %806, ptr %808, align 8
  call void @memrefCopy(i64 4, ptr %807, ptr %808)
  call void @llvm.stackrestore.p0(ptr %802)
  %809 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %539, ptr %577, ptr %501, i32 2, i32 42, i32 117)
  %810 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %811 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %812 = insertvalue { ptr, ptr, i64 } poison, ptr %810, 0
  %813 = insertvalue { ptr, ptr, i64 } %812, ptr %811, 1
  %814 = insertvalue { ptr, ptr, i64 } %813, i64 0, 2
  %815 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %816 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %817 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %818 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %819 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %820 = extractvalue { ptr, ptr, i64 } %814, 0
  %821 = extractvalue { ptr, ptr, i64 } %814, 1
  %822 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %820, 0
  %823 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %822, ptr %821, 1
  %824 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %823, i64 192, 2
  %825 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %824, i64 4, 3, 0
  %826 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %825, i64 40, 4, 0
  %827 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %826, i64 8, 3, 1
  %828 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %827, i64 1, 4, 1
  %829 = call ptr @llvm.stacksave.p0()
  %830 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %828, ptr %830, align 8
  %831 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %830, 1
  %832 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %659, ptr %832, align 8
  %833 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %832, 1
  %834 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %831, ptr %834, align 8
  %835 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %833, ptr %835, align 8
  call void @memrefCopy(i64 4, ptr %834, ptr %835)
  call void @llvm.stackrestore.p0(ptr %829)
  %836 = call ptr @llvm.stacksave.p0()
  %837 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %688, ptr %837, align 8
  %838 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %837, 1
  %839 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %697, ptr %839, align 8
  %840 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %839, 1
  %841 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %838, ptr %841, align 8
  %842 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %840, ptr %842, align 8
  call void @memrefCopy(i64 4, ptr %841, ptr %842)
  call void @llvm.stackrestore.p0(ptr %836)
  %843 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 8, i32 4, ptr %669, ptr %707, ptr %501, i32 2, i32 120, i32 109)
  %844 = call ptr @llvm.stacksave.p0()
  %845 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %491, ptr %845, align 8
  %846 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %845, 1
  %847 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %734, ptr %847, align 8
  %848 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %847, 1
  %849 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %846, ptr %849, align 8
  %850 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %848, ptr %850, align 8
  call void @memrefCopy(i64 4, ptr %849, ptr %850)
  call void @llvm.stackrestore.p0(ptr %844)
  %851 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %852 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %853 = insertvalue { ptr, ptr, i64 } poison, ptr %851, 0
  %854 = insertvalue { ptr, ptr, i64 } %853, ptr %852, 1
  %855 = insertvalue { ptr, ptr, i64 } %854, i64 0, 2
  %856 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %857 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %858 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %859 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %860 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %861 = extractvalue { ptr, ptr, i64 } %855, 0
  %862 = extractvalue { ptr, ptr, i64 } %855, 1
  %863 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %861, 0
  %864 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %863, ptr %862, 1
  %865 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %864, i64 160, 2
  %866 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %865, i64 8, 3, 0
  %867 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %866, i64 20, 4, 0
  %868 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %867, i64 8, 3, 1
  %869 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %868, i64 1, 4, 1
  %870 = call ptr @llvm.stacksave.p0()
  %871 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %869, ptr %871, align 8
  %872 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %871, 1
  %873 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %104, ptr %873, align 8
  %874 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %873, 1
  %875 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %872, ptr %875, align 8
  %876 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %874, ptr %876, align 8
  call void @memrefCopy(i64 4, ptr %875, ptr %876)
  call void @llvm.stackrestore.p0(ptr %870)
  %877 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %878 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %879 = insertvalue { ptr, ptr, i64 } poison, ptr %877, 0
  %880 = insertvalue { ptr, ptr, i64 } %879, ptr %878, 1
  %881 = insertvalue { ptr, ptr, i64 } %880, i64 0, 2
  %882 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %883 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %884 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %885 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %886 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %887 = extractvalue { ptr, ptr, i64 } %881, 0
  %888 = extractvalue { ptr, ptr, i64 } %881, 1
  %889 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %887, 0
  %890 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %889, ptr %888, 1
  %891 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %890, i64 320, 2
  %892 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %891, i64 8, 3, 0
  %893 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %892, i64 40, 4, 0
  %894 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %893, i64 16, 3, 1
  %895 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %894, i64 1, 4, 1
  %896 = call ptr @llvm.stacksave.p0()
  %897 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %895, ptr %897, align 8
  %898 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %897, 1
  %899 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %142, ptr %899, align 8
  %900 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %899, 1
  %901 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %898, ptr %901, align 8
  %902 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %900, ptr %902, align 8
  call void @memrefCopy(i64 4, ptr %901, ptr %902)
  call void @llvm.stackrestore.p0(ptr %896)
  %903 = call ptr @llvm.stacksave.p0()
  %904 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %171, ptr %904, align 8
  %905 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %904, 1
  %906 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %180, ptr %906, align 8
  %907 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %906, 1
  %908 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %905, ptr %908, align 8
  %909 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %907, ptr %909, align 8
  call void @memrefCopy(i64 4, ptr %908, ptr %909)
  call void @llvm.stackrestore.p0(ptr %903)
  %910 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %152, ptr %190, ptr %114, i32 0, i32 404, i32 125)
  %911 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %912 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %913 = insertvalue { ptr, ptr, i64 } poison, ptr %911, 0
  %914 = insertvalue { ptr, ptr, i64 } %913, ptr %912, 1
  %915 = insertvalue { ptr, ptr, i64 } %914, i64 0, 2
  %916 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %917 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %918 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %919 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %920 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %921 = extractvalue { ptr, ptr, i64 } %915, 0
  %922 = extractvalue { ptr, ptr, i64 } %915, 1
  %923 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %921, 0
  %924 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %923, ptr %922, 1
  %925 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %924, i64 336, 2
  %926 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %925, i64 8, 3, 0
  %927 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %926, i64 40, 4, 0
  %928 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %927, i64 16, 3, 1
  %929 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %928, i64 1, 4, 1
  %930 = call ptr @llvm.stacksave.p0()
  %931 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %929, ptr %931, align 8
  %932 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %931, 1
  %933 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %142, ptr %933, align 8
  %934 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %933, 1
  %935 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %932, ptr %935, align 8
  %936 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %934, ptr %936, align 8
  call void @memrefCopy(i64 4, ptr %935, ptr %936)
  call void @llvm.stackrestore.p0(ptr %930)
  %937 = call ptr @llvm.stacksave.p0()
  %938 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %236, ptr %938, align 8
  %939 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %938, 1
  %940 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %180, ptr %940, align 8
  %941 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %940, 1
  %942 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %939, ptr %942, align 8
  %943 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %941, ptr %943, align 8
  call void @memrefCopy(i64 4, ptr %942, ptr %943)
  call void @llvm.stackrestore.p0(ptr %937)
  %944 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %152, ptr %190, ptr %114, i32 0, i32 473, i32 125)
  %945 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %946 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %947 = insertvalue { ptr, ptr, i64 } poison, ptr %945, 0
  %948 = insertvalue { ptr, ptr, i64 } %947, ptr %946, 1
  %949 = insertvalue { ptr, ptr, i64 } %948, i64 0, 2
  %950 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %951 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %952 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %953 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %954 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %955 = extractvalue { ptr, ptr, i64 } %949, 0
  %956 = extractvalue { ptr, ptr, i64 } %949, 1
  %957 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %955, 0
  %958 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %957, ptr %956, 1
  %959 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %958, i64 352, 2
  %960 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %959, i64 8, 3, 0
  %961 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %960, i64 40, 4, 0
  %962 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %961, i64 8, 3, 1
  %963 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %962, i64 1, 4, 1
  %964 = call ptr @llvm.stacksave.p0()
  %965 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %963, ptr %965, align 8
  %966 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %965, 1
  %967 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %272, ptr %967, align 8
  %968 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %967, 1
  %969 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %966, ptr %969, align 8
  %970 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %968, ptr %970, align 8
  call void @memrefCopy(i64 4, ptr %969, ptr %970)
  call void @llvm.stackrestore.p0(ptr %964)
  %971 = call ptr @llvm.stacksave.p0()
  %972 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %301, ptr %972, align 8
  %973 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %972, 1
  %974 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %310, ptr %974, align 8
  %975 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %974, 1
  %976 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %973, ptr %976, align 8
  %977 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %975, ptr %977, align 8
  call void @memrefCopy(i64 4, ptr %976, ptr %977)
  call void @llvm.stackrestore.p0(ptr %971)
  %978 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 8, i32 8, ptr %282, ptr %320, ptr %114, i32 0, i32 542, i32 117)
  %979 = call ptr @llvm.stacksave.p0()
  %980 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %104, ptr %980, align 8
  %981 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %980, 1
  %982 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %869, ptr %982, align 8
  %983 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %982, 1
  %984 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %981, ptr %984, align 8
  %985 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %983, ptr %985, align 8
  call void @memrefCopy(i64 4, ptr %984, ptr %985)
  call void @llvm.stackrestore.p0(ptr %979)
  %986 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %987 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %988 = insertvalue { ptr, ptr, i64 } poison, ptr %986, 0
  %989 = insertvalue { ptr, ptr, i64 } %988, ptr %987, 1
  %990 = insertvalue { ptr, ptr, i64 } %989, i64 0, 2
  %991 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %992 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %993 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %994 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %995 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %996 = extractvalue { ptr, ptr, i64 } %990, 0
  %997 = extractvalue { ptr, ptr, i64 } %990, 1
  %998 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %996, 0
  %999 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %998, ptr %997, 1
  %1000 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %999, i64 168, 2
  %1001 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1000, i64 8, 3, 0
  %1002 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1001, i64 20, 4, 0
  %1003 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1002, i64 8, 3, 1
  %1004 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1003, i64 1, 4, 1
  %1005 = call ptr @llvm.stacksave.p0()
  %1006 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1004, ptr %1006, align 8
  %1007 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1006, 1
  %1008 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %104, ptr %1008, align 8
  %1009 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1008, 1
  %1010 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1007, ptr %1010, align 8
  %1011 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1009, ptr %1011, align 8
  call void @memrefCopy(i64 4, ptr %1010, ptr %1011)
  call void @llvm.stackrestore.p0(ptr %1005)
  %1012 = call ptr @llvm.stacksave.p0()
  %1013 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %895, ptr %1013, align 8
  %1014 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1013, 1
  %1015 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %142, ptr %1015, align 8
  %1016 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1015, 1
  %1017 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1014, ptr %1017, align 8
  %1018 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1016, ptr %1018, align 8
  call void @memrefCopy(i64 4, ptr %1017, ptr %1018)
  call void @llvm.stackrestore.p0(ptr %1012)
  %1019 = call ptr @llvm.stacksave.p0()
  %1020 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %380, ptr %1020, align 8
  %1021 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1020, 1
  %1022 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %180, ptr %1022, align 8
  %1023 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1022, 1
  %1024 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1021, ptr %1024, align 8
  %1025 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1023, ptr %1025, align 8
  call void @memrefCopy(i64 4, ptr %1024, ptr %1025)
  call void @llvm.stackrestore.p0(ptr %1019)
  %1026 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %152, ptr %190, ptr %114, i32 0, i32 611, i32 125)
  %1027 = call ptr @llvm.stacksave.p0()
  %1028 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %929, ptr %1028, align 8
  %1029 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1028, 1
  %1030 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %142, ptr %1030, align 8
  %1031 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1030, 1
  %1032 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1029, ptr %1032, align 8
  %1033 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1031, ptr %1033, align 8
  call void @memrefCopy(i64 4, ptr %1032, ptr %1033)
  call void @llvm.stackrestore.p0(ptr %1027)
  %1034 = call ptr @llvm.stacksave.p0()
  %1035 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %414, ptr %1035, align 8
  %1036 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1035, 1
  %1037 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %180, ptr %1037, align 8
  %1038 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1037, 1
  %1039 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1036, ptr %1039, align 8
  %1040 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1038, ptr %1040, align 8
  call void @memrefCopy(i64 4, ptr %1039, ptr %1040)
  call void @llvm.stackrestore.p0(ptr %1034)
  %1041 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %152, ptr %190, ptr %114, i32 0, i32 680, i32 125)
  %1042 = call ptr @llvm.stacksave.p0()
  %1043 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %963, ptr %1043, align 8
  %1044 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1043, 1
  %1045 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %272, ptr %1045, align 8
  %1046 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1045, 1
  %1047 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1044, ptr %1047, align 8
  %1048 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1046, ptr %1048, align 8
  call void @memrefCopy(i64 4, ptr %1047, ptr %1048)
  call void @llvm.stackrestore.p0(ptr %1042)
  %1049 = call ptr @llvm.stacksave.p0()
  %1050 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %448, ptr %1050, align 8
  %1051 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1050, 1
  %1052 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %310, ptr %1052, align 8
  %1053 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1052, 1
  %1054 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1051, ptr %1054, align 8
  %1055 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1053, ptr %1055, align 8
  call void @memrefCopy(i64 4, ptr %1054, ptr %1055)
  call void @llvm.stackrestore.p0(ptr %1049)
  %1056 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 8, i32 8, ptr %282, ptr %320, ptr %114, i32 0, i32 749, i32 117)
  %1057 = call ptr @llvm.stacksave.p0()
  %1058 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %104, ptr %1058, align 8
  %1059 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1058, 1
  %1060 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1004, ptr %1060, align 8
  %1061 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1060, 1
  %1062 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1059, ptr %1062, align 8
  %1063 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1061, ptr %1063, align 8
  call void @memrefCopy(i64 4, ptr %1062, ptr %1063)
  call void @llvm.stackrestore.p0(ptr %1057)
  %1064 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %1065 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %1066 = insertvalue { ptr, ptr, i64 } poison, ptr %1064, 0
  %1067 = insertvalue { ptr, ptr, i64 } %1066, ptr %1065, 1
  %1068 = insertvalue { ptr, ptr, i64 } %1067, i64 0, 2
  %1069 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %1070 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %1071 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %1072 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %1073 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %1074 = extractvalue { ptr, ptr, i64 } %1068, 0
  %1075 = extractvalue { ptr, ptr, i64 } %1068, 1
  %1076 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1074, 0
  %1077 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1076, ptr %1075, 1
  %1078 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1077, i64 176, 2
  %1079 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1078, i64 4, 3, 0
  %1080 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1079, i64 20, 4, 0
  %1081 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1080, i64 4, 3, 1
  %1082 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1081, i64 1, 4, 1
  %1083 = call ptr @llvm.stacksave.p0()
  %1084 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1082, ptr %1084, align 8
  %1085 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1084, 1
  %1086 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %491, ptr %1086, align 8
  %1087 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1086, 1
  %1088 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1085, ptr %1088, align 8
  %1089 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1087, ptr %1089, align 8
  call void @memrefCopy(i64 4, ptr %1088, ptr %1089)
  call void @llvm.stackrestore.p0(ptr %1083)
  %1090 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %1091 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %1092 = insertvalue { ptr, ptr, i64 } poison, ptr %1090, 0
  %1093 = insertvalue { ptr, ptr, i64 } %1092, ptr %1091, 1
  %1094 = insertvalue { ptr, ptr, i64 } %1093, i64 0, 2
  %1095 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %1096 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %1097 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %1098 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %1099 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %1100 = extractvalue { ptr, ptr, i64 } %1094, 0
  %1101 = extractvalue { ptr, ptr, i64 } %1094, 1
  %1102 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1100, 0
  %1103 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1102, ptr %1101, 1
  %1104 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1103, i64 320, 2
  %1105 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1104, i64 4, 3, 0
  %1106 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1105, i64 40, 4, 0
  %1107 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1106, i64 16, 3, 1
  %1108 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1107, i64 1, 4, 1
  %1109 = call ptr @llvm.stacksave.p0()
  %1110 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1108, ptr %1110, align 8
  %1111 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1110, 1
  %1112 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %529, ptr %1112, align 8
  %1113 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1112, 1
  %1114 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1111, ptr %1114, align 8
  %1115 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1113, ptr %1115, align 8
  call void @memrefCopy(i64 4, ptr %1114, ptr %1115)
  call void @llvm.stackrestore.p0(ptr %1109)
  %1116 = call ptr @llvm.stacksave.p0()
  %1117 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %558, ptr %1117, align 8
  %1118 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1117, 1
  %1119 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %567, ptr %1119, align 8
  %1120 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1119, 1
  %1121 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1118, ptr %1121, align 8
  %1122 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1120, ptr %1122, align 8
  call void @memrefCopy(i64 4, ptr %1121, ptr %1122)
  call void @llvm.stackrestore.p0(ptr %1116)
  %1123 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %539, ptr %577, ptr %501, i32 3, i32 0, i32 117)
  %1124 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %1125 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %1126 = insertvalue { ptr, ptr, i64 } poison, ptr %1124, 0
  %1127 = insertvalue { ptr, ptr, i64 } %1126, ptr %1125, 1
  %1128 = insertvalue { ptr, ptr, i64 } %1127, i64 0, 2
  %1129 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %1130 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %1131 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %1132 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %1133 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %1134 = extractvalue { ptr, ptr, i64 } %1128, 0
  %1135 = extractvalue { ptr, ptr, i64 } %1128, 1
  %1136 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1134, 0
  %1137 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1136, ptr %1135, 1
  %1138 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1137, i64 336, 2
  %1139 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1138, i64 4, 3, 0
  %1140 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1139, i64 40, 4, 0
  %1141 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1140, i64 16, 3, 1
  %1142 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1141, i64 1, 4, 1
  %1143 = call ptr @llvm.stacksave.p0()
  %1144 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1142, ptr %1144, align 8
  %1145 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1144, 1
  %1146 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %529, ptr %1146, align 8
  %1147 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1146, 1
  %1148 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1145, ptr %1148, align 8
  %1149 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1147, ptr %1149, align 8
  call void @memrefCopy(i64 4, ptr %1148, ptr %1149)
  call void @llvm.stackrestore.p0(ptr %1143)
  %1150 = call ptr @llvm.stacksave.p0()
  %1151 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %623, ptr %1151, align 8
  %1152 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1151, 1
  %1153 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %567, ptr %1153, align 8
  %1154 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1153, 1
  %1155 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1152, ptr %1155, align 8
  %1156 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1154, ptr %1156, align 8
  call void @memrefCopy(i64 4, ptr %1155, ptr %1156)
  call void @llvm.stackrestore.p0(ptr %1150)
  %1157 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %539, ptr %577, ptr %501, i32 3, i32 42, i32 117)
  %1158 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %1159 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %1160 = insertvalue { ptr, ptr, i64 } poison, ptr %1158, 0
  %1161 = insertvalue { ptr, ptr, i64 } %1160, ptr %1159, 1
  %1162 = insertvalue { ptr, ptr, i64 } %1161, i64 0, 2
  %1163 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %1164 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %1165 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %1166 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %1167 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %1168 = extractvalue { ptr, ptr, i64 } %1162, 0
  %1169 = extractvalue { ptr, ptr, i64 } %1162, 1
  %1170 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1168, 0
  %1171 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1170, ptr %1169, 1
  %1172 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1171, i64 352, 2
  %1173 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1172, i64 4, 3, 0
  %1174 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1173, i64 40, 4, 0
  %1175 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1174, i64 8, 3, 1
  %1176 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1175, i64 1, 4, 1
  %1177 = call ptr @llvm.stacksave.p0()
  %1178 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1176, ptr %1178, align 8
  %1179 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1178, 1
  %1180 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %659, ptr %1180, align 8
  %1181 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1180, 1
  %1182 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1179, ptr %1182, align 8
  %1183 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1181, ptr %1183, align 8
  call void @memrefCopy(i64 4, ptr %1182, ptr %1183)
  call void @llvm.stackrestore.p0(ptr %1177)
  %1184 = call ptr @llvm.stacksave.p0()
  %1185 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %688, ptr %1185, align 8
  %1186 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1185, 1
  %1187 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %697, ptr %1187, align 8
  %1188 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1187, 1
  %1189 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1186, ptr %1189, align 8
  %1190 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1188, ptr %1190, align 8
  call void @memrefCopy(i64 4, ptr %1189, ptr %1190)
  call void @llvm.stackrestore.p0(ptr %1184)
  %1191 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 8, i32 4, ptr %669, ptr %707, ptr %501, i32 3, i32 120, i32 109)
  %1192 = call ptr @llvm.stacksave.p0()
  %1193 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %491, ptr %1193, align 8
  %1194 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1193, 1
  %1195 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1082, ptr %1195, align 8
  %1196 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1195, 1
  %1197 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1194, ptr %1197, align 8
  %1198 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1196, ptr %1198, align 8
  call void @memrefCopy(i64 4, ptr %1197, ptr %1198)
  call void @llvm.stackrestore.p0(ptr %1192)
  %1199 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %1200 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %1201 = insertvalue { ptr, ptr, i64 } poison, ptr %1199, 0
  %1202 = insertvalue { ptr, ptr, i64 } %1201, ptr %1200, 1
  %1203 = insertvalue { ptr, ptr, i64 } %1202, i64 0, 2
  %1204 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %1205 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %1206 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %1207 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %1208 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %1209 = extractvalue { ptr, ptr, i64 } %1203, 0
  %1210 = extractvalue { ptr, ptr, i64 } %1203, 1
  %1211 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1209, 0
  %1212 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1211, ptr %1210, 1
  %1213 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1212, i64 256, 2
  %1214 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1213, i64 4, 3, 0
  %1215 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1214, i64 20, 4, 0
  %1216 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1215, i64 4, 3, 1
  %1217 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1216, i64 1, 4, 1
  %1218 = call ptr @llvm.stacksave.p0()
  %1219 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1217, ptr %1219, align 8
  %1220 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1219, 1
  %1221 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %491, ptr %1221, align 8
  %1222 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1221, 1
  %1223 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1220, ptr %1223, align 8
  %1224 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1222, ptr %1224, align 8
  call void @memrefCopy(i64 4, ptr %1223, ptr %1224)
  call void @llvm.stackrestore.p0(ptr %1218)
  %1225 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %1226 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %1227 = insertvalue { ptr, ptr, i64 } poison, ptr %1225, 0
  %1228 = insertvalue { ptr, ptr, i64 } %1227, ptr %1226, 1
  %1229 = insertvalue { ptr, ptr, i64 } %1228, i64 0, 2
  %1230 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %1231 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %1232 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %1233 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %1234 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %1235 = extractvalue { ptr, ptr, i64 } %1229, 0
  %1236 = extractvalue { ptr, ptr, i64 } %1229, 1
  %1237 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1235, 0
  %1238 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1237, ptr %1236, 1
  %1239 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1238, i64 480, 2
  %1240 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1239, i64 4, 3, 0
  %1241 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1240, i64 40, 4, 0
  %1242 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1241, i64 16, 3, 1
  %1243 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1242, i64 1, 4, 1
  %1244 = call ptr @llvm.stacksave.p0()
  %1245 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1243, ptr %1245, align 8
  %1246 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1245, 1
  %1247 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %529, ptr %1247, align 8
  %1248 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1247, 1
  %1249 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1246, ptr %1249, align 8
  %1250 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1248, ptr %1250, align 8
  call void @memrefCopy(i64 4, ptr %1249, ptr %1250)
  call void @llvm.stackrestore.p0(ptr %1244)
  %1251 = call ptr @llvm.stacksave.p0()
  %1252 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %558, ptr %1252, align 8
  %1253 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1252, 1
  %1254 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %567, ptr %1254, align 8
  %1255 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1254, 1
  %1256 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1253, ptr %1256, align 8
  %1257 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1255, ptr %1257, align 8
  call void @memrefCopy(i64 4, ptr %1256, ptr %1257)
  call void @llvm.stackrestore.p0(ptr %1251)
  %1258 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %539, ptr %577, ptr %501, i32 1, i32 189, i32 117)
  %1259 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %1260 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %1261 = insertvalue { ptr, ptr, i64 } poison, ptr %1259, 0
  %1262 = insertvalue { ptr, ptr, i64 } %1261, ptr %1260, 1
  %1263 = insertvalue { ptr, ptr, i64 } %1262, i64 0, 2
  %1264 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %1265 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %1266 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %1267 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %1268 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %1269 = extractvalue { ptr, ptr, i64 } %1263, 0
  %1270 = extractvalue { ptr, ptr, i64 } %1263, 1
  %1271 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1269, 0
  %1272 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1271, ptr %1270, 1
  %1273 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1272, i64 496, 2
  %1274 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1273, i64 4, 3, 0
  %1275 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1274, i64 40, 4, 0
  %1276 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1275, i64 16, 3, 1
  %1277 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1276, i64 1, 4, 1
  %1278 = call ptr @llvm.stacksave.p0()
  %1279 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1277, ptr %1279, align 8
  %1280 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1279, 1
  %1281 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %529, ptr %1281, align 8
  %1282 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1281, 1
  %1283 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1280, ptr %1283, align 8
  %1284 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1282, ptr %1284, align 8
  call void @memrefCopy(i64 4, ptr %1283, ptr %1284)
  call void @llvm.stackrestore.p0(ptr %1278)
  %1285 = call ptr @llvm.stacksave.p0()
  %1286 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %623, ptr %1286, align 8
  %1287 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1286, 1
  %1288 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %567, ptr %1288, align 8
  %1289 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1288, 1
  %1290 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1287, ptr %1290, align 8
  %1291 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1289, ptr %1291, align 8
  call void @memrefCopy(i64 4, ptr %1290, ptr %1291)
  call void @llvm.stackrestore.p0(ptr %1285)
  %1292 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %539, ptr %577, ptr %501, i32 1, i32 258, i32 117)
  %1293 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %1294 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %1295 = insertvalue { ptr, ptr, i64 } poison, ptr %1293, 0
  %1296 = insertvalue { ptr, ptr, i64 } %1295, ptr %1294, 1
  %1297 = insertvalue { ptr, ptr, i64 } %1296, i64 0, 2
  %1298 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %1299 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %1300 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %1301 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %1302 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %1303 = extractvalue { ptr, ptr, i64 } %1297, 0
  %1304 = extractvalue { ptr, ptr, i64 } %1297, 1
  %1305 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1303, 0
  %1306 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1305, ptr %1304, 1
  %1307 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1306, i64 512, 2
  %1308 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1307, i64 4, 3, 0
  %1309 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1308, i64 40, 4, 0
  %1310 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1309, i64 8, 3, 1
  %1311 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1310, i64 1, 4, 1
  %1312 = call ptr @llvm.stacksave.p0()
  %1313 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1311, ptr %1313, align 8
  %1314 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1313, 1
  %1315 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %659, ptr %1315, align 8
  %1316 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1315, 1
  %1317 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1314, ptr %1317, align 8
  %1318 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1316, ptr %1318, align 8
  call void @memrefCopy(i64 4, ptr %1317, ptr %1318)
  call void @llvm.stackrestore.p0(ptr %1312)
  %1319 = call ptr @llvm.stacksave.p0()
  %1320 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %688, ptr %1320, align 8
  %1321 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1320, 1
  %1322 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %697, ptr %1322, align 8
  %1323 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1322, 1
  %1324 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1321, ptr %1324, align 8
  %1325 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1323, ptr %1325, align 8
  call void @memrefCopy(i64 4, ptr %1324, ptr %1325)
  call void @llvm.stackrestore.p0(ptr %1319)
  %1326 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 8, i32 4, ptr %669, ptr %707, ptr %501, i32 1, i32 327, i32 109)
  %1327 = call ptr @llvm.stacksave.p0()
  %1328 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %491, ptr %1328, align 8
  %1329 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1328, 1
  %1330 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1217, ptr %1330, align 8
  %1331 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1330, 1
  %1332 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1329, ptr %1332, align 8
  %1333 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1331, ptr %1333, align 8
  call void @memrefCopy(i64 4, ptr %1332, ptr %1333)
  call void @llvm.stackrestore.p0(ptr %1327)
  %1334 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %1335 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %1336 = insertvalue { ptr, ptr, i64 } poison, ptr %1334, 0
  %1337 = insertvalue { ptr, ptr, i64 } %1336, ptr %1335, 1
  %1338 = insertvalue { ptr, ptr, i64 } %1337, i64 0, 2
  %1339 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %1340 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %1341 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %1342 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %1343 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %1344 = extractvalue { ptr, ptr, i64 } %1338, 0
  %1345 = extractvalue { ptr, ptr, i64 } %1338, 1
  %1346 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1344, 0
  %1347 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1346, ptr %1345, 1
  %1348 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1347, i64 320, 2
  %1349 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1348, i64 8, 3, 0
  %1350 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1349, i64 20, 4, 0
  %1351 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1350, i64 8, 3, 1
  %1352 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1351, i64 1, 4, 1
  %1353 = call ptr @llvm.stacksave.p0()
  %1354 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1352, ptr %1354, align 8
  %1355 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1354, 1
  %1356 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %104, ptr %1356, align 8
  %1357 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1356, 1
  %1358 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1355, ptr %1358, align 8
  %1359 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1357, ptr %1359, align 8
  call void @memrefCopy(i64 4, ptr %1358, ptr %1359)
  call void @llvm.stackrestore.p0(ptr %1353)
  %1360 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %1361 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %1362 = insertvalue { ptr, ptr, i64 } poison, ptr %1360, 0
  %1363 = insertvalue { ptr, ptr, i64 } %1362, ptr %1361, 1
  %1364 = insertvalue { ptr, ptr, i64 } %1363, i64 0, 2
  %1365 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %1366 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %1367 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %1368 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %1369 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %1370 = extractvalue { ptr, ptr, i64 } %1364, 0
  %1371 = extractvalue { ptr, ptr, i64 } %1364, 1
  %1372 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1370, 0
  %1373 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1372, ptr %1371, 1
  %1374 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1373, i64 640, 2
  %1375 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1374, i64 8, 3, 0
  %1376 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1375, i64 40, 4, 0
  %1377 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1376, i64 16, 3, 1
  %1378 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1377, i64 1, 4, 1
  %1379 = call ptr @llvm.stacksave.p0()
  %1380 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1378, ptr %1380, align 8
  %1381 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1380, 1
  %1382 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %142, ptr %1382, align 8
  %1383 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1382, 1
  %1384 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1381, ptr %1384, align 8
  %1385 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1383, ptr %1385, align 8
  call void @memrefCopy(i64 4, ptr %1384, ptr %1385)
  call void @llvm.stackrestore.p0(ptr %1379)
  %1386 = call ptr @llvm.stacksave.p0()
  %1387 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %171, ptr %1387, align 8
  %1388 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1387, 1
  %1389 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %180, ptr %1389, align 8
  %1390 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1389, 1
  %1391 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1388, ptr %1391, align 8
  %1392 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1390, ptr %1392, align 8
  call void @memrefCopy(i64 4, ptr %1391, ptr %1392)
  call void @llvm.stackrestore.p0(ptr %1386)
  %1393 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %152, ptr %190, ptr %114, i32 0, i32 818, i32 125)
  %1394 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %1395 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %1396 = insertvalue { ptr, ptr, i64 } poison, ptr %1394, 0
  %1397 = insertvalue { ptr, ptr, i64 } %1396, ptr %1395, 1
  %1398 = insertvalue { ptr, ptr, i64 } %1397, i64 0, 2
  %1399 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %1400 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %1401 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %1402 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %1403 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %1404 = extractvalue { ptr, ptr, i64 } %1398, 0
  %1405 = extractvalue { ptr, ptr, i64 } %1398, 1
  %1406 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1404, 0
  %1407 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1406, ptr %1405, 1
  %1408 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1407, i64 656, 2
  %1409 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1408, i64 8, 3, 0
  %1410 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1409, i64 40, 4, 0
  %1411 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1410, i64 16, 3, 1
  %1412 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1411, i64 1, 4, 1
  %1413 = call ptr @llvm.stacksave.p0()
  %1414 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1412, ptr %1414, align 8
  %1415 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1414, 1
  %1416 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %142, ptr %1416, align 8
  %1417 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1416, 1
  %1418 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1415, ptr %1418, align 8
  %1419 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1417, ptr %1419, align 8
  call void @memrefCopy(i64 4, ptr %1418, ptr %1419)
  call void @llvm.stackrestore.p0(ptr %1413)
  %1420 = call ptr @llvm.stacksave.p0()
  %1421 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %236, ptr %1421, align 8
  %1422 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1421, 1
  %1423 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %180, ptr %1423, align 8
  %1424 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1423, 1
  %1425 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1422, ptr %1425, align 8
  %1426 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1424, ptr %1426, align 8
  call void @memrefCopy(i64 4, ptr %1425, ptr %1426)
  call void @llvm.stackrestore.p0(ptr %1420)
  %1427 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 16, i32 8, ptr %152, ptr %190, ptr %114, i32 0, i32 887, i32 125)
  %1428 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %1429 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %1430 = insertvalue { ptr, ptr, i64 } poison, ptr %1428, 0
  %1431 = insertvalue { ptr, ptr, i64 } %1430, ptr %1429, 1
  %1432 = insertvalue { ptr, ptr, i64 } %1431, i64 0, 2
  %1433 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %1434 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %1435 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %1436 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %1437 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %1438 = extractvalue { ptr, ptr, i64 } %1432, 0
  %1439 = extractvalue { ptr, ptr, i64 } %1432, 1
  %1440 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1438, 0
  %1441 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1440, ptr %1439, 1
  %1442 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1441, i64 672, 2
  %1443 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1442, i64 8, 3, 0
  %1444 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1443, i64 40, 4, 0
  %1445 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1444, i64 8, 3, 1
  %1446 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1445, i64 1, 4, 1
  %1447 = call ptr @llvm.stacksave.p0()
  %1448 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1446, ptr %1448, align 8
  %1449 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1448, 1
  %1450 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %272, ptr %1450, align 8
  %1451 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1450, 1
  %1452 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1449, ptr %1452, align 8
  %1453 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1451, ptr %1453, align 8
  call void @memrefCopy(i64 4, ptr %1452, ptr %1453)
  call void @llvm.stackrestore.p0(ptr %1447)
  %1454 = call ptr @llvm.stacksave.p0()
  %1455 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %301, ptr %1455, align 8
  %1456 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1455, 1
  %1457 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %310, ptr %1457, align 8
  %1458 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1457, 1
  %1459 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1456, ptr %1459, align 8
  %1460 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1458, ptr %1460, align 8
  call void @memrefCopy(i64 4, ptr %1459, ptr %1460)
  call void @llvm.stackrestore.p0(ptr %1454)
  %1461 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 8, i32 8, i32 8, ptr %282, ptr %320, ptr %114, i32 0, i32 956, i32 117)
  %1462 = call ptr @llvm.stacksave.p0()
  %1463 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %104, ptr %1463, align 8
  %1464 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1463, 1
  %1465 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1352, ptr %1465, align 8
  %1466 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1465, 1
  %1467 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1464, ptr %1467, align 8
  %1468 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1466, ptr %1468, align 8
  call void @memrefCopy(i64 4, ptr %1467, ptr %1468)
  call void @llvm.stackrestore.p0(ptr %1462)
  %1469 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %1470 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %1471 = insertvalue { ptr, ptr, i64 } poison, ptr %1469, 0
  %1472 = insertvalue { ptr, ptr, i64 } %1471, ptr %1470, 1
  %1473 = insertvalue { ptr, ptr, i64 } %1472, i64 0, 2
  %1474 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %1475 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %1476 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %1477 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %1478 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %1479 = extractvalue { ptr, ptr, i64 } %1473, 0
  %1480 = extractvalue { ptr, ptr, i64 } %1473, 1
  %1481 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1479, 0
  %1482 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1481, ptr %1480, 1
  %1483 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1482, i64 328, 2
  %1484 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1483, i64 4, 3, 0
  %1485 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1484, i64 20, 4, 0
  %1486 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1485, i64 4, 3, 1
  %1487 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1486, i64 1, 4, 1
  %1488 = call ptr @llvm.stacksave.p0()
  %1489 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1487, ptr %1489, align 8
  %1490 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1489, 1
  %1491 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %491, ptr %1491, align 8
  %1492 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1491, 1
  %1493 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1490, ptr %1493, align 8
  %1494 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1492, ptr %1494, align 8
  call void @memrefCopy(i64 4, ptr %1493, ptr %1494)
  call void @llvm.stackrestore.p0(ptr %1488)
  %1495 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %1496 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %1497 = insertvalue { ptr, ptr, i64 } poison, ptr %1495, 0
  %1498 = insertvalue { ptr, ptr, i64 } %1497, ptr %1496, 1
  %1499 = insertvalue { ptr, ptr, i64 } %1498, i64 0, 2
  %1500 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %1501 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %1502 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %1503 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %1504 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %1505 = extractvalue { ptr, ptr, i64 } %1499, 0
  %1506 = extractvalue { ptr, ptr, i64 } %1499, 1
  %1507 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1505, 0
  %1508 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1507, ptr %1506, 1
  %1509 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1508, i64 640, 2
  %1510 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1509, i64 4, 3, 0
  %1511 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1510, i64 40, 4, 0
  %1512 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1511, i64 16, 3, 1
  %1513 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1512, i64 1, 4, 1
  %1514 = call ptr @llvm.stacksave.p0()
  %1515 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1513, ptr %1515, align 8
  %1516 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1515, 1
  %1517 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %529, ptr %1517, align 8
  %1518 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1517, 1
  %1519 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1516, ptr %1519, align 8
  %1520 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1518, ptr %1520, align 8
  call void @memrefCopy(i64 4, ptr %1519, ptr %1520)
  call void @llvm.stackrestore.p0(ptr %1514)
  %1521 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %1522 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %1523 = insertvalue { ptr, ptr, i64 } poison, ptr %1521, 0
  %1524 = insertvalue { ptr, ptr, i64 } %1523, ptr %1522, 1
  %1525 = insertvalue { ptr, ptr, i64 } %1524, i64 0, 2
  %1526 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %1527 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %1528 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %1529 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %1530 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %1531 = extractvalue { ptr, ptr, i64 } %1525, 0
  %1532 = extractvalue { ptr, ptr, i64 } %1525, 1
  %1533 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1531, 0
  %1534 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1533, ptr %1532, 1
  %1535 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1534, i64 8, 2
  %1536 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1535, i64 16, 3, 0
  %1537 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1536, i64 20, 4, 0
  %1538 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1537, i64 4, 3, 1
  %1539 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1538, i64 1, 4, 1
  %1540 = call ptr @llvm.stacksave.p0()
  %1541 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1539, ptr %1541, align 8
  %1542 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1541, 1
  %1543 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %567, ptr %1543, align 8
  %1544 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1543, 1
  %1545 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1542, ptr %1545, align 8
  %1546 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1544, ptr %1546, align 8
  call void @memrefCopy(i64 4, ptr %1545, ptr %1546)
  call void @llvm.stackrestore.p0(ptr %1540)
  %1547 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %539, ptr %577, ptr %501, i32 2, i32 189, i32 117)
  %1548 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %1549 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %1550 = insertvalue { ptr, ptr, i64 } poison, ptr %1548, 0
  %1551 = insertvalue { ptr, ptr, i64 } %1550, ptr %1549, 1
  %1552 = insertvalue { ptr, ptr, i64 } %1551, i64 0, 2
  %1553 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %1554 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %1555 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %1556 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %1557 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %1558 = extractvalue { ptr, ptr, i64 } %1552, 0
  %1559 = extractvalue { ptr, ptr, i64 } %1552, 1
  %1560 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1558, 0
  %1561 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1560, ptr %1559, 1
  %1562 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1561, i64 656, 2
  %1563 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1562, i64 4, 3, 0
  %1564 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1563, i64 40, 4, 0
  %1565 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1564, i64 16, 3, 1
  %1566 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1565, i64 1, 4, 1
  %1567 = call ptr @llvm.stacksave.p0()
  %1568 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1566, ptr %1568, align 8
  %1569 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1568, 1
  %1570 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %529, ptr %1570, align 8
  %1571 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1570, 1
  %1572 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1569, ptr %1572, align 8
  %1573 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1571, ptr %1573, align 8
  call void @memrefCopy(i64 4, ptr %1572, ptr %1573)
  call void @llvm.stackrestore.p0(ptr %1567)
  %1574 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %1575 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %1576 = insertvalue { ptr, ptr, i64 } poison, ptr %1574, 0
  %1577 = insertvalue { ptr, ptr, i64 } %1576, ptr %1575, 1
  %1578 = insertvalue { ptr, ptr, i64 } %1577, i64 0, 2
  %1579 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %1580 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %1581 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %1582 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %1583 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %1584 = extractvalue { ptr, ptr, i64 } %1578, 0
  %1585 = extractvalue { ptr, ptr, i64 } %1578, 1
  %1586 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1584, 0
  %1587 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1586, ptr %1585, 1
  %1588 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1587, i64 328, 2
  %1589 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1588, i64 16, 3, 0
  %1590 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1589, i64 20, 4, 0
  %1591 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1590, i64 4, 3, 1
  %1592 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1591, i64 1, 4, 1
  %1593 = call ptr @llvm.stacksave.p0()
  %1594 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1592, ptr %1594, align 8
  %1595 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1594, 1
  %1596 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %567, ptr %1596, align 8
  %1597 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1596, 1
  %1598 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1595, ptr %1598, align 8
  %1599 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1597, ptr %1599, align 8
  call void @memrefCopy(i64 4, ptr %1598, ptr %1599)
  call void @llvm.stackrestore.p0(ptr %1593)
  %1600 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %539, ptr %577, ptr %501, i32 2, i32 258, i32 117)
  %1601 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %1602 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %1603 = insertvalue { ptr, ptr, i64 } poison, ptr %1601, 0
  %1604 = insertvalue { ptr, ptr, i64 } %1603, ptr %1602, 1
  %1605 = insertvalue { ptr, ptr, i64 } %1604, i64 0, 2
  %1606 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %1607 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %1608 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %1609 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %1610 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %1611 = extractvalue { ptr, ptr, i64 } %1605, 0
  %1612 = extractvalue { ptr, ptr, i64 } %1605, 1
  %1613 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1611, 0
  %1614 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1613, ptr %1612, 1
  %1615 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1614, i64 672, 2
  %1616 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1615, i64 4, 3, 0
  %1617 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1616, i64 40, 4, 0
  %1618 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1617, i64 8, 3, 1
  %1619 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1618, i64 1, 4, 1
  %1620 = call ptr @llvm.stacksave.p0()
  %1621 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1619, ptr %1621, align 8
  %1622 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1621, 1
  %1623 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %659, ptr %1623, align 8
  %1624 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1623, 1
  %1625 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1622, ptr %1625, align 8
  %1626 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1624, ptr %1626, align 8
  call void @memrefCopy(i64 4, ptr %1625, ptr %1626)
  call void @llvm.stackrestore.p0(ptr %1620)
  %1627 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %1628 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %1629 = insertvalue { ptr, ptr, i64 } poison, ptr %1627, 0
  %1630 = insertvalue { ptr, ptr, i64 } %1629, ptr %1628, 1
  %1631 = insertvalue { ptr, ptr, i64 } %1630, i64 0, 2
  %1632 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %1633 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %1634 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %1635 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %1636 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %1637 = extractvalue { ptr, ptr, i64 } %1631, 0
  %1638 = extractvalue { ptr, ptr, i64 } %1631, 1
  %1639 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1637, 0
  %1640 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1639, ptr %1638, 1
  %1641 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1640, i64 648, 2
  %1642 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1641, i64 8, 3, 0
  %1643 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1642, i64 20, 4, 0
  %1644 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1643, i64 4, 3, 1
  %1645 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1644, i64 1, 4, 1
  %1646 = call ptr @llvm.stacksave.p0()
  %1647 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1645, ptr %1647, align 8
  %1648 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1647, 1
  %1649 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %697, ptr %1649, align 8
  %1650 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1649, 1
  %1651 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1648, ptr %1651, align 8
  %1652 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1650, ptr %1652, align 8
  call void @memrefCopy(i64 4, ptr %1651, ptr %1652)
  call void @llvm.stackrestore.p0(ptr %1646)
  %1653 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 8, i32 4, ptr %669, ptr %707, ptr %501, i32 2, i32 327, i32 109)
  %1654 = call ptr @llvm.stacksave.p0()
  %1655 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %491, ptr %1655, align 8
  %1656 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1655, 1
  %1657 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1487, ptr %1657, align 8
  %1658 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1657, 1
  %1659 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1656, ptr %1659, align 8
  %1660 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1658, ptr %1660, align 8
  call void @memrefCopy(i64 4, ptr %1659, ptr %1660)
  call void @llvm.stackrestore.p0(ptr %1654)
  %1661 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %1662 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %1663 = insertvalue { ptr, ptr, i64 } poison, ptr %1661, 0
  %1664 = insertvalue { ptr, ptr, i64 } %1663, ptr %1662, 1
  %1665 = insertvalue { ptr, ptr, i64 } %1664, i64 0, 2
  %1666 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %1667 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %1668 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %1669 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %1670 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %1671 = extractvalue { ptr, ptr, i64 } %1665, 0
  %1672 = extractvalue { ptr, ptr, i64 } %1665, 1
  %1673 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1671, 0
  %1674 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1673, ptr %1672, 1
  %1675 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1674, i64 332, 2
  %1676 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1675, i64 4, 3, 0
  %1677 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1676, i64 20, 4, 0
  %1678 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1677, i64 4, 3, 1
  %1679 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1678, i64 1, 4, 1
  %1680 = call ptr @llvm.stacksave.p0()
  %1681 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1679, ptr %1681, align 8
  %1682 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1681, 1
  %1683 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %491, ptr %1683, align 8
  %1684 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1683, 1
  %1685 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1682, ptr %1685, align 8
  %1686 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1684, ptr %1686, align 8
  call void @memrefCopy(i64 4, ptr %1685, ptr %1686)
  call void @llvm.stackrestore.p0(ptr %1680)
  %1687 = call ptr @llvm.stacksave.p0()
  %1688 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1513, ptr %1688, align 8
  %1689 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1688, 1
  %1690 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %529, ptr %1690, align 8
  %1691 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1690, 1
  %1692 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1689, ptr %1692, align 8
  %1693 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1691, ptr %1693, align 8
  call void @memrefCopy(i64 4, ptr %1692, ptr %1693)
  call void @llvm.stackrestore.p0(ptr %1687)
  %1694 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %1695 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %1696 = insertvalue { ptr, ptr, i64 } poison, ptr %1694, 0
  %1697 = insertvalue { ptr, ptr, i64 } %1696, ptr %1695, 1
  %1698 = insertvalue { ptr, ptr, i64 } %1697, i64 0, 2
  %1699 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %1700 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %1701 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %1702 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %1703 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %1704 = extractvalue { ptr, ptr, i64 } %1698, 0
  %1705 = extractvalue { ptr, ptr, i64 } %1698, 1
  %1706 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1704, 0
  %1707 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1706, ptr %1705, 1
  %1708 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1707, i64 12, 2
  %1709 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1708, i64 16, 3, 0
  %1710 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1709, i64 20, 4, 0
  %1711 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1710, i64 4, 3, 1
  %1712 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1711, i64 1, 4, 1
  %1713 = call ptr @llvm.stacksave.p0()
  %1714 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1712, ptr %1714, align 8
  %1715 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1714, 1
  %1716 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %567, ptr %1716, align 8
  %1717 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1716, 1
  %1718 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1715, ptr %1718, align 8
  %1719 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1717, ptr %1719, align 8
  call void @memrefCopy(i64 4, ptr %1718, ptr %1719)
  call void @llvm.stackrestore.p0(ptr %1713)
  %1720 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %539, ptr %577, ptr %501, i32 3, i32 189, i32 117)
  %1721 = call ptr @llvm.stacksave.p0()
  %1722 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1566, ptr %1722, align 8
  %1723 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1722, 1
  %1724 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %529, ptr %1724, align 8
  %1725 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1724, 1
  %1726 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1723, ptr %1726, align 8
  %1727 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1725, ptr %1727, align 8
  call void @memrefCopy(i64 4, ptr %1726, ptr %1727)
  call void @llvm.stackrestore.p0(ptr %1721)
  %1728 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %1729 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %1730 = insertvalue { ptr, ptr, i64 } poison, ptr %1728, 0
  %1731 = insertvalue { ptr, ptr, i64 } %1730, ptr %1729, 1
  %1732 = insertvalue { ptr, ptr, i64 } %1731, i64 0, 2
  %1733 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %1734 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %1735 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %1736 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %1737 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %1738 = extractvalue { ptr, ptr, i64 } %1732, 0
  %1739 = extractvalue { ptr, ptr, i64 } %1732, 1
  %1740 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1738, 0
  %1741 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1740, ptr %1739, 1
  %1742 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1741, i64 332, 2
  %1743 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1742, i64 16, 3, 0
  %1744 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1743, i64 20, 4, 0
  %1745 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1744, i64 4, 3, 1
  %1746 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1745, i64 1, 4, 1
  %1747 = call ptr @llvm.stacksave.p0()
  %1748 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1746, ptr %1748, align 8
  %1749 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1748, 1
  %1750 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %567, ptr %1750, align 8
  %1751 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1750, 1
  %1752 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1749, ptr %1752, align 8
  %1753 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1751, ptr %1753, align 8
  call void @memrefCopy(i64 4, ptr %1752, ptr %1753)
  call void @llvm.stackrestore.p0(ptr %1747)
  %1754 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %539, ptr %577, ptr %501, i32 3, i32 258, i32 117)
  %1755 = call ptr @llvm.stacksave.p0()
  %1756 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1619, ptr %1756, align 8
  %1757 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1756, 1
  %1758 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %659, ptr %1758, align 8
  %1759 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1758, 1
  %1760 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1757, ptr %1760, align 8
  %1761 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1759, ptr %1761, align 8
  call void @memrefCopy(i64 4, ptr %1760, ptr %1761)
  call void @llvm.stackrestore.p0(ptr %1755)
  %1762 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %1763 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %1764 = insertvalue { ptr, ptr, i64 } poison, ptr %1762, 0
  %1765 = insertvalue { ptr, ptr, i64 } %1764, ptr %1763, 1
  %1766 = insertvalue { ptr, ptr, i64 } %1765, i64 0, 2
  %1767 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %1768 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %1769 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %1770 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %1771 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %1772 = extractvalue { ptr, ptr, i64 } %1766, 0
  %1773 = extractvalue { ptr, ptr, i64 } %1766, 1
  %1774 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1772, 0
  %1775 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1774, ptr %1773, 1
  %1776 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1775, i64 652, 2
  %1777 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1776, i64 8, 3, 0
  %1778 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1777, i64 20, 4, 0
  %1779 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1778, i64 4, 3, 1
  %1780 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1779, i64 1, 4, 1
  %1781 = call ptr @llvm.stacksave.p0()
  %1782 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1780, ptr %1782, align 8
  %1783 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1782, 1
  %1784 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %697, ptr %1784, align 8
  %1785 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1784, 1
  %1786 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1783, ptr %1786, align 8
  %1787 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1785, ptr %1787, align 8
  call void @memrefCopy(i64 4, ptr %1786, ptr %1787)
  call void @llvm.stackrestore.p0(ptr %1781)
  %1788 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 8, i32 4, ptr %669, ptr %707, ptr %501, i32 3, i32 327, i32 109)
  %1789 = call ptr @llvm.stacksave.p0()
  %1790 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %491, ptr %1790, align 8
  %1791 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1790, 1
  %1792 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1679, ptr %1792, align 8
  %1793 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1792, 1
  %1794 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1791, ptr %1794, align 8
  %1795 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1793, ptr %1795, align 8
  call void @memrefCopy(i64 4, ptr %1794, ptr %1795)
  call void @llvm.stackrestore.p0(ptr %1789)
  %1796 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %1797 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %1798 = insertvalue { ptr, ptr, i64 } poison, ptr %1796, 0
  %1799 = insertvalue { ptr, ptr, i64 } %1798, ptr %1797, 1
  %1800 = insertvalue { ptr, ptr, i64 } %1799, i64 0, 2
  %1801 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %1802 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %1803 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %1804 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %1805 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %1806 = extractvalue { ptr, ptr, i64 } %1800, 0
  %1807 = extractvalue { ptr, ptr, i64 } %1800, 1
  %1808 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1806, 0
  %1809 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1808, ptr %1807, 1
  %1810 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1809, i64 336, 2
  %1811 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1810, i64 4, 3, 0
  %1812 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1811, i64 20, 4, 0
  %1813 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1812, i64 4, 3, 1
  %1814 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1813, i64 1, 4, 1
  %1815 = call ptr @llvm.stacksave.p0()
  %1816 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1814, ptr %1816, align 8
  %1817 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1816, 1
  %1818 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %491, ptr %1818, align 8
  %1819 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1818, 1
  %1820 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1817, ptr %1820, align 8
  %1821 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1819, ptr %1821, align 8
  call void @memrefCopy(i64 4, ptr %1820, ptr %1821)
  call void @llvm.stackrestore.p0(ptr %1815)
  %1822 = call ptr @llvm.stacksave.p0()
  %1823 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1513, ptr %1823, align 8
  %1824 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1823, 1
  %1825 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %529, ptr %1825, align 8
  %1826 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1825, 1
  %1827 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1824, ptr %1827, align 8
  %1828 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1826, ptr %1828, align 8
  call void @memrefCopy(i64 4, ptr %1827, ptr %1828)
  call void @llvm.stackrestore.p0(ptr %1822)
  %1829 = call ptr @llvm.stacksave.p0()
  %1830 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %558, ptr %1830, align 8
  %1831 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1830, 1
  %1832 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %567, ptr %1832, align 8
  %1833 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1832, 1
  %1834 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1831, ptr %1834, align 8
  %1835 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1833, ptr %1835, align 8
  call void @memrefCopy(i64 4, ptr %1834, ptr %1835)
  call void @llvm.stackrestore.p0(ptr %1829)
  %1836 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %539, ptr %577, ptr %501, i32 1, i32 396, i32 117)
  %1837 = call ptr @llvm.stacksave.p0()
  %1838 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1566, ptr %1838, align 8
  %1839 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1838, 1
  %1840 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %529, ptr %1840, align 8
  %1841 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1840, 1
  %1842 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1839, ptr %1842, align 8
  %1843 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1841, ptr %1843, align 8
  call void @memrefCopy(i64 4, ptr %1842, ptr %1843)
  call void @llvm.stackrestore.p0(ptr %1837)
  %1844 = call ptr @llvm.stacksave.p0()
  %1845 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %623, ptr %1845, align 8
  %1846 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1845, 1
  %1847 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %567, ptr %1847, align 8
  %1848 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1847, 1
  %1849 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1846, ptr %1849, align 8
  %1850 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1848, ptr %1850, align 8
  call void @memrefCopy(i64 4, ptr %1849, ptr %1850)
  call void @llvm.stackrestore.p0(ptr %1844)
  %1851 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %539, ptr %577, ptr %501, i32 1, i32 465, i32 117)
  %1852 = call ptr @llvm.stacksave.p0()
  %1853 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1619, ptr %1853, align 8
  %1854 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1853, 1
  %1855 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %659, ptr %1855, align 8
  %1856 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1855, 1
  %1857 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1854, ptr %1857, align 8
  %1858 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1856, ptr %1858, align 8
  call void @memrefCopy(i64 4, ptr %1857, ptr %1858)
  call void @llvm.stackrestore.p0(ptr %1852)
  %1859 = call ptr @llvm.stacksave.p0()
  %1860 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %688, ptr %1860, align 8
  %1861 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1860, 1
  %1862 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %697, ptr %1862, align 8
  %1863 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1862, 1
  %1864 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1861, ptr %1864, align 8
  %1865 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1863, ptr %1865, align 8
  call void @memrefCopy(i64 4, ptr %1864, ptr %1865)
  call void @llvm.stackrestore.p0(ptr %1859)
  %1866 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 8, i32 4, ptr %669, ptr %707, ptr %501, i32 1, i32 534, i32 109)
  %1867 = call ptr @llvm.stacksave.p0()
  %1868 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %491, ptr %1868, align 8
  %1869 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1868, 1
  %1870 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1814, ptr %1870, align 8
  %1871 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1870, 1
  %1872 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1869, ptr %1872, align 8
  %1873 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1871, ptr %1873, align 8
  call void @memrefCopy(i64 4, ptr %1872, ptr %1873)
  call void @llvm.stackrestore.p0(ptr %1867)
  %1874 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %1875 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %1876 = insertvalue { ptr, ptr, i64 } poison, ptr %1874, 0
  %1877 = insertvalue { ptr, ptr, i64 } %1876, ptr %1875, 1
  %1878 = insertvalue { ptr, ptr, i64 } %1877, i64 0, 2
  %1879 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %1880 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %1881 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %1882 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %1883 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %1884 = extractvalue { ptr, ptr, i64 } %1878, 0
  %1885 = extractvalue { ptr, ptr, i64 } %1878, 1
  %1886 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1884, 0
  %1887 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1886, ptr %1885, 1
  %1888 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1887, i64 408, 2
  %1889 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1888, i64 4, 3, 0
  %1890 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1889, i64 20, 4, 0
  %1891 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1890, i64 4, 3, 1
  %1892 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1891, i64 1, 4, 1
  %1893 = call ptr @llvm.stacksave.p0()
  %1894 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1892, ptr %1894, align 8
  %1895 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1894, 1
  %1896 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %491, ptr %1896, align 8
  %1897 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1896, 1
  %1898 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1895, ptr %1898, align 8
  %1899 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1897, ptr %1899, align 8
  call void @memrefCopy(i64 4, ptr %1898, ptr %1899)
  call void @llvm.stackrestore.p0(ptr %1893)
  %1900 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %1901 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %1902 = insertvalue { ptr, ptr, i64 } poison, ptr %1900, 0
  %1903 = insertvalue { ptr, ptr, i64 } %1902, ptr %1901, 1
  %1904 = insertvalue { ptr, ptr, i64 } %1903, i64 0, 2
  %1905 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %1906 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %1907 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %1908 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %1909 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %1910 = extractvalue { ptr, ptr, i64 } %1904, 0
  %1911 = extractvalue { ptr, ptr, i64 } %1904, 1
  %1912 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1910, 0
  %1913 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1912, ptr %1911, 1
  %1914 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1913, i64 800, 2
  %1915 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1914, i64 4, 3, 0
  %1916 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1915, i64 40, 4, 0
  %1917 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1916, i64 16, 3, 1
  %1918 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1917, i64 1, 4, 1
  %1919 = call ptr @llvm.stacksave.p0()
  %1920 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1918, ptr %1920, align 8
  %1921 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1920, 1
  %1922 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %529, ptr %1922, align 8
  %1923 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1922, 1
  %1924 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1921, ptr %1924, align 8
  %1925 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1923, ptr %1925, align 8
  call void @memrefCopy(i64 4, ptr %1924, ptr %1925)
  call void @llvm.stackrestore.p0(ptr %1919)
  %1926 = call ptr @llvm.stacksave.p0()
  %1927 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1539, ptr %1927, align 8
  %1928 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1927, 1
  %1929 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %567, ptr %1929, align 8
  %1930 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1929, 1
  %1931 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1928, ptr %1931, align 8
  %1932 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1930, ptr %1932, align 8
  call void @memrefCopy(i64 4, ptr %1931, ptr %1932)
  call void @llvm.stackrestore.p0(ptr %1926)
  %1933 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %539, ptr %577, ptr %501, i32 2, i32 396, i32 117)
  %1934 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %1935 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %1936 = insertvalue { ptr, ptr, i64 } poison, ptr %1934, 0
  %1937 = insertvalue { ptr, ptr, i64 } %1936, ptr %1935, 1
  %1938 = insertvalue { ptr, ptr, i64 } %1937, i64 0, 2
  %1939 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %1940 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %1941 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %1942 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %1943 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %1944 = extractvalue { ptr, ptr, i64 } %1938, 0
  %1945 = extractvalue { ptr, ptr, i64 } %1938, 1
  %1946 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1944, 0
  %1947 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1946, ptr %1945, 1
  %1948 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1947, i64 816, 2
  %1949 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1948, i64 4, 3, 0
  %1950 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1949, i64 40, 4, 0
  %1951 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1950, i64 16, 3, 1
  %1952 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1951, i64 1, 4, 1
  %1953 = call ptr @llvm.stacksave.p0()
  %1954 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1952, ptr %1954, align 8
  %1955 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1954, 1
  %1956 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %529, ptr %1956, align 8
  %1957 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1956, 1
  %1958 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1955, ptr %1958, align 8
  %1959 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1957, ptr %1959, align 8
  call void @memrefCopy(i64 4, ptr %1958, ptr %1959)
  call void @llvm.stackrestore.p0(ptr %1953)
  %1960 = call ptr @llvm.stacksave.p0()
  %1961 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1592, ptr %1961, align 8
  %1962 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1961, 1
  %1963 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %567, ptr %1963, align 8
  %1964 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1963, 1
  %1965 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1962, ptr %1965, align 8
  %1966 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1964, ptr %1966, align 8
  call void @memrefCopy(i64 4, ptr %1965, ptr %1966)
  call void @llvm.stackrestore.p0(ptr %1960)
  %1967 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %539, ptr %577, ptr %501, i32 2, i32 465, i32 117)
  %1968 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %1969 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %1970 = insertvalue { ptr, ptr, i64 } poison, ptr %1968, 0
  %1971 = insertvalue { ptr, ptr, i64 } %1970, ptr %1969, 1
  %1972 = insertvalue { ptr, ptr, i64 } %1971, i64 0, 2
  %1973 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %1974 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %1975 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %1976 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %1977 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %1978 = extractvalue { ptr, ptr, i64 } %1972, 0
  %1979 = extractvalue { ptr, ptr, i64 } %1972, 1
  %1980 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %1978, 0
  %1981 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1980, ptr %1979, 1
  %1982 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1981, i64 832, 2
  %1983 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1982, i64 4, 3, 0
  %1984 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1983, i64 40, 4, 0
  %1985 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1984, i64 8, 3, 1
  %1986 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %1985, i64 1, 4, 1
  %1987 = call ptr @llvm.stacksave.p0()
  %1988 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1986, ptr %1988, align 8
  %1989 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1988, 1
  %1990 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %659, ptr %1990, align 8
  %1991 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1990, 1
  %1992 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1989, ptr %1992, align 8
  %1993 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1991, ptr %1993, align 8
  call void @memrefCopy(i64 4, ptr %1992, ptr %1993)
  call void @llvm.stackrestore.p0(ptr %1987)
  %1994 = call ptr @llvm.stacksave.p0()
  %1995 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1645, ptr %1995, align 8
  %1996 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1995, 1
  %1997 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %697, ptr %1997, align 8
  %1998 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %1997, 1
  %1999 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1996, ptr %1999, align 8
  %2000 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %1998, ptr %2000, align 8
  call void @memrefCopy(i64 4, ptr %1999, ptr %2000)
  call void @llvm.stackrestore.p0(ptr %1994)
  %2001 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 8, i32 4, ptr %669, ptr %707, ptr %501, i32 2, i32 534, i32 109)
  %2002 = call ptr @llvm.stacksave.p0()
  %2003 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %491, ptr %2003, align 8
  %2004 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2003, 1
  %2005 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1892, ptr %2005, align 8
  %2006 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2005, 1
  %2007 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2004, ptr %2007, align 8
  %2008 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2006, ptr %2008, align 8
  call void @memrefCopy(i64 4, ptr %2007, ptr %2008)
  call void @llvm.stackrestore.p0(ptr %2002)
  %2009 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %2010 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %2011 = insertvalue { ptr, ptr, i64 } poison, ptr %2009, 0
  %2012 = insertvalue { ptr, ptr, i64 } %2011, ptr %2010, 1
  %2013 = insertvalue { ptr, ptr, i64 } %2012, i64 0, 2
  %2014 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %2015 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %2016 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %2017 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %2018 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %2019 = extractvalue { ptr, ptr, i64 } %2013, 0
  %2020 = extractvalue { ptr, ptr, i64 } %2013, 1
  %2021 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2019, 0
  %2022 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2021, ptr %2020, 1
  %2023 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2022, i64 412, 2
  %2024 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2023, i64 4, 3, 0
  %2025 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2024, i64 20, 4, 0
  %2026 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2025, i64 4, 3, 1
  %2027 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2026, i64 1, 4, 1
  %2028 = call ptr @llvm.stacksave.p0()
  %2029 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2027, ptr %2029, align 8
  %2030 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2029, 1
  %2031 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %491, ptr %2031, align 8
  %2032 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2031, 1
  %2033 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2030, ptr %2033, align 8
  %2034 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2032, ptr %2034, align 8
  call void @memrefCopy(i64 4, ptr %2033, ptr %2034)
  call void @llvm.stackrestore.p0(ptr %2028)
  %2035 = call ptr @llvm.stacksave.p0()
  %2036 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1918, ptr %2036, align 8
  %2037 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2036, 1
  %2038 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %529, ptr %2038, align 8
  %2039 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2038, 1
  %2040 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2037, ptr %2040, align 8
  %2041 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2039, ptr %2041, align 8
  call void @memrefCopy(i64 4, ptr %2040, ptr %2041)
  call void @llvm.stackrestore.p0(ptr %2035)
  %2042 = call ptr @llvm.stacksave.p0()
  %2043 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1712, ptr %2043, align 8
  %2044 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2043, 1
  %2045 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %567, ptr %2045, align 8
  %2046 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2045, 1
  %2047 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2044, ptr %2047, align 8
  %2048 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2046, ptr %2048, align 8
  call void @memrefCopy(i64 4, ptr %2047, ptr %2048)
  call void @llvm.stackrestore.p0(ptr %2042)
  %2049 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %539, ptr %577, ptr %501, i32 3, i32 396, i32 117)
  %2050 = call ptr @llvm.stacksave.p0()
  %2051 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1952, ptr %2051, align 8
  %2052 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2051, 1
  %2053 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %529, ptr %2053, align 8
  %2054 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2053, 1
  %2055 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2052, ptr %2055, align 8
  %2056 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2054, ptr %2056, align 8
  call void @memrefCopy(i64 4, ptr %2055, ptr %2056)
  call void @llvm.stackrestore.p0(ptr %2050)
  %2057 = call ptr @llvm.stacksave.p0()
  %2058 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1746, ptr %2058, align 8
  %2059 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2058, 1
  %2060 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %567, ptr %2060, align 8
  %2061 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2060, 1
  %2062 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2059, ptr %2062, align 8
  %2063 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2061, ptr %2063, align 8
  call void @memrefCopy(i64 4, ptr %2062, ptr %2063)
  call void @llvm.stackrestore.p0(ptr %2057)
  %2064 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %539, ptr %577, ptr %501, i32 3, i32 465, i32 117)
  %2065 = call ptr @llvm.stacksave.p0()
  %2066 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1986, ptr %2066, align 8
  %2067 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2066, 1
  %2068 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %659, ptr %2068, align 8
  %2069 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2068, 1
  %2070 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2067, ptr %2070, align 8
  %2071 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2069, ptr %2071, align 8
  call void @memrefCopy(i64 4, ptr %2070, ptr %2071)
  call void @llvm.stackrestore.p0(ptr %2065)
  %2072 = call ptr @llvm.stacksave.p0()
  %2073 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1780, ptr %2073, align 8
  %2074 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2073, 1
  %2075 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %697, ptr %2075, align 8
  %2076 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2075, 1
  %2077 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2074, ptr %2077, align 8
  %2078 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2076, ptr %2078, align 8
  call void @memrefCopy(i64 4, ptr %2077, ptr %2078)
  call void @llvm.stackrestore.p0(ptr %2072)
  %2079 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 8, i32 4, ptr %669, ptr %707, ptr %501, i32 3, i32 534, i32 109)
  %2080 = call ptr @llvm.stacksave.p0()
  %2081 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %491, ptr %2081, align 8
  %2082 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2081, 1
  %2083 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2027, ptr %2083, align 8
  %2084 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2083, 1
  %2085 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2082, ptr %2085, align 8
  %2086 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2084, ptr %2086, align 8
  call void @memrefCopy(i64 4, ptr %2085, ptr %2086)
  call void @llvm.stackrestore.p0(ptr %2080)
  %2087 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %2088 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %2089 = insertvalue { ptr, ptr, i64 } poison, ptr %2087, 0
  %2090 = insertvalue { ptr, ptr, i64 } %2089, ptr %2088, 1
  %2091 = insertvalue { ptr, ptr, i64 } %2090, i64 0, 2
  %2092 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %2093 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %2094 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %2095 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %2096 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %2097 = extractvalue { ptr, ptr, i64 } %2091, 0
  %2098 = extractvalue { ptr, ptr, i64 } %2091, 1
  %2099 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %2097, 0
  %2100 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2099, ptr %2098, 1
  %2101 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2100, i64 416, 2
  %2102 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2101, i64 4, 3, 0
  %2103 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2102, i64 20, 4, 0
  %2104 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2103, i64 4, 3, 1
  %2105 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %2104, i64 1, 4, 1
  %2106 = call ptr @llvm.stacksave.p0()
  %2107 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2105, ptr %2107, align 8
  %2108 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2107, 1
  %2109 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %491, ptr %2109, align 8
  %2110 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2109, 1
  %2111 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2108, ptr %2111, align 8
  %2112 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2110, ptr %2112, align 8
  call void @memrefCopy(i64 4, ptr %2111, ptr %2112)
  call void @llvm.stackrestore.p0(ptr %2106)
  %2113 = call ptr @llvm.stacksave.p0()
  %2114 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1918, ptr %2114, align 8
  %2115 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2114, 1
  %2116 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %529, ptr %2116, align 8
  %2117 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2116, 1
  %2118 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2115, ptr %2118, align 8
  %2119 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2117, ptr %2119, align 8
  call void @memrefCopy(i64 4, ptr %2118, ptr %2119)
  call void @llvm.stackrestore.p0(ptr %2113)
  %2120 = call ptr @llvm.stacksave.p0()
  %2121 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %558, ptr %2121, align 8
  %2122 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2121, 1
  %2123 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %567, ptr %2123, align 8
  %2124 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2123, 1
  %2125 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2122, ptr %2125, align 8
  %2126 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2124, ptr %2126, align 8
  call void @memrefCopy(i64 4, ptr %2125, ptr %2126)
  call void @llvm.stackrestore.p0(ptr %2120)
  %2127 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %539, ptr %577, ptr %501, i32 1, i32 603, i32 117)
  %2128 = call ptr @llvm.stacksave.p0()
  %2129 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1952, ptr %2129, align 8
  %2130 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2129, 1
  %2131 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %529, ptr %2131, align 8
  %2132 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2131, 1
  %2133 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2130, ptr %2133, align 8
  %2134 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2132, ptr %2134, align 8
  call void @memrefCopy(i64 4, ptr %2133, ptr %2134)
  call void @llvm.stackrestore.p0(ptr %2128)
  %2135 = call ptr @llvm.stacksave.p0()
  %2136 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %623, ptr %2136, align 8
  %2137 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2136, 1
  %2138 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %567, ptr %2138, align 8
  %2139 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2138, 1
  %2140 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2137, ptr %2140, align 8
  %2141 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2139, ptr %2141, align 8
  call void @memrefCopy(i64 4, ptr %2140, ptr %2141)
  call void @llvm.stackrestore.p0(ptr %2135)
  %2142 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 16, i32 4, ptr %539, ptr %577, ptr %501, i32 1, i32 672, i32 117)
  %2143 = call ptr @llvm.stacksave.p0()
  %2144 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %1986, ptr %2144, align 8
  %2145 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2144, 1
  %2146 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %659, ptr %2146, align 8
  %2147 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2146, 1
  %2148 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2145, ptr %2148, align 8
  %2149 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2147, ptr %2149, align 8
  call void @memrefCopy(i64 4, ptr %2148, ptr %2149)
  call void @llvm.stackrestore.p0(ptr %2143)
  %2150 = call ptr @llvm.stacksave.p0()
  %2151 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %688, ptr %2151, align 8
  %2152 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2151, 1
  %2153 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %697, ptr %2153, align 8
  %2154 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2153, 1
  %2155 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2152, ptr %2155, align 8
  %2156 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2154, ptr %2156, align 8
  call void @memrefCopy(i64 4, ptr %2155, ptr %2156)
  call void @llvm.stackrestore.p0(ptr %2150)
  %2157 = call i32 @fpga_matmul_tiled_auto_scheduled(i32 4, i32 8, i32 4, ptr %669, ptr %707, ptr %501, i32 1, i32 741, i32 109)
  %2158 = call ptr @llvm.stacksave.p0()
  %2159 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %491, ptr %2159, align 8
  %2160 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2159, 1
  %2161 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %2105, ptr %2161, align 8
  %2162 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %2161, 1
  %2163 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2160, ptr %2163, align 8
  %2164 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %2162, ptr %2164, align 8
  call void @memrefCopy(i64 4, ptr %2163, ptr %2164)
  call void @llvm.stackrestore.p0(ptr %2158)
  ret void
}

; Function Attrs: nocallback nofree nosync nounwind willreturn
declare ptr @llvm.stacksave.p0() #0

; Function Attrs: nocallback nofree nosync nounwind willreturn
declare void @llvm.stackrestore.p0(ptr) #0

attributes #0 = { nocallback nofree nosync nounwind willreturn }

!llvm.module.flags = !{!0}

!0 = !{i32 2, !"Debug Info Version", i32 3}
