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
    		
			if(pthread_create(&hilo_de_atencion, NULL, admitir, socket_de_atencion) != 0)
			{
				perror("pthread_create");

				close(*socket_de_atencion);
				free(socket_de_atencion);
			}
			else
			{
				pthread_detach(hilo_de_atencion);
			}
   	}   	
  }


	return NULL;

}

void * admitir (void * socket_de_atencion)
{
	t_recurso * recurso;
	char * id ;
	int socket = *(int*) socket_de_atencion;
	free(socket_de_atencion);  // liberar memoria
	printf ("%d\n", socket);//sera eliminado
	t_list * parametros_new_io = NULL;
	t_list * parametros_new_cpu = NULL;
	
	printf("Antes de recibir_operacion\n");
	int operacion = recibir_operacion(socket);
	printf("Operacion: %d\n", operacion);
    
	switch (operacion) //Selecciona el tipo de operacion
  {
		case NEW_IO: //Se encolar el socket io en la lista de ios
			parametros_new_io = recibir_carga_util (socket); //Recibe tipo de io
			
			recurso = (t_recurso *) malloc (sizeof(t_recurso));
			id = list_get (parametros_new_io, 0/*Posicion de tipo de io*/);
			recurso->id = strdup ( id );
			recurso->socket = socket;
			recurso->disponible = true;
						
			pthread_mutex_lock(&mutex_recursos_io);
				list_add (recursos_io, recurso);
			pthread_mutex_unlock(&mutex_recursos_io);
			
			log_info ( logger, "NUEVO IO CONECTADO A KERNEL_SCHEDULER");
			list_destroy_and_destroy_elements (parametros_new_io, free);
    	
			break;
		case NEW_CPU://Se encolar el socket cpu en la lista de cpus
			parametros_new_cpu = recibir_carga_util (socket); //Recibe identificador de cpu
			if (parametros_new_cpu == NULL || list_size(parametros_new_cpu) == 0) 
			{
    		log_error(logger, "Error: parametros_new_cpu inválido");

    		if(parametros_new_cpu != NULL)
					list_destroy_and_destroy_elements(parametros_new_cpu, free);

				close(socket);

    		return NULL;
			}
			recurso = (t_recurso *) malloc (sizeof(t_recurso));

			id = list_get(parametros_new_cpu, 0);
			recurso->id = strdup(id);
			recurso->socket = socket;
			recurso->disponible = true;
			
			pthread_mutex_lock(&mutex_recursos_cpu);
				list_add (recursos_cpu, recurso);
			pthread_mutex_unlock(&mutex_recursos_cpu);
			
			log_info ( logger, "NUEVO CPU CONECTADO A KERNEL_SCHEDULER");
			list_destroy_and_destroy_elements(parametros_new_cpu, free);
			break;
		case OPERACION_DESCONOCIDA:
			fprintf (stderr, "Operación no registrada");
		default:
			; /*operación desconocida*/
	}		          			

	return NULL;
}
