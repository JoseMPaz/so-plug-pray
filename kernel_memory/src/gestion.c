#include "gestion.h"

void * admitir_clientes (void * socket_escucha)
{
	int socket_temporal;
	int socket = *(int*) socket_escucha;
	free (socket_escucha);  // liberar memoria
	
	while(true)
	{
   	pthread_t hilo_de_atencion; //Por cada solicitud de atencion genera un hilo nuevo  	
    	
   	socket_temporal = accept (socket, NULL, NULL);//Se queda aca esperando peticiones, ya que accept es bloqueante
    	
   	if (socket_temporal >= 0)
   	{
   		//La funcion dentro del hilo_de_atencion debe liberar esta peticion de memoria
   		int * socket_de_atencion = (int *) malloc (sizeof(int)); 
    		
   		*socket_de_atencion = socket_temporal;
    		
   		pthread_create (&hilo_de_atencion, NULL, admitir, (void *) socket_de_atencion);
   		pthread_detach (hilo_de_atencion);
   	}   	
  }
  pthread_exit(NULL);

	return NULL;

}

void * admitir (void * socket_de_atencion)
{
	char * id;
	int socket = *(int*) socket_de_atencion;
	t_recurso * recurso;
	char ruta_pseudocodigo[100] = "";
	
	
	free (socket_de_atencion);  // liberar memoria
	printf ("%d\n", socket);//sera eliminado

	t_list * parametros_new_memory_stick = NULL;
	t_list * parametros_new_cpu = NULL;

	int operacion = recibir_operacion(socket);

    
	switch (operacion) //Selecciona el tipo de operacion
  {
		case NEW_SWAP: //Se encolar el socket swap en la lista de swaps
			if (recurso_swap == NULL)
			{
				recurso_swap = (int *) malloc (sizeof(int));
				*recurso_swap = socket;
				log_info ( logger, "NUEVO SWAP CONECTADO A KERNEL_MEMORY");
			}
			else
			{
				log_info ( logger, "UN SWAP INTENTO CONECTARSE A KERNEL_MEMORY");
				close (socket);
			}
			break;
		case INICIAR_PROCESO: //Se encolar el socket kernel_scheduler en la lista de kernel_scheduler
			t_list * parametros_iniciar_proceso = recibir_carga_util (socket);
			char * nombre_archivo = strdup ( list_get(parametros_iniciar_proceso, 0) ); 
			uint32_t pid = strtol ( list_get(parametros_iniciar_proceso, 1) , NULL, BASE_DIEZ);
			/************************* LOG 04 Obligatorio ***********************/
			log_info ( logger, "## PID: %d - Proceso Creado", pid);
			
			strcpy (ruta_pseudocodigo, config_get_string_value (config, "SCRIPTS_BASEPATH") );
			strcat (ruta_pseudocodigo, "/");
			strcat (ruta_pseudocodigo, nombre_archivo);
			
			printf ("Ruta del archivo de pseudocodigo: ");
			puts (ruta_pseudocodigo);
			
			//FILE * ptr_file = fopen (ruta_pseudocodigo, "r");
			
		
			
			//cargar_instrucciones (pid, ptr_file);
			
			//imprimir_pid_instrucciones ();
			
			
			
			
			free (nombre_archivo);
			//list_destroy_and_destroy_elements (parametros_iniciar_proceso, free);
		/*
			if (recurso_kernel_scheduler == NULL)
			{
				recurso_kernel_scheduler = (int *) malloc (sizeof(int));
				*recurso_kernel_scheduler = socket;
				log_info ( logger, "NUEVO KERNEL_SCHEDULER CONECTADO A KERNEL_MEMORY");
			}
			else
			{
				log_info ( logger, "UN KERNEL_SCHEDULER INTENTO CONECTARSE A KERNEL_MEMORY");
				close (socket);
			}*/
			break;
		case NEW_MEMORY_STICK://Se encolar el socket cpu en la lista de cpus
			parametros_new_memory_stick = recibir_carga_util (socket); //Recibe identificador de cpu
			if (parametros_new_memory_stick == NULL || list_size(parametros_new_memory_stick) == 0) 
			{
    		log_error (logger, "Error: parametros_new_memory_stick inválido");
    		return NULL;
			}
			
			recurso = (t_recurso *) malloc (sizeof(t_recurso));

			id = list_get (parametros_new_memory_stick, 0);
			recurso->id= strdup(id);
			recurso->socket = socket;
			recurso->disponible = true;
			
			list_add ( recursos_memory_stick, (void *) recurso);//Se le debe agregar mutex
			
			log_info ( logger, "NUEVO MEMORY_STICK CONECTADO A KERNEL_MEMORY");
			list_destroy(parametros_new_memory_stick);
			break;
		case NEW_CPU://Se encolar el socket cpu en la lista de cpus
			parametros_new_cpu = recibir_carga_util (socket); //Recibe identificador de cpu
			if (parametros_new_cpu == NULL || list_size(parametros_new_cpu) == 0) 
			{
    		log_error (logger, "Error: parametros_new_memory_stick inválido");
    		return NULL;
			}
			
			recurso = (t_recurso *) malloc (sizeof(t_recurso));

			id = list_get (parametros_new_cpu, 0/*ID del CPU*/);
			recurso->id= strdup(id);
			recurso->socket = socket;
			recurso->disponible = true;
			
			list_add ( recursos_cpu, (void *) recurso);//Se le debe agregar mutex
			
			log_info ( logger, "NUEVO CPU CONECTADO A KERNEL_MEMORY");
			list_destroy(parametros_new_cpu);
			break;
		case OPERACION_DESCONOCIDA:
			fprintf (stderr, "Operación no registrada");
		default:
			; /*operación desconocida*/
	}		          			

	return NULL;
}

void cargar_instrucciones (uint32_t pid, FILE * ptr_file)
{
	if(ptr_file == NULL)
  {
		fprintf (stderr, "%s\n", "Error: No existe el archivo de pseudocodigos");
		return;
	}
	t_pid_instrucciones * nuevo = malloc (sizeof(t_pid_instrucciones));

	nuevo->pid = pid;
	nuevo->instrucciones = list_create();

	char buffer[256];

	while(fgets(buffer, sizeof(buffer), ptr_file) != NULL)
	{
    size_t len = strlen(buffer);

    if(len > 0 && buffer[len - 1] == '\n')
    {
        buffer[len - 1] = '\0';
    }

    char * instruccion = strdup(buffer);

		list_add(nuevo->instrucciones, instruccion);
}	

	list_add(pid_instrucciones, nuevo);
}

void destruir_instruccion(void * elem)
{
	free(elem);
}

void destruir_pid_instrucciones(t_pid_instrucciones * proceso)
{
	list_destroy_and_destroy_elements(
		proceso->instrucciones,
		destruir_instruccion
	);

	free(proceso);
}

void imprimir_instruccion (void * instr)
{
	char * instruccion = (char *) instr;
	printf("   - %s\n", instruccion);
	return;
}
        
void imprimir_proceso(void * elemento)
{
	t_pid_instrucciones * proceso = (t_pid_instrucciones *) elemento;

	printf("PID: %u\n", proceso->pid);

	list_iterate(proceso->instrucciones, imprimir_instruccion);

	printf("\n");
	return;
}
    
void imprimir_pid_instrucciones (void)
{
    list_iterate(pid_instrucciones, imprimir_proceso);
    return;
}
