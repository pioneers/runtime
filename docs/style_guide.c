// This is Runtime's style guide!

// ************************************************** GENERAL **************************************** //

/*

1) Use 4 spaces for each indentation level. We recommend setting your text editor to convert tabs to 4 spaces ("soft" tabs)

2) Use just newline characters to mark the end of your line (sometimes knoown as linefeed, "LF"), instead of carriage return "CR" + linefeed "LF"
   There should be a setting in your text editor to view invisible characters or to make sure you have UNIX-style line endings.

3) Put one space between an inline comment and the beginning of the comment, like this: // here is my comment (i.e. not //here is my comment)

4) For a list of global variables at the top of the file, or a list of members in a structure, add spaces between
   the end of the variable or member until they all line up, and add inline comments describing what they do, for example:

    int important_var1;     // this is an important variable
    float important_var2;   // this is another important variable

    typedef struct {
        int bar;   <spaces> // this is a dummy variable
        float baz; <spaces> // in a dummy struct
    } foo_t;

5) Generally, comments inside of block comments (/* and */
/*) should be complete sentences and have proper capitalization. Inline comments, not so much.

6) Delimiters should look like the ones that you see in this doc, with the length consistent through any one file
    (By "delimiters", we mean the things like the // ********** GENERAL ************ // you see in this file)

7) For declaring pointers, use int *ivar, not int* ivar. Syntactically, they're no different, but in nearly all C manual (man) pages and reference documents,
    they use the notation int *ivar to denote pointers. So we'll stick to this convention (even though it could be argued that int* ivar makes more sense)

8) When logging errors from system calls or functions that set errno, follow this convention:
    
    int <function>() {
        ...
        if (<system_call>(<args>) != 0) {
            log_printf(ERROR, "<function>: <message describing operation that errorede>: %s", strerror(errno));
            <any additional error handling code>
        }
        ...
    }
    
    Note: the "!= 0" condition in the example above is not completely general. Most system calls return 0 on success, 
        but some functions don't (or don't set the value of errno). Notable examples are the mmap system call (which returns 
        the defined constant MAP_FAILED on failure), or all the pthread functions (which return the error number directly 
        on failure and don't set errno).
    
    Additionally, if the system call failing is something that should cause Runtime to crash completely, the level of the log
        should be set to FATAL, not ERROR, and the last statement in the "if" block should be exit(1) (or some other nonzero number)

9) Do not use the "goto" keyword. Ever. Please.

*/

// ********************************************* HEADER FILES *************************************** //

#ifndef SOMETHING_H  // always include these two lines (replace SOMETHING with a descriptive name) at the top of your header files
#define SOMETHING_H  // this prevents code from being included twice

#include <standard_headers.h>  // always include standard headers first (stdio, string, stlib, etc.), separated by a newline from the #ifndef and #define
#include <stdio.h>             // for printf, fprintf, fopen <-- include a message like this to indicate what you'll be using from that header

#include "c-runtime.h"  // separate standard headers with C-runtime headers with a newline

#endif  // put this at the end of all header files

// ******************************************** SOURCE CODE ***************************************** //

/*
 * Any public function should be listed in the header with a comment like this
 * Included the arguments and return values if there are any (it's ok to omit if either of these is void)
 * Arguments:
 *    a: copy the argument name exactly as in the function definition
 *    b: type a short description here. there are four spaces between the '*' and the '-'
 *        (if the description is too long, go onto the next line. There are 8 spaces between the '*' and the '(' )
 * Returns:
 *    0: for functions that return error codes, list them out like the arguments and what each error code means
 *    1: for functions that return data, it's okay to write it following the word, like this: "Returns: <put description here>"
 */
int example(int a, float b);

// There should be a newline between this function's definition and the next function's comment
// The comment describing a public function should NOT be copied into the source file above the implementation of the function.

/*
 * All private functions should be declared "static" in the source file and have a comment with 
 * the same structure as the public functions presented above.
 * Arguments:
 *    bar: this is a dummy variable
 *    baz: this is another dummy variable
 */
static void foo(int bar, int baz) {
    printf("this is a dummy function\n");
}

// The brace does not go on the next line, and the function begins immediately (no newlines)

// *********************************** FOR, IF, WHILE, ETC. ***************************************** //

// comments describing the function of a code block, if necessary, should appear above the condition
while (foo < 1) {
    // do something
}

// comments for else's should go after the open brace
if (foo == 0) {
    // foo is 0
} else {  // opening brace goes on same line
    // foo is not 0
}

// for loops is the same idea
for (int i = 0; i < n; i++) {
    // do something
}

// ************************************* VARIABLES, TYPEDEF ***************************************** //

// variables should have descriptive names, all lower case and words separated with underscores
int important_var;
float unimporant;

// typedefs should always have a "_t" at the end, by C convention. Here is an example:
typedef struct {
    int bar;
    float baz;
} foo_t;

// ************************************ CODE STRUCTURE TIPS ***************************************** //

/*
 * 1) Always remember the golden rule: only expose what you need to:
 *    - Don't put global variables in header files
 *    - Global variables in the source file should only be used when absolutely necessary. But if they're needed, they should be at the top of the file and well documented
 *    - #define stuff that might be needed outside of the module in the header. #define stuff that is only needed in that one source file, in that one source file
 *    - Don't make functions that don't need to be public, public. Keep as many functions as you can in the source file.
 *    All of this makes it a lot easier to work with each header file, a lot easier to maintain, and keeps the interactions between different parts of the code base as simple as possible
 */

// 2) If you have logic similar to this:
if (some_condition) {
    x = 1;
} else {
    x = 0;
}
// you can simplify it by writing it with the ternary operator
x = (some_condition) ? 1 : 0;

/*
 * 3) Use a switch-case instead of a long list of if-else if-else if.....-else when comparing a lot of integers or enums
 *    and each case has a relatively small amount of logic (4 - 5 lines at most). It's more obvious what's going on, we think
 *    (in the example below, it's obvious that important_number should one of val1 or val2)
 */
switch (important_number) {
    case val1:
        printf("one value\n");
        break;
    case val2:
        printf("second value\n");
        break;
    default:
        printf("something messed up???\n");
        break;
}
