declare i32 @printf(i8*, ...)
@print.str = constant [4 x i8] c"%d\0A\00"

%Rectangle = type { i32, i32, i32, i32 }
%Circle = type { i32, i32, i32 }
%Shape = type { i32 }
define i32 @Rectangle_area(%Rectangle* %this) {
  %1 = getelementptr %Rectangle, %Rectangle* %this, i32 0, i32 2
  %2 = load i32, i32* %1
  %3 = getelementptr %Rectangle, %Rectangle* %this, i32 0, i32 1
  %4 = load i32, i32* %3
  %5 = mul i32 %2, %4
  ret i32 %5
  ret i32 0
}

define i32 @Circle_area(%Circle* %this) {
  %6 = getelementptr %Circle, %Circle* %this, i32 0, i32 1
  %7 = load i32, i32* %6
  %8 = mul i32 3, %7
  %9 = getelementptr %Circle, %Circle* %this, i32 0, i32 1
  %10 = load i32, i32* %9
  %11 = mul i32 %8, %10
  ret i32 %11
  ret i32 0
}

define i32 @Shape_area(%Shape* %this) {
  ret i32 0
  ret i32 0
}

define i32 @main() {
  %12 = alloca %Shape
  %circle = alloca %Shape*
  store %Shape* %12, %Shape** %circle
  %13 = load %Shape*, %Shape** %circle
  %rectangle = alloca i32
  store i32 0, i32* %rectangle
  %14 = load i32, i32* %rectangle
  %x = alloca i32
  store i32 0, i32* %x
  %15 = load i32, i32* %x
  %area1 = alloca i32
  store i32 0, i32* %area1
  %16 = load i32, i32* %area1
  %17 = load i32, i32* %x
  %18 = icmp sgt i32 %17, 3
  %19 = zext i1 %18 to i32
  %cond20 = icmp ne i32 %19, 0
  br i1 %cond20, label %then20, label %else20
  then20:
    %21 = alloca %Circle
    store i32 %21, i32* %circle
    %22 = load i32, i32* %circle
    ret i32 %22
    br label %endif20
  else20:
    %23 = alloca %Rectangle
    store i32 %23, i32* %circle
    %24 = load i32, i32* %circle
    ret i32 %24
    br label %endif20
  endif20:
  %25 = bitcast %Shape* %24 to %Shape*
  %26 = call i32 @Shape_area(%Shape* %25)
  store i32 %26, i32* %area1
  %27 = load i32, i32* %area1
  %28 = load i32, i32* %area1
  %29 = call i32 (i8*, ...) @printf(i8* getelementptr ([4 x i8], [4 x i8]* @print.str, i32 0, i32 0), i32 %28)
  ret i32 0
}
