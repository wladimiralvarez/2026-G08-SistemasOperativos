//manejo de señales 
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include "signals.h"

static void install(int signum, void (*handler)(int), int flags)
{
    struct sigaction sa;
   
    memset(&sa, 0, sizeof(sa));

    sa.sa_handler = handler;

    sigemptyset(&sa.sa_mask);

    sa.sa_flags = flags;

    if (sigaction(signum, &sa, NULL) == -1)
        perror("mishell: sigaction");
}

void signals_setup_shell(void)
{
    //SA_RESTART reintenta la llamada interrumpida, sin él fgets falla con EINTR
    install(SIGINT,  SIG_IGN, SA_RESTART);
    install(SIGQUIT, SIG_IGN, SA_RESTART);

    //TODO R5: manejador de sigchld aqui
}

void signals_reset_child(void)
{
    install(SIGINT,  SIG_DFL, 0);
    install(SIGQUIT, SIG_DFL, 0);
    install(SIGCHLD, SIG_DFL, 0);
}
