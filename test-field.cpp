#include <stdio.h>

void hunt() {
    const char * helloWorld = "Hello World";
    printf("%s%s", helloWorld, "\n");
}

int main(int argc, const char ** argv) {
    hunt();
    return 0;
}