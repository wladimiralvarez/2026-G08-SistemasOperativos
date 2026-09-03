//tokenizar la linea de comandos
#include <stdio.h>
#include <string.h>

#include "parser.h"

//caracteres que separan tokens, al pasar varios separadores seguidos cuentan como uno solo

static const char *DELIMS = " \t\r\n";

//separa los operadores del texto pegado 
static int normalize(const char *src, char *dst, size_t dstsz)
{
    size_t j = 0;

    while (*src != '\0') {

        if (*src == '<' || *src == '>' || *src == '|' || *src == '&') {

            if (j + 4 >= dstsz)
                return -1;

            dst[j++] = ' ';
            dst[j++] = *src;

            //>> es un solo operador
            if (*src == '>' && *(src + 1) == '>') {
                dst[j++] = '>';
                src++;
            }

            dst[j++] = ' ';
            src++;

        } else {

            if (j + 2 >= dstsz)
                return -1;

            dst[j++] = *src++;
        }
    }

    dst[j] = '\0';
    return 0;
}

static int is_operator(const char *tok)
{
    return strcmp(tok, "<")  == 0 || strcmp(tok, ">") == 0 ||
           strcmp(tok, ">>") == 0 || strcmp(tok, "|") == 0 ||
           strcmp(tok, "&")  == 0;
}

int parse_line(const char *line, pipeline_t *pl)
{
    char      *tok;
    command_t *cmd;

    //iniciamos la estructura en 0, punteros en null y background 0 para no arrastrar basura de la iteración anterior
    memset(pl, 0, sizeof(*pl));

    strncpy(pl->rawline, line, MAX_LINE - 1);
    pl->rawline[MAX_LINE - 1] = '\0';

    //strcspn devuelve la posicion del primer \n, se le escribe \0 encima para sacar el salto de linea que deja fgets
    pl->rawline[strcspn(pl->rawline, "\n")] = '\0';

    if (normalize(pl->rawline, pl->work, sizeof(pl->work)) == -1) {
        fprintf(stderr, "mishell: linea demasiado larga\n");
        return -1;
    }

    pl->ncmds = 1;
    cmd = &pl->cmds[0];

    for (tok = strtok(pl->work, DELIMS); tok != NULL; tok = strtok(NULL, DELIMS)) {

        //el & solo vale como ultimo token
        if (strcmp(tok, "&") == 0) {
            if (strtok(NULL, DELIMS) != NULL) {
                fprintf(stderr, "mishell: el & tiene que ir al final de la linea\n");
                return -1;
            }
            pl->background = 1;
            break;
        }

        //cerramos el comando actual y pasamos al siguiente de la tuberia
        if (strcmp(tok, "|") == 0) {

            if (cmd->argc == 0) {
                fprintf(stderr, "mishell: falta un comando antes del |\n");
                return -1;
            }
            if (pl->ncmds >= MAX_CMDS) {
                fprintf(stderr, "mishell: demasiados comandos en la tuberia (maximo %d)\n", MAX_CMDS);
                return -1;
            }

            cmd->argv[cmd->argc] = NULL;
            cmd = &pl->cmds[pl->ncmds++];
            continue;
        }

        //despues de un operador de redireccion viene el archivo
        if (strcmp(tok, "<") == 0 || strcmp(tok, ">") == 0 || strcmp(tok, ">>") == 0) {

            char *op   = tok;
            char *file = strtok(NULL, DELIMS);

            if (file == NULL || is_operator(file)) {
                fprintf(stderr, "mishell: falta el archivo despues de %s\n", op);
                return -1;
            }

            if (strcmp(op, "<") == 0) {
                cmd->infile = file;
            } else {
                cmd->outfile = file;
                cmd->append  = (strcmp(op, ">>") == 0);
            }
            continue;
        }

        if (cmd->argc >= MAX_ARGS) {
            fprintf(stderr, "mishell: demasiados argumentos (máximo %d)\n", MAX_ARGS);
            return -1;
        }
        cmd->argv[cmd->argc++] = tok;
    }

    // el null final que execvp necesita para saber donde termina argv
    cmd->argv[cmd->argc] = NULL;

    // Línea vacía o solo espacios
    if (pl->ncmds == 1 && cmd->argc == 0 && !pl->background)
        return 0;

    //quedo un comando sin nombre, pasa con "ls |" o con un & suelto
    if (cmd->argc == 0) {
        fprintf(stderr, "mishell: falta un comando\n");
        return -1;
    }

    return 1;
}
