; Generado automáticamente por el compilador

declare i8* @strdup(i8*)
declare i32 @llvm.pow.i32(i32, i32)
declare void @print_int(i32)
declare void @print_str(i8*)

declare i8* @int_to_string(i32)
declare i8* @strcat2(i8*, i8*)

@.str.0 = private unnamed_addr constant [23 x i8] c"The meaning of life is\00"
define i32 @main() {
entry:
  %number = alloca i32
  %0 = add i32 0, 42
  store i32 %0, i32* %number
  %text = alloca i8*
  %1 = call i8* @strdup(i8* getelementptr inbounds ([23 x i8], [23 x i8]* @.str.0, i32 0, i32 0))
  store i8* %1, i8** %text
  %2 = load i8*, i8** %text
  %3 = load i32, i32* %number
  %4 = call i8* @int_to_string(i32 %3)
  %5 = call i8* @strcat2(i8* %2, i8* %4)
  call void @print_str(i8* %5)
  ret i32 0
}
