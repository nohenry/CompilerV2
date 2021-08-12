; ModuleID = 'input'
source_filename = "input"

%input.Object = type { i32, i1, %input.Inline }
%input.Inline = type { i32 }

define private void @_ZN5input6Object11doSomething() {
entry:
  ret void
}

define private void @_ZN5input1f() {
entry:
  %x = alloca i32, align 4
  %o = alloca %input.Object, align 8
  %0 = getelementptr inbounds %input.Object, %input.Object* %o, i32 0, i32 0
  store i32 0, i32* %0, align 4
  %1 = getelementptr inbounds %input.Object, %input.Object* %o, i32 0, i32 1
  store i1 true, i1* %1, align 1
  %2 = getelementptr inbounds %input.Object, %input.Object* %o, i32 0, i32 2
  %3 = getelementptr inbounds %input.Inline, %input.Inline* %2, i32 0, i32 0
  store i32 5, i32* %3, align 4
  %4 = getelementptr inbounds %input.Object, %input.Object* %o, i32 0, i32 2
  %5 = getelementptr inbounds %input.Inline, %input.Inline* %4, i32 0, i32 0
  %6 = load i32, i32* %5, align 4
  store i32 %6, i32* %x, align 4
  ret void
}
