#ifndef _GESTION_H_
#define _GESTION_H_


#include <commons/collections/list.h>
#include <commons/string.h>
#include <stdio.h>
#include <utils/hello.h>
#include <pthread.h>


typedef struct {
    uint32_t id_segmento;
    uint32_t direccion_base;
    uint32_t limite;
} t_segmento;

typedef struct {
    uint32_t pid;
    t_list* segmentos;
} t_tabla_segmentos;

typedef struct {
    uint32_t direccion_base;
    uint32_t tamano;
} t_hueco_libre;

extern int * recurso_swap;
extern int * socket_kernel_scheduler;
extern t_list * recursos_memory_stick;
extern t_list * recursos_cpu;
extern t_list * instrucciones_proceso;
extern pthread_mutex_t mutex_instrucciones;
extern t_log * logger;
extern t_list * tablas_segmentos_procesos;
extern t_list * lista_huecos_libres;
extern pthread_mutex_t mutex_tablas_segmentos;
extern pthread_mutex_t mutex_memory_sticks;
extern pthread_mutex_t mutex_huecos_libres;
extern char * scripts_basepath;
extern t_list * codigos;


void * admitir_clientes (void * socket_escucha);
void * admitir (void * socket_de_atencion);

t_list* parsear_instrucciones(char* path_archivo);

void atender_cpu(int socket);
void atender_kernel_scheduler(int socket);


void crear_tabla_segmentos_proceso(uint32_t pid);
t_tabla_segmentos* buscar_tabla_segmentos(uint32_t pid);
t_segmento* buscar_segmento_en_proceso(t_tabla_segmentos* tabla, uint32_t id_segmento);
void registrar_nuevo_segmento(uint32_t pid, uint32_t id_segmento, uint32_t base_fisica, uint32_t limite);
void eliminar_segmento_de_proceso(uint32_t pid, uint32_t id_segmento);


#endif
