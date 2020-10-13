#include "simple_regex.h"


char *rstrstr(char *haystack, const char *needle) {
    regmatch_t tracker;
    regex_t expr;

    if (regcomp(&expr, needle, REG_EXTENDED)) {
        fprintf(stderr, "Unable to build regex expression");
        return NULL;
    }
    if (regexec(&expr, haystack, 1, &tracker, REG_EXTENDED)) {
        return NULL;
    }
    return haystack + tracker.rm_so;
}

