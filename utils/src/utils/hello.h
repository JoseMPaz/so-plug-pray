#ifndef UTILS_HELLO_H_
#define UTILS_HELLO_H_

#include <stdlib.h>
#include <stdio.h>
#include <commons/log.h>
#include <commons/config.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <stdio.h>
#include <commons/collections/list.h>
#include <pthread.h>
#include <commons/string.h>
#include <signal.h>
#include <semaphore.h>
#include <arpa/inet.h>



typedef enum
{
	CLIENTE = 0,
	SERVIDOR = 1
}t_conexion;

typedef enum
{
	NEW_IO = 0,
	NEW_CPU = 1,
	NEW_SWAP = 2,
	NEW_MEMORY_STICK = 3,
	NEW_KERNEL_SCHEDULER = 4,
	
	OPERACION_DESCONOCIDA = 50
	/* 
	...
	Van las operaciones que maneja cada servidor
	...
	*/
}t_operacion;

typedef struct 
{
	char * id;
	int socket;
	bool disponible;
}t_recurso;

typedef struct
{
	char * identificador;
	int socket;
}t_pcb;

typedef struct
{
	int longitud;
	char * flujo;
}t_carga_util;

typedef struct
{
	t_operacion operacion;
	t_carga_util * carga_util;
}t_paquete;
/**
* @brief Imprime un saludo por consola
* @param quien Módulo desde donde se llama a la función
* @return No devuelve nada
*/
void saludar(char* quien);

t_config * iniciar_config (char * ruta_archivo);
t_log * iniciar_log (char * archivo_log, char * etiqueta_log, t_log_level level);
int crear_socket (t_conexion tipo_conexion, const char * ip, const char * puerto);
void conectar_a_servidor (int socket_cliente, char * ip_servidor, char * puerto_servidor);
t_operacion recibir_operacion (int socket);
t_list * recibir_carga_util (int socket);
t_paquete * crear_paquete (t_operacion operacion);
void destruir_paquete (t_paquete * paquete);
void agregar_a_paquete (t_paquete * paquete, void * cadena, int longitud);
void enviar_paquete (t_paquete * paquete, int socket);
void * serializar_paquete (t_paquete * paquete, int bytes_a_enviar);
#endif
