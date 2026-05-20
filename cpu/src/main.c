#include <utils/hello.h>

t_config * config = NULL;
t_log * logger = NULL;

int main(int argc, char* argv[]) 
{
	int socket_kernel_scheduler; //Este socket cliente se conecta al servidor kernel scheduler
	int socket_kernel_memory; //Este socket cliente se conecta al servidor kernel memory
	int socket_memory_stick; //Este socket cliente se conecta al servidor memory stick
	char * log_level;
	char * ip_kernel_scheduler, * puerto_kernek_scheduler;
	char * ip_kernel_memory, * puerto_kernek_memory;
	char * ip_memory_stick, * puerto_memory_stick;
	
	saludar("cpu");//Se eliminara
	
	config = iniciar_config ("./cpu.config");
	
	log_level = config_get_string_value (config, "LOG_LEVEL");
  logger = iniciar_log ( "cpu.log", "CPU", log_level_from_string ( log_level ) );
	log_info ( logger, "MODULO CPU HA INICIADO");
	
	ip_kernel_scheduler = config_get_string_value (config, "IP_KERNEL_SCHEDULER");
	puerto_kernek_scheduler = config_get_string_value (config, "PUERTO_KERNEL_SCHEDULER");
	ip_kernel_memory = config_get_string_value (config, "IP_KERNEL_MEMORY");
	puerto_kernek_memory = config_get_string_value (config, "PUERTO_KERNEL_MEMORY");
	ip_memory_stick = config_get_string_value (config, "IP_MEMORY_STICK");
	puerto_memory_stick = config_get_string_value (config, "PUERTO_MEMORY_STICK");
	
	socket_kernel_scheduler = crear_socket ( CLIENTE, ip_kernel_scheduler, puerto_kernek_scheduler);
	socket_kernel_memory = crear_socket ( CLIENTE, ip_kernel_memory, puerto_kernek_memory);
	socket_memory_stick = crear_socket ( CLIENTE, ip_memory_stick, puerto_memory_stick);
	
	conectar_a_servidor ( socket_kernel_memory, ip_kernel_memory, puerto_kernek_memory);
	t_paquete * paquete_kernel_memory = crear_paquete (NEW_CPU);
	agregar_a_paquete (paquete_kernel_memory, "INTEL", strlen("INTEL"));
	enviar_paquete (paquete_kernel_memory, socket_kernel_memory);
	
	conectar_a_servidor ( socket_kernel_scheduler, ip_kernel_scheduler, puerto_kernek_scheduler);
	t_paquete * paquete_kernel_scheduler = crear_paquete (NEW_CPU);
	agregar_a_paquete (paquete_kernel_scheduler, "INTEL", strlen("INTEL"));
	enviar_paquete (paquete_kernel_scheduler, socket_kernel_scheduler);
	
	conectar_a_servidor ( socket_memory_stick, ip_memory_stick, puerto_memory_stick);
	t_paquete * paquete_memory_stick = crear_paquete (NEW_CPU);
	agregar_a_paquete (paquete_memory_stick, "INTEL", strlen("INTEL"));
	enviar_paquete (paquete_memory_stick, socket_memory_stick);
	
	getchar ();
	
	
	return 0;
}
