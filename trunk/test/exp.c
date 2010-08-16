#include <math.h>
#include <stdlib.h>

float rand_float();
void start_timer();
void stop_timer();

int main()
{
    float* a = (float*) malloc(40000000 * sizeof(float));
    for (size_t i = 0; i < 40000000; ++i)
        a[i] = rand_float();

    start_timer();
    for (size_t i = 0; i < 40000000; ++i)
        a[i] = expf(a[i]);
    stop_timer();
}
