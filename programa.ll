; archivo LLVM generado
%Point = type { i32, i32 }
declare void @print(i32)

define i32 @main() {
entry:
  %p = alloca %Point
  %0 = getelementptr %Point, %Point* %p, i32 0, i32 0
  %1 = add i32 0, 42
  store i32 %1, i32* %0
  %2 = getelementptr %Point, %Point* %p, i32 0, i32 1
  %3 = add i32 0, 100
  store i32 %3, i32* %2
  %4 = getelementptr %Point, %Point* %p, i32 0, i32 0
  %5 = load i32, i32* %4
  %6 = add i32 0, 42
  %7 = icmp eq i32 %5, %6
  br i1 %7, label %L0, label %L1
L0:
  %8 = add i32 0, 1
  call void @print(i32 %8)
  br label %L2
L1:
  %9 = add i32 0, 0
  call void @print(i32 %9)
  br label %L2
L2:
  ret i32 0
}
