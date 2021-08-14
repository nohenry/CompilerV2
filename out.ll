; ModuleID = 'input'
source_filename = "input"

%input.Object = type { i32, i1, %input.Inline }
%input.Inline = type { i32 }

define private void @_ZN5input6Object11doSomething(%input.Object* %0, i32 %1) {
entry:
  %f = alloca i32, align 4
  %self = alloca %input.Object*, align 8
  store %input.Object* %0, %input.Object** %self, align 8
  store i32 %1, i32* %f, align 4
  %2 = load %input.Object*, %input.Object** %self, align 8
  %3 = getelementptr inbounds %input.Object, %input.Object* %2, i32 0, i32 0
  %4 = load i32, i32* %f, align 4
  store i32 %4, i32* %3, align 4
  ret void
}

define private %input.Object @_ZN5input6Object6create() {
entry:
  %0 = alloca %input.Object, align 8
  %1 = getelementptr inbounds %input.Object, %input.Object* %0, i32 0, i32 0
  store i32 0, i32* %1, align 4
  %2 = getelementptr inbounds %input.Object, %input.Object* %0, i32 0, i32 1
  store i1 true, i1* %2, align 1
  %3 = getelementptr inbounds %input.Object, %input.Object* %0, i32 0, i32 2
  %4 = getelementptr inbounds %input.Inline, %input.Inline* %3, i32 0, i32 0
  store i32 5, i32* %4, align 4
  %5 = load %input.Object, %input.Object* %0, align 4
  ret %input.Object %5
}

define private void @_ZN5input4main() {
entry:
  %o = alloca %input.Object, align 8
  %0 = call %input.Object @_ZN5input6Object6create()
  store %input.Object %0, %input.Object* %o, align 4
  call void @_ZN5input6Object11doSomething(%input.Object* %o, i32 5)
  ret void
}

define i32 @main(i32 %0, i8** %1) {
entry:
  call void @_ZN5input4main()
  ret i32 0
}
