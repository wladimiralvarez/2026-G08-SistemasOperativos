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

        printf("[%d] %-12s %s\n",
               jobs[i].id,
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

void jobs_mark_done(pid_t pid, int status)
{
    /*
     * TODO R5: buscar el job con ese pid y hacer
     *     jobs[i].state  = JOB_DONE;
     *     jobs[i].status = status;
     */
    (void)pid;      
    (void)status;   
}

void jobs_report_finished(void)
{
    /*
     * TODO R5: recorrer la tabla y por cada job en JOB_DONE imprimir:
     *     [1]+ Done   sleep 30
     *y dejar la ranura libre (state = JOB_FREE) para que no se avise dos veces. 
     */
}
