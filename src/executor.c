// fork + execvp + waitpid y el lugar donde iran los pipes
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include "executor.h"
#include "builtins.h"
#include "signals.h"
#include "jobs.h"

//traduce el estado de waitpid a un codigo de salida legible, si el proceso murio por una señal el codigo es 128 + numero de señal
static int status_to_code(int status)
{
    if (WIFEXITED(status))
        return WEXITSTATUS(status);
    if (WIFSIGNALED(status))
        return 128 + WTERMSIG(status);
    return 0;
}

//conecta stdin y stdout del hijo a los archivos que pidio el usuario
static int apply_redirections(command_t *cmd)
{
    int fd;

    if (cmd->infile != NULL) {

        fd = open(cmd->infile, O_RDONLY);
        if (fd == -1) {
            fprintf(stderr, "mishell: %s: %s\n", cmd->infile, strerror(errno));
            return -1;
        }

        //el descriptor 0 pasa a apuntar al archivo
        if (dup2(fd, STDIN_FILENO) == -1) {
            perror("mishell: dup2");
            return -1;
        }

        close(fd);
    }

    if (cmd->outfile != NULL) {

        //O_TRUNC vacia el archivo y O_APPEND escribe al final
        int flags = O_WRONLY | O_CREAT | (cmd->append ? O_APPEND : O_TRUNC);

        //0644 son los permisos por si hay que crearlo, lectura y escritura al dueño y lectura al resto
        fd = open(cmd->outfile, flags, 0644);
        if (fd == -1) {
            fprintf(stderr, "mishell: %s: %s\n", cmd->outfile, strerror(errno));
            return -1;
        }

        if (dup2(fd, STDOUT_FILENO) == -1) {
            perror("mishell: dup2");
            return -1;
        }

        close(fd);
    }

    return 0;
}

int execute_pipeline(pipeline_t *pl)
{
    command_t *cmd = &pl->cmds[0];
    pid_t      pid;
    int        status;

    //un built in en primer plano se ejecuta en la shell, si estuviera dentro de una tuberia bash lo corre en un hijo
    if (pl->ncmds == 1 && !pl->background && is_builtin(cmd->argv[0]))
        return run_builtin(cmd);

    // TODO R4: pipes de largo arbitrario.
    if (pl->ncmds > 1) {
        fprintf(stderr, "mishell: los pipes todavia no estan implementados\n");
        return 1;
    }

    pid = fork();

    if (pid < 0) {
        perror("mishell: fork");
        return -1;
    }

    if (pid == 0) {

        //el hijo hereda el SIG_IGN de la shell, hay que restaurarlo antes del exec
        signals_reset_child();

        //el programa arranca con los descriptores ya puestos
        if (apply_redirections(cmd) == -1)
            _exit(1);

        execvp(cmd->argv[0], cmd->argv);

        // si execvp retorna es porque falló   
        fprintf(stderr, "mishell: %s: %s\n", cmd->argv[0], strerror(errno));

        //_exit y no exit, el hijo heredó los buffers del padre y se imprimirían dos veces
        _exit(127);
    }

    //proceso padre

    if (pl->background) {
        
        //registramos el job y volvemos al prompt, se debe hacer que sigchld recoja a este hijo
        int id = jobs_add(pid, pl->rawline);
        if (id > 0)
            printf("[%d] %d\n", id, (int)pid);
        return 0;
    }

    //bloqueamos hasta que el hijo termine
    if (waitpid(pid, &status, 0) == -1) {
        perror("mishell: waitpid");
        return -1;
    }

    return status_to_code(status);
}
