; ModuleID = 'input'
source_filename = "input"

define private void @_ZN5input4main() {
entry:
  ret void
}

define i32 @main(i32 %0, i8** %1) {
entry:
  call void @_ZN5input4main()
  ret i32 0
}
