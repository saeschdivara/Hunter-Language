./cmake-build-debug/bin/Hunter_Compiler Hunter-Compiler/compiler.hunt
clang -stdlib=libc++ ./output.o -o ./cmake-build-debug/bin/compiler
rm output.o
rm output.bc
./cmake-build-debug/bin/compiler ./Examples/hello-world.hunt