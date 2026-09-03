//comandos internos de la shell
#ifndef BUILTINS_H
#define BUILTINS_H

#include "parser.h"

//retorna 1 si name es un comando interno, 0 si no
int is_builtin(const char *name);

//ejecuta el built in en el proceso de la shell y retorna el codigo de salida (0 si éxito)
int run_builtin(command_t *cmd);

#endif 
