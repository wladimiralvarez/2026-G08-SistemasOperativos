
//estructuras de datos del comando, la tuberia y el parser, el parser no ejecuta nada, solo traduce para el executor

#ifndef PARSER_H
#define PARSER_H

#include "mishell.h"

//el programa a ejecutar, sus argumentos y a que archivos hay que conectar su entrada/salida
typedef struct {
    
    char *argv[MAX_ARGS + 1];
    int   argc;

    //redireccion: null no redirigir, hereda el de la shell
    char *infile;    //  < archivo   
    char *outfile;   //  > archivo   o   >> archivo
    int   append;    //  1 si era >> (abrir en modo append), 0 si era >
} command_t;

//una linea es una tuberia de 1 o mas comandos, una linea sin pipes es una tuberia de largo 1
typedef struct {
    command_t cmds[MAX_CMDS];
    int       ncmds;       // cuántos comandos hay realmente en cmds
    int       background;  // 1 si la línea terminaba en &

    char      rawline[MAX_LINE];   // linea original

    //los argv apuntan aca dentro
    char      work[MAX_LINE * 3];
} pipeline_t;

/*
 * retorna:
 *    1  línea parseada correctamente
 *    0  línea vacía o solo espacios
 *   -1  error de sintaxis
 */
int parse_line(const char *line, pipeline_t *pl);

#endif 
