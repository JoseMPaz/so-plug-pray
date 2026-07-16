/*
	Para ejecutar debe ingresar: 
	./bin/kernel_scheduler kernel_scheduler.config padre
*/

#include "gestion.h"

#define CANTIDAD_DE_ARGUMENTOS 3/*1 por el ejecutable, 1 por la ruta al config, 1 ruta proceso inicial*/
#define ARCHIVO_CONFIG 1
#define PROCESO_INICIAL 2

t_config * config = NULL;
t_log * logger = NULL;
t_list * recursos_io = NULL;
t_list * recursos_cpu = NULL;
int * recurso_kernel_memory = NULL;
pthread_mutex_t mutex_recursos_io;
pthread_mutex_t mutex_recursos_cpu;
uint32_t pid = 0;


int main(int argc, char* argv[]) 
{
	int * socket_escucha = (int *) malloc (sizeof(int));
	char * log_level;
	char * puerto_escucha;
	char * puerto_kernel_memory, * ip_kernel_memory;
	pthread_t hilo_servidor;
	
	if (argc != CANTIDAD_DE_ARGUMENTOS)
	{
		fprintf (stderr, "%s\n", "Error: necesita agregar 2 argumentos junto al ejecutable");
		return EXIT_FAILURE;
	}
	
	/*Estos son los recursos utilizados por este módulo para realizar su trabajo*/
	recursos_io 					= list_create();
	recursos_cpu 					= list_create ();
	recurso_kernel_memory = (int *) malloc (sizeof(int));

	/*Crea el config a partir de la ruta pasada como Argumento a traves de la Linea de comandos*/ 
	config = iniciar_config ( argv[ARCHIVO_CONFIG] );
	
	log_level = config_get_string_value (config, "LOG_LEVEL");
	logger 		= iniciar_log (	"kernel_scheduler.log", 
													"KERNEL_SCHEDULER", 
													log_level_from_string ( log_level ) );
	
	/************************* LOG -01 Adicional ***********************/
	log_info ( logger, "@@ Inicio del Módulo Kernel Scheduler");
	
	/* Inicio Cliente */
	puerto_kernel_memory = config_get_string_value (config, "PUERTO_KERNEL_MEMORY");
	ip_kernel_memory = config_get_string_value (config, "IP_KERNEL_MEMORY");
  *recurso_kernel_memory = crear_socket ( CLIENTE, 
  																				ip_kernel_memory, 
  																				puerto_kernel_memory);
  																				
	/*Kernel Scheduler se conecta a Kernel Memory*/
  conectar_a_servidor ( *recurso_kernel_memory, ip_kernel_memory, puerto_kernel_memory);
  /************************* LOG 01 Obligatorio ***********************/
	log_info ( logger, "## Conectado a Kernel Memory");
  
  /*HandShake hacia Kernel Memory*/
  t_paquete * paquete = crear_paquete (INICIAR_PROCESO);//Operacion
  char * proceso_inicial = argv[PROCESO_INICIAL]; 
  agregar_a_paquete (paquete, proceso_inicial, strlen(proceso_inicial));//primer parametro
  char * str_pid = (char *) malloc (sizeof (char) * TAMANO_STRING);
  snprintf (str_pid, TAMANO_STRING, "%u", pid++);
  agregar_a_paquete (paquete, str_pid, strlen(str_pid));//segundo parametro
	enviar_paquete (paquete, *recurso_kernel_memory);
	free (str_pid);
	//crear nuevo pcb
		
	/*Crea un servidor multihilos para admitir conexiones de IOs y CPUs*/
  puerto_escucha = config_get_string_value (config, "PUERTO_ESCUCHA");
  *socket_escucha = crear_socket ( SERVIDOR, NULL, puerto_escucha);
  pthread_mutex_init(&mutex_recursos_io, NULL);
	pthread_mutex_init(&mutex_recursos_cpu, NULL);
	if(pthread_create(&hilo_servidor, NULL, admitir_clientes, (void *)socket_escucha) != 0)
	{
		perror("pthread_create");
		free(socket_escucha);
		return EXIT_FAILURE;
	}
	pthread_join(hilo_servidor, NULL);
								
	return EXIT_SUCCESS;
}


