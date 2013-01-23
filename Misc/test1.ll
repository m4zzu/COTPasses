; ModuleID = 'Misc/test1.c'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

define i32 @main() nounwind uwtable {
  %1 = alloca i32, align 4
  %i = alloca i32, align 4
  %a = alloca i32, align 4
  %b = alloca i32, align 4
  store i32 0, i32* %1
  store i32 0, i32* %i, align 4
  store i32 1, i32* %a, align 4
  store i32 0, i32* %b, align 4
  br label %2

; <label>:2                                       ; preds = %5, %0
  %3 = load i32* %i, align 4
  %4 = icmp slt i32 %3, 8
  br i1 %4, label %5, label %13

; <label>:5                                       ; preds = %2
  %6 = load i32* %a, align 4
  %7 = load i32* %b, align 4
  %8 = add nsw i32 %6, %7
  store i32 %8, i32* %i, align 4
  %9 = load i32* %b, align 4
  %10 = add nsw i32 %9, 1
  store i32 %10, i32* %b, align 4
  %11 = load i32* %a, align 4
  %12 = mul nsw i32 %11, 4
  store i32 %12, i32* %a, align 4
  br label %2

; <label>:13                                      ; preds = %2
  ret i32 0
}
