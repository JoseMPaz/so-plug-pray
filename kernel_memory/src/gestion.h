#ifndef _GESTION_H_
#define _GESTION_H_


#include <commons/collections/list.h>
#include <commons/string.h>
#include <stdio.h>
#include <utils/hello.h>

extern int * recurso_swap;
extern int * recurso_kernel_scheduler;
extern t_list * recursos_memory_stick;
extern t_list * recursos_cpu;
extern t_list * instrucciones_proceso;
extern pthread_mutex_t mutex_instrucciones;
extern t_log * logger;

void * admitir_clientes (void * socket_escucha);
void * admitir (void * socket_de_atencion);

t_list* parsear_instrucciones(char* path_archivo);

void atender_cpu(int socket);
void atender_kernel_scheduler(int socket);

#endif
