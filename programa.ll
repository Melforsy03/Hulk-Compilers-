; Generado automáticamente por el compilador

declare i8* @strdup(i8*)
declare i32 @llvm.pow.i32(i32, i32)
declare void @print_int(i32)
declare void @print_str(i8*)

@.str.0 = private unnamed_addr constant [12 x i8] c"Hello World\00"
define i32 @main() {
entry:
  %msg = alloca i8*
  %0 = call i8* @strdup(i8* getelementptr inbounds ([12 x i8], [12 x i8]* @.str.0, i32 0, i32 0))
  store i8* %0, i8** %msg
  %1 = load i8*, i8** %msg
  call void @print_str(i8* %1)
  ret i32 0
}
