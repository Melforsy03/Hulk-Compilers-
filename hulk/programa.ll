declare i32 @printf(i8*, ...)
@print.str = constant [4 x i8] c"%d\0A\00"

define i32 @main() {
  %1 = add i32 1, 2
  %x = alloca i32
  store i32 %1, i32* %x
  %x_val = load i32, i32* %x
  %2 = sub i32 0, 5
  %y = alloca i32
  store i32 %2, i32* %y
  %y_val = load i32, i32* %y
  call i32 (i8*, ...) @printf(i8* getelementptr ([4 x i8], [4 x i8]* @print.str, i32 0, i32 0), i32 %x_val)
  call i32 (i8*, ...) @printf(i8* getelementptr ([4 x i8], [4 x i8]* @print.str, i32 0, i32 0), i32 %y_val)
  ret i32 0
}
