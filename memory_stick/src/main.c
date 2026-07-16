#include "gestion.h"

t_config * config = NULL;
t_log * logger = NULL;
int * recurso_kernel_memory = NULL;
t_list * recursos_cpu = NULL;
char * memory_stick;

int main(int argc, char* argv[]) 
{
	int * socket_escucha = (int *) malloc (sizeof(int));
	char * log_level;
	char * ip_kernel_memory, * puerto_kernek_memory;
	char * puerto_escucha;
	pthread_t hilo_servidor;
	unsigned int tamano_stick;
	
	/******** Inicio Validación de los Argumentos ingresados por la Linea de Comandos ********/
	if (argc != 1/*Por el ejecutable*/ + 1/*Por argumento archivo configuracion*/ + 1/*Por argmento tamaño stick*/)
	{
		fprintf ( 	stderr, "%s\n%s\n", 
				 	"Error: debe ingresar 2 argumentos por linea de comandos", 
					"Ejemplo: ./bin/memoria memory_stick.config 32");
		return EXIT_FAILURE;
	}
	else
	{
		/*Valida que el archivo de configuración exista*/
		FILE * pf;	
		if ( (pf = fopen (argv[ARCHIVO_CONFIGURACION] , "r")) == NULL)
		{
			fprintf ( stderr, "%s\n", "Error: El archivo de configuracion no se pudo abrir" );
			return EXIT_FAILURE;
		}		
		fclose (pf);
		/*Valida el tamaño del memory_stick*/
		char * ptr_fin_str;
		tamano_stick = (unsigned int) strtol (argv[TAMANO_STICK], &ptr_fin_str, 10);
		if (*ptr_fin_str != '\0')
		{
			fprintf ( stderr, "%s\n", "Error: El tamaño de stick debe ser un valor decimal" );
			return EXIT_FAILURE;
		}
		if (tamano_stick <= 0)
		{
			fprintf ( stderr, "%s\n", "Error: El tamaño de stick debe ser positivo" );
			return EXIT_FAILURE;
		}
	}
	/******** Final Validación de los Argumentos ingresados por la Linea de Comandos ********/
	
	recursos_cpu = list_create ();
	recurso_kernel_memory = (int *) malloc (sizeof(int));
		
	config = iniciar_config ( argv[ARCHIVO_CONFIGURACION] );
	
	log_level = config_get_string_value (config, "LOG_LEVEL");
	logger = iniciar_log ( "memory_stick.log", "MEMORY_STICK", log_level_from_string ( log_level ) );
	log_info ( logger, "MODULO MEMORY_STICK HA INICIADO");
	
	/*Crea el vector del memory stick*/
	memory_stick = (char *) malloc (sizeof(char)*tamano_stick);
	
	/* Inicia Cliente */
	ip_kernel_memory = config_get_string_value (config, "IP_KERNEL_MEMORY");
	puerto_kernek_memory = config_get_string_value (config, "PUERTO_KERNEL_MEMORY");
	*recurso_kernel_memory = crear_socket ( CLIENTE, ip_kernel_memory, puerto_kernek_memory);
	conectar_a_servidor ( *recurso_kernel_memory, ip_kernel_memory, puerto_kernek_memory);
	t_paquete * paquete = crear_paquete (NEW_MEMORY_STICK);
	agregar_a_paquete (paquete, "USB", strlen("USB"));
	enviar_paquete (paquete, *recurso_kernel_memory);
	/* Fin Cliente */
	
	/* Inicio Servidor Multihilos */
  puerto_escucha = config_get_string_value (config, "PUERTO_ESCUCHA");
  *socket_escucha = crear_socket ( SERVIDOR, NULL, puerto_escucha);
	
	pthread_create (&hilo_servidor, NULL, admitir_clientes, (void *)socket_escucha);
	pthread_join (hilo_servidor, NULL);
	/* Fin Servidor Multihilos */
	
	free(memory_stick);
	
	return 0;
}

