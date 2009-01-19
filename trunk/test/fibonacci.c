#include <stdio.h>
#include <stdlib.h>

int swift_2(int);

int main(int argc, char** argv)
{
    printf( "%i\n", swift_2(atoi(argv[1])) );
    return 0;
}
