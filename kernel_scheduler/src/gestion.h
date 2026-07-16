#ifndef _GESTION_H_
#define _GESTION_H_

#include <utils/hello.h>

extern t_list * recursos_io;
extern t_list * recursos_cpu;
extern t_log * logger;
extern pthread_mutex_t mutex_recursos_io;
extern pthread_mutex_t mutex_recursos_cpu;

void * admitir_clientes (void * socket_escucha);
void * admitir (void * socket_de_atencion);

#endif
