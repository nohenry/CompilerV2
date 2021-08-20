; ModuleID = 'input'
source_filename = "input"

%input.Object.0 = type { i32, i1 }
%input.Object = type { i32, i1 }

define private void @_ZN5input4main() {
entry:
  %o1 = alloca %input.Object.0, align 8
  %o = alloca %input.Object, align 8
  %0 = getelementptr inbounds %input.Object, %input.Object* %o, i32 0, i32 0
  store i32 0, i32* %0, align 4
  %1 = getelementptr inbounds %input.Object, %input.Object* %o, i32 0, i32 1
  store i1 true, i1* %1, align 1
  %2 = getelementptr inbounds %input.Object.0, %input.Object.0* %o1, i32 0, i32 0
  store i32 0, i32* %2, align 4
  %3 = getelementptr inbounds %input.Object.0, %input.Object.0* %o1, i32 0, i32 1
  store i1 true, i1* %3, align 1
  ret void
}

define i32 @main(i32 %0, i8** %1) {
entry:
  call void @_ZN5input4main()
  ret i32 0
}
