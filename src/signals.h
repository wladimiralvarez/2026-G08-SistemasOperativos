//manejadores de señales
#ifndef SIGNALS_H
#define SIGNALS_H

//configura las señales del proceso de la shell
void signals_setup_shell(void);

//devuelve las señales a su comportamiento por defecto
void signals_reset_child(void);

#endif 
