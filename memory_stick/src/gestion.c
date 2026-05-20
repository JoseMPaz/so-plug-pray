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
	t_recurso * recurso;
	char * id ;
	int socket = *(int*) socket_de_atencion;
	free(socket_de_atencion);  // liberar memoria
	printf ("%d\n", socket);//sera eliminado
	t_list * parametros_new_cpu = NULL;
	
	printf("Antes de recibir_operacion\n");
	int operacion = recibir_operacion(socket);
	printf("Operacion: %d\n", operacion);
    
	switch (operacion) //Selecciona el tipo de operacion
  {
		case NEW_CPU://Se encolar el socket cpu en la lista de cpus
			parametros_new_cpu = recibir_carga_util (socket); //Recibe identificador de cpu
			if (parametros_new_cpu == NULL || list_size(parametros_new_cpu) == 0) 
			{
    		log_error(logger, "Error: parametros_new_cpu inválido");
    		return NULL;
			}
			recurso = (t_recurso *) malloc (sizeof(t_recurso));

			id = list_get(parametros_new_cpu, 0);
			recurso->id = strdup(id);
			recurso->socket = socket;
			recurso->disponible = true;
			
			list_add ( recursos_cpu, (void *) recurso );//Se le debe agregar mutex
			
			log_info ( logger, "NUEVO CPU CONECTADO A KERNEL_SCHEDULER");
			list_destroy(parametros_new_cpu);
			break;
		case OPERACION_DESCONOCIDA:
			fprintf (stderr, "Operación no registrada");
		default:
			; /*operación desconocida*/
	}		          			

	return NULL;
}
