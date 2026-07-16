/* 
Ejecucion del modulo con Argumentos en la Linea de Comandos
	./bin/kernel_scheduler <ruta archivo configuracion> <nombre archivo pseudocodigo inicial>
Ejemplo:
	./bin/kernel_scheduler kernel_scheduler.config padre.prc
	argv[0] = "./bin/kernel_scheduler"
	argv[1] = "kernel_scheduler.config"
	argv[2] = "padre.prc"
*/

/* Cantidad de LOG Obligatorios = 2*/
/* Cantidad de LOG Adicionales = 2*/

#include "gestion.h"
#define RUTA_CONFIGURACION 1
#define ARCHIVO_PSEUDOCODIGO 2

t_config * config = NULL;
t_log * logger = NULL;

t_list * recursos_io = NULL;
t_list * recursos_cpu = NULL;
pthread_mutex_t mutex_io;
pthread_mutex_t mutex_cpu;


t_list * cola_new = NULL;
pthread_mutex_t mutex_new;
t_list * cola_ready = NULL;
pthread_mutex_t mutex_ready;
t_list * cola_exec = NULL;
pthread_mutex_t mutex_exec;
t_list * cola_block = NULL;
pthread_mutex_t mutex_block;

pthread_mutex_t mutex_pid;
uint32_t pid = 0;

t_list * instrucciones_padre = NULL;

// Variables para Round Robin
int rr_quantum = 0;
pthread_mutex_t mutex_quantum;

sem_t sem_ready;
sem_t sem_new;
sem_t sem_new_ready;
int * recurso_kernel_memory = NULL;


int main(int argc, char* argv[]) 
{
	int * socket_escucha = (int *) malloc (sizeof(int));
	char * log_level;
	char  * algoritmo_de_planificacion;
	char * puerto_escucha;
	char * puerto_kernel_memory, * ip_kernel_memory;
	pthread_t hilo_servidor;
	t_scheduler short_term_scheduler;//t_scheduler esta definido en hello.h
	int pid_padre;
	pthread_t hilo_planificador_corto_plazo;
	pthread_t hilo_planificador_largo_plazo;
	
	if (argc != 1/*Por el ejecutable*/ + 2/*Por argumentos en linea de comando*/)
	{
		fprintf (	stderr, 
					"%s\n%s\n", 
					"Error: debe ingresar 2 argumentos", 
					"Ejemplo: ./bin/kernel_scheduler kernel_scheduler.config padre");
		return EXIT_FAILURE;
	}
	else
	{
		FILE * pf;	
		if ( (pf = fopen (argv[RUTA_CONFIGURACION] , "r")) == NULL)
		{
			fprintf (	stderr, 
						"%s\n", 
						"Error: Ruta archivo de configuracion no se pudo abrir");
			return EXIT_FAILURE;
		}		
		fclose (pf);
	}
	
	/*Se crean la listas de recursos*/
	recursos_io = list_create();
	recursos_cpu = list_create ();
	/*Se crean las listas de procesos*/
	cola_new = list_create ();
	cola_ready = list_create();
	cola_exec = list_create();
	cola_block = list_create();
	

	pthread_mutex_init (&mutex_new, NULL);
	pthread_mutex_init (&mutex_ready, NULL);
	pthread_mutex_init (&mutex_exec, NULL);
	pthread_mutex_init (&mutex_block, NULL);
	pthread_mutex_init (&mutex_io, NULL);
	pthread_mutex_init (&mutex_cpu, NULL);
	pthread_mutex_init (&mutex_pid, NULL);
		
	sem_init (	&sem_ready, 
				0/*semaforo compartido entre hilos de este modulo*/, 
				0/*valor inicial del semaforo*/	);
	sem_init (	&sem_new, 
				0/*semaforo compartido entre hilos de este modulo*/, 
				0/*valor inicial del semaforo*/	);
	sem_init (	&sem_new_ready, 
				0/*semaforo compartido entre hilos de este modulo*/, 
				0/*valor inicial del semaforo*/	);
	pthread_mutex_init(&mutex_quantum, NULL);
	
	recurso_kernel_memory = (int *) malloc (sizeof(int));
	
	/*Obtiene la informacion del archivo de configuracion*/    
	config = iniciar_config (argv[RUTA_CONFIGURACION]);
	
	/*carga la informacion de config en variables*/
	log_level = config_get_string_value (config, "LOG_LEVEL");
	algoritmo_de_planificacion = config_get_string_value(config, "PLANIFICATION_ALGORITHM");
	rr_quantum = config_get_int_value(config, "RR_QUANTUM");
	/*	QUEUES_ALGORITHMS=[FIFO,RR,RR,FIFO,RR,FIFO] en caso de ser CMN
		QUEUE_PREEMPTION=TRUE en caso de ser CMN
		SUSPENSION_TIMEOUT=35000 en caso de ser CMN	*/
	puerto_escucha = config_get_string_value (config, "PUERTO_ESCUCHA");
	ip_kernel_memory = config_get_string_value (config, "IP_KERNEL_MEMORY");
	puerto_kernel_memory = config_get_string_value (config, "PUERTO_KERNEL_MEMORY");
	
	/*	Crea el log en el nivel indicado en el archivo de configuracion*/
	logger = iniciar_log (	"kernel_scheduler.log", "KERNEL_SCHEDULER", log_level_from_string ( log_level ) );
	
	/********************** LOG 15 ADICIONAL **********************/
	log_info ( logger, "## El modulo kernel scheduler ha iniciado");
	
	/*	scheduler es un puntero a funcion que dependiendo el algoritmo de planificacion definido en el archivo de configuracion
	asigna una funcion a este puntero a funcion, que posteriormente es ejecutado en su respectivo hilo.	*/
	if (!strcmp (algoritmo_de_planificacion, "FIFO") )
		short_term_scheduler = fifo;
	else if (!strcmp (algoritmo_de_planificacion, "RR"))
		short_term_scheduler = round_robin;
	else if (!strcmp (algoritmo_de_planificacion, "CMN"))
		short_term_scheduler =colas_multinivel;
	else
	{
		fprintf (stderr, "%s\n", "Error: Algoritmo de planificación no definido");
		return EXIT_FAILURE;
	}
	/********************** LOG 16 ADICIONAL **********************/
	log_info (logger, "##Algoritmo de planificacion: %s\n", algoritmo_de_planificacion);
	
	/* Saludo a Kernel Memory */	
  	*recurso_kernel_memory = crear_socket ( CLIENTE, ip_kernel_memory, puerto_kernel_memory);
  	conectar_a_servidor ( *recurso_kernel_memory, ip_kernel_memory, puerto_kernel_memory);
  	t_paquete * paquete = crear_paquete (NEW_KERNEL_SCHEDULER);
	enviar_paquete (paquete, *recurso_kernel_memory);
  	
  	/*Crea el PCB del proceso inicial*/
	
	// Cargar pseudocódigo del archivo
	char archivo_pseudocodigo[256];
	sprintf(archivo_pseudocodigo, "./pseudocodigos/%s", argv[ARCHIVO_PSEUDOCODIGO]);
	instrucciones_padre = parsear_instrucciones(archivo_pseudocodigo);
	log_info(logger, "## Se cargaron %d instrucciones del archivo %s", list_size(instrucciones_padre), argv[ARCHIVO_PSEUDOCODIGO]);
	
	// Enviar instrucciones a kernel_memory
	t_paquete * paq_instrucciones = crear_paquete(EXECUTE_PROCESS);
	agregar_a_paquete(paq_instrucciones, argv[ARCHIVO_PSEUDOCODIGO], strlen(argv[ARCHIVO_PSEUDOCODIGO]) + 1);
	for (int i = 0; i < list_size(instrucciones_padre); i++) {
		char* instr = (char*) list_get(instrucciones_padre, i);
		agregar_a_paquete(paq_instrucciones, instr, strlen(instr) + 1);
	}
	enviar_paquete(paq_instrucciones, *recurso_kernel_memory);
	destruir_paquete(paq_instrucciones);
	
	pthread_mutex_lock(&mutex_pid);
  		pid_padre = pid++;
  	pthread_mutex_unlock(&mutex_pid);
  	t_pcb * nuevo_pcb = NULL;
  	nuevo_pcb = crear_pcb (pid_padre, 0/*prioridad*/);
  	pthread_mutex_lock(&mutex_new);
  		list_add (cola_new, nuevo_pcb);
  	pthread_mutex_unlock(&mutex_new);
  	/********************** LOG 03 Obligatorio **********************/
  	log_info (logger, "## (%u) Se crea el proceso - Estado: NEW", nuevo_pcb->pid);
  	
  	signal (&sem_new);//Habilita al planificador de largo plazo a transicionar de new a ready
  	
  	
  	
  	
  	/*Inicia servidor multihilos para admitir IO y CPU*/
  	*socket_escucha = crear_socket ( SERVIDOR, NULL, puerto_escucha);
	pthread_create (&hilo_servidor, NULL, admitir_clientes, (void *)socket_escucha);
	
	/*Inicia el planificador de largo plazo*/
	pthread_create ( &hilo_planificador_largo_plazo, NULL, long_term_scheduler, NULL );
	
	/*Inicia el planificador de corto plazo*/
	pthread_create ( &hilo_planificador_corto_plazo,	NULL, short_term_scheduler, NULL );
	
	
	pthread_join (hilo_servidor, NULL);
	pthread_join (hilo_planificador_corto_plazo, NULL);
	
	if (nuevo_pcb != NULL)
		free (nuevo_pcb);
						
	return 0;
}


