// constantes y límites compartidos.

#ifndef MISHELL_H
#define MISHELL_H

//largo máximo de una linea de comandos desde stdin
#define MAX_LINE      1024

// maximo de argumentos por comando sin contar el null final de execvp
#define MAX_ARGS        64

// maximo de comandos encadenados en una tubería
#define MAX_CMDS        16

// maximo de jobs en background que la shell puede recordar a la vez
#define MAX_JOBS        32

// largo máximo de una ruta para el prompt 
#define MAX_PATH_LEN  4096

#endif 
