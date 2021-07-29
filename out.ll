; ModuleID = 'input'
source_filename = "input"

@_ZN105input1f = private global i8 5, align 1

define void @_ZN105input1f.1(i32 %0) {
entry:
  %i = alloca i8, align 1
  store i8 5, i8* %i, align 1
  ret void
}
