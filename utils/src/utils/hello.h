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
#include <commons/collections/list.h>
#include <pthread.h>
#include <commons/string.h>
#include <signal.h>
#include <semaphore.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <stdint.h>





#define wait(semaforo) sem_wait(semaforo)
#define signal(semaforo) sem_post(semaforo)

typedef void * (*t_scheduler)(void * arg);

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
	
	IO_SLEEP = 5,
	IO_STDIN = 6,
	IO_STDOUT = 7,

	IO_FINISHED = 8,
	EXECUTE_PROCESS = 9,

	//CPU <-> MEMORY
	MOV_IN = 10,
	MOV_OUT = 11,
	RES_OK = 12,
	//SCHEDULER <-> MEMORY 
	ESPACIO_LIBRE = 13,
	R_ESPACIO = 14,

	OPERACION_DESCONOCIDA = 50,
	/* 
	...
	Van las operaciones que maneja cada servidor
	...
	*/
	
	RESPUESTA_IMAGEN_PROCESO=100
}t_operacion;

typedef struct 
{
	char * id;
	int socket;
	bool disponible;
}t_recurso;

typedef struct
{
	// Registros de 8 bits
	uint8_t ax;
	uint8_t bx;
	uint8_t cx;
	uint8_t dx;

	// Registros de 32 bits
	uint32_t eax;
	uint32_t ebx;
	uint32_t ecx;
	uint32_t edx;

	// Registros de direcciones lógicas
	uint32_t si;
	uint32_t di;

}t_registros_cpu;

typedef enum
{
	NEW,
	READY,
	EXEC,
	BLOCK,
	EXIT
}t_estado_proceso;

typedef struct
{
	uint32_t pid;// Identificador único del proceso
	uint32_t prioridad;// Prioridad del proceso
	uint32_t pc;// Program Counter: próxima instrucción a ejecutar
	t_estado_proceso estado;// Estado actual del proceso
	t_registros_cpu contexto;// Contexto de ejecución del proceso
} t_pcb;

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

typedef struct
{
	char * str_pid;
	t_list * instrucciones;
}t_codigo;
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
char * pasar_a_minusculas (char* str);
char * uint32_to_string (uint32_t numero);
t_list * leer_archivo_a_lista (const char *ruta);

#endif
