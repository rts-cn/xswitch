#include <switch.h>

SWITCH_BEGIN_EXTERN_C

char *expand(char *buf);
char *md5(const char *input);
int match(const char *input, const char *pattern);

SWITCH_END_EXTERN_C
