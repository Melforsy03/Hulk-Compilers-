; archivo LLVM generado
%Point = type { i32, i32 }
declare void @print(i32)

define i32 @main() {
entry:
  %0 = add i32 0, 4
  %1 = add i32 0, 5
  %2 = icmp eq i32 %0, %1
  br i1 %2, label %L0, label %L1
L0:
  %3 = add i32 0, 99
  call void @print(i32 %3)
  br label %L2
L1:
  %4 = add i32 0, 11
  call void @print(i32 %4)
  br label %L2
L2:
  ret i32 0
}
