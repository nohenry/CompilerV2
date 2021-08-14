cmake_minimum_required(VERSION 3.0.0)
project(CompilerV2 VERSION 0.1.0)
add_compile_definitions(PLATFORM_WINDOWS)

set(CMAKE_CXX_STANDARD 20)

include_directories()
set(LLVM_PATH "E:/llvm-project")
link_directories(${LLVM_PATH}/build/Debug/lib)
include_directories(${LLVM_PATH}/llvm/include)
include_directories(${LLVM_PATH}/build/include)

file(GLOB_RECURSE TARGET_SRC "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp" )
list(REMOVE_ITEM TARGET_SRC "${CMAKE_CURRENT_SOURCE_DIR}/src/test.cpp")
message(${TARGET_SRC})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

add_executable(CompilerV2 ${TARGET_SRC})


set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})

target_link_libraries(CompilerV2
  LLVMX86CodeGen
  LLVMX86Disassembler
  LLVMX86AsmParser
  LLVMX86CodeGen
  LLVMX86Disassembler
  LLVMGlobalISel
  LLVMX86Desc
  LLVMX86Info
  LLVMCFGuard
  LLVMGlobalISel
  LLVMMCDisassembler
  LLVMSelectionDAG
  LLVMAsmPrinter
  LLVMDebugInfoMSF
  LLVMDebugInfoDWARF
  LLVMCodeGen
  LLVMTarget
  LLVMScalarOpts
  LLVMInstCombine
  LLVMAggressiveInstCombine
  LLVMTransformUtils
  LLVMAnalysis
  LLVMProfileData
  LLVMObject
  LLVMTextAPI
  LLVMMCParser
  LLVMDebugInfoCodeView
  LLVMMC

  LLVMIRReader
  LLVMBitReader
  LLVMAsmParser
  LLVMCore # Support
  LLVMRemarks
  LLVMBitstreamReader
  LLVMBinaryFormat
  LLVMSupport
  LLVMDemangle
)

# set(CMAKE_VERBOSE_MAKEFILE yes)

# add_library(LLVMLib SHARED IMPORTED)

# # Provide the full path to the library, so CMake knows where to find it.

# set(llvm_components core)
# execute_process(COMMAND "llvm-config.exe" --libs ${llvm_components} OUTPUT_VARIABLE llvm_libs OUTPUT_STRIP_TRAILING_WHITESPACE )
# set_target_properties(LLVMLib PROPERTIES IMPORTED_IMPLIB ${llvm_libs})
# # set(llvm_libs "-LLLVMRemarks.lib ${llvm_libs}" )
# message(${llvm_libs} ${LLVMLib})

# # target_link_libraries(CompilerV2 ${llvm_libs})
# target_link_libraries(CompilerV2 PRIVATE LLVMLib)
