; ModuleID = 'LLVMDialectModule'
source_filename = "LLVMDialectModule"

declare void @free(ptr)

declare ptr @malloc(i64)

declare void @memrefCopy(i64, ptr, ptr)

declare i32 @fpga_gemm_wait(ptr)

declare i32 @fpga_tile_submit(ptr, i32, i32, i32, i32, i32, i32, ptr, ptr, ptr, i32, i32, i32, i32)

declare ptr @fpga_gemm_begin(i32, i32, i32, i32)

define void @bat__systolic_kernel(ptr %0, ptr %1, i64 %2, i64 %3, i64 %4, i64 %5, i64 %6, i64 %7, i64 %8, ptr %9, ptr %10, i64 %11, i64 %12, i64 %13, i64 %14, i64 %15, i64 %16, i64 %17, ptr %18, ptr %19, i64 %20, i64 %21, i64 %22, i64 %23, i64 %24, i64 %25, i64 %26) {
  %28 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } poison, ptr %18, 0
  %29 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %28, ptr %19, 1
  %30 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %29, i64 %20, 2
  %31 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %30, i64 %21, 3, 0
  %32 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %31, i64 %24, 4, 0
  %33 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %32, i64 %22, 3, 1
  %34 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %33, i64 %25, 4, 1
  %35 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %34, i64 %23, 3, 2
  %36 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %35, i64 %26, 4, 2
  %37 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } poison, ptr %9, 0
  %38 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %37, ptr %10, 1
  %39 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %38, i64 %11, 2
  %40 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %39, i64 %12, 3, 0
  %41 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %40, i64 %15, 4, 0
  %42 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %41, i64 %13, 3, 1
  %43 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %42, i64 %16, 4, 1
  %44 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %43, i64 %14, 3, 2
  %45 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %44, i64 %17, 4, 2
  %46 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } poison, ptr %0, 0
  %47 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %46, ptr %1, 1
  %48 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %47, i64 %2, 2
  %49 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %48, i64 %3, 3, 0
  %50 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %49, i64 %6, 4, 0
  %51 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %50, i64 %4, 3, 1
  %52 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %51, i64 %7, 4, 1
  %53 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %52, i64 %5, 3, 2
  %54 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %53, i64 %8, 4, 2
  %55 = alloca float, i64 128, align 4
  %56 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } poison, ptr %55, 0
  %57 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %56, ptr %55, 1
  %58 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %57, i64 0, 2
  %59 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %58, i64 128, 3, 0
  %60 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %59, i64 1, 4, 0
  %61 = alloca float, i64 128, align 4
  %62 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } poison, ptr %61, 0
  %63 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %62, ptr %61, 1
  %64 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %63, i64 0, 2
  %65 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %64, i64 128, 3, 0
  %66 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %65, i64 1, 4, 0
  br label %67

67:                                               ; preds = %70, %27
  %68 = phi i64 [ %182, %70 ], [ 0, %27 ]
  %69 = icmp slt i64 %68, 2
  br i1 %69, label %70, label %183

70:                                               ; preds = %67
  %71 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 0
  %72 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 1
  %73 = insertvalue { ptr, ptr, i64 } poison, ptr %71, 0
  %74 = insertvalue { ptr, ptr, i64 } %73, ptr %72, 1
  %75 = insertvalue { ptr, ptr, i64 } %74, i64 0, 2
  %76 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 2
  %77 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 3, 0
  %78 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 3, 1
  %79 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 3, 2
  %80 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 4, 0
  %81 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 4, 1
  %82 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 4, 2
  %83 = mul nsw i64 %68, 16
  %84 = extractvalue { ptr, ptr, i64 } %75, 0
  %85 = extractvalue { ptr, ptr, i64 } %75, 1
  %86 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %84, 0
  %87 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %86, ptr %85, 1
  %88 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %87, i64 %83, 2
  %89 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, i64 4, 3, 0
  %90 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %89, i64 4, 4, 0
  %91 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %90, i64 4, 3, 1
  %92 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %91, i64 1, 4, 1
  %93 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 0
  %94 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 1
  %95 = insertvalue { ptr, ptr, i64 } poison, ptr %93, 0
  %96 = insertvalue { ptr, ptr, i64 } %95, ptr %94, 1
  %97 = insertvalue { ptr, ptr, i64 } %96, i64 0, 2
  %98 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 2
  %99 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 3, 0
  %100 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 3, 1
  %101 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 3, 2
  %102 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 4, 0
  %103 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 4, 1
  %104 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 4, 2
  %105 = mul nsw i64 %68, 16
  %106 = extractvalue { ptr, ptr, i64 } %97, 0
  %107 = extractvalue { ptr, ptr, i64 } %97, 1
  %108 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %106, 0
  %109 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %108, ptr %107, 1
  %110 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %109, i64 %105, 2
  %111 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %110, i64 4, 3, 0
  %112 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %111, i64 4, 4, 0
  %113 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %112, i64 4, 3, 1
  %114 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %113, i64 1, 4, 1
  %115 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 0
  %116 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 1
  %117 = insertvalue { ptr, ptr, i64 } poison, ptr %115, 0
  %118 = insertvalue { ptr, ptr, i64 } %117, ptr %116, 1
  %119 = insertvalue { ptr, ptr, i64 } %118, i64 0, 2
  %120 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 2
  %121 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 3, 0
  %122 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 3, 1
  %123 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 3, 2
  %124 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 4, 0
  %125 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 4, 1
  %126 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 4, 2
  %127 = mul nsw i64 %68, 16
  %128 = extractvalue { ptr, ptr, i64 } %119, 1
  %129 = ptrtoint ptr %128 to i64
  %130 = mul i64 %127, 4
  %131 = add i64 %129, %130
  %132 = inttoptr i64 %131 to ptr
  %133 = call ptr @fpga_gemm_begin(i32 4, i32 4, i32 4, i32 1)
  %134 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 0
  %135 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 1
  %136 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %134, 0
  %137 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %136, ptr %135, 1
  %138 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %137, i64 0, 2
  %139 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %138, i64 4, 3, 0
  %140 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %139, i64 4, 4, 0
  %141 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %140, i64 4, 3, 1
  %142 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %141, i64 1, 4, 1
  %143 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %92, 3, 0
  %144 = mul i64 1, %143
  %145 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %92, 3, 1
  %146 = mul i64 %144, %145
  %147 = mul i64 %146, 4
  %148 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %92, 1
  %149 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %92, 2
  %150 = getelementptr float, ptr %148, i64 %149
  %151 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %142, 1
  %152 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %142, 2
  %153 = getelementptr float, ptr %151, i64 %152
  call void @llvm.memcpy.p0.p0.i64(ptr %153, ptr %150, i64 %147, i1 false)
  %154 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 1
  %155 = ptrtoint ptr %154 to i64
  %156 = inttoptr i64 %155 to ptr
  %157 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %66, 0
  %158 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %66, 1
  %159 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %157, 0
  %160 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %159, ptr %158, 1
  %161 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %160, i64 0, 2
  %162 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %161, i64 4, 3, 0
  %163 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %162, i64 4, 4, 0
  %164 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %163, i64 4, 3, 1
  %165 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, i64 1, 4, 1
  %166 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %114, 3, 0
  %167 = mul i64 1, %166
  %168 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %114, 3, 1
  %169 = mul i64 %167, %168
  %170 = mul i64 %169, 4
  %171 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %114, 1
  %172 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %114, 2
  %173 = getelementptr float, ptr %171, i64 %172
  %174 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %165, 1
  %175 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %165, 2
  %176 = getelementptr float, ptr %174, i64 %175
  call void @llvm.memcpy.p0.p0.i64(ptr %176, ptr %173, i64 %170, i1 false)
  %177 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %66, 1
  %178 = ptrtoint ptr %177 to i64
  %179 = inttoptr i64 %178 to ptr
  %180 = call i32 @fpga_tile_submit(ptr %133, i32 0, i32 0, i32 1, i32 4, i32 4, i32 4, ptr %156, ptr %179, ptr %132, i32 4, i32 1, i32 0, i32 105)
  %181 = call i32 @fpga_gemm_wait(ptr %133)
  %182 = add i64 %68, 1
  br label %67

183:                                              ; preds = %67
  ret void
}

define void @bat_mid__systolic_kernel(ptr %0, ptr %1, i64 %2, i64 %3, i64 %4, i64 %5, i64 %6, i64 %7, i64 %8, ptr %9, ptr %10, i64 %11, i64 %12, i64 %13, i64 %14, i64 %15, i64 %16, i64 %17, ptr %18, ptr %19, i64 %20, i64 %21, i64 %22, i64 %23, i64 %24, i64 %25, i64 %26) {
  %28 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } poison, ptr %18, 0
  %29 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %28, ptr %19, 1
  %30 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %29, i64 %20, 2
  %31 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %30, i64 %21, 3, 0
  %32 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %31, i64 %24, 4, 0
  %33 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %32, i64 %22, 3, 1
  %34 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %33, i64 %25, 4, 1
  %35 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %34, i64 %23, 3, 2
  %36 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %35, i64 %26, 4, 2
  %37 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } poison, ptr %9, 0
  %38 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %37, ptr %10, 1
  %39 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %38, i64 %11, 2
  %40 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %39, i64 %12, 3, 0
  %41 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %40, i64 %15, 4, 0
  %42 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %41, i64 %13, 3, 1
  %43 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %42, i64 %16, 4, 1
  %44 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %43, i64 %14, 3, 2
  %45 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %44, i64 %17, 4, 2
  %46 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } poison, ptr %0, 0
  %47 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %46, ptr %1, 1
  %48 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %47, i64 %2, 2
  %49 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %48, i64 %3, 3, 0
  %50 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %49, i64 %6, 4, 0
  %51 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %50, i64 %4, 3, 1
  %52 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %51, i64 %7, 4, 1
  %53 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %52, i64 %5, 3, 2
  %54 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %53, i64 %8, 4, 2
  %55 = alloca float, i64 128, align 4
  %56 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } poison, ptr %55, 0
  %57 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %56, ptr %55, 1
  %58 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %57, i64 0, 2
  %59 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %58, i64 128, 3, 0
  %60 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %59, i64 1, 4, 0
  %61 = alloca float, i64 128, align 4
  %62 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } poison, ptr %61, 0
  %63 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %62, ptr %61, 1
  %64 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %63, i64 0, 2
  %65 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %64, i64 128, 3, 0
  %66 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %65, i64 1, 4, 0
  br label %67

67:                                               ; preds = %70, %27
  %68 = phi i64 [ %182, %70 ], [ 0, %27 ]
  %69 = icmp slt i64 %68, 3
  br i1 %69, label %70, label %183

70:                                               ; preds = %67
  %71 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 0
  %72 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 1
  %73 = insertvalue { ptr, ptr, i64 } poison, ptr %71, 0
  %74 = insertvalue { ptr, ptr, i64 } %73, ptr %72, 1
  %75 = insertvalue { ptr, ptr, i64 } %74, i64 0, 2
  %76 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 2
  %77 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 3, 0
  %78 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 3, 1
  %79 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 3, 2
  %80 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 4, 0
  %81 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 4, 1
  %82 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 4, 2
  %83 = mul nsw i64 %68, 64
  %84 = extractvalue { ptr, ptr, i64 } %75, 0
  %85 = extractvalue { ptr, ptr, i64 } %75, 1
  %86 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %84, 0
  %87 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %86, ptr %85, 1
  %88 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %87, i64 %83, 2
  %89 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, i64 8, 3, 0
  %90 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %89, i64 8, 4, 0
  %91 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %90, i64 8, 3, 1
  %92 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %91, i64 1, 4, 1
  %93 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 0
  %94 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 1
  %95 = insertvalue { ptr, ptr, i64 } poison, ptr %93, 0
  %96 = insertvalue { ptr, ptr, i64 } %95, ptr %94, 1
  %97 = insertvalue { ptr, ptr, i64 } %96, i64 0, 2
  %98 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 2
  %99 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 3, 0
  %100 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 3, 1
  %101 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 3, 2
  %102 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 4, 0
  %103 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 4, 1
  %104 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 4, 2
  %105 = mul nsw i64 %68, 64
  %106 = extractvalue { ptr, ptr, i64 } %97, 0
  %107 = extractvalue { ptr, ptr, i64 } %97, 1
  %108 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %106, 0
  %109 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %108, ptr %107, 1
  %110 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %109, i64 %105, 2
  %111 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %110, i64 8, 3, 0
  %112 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %111, i64 8, 4, 0
  %113 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %112, i64 8, 3, 1
  %114 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %113, i64 1, 4, 1
  %115 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 0
  %116 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 1
  %117 = insertvalue { ptr, ptr, i64 } poison, ptr %115, 0
  %118 = insertvalue { ptr, ptr, i64 } %117, ptr %116, 1
  %119 = insertvalue { ptr, ptr, i64 } %118, i64 0, 2
  %120 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 2
  %121 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 3, 0
  %122 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 3, 1
  %123 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 3, 2
  %124 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 4, 0
  %125 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 4, 1
  %126 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 4, 2
  %127 = mul nsw i64 %68, 64
  %128 = extractvalue { ptr, ptr, i64 } %119, 1
  %129 = ptrtoint ptr %128 to i64
  %130 = mul i64 %127, 4
  %131 = add i64 %129, %130
  %132 = inttoptr i64 %131 to ptr
  %133 = call ptr @fpga_gemm_begin(i32 8, i32 8, i32 8, i32 1)
  %134 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 0
  %135 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 1
  %136 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %134, 0
  %137 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %136, ptr %135, 1
  %138 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %137, i64 0, 2
  %139 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %138, i64 8, 3, 0
  %140 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %139, i64 8, 4, 0
  %141 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %140, i64 8, 3, 1
  %142 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %141, i64 1, 4, 1
  %143 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %92, 3, 0
  %144 = mul i64 1, %143
  %145 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %92, 3, 1
  %146 = mul i64 %144, %145
  %147 = mul i64 %146, 4
  %148 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %92, 1
  %149 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %92, 2
  %150 = getelementptr float, ptr %148, i64 %149
  %151 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %142, 1
  %152 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %142, 2
  %153 = getelementptr float, ptr %151, i64 %152
  call void @llvm.memcpy.p0.p0.i64(ptr %153, ptr %150, i64 %147, i1 false)
  %154 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 1
  %155 = ptrtoint ptr %154 to i64
  %156 = inttoptr i64 %155 to ptr
  %157 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %66, 0
  %158 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %66, 1
  %159 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %157, 0
  %160 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %159, ptr %158, 1
  %161 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %160, i64 0, 2
  %162 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %161, i64 8, 3, 0
  %163 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %162, i64 8, 4, 0
  %164 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %163, i64 8, 3, 1
  %165 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, i64 1, 4, 1
  %166 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %114, 3, 0
  %167 = mul i64 1, %166
  %168 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %114, 3, 1
  %169 = mul i64 %167, %168
  %170 = mul i64 %169, 4
  %171 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %114, 1
  %172 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %114, 2
  %173 = getelementptr float, ptr %171, i64 %172
  %174 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %165, 1
  %175 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %165, 2
  %176 = getelementptr float, ptr %174, i64 %175
  call void @llvm.memcpy.p0.p0.i64(ptr %176, ptr %173, i64 %170, i1 false)
  %177 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %66, 1
  %178 = ptrtoint ptr %177 to i64
  %179 = inttoptr i64 %178 to ptr
  %180 = call i32 @fpga_tile_submit(ptr %133, i32 0, i32 0, i32 1, i32 8, i32 8, i32 8, ptr %156, ptr %179, ptr %132, i32 8, i32 0, i32 0, i32 117)
  %181 = call i32 @fpga_gemm_wait(ptr %133)
  %182 = add i64 %68, 1
  br label %67

183:                                              ; preds = %67
  ret void
}

define void @bat_step__systolic_kernel(ptr %0, ptr %1, i64 %2, i64 %3, i64 %4, i64 %5, i64 %6, i64 %7, i64 %8, ptr %9, ptr %10, i64 %11, i64 %12, i64 %13, i64 %14, i64 %15, i64 %16, i64 %17, ptr %18, ptr %19, i64 %20, i64 %21, i64 %22, i64 %23, i64 %24, i64 %25, i64 %26) {
  %28 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } poison, ptr %18, 0
  %29 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %28, ptr %19, 1
  %30 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %29, i64 %20, 2
  %31 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %30, i64 %21, 3, 0
  %32 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %31, i64 %24, 4, 0
  %33 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %32, i64 %22, 3, 1
  %34 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %33, i64 %25, 4, 1
  %35 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %34, i64 %23, 3, 2
  %36 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %35, i64 %26, 4, 2
  %37 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } poison, ptr %9, 0
  %38 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %37, ptr %10, 1
  %39 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %38, i64 %11, 2
  %40 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %39, i64 %12, 3, 0
  %41 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %40, i64 %15, 4, 0
  %42 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %41, i64 %13, 3, 1
  %43 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %42, i64 %16, 4, 1
  %44 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %43, i64 %14, 3, 2
  %45 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %44, i64 %17, 4, 2
  %46 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } poison, ptr %0, 0
  %47 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %46, ptr %1, 1
  %48 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %47, i64 %2, 2
  %49 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %48, i64 %3, 3, 0
  %50 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %49, i64 %6, 4, 0
  %51 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %50, i64 %4, 3, 1
  %52 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %51, i64 %7, 4, 1
  %53 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %52, i64 %5, 3, 2
  %54 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %53, i64 %8, 4, 2
  %55 = alloca float, i64 128, align 4
  %56 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } poison, ptr %55, 0
  %57 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %56, ptr %55, 1
  %58 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %57, i64 0, 2
  %59 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %58, i64 128, 3, 0
  %60 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %59, i64 1, 4, 0
  %61 = alloca float, i64 128, align 4
  %62 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } poison, ptr %61, 0
  %63 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %62, ptr %61, 1
  %64 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %63, i64 0, 2
  %65 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %64, i64 128, 3, 0
  %66 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %65, i64 1, 4, 0
  br label %67

67:                                               ; preds = %70, %27
  %68 = phi i64 [ %182, %70 ], [ 1, %27 ]
  %69 = icmp slt i64 %68, 7
  br i1 %69, label %70, label %183

70:                                               ; preds = %67
  %71 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 0
  %72 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 1
  %73 = insertvalue { ptr, ptr, i64 } poison, ptr %71, 0
  %74 = insertvalue { ptr, ptr, i64 } %73, ptr %72, 1
  %75 = insertvalue { ptr, ptr, i64 } %74, i64 0, 2
  %76 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 2
  %77 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 3, 0
  %78 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 3, 1
  %79 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 3, 2
  %80 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 4, 0
  %81 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 4, 1
  %82 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 4, 2
  %83 = mul nsw i64 %68, 16
  %84 = extractvalue { ptr, ptr, i64 } %75, 0
  %85 = extractvalue { ptr, ptr, i64 } %75, 1
  %86 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %84, 0
  %87 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %86, ptr %85, 1
  %88 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %87, i64 %83, 2
  %89 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, i64 4, 3, 0
  %90 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %89, i64 4, 4, 0
  %91 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %90, i64 4, 3, 1
  %92 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %91, i64 1, 4, 1
  %93 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 0
  %94 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 1
  %95 = insertvalue { ptr, ptr, i64 } poison, ptr %93, 0
  %96 = insertvalue { ptr, ptr, i64 } %95, ptr %94, 1
  %97 = insertvalue { ptr, ptr, i64 } %96, i64 0, 2
  %98 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 2
  %99 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 3, 0
  %100 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 3, 1
  %101 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 3, 2
  %102 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 4, 0
  %103 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 4, 1
  %104 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 4, 2
  %105 = mul nsw i64 %68, 16
  %106 = extractvalue { ptr, ptr, i64 } %97, 0
  %107 = extractvalue { ptr, ptr, i64 } %97, 1
  %108 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %106, 0
  %109 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %108, ptr %107, 1
  %110 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %109, i64 %105, 2
  %111 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %110, i64 4, 3, 0
  %112 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %111, i64 4, 4, 0
  %113 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %112, i64 4, 3, 1
  %114 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %113, i64 1, 4, 1
  %115 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 0
  %116 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 1
  %117 = insertvalue { ptr, ptr, i64 } poison, ptr %115, 0
  %118 = insertvalue { ptr, ptr, i64 } %117, ptr %116, 1
  %119 = insertvalue { ptr, ptr, i64 } %118, i64 0, 2
  %120 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 2
  %121 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 3, 0
  %122 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 3, 1
  %123 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 3, 2
  %124 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 4, 0
  %125 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 4, 1
  %126 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 4, 2
  %127 = mul nsw i64 %68, 16
  %128 = extractvalue { ptr, ptr, i64 } %119, 1
  %129 = ptrtoint ptr %128 to i64
  %130 = mul i64 %127, 4
  %131 = add i64 %129, %130
  %132 = inttoptr i64 %131 to ptr
  %133 = call ptr @fpga_gemm_begin(i32 4, i32 4, i32 4, i32 1)
  %134 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 0
  %135 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 1
  %136 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %134, 0
  %137 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %136, ptr %135, 1
  %138 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %137, i64 0, 2
  %139 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %138, i64 4, 3, 0
  %140 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %139, i64 4, 4, 0
  %141 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %140, i64 4, 3, 1
  %142 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %141, i64 1, 4, 1
  %143 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %92, 3, 0
  %144 = mul i64 1, %143
  %145 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %92, 3, 1
  %146 = mul i64 %144, %145
  %147 = mul i64 %146, 4
  %148 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %92, 1
  %149 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %92, 2
  %150 = getelementptr float, ptr %148, i64 %149
  %151 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %142, 1
  %152 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %142, 2
  %153 = getelementptr float, ptr %151, i64 %152
  call void @llvm.memcpy.p0.p0.i64(ptr %153, ptr %150, i64 %147, i1 false)
  %154 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 1
  %155 = ptrtoint ptr %154 to i64
  %156 = inttoptr i64 %155 to ptr
  %157 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %66, 0
  %158 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %66, 1
  %159 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %157, 0
  %160 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %159, ptr %158, 1
  %161 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %160, i64 0, 2
  %162 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %161, i64 4, 3, 0
  %163 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %162, i64 4, 4, 0
  %164 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %163, i64 4, 3, 1
  %165 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, i64 1, 4, 1
  %166 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %114, 3, 0
  %167 = mul i64 1, %166
  %168 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %114, 3, 1
  %169 = mul i64 %167, %168
  %170 = mul i64 %169, 4
  %171 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %114, 1
  %172 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %114, 2
  %173 = getelementptr float, ptr %171, i64 %172
  %174 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %165, 1
  %175 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %165, 2
  %176 = getelementptr float, ptr %174, i64 %175
  call void @llvm.memcpy.p0.p0.i64(ptr %176, ptr %173, i64 %170, i1 false)
  %177 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %66, 1
  %178 = ptrtoint ptr %177 to i64
  %179 = inttoptr i64 %178 to ptr
  %180 = call i32 @fpga_tile_submit(ptr %133, i32 0, i32 0, i32 1, i32 4, i32 4, i32 4, ptr %156, ptr %179, ptr %132, i32 4, i32 1, i32 0, i32 105)
  %181 = call i32 @fpga_gemm_wait(ptr %133)
  %182 = add i64 %68, 2
  br label %67

183:                                              ; preds = %67
  ret void
}

define void @bat_two__systolic_kernel(ptr %0, ptr %1, i64 %2, i64 %3, i64 %4, i64 %5, i64 %6, i64 %7, i64 %8, i64 %9, i64 %10, ptr %11, ptr %12, i64 %13, i64 %14, i64 %15, i64 %16, i64 %17, i64 %18, i64 %19, i64 %20, i64 %21, ptr %22, ptr %23, i64 %24, i64 %25, i64 %26, i64 %27, i64 %28, i64 %29, i64 %30, i64 %31, i64 %32) {
  %34 = insertvalue { ptr, ptr, i64, [4 x i64], [4 x i64] } poison, ptr %22, 0
  %35 = insertvalue { ptr, ptr, i64, [4 x i64], [4 x i64] } %34, ptr %23, 1
  %36 = insertvalue { ptr, ptr, i64, [4 x i64], [4 x i64] } %35, i64 %24, 2
  %37 = insertvalue { ptr, ptr, i64, [4 x i64], [4 x i64] } %36, i64 %25, 3, 0
  %38 = insertvalue { ptr, ptr, i64, [4 x i64], [4 x i64] } %37, i64 %29, 4, 0
  %39 = insertvalue { ptr, ptr, i64, [4 x i64], [4 x i64] } %38, i64 %26, 3, 1
  %40 = insertvalue { ptr, ptr, i64, [4 x i64], [4 x i64] } %39, i64 %30, 4, 1
  %41 = insertvalue { ptr, ptr, i64, [4 x i64], [4 x i64] } %40, i64 %27, 3, 2
  %42 = insertvalue { ptr, ptr, i64, [4 x i64], [4 x i64] } %41, i64 %31, 4, 2
  %43 = insertvalue { ptr, ptr, i64, [4 x i64], [4 x i64] } %42, i64 %28, 3, 3
  %44 = insertvalue { ptr, ptr, i64, [4 x i64], [4 x i64] } %43, i64 %32, 4, 3
  %45 = insertvalue { ptr, ptr, i64, [4 x i64], [4 x i64] } poison, ptr %11, 0
  %46 = insertvalue { ptr, ptr, i64, [4 x i64], [4 x i64] } %45, ptr %12, 1
  %47 = insertvalue { ptr, ptr, i64, [4 x i64], [4 x i64] } %46, i64 %13, 2
  %48 = insertvalue { ptr, ptr, i64, [4 x i64], [4 x i64] } %47, i64 %14, 3, 0
  %49 = insertvalue { ptr, ptr, i64, [4 x i64], [4 x i64] } %48, i64 %18, 4, 0
  %50 = insertvalue { ptr, ptr, i64, [4 x i64], [4 x i64] } %49, i64 %15, 3, 1
  %51 = insertvalue { ptr, ptr, i64, [4 x i64], [4 x i64] } %50, i64 %19, 4, 1
  %52 = insertvalue { ptr, ptr, i64, [4 x i64], [4 x i64] } %51, i64 %16, 3, 2
  %53 = insertvalue { ptr, ptr, i64, [4 x i64], [4 x i64] } %52, i64 %20, 4, 2
  %54 = insertvalue { ptr, ptr, i64, [4 x i64], [4 x i64] } %53, i64 %17, 3, 3
  %55 = insertvalue { ptr, ptr, i64, [4 x i64], [4 x i64] } %54, i64 %21, 4, 3
  %56 = insertvalue { ptr, ptr, i64, [4 x i64], [4 x i64] } poison, ptr %0, 0
  %57 = insertvalue { ptr, ptr, i64, [4 x i64], [4 x i64] } %56, ptr %1, 1
  %58 = insertvalue { ptr, ptr, i64, [4 x i64], [4 x i64] } %57, i64 %2, 2
  %59 = insertvalue { ptr, ptr, i64, [4 x i64], [4 x i64] } %58, i64 %3, 3, 0
  %60 = insertvalue { ptr, ptr, i64, [4 x i64], [4 x i64] } %59, i64 %7, 4, 0
  %61 = insertvalue { ptr, ptr, i64, [4 x i64], [4 x i64] } %60, i64 %4, 3, 1
  %62 = insertvalue { ptr, ptr, i64, [4 x i64], [4 x i64] } %61, i64 %8, 4, 1
  %63 = insertvalue { ptr, ptr, i64, [4 x i64], [4 x i64] } %62, i64 %5, 3, 2
  %64 = insertvalue { ptr, ptr, i64, [4 x i64], [4 x i64] } %63, i64 %9, 4, 2
  %65 = insertvalue { ptr, ptr, i64, [4 x i64], [4 x i64] } %64, i64 %6, 3, 3
  %66 = insertvalue { ptr, ptr, i64, [4 x i64], [4 x i64] } %65, i64 %10, 4, 3
  %67 = alloca float, i64 128, align 4
  %68 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } poison, ptr %67, 0
  %69 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %68, ptr %67, 1
  %70 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %69, i64 0, 2
  %71 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %70, i64 128, 3, 0
  %72 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %71, i64 1, 4, 0
  %73 = alloca float, i64 128, align 4
  %74 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } poison, ptr %73, 0
  %75 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %74, ptr %73, 1
  %76 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %75, i64 0, 2
  %77 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %76, i64 128, 3, 0
  %78 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %77, i64 1, 4, 0
  br label %79

79:                                               ; preds = %211, %33
  %80 = phi i64 [ %212, %211 ], [ 0, %33 ]
  %81 = icmp slt i64 %80, 2
  br i1 %81, label %82, label %213

82:                                               ; preds = %79
  br label %83

83:                                               ; preds = %86, %82
  %84 = phi i64 [ %210, %86 ], [ 0, %82 ]
  %85 = icmp slt i64 %84, 3
  br i1 %85, label %86, label %211

86:                                               ; preds = %83
  %87 = extractvalue { ptr, ptr, i64, [4 x i64], [4 x i64] } %66, 0
  %88 = extractvalue { ptr, ptr, i64, [4 x i64], [4 x i64] } %66, 1
  %89 = insertvalue { ptr, ptr, i64 } poison, ptr %87, 0
  %90 = insertvalue { ptr, ptr, i64 } %89, ptr %88, 1
  %91 = insertvalue { ptr, ptr, i64 } %90, i64 0, 2
  %92 = extractvalue { ptr, ptr, i64, [4 x i64], [4 x i64] } %66, 2
  %93 = extractvalue { ptr, ptr, i64, [4 x i64], [4 x i64] } %66, 3, 0
  %94 = extractvalue { ptr, ptr, i64, [4 x i64], [4 x i64] } %66, 3, 1
  %95 = extractvalue { ptr, ptr, i64, [4 x i64], [4 x i64] } %66, 3, 2
  %96 = extractvalue { ptr, ptr, i64, [4 x i64], [4 x i64] } %66, 3, 3
  %97 = extractvalue { ptr, ptr, i64, [4 x i64], [4 x i64] } %66, 4, 0
  %98 = extractvalue { ptr, ptr, i64, [4 x i64], [4 x i64] } %66, 4, 1
  %99 = extractvalue { ptr, ptr, i64, [4 x i64], [4 x i64] } %66, 4, 2
  %100 = extractvalue { ptr, ptr, i64, [4 x i64], [4 x i64] } %66, 4, 3
  %101 = mul nsw i64 %80, 48
  %102 = mul nsw i64 %84, 16
  %103 = add i64 %101, %102
  %104 = extractvalue { ptr, ptr, i64 } %91, 0
  %105 = extractvalue { ptr, ptr, i64 } %91, 1
  %106 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %104, 0
  %107 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %106, ptr %105, 1
  %108 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %107, i64 %103, 2
  %109 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %108, i64 4, 3, 0
  %110 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %109, i64 4, 4, 0
  %111 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %110, i64 4, 3, 1
  %112 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %111, i64 1, 4, 1
  %113 = extractvalue { ptr, ptr, i64, [4 x i64], [4 x i64] } %55, 0
  %114 = extractvalue { ptr, ptr, i64, [4 x i64], [4 x i64] } %55, 1
  %115 = insertvalue { ptr, ptr, i64 } poison, ptr %113, 0
  %116 = insertvalue { ptr, ptr, i64 } %115, ptr %114, 1
  %117 = insertvalue { ptr, ptr, i64 } %116, i64 0, 2
  %118 = extractvalue { ptr, ptr, i64, [4 x i64], [4 x i64] } %55, 2
  %119 = extractvalue { ptr, ptr, i64, [4 x i64], [4 x i64] } %55, 3, 0
  %120 = extractvalue { ptr, ptr, i64, [4 x i64], [4 x i64] } %55, 3, 1
  %121 = extractvalue { ptr, ptr, i64, [4 x i64], [4 x i64] } %55, 3, 2
  %122 = extractvalue { ptr, ptr, i64, [4 x i64], [4 x i64] } %55, 3, 3
  %123 = extractvalue { ptr, ptr, i64, [4 x i64], [4 x i64] } %55, 4, 0
  %124 = extractvalue { ptr, ptr, i64, [4 x i64], [4 x i64] } %55, 4, 1
  %125 = extractvalue { ptr, ptr, i64, [4 x i64], [4 x i64] } %55, 4, 2
  %126 = extractvalue { ptr, ptr, i64, [4 x i64], [4 x i64] } %55, 4, 3
  %127 = mul nsw i64 %80, 48
  %128 = mul nsw i64 %84, 16
  %129 = add i64 %127, %128
  %130 = extractvalue { ptr, ptr, i64 } %117, 0
  %131 = extractvalue { ptr, ptr, i64 } %117, 1
  %132 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %130, 0
  %133 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %132, ptr %131, 1
  %134 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %133, i64 %129, 2
  %135 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %134, i64 4, 3, 0
  %136 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %135, i64 4, 4, 0
  %137 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %136, i64 4, 3, 1
  %138 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %137, i64 1, 4, 1
  %139 = extractvalue { ptr, ptr, i64, [4 x i64], [4 x i64] } %44, 0
  %140 = extractvalue { ptr, ptr, i64, [4 x i64], [4 x i64] } %44, 1
  %141 = insertvalue { ptr, ptr, i64 } poison, ptr %139, 0
  %142 = insertvalue { ptr, ptr, i64 } %141, ptr %140, 1
  %143 = insertvalue { ptr, ptr, i64 } %142, i64 0, 2
  %144 = extractvalue { ptr, ptr, i64, [4 x i64], [4 x i64] } %44, 2
  %145 = extractvalue { ptr, ptr, i64, [4 x i64], [4 x i64] } %44, 3, 0
  %146 = extractvalue { ptr, ptr, i64, [4 x i64], [4 x i64] } %44, 3, 1
  %147 = extractvalue { ptr, ptr, i64, [4 x i64], [4 x i64] } %44, 3, 2
  %148 = extractvalue { ptr, ptr, i64, [4 x i64], [4 x i64] } %44, 3, 3
  %149 = extractvalue { ptr, ptr, i64, [4 x i64], [4 x i64] } %44, 4, 0
  %150 = extractvalue { ptr, ptr, i64, [4 x i64], [4 x i64] } %44, 4, 1
  %151 = extractvalue { ptr, ptr, i64, [4 x i64], [4 x i64] } %44, 4, 2
  %152 = extractvalue { ptr, ptr, i64, [4 x i64], [4 x i64] } %44, 4, 3
  %153 = mul nsw i64 %80, 48
  %154 = mul nsw i64 %84, 16
  %155 = add i64 %153, %154
  %156 = extractvalue { ptr, ptr, i64 } %143, 1
  %157 = ptrtoint ptr %156 to i64
  %158 = mul i64 %155, 4
  %159 = add i64 %157, %158
  %160 = inttoptr i64 %159 to ptr
  %161 = call ptr @fpga_gemm_begin(i32 4, i32 4, i32 4, i32 1)
  %162 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %72, 0
  %163 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %72, 1
  %164 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %162, 0
  %165 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %163, 1
  %166 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %165, i64 0, 2
  %167 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %166, i64 4, 3, 0
  %168 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %167, i64 4, 4, 0
  %169 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %168, i64 4, 3, 1
  %170 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %169, i64 1, 4, 1
  %171 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %112, 3, 0
  %172 = mul i64 1, %171
  %173 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %112, 3, 1
  %174 = mul i64 %172, %173
  %175 = mul i64 %174, 4
  %176 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %112, 1
  %177 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %112, 2
  %178 = getelementptr float, ptr %176, i64 %177
  %179 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %170, 1
  %180 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %170, 2
  %181 = getelementptr float, ptr %179, i64 %180
  call void @llvm.memcpy.p0.p0.i64(ptr %181, ptr %178, i64 %175, i1 false)
  %182 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %72, 1
  %183 = ptrtoint ptr %182 to i64
  %184 = inttoptr i64 %183 to ptr
  %185 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %78, 0
  %186 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %78, 1
  %187 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %185, 0
  %188 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %187, ptr %186, 1
  %189 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %188, i64 0, 2
  %190 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %189, i64 4, 3, 0
  %191 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %190, i64 4, 4, 0
  %192 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %191, i64 4, 3, 1
  %193 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %192, i64 1, 4, 1
  %194 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %138, 3, 0
  %195 = mul i64 1, %194
  %196 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %138, 3, 1
  %197 = mul i64 %195, %196
  %198 = mul i64 %197, 4
  %199 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %138, 1
  %200 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %138, 2
  %201 = getelementptr float, ptr %199, i64 %200
  %202 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %193, 1
  %203 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %193, 2
  %204 = getelementptr float, ptr %202, i64 %203
  call void @llvm.memcpy.p0.p0.i64(ptr %204, ptr %201, i64 %198, i1 false)
  %205 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %78, 1
  %206 = ptrtoint ptr %205 to i64
  %207 = inttoptr i64 %206 to ptr
  %208 = call i32 @fpga_tile_submit(ptr %161, i32 0, i32 0, i32 1, i32 4, i32 4, i32 4, ptr %184, ptr %207, ptr %160, i32 4, i32 1, i32 0, i32 105)
  %209 = call i32 @fpga_gemm_wait(ptr %161)
  %210 = add i64 %84, 1
  br label %83

211:                                              ; preds = %83
  %212 = add i64 %80, 1
  br label %79

213:                                              ; preds = %79
  ret void
}

define void @bat_inner__systolic_kernel(ptr %0, ptr %1, i64 %2, i64 %3, i64 %4, i64 %5, i64 %6, i64 %7, i64 %8, ptr %9, ptr %10, i64 %11, i64 %12, i64 %13, i64 %14, i64 %15, i64 %16, i64 %17, ptr %18, ptr %19, i64 %20, i64 %21, i64 %22, i64 %23, i64 %24, i64 %25, i64 %26) {
  %28 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } poison, ptr %18, 0
  %29 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %28, ptr %19, 1
  %30 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %29, i64 %20, 2
  %31 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %30, i64 %21, 3, 0
  %32 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %31, i64 %24, 4, 0
  %33 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %32, i64 %22, 3, 1
  %34 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %33, i64 %25, 4, 1
  %35 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %34, i64 %23, 3, 2
  %36 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %35, i64 %26, 4, 2
  %37 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } poison, ptr %9, 0
  %38 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %37, ptr %10, 1
  %39 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %38, i64 %11, 2
  %40 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %39, i64 %12, 3, 0
  %41 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %40, i64 %15, 4, 0
  %42 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %41, i64 %13, 3, 1
  %43 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %42, i64 %16, 4, 1
  %44 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %43, i64 %14, 3, 2
  %45 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %44, i64 %17, 4, 2
  %46 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } poison, ptr %0, 0
  %47 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %46, ptr %1, 1
  %48 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %47, i64 %2, 2
  %49 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %48, i64 %3, 3, 0
  %50 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %49, i64 %6, 4, 0
  %51 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %50, i64 %4, 3, 1
  %52 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %51, i64 %7, 4, 1
  %53 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %52, i64 %5, 3, 2
  %54 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %53, i64 %8, 4, 2
  %55 = alloca float, i64 128, align 4
  %56 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } poison, ptr %55, 0
  %57 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %56, ptr %55, 1
  %58 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %57, i64 0, 2
  %59 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %58, i64 128, 3, 0
  %60 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %59, i64 1, 4, 0
  %61 = alloca float, i64 128, align 4
  %62 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } poison, ptr %61, 0
  %63 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %62, ptr %61, 1
  %64 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %63, i64 0, 2
  %65 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %64, i64 128, 3, 0
  %66 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %65, i64 1, 4, 0
  br label %67

67:                                               ; preds = %70, %27
  %68 = phi i64 [ %178, %70 ], [ 0, %27 ]
  %69 = icmp slt i64 %68, 2
  br i1 %69, label %70, label %179

70:                                               ; preds = %67
  %71 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 0
  %72 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 1
  %73 = insertvalue { ptr, ptr, i64 } poison, ptr %71, 0
  %74 = insertvalue { ptr, ptr, i64 } %73, ptr %72, 1
  %75 = insertvalue { ptr, ptr, i64 } %74, i64 0, 2
  %76 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 2
  %77 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 3, 0
  %78 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 3, 1
  %79 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 3, 2
  %80 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 4, 0
  %81 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 4, 1
  %82 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 4, 2
  %83 = mul nsw i64 %68, 4
  %84 = extractvalue { ptr, ptr, i64 } %75, 0
  %85 = extractvalue { ptr, ptr, i64 } %75, 1
  %86 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %84, 0
  %87 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %86, ptr %85, 1
  %88 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %87, i64 %83, 2
  %89 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, i64 4, 3, 0
  %90 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %89, i64 8, 4, 0
  %91 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %90, i64 4, 3, 1
  %92 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %91, i64 1, 4, 1
  %93 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 0
  %94 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 1
  %95 = insertvalue { ptr, ptr, i64 } poison, ptr %93, 0
  %96 = insertvalue { ptr, ptr, i64 } %95, ptr %94, 1
  %97 = insertvalue { ptr, ptr, i64 } %96, i64 0, 2
  %98 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 2
  %99 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 3, 0
  %100 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 3, 1
  %101 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 3, 2
  %102 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 4, 0
  %103 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 4, 1
  %104 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 4, 2
  %105 = mul nsw i64 %68, 16
  %106 = extractvalue { ptr, ptr, i64 } %97, 0
  %107 = extractvalue { ptr, ptr, i64 } %97, 1
  %108 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %106, 0
  %109 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %108, ptr %107, 1
  %110 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %109, i64 %105, 2
  %111 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %110, i64 4, 3, 0
  %112 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %111, i64 4, 4, 0
  %113 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %112, i64 4, 3, 1
  %114 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %113, i64 1, 4, 1
  %115 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 0
  %116 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 1
  %117 = insertvalue { ptr, ptr, i64 } poison, ptr %115, 0
  %118 = insertvalue { ptr, ptr, i64 } %117, ptr %116, 1
  %119 = insertvalue { ptr, ptr, i64 } %118, i64 0, 2
  %120 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 2
  %121 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 3, 0
  %122 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 3, 1
  %123 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 3, 2
  %124 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 4, 0
  %125 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 4, 1
  %126 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 4, 2
  %127 = mul nsw i64 %68, 16
  %128 = extractvalue { ptr, ptr, i64 } %119, 1
  %129 = ptrtoint ptr %128 to i64
  %130 = mul i64 %127, 4
  %131 = add i64 %129, %130
  %132 = inttoptr i64 %131 to ptr
  %133 = call ptr @fpga_gemm_begin(i32 4, i32 4, i32 4, i32 1)
  %134 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 0
  %135 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 1
  %136 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %134, 0
  %137 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %136, ptr %135, 1
  %138 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %137, i64 0, 2
  %139 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %138, i64 4, 3, 0
  %140 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %139, i64 4, 4, 0
  %141 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %140, i64 4, 3, 1
  %142 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %141, i64 1, 4, 1
  %143 = call ptr @llvm.stacksave.p0()
  %144 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %92, ptr %144, align 8
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
  %150 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 1
  %151 = ptrtoint ptr %150 to i64
  %152 = inttoptr i64 %151 to ptr
  %153 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %66, 0
  %154 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %66, 1
  %155 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %153, 0
  %156 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %155, ptr %154, 1
  %157 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %156, i64 0, 2
  %158 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %157, i64 4, 3, 0
  %159 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %158, i64 4, 4, 0
  %160 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %159, i64 4, 3, 1
  %161 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %160, i64 1, 4, 1
  %162 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %114, 3, 0
  %163 = mul i64 1, %162
  %164 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %114, 3, 1
  %165 = mul i64 %163, %164
  %166 = mul i64 %165, 4
  %167 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %114, 1
  %168 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %114, 2
  %169 = getelementptr float, ptr %167, i64 %168
  %170 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %161, 1
  %171 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %161, 2
  %172 = getelementptr float, ptr %170, i64 %171
  call void @llvm.memcpy.p0.p0.i64(ptr %172, ptr %169, i64 %166, i1 false)
  %173 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %66, 1
  %174 = ptrtoint ptr %173 to i64
  %175 = inttoptr i64 %174 to ptr
  %176 = call i32 @fpga_tile_submit(ptr %133, i32 0, i32 0, i32 1, i32 4, i32 4, i32 4, ptr %152, ptr %175, ptr %132, i32 4, i32 1, i32 0, i32 105)
  %177 = call i32 @fpga_gemm_wait(ptr %133)
  %178 = add i64 %68, 1
  br label %67

179:                                              ; preds = %67
  ret void
}

define void @bat_tr_neg__systolic_kernel(ptr %0, ptr %1, i64 %2, i64 %3, i64 %4, i64 %5, i64 %6, i64 %7, i64 %8, ptr %9, ptr %10, i64 %11, i64 %12, i64 %13, i64 %14, i64 %15, i64 %16, i64 %17, ptr %18, ptr %19, i64 %20, i64 %21, i64 %22, i64 %23, i64 %24, i64 %25, i64 %26) {
  %28 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } poison, ptr %18, 0
  %29 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %28, ptr %19, 1
  %30 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %29, i64 %20, 2
  %31 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %30, i64 %21, 3, 0
  %32 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %31, i64 %24, 4, 0
  %33 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %32, i64 %22, 3, 1
  %34 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %33, i64 %25, 4, 1
  %35 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %34, i64 %23, 3, 2
  %36 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %35, i64 %26, 4, 2
  %37 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } poison, ptr %9, 0
  %38 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %37, ptr %10, 1
  %39 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %38, i64 %11, 2
  %40 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %39, i64 %12, 3, 0
  %41 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %40, i64 %15, 4, 0
  %42 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %41, i64 %13, 3, 1
  %43 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %42, i64 %16, 4, 1
  %44 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %43, i64 %14, 3, 2
  %45 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %44, i64 %17, 4, 2
  %46 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } poison, ptr %0, 0
  %47 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %46, ptr %1, 1
  %48 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %47, i64 %2, 2
  %49 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %48, i64 %3, 3, 0
  %50 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %49, i64 %6, 4, 0
  %51 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %50, i64 %4, 3, 1
  %52 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %51, i64 %7, 4, 1
  %53 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %52, i64 %5, 3, 2
  %54 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %53, i64 %8, 4, 2
  %55 = alloca float, i64 128, align 4
  %56 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } poison, ptr %55, 0
  %57 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %56, ptr %55, 1
  %58 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %57, i64 0, 2
  %59 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %58, i64 128, 3, 0
  %60 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %59, i64 1, 4, 0
  %61 = alloca float, i64 128, align 4
  %62 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } poison, ptr %61, 0
  %63 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %62, ptr %61, 1
  %64 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %63, i64 0, 2
  %65 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %64, i64 128, 3, 0
  %66 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %65, i64 1, 4, 0
  br label %67

67:                                               ; preds = %70, %27
  %68 = phi i64 [ %228, %70 ], [ 0, %27 ]
  %69 = icmp slt i64 %68, 2
  br i1 %69, label %70, label %229

70:                                               ; preds = %67
  %71 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 0
  %72 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 1
  %73 = insertvalue { ptr, ptr, i64 } poison, ptr %71, 0
  %74 = insertvalue { ptr, ptr, i64 } %73, ptr %72, 1
  %75 = insertvalue { ptr, ptr, i64 } %74, i64 0, 2
  %76 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 2
  %77 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 3, 0
  %78 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 3, 1
  %79 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 3, 2
  %80 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 4, 0
  %81 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 4, 1
  %82 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 4, 2
  %83 = mul nsw i64 %68, 64
  %84 = extractvalue { ptr, ptr, i64 } %75, 0
  %85 = extractvalue { ptr, ptr, i64 } %75, 1
  %86 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %84, 0
  %87 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %86, ptr %85, 1
  %88 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %87, i64 %83, 2
  %89 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, i64 8, 3, 0
  %90 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %89, i64 8, 4, 0
  %91 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %90, i64 8, 3, 1
  %92 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %91, i64 1, 4, 1
  %93 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %92, 0
  %94 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %93, 0
  %95 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %92, 1
  %96 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %94, ptr %95, 1
  %97 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %92, 2
  %98 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %96, i64 %97, 2
  %99 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %92, 3, 1
  %100 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %98, i64 %99, 3, 0
  %101 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %92, 4, 1
  %102 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %100, i64 %101, 4, 0
  %103 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %92, 3, 0
  %104 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %102, i64 %103, 3, 1
  %105 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %92, 4, 0
  %106 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %104, i64 %105, 4, 1
  %107 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 0
  %108 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 1
  %109 = insertvalue { ptr, ptr, i64 } poison, ptr %107, 0
  %110 = insertvalue { ptr, ptr, i64 } %109, ptr %108, 1
  %111 = insertvalue { ptr, ptr, i64 } %110, i64 0, 2
  %112 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 2
  %113 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 3, 0
  %114 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 3, 1
  %115 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 3, 2
  %116 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 4, 0
  %117 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 4, 1
  %118 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 4, 2
  %119 = mul nsw i64 %68, 64
  %120 = extractvalue { ptr, ptr, i64 } %111, 0
  %121 = extractvalue { ptr, ptr, i64 } %111, 1
  %122 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %120, 0
  %123 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %122, ptr %121, 1
  %124 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %123, i64 %119, 2
  %125 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %124, i64 8, 3, 0
  %126 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %125, i64 8, 4, 0
  %127 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, i64 8, 3, 1
  %128 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %127, i64 1, 4, 1
  %129 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 0
  %130 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 1
  %131 = insertvalue { ptr, ptr, i64 } poison, ptr %129, 0
  %132 = insertvalue { ptr, ptr, i64 } %131, ptr %130, 1
  %133 = insertvalue { ptr, ptr, i64 } %132, i64 0, 2
  %134 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 2
  %135 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 3, 0
  %136 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 3, 1
  %137 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 3, 2
  %138 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 4, 0
  %139 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 4, 1
  %140 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 4, 2
  %141 = mul nsw i64 %68, 64
  %142 = add i64 %141, 7
  %143 = extractvalue { ptr, ptr, i64 } %133, 0
  %144 = extractvalue { ptr, ptr, i64 } %133, 1
  %145 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %143, 0
  %146 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %145, ptr %144, 1
  %147 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %146, i64 %142, 2
  %148 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %147, i64 8, 3, 0
  %149 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %148, i64 8, 4, 0
  %150 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %149, i64 8, 3, 1
  %151 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %150, i64 -1, 4, 1
  %152 = call ptr @fpga_gemm_begin(i32 8, i32 8, i32 8, i32 1)
  %153 = call ptr @malloc(i64 320)
  %154 = ptrtoint ptr %153 to i64
  %155 = add i64 %154, 63
  %156 = urem i64 %155, 64
  %157 = sub i64 %155, %156
  %158 = inttoptr i64 %157 to ptr
  %159 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %153, 0
  %160 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %159, ptr %158, 1
  %161 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %160, i64 0, 2
  %162 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %161, i64 8, 3, 0
  %163 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %162, i64 8, 3, 1
  %164 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %163, i64 8, 4, 0
  %165 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, i64 1, 4, 1
  %166 = call ptr @llvm.stacksave.p0()
  %167 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %151, ptr %167, align 8
  %168 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %167, 1
  %169 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %165, ptr %169, align 8
  %170 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %169, 1
  %171 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %168, ptr %171, align 8
  %172 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %170, ptr %172, align 8
  call void @memrefCopy(i64 4, ptr %171, ptr %172)
  call void @llvm.stackrestore.p0(ptr %166)
  %173 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %165, 1
  %174 = ptrtoint ptr %173 to i64
  %175 = inttoptr i64 %174 to ptr
  %176 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 0
  %177 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 1
  %178 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %176, 0
  %179 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %178, ptr %177, 1
  %180 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %179, i64 0, 2
  %181 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %180, i64 8, 3, 0
  %182 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %181, i64 8, 4, 0
  %183 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %182, i64 8, 3, 1
  %184 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %183, i64 1, 4, 1
  %185 = call ptr @llvm.stacksave.p0()
  %186 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %106, ptr %186, align 8
  %187 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %186, 1
  %188 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %184, ptr %188, align 8
  %189 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %188, 1
  %190 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %187, ptr %190, align 8
  %191 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %189, ptr %191, align 8
  call void @memrefCopy(i64 4, ptr %190, ptr %191)
  call void @llvm.stackrestore.p0(ptr %185)
  %192 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 1
  %193 = ptrtoint ptr %192 to i64
  %194 = inttoptr i64 %193 to ptr
  %195 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %66, 0
  %196 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %66, 1
  %197 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %195, 0
  %198 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %197, ptr %196, 1
  %199 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %198, i64 0, 2
  %200 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %199, i64 8, 3, 0
  %201 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %200, i64 8, 4, 0
  %202 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %201, i64 8, 3, 1
  %203 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %202, i64 1, 4, 1
  %204 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %128, 3, 0
  %205 = mul i64 1, %204
  %206 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %128, 3, 1
  %207 = mul i64 %205, %206
  %208 = mul i64 %207, 4
  %209 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %128, 1
  %210 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %128, 2
  %211 = getelementptr float, ptr %209, i64 %210
  %212 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %203, 1
  %213 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %203, 2
  %214 = getelementptr float, ptr %212, i64 %213
  call void @llvm.memcpy.p0.p0.i64(ptr %214, ptr %211, i64 %208, i1 false)
  %215 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %66, 1
  %216 = ptrtoint ptr %215 to i64
  %217 = inttoptr i64 %216 to ptr
  %218 = call i32 @fpga_tile_submit(ptr %152, i32 0, i32 0, i32 1, i32 8, i32 8, i32 8, ptr %194, ptr %217, ptr %175, i32 8, i32 0, i32 0, i32 117)
  %219 = call i32 @fpga_gemm_wait(ptr %152)
  %220 = call ptr @llvm.stacksave.p0()
  %221 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %165, ptr %221, align 8
  %222 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %221, 1
  %223 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %151, ptr %223, align 8
  %224 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %223, 1
  %225 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %222, ptr %225, align 8
  %226 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %224, ptr %226, align 8
  call void @memrefCopy(i64 4, ptr %225, ptr %226)
  call void @llvm.stackrestore.p0(ptr %220)
  %227 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %165, 0
  call void @free(ptr %227)
  %228 = add i64 %68, 1
  br label %67

229:                                              ; preds = %67
  ret void
}

define void @bat_down__systolic_kernel(ptr %0, ptr %1, i64 %2, i64 %3, i64 %4, i64 %5, i64 %6, i64 %7, i64 %8, ptr %9, ptr %10, i64 %11, i64 %12, i64 %13, i64 %14, i64 %15, i64 %16, i64 %17, ptr %18, ptr %19, i64 %20, i64 %21, i64 %22, i64 %23, i64 %24, i64 %25, i64 %26) {
  %28 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } poison, ptr %18, 0
  %29 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %28, ptr %19, 1
  %30 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %29, i64 %20, 2
  %31 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %30, i64 %21, 3, 0
  %32 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %31, i64 %24, 4, 0
  %33 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %32, i64 %22, 3, 1
  %34 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %33, i64 %25, 4, 1
  %35 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %34, i64 %23, 3, 2
  %36 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %35, i64 %26, 4, 2
  %37 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } poison, ptr %9, 0
  %38 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %37, ptr %10, 1
  %39 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %38, i64 %11, 2
  %40 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %39, i64 %12, 3, 0
  %41 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %40, i64 %15, 4, 0
  %42 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %41, i64 %13, 3, 1
  %43 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %42, i64 %16, 4, 1
  %44 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %43, i64 %14, 3, 2
  %45 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %44, i64 %17, 4, 2
  %46 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } poison, ptr %0, 0
  %47 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %46, ptr %1, 1
  %48 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %47, i64 %2, 2
  %49 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %48, i64 %3, 3, 0
  %50 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %49, i64 %6, 4, 0
  %51 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %50, i64 %4, 3, 1
  %52 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %51, i64 %7, 4, 1
  %53 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %52, i64 %5, 3, 2
  %54 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %53, i64 %8, 4, 2
  %55 = alloca float, i64 128, align 4
  %56 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } poison, ptr %55, 0
  %57 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %56, ptr %55, 1
  %58 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %57, i64 0, 2
  %59 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %58, i64 128, 3, 0
  %60 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %59, i64 1, 4, 0
  %61 = alloca float, i64 128, align 4
  %62 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } poison, ptr %61, 0
  %63 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %62, ptr %61, 1
  %64 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %63, i64 0, 2
  %65 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %64, i64 128, 3, 0
  %66 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %65, i64 1, 4, 0
  br label %67

67:                                               ; preds = %70, %27
  %68 = phi i64 [ %184, %70 ], [ 0, %27 ]
  %69 = icmp slt i64 %68, 4
  br i1 %69, label %70, label %185

70:                                               ; preds = %67
  %71 = mul nsw i64 %68, -1
  %72 = add i64 %71, 3
  %73 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 0
  %74 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 1
  %75 = insertvalue { ptr, ptr, i64 } poison, ptr %73, 0
  %76 = insertvalue { ptr, ptr, i64 } %75, ptr %74, 1
  %77 = insertvalue { ptr, ptr, i64 } %76, i64 0, 2
  %78 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 2
  %79 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 3, 0
  %80 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 3, 1
  %81 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 3, 2
  %82 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 4, 0
  %83 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 4, 1
  %84 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 4, 2
  %85 = mul nsw i64 %72, 16
  %86 = extractvalue { ptr, ptr, i64 } %77, 0
  %87 = extractvalue { ptr, ptr, i64 } %77, 1
  %88 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %86, 0
  %89 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, ptr %87, 1
  %90 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %89, i64 %85, 2
  %91 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %90, i64 4, 3, 0
  %92 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %91, i64 4, 4, 0
  %93 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %92, i64 4, 3, 1
  %94 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %93, i64 1, 4, 1
  %95 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 0
  %96 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 1
  %97 = insertvalue { ptr, ptr, i64 } poison, ptr %95, 0
  %98 = insertvalue { ptr, ptr, i64 } %97, ptr %96, 1
  %99 = insertvalue { ptr, ptr, i64 } %98, i64 0, 2
  %100 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 2
  %101 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 3, 0
  %102 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 3, 1
  %103 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 3, 2
  %104 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 4, 0
  %105 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 4, 1
  %106 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 4, 2
  %107 = mul nsw i64 %72, 16
  %108 = extractvalue { ptr, ptr, i64 } %99, 0
  %109 = extractvalue { ptr, ptr, i64 } %99, 1
  %110 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %108, 0
  %111 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %110, ptr %109, 1
  %112 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %111, i64 %107, 2
  %113 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %112, i64 4, 3, 0
  %114 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %113, i64 4, 4, 0
  %115 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %114, i64 4, 3, 1
  %116 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %115, i64 1, 4, 1
  %117 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 0
  %118 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 1
  %119 = insertvalue { ptr, ptr, i64 } poison, ptr %117, 0
  %120 = insertvalue { ptr, ptr, i64 } %119, ptr %118, 1
  %121 = insertvalue { ptr, ptr, i64 } %120, i64 0, 2
  %122 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 2
  %123 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 3, 0
  %124 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 3, 1
  %125 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 3, 2
  %126 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 4, 0
  %127 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 4, 1
  %128 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 4, 2
  %129 = mul nsw i64 %72, 16
  %130 = extractvalue { ptr, ptr, i64 } %121, 1
  %131 = ptrtoint ptr %130 to i64
  %132 = mul i64 %129, 4
  %133 = add i64 %131, %132
  %134 = inttoptr i64 %133 to ptr
  %135 = call ptr @fpga_gemm_begin(i32 4, i32 4, i32 4, i32 1)
  %136 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 0
  %137 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 1
  %138 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %136, 0
  %139 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %138, ptr %137, 1
  %140 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %139, i64 0, 2
  %141 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %140, i64 4, 3, 0
  %142 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %141, i64 4, 4, 0
  %143 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %142, i64 4, 3, 1
  %144 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %143, i64 1, 4, 1
  %145 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %94, 3, 0
  %146 = mul i64 1, %145
  %147 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %94, 3, 1
  %148 = mul i64 %146, %147
  %149 = mul i64 %148, 4
  %150 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %94, 1
  %151 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %94, 2
  %152 = getelementptr float, ptr %150, i64 %151
  %153 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %144, 1
  %154 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %144, 2
  %155 = getelementptr float, ptr %153, i64 %154
  call void @llvm.memcpy.p0.p0.i64(ptr %155, ptr %152, i64 %149, i1 false)
  %156 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 1
  %157 = ptrtoint ptr %156 to i64
  %158 = inttoptr i64 %157 to ptr
  %159 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %66, 0
  %160 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %66, 1
  %161 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %159, 0
  %162 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %161, ptr %160, 1
  %163 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %162, i64 0, 2
  %164 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %163, i64 4, 3, 0
  %165 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, i64 4, 4, 0
  %166 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %165, i64 4, 3, 1
  %167 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %166, i64 1, 4, 1
  %168 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %116, 3, 0
  %169 = mul i64 1, %168
  %170 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %116, 3, 1
  %171 = mul i64 %169, %170
  %172 = mul i64 %171, 4
  %173 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %116, 1
  %174 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %116, 2
  %175 = getelementptr float, ptr %173, i64 %174
  %176 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %167, 1
  %177 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %167, 2
  %178 = getelementptr float, ptr %176, i64 %177
  call void @llvm.memcpy.p0.p0.i64(ptr %178, ptr %175, i64 %172, i1 false)
  %179 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %66, 1
  %180 = ptrtoint ptr %179 to i64
  %181 = inttoptr i64 %180 to ptr
  %182 = call i32 @fpga_tile_submit(ptr %135, i32 0, i32 0, i32 1, i32 4, i32 4, i32 4, ptr %158, ptr %181, ptr %134, i32 4, i32 1, i32 0, i32 105)
  %183 = call i32 @fpga_gemm_wait(ptr %135)
  %184 = add i64 %68, 1
  br label %67

185:                                              ; preds = %67
  ret void
}

define void @bat_offset__systolic_kernel(ptr %0, ptr %1, i64 %2, i64 %3, i64 %4, i64 %5, i64 %6, i64 %7, i64 %8, ptr %9, ptr %10, i64 %11, i64 %12, i64 %13, i64 %14, i64 %15, i64 %16, i64 %17, ptr %18, ptr %19, i64 %20, i64 %21, i64 %22, i64 %23, i64 %24, i64 %25, i64 %26) {
  %28 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } poison, ptr %18, 0
  %29 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %28, ptr %19, 1
  %30 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %29, i64 %20, 2
  %31 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %30, i64 %21, 3, 0
  %32 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %31, i64 %24, 4, 0
  %33 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %32, i64 %22, 3, 1
  %34 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %33, i64 %25, 4, 1
  %35 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %34, i64 %23, 3, 2
  %36 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %35, i64 %26, 4, 2
  %37 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } poison, ptr %9, 0
  %38 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %37, ptr %10, 1
  %39 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %38, i64 %11, 2
  %40 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %39, i64 %12, 3, 0
  %41 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %40, i64 %15, 4, 0
  %42 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %41, i64 %13, 3, 1
  %43 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %42, i64 %16, 4, 1
  %44 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %43, i64 %14, 3, 2
  %45 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %44, i64 %17, 4, 2
  %46 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } poison, ptr %0, 0
  %47 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %46, ptr %1, 1
  %48 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %47, i64 %2, 2
  %49 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %48, i64 %3, 3, 0
  %50 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %49, i64 %6, 4, 0
  %51 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %50, i64 %4, 3, 1
  %52 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %51, i64 %7, 4, 1
  %53 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %52, i64 %5, 3, 2
  %54 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %53, i64 %8, 4, 2
  %55 = alloca float, i64 128, align 4
  %56 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } poison, ptr %55, 0
  %57 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %56, ptr %55, 1
  %58 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %57, i64 0, 2
  %59 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %58, i64 128, 3, 0
  %60 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %59, i64 1, 4, 0
  %61 = alloca float, i64 128, align 4
  %62 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } poison, ptr %61, 0
  %63 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %62, ptr %61, 1
  %64 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %63, i64 0, 2
  %65 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %64, i64 128, 3, 0
  %66 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %65, i64 1, 4, 0
  br label %67

67:                                               ; preds = %70, %27
  %68 = phi i64 [ %183, %70 ], [ 0, %27 ]
  %69 = icmp slt i64 %68, 3
  br i1 %69, label %70, label %184

70:                                               ; preds = %67
  %71 = add i64 %68, 2
  %72 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 0
  %73 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 1
  %74 = insertvalue { ptr, ptr, i64 } poison, ptr %72, 0
  %75 = insertvalue { ptr, ptr, i64 } %74, ptr %73, 1
  %76 = insertvalue { ptr, ptr, i64 } %75, i64 0, 2
  %77 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 2
  %78 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 3, 0
  %79 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 3, 1
  %80 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 3, 2
  %81 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 4, 0
  %82 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 4, 1
  %83 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 4, 2
  %84 = mul nsw i64 %71, 16
  %85 = extractvalue { ptr, ptr, i64 } %76, 0
  %86 = extractvalue { ptr, ptr, i64 } %76, 1
  %87 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %85, 0
  %88 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %87, ptr %86, 1
  %89 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, i64 %84, 2
  %90 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %89, i64 4, 3, 0
  %91 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %90, i64 4, 4, 0
  %92 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %91, i64 4, 3, 1
  %93 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %92, i64 1, 4, 1
  %94 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 0
  %95 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 1
  %96 = insertvalue { ptr, ptr, i64 } poison, ptr %94, 0
  %97 = insertvalue { ptr, ptr, i64 } %96, ptr %95, 1
  %98 = insertvalue { ptr, ptr, i64 } %97, i64 0, 2
  %99 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 2
  %100 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 3, 0
  %101 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 3, 1
  %102 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 3, 2
  %103 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 4, 0
  %104 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 4, 1
  %105 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 4, 2
  %106 = mul nsw i64 %71, 16
  %107 = extractvalue { ptr, ptr, i64 } %98, 0
  %108 = extractvalue { ptr, ptr, i64 } %98, 1
  %109 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %107, 0
  %110 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %109, ptr %108, 1
  %111 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %110, i64 %106, 2
  %112 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %111, i64 4, 3, 0
  %113 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %112, i64 4, 4, 0
  %114 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %113, i64 4, 3, 1
  %115 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %114, i64 1, 4, 1
  %116 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 0
  %117 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 1
  %118 = insertvalue { ptr, ptr, i64 } poison, ptr %116, 0
  %119 = insertvalue { ptr, ptr, i64 } %118, ptr %117, 1
  %120 = insertvalue { ptr, ptr, i64 } %119, i64 0, 2
  %121 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 2
  %122 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 3, 0
  %123 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 3, 1
  %124 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 3, 2
  %125 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 4, 0
  %126 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 4, 1
  %127 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 4, 2
  %128 = mul nsw i64 %71, 16
  %129 = extractvalue { ptr, ptr, i64 } %120, 1
  %130 = ptrtoint ptr %129 to i64
  %131 = mul i64 %128, 4
  %132 = add i64 %130, %131
  %133 = inttoptr i64 %132 to ptr
  %134 = call ptr @fpga_gemm_begin(i32 4, i32 4, i32 4, i32 1)
  %135 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 0
  %136 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 1
  %137 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %135, 0
  %138 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %137, ptr %136, 1
  %139 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %138, i64 0, 2
  %140 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %139, i64 4, 3, 0
  %141 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %140, i64 4, 4, 0
  %142 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %141, i64 4, 3, 1
  %143 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %142, i64 1, 4, 1
  %144 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %93, 3, 0
  %145 = mul i64 1, %144
  %146 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %93, 3, 1
  %147 = mul i64 %145, %146
  %148 = mul i64 %147, 4
  %149 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %93, 1
  %150 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %93, 2
  %151 = getelementptr float, ptr %149, i64 %150
  %152 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %143, 1
  %153 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %143, 2
  %154 = getelementptr float, ptr %152, i64 %153
  call void @llvm.memcpy.p0.p0.i64(ptr %154, ptr %151, i64 %148, i1 false)
  %155 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 1
  %156 = ptrtoint ptr %155 to i64
  %157 = inttoptr i64 %156 to ptr
  %158 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %66, 0
  %159 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %66, 1
  %160 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %158, 0
  %161 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %160, ptr %159, 1
  %162 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %161, i64 0, 2
  %163 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %162, i64 4, 3, 0
  %164 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %163, i64 4, 4, 0
  %165 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, i64 4, 3, 1
  %166 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %165, i64 1, 4, 1
  %167 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %115, 3, 0
  %168 = mul i64 1, %167
  %169 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %115, 3, 1
  %170 = mul i64 %168, %169
  %171 = mul i64 %170, 4
  %172 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %115, 1
  %173 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %115, 2
  %174 = getelementptr float, ptr %172, i64 %173
  %175 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %166, 1
  %176 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %166, 2
  %177 = getelementptr float, ptr %175, i64 %176
  call void @llvm.memcpy.p0.p0.i64(ptr %177, ptr %174, i64 %171, i1 false)
  %178 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %66, 1
  %179 = ptrtoint ptr %178 to i64
  %180 = inttoptr i64 %179 to ptr
  %181 = call i32 @fpga_tile_submit(ptr %134, i32 0, i32 0, i32 1, i32 4, i32 4, i32 4, ptr %157, ptr %180, ptr %133, i32 4, i32 1, i32 0, i32 105)
  %182 = call i32 @fpga_gemm_wait(ptr %134)
  %183 = add i64 %68, 1
  br label %67

184:                                              ; preds = %67
  ret void
}

define void @bat_tiles__systolic_kernel(ptr %0, ptr %1, i64 %2, i64 %3, i64 %4, i64 %5, i64 %6, i64 %7, i64 %8, ptr %9, ptr %10, i64 %11, i64 %12, i64 %13, i64 %14, i64 %15, i64 %16, i64 %17, ptr %18, ptr %19, i64 %20, i64 %21, i64 %22, i64 %23, i64 %24, i64 %25, i64 %26) {
  %28 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } poison, ptr %18, 0
  %29 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %28, ptr %19, 1
  %30 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %29, i64 %20, 2
  %31 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %30, i64 %21, 3, 0
  %32 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %31, i64 %24, 4, 0
  %33 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %32, i64 %22, 3, 1
  %34 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %33, i64 %25, 4, 1
  %35 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %34, i64 %23, 3, 2
  %36 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %35, i64 %26, 4, 2
  %37 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } poison, ptr %9, 0
  %38 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %37, ptr %10, 1
  %39 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %38, i64 %11, 2
  %40 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %39, i64 %12, 3, 0
  %41 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %40, i64 %15, 4, 0
  %42 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %41, i64 %13, 3, 1
  %43 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %42, i64 %16, 4, 1
  %44 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %43, i64 %14, 3, 2
  %45 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %44, i64 %17, 4, 2
  %46 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } poison, ptr %0, 0
  %47 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %46, ptr %1, 1
  %48 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %47, i64 %2, 2
  %49 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %48, i64 %3, 3, 0
  %50 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %49, i64 %6, 4, 0
  %51 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %50, i64 %4, 3, 1
  %52 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %51, i64 %7, 4, 1
  %53 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %52, i64 %5, 3, 2
  %54 = insertvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %53, i64 %8, 4, 2
  %55 = alloca float, i64 128, align 4
  %56 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } poison, ptr %55, 0
  %57 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %56, ptr %55, 1
  %58 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %57, i64 0, 2
  %59 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %58, i64 128, 3, 0
  %60 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %59, i64 1, 4, 0
  %61 = alloca float, i64 128, align 4
  %62 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } poison, ptr %61, 0
  %63 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %62, ptr %61, 1
  %64 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %63, i64 0, 2
  %65 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %64, i64 128, 3, 0
  %66 = insertvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %65, i64 1, 4, 0
  br label %67

67:                                               ; preds = %70, %27
  %68 = phi i64 [ %478, %70 ], [ 0, %27 ]
  %69 = icmp slt i64 %68, 2
  br i1 %69, label %70, label %479

70:                                               ; preds = %67
  %71 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 0
  %72 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 1
  %73 = insertvalue { ptr, ptr, i64 } poison, ptr %71, 0
  %74 = insertvalue { ptr, ptr, i64 } %73, ptr %72, 1
  %75 = insertvalue { ptr, ptr, i64 } %74, i64 0, 2
  %76 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 2
  %77 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 3, 0
  %78 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 3, 1
  %79 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 3, 2
  %80 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 4, 0
  %81 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 4, 1
  %82 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %36, 4, 2
  %83 = mul nsw i64 %68, 256
  %84 = extractvalue { ptr, ptr, i64 } %75, 1
  %85 = ptrtoint ptr %84 to i64
  %86 = mul i64 %83, 4
  %87 = add i64 %85, %86
  %88 = inttoptr i64 %87 to ptr
  %89 = call ptr @fpga_gemm_begin(i32 16, i32 16, i32 16, i32 7)
  %90 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 0
  %91 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 1
  %92 = insertvalue { ptr, ptr, i64 } poison, ptr %90, 0
  %93 = insertvalue { ptr, ptr, i64 } %92, ptr %91, 1
  %94 = insertvalue { ptr, ptr, i64 } %93, i64 0, 2
  %95 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 2
  %96 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 3, 0
  %97 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 3, 1
  %98 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 3, 2
  %99 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 4, 0
  %100 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 4, 1
  %101 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 4, 2
  %102 = mul nsw i64 %68, 256
  %103 = extractvalue { ptr, ptr, i64 } %94, 0
  %104 = extractvalue { ptr, ptr, i64 } %94, 1
  %105 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %103, 0
  %106 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %105, ptr %104, 1
  %107 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %106, i64 %102, 2
  %108 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %107, i64 8, 3, 0
  %109 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %108, i64 16, 4, 0
  %110 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %109, i64 16, 3, 1
  %111 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %110, i64 1, 4, 1
  %112 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 0
  %113 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 1
  %114 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %112, 0
  %115 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %114, ptr %113, 1
  %116 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %115, i64 0, 2
  %117 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %116, i64 8, 3, 0
  %118 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %117, i64 16, 4, 0
  %119 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %118, i64 16, 3, 1
  %120 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %119, i64 1, 4, 1
  %121 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %111, 3, 0
  %122 = mul i64 1, %121
  %123 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %111, 3, 1
  %124 = mul i64 %122, %123
  %125 = mul i64 %124, 4
  %126 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %111, 1
  %127 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %111, 2
  %128 = getelementptr float, ptr %126, i64 %127
  %129 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %120, 1
  %130 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %120, 2
  %131 = getelementptr float, ptr %129, i64 %130
  call void @llvm.memcpy.p0.p0.i64(ptr %131, ptr %128, i64 %125, i1 false)
  %132 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 1
  %133 = ptrtoint ptr %132 to i64
  %134 = inttoptr i64 %133 to ptr
  %135 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 0
  %136 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 1
  %137 = insertvalue { ptr, ptr, i64 } poison, ptr %135, 0
  %138 = insertvalue { ptr, ptr, i64 } %137, ptr %136, 1
  %139 = insertvalue { ptr, ptr, i64 } %138, i64 0, 2
  %140 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 2
  %141 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 3, 0
  %142 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 3, 1
  %143 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 3, 2
  %144 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 4, 0
  %145 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 4, 1
  %146 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 4, 2
  %147 = mul nsw i64 %68, 256
  %148 = extractvalue { ptr, ptr, i64 } %139, 0
  %149 = extractvalue { ptr, ptr, i64 } %139, 1
  %150 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %148, 0
  %151 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %150, ptr %149, 1
  %152 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %151, i64 %147, 2
  %153 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %152, i64 16, 3, 0
  %154 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %153, i64 16, 4, 0
  %155 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %154, i64 8, 3, 1
  %156 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %155, i64 1, 4, 1
  %157 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %66, 0
  %158 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %66, 1
  %159 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %157, 0
  %160 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %159, ptr %158, 1
  %161 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %160, i64 0, 2
  %162 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %161, i64 16, 3, 0
  %163 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %162, i64 8, 4, 0
  %164 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %163, i64 8, 3, 1
  %165 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, i64 1, 4, 1
  %166 = call ptr @llvm.stacksave.p0()
  %167 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %156, ptr %167, align 8
  %168 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %167, 1
  %169 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %165, ptr %169, align 8
  %170 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %169, 1
  %171 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %168, ptr %171, align 8
  %172 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %170, ptr %172, align 8
  call void @memrefCopy(i64 4, ptr %171, ptr %172)
  call void @llvm.stackrestore.p0(ptr %166)
  %173 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %66, 1
  %174 = ptrtoint ptr %173 to i64
  %175 = inttoptr i64 %174 to ptr
  %176 = call i32 @fpga_tile_submit(ptr %89, i32 0, i32 0, i32 1, i32 8, i32 16, i32 8, ptr %134, ptr %175, ptr %88, i32 16, i32 0, i32 0, i32 125)
  %177 = add i64 %83, 8
  %178 = mul i64 %177, 4
  %179 = add i64 %85, %178
  %180 = inttoptr i64 %179 to ptr
  %181 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %111, 3, 0
  %182 = mul i64 1, %181
  %183 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %111, 3, 1
  %184 = mul i64 %182, %183
  %185 = mul i64 %184, 4
  %186 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %111, 1
  %187 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %111, 2
  %188 = getelementptr float, ptr %186, i64 %187
  %189 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %120, 1
  %190 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %120, 2
  %191 = getelementptr float, ptr %189, i64 %190
  call void @llvm.memcpy.p0.p0.i64(ptr %191, ptr %188, i64 %185, i1 false)
  %192 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 0
  %193 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 1
  %194 = insertvalue { ptr, ptr, i64 } poison, ptr %192, 0
  %195 = insertvalue { ptr, ptr, i64 } %194, ptr %193, 1
  %196 = insertvalue { ptr, ptr, i64 } %195, i64 0, 2
  %197 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 2
  %198 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 3, 0
  %199 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 3, 1
  %200 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 3, 2
  %201 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 4, 0
  %202 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 4, 1
  %203 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 4, 2
  %204 = mul nsw i64 %68, 256
  %205 = add i64 %204, 8
  %206 = extractvalue { ptr, ptr, i64 } %196, 0
  %207 = extractvalue { ptr, ptr, i64 } %196, 1
  %208 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %206, 0
  %209 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %208, ptr %207, 1
  %210 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %209, i64 %205, 2
  %211 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %210, i64 16, 3, 0
  %212 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %211, i64 16, 4, 0
  %213 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %212, i64 8, 3, 1
  %214 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %213, i64 1, 4, 1
  %215 = call ptr @llvm.stacksave.p0()
  %216 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %214, ptr %216, align 8
  %217 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %216, 1
  %218 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %165, ptr %218, align 8
  %219 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %218, 1
  %220 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %217, ptr %220, align 8
  %221 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %219, ptr %221, align 8
  call void @memrefCopy(i64 4, ptr %220, ptr %221)
  call void @llvm.stackrestore.p0(ptr %215)
  %222 = call i32 @fpga_tile_submit(ptr %89, i32 1, i32 0, i32 1, i32 8, i32 16, i32 8, ptr %134, ptr %175, ptr %180, i32 16, i32 0, i32 42, i32 125)
  %223 = add i64 %83, 128
  %224 = mul i64 %223, 4
  %225 = add i64 %85, %224
  %226 = inttoptr i64 %225 to ptr
  %227 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 0
  %228 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 1
  %229 = insertvalue { ptr, ptr, i64 } poison, ptr %227, 0
  %230 = insertvalue { ptr, ptr, i64 } %229, ptr %228, 1
  %231 = insertvalue { ptr, ptr, i64 } %230, i64 0, 2
  %232 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 2
  %233 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 3, 0
  %234 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 3, 1
  %235 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 3, 2
  %236 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 4, 0
  %237 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 4, 1
  %238 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 4, 2
  %239 = mul nsw i64 %68, 256
  %240 = add i64 %239, 128
  %241 = extractvalue { ptr, ptr, i64 } %231, 0
  %242 = extractvalue { ptr, ptr, i64 } %231, 1
  %243 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %241, 0
  %244 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %243, ptr %242, 1
  %245 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %244, i64 %240, 2
  %246 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %245, i64 8, 3, 0
  %247 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %246, i64 16, 4, 0
  %248 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %247, i64 16, 3, 1
  %249 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %248, i64 1, 4, 1
  %250 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %249, 3, 0
  %251 = mul i64 1, %250
  %252 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %249, 3, 1
  %253 = mul i64 %251, %252
  %254 = mul i64 %253, 4
  %255 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %249, 1
  %256 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %249, 2
  %257 = getelementptr float, ptr %255, i64 %256
  %258 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %120, 1
  %259 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %120, 2
  %260 = getelementptr float, ptr %258, i64 %259
  call void @llvm.memcpy.p0.p0.i64(ptr %260, ptr %257, i64 %254, i1 false)
  %261 = call ptr @llvm.stacksave.p0()
  %262 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %156, ptr %262, align 8
  %263 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %262, 1
  %264 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %165, ptr %264, align 8
  %265 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %264, 1
  %266 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %263, ptr %266, align 8
  %267 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %265, ptr %267, align 8
  call void @memrefCopy(i64 4, ptr %266, ptr %267)
  call void @llvm.stackrestore.p0(ptr %261)
  %268 = call i32 @fpga_tile_submit(ptr %89, i32 2, i32 0, i32 1, i32 8, i32 16, i32 8, ptr %134, ptr %175, ptr %226, i32 16, i32 0, i32 128, i32 125)
  %269 = add i64 %83, 136
  %270 = mul i64 %269, 4
  %271 = add i64 %85, %270
  %272 = inttoptr i64 %271 to ptr
  %273 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 0
  %274 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 1
  %275 = insertvalue { ptr, ptr, i64 } poison, ptr %273, 0
  %276 = insertvalue { ptr, ptr, i64 } %275, ptr %274, 1
  %277 = insertvalue { ptr, ptr, i64 } %276, i64 0, 2
  %278 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 2
  %279 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 3, 0
  %280 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 3, 1
  %281 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 3, 2
  %282 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 4, 0
  %283 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 4, 1
  %284 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 4, 2
  %285 = mul nsw i64 %68, 256
  %286 = add i64 %285, 128
  %287 = extractvalue { ptr, ptr, i64 } %277, 0
  %288 = extractvalue { ptr, ptr, i64 } %277, 1
  %289 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %287, 0
  %290 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %289, ptr %288, 1
  %291 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %290, i64 %286, 2
  %292 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %291, i64 4, 3, 0
  %293 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %292, i64 16, 4, 0
  %294 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %293, i64 16, 3, 1
  %295 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %294, i64 1, 4, 1
  %296 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 0
  %297 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 1
  %298 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %296, 0
  %299 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %298, ptr %297, 1
  %300 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %299, i64 0, 2
  %301 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %300, i64 4, 3, 0
  %302 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %301, i64 16, 4, 0
  %303 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %302, i64 16, 3, 1
  %304 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %303, i64 1, 4, 1
  %305 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %295, 3, 0
  %306 = mul i64 1, %305
  %307 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %295, 3, 1
  %308 = mul i64 %306, %307
  %309 = mul i64 %308, 4
  %310 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %295, 1
  %311 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %295, 2
  %312 = getelementptr float, ptr %310, i64 %311
  %313 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %304, 1
  %314 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %304, 2
  %315 = getelementptr float, ptr %313, i64 %314
  call void @llvm.memcpy.p0.p0.i64(ptr %315, ptr %312, i64 %309, i1 false)
  %316 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %60, 1
  %317 = ptrtoint ptr %316 to i64
  %318 = inttoptr i64 %317 to ptr
  %319 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 0
  %320 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 1
  %321 = insertvalue { ptr, ptr, i64 } poison, ptr %319, 0
  %322 = insertvalue { ptr, ptr, i64 } %321, ptr %320, 1
  %323 = insertvalue { ptr, ptr, i64 } %322, i64 0, 2
  %324 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 2
  %325 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 3, 0
  %326 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 3, 1
  %327 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 3, 2
  %328 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 4, 0
  %329 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 4, 1
  %330 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 4, 2
  %331 = mul nsw i64 %68, 256
  %332 = add i64 %331, 8
  %333 = extractvalue { ptr, ptr, i64 } %323, 0
  %334 = extractvalue { ptr, ptr, i64 } %323, 1
  %335 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %333, 0
  %336 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %335, ptr %334, 1
  %337 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %336, i64 %332, 2
  %338 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %337, i64 16, 3, 0
  %339 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %338, i64 16, 4, 0
  %340 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %339, i64 4, 3, 1
  %341 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %340, i64 1, 4, 1
  %342 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %66, 0
  %343 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %66, 1
  %344 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %342, 0
  %345 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %344, ptr %343, 1
  %346 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %345, i64 0, 2
  %347 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %346, i64 16, 3, 0
  %348 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %347, i64 4, 4, 0
  %349 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %348, i64 4, 3, 1
  %350 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %349, i64 1, 4, 1
  %351 = call ptr @llvm.stacksave.p0()
  %352 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %341, ptr %352, align 8
  %353 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %352, 1
  %354 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %350, ptr %354, align 8
  %355 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %354, 1
  %356 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %353, ptr %356, align 8
  %357 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %355, ptr %357, align 8
  call void @memrefCopy(i64 4, ptr %356, ptr %357)
  call void @llvm.stackrestore.p0(ptr %351)
  %358 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %66, 1
  %359 = ptrtoint ptr %358 to i64
  %360 = inttoptr i64 %359 to ptr
  %361 = call i32 @fpga_tile_submit(ptr %89, i32 3, i32 0, i32 1, i32 4, i32 16, i32 4, ptr %318, ptr %360, ptr %272, i32 16, i32 1, i32 0, i32 117)
  %362 = add i64 %83, 140
  %363 = mul i64 %362, 4
  %364 = add i64 %85, %363
  %365 = inttoptr i64 %364 to ptr
  %366 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %295, 3, 0
  %367 = mul i64 1, %366
  %368 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %295, 3, 1
  %369 = mul i64 %367, %368
  %370 = mul i64 %369, 4
  %371 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %295, 1
  %372 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %295, 2
  %373 = getelementptr float, ptr %371, i64 %372
  %374 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %304, 1
  %375 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %304, 2
  %376 = getelementptr float, ptr %374, i64 %375
  call void @llvm.memcpy.p0.p0.i64(ptr %376, ptr %373, i64 %370, i1 false)
  %377 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 0
  %378 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 1
  %379 = insertvalue { ptr, ptr, i64 } poison, ptr %377, 0
  %380 = insertvalue { ptr, ptr, i64 } %379, ptr %378, 1
  %381 = insertvalue { ptr, ptr, i64 } %380, i64 0, 2
  %382 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 2
  %383 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 3, 0
  %384 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 3, 1
  %385 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 3, 2
  %386 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 4, 0
  %387 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 4, 1
  %388 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %45, 4, 2
  %389 = mul nsw i64 %68, 256
  %390 = add i64 %389, 12
  %391 = extractvalue { ptr, ptr, i64 } %381, 0
  %392 = extractvalue { ptr, ptr, i64 } %381, 1
  %393 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %391, 0
  %394 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %393, ptr %392, 1
  %395 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %394, i64 %390, 2
  %396 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %395, i64 16, 3, 0
  %397 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %396, i64 16, 4, 0
  %398 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %397, i64 4, 3, 1
  %399 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %398, i64 1, 4, 1
  %400 = call ptr @llvm.stacksave.p0()
  %401 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %399, ptr %401, align 8
  %402 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %401, 1
  %403 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %350, ptr %403, align 8
  %404 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %403, 1
  %405 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %402, ptr %405, align 8
  %406 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %404, ptr %406, align 8
  call void @memrefCopy(i64 4, ptr %405, ptr %406)
  call void @llvm.stackrestore.p0(ptr %400)
  %407 = call i32 @fpga_tile_submit(ptr %89, i32 4, i32 0, i32 1, i32 4, i32 16, i32 4, ptr %318, ptr %360, ptr %365, i32 16, i32 2, i32 0, i32 117)
  %408 = add i64 %83, 200
  %409 = mul i64 %408, 4
  %410 = add i64 %85, %409
  %411 = inttoptr i64 %410 to ptr
  %412 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 0
  %413 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 1
  %414 = insertvalue { ptr, ptr, i64 } poison, ptr %412, 0
  %415 = insertvalue { ptr, ptr, i64 } %414, ptr %413, 1
  %416 = insertvalue { ptr, ptr, i64 } %415, i64 0, 2
  %417 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 2
  %418 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 3, 0
  %419 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 3, 1
  %420 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 3, 2
  %421 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 4, 0
  %422 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 4, 1
  %423 = extractvalue { ptr, ptr, i64, [3 x i64], [3 x i64] } %54, 4, 2
  %424 = mul nsw i64 %68, 256
  %425 = add i64 %424, 192
  %426 = extractvalue { ptr, ptr, i64 } %416, 0
  %427 = extractvalue { ptr, ptr, i64 } %416, 1
  %428 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %426, 0
  %429 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %428, ptr %427, 1
  %430 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %429, i64 %425, 2
  %431 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %430, i64 4, 3, 0
  %432 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %431, i64 16, 4, 0
  %433 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %432, i64 16, 3, 1
  %434 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %433, i64 1, 4, 1
  %435 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %434, 3, 0
  %436 = mul i64 1, %435
  %437 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %434, 3, 1
  %438 = mul i64 %436, %437
  %439 = mul i64 %438, 4
  %440 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %434, 1
  %441 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %434, 2
  %442 = getelementptr float, ptr %440, i64 %441
  %443 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %304, 1
  %444 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %304, 2
  %445 = getelementptr float, ptr %443, i64 %444
  call void @llvm.memcpy.p0.p0.i64(ptr %445, ptr %442, i64 %439, i1 false)
  %446 = call ptr @llvm.stacksave.p0()
  %447 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %341, ptr %447, align 8
  %448 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %447, 1
  %449 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %350, ptr %449, align 8
  %450 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %449, 1
  %451 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %448, ptr %451, align 8
  %452 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %450, ptr %452, align 8
  call void @memrefCopy(i64 4, ptr %451, ptr %452)
  call void @llvm.stackrestore.p0(ptr %446)
  %453 = call i32 @fpga_tile_submit(ptr %89, i32 5, i32 0, i32 1, i32 4, i32 16, i32 4, ptr %318, ptr %360, ptr %411, i32 16, i32 3, i32 0, i32 117)
  %454 = add i64 %83, 204
  %455 = mul i64 %454, 4
  %456 = add i64 %85, %455
  %457 = inttoptr i64 %456 to ptr
  %458 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %434, 3, 0
  %459 = mul i64 1, %458
  %460 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %434, 3, 1
  %461 = mul i64 %459, %460
  %462 = mul i64 %461, 4
  %463 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %434, 1
  %464 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %434, 2
  %465 = getelementptr float, ptr %463, i64 %464
  %466 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %304, 1
  %467 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %304, 2
  %468 = getelementptr float, ptr %466, i64 %467
  call void @llvm.memcpy.p0.p0.i64(ptr %468, ptr %465, i64 %462, i1 false)
  %469 = call ptr @llvm.stacksave.p0()
  %470 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %399, ptr %470, align 8
  %471 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %470, 1
  %472 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %350, ptr %472, align 8
  %473 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %472, 1
  %474 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %471, ptr %474, align 8
  %475 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %473, ptr %475, align 8
  call void @memrefCopy(i64 4, ptr %474, ptr %475)
  call void @llvm.stackrestore.p0(ptr %469)
  %476 = call i32 @fpga_tile_submit(ptr %89, i32 6, i32 0, i32 1, i32 4, i32 16, i32 4, ptr %318, ptr %360, ptr %457, i32 16, i32 1, i32 42, i32 117)
  %477 = call i32 @fpga_gemm_wait(ptr %89)
  %478 = add i64 %68, 1
  br label %67

479:                                              ; preds = %67
  ret void
}

; Function Attrs: nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
declare void @llvm.memcpy.p0.p0.i64(ptr noalias writeonly captures(none), ptr noalias readonly captures(none), i64, i1 immarg) #0

; Function Attrs: nocallback nofree nosync nounwind willreturn
declare ptr @llvm.stacksave.p0() #1

; Function Attrs: nocallback nofree nosync nounwind willreturn
declare void @llvm.stackrestore.p0(ptr) #1

attributes #0 = { nocallback nofree nosync nounwind willreturn memory(argmem: readwrite) }
attributes #1 = { nocallback nofree nosync nounwind willreturn }

!llvm.module.flags = !{!0}

!0 = !{i32 2, !"Debug Info Version", i32 3}
