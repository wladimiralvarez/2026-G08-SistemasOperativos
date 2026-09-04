//tokenizar la linea de comandos
#include <stdio.h>
#include <string.h>

#include "parser.h"

//por donde va la lectura y donde se van copiando los tokens
typedef struct {
    const char *p;
    char       *buf;
    size_t      size;
    size_t      used;
    int         error;
} scanner_t;

static int is_space(char c)
{
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static int is_op_char(char c)
{
    return c == '<' || c == '>' || c == '|' || c == '&';
}

//agrega un caracter al token que se esta armando
static int put(scanner_t *sc, char c)
{
    if (sc->used + 1 >= sc->size) {
        fprintf(stderr, "mishell: linea demasiado larga\n");
        sc->error = 1;
        return 0;
    }
    sc->buf[sc->used++] = c;
    return 1;
}

//saca el siguiente token, devuelve NULL al terminar la linea o si hubo error
static char *next_token(scanner_t *sc, int *is_op)
{
    char *tok;

    *is_op = 0;

    while (is_space(*sc->p))
        sc->p++;

    if (*sc->p == '\0')
        return NULL;

    tok = sc->buf + sc->used;

    if (is_op_char(*sc->p)) {

        //un operador es un token completo por si solo
        char op = *sc->p++;

        if (!put(sc, op))
            return NULL;

        //>> es un solo operador
        if (op == '>' && *sc->p == '>') {
            sc->p++;
            if (!put(sc, '>'))
                return NULL;
        }

        *is_op = 1;

    } else {

        //una palabra llega hasta un espacio o un operador
        while (*sc->p != '\0' && !is_space(*sc->p) && !is_op_char(*sc->p)) {

            if (*sc->p == '"' || *sc->p == '\'') {

                //las comillas no van al token
                char quote = *sc->p++;

                while (*sc->p != quote) {
                    if (*sc->p == '\0') {
                        fprintf(stderr, "mishell: falta cerrar la comilla %c\n", quote);
                        sc->error = 1;
                        return NULL;
                    }
                    if (!put(sc, *sc->p++))
                        return NULL;
                }
                sc->p++;

            } else {
                if (!put(sc, *sc->p++))
                    return NULL;
            }
        }
    }

    sc->buf[sc->used++] = '\0';
    return tok;
}

int parse_line(const char *line, pipeline_t *pl)
{
    scanner_t  sc;
    command_t *cmd;
    char      *tok;
    int        is_op;

    //iniciamos la estructura en 0, punteros en null y background 0 para no arrastrar basura de la iteración anterior
    memset(pl, 0, sizeof(*pl));

    strncpy(pl->rawline, line, MAX_LINE - 1);
    pl->rawline[MAX_LINE - 1] = '\0';

    //strcspn devuelve la posicion del primer \n, se le escribe \0 encima para sacar el salto de linea que deja fgets
    pl->rawline[strcspn(pl->rawline, "\n")] = '\0';

    sc.p     = pl->rawline;
    sc.buf   = pl->work;
    sc.size  = sizeof(pl->work);
    sc.used  = 0;
    sc.error = 0;

    pl->ncmds = 1;
    cmd = &pl->cmds[0];

    for (;;) {

        tok = next_token(&sc, &is_op);

        if (sc.error)
            return -1;
        if (tok == NULL)
            break;

        //el & solo vale como ultimo token
        if (is_op && strcmp(tok, "&") == 0) {

            char *extra = next_token(&sc, &is_op);

            if (sc.error)
                return -1;
            if (extra != NULL) {
                fprintf(stderr, "mishell: el & tiene que ir al final de la linea\n");
                return -1;
            }

            pl->background = 1;
            break;
        }

        //cerramos el comando actual y pasamos al siguiente de la tuberia
        if (is_op && strcmp(tok, "|") == 0) {

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
        if (is_op && (strcmp(tok, "<") == 0 || strcmp(tok, ">") == 0 || strcmp(tok, ">>") == 0)) {

            char *op = tok;
            char *file;
            int   file_is_op;

            file = next_token(&sc, &file_is_op);

            if (sc.error)
                return -1;
            if (file == NULL || file_is_op) {
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
