//tokenizar la linea de comandos
#include <stdio.h>
#include <string.h>

#include "parser.h"

//caracteres que separan tokens, al pasar varios separadores seguidos cuentan como uno solo

static const char *DELIMS = " \t\r\n";

int parse_line(char *line, pipeline_t *pl)
{
    char      *tok;
    command_t *cmd;

    //iniciamos la estructura en 0, punteros en null y background 0 para no arrastrar basura de la iteración anterior
    memset(pl, 0, sizeof(*pl));

    //copia de respaldo antes de tocar la linea
    strncpy(pl->rawline, line, MAX_LINE - 1);
    pl->rawline[MAX_LINE - 1] = '\0';

    //strcspn devuelve la posicion del primer \n, se le escribe \0 encima para sacar el salto de linea que deja fgets   
    pl->rawline[strcspn(pl->rawline, "\n")] = '\0';

    //toda linea es una tuberia de un solo comando por ahora
    pl->ncmds = 1;
    cmd = &pl->cmds[0];

    for (tok = strtok(line, DELIMS); tok != NULL; tok = strtok(NULL, DELIMS)) {

        /*
         * TODO R5: si tok es & y es el último token,
         *
         * TODO R4: si tok es |, cerrar el comando actual
         *
         * TODO R3: si tok es <, > o >>, el siguiente token es el nombre del archivo
         */

        if (cmd->argc >= MAX_ARGS) {
            fprintf(stderr, "mishell: demasiados argumentos (máximo %d)\n", MAX_ARGS);
            return -1;
        }
        cmd->argv[cmd->argc++] = tok;
    }

    // el null final que execvp necesita para saber donde termina argv 
    cmd->argv[cmd->argc] = NULL;

    // Línea vacía o solo espacios
    if (pl->cmds[0].argc == 0)
        return 0;

    return 1;
}
