/*
	Para ejecutar debe ingresar: 
	./bin/kernel_memory kernel_memory.config
*/

#include "gestion.h"

#define CANTIDAD_DE_ARGUMENTOS 2/*1 por el ejecutable, 1 por la ruta al config*/
#define ARCHIVO_CONFIG 1

t_log * logger = NULL;
t_config * config = NULL;
int * recurso_swap = NULL;
int * recurso_kernel_scheduler = NULL;
t_list * recursos_memory_stick = NULL;
t_list * recursos_cpu = NULL;
t_list * pid_instrucciones = NULL;

int main(int argc, char* argv[]) 
{
	int * socket_escucha = (int *) malloc (sizeof(int));
	char * log_level;
	char * puerto_escucha;
	pthread_t hilo_servidor;
	
	if (argc != CANTIDAD_DE_ARGUMENTOS)
	{
		fprintf (stderr, "%s\n", "Error: necesita agregar 1 argumentos junto al ejecutable");
		return EXIT_FAILURE;
	}
	
	/*Estos son los recursos utilizados por este módulo para realizar su trabajo*/
	recursos_memory_stick = list_create ();
	recursos_cpu = list_create ();
	
	pid_instrucciones = list_create ();
		
	/*Crea el config a partir de la ruta pasada como Argumento a traves de la Linea de comandos*/ 
	config = iniciar_config ( argv[ARCHIVO_CONFIG] );
	
	log_level = config_get_string_value (config, "LOG_LEVEL");
	logger= iniciar_log (	"kernel_memory.log", 
												"KERNEL_MEMORY", 
												log_level_from_string ( log_level ) );
												
	/************************* LOG -01 Adicional ***********************/
	log_info ( logger, "@@ Inicio del Módulo Kernel Memory");
   
  puerto_escucha = config_get_string_value (config, "PUERTO_ESCUCHA");
  *socket_escucha = crear_socket ( SERVIDOR, NULL, puerto_escucha);
  
	pthread_create (&hilo_servidor, NULL, admitir_clientes, (void *)socket_escucha);
	pthread_join (hilo_servidor, NULL);
		
	return EXIT_SUCCESS;
}
