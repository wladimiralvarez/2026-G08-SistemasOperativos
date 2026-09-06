//monitor de procesos
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

#include "pmon.h"
#include "jobs.h"

//las ponen los manejadores y las lee el ciclo. volatile para que el compilador no las cachee en un registro
static volatile sig_atomic_t tick;
static volatile sig_atomic_t salir;

//tiempo de cpu de la vuelta anterior
static struct {
    pid_t         pid;
    unsigned long cpu;
} previo[MAX_JOBS];

static void on_alarm(int s)
{
    (void)s;
    tick = 1;
}

static void on_int(int s)
{
    (void)s;
    salir = 1;
}

static int buscar_cpu(pid_t pid, unsigned long *cpu)
{
    int i;

    for (i = 0; i < MAX_JOBS; i++)
        if (previo[i].pid == pid) {
            *cpu = previo[i].cpu;
            return 1;
        }

    return 0;
}

static void guardar_cpu(pid_t pid, unsigned long cpu)
{
    int i, libre = -1;

    for (i = 0; i < MAX_JOBS; i++) {
        if (previo[i].pid == pid) {
            previo[i].cpu = cpu;
            return;
        }
        if (previo[i].pid == 0 && libre == -1)
            libre = i;
    }

    if (libre != -1) {
        previo[libre].pid = pid;
        previo[libre].cpu = cpu;
    }
}

//lee estado y tiempo de cpu de /proc/<pid>/stat, devuelve 0 si el proceso ya no esta
static int leer_stat(pid_t pid, char *estado, unsigned long *cpu)
{
    char  ruta[64], linea[1024], *p;
    FILE *f;
    unsigned long utime, stime;

    snprintf(ruta, sizeof(ruta), "/proc/%d/stat", (int)pid);

    //si el proceso murio entre dos lecturas el archivo ya no existe
    f = fopen(ruta, "r");
    if (f == NULL)
        return 0;

    if (fgets(linea, sizeof(linea), f) == NULL) {
        fclose(f);
        return 0;
    }
    fclose(f);

    //el nombre del comando viene entre parentesis y puede traer espacios y parentesis dentro, asi que contamos los campos desde el ultimo )
    p = strrchr(linea, ')');
    if (p == NULL)
        return 0;
    p++;

    if (sscanf(p, " %c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %lu %lu",
               estado, &utime, &stime) != 3)
        return 0;

    //el tiempo de cpu es lo gastado en modo usuario mas lo gastado en el kernel
    *cpu = utime + stime;
    return 1;
}

//busca VmRSS en /proc/<pid>/status
static long leer_rss(pid_t pid)
{
    char  ruta[64], linea[256];
    FILE *f;
    long  rss = 0;

    snprintf(ruta, sizeof(ruta), "/proc/%d/status", (int)pid);

    f = fopen(ruta, "r");
    if (f == NULL)
        return 0;

    while (fgets(linea, sizeof(linea), f) != NULL)
        if (strncmp(linea, "VmRSS:", 6) == 0) {
            sscanf(linea + 6, "%ld", &rss);
            break;
        }

    fclose(f);
    return rss;
}

static const char *nombre_estado(char c)
{
    switch (c) {
        case 'R': return "ejecutando";
        case 'S': return "durmiendo";
        case 'D': return "esperando";
        case 'Z': return "zombie";
        case 'T': return "detenido";
        default:  return "?";
    }
}

//juntamos las filas antes de imprimir
typedef struct {
    pid_t       pid;
    const char *cmd;
    const char *estado;
    double      pct;
    long        rss;
} fila_t;

//de mayor a menor %CPU
static int cmp_pct(const void *a, const void *b)
{
    const fila_t *x = a;
    const fila_t *y = b;

    if (x->pct < y->pct) return  1;
    if (x->pct > y->pct) return -1;
    return 0;
}

static void dibujar(int seconds, double transcurrido)
{
    long   ticks_seg = sysconf(_SC_CLK_TCK);   //cuantos ticks trae un segundo
    fila_t filas[MAX_JOBS];
    int    i, n = 0;

    for (i = 0; i < MAX_JOBS; i++) {

        job_t        *j = jobs_get(i);
        char          estado;
        unsigned long cpu, antes;
        double        pct = 0.0;

        if (j == NULL || j->state != JOB_RUNNING)
            continue;

        //el proceso pudo morir, en ese caso lo saltamos y en la siguiente vuelta ya no aparece
        if (!leer_stat(j->pid, &estado, &cpu))
            continue;

        //%CPU = ticks gastados desde la lectura anterior sobre el tiempo real
        if (buscar_cpu(j->pid, &antes) && cpu >= antes && transcurrido > 0.0 && ticks_seg > 0)
            pct = 100.0 * ((double)(cpu - antes) / (double)ticks_seg) / transcurrido;

        guardar_cpu(j->pid, cpu);

        filas[n].pid    = j->pid;
        filas[n].cmd    = j->cmdline;
        filas[n].estado = nombre_estado(estado);
        filas[n].pct    = pct;
        filas[n].rss    = leer_rss(j->pid);
        n++;
    }

    qsort(filas, n, sizeof(filas[0]), cmp_pct);

    printf("\033[H\033[J");

    printf("pmon - refresco cada %d s - Ctrl+C para volver al prompt\n\n", seconds);
    printf("%-8s %-26s %-12s %12s %10s\n",
           "PID", "COMANDO", "ESTADO", "%CPU(aprox)", "RSS(KB)");

    for (i = 0; i < n; i++) {

        //el que mas CPU usa va en negrita si usa algo
        int destacar = (i == 0 && filas[i].pct > 0.0);

        printf("%s%-8d %-26.26s %-12s %12.1f %10ld%s\n",
               destacar ? "\033[1m" : "",
               (int)filas[i].pid, filas[i].cmd, filas[i].estado,
               filas[i].pct, filas[i].rss,
               destacar ? "\033[0m" : "");
    }

    if (n == 0)
        printf("\n(no hay procesos en background)\n");

    fflush(stdout);
}

int pmon_run(int seconds)
{
    struct sigaction sa, old_alrm, old_int;
    sigset_t         bloquear, vacia, antes_mask;
    struct timespec  t_antes, t_ahora;

    memset(previo, 0, sizeof(previo));

    //manejadores temporales. guardamos los de la shell para reponerlos al salir
    memset(&sa, 0, sizeof(sa));
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sa.sa_handler = on_alarm;
    sigaction(SIGALRM, &sa, &old_alrm);

    sa.sa_handler = on_int;
    sigaction(SIGINT, &sa, &old_int);

    //las bloqueamos y esperamos con sigsuspend, que desbloquea y espera 
    sigemptyset(&bloquear);
    sigaddset(&bloquear, SIGALRM);
    sigaddset(&bloquear, SIGINT);
    sigprocmask(SIG_BLOCK, &bloquear, &antes_mask);

    sigemptyset(&vacia);

    tick  = 0;
    salir = 0;

    clock_gettime(CLOCK_MONOTONIC, &t_antes);

    while (!salir) {

        double transcurrido;

        clock_gettime(CLOCK_MONOTONIC, &t_ahora);
        transcurrido = (t_ahora.tv_sec  - t_antes.tv_sec) +
                       (t_ahora.tv_nsec - t_antes.tv_nsec) / 1e9;
        t_antes = t_ahora;

        dibujar(seconds, transcurrido);

        alarm(seconds);

        //SIGCHLD tambien despierta a sigsuspend, se revisa en ciclo
        while (!tick && !salir)
            sigsuspend(&vacia);

        tick = 0;
    }

    alarm(0);   //cancelamos la alarma que quedo pendiente

    sigprocmask(SIG_SETMASK, &antes_mask, NULL);
    sigaction(SIGALRM, &old_alrm, NULL);
    sigaction(SIGINT,  &old_int,  NULL);

    printf("\n");
    return 0;
}
