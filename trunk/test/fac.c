#include <stdio.h>
#include <stdlib.h>

float swift_1(float);

int main(int argc, char** argv)
{
    printf( "%f\n", swift_1(atof(argv[1])) );
    return 0;
}
