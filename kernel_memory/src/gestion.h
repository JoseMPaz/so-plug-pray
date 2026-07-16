#ifndef _GESTION_H_
#define _GESTION_H_

#include <utils/hello.h>

extern int * recurso_swap;
extern int * recurso_kernel_scheduler;
extern t_list * recursos_memory_stick;
extern t_list * recursos_cpu;
extern t_log * logger;
extern t_config * config;
extern t_list * pid_instrucciones;

typedef struct
{
	uint32_t pid;
	t_list * instrucciones;
}t_pid_instrucciones;

void * admitir_clientes (void * socket_escucha);
void * admitir (void * socket_de_atencion);

void cargar_instrucciones(uint32_t pid, FILE * ptr_file);
void destruir_pid_instrucciones(t_pid_instrucciones * pid_instrucciones);
void destruir_instruccion(void * elem);
void imprimir_instruccion (void * instr);
void imprimir_proceso (void * elemento);
void imprimir_pid_instrucciones (void);

#endif
