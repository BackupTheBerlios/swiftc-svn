#include <stdio.h>
#include <stdlib.h>

float swift_2(float);

int main(int argc, char** argv)
{
    printf( "%f\n", swift_2(atof(argv[1])) );
    return 0;
}
