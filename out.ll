; ModuleID = 'input'
source_filename = "input"

%input.Object = type { i32, i1 }

define private void @_ZN5input6Object11doSomething() {
entry:
  ret void
}

define private void @_ZN5input1f() {
entry:
  %o = alloca %input.Object, align 8
  %0 = getelementptr inbounds %input.Object, %input.Object* %o, i32 0, i32 1
  store i1 true, i1* %0, align 1
  %1 = getelementptr inbounds %input.Object, %input.Object* %o, i32 0, i32 0
  store i32 0, i32* %1, align 4
  ret void
}
