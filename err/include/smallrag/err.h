#ifndef SMALLRAG_ERR_H
#define SMALLRAG_ERR_H

#ifndef smallragERR_OUT
#include <stdio.h>
#define smallragERR_OUT stderr
#endif

/* A constant pointer to a constant string. The value of the pointer itself
 * represents an error code; the entire string is the error name.
 * Use smallragDECLARE_ERR to declare error codes with their corresponding string names.
 */
typedef const char *const smallrag_err_t;

#define smallragDECLARE_ERR(name) smallrag_err_t name = #name


/* Pushes an error down the error stack. Do this when your function's execution ends up in an invalid state 
 * and you want to give the caller error information.
 */
void smallrag_pusherr(smallrag_err_t err);

/* Gets the most recent error — the one at the top of the error stack.
 */
smallrag_err_t smallrag_geterr(); 

/* Clears the error stack; do that functions you called have pushed their own errors down the stack,
 * but you have resolved the errors.
 */
void smallrag_clearerr();

/* Prints out the entire error stack and exits the program.
 */
noreturn void smallrag_throw();

/* Prints out an error message for a standard error (errno.h) and exits the program.
 */
noreturn void smallrag_throwstd();


