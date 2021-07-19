cmake_minimum_required(VERSION 3.0.0)
project(CompilerV2 VERSION 0.1.0)

include_directories()
set(LLVM_PATH "E:/llvm-project")
link_directories(${LLVM_PATH}/build/Debug/lib)
include_directories(${LLVM_PATH}/llvm/include)
include_directories(${LLVM_PATH}/build/include)

file(GLOB_RECURSE TARGET_SRC "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp" )

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

add_executable(CompilerV2 ${TARGET_SRC})


set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})

set(llvm_components support core irreader)
execute_process(COMMAND llvm-config.exe --libs ${llvm_components} OUTPUT_VARIABLE llvm_libs OUTPUT_STRIP_TRAILING_WHITESPACE )
message(${llvm_libs})

target_link_libraries(CompilerV2 ${llvm_libs})
