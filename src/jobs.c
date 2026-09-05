//tabla de jobs en background, se usa un arreglo estatico por seguridad 
#include <stdio.h>
#include <string.h>

#include "jobs.h"

static job_t jobs[MAX_JOBS];

// contador que solo aumenta
static int next_id;

void jobs_init(void)
{
    memset(jobs, 0, sizeof(jobs));
    next_id = 1;
}

int jobs_add(pid_t pid, const char *cmdline)
{
    int i;

    for (i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].state != JOB_FREE)
            continue;

        jobs[i].id     = next_id++;
        jobs[i].pid    = pid;
        jobs[i].state  = JOB_RUNNING;
        jobs[i].status = 0;

        strncpy(jobs[i].cmdline, cmdline, MAX_LINE - 1);
        jobs[i].cmdline[MAX_LINE - 1] = '\0';

        return jobs[i].id;
    }

    fprintf(stderr, "mishell: tabla de jobs llena (máximo %d)\n", MAX_JOBS);
    return -1;
}

void jobs_list(void)
{
    int i;

    for (i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].state == JOB_FREE)
            continue;

        printf("[%d] %d %-12s %s\n",
               jobs[i].id,
               (int)jobs[i].pid,
               jobs[i].state == JOB_RUNNING ? "Ejecutando" : "Terminado",
               jobs[i].cmdline);
    }
}

job_t *jobs_get(int index)
{
    if (index < 0 || index >= MAX_JOBS)
        return NULL;
    if (jobs[index].state == JOB_FREE)
        return NULL;
    return &jobs[index];
}

//corre dentro del manejador de SIGCHLD, asi que solo escribe en la tabla
void jobs_mark_done(pid_t pid, int status)
{
    int i;

    for (i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].state == JOB_RUNNING && jobs[i].pid == pid) {
            jobs[i].status = status;
            jobs[i].state  = JOB_DONE;
            return;
        }
    }

    //si el pid no esta en la tabla era un hijo de primer plano
}

//corre en el bucle principal
void jobs_report_finished(void)
{
    int i;

    for (i = 0; i < MAX_JOBS; i++) {

        if (jobs[i].state != JOB_DONE)
            continue;

        printf("[%d]+ Done   %s\n", jobs[i].id, jobs[i].cmdline);

        //liberamos la ranura para no avisar dos veces del mismo job
        jobs[i].state = JOB_FREE;
    }
}
