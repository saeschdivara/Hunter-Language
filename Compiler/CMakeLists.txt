cmake_minimum_required(VERSION 3.19)
project(Hunter_Language_Compiler)

set(CMAKE_CXX_STANDARD 20)
set(LLVM_DIR /usr/local/opt/llvm/lib/cmake/llvm)
find_package(LLVM CONFIG REQUIRED)

include_directories("/usr/local/opt/llvm/include")

add_executable(Hunter_Compiler

        src/main.cpp
        src/Parser.cpp src/Parser.h
        src/CodeGenerator.cpp src/CodeGenerator.h
        src/Compiler.cpp src/Compiler.h
        src/Expressions.cpp src/Expressions.h
        src/ImportResolver.cpp src/ImportResolver.h
        src/utils/strings.h src/utils/strings.cpp
        src/utils/files.h src/utils/files.cpp
        src/utils/logger.h src/utils/logger.cpp
        src/DebugData.cpp src/DebugData.h
        src/DebugGenerator.cpp src/DebugGenerator.h)

llvm_map_components_to_libnames(llvm_libraries analysis support core object target  irreader executionengine scalaropts instcombine orcjit runtimedyld)

target_include_directories(Hunter_Compiler PUBLIC ${LLVM_INCLUDE_DIRS})
target_compile_definitions(Hunter_Compiler PUBLIC ${LLVM_DEFINITIONS})


foreach(target ${LLVM_TARGETS_TO_BUILD})
    list(APPEND targets "LLVM${target}CodeGen")
endforeach()

target_link_libraries(Hunter_Compiler ${CONAN_LIBS})  # Specifies what libraries to link, using Conan.
target_link_libraries(Hunter_Compiler ${llvm_libraries} ${targets})

#########################
find_package(Catch2 2 REQUIRED)

add_executable(Parser_Test
        testing/Parser.cpp
        src/Parser.cpp src/Parser.h
        src/Expressions.cpp src/Expressions.h
        )

target_link_libraries(Parser_Test Catch2::Catch2)

target_include_directories(Parser_Test PUBLIC ${LLVM_INCLUDE_DIRS})
target_compile_definitions(Parser_Test PUBLIC ${LLVM_DEFINITIONS})

target_link_libraries(Parser_Test ${CONAN_LIBS})  # Specifies what libraries to link, using Conan.
target_link_libraries(Parser_Test ${llvm_libraries} ${targets})

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(CTest)
include(Catch)
catch_discover_tests(Parser_Test)
