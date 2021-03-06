cmake_minimum_required(VERSION 3.0.0)
project(grimec)
enable_language(C ASM)
enable_testing()

set(CMAKE_C_COMPILER /usr/bin/clang)
set(CMAKE_C_FLAGS -g\ --stdlib=libc++\ --rtlib=compiler-rt)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

file(GLOB_RECURSE HEADERS ${PROJECT_SOURCE_DIR}/src/*.h)
file(GLOB_RECURSE SOURCE ${PROJECT_SOURCE_DIR}/src/*.c)
file(GLOB_RECURSE ASM ${PROJECT_SOURCE_DIR}/src/main.s)
add_executable(grimec ${HEADERS} ${SOURCE} ${ASM})

file(GLOB TEST_FILES ${PROJECT_SOURCE_DIR}/test/*.gm)
foreach (TEST_FILE ${TEST_FILES})
	add_test(${TEST_FILE} ./grimec ${TEST_FILE})
endforeach()

llvm_map_components_to_libnames(LLVM_LIBS core mcjit x86codegen)

target_link_libraries(grimec ${LLVM_LIBS})

install(TARGETS grimec)
