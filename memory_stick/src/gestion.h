#ifndef _GESTION_H_
#define _GESTION_H_

#include <utils/hello.h>

#define ARCHIVO_CONFIGURACION 1
#define TAMANO_STICK 2

extern t_list * recursos_cpu;
extern char * memory_stick;
extern t_log * logger;

void * admitir_clientes (void * socket_escucha);
void * admitir (void * socket_de_atencion);

#endif
