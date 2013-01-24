; ModuleID = 'Misc/20.4b.c'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@main.U = private unnamed_addr constant [5 x i32] [i32 10, i32 9, i32 8, i32 7, i32 6], align 16
@main.X = private unnamed_addr constant [5 x i32] [i32 5, i32 4, i32 3, i32 2, i32 1], align 16

define i32 @main() nounwind uwtable {
  %1 = alloca i32, align 4
  %U = alloca [5 x i32], align 16
  %X = alloca [5 x i32], align 16
  %W = alloca [5 x i32], align 16
  %V = alloca [5 x i32], align 16
  %a = alloca i32, align 4
  %b = alloca i32, align 4
  %c = alloca i32, align 4
  %d = alloca i32, align 4
  %e = alloca i32, align 4
  %f = alloca i32, align 4
  %j = alloca i32, align 4
  %i = alloca i32, align 4
  store i32 0, i32* %1
  %2 = bitcast [5 x i32]* %U to i8*
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %2, i8* bitcast ([5 x i32]* @main.U to i8*), i64 20, i32 16, i1 false)
  %3 = bitcast [5 x i32]* %X to i8*
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %3, i8* bitcast ([5 x i32]* @main.X to i8*), i64 20, i32 16, i1 false)
  %4 = bitcast [5 x i32]* %W to i8*
  call void @llvm.memset.p0i8.i64(i8* %4, i8 0, i64 20, i32 16, i1 false)
  %5 = bitcast [5 x i32]* %V to i8*
  call void @llvm.memset.p0i8.i64(i8* %5, i8 0, i64 20, i32 16, i1 false)
  store i32 0, i32* %j, align 4
  store i32 1, i32* %b, align 4
  store i32 2, i32* %f, align 4
  store i32 3, i32* %e, align 4
  store i32 1, i32* %i, align 4
  br label %6

; <label>:6                                       ; preds = %29, %0
  %7 = load i32* %i, align 4
  %8 = icmp slt i32 %7, 5
  br i1 %8, label %9, label %32

; <label>:9                                       ; preds = %6
  %10 = load i32* %j, align 4
  %11 = load i32* %b, align 4
  %12 = add nsw i32 %10, %11
  store i32 %12, i32* %a, align 4
  %13 = load i32* %a, align 4
  %14 = load i32* %f, align 4
  %15 = add nsw i32 %13, %14
  store i32 %15, i32* %b, align 4
  %16 = load i32* %e, align 4
  %17 = load i32* %j, align 4
  %18 = add nsw i32 %16, %17
  store i32 %18, i32* %c, align 4
  %19 = load i32* %f, align 4
  %20 = load i32* %c, align 4
  %21 = add nsw i32 %19, %20
  store i32 %21, i32* %d, align 4
  %22 = load i32* %b, align 4
  %23 = load i32* %d, align 4
  %24 = add nsw i32 %22, %23
  store i32 %24, i32* %e, align 4
  %25 = load i32* %i, align 4
  %26 = mul nsw i32 %25, 2
  store i32 %26, i32* %f, align 4
  %27 = load i32* %i, align 4
  %28 = mul nsw i32 %27, 4
  store i32 %28, i32* %j, align 4
  br label %29

; <label>:29                                      ; preds = %9
  %30 = load i32* %i, align 4
  %31 = add nsw i32 %30, 1
  store i32 %31, i32* %i, align 4
  br label %6

; <label>:32                                      ; preds = %6
  ret i32 0
}

declare void @llvm.memcpy.p0i8.p0i8.i64(i8* nocapture, i8* nocapture, i64, i32, i1) nounwind

declare void @llvm.memset.p0i8.i64(i8* nocapture, i8, i64, i32, i1) nounwind
