#ifndef THREAD_EJECUCION_H_
#define THREAD_EJECUCION_H_

#include <utils/hello.h> 

void* thread_ejecucion(void* arg);
void ejecutar_ciclo_de_instruccion(t_pcb* pcb, int delay);

#endif /* THREAD_EJECUCION_H_ */