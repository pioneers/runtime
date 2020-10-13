#ifndef SIMPLE_REGEX_H
#define SIMPLE_REGEX_H

#include <regex.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>

/**
 * An implementation of strstr that uses regex
 * Functions exactly the same, but has regex
 * capabilities. If nothing is found in the
 * HAYSTACK, then it will return NULL, otherwise
 * it will return the rest of the HAYSTACK after
 * the first instance of NEEDLE. 
 * 
 * Example usage:
 * rstrstr("     There are 123456789 chickens here", 
 *      "[[:space]:]* There are [0-9]+")
 * returns 
 *      " chickens here"
 * 
 * 
 * Common Regex:
 * [[:space:]]* represents arbitrary whitespace
 * [0-9]+ represents a number
 * [a-z]+ lowercase word
 * [A-Z]+ uppercase word
 * [a-zA-Z]+ alphabetical words
 * [a-zA-Z0-9]+ alphanumeric strings
 * * represents zero or more instances
 * + represents one or more instances
 * "something here" will look for the entire string 'something here' within the haystack
 */

char *rstrstr(char *, const char *);

#endif
