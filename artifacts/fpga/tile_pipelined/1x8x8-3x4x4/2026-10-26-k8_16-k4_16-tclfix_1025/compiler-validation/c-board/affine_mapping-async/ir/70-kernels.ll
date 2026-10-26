; ModuleID = 'LLVMDialectModule'
source_filename = "LLVMDialectModule"

declare void @free(ptr)

declare void @memrefCopy(i64, ptr, ptr)

declare ptr @malloc(i64)

declare i32 @fpga_gemm_wait(ptr)

declare i32 @fpga_tile_submit(ptr, i32, i32, i32, i32, i32, i32, ptr, ptr, ptr, i32, i32, i32, i32)

declare ptr @fpga_gemm_begin(i32, i32, i32, i32)

define void @expressions__systolic_kernel(ptr %0, ptr %1, i64 %2, i64 %3, i64 %4, i64 %5, i64 %6, ptr %7, ptr %8, i64 %9, i64 %10, i64 %11, i64 %12, i64 %13, ptr %14, ptr %15, i64 %16, i64 %17, i64 %18, i64 %19, i64 %20) {
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
  %55 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %56 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %57 = insertvalue { ptr, ptr, i64 } poison, ptr %55, 0
  %58 = insertvalue { ptr, ptr, i64 } %57, ptr %56, 1
  %59 = insertvalue { ptr, ptr, i64 } %58, i64 0, 2
  %60 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %61 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %62 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %63 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %64 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %65 = extractvalue { ptr, ptr, i64 } %59, 0
  %66 = extractvalue { ptr, ptr, i64 } %59, 1
  %67 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %65, 0
  %68 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %67, ptr %66, 1
  %69 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %68, i64 10, 2
  %70 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %69, i64 4, 3, 0
  %71 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %70, i64 20, 4, 0
  %72 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %71, i64 4, 3, 1
  %73 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %72, i64 3, 4, 1
  %74 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %75 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %76 = insertvalue { ptr, ptr, i64 } poison, ptr %74, 0
  %77 = insertvalue { ptr, ptr, i64 } %76, ptr %75, 1
  %78 = insertvalue { ptr, ptr, i64 } %77, i64 0, 2
  %79 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %80 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %81 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %82 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %83 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %84 = extractvalue { ptr, ptr, i64 } %78, 0
  %85 = extractvalue { ptr, ptr, i64 } %78, 1
  %86 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %84, 0
  %87 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %86, ptr %85, 1
  %88 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %87, i64 8, 2
  %89 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, i64 4, 3, 0
  %90 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %89, i64 24, 4, 0
  %91 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %90, i64 4, 3, 1
  %92 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %91, i64 1, 4, 1
  %93 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 0
  %94 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %95 = insertvalue { ptr, ptr, i64 } poison, ptr %93, 0
  %96 = insertvalue { ptr, ptr, i64 } %95, ptr %94, 1
  %97 = insertvalue { ptr, ptr, i64 } %96, i64 0, 2
  %98 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 2
  %99 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 0
  %100 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 3, 1
  %101 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 0
  %102 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 4, 1
  %103 = extractvalue { ptr, ptr, i64 } %97, 0
  %104 = extractvalue { ptr, ptr, i64 } %97, 1
  %105 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %103, 0
  %106 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %105, ptr %104, 1
  %107 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %106, i64 1, 2
  %108 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %107, i64 4, 3, 0
  %109 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %108, i64 32, 4, 0
  %110 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %109, i64 4, 3, 1
  %111 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %110, i64 2, 4, 1
  %112 = call ptr @fpga_gemm_begin(i32 4, i32 4, i32 4, i32 1)
  %113 = call ptr @malloc(i64 128)
  %114 = ptrtoint ptr %113 to i64
  %115 = add i64 %114, 63
  %116 = urem i64 %115, 64
  %117 = sub i64 %115, %116
  %118 = inttoptr i64 %117 to ptr
  %119 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %113, 0
  %120 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %119, ptr %118, 1
  %121 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %120, i64 0, 2
  %122 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %121, i64 4, 3, 0
  %123 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %122, i64 4, 3, 1
  %124 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %123, i64 4, 4, 0
  %125 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %124, i64 1, 4, 1
  %126 = call ptr @llvm.stacksave.p0()
  %127 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %111, ptr %127, align 8
  %128 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %127, 1
  %129 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %125, ptr %129, align 8
  %130 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %129, 1
  %131 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %128, ptr %131, align 8
  %132 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %130, ptr %132, align 8
  call void @memrefCopy(i64 4, ptr %131, ptr %132)
  call void @llvm.stackrestore.p0(ptr %126)
  %133 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %125, 1
  %134 = ptrtoint ptr %133 to i64
  %135 = inttoptr i64 %134 to ptr
  %136 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 0
  %137 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %138 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %136, 0
  %139 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %138, ptr %137, 1
  %140 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %139, i64 0, 2
  %141 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %140, i64 4, 3, 0
  %142 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %141, i64 4, 4, 0
  %143 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %142, i64 4, 3, 1
  %144 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %143, i64 1, 4, 1
  %145 = call ptr @llvm.stacksave.p0()
  %146 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %73, ptr %146, align 8
  %147 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %146, 1
  %148 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %144, ptr %148, align 8
  %149 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %148, 1
  %150 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %147, ptr %150, align 8
  %151 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %149, ptr %151, align 8
  call void @memrefCopy(i64 4, ptr %150, ptr %151)
  call void @llvm.stackrestore.p0(ptr %145)
  %152 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %153 = ptrtoint ptr %152 to i64
  %154 = inttoptr i64 %153 to ptr
  %155 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 0
  %156 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %157 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %155, 0
  %158 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %157, ptr %156, 1
  %159 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %158, i64 0, 2
  %160 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %159, i64 4, 3, 0
  %161 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %160, i64 4, 4, 0
  %162 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %161, i64 4, 3, 1
  %163 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %162, i64 1, 4, 1
  %164 = call ptr @llvm.stacksave.p0()
  %165 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %92, ptr %165, align 8
  %166 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %165, 1
  %167 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %163, ptr %167, align 8
  %168 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %167, 1
  %169 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %166, ptr %169, align 8
  %170 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %168, ptr %170, align 8
  call void @memrefCopy(i64 4, ptr %169, ptr %170)
  call void @llvm.stackrestore.p0(ptr %164)
  %171 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %172 = ptrtoint ptr %171 to i64
  %173 = inttoptr i64 %172 to ptr
  %174 = call i32 @fpga_tile_submit(ptr %112, i32 0, i32 0, i32 1, i32 4, i32 4, i32 4, ptr %154, ptr %173, ptr %135, i32 4, i32 1, i32 0, i32 105)
  %175 = call i32 @fpga_gemm_wait(ptr %112)
  %176 = call ptr @llvm.stacksave.p0()
  %177 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %125, ptr %177, align 8
  %178 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %177, 1
  %179 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %111, ptr %179, align 8
  %180 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %179, 1
  %181 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %178, ptr %181, align 8
  %182 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %180, ptr %182, align 8
  call void @memrefCopy(i64 4, ptr %181, ptr %182)
  call void @llvm.stackrestore.p0(ptr %176)
  %183 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %125, 0
  call void @free(ptr %183)
  ret void
}

define void @runs_once__systolic_kernel(ptr %0, ptr %1, i64 %2, i64 %3, i64 %4, i64 %5, i64 %6, ptr %7, ptr %8, i64 %9, i64 %10, i64 %11, i64 %12, i64 %13, ptr %14, ptr %15, i64 %16, i64 %17, i64 %18, i64 %19, i64 %20) {
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
  %55 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %56 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %57 = insertvalue { ptr, ptr, i64 } poison, ptr %55, 0
  %58 = insertvalue { ptr, ptr, i64 } %57, ptr %56, 1
  %59 = insertvalue { ptr, ptr, i64 } %58, i64 0, 2
  %60 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %61 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %62 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %63 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %64 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %65 = extractvalue { ptr, ptr, i64 } %59, 0
  %66 = extractvalue { ptr, ptr, i64 } %59, 1
  %67 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %65, 0
  %68 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %67, ptr %66, 1
  %69 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %68, i64 48, 2
  %70 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %69, i64 8, 3, 0
  %71 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %70, i64 8, 4, 0
  %72 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %71, i64 8, 3, 1
  %73 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %72, i64 1, 4, 1
  %74 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %75 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %76 = insertvalue { ptr, ptr, i64 } poison, ptr %74, 0
  %77 = insertvalue { ptr, ptr, i64 } %76, ptr %75, 1
  %78 = insertvalue { ptr, ptr, i64 } %77, i64 0, 2
  %79 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %80 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %81 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %82 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %83 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %84 = extractvalue { ptr, ptr, i64 } %78, 0
  %85 = extractvalue { ptr, ptr, i64 } %78, 1
  %86 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %84, 0
  %87 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %86, ptr %85, 1
  %88 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %87, i64 2, 2
  %89 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, i64 8, 3, 0
  %90 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %89, i64 10, 4, 0
  %91 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %90, i64 8, 3, 1
  %92 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %91, i64 1, 4, 1
  %93 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %94 = ptrtoint ptr %93 to i64
  %95 = add i64 %94, 144
  %96 = inttoptr i64 %95 to ptr
  %97 = call ptr @fpga_gemm_begin(i32 8, i32 8, i32 8, i32 1)
  %98 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 0
  %99 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %100 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %98, 0
  %101 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %100, ptr %99, 1
  %102 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %101, i64 0, 2
  %103 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %102, i64 8, 3, 0
  %104 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %103, i64 8, 4, 0
  %105 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %104, i64 8, 3, 1
  %106 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %105, i64 1, 4, 1
  %107 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %73, 3, 0
  %108 = mul i64 1, %107
  %109 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %73, 3, 1
  %110 = mul i64 %108, %109
  %111 = mul i64 %110, 4
  %112 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %73, 1
  %113 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %73, 2
  %114 = getelementptr float, ptr %112, i64 %113
  %115 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %106, 1
  %116 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %106, 2
  %117 = getelementptr float, ptr %115, i64 %116
  call void @llvm.memcpy.p0.p0.i64(ptr %117, ptr %114, i64 %111, i1 false)
  %118 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %119 = ptrtoint ptr %118 to i64
  %120 = inttoptr i64 %119 to ptr
  %121 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 0
  %122 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %123 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %121, 0
  %124 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %123, ptr %122, 1
  %125 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %124, i64 0, 2
  %126 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %125, i64 8, 3, 0
  %127 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, i64 8, 4, 0
  %128 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %127, i64 8, 3, 1
  %129 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %128, i64 1, 4, 1
  %130 = call ptr @llvm.stacksave.p0()
  %131 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %92, ptr %131, align 8
  %132 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %131, 1
  %133 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %129, ptr %133, align 8
  %134 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %133, 1
  %135 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %132, ptr %135, align 8
  %136 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %134, ptr %136, align 8
  call void @memrefCopy(i64 4, ptr %135, ptr %136)
  call void @llvm.stackrestore.p0(ptr %130)
  %137 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %138 = ptrtoint ptr %137 to i64
  %139 = inttoptr i64 %138 to ptr
  %140 = call i32 @fpga_tile_submit(ptr %97, i32 0, i32 0, i32 1, i32 8, i32 8, i32 8, ptr %120, ptr %139, ptr %96, i32 11, i32 0, i32 0, i32 117)
  %141 = call i32 @fpga_gemm_wait(ptr %97)
  ret void
}

define void @steps__systolic_kernel(ptr %0, ptr %1, i64 %2, i64 %3, i64 %4, i64 %5, i64 %6, ptr %7, ptr %8, i64 %9, i64 %10, i64 %11, i64 %12, i64 %13, ptr %14, ptr %15, i64 %16, i64 %17, i64 %18, i64 %19, i64 %20) {
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
  %55 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %56 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %57 = insertvalue { ptr, ptr, i64 } poison, ptr %55, 0
  %58 = insertvalue { ptr, ptr, i64 } %57, ptr %56, 1
  %59 = insertvalue { ptr, ptr, i64 } %58, i64 0, 2
  %60 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %61 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %62 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %63 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %64 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %65 = extractvalue { ptr, ptr, i64 } %59, 0
  %66 = extractvalue { ptr, ptr, i64 } %59, 1
  %67 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %65, 0
  %68 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %67, ptr %66, 1
  %69 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %68, i64 15, 2
  %70 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %69, i64 3, 3, 0
  %71 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %70, i64 30, 4, 0
  %72 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %71, i64 8, 3, 1
  %73 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %72, i64 2, 4, 1
  %74 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %75 = ptrtoint ptr %74 to i64
  %76 = add i64 %75, 44
  %77 = inttoptr i64 %76 to ptr
  %78 = call ptr @fpga_gemm_begin(i32 3, i32 11, i32 8, i32 3)
  %79 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 0
  %80 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %81 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %79, 0
  %82 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %81, ptr %80, 1
  %83 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %82, i64 0, 2
  %84 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %83, i64 3, 3, 0
  %85 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %84, i64 8, 4, 0
  %86 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %85, i64 8, 3, 1
  %87 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %86, i64 1, 4, 1
  %88 = call ptr @llvm.stacksave.p0()
  %89 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %73, ptr %89, align 8
  %90 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %89, 1
  %91 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %87, ptr %91, align 8
  %92 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %91, 1
  %93 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %90, ptr %93, align 8
  %94 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %92, ptr %94, align 8
  call void @memrefCopy(i64 4, ptr %93, ptr %94)
  call void @llvm.stackrestore.p0(ptr %88)
  %95 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %96 = ptrtoint ptr %95 to i64
  %97 = inttoptr i64 %96 to ptr
  %98 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %99 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %100 = insertvalue { ptr, ptr, i64 } poison, ptr %98, 0
  %101 = insertvalue { ptr, ptr, i64 } %100, ptr %99, 1
  %102 = insertvalue { ptr, ptr, i64 } %101, i64 0, 2
  %103 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %104 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %105 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %106 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %107 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %108 = extractvalue { ptr, ptr, i64 } %102, 0
  %109 = extractvalue { ptr, ptr, i64 } %102, 1
  %110 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %108, 0
  %111 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %110, ptr %109, 1
  %112 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %111, i64 0, 2
  %113 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %112, i64 8, 3, 0
  %114 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %113, i64 22, 4, 0
  %115 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %114, i64 4, 3, 1
  %116 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %115, i64 1, 4, 1
  %117 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 0
  %118 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %119 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %117, 0
  %120 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %119, ptr %118, 1
  %121 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %120, i64 0, 2
  %122 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %121, i64 8, 3, 0
  %123 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %122, i64 4, 4, 0
  %124 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %123, i64 4, 3, 1
  %125 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %124, i64 1, 4, 1
  %126 = call ptr @llvm.stacksave.p0()
  %127 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %116, ptr %127, align 8
  %128 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %127, 1
  %129 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %125, ptr %129, align 8
  %130 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %129, 1
  %131 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %128, ptr %131, align 8
  %132 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %130, ptr %132, align 8
  call void @memrefCopy(i64 4, ptr %131, ptr %132)
  call void @llvm.stackrestore.p0(ptr %126)
  %133 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %134 = ptrtoint ptr %133 to i64
  %135 = inttoptr i64 %134 to ptr
  %136 = call i32 @fpga_tile_submit(ptr %78, i32 0, i32 0, i32 1, i32 3, i32 8, i32 4, ptr %97, ptr %135, ptr %77, i32 22, i32 1, i32 0, i32 109)
  %137 = add i64 %75, 60
  %138 = inttoptr i64 %137 to ptr
  %139 = call ptr @llvm.stacksave.p0()
  %140 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %73, ptr %140, align 8
  %141 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %140, 1
  %142 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %87, ptr %142, align 8
  %143 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %142, 1
  %144 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %141, ptr %144, align 8
  %145 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %143, ptr %145, align 8
  call void @memrefCopy(i64 4, ptr %144, ptr %145)
  call void @llvm.stackrestore.p0(ptr %139)
  %146 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %147 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %148 = insertvalue { ptr, ptr, i64 } poison, ptr %146, 0
  %149 = insertvalue { ptr, ptr, i64 } %148, ptr %147, 1
  %150 = insertvalue { ptr, ptr, i64 } %149, i64 0, 2
  %151 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %152 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %153 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %154 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %155 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %156 = extractvalue { ptr, ptr, i64 } %150, 0
  %157 = extractvalue { ptr, ptr, i64 } %150, 1
  %158 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %156, 0
  %159 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %158, ptr %157, 1
  %160 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %159, i64 4, 2
  %161 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %160, i64 8, 3, 0
  %162 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %161, i64 22, 4, 0
  %163 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %162, i64 4, 3, 1
  %164 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %163, i64 1, 4, 1
  %165 = call ptr @llvm.stacksave.p0()
  %166 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %164, ptr %166, align 8
  %167 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %166, 1
  %168 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %125, ptr %168, align 8
  %169 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %168, 1
  %170 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %167, ptr %170, align 8
  %171 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %169, ptr %171, align 8
  call void @memrefCopy(i64 4, ptr %170, ptr %171)
  call void @llvm.stackrestore.p0(ptr %165)
  %172 = call i32 @fpga_tile_submit(ptr %78, i32 1, i32 0, i32 1, i32 3, i32 8, i32 4, ptr %97, ptr %135, ptr %138, i32 22, i32 2, i32 0, i32 109)
  %173 = add i64 %75, 76
  %174 = inttoptr i64 %173 to ptr
  %175 = call ptr @llvm.stacksave.p0()
  %176 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %73, ptr %176, align 8
  %177 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %176, 1
  %178 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %87, ptr %178, align 8
  %179 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %178, 1
  %180 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %177, ptr %180, align 8
  %181 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %179, ptr %181, align 8
  call void @memrefCopy(i64 4, ptr %180, ptr %181)
  call void @llvm.stackrestore.p0(ptr %175)
  %182 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %183 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %184 = insertvalue { ptr, ptr, i64 } poison, ptr %182, 0
  %185 = insertvalue { ptr, ptr, i64 } %184, ptr %183, 1
  %186 = insertvalue { ptr, ptr, i64 } %185, i64 0, 2
  %187 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %188 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %189 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %190 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %191 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %192 = extractvalue { ptr, ptr, i64 } %186, 0
  %193 = extractvalue { ptr, ptr, i64 } %186, 1
  %194 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %192, 0
  %195 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %194, ptr %193, 1
  %196 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %195, i64 8, 2
  %197 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %196, i64 8, 3, 0
  %198 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %197, i64 22, 4, 0
  %199 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %198, i64 3, 3, 1
  %200 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %199, i64 1, 4, 1
  %201 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 0
  %202 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %203 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %201, 0
  %204 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %203, ptr %202, 1
  %205 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %204, i64 0, 2
  %206 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %205, i64 8, 3, 0
  %207 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %206, i64 3, 4, 0
  %208 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %207, i64 3, 3, 1
  %209 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %208, i64 1, 4, 1
  %210 = call ptr @llvm.stacksave.p0()
  %211 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %200, ptr %211, align 8
  %212 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %211, 1
  %213 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %209, ptr %213, align 8
  %214 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %213, 1
  %215 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %212, ptr %215, align 8
  %216 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %214, ptr %216, align 8
  call void @memrefCopy(i64 4, ptr %215, ptr %216)
  call void @llvm.stackrestore.p0(ptr %210)
  %217 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %218 = ptrtoint ptr %217 to i64
  %219 = inttoptr i64 %218 to ptr
  %220 = call i32 @fpga_tile_submit(ptr %78, i32 2, i32 0, i32 1, i32 3, i32 8, i32 3, ptr %97, ptr %219, ptr %174, i32 22, i32 3, i32 0, i32 109)
  %221 = call i32 @fpga_gemm_wait(ptr %78)
  ret void
}

define void @split__systolic_kernel(ptr %0, ptr %1, i64 %2, i64 %3, i64 %4, i64 %5, i64 %6, ptr %7, ptr %8, i64 %9, i64 %10, i64 %11, i64 %12, i64 %13, ptr %14, ptr %15, i64 %16, i64 %17, i64 %18, i64 %19, i64 %20) {
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
  %55 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %56 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %57 = insertvalue { ptr, ptr, i64 } poison, ptr %55, 0
  %58 = insertvalue { ptr, ptr, i64 } %57, ptr %56, 1
  %59 = insertvalue { ptr, ptr, i64 } %58, i64 0, 2
  %60 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %61 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %62 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %63 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %64 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %65 = extractvalue { ptr, ptr, i64 } %59, 0
  %66 = extractvalue { ptr, ptr, i64 } %59, 1
  %67 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %65, 0
  %68 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %67, ptr %66, 1
  %69 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %68, i64 8, 2
  %70 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %69, i64 8, 3, 0
  %71 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %70, i64 16, 4, 0
  %72 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %71, i64 8, 3, 1
  %73 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %72, i64 1, 4, 1
  %74 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %75 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %76 = insertvalue { ptr, ptr, i64 } poison, ptr %74, 0
  %77 = insertvalue { ptr, ptr, i64 } %76, ptr %75, 1
  %78 = insertvalue { ptr, ptr, i64 } %77, i64 0, 2
  %79 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %80 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %81 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %82 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %83 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %84 = extractvalue { ptr, ptr, i64 } %78, 0
  %85 = extractvalue { ptr, ptr, i64 } %78, 1
  %86 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %84, 0
  %87 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %86, ptr %85, 1
  %88 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %87, i64 0, 2
  %89 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, i64 8, 3, 0
  %90 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %89, i64 8, 4, 0
  %91 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %90, i64 8, 3, 1
  %92 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %91, i64 1, 4, 1
  %93 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %94 = ptrtoint ptr %93 to i64
  %95 = add i64 %94, 160
  %96 = inttoptr i64 %95 to ptr
  %97 = call ptr @fpga_gemm_begin(i32 8, i32 8, i32 8, i32 1)
  %98 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 0
  %99 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %100 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %98, 0
  %101 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %100, ptr %99, 1
  %102 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %101, i64 0, 2
  %103 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %102, i64 8, 3, 0
  %104 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %103, i64 8, 4, 0
  %105 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %104, i64 8, 3, 1
  %106 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %105, i64 1, 4, 1
  %107 = call ptr @llvm.stacksave.p0()
  %108 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %73, ptr %108, align 8
  %109 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %108, 1
  %110 = alloca { ptr, ptr, i64, [2 x i64], [2 x i64] }, i64 1, align 8
  store { ptr, ptr, i64, [2 x i64], [2 x i64] } %106, ptr %110, align 8
  %111 = insertvalue { i64, ptr } { i64 2, ptr poison }, ptr %110, 1
  %112 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %109, ptr %112, align 8
  %113 = alloca { i64, ptr }, i64 1, align 8
  store { i64, ptr } %111, ptr %113, align 8
  call void @memrefCopy(i64 4, ptr %112, ptr %113)
  call void @llvm.stackrestore.p0(ptr %107)
  %114 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %115 = ptrtoint ptr %114 to i64
  %116 = inttoptr i64 %115 to ptr
  %117 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 0
  %118 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %119 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %117, 0
  %120 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %119, ptr %118, 1
  %121 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %120, i64 0, 2
  %122 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %121, i64 8, 3, 0
  %123 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %122, i64 8, 4, 0
  %124 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %123, i64 8, 3, 1
  %125 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %124, i64 1, 4, 1
  %126 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %92, 3, 0
  %127 = mul i64 1, %126
  %128 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %92, 3, 1
  %129 = mul i64 %127, %128
  %130 = mul i64 %129, 4
  %131 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %92, 1
  %132 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %92, 2
  %133 = getelementptr float, ptr %131, i64 %132
  %134 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %125, 1
  %135 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %125, 2
  %136 = getelementptr float, ptr %134, i64 %135
  call void @llvm.memcpy.p0.p0.i64(ptr %136, ptr %133, i64 %130, i1 false)
  %137 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %138 = ptrtoint ptr %137 to i64
  %139 = inttoptr i64 %138 to ptr
  %140 = call i32 @fpga_tile_submit(ptr %97, i32 0, i32 0, i32 1, i32 8, i32 8, i32 8, ptr %116, ptr %139, ptr %96, i32 32, i32 0, i32 0, i32 117)
  %141 = call i32 @fpga_gemm_wait(ptr %97)
  ret void
}

define void @near_max__systolic_kernel(ptr %0, ptr %1, i64 %2, i64 %3, i64 %4, i64 %5, i64 %6, ptr %7, ptr %8, i64 %9, i64 %10, i64 %11, i64 %12, i64 %13, ptr %14, ptr %15, i64 %16, i64 %17, i64 %18, i64 %19, i64 %20) {
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
  %55 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 0
  %56 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 1
  %57 = insertvalue { ptr, ptr, i64 } poison, ptr %55, 0
  %58 = insertvalue { ptr, ptr, i64 } %57, ptr %56, 1
  %59 = insertvalue { ptr, ptr, i64 } %58, i64 0, 2
  %60 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 2
  %61 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 0
  %62 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 3, 1
  %63 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 0
  %64 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %42, 4, 1
  %65 = extractvalue { ptr, ptr, i64 } %59, 0
  %66 = extractvalue { ptr, ptr, i64 } %59, 1
  %67 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %65, 0
  %68 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %67, ptr %66, 1
  %69 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %68, i64 0, 2
  %70 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %69, i64 7, 3, 0
  %71 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %70, i64 8, 4, 0
  %72 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %71, i64 8, 3, 1
  %73 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %72, i64 1, 4, 1
  %74 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 0
  %75 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 1
  %76 = insertvalue { ptr, ptr, i64 } poison, ptr %74, 0
  %77 = insertvalue { ptr, ptr, i64 } %76, ptr %75, 1
  %78 = insertvalue { ptr, ptr, i64 } %77, i64 0, 2
  %79 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 2
  %80 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 0
  %81 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 3, 1
  %82 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 0
  %83 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %35, 4, 1
  %84 = extractvalue { ptr, ptr, i64 } %78, 0
  %85 = extractvalue { ptr, ptr, i64 } %78, 1
  %86 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %84, 0
  %87 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %86, ptr %85, 1
  %88 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %87, i64 0, 2
  %89 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %88, i64 8, 3, 0
  %90 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %89, i64 8, 4, 0
  %91 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %90, i64 8, 3, 1
  %92 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %91, i64 1, 4, 1
  %93 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %28, 1
  %94 = ptrtoint ptr %93 to i64
  %95 = inttoptr i64 %94 to ptr
  %96 = call ptr @fpga_gemm_begin(i32 7, i32 8, i32 8, i32 1)
  %97 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 0
  %98 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %99 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %97, 0
  %100 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %99, ptr %98, 1
  %101 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %100, i64 0, 2
  %102 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %101, i64 7, 3, 0
  %103 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %102, i64 8, 4, 0
  %104 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %103, i64 8, 3, 1
  %105 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %104, i64 1, 4, 1
  %106 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %73, 3, 0
  %107 = mul i64 1, %106
  %108 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %73, 3, 1
  %109 = mul i64 %107, %108
  %110 = mul i64 %109, 4
  %111 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %73, 1
  %112 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %73, 2
  %113 = getelementptr float, ptr %111, i64 %112
  %114 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %105, 1
  %115 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %105, 2
  %116 = getelementptr float, ptr %114, i64 %115
  call void @llvm.memcpy.p0.p0.i64(ptr %116, ptr %113, i64 %110, i1 false)
  %117 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %48, 1
  %118 = ptrtoint ptr %117 to i64
  %119 = inttoptr i64 %118 to ptr
  %120 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 0
  %121 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %122 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } poison, ptr %120, 0
  %123 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %122, ptr %121, 1
  %124 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %123, i64 0, 2
  %125 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %124, i64 8, 3, 0
  %126 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %125, i64 8, 4, 0
  %127 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %126, i64 8, 3, 1
  %128 = insertvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %127, i64 1, 4, 1
  %129 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %92, 3, 0
  %130 = mul i64 1, %129
  %131 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %92, 3, 1
  %132 = mul i64 %130, %131
  %133 = mul i64 %132, 4
  %134 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %92, 1
  %135 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %92, 2
  %136 = getelementptr float, ptr %134, i64 %135
  %137 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %128, 1
  %138 = extractvalue { ptr, ptr, i64, [2 x i64], [2 x i64] } %128, 2
  %139 = getelementptr float, ptr %137, i64 %138
  call void @llvm.memcpy.p0.p0.i64(ptr %139, ptr %136, i64 %133, i1 false)
  %140 = extractvalue { ptr, ptr, i64, [1 x i64], [1 x i64] } %54, 1
  %141 = ptrtoint ptr %140 to i64
  %142 = inttoptr i64 %141 to ptr
  %143 = call i32 @fpga_tile_submit(ptr %96, i32 0, i32 0, i32 1, i32 7, i32 8, i32 8, ptr %119, ptr %142, ptr %95, i32 8, i32 0, i32 0, i32 117)
  %144 = call i32 @fpga_gemm_wait(ptr %96)
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
