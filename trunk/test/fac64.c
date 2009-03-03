#include <stdio.h>
#include <stdlib.h>

double swift_1(double);

int main(int argc, char** argv)
{
    printf( "%f\n", swift_1(atof(argv[1])) );
    return 0;
}
