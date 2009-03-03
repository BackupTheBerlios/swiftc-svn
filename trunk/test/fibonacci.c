#include <stdio.h>
#include <stdlib.h>

int swift_1(int);

int main(int argc, char** argv)
{
    printf( "%i\n", swift_1(atoi(argv[1])) );
    return 0;
}
