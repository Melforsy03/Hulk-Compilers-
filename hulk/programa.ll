; Generado automáticamente por el compilador

declare i8* @strdup(i8*)
declare i32 @int_pow(i32, i32)
declare void @print_int(i32)
declare void @print_str(i8*)
declare i8* @int_to_string(i32)
declare i8* @strcat2(i8*, i8*)

declare i32 @strcmp(i8*, i8*)
declare i8* @float_to_string(float)
declare i8* @bool_to_string(i1)
declare float @my_sin(float)
declare float @my_cos(float)
declare float @my_tan(float)
declare float @my_cot(float)
declare void @print_float(float)
define i32 @user_main() {
entry:
  %x = alloca i32
  %0 = add i32 0, 5
  store i32 %0, i32* %x
  %1 = load i32, i32* %x
  %2 = add i32 0, 1
  %3 = sub i32 %1, %2
  call void @print_int(i32 %3)
  ret i32 0
}
define i32 @main() {
entry:
  %ret = call i32 @user_main()
  ret i32 %ret
}
