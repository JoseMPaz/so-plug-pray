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

typedef enum
{
	CLIENTE = 0,
	SERVIDOR = 1
}t_conexion;

/**
* @brief Imprime un saludo por consola
* @param quien Módulo desde donde se llama a la función
* @return No devuelve nada
*/
void saludar(char* quien);

int crear_socket (t_conexion tipo_conexion, const char * ip, const char * puerto);

#endif
