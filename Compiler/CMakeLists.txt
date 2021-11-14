cmake_minimum_required(VERSION 3.19)
project(Hunter_Language_Compiler)

set(CMAKE_CXX_STANDARD 20)
set(LLVM_DIR /usr/local/opt/llvm/lib/cmake/llvm)
find_package(LLVM CONFIG REQUIRED)

include_directories("/usr/local/opt/llvm/include")

add_executable(Hunter_Compiler src/main.cpp src/Parser.cpp src/Parser.h src/CodeGenerator.cpp src/CodeGenerator.h src/Compiler.cpp src/Compiler.h)

llvm_map_components_to_libnames(llvm_libraries analysis support core object target  irreader executionengine scalaropts instcombine orcjit runtimedyld)

target_include_directories(Hunter_Compiler PUBLIC ${LLVM_INCLUDE_DIRS})
target_compile_definitions(Hunter_Compiler PUBLIC ${LLVM_DEFINITIONS})


foreach(target ${LLVM_TARGETS_TO_BUILD})
    list(APPEND targets "LLVM${target}CodeGen")
endforeach()

target_link_libraries(Hunter_Compiler ${CONAN_LIBS})  # Specifies what libraries to link, using Conan.
target_link_libraries(Hunter_Compiler ${llvm_libraries} ${targets})

