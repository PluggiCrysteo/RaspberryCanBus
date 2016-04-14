#ifdef DEBUG
#include <iostream>
#define DEBUG_(x, ...) fprintf(stderr,"\033[31m%s/%s/%d: " x "\033[0m\n",__FILE__,__func__,__LINE__,##__VA_ARGS__);
#else
#define DEBUG_(x, ...)
#endif
