#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include "../core/storm.h"
#include "../core/format.h"
#include "../core/monad.h"
#include "../core/alloc.h"

#define LINE_SIZE 2048

int main()
{
    a0_init();

    int run;
    char line[LINE_SIZE];
    char *ptr;

    run = 1;
    while (run)
    {
        printf(">");

        ptr = fgets(line, LINE_SIZE, stdin);

        printf("%s %p", line, ptr);

        g0 value = til(123);
        // g0 value = new_scalar_i64(9223372036854775807);
        str buffer;
        Result res = g0_fmt(&buffer, value);
        if (res == Ok)
        {
            printf("%s\n", buffer);
        }
        result_fmt(&buffer, res);
        printf("Result: %s\n", buffer);

        g0_free(value);
    }

    a0_deinit();

    return 0;
}
