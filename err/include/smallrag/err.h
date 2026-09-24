#ifndef SMALLRAG_ERR_H
#define SMALLRAG_ERR_H

#include <stdio.h>
#include <stdnoreturn.h>


#ifndef smallragERR_OUT
#define smallragERR_OUT stderr
#endif

/* A constant pointer to a constant char. The value of the pointer itself
 * represents an error code; the entire string is the error name.
 * Use smallragDECLARE_ERR to declare error codes with their corresponding string names.
 */
typedef char *smallrag_err_t;

#define smallragDECLARE_ERR(name) const smallrag_err_t name = #name

smallragDECLARE_ERR(smallragERR_ARGUMENT);
smallragDECLARE_ERR(smallragERR_OUT_OF_BOUNDS);
smallragDECLARE_ERR(smallragERR_INVALID_STATE);
smallragDECLARE_ERR(smallragERR_UNSUPPORTED);

/* Pushes an error down the error stack. Do this when your function's execution ends up in an invalid state 
 * and you want to give the caller error information.
 */
void smallrag_pusherr(smallrag_err_t err);

/* Like smallrag_pusherr(), but adds an error message
 */
void smallrag_pusherrmsg(smallrag_err_t err, char *msg);


/* Pushes an error down the error stack. Works similarly to smallrag_pusherr(), but the error entry
 * pushed down the error stack is associated with the most recent standard errno (errno.h errno) rather than
 * the smallrag-specific smallrag_err_t;
 */
void smallrag_pusherrstd();

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

#endif
