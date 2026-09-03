//monitor de procesos
#ifndef PMON_H
#define PMON_H

//intervalo de refresco por defecto en segundos   
#define PMON_DEFAULT_SECS 2

//redibuja la tabla de jobs en background cada seconds segundos hasta que el usuario presione Ctrl+C 
int pmon_run(int seconds);

#endif 
