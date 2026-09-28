#ifndef BUILTINS_H
#define BUILTINS_H

#include "lexer.h"

/* Returns 1 on success, 0 if not a built-in, or -1 on a built-in error. */
int handle_builtin(tokenlist *tokens);

#endif
