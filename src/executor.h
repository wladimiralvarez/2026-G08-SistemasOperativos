// ejecucion de una tuberia parseada
#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "parser.h"

//ejecuta la tuberia pl y retorna el codigo de salida del ultimo comando o 0 si fue a background
int execute_pipeline(pipeline_t *pl);

#endif 
