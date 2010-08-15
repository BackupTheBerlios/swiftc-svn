#include <stdio.h>
#include <sys/time.h> 
#include <sys/resource.h> 
#include <stdlib.h>
#include <stdint.h>

static struct rusage start;

static struct timeval sub(const struct timeval* t1, const struct timeval* t2)
{
    struct timeval res;
    res.tv_sec  = t1->tv_sec  - t2->tv_sec;
    res.tv_usec = t1->tv_usec - t2->tv_usec;

    if (res.tv_usec < 0)
    {
        res.tv_usec = res.tv_usec + 1000000;
        --res.tv_sec;
    }

    return res;
}

void start_timer()
{
    getrusage(RUSAGE_SELF, &start);
}

void stop_timer()
{
    struct rusage end;
    getrusage(RUSAGE_SELF, &end);

    struct timeval utime = sub(&end.ru_utime, &start.ru_utime);
    struct timeval stime = sub(&end.ru_stime, &start.ru_stime);

    printf("user time: %jd:%06jd\n",  
            (intmax_t) utime.tv_sec,
            (intmax_t) utime.tv_usec);
    printf("system time: %jd:%06jd\n",  
            (intmax_t) stime.tv_sec,
            (intmax_t) stime.tv_usec);
}

float rand_float()
{
    return ((float) rand() / (float) RAND_MAX) * 2.f - 1.f;
}

void println()
{
    printf("\n");
}

void print_int(int n)
{
    printf("%i\n", n);
}

void print_uint(int n)
{
    printf("%u\n", n);
}

void print_byte(int n)
{
    printf("%c", n);
}

void print_float(float f)
{
    printf("%f\n", (float) f);
}

void print_double(double d)
{
    printf("%f\n", d);
}
