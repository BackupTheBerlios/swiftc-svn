#include <stdio.h>
#include <stdlib.h>

double swift_2(double);

int main(int argc, char** argv)
{
    printf( "%f\n", swift_2(atof(argv[1])) );
    return 0;
}
