; ModuleID = 'Misc/20.4b.c'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

define i32 @main() nounwind uwtable {
  %1 = alloca i32, align 4
  %a = alloca i32, align 4
  %b = alloca i32, align 4
  %c = alloca i32, align 4
  %d = alloca i32, align 4
  %e = alloca i32, align 4
  %f = alloca i32, align 4
  %j = alloca i32, align 4
  %i = alloca i32, align 4
  store i32 0, i32* %1
  store i32 0, i32* %j, align 4
  store i32 1, i32* %b, align 4
  store i32 2, i32* %f, align 4
  store i32 3, i32* %e, align 4
  store i32 1, i32* %i, align 4
  br label %2

; <label>:2                                       ; preds = %25, %0
  %3 = load i32* %i, align 4
  %4 = icmp slt i32 %3, 5
  br i1 %4, label %5, label %28

; <label>:5                                       ; preds = %2
  %6 = load i32* %j, align 4
  %7 = load i32* %b, align 4
  %8 = add nsw i32 %6, %7
  store i32 %8, i32* %a, align 4
  %9 = load i32* %a, align 4
  %10 = load i32* %f, align 4
  %11 = add nsw i32 %9, %10
  store i32 %11, i32* %b, align 4
  %12 = load i32* %e, align 4
  %13 = load i32* %j, align 4
  %14 = sub nsw i32 %12, %13
  store i32 %14, i32* %c, align 4
  %15 = load i32* %f, align 4
  %16 = load i32* %c, align 4
  %17 = sub nsw i32 %15, %16
  store i32 %17, i32* %d, align 4
  %18 = load i32* %b, align 4
  %19 = load i32* %d, align 4
  %20 = add nsw i32 %18, %19
  store i32 %20, i32* %e, align 4
  %21 = load i32* %i, align 4
  %22 = sdiv i32 %21, 2
  store i32 %22, i32* %f, align 4
  %23 = load i32* %i, align 4
  %24 = mul nsw i32 %23, 4
  store i32 %24, i32* %j, align 4
  br label %25

; <label>:25                                      ; preds = %5
  %26 = load i32* %i, align 4
  %27 = add nsw i32 %26, 1
  store i32 %27, i32* %i, align 4
  br label %2

; <label>:28                                      ; preds = %2
  ret i32 0
}
