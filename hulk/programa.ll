declare i32 @printf(i8*, ...)
@print.str = constant [4 x i8] c"%d\0A\00"

%Rectangle = type { i8**, i32, i32, i32, i32 }
%Circle = type { i8**, i32, i32, i32 }
%Shape = type { i8**, i32 }
@Rectangle_vtable = global [1 x i8* (%Rectangle*)*] [
  i8* (%Rectangle*)* @Rectangle_area
]

@Circle_vtable = global [1 x i8* (%Circle*)*] [
  i8* (%Circle*)* @Circle_area
]

@Shape_vtable = global [1 x i8* (%Shape*)*] [
  i8* (%Shape*)* @Shape_area
]

define i32 @Rectangle_area(%Rectangle* %this) {
  %1 = bitcast %Rectangle* %this to %Rectangle*
  %2 = getelementptr %Rectangle, %Rectangle* %1, i32 0, i32 2
  %3 = load i32, i32* %2
  %4 = bitcast %Rectangle* %this to %Rectangle*
  %5 = getelementptr %Rectangle, %Rectangle* %4, i32 0, i32 1
  %6 = load i32, i32* %5
  %7 = mul i32 %3, %6
  ret i32 %7
}

define i32 @Circle_area(%Circle* %this) {
  %8 = bitcast %Circle* %this to %Circle*
  %9 = getelementptr %Circle, %Circle* %8, i32 0, i32 1
  %10 = load i32, i32* %9
  %11 = mul i32 4, %10
  %12 = bitcast %Circle* %this to %Circle*
  %13 = getelementptr %Circle, %Circle* %12, i32 0, i32 1
  %14 = load i32, i32* %13
  %15 = mul i32 %11, %14
  ret i32 %15
}

define i32 @Shape_area(%Shape* %this) {
  ret i32 0
}

define i32 @main() {
  %16 = alloca %Shape
  %17 = getelementptr %Shape, %Shape* %16, i32 0, i32 0
  store i8** @Shape_vtable, i8*** %17
  %circle = alloca %Shape*
  store %Shape* %16, %Shape** %circle
  %18 = load %Shape*, %Shape** %circle
  %rectangle = alloca i32
  store i32 0, i32* %rectangle
  %19 = load i32, i32* %rectangle
  %x = alloca i32
  store i32 0, i32* %x
  %20 = load i32, i32* %x
  %area1 = alloca i32
  store i32 0, i32* %area1
  %21 = load i32, i32* %area1
  %22 = load i32, i32* %x
  %23 = icmp sgt i32 %22, 3
  %24 = zext i1 %23 to i32
  %cond25 = icmp ne i32 %24, 0
  br i1 %cond25, label %then25, label %else25
  then25:
    %26 = alloca %Circle
    %27 = getelementptr %Circle, %Circle* %26, i32 0, i32 0
    store i8** @Circle_vtable, i8*** %27
    %28 = getelementptr %Circle, %Circle* %26, i32 0, i32 1
    store i32 1, i32* %28
    %29 = bitcast %Circle* %26 to %Shape*
    store %Shape* %29, %Shape** %circle
    br label %endif25
  else25:
    %30 = alloca %Rectangle
    %31 = getelementptr %Rectangle, %Rectangle* %30, i32 0, i32 0
    store i8** @Rectangle_vtable, i8*** %31
    %32 = getelementptr %Rectangle, %Rectangle* %30, i32 0, i32 1
    store i32 4, i32* %32
    %33 = getelementptr %Rectangle, %Rectangle* %30, i32 0, i32 2
    store i32 2, i32* %33
    %34 = bitcast %Rectangle* %30 to %Shape*
    store %Shape* %34, %Shape** %circle
    br label %endif25
  endif25:
  %35 = load %Shape*, %Shape** %circle
  %36 = load %Shape*, %Shape** %circle
  %37 = load %Shape*, %Shape** %circle
  %38 = getelementptr %Shape, %Shape* %35, i32 0, i32 0
  %39 = load i8**, i8*** %38
  %40 = getelementptr i8*, i8** %39, i32 0
  %41 = load i8*, i8** %40
  %42 = bitcast i8* %41 to i32 (%Shape*)*
  %43 = bitcast %Shape* %35 to %Shape*
  %44 = call i32 %42(%Shape* %43)
  store i32 %44, i32* %area1
  %45 = load i32, i32* %area1
  %46 = call i32 (i8*, ...) @printf(i8* getelementptr ([4 x i8], [4 x i8]* @print.str, i32 0, i32 0), i32 %45)
  ret i32 0
}
