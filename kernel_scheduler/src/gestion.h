#ifndef _GESTION_H_
#define _GESTION_H_

#include <utils/hello.h>




extern t_list * recursos_io;
extern t_list * recursos_cpu;
extern t_log * logger;
extern t_list * cola_block;
extern pthread_mutex_t mutex_block;
extern t_list * cola_ready;
extern pthread_mutex_t mutex_ready;
extern t_list * cola_new;
extern pthread_mutex_t mutex_new;
extern t_list * cola_exec;
extern pthread_mutex_t mutex_exec;
extern sem_t sem_ready;
extern sem_t sem_new;
extern sem_t sem_new_ready;

extern pthread_mutex_t mutex_cpu;
extern t_list * instrucciones_padre;


extern pthread_mutex_t mutex_io;
extern pthread_mutex_t mutex_cpu;

extern int rr_quantum;
extern pthread_mutex_t mutex_quantum;

void * admitir_clientes (void * socket_escucha);
bool _es_el_pid(void* elemento);
void * admitir (void * socket_de_atencion);
void * fifo (void *);
void * round_robin (void *);
void * colas_multinivel (void *);
t_pcb * crear_pcb (uint32_t pid, uint32_t prioridad);
void * long_term_scheduler (void * arg);
void * medium_term_scheduler (void * arg);
bool esta_disponible (void * elemento);
t_recurso * buscar_cpu_libre(void);
t_list* parsear_instrucciones(char* path_archivo);

#endif
