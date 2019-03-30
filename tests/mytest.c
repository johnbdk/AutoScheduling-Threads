#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

int maxof(int, ...) ;
void f(void);

main(){
        f();
        exit(EXIT_SUCCESS);
}

int maxof(int n_args, ...){
        register int i;
        int max, a;
        va_list ap;

        va_start(ap, n_args);
//         max = va_arg(ap, int);
        printf("num = %d\n",n_args);
        for(i = 0; i < n_args; i++) {
//                 if((a = va_arg(ap, int)) > max)
//                         max = a;
            printf("arg = %d,i = %d\n",va_arg(ap, int),i);
        }

        va_end(ap);
        return max;
}

void f(void) {
        int i = 5;
        int j[256];
        j[42] = 24;
        printf("%d\n",maxof(3, i, j[42], 0));
}
