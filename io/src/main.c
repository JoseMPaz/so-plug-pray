#include <utils/hello.h>

t_config * config = NULL;
t_log * logger = NULL;

int main(int argc, char* argv[]) 
{
	int socket; //Este socket se conecta al servidor kernel scheduler
	char * log_level;
	char * ip_kernel_scheduler, * puerto_kernek_scheduler;
	
	saludar("io");//Se eliminara
    
	config = iniciar_config ("./io.config");
    
	log_level = config_get_string_value (config, "LOG_LEVEL");
	
	logger = iniciar_log ( "io.log", "IO", log_level_from_string ( log_level ) );
	log_info ( logger, "MODULO IO HA INICIADO");
		
	ip_kernel_scheduler = config_get_string_value (config, "IP_KERNEL_SCHEDULER");
	puerto_kernek_scheduler = config_get_string_value (config, "PUERTO_KERNEL_SCHEDULER");
	
	socket = crear_socket ( CLIENTE, ip_kernel_scheduler, puerto_kernek_scheduler);
		
	conectar_a_servidor ( socket, ip_kernel_scheduler, puerto_kernek_scheduler);
	t_paquete * paquete = crear_paquete (NEW_IO);
	agregar_a_paquete (paquete, "STDIN", strlen("STDIN"));
	enviar_paquete (paquete, socket);
	
	while(1)
{
	t_operacion operacion = recibir_operacion(socket);

	switch(operacion)
	{
		case IO_SLEEP:
			t_list* parametros = recibir_carga_util(socket);

			char* pid = list_get(parametros, 0);
			char* tiempo = list_get(parametros, 1);

			log_info(logger,
				"Recibi IO_SLEEP - PID: %s - Tiempo: %s",
				pid,
				tiempo
			);

			int tiempo_sleep = atoi(tiempo);

			log_info(logger,
				"Ejecutando IO_SLEEP durante %d segundos",
				tiempo_sleep
			);

			sleep(tiempo_sleep);

			t_paquete* paquete_fin = crear_paquete(IO_FINISHED);

			agregar_a_paquete(paquete_fin, pid, strlen(pid) + 1);

			enviar_paquete(paquete_fin, socket);

			destruir_paquete(paquete_fin);

			log_info(logger,
				"IO finalizada para PID: %s",
				pid
			);

			list_destroy_and_destroy_elements(parametros, free);

			break;

		case IO_STDIN:
			{
				t_list* parametros = recibir_carga_util(socket);

				char* pid = list_get(parametros, 0);

				log_info(logger,
					"IO_STDIN solicitado para PID: %s",
					pid
				);

				char buffer[256];

				printf("PID %s - Ingrese texto: ", pid);

				fgets(buffer, sizeof(buffer), stdin);

				buffer[strcspn(buffer, "\n")] = '\0';

				log_info(logger,
					"Texto recibido para PID %s: %s",
					pid,
					buffer
				);

				t_paquete* paquete_fin = crear_paquete(IO_FINISHED);

				agregar_a_paquete(
					paquete_fin,
					pid,
					strlen(pid) + 1
				);

				enviar_paquete(paquete_fin, socket);

				destruir_paquete(paquete_fin);

				list_destroy_and_destroy_elements(
					parametros,
					free
				);

				break;
			}

		case IO_STDOUT:
			{
				t_list* parametros = recibir_carga_util(socket);

				char* pid = list_get(parametros, 0);

				char* mensaje = list_get(parametros, 1);

				log_info(logger,
					"IO_STDOUT PID %s: %s",
					pid,
					mensaje
				);

				t_paquete* paquete_fin = crear_paquete(IO_FINISHED);

				agregar_a_paquete(
					paquete_fin,
					pid,
					strlen(pid) + 1
				);

				enviar_paquete(paquete_fin, socket);

				destruir_paquete(paquete_fin);

				list_destroy_and_destroy_elements(
					parametros,
					free
				);

				break;
			}

		default:
			log_warning(logger, "Operacion desconocida");
			break;
	}
}
	
	
	return 0;
}
