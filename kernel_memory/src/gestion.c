#include "gestion.h"

void * admitir_clientes (void * socket_escucha)
{
	int socket_temporal;
	int socket = *(int*) socket_escucha;
	free(socket_escucha);  // liberar memoria
	
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
	free (socket_de_atencion);  // liberar memoria
	printf ("%d\n", socket);//sera eliminado

	t_list * parametros_new_memory_stick = NULL;
	t_list * parametros_new_cpu = NULL;

	int operacion = recibir_operacion(socket);

    
	switch (operacion) //Selecciona el tipo de operacion
  {
		case NEW_SWAP: //Se encolar el socket io en la lista de ios
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
		case NEW_KERNEL_SCHEDULER: //Se encolar el socket io en la lista de ios
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
			}
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
