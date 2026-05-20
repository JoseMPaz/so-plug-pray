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
	
	getchar ();
	
	
	return 0;
}
