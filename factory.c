#include "network/tcp_ip.h"
#include "interfaces/network_commands.h"
#include "interfaces/peripherals_network.h"
#include "utils/host_list.h"

host_node * host_list;
pthread_t server_thread;

int fact_ID = -1;
char cmd_args[MAX_ARGS_BUFFER_SIZE];

ClientThreadData * connect_new_factory(char * args) {
    char ip_address[20];
    int host_id;
    sscanf(args, "%s %d", ip_address, &host_id);
    printf("Connecting to new factory ID=%d at %s\n.", host_id, ip_address);

    ClientThreadData * newFactoryClient;
    connect_to_tcp_server(ip_address, &newFactoryClient);
    push_host(host_list, host_id, newFactoryClient);
    return newFactoryClient;
}

void trigger_alarm(int fact_id_alarm) {
    // TODO
}

void handle_command(int commandId, char * args, char * response) {
    if (commandId == CMD_ANNOUNCE_NEW_FACTORY) {
        ClientThreadData * newFactoryClient = connect_new_factory(args);

        // tell new factory to also connect to our server
        char ip_address[20];
        get_ip_address(ip_address);
        sprintf(cmd_args, "%s %d", ip_address, fact_ID);
        // send CMD_CONNECT_BACK_FACTORY
        send_command_to_server(CMD_CONNECT_BACK_FACTORY, cmd_args, NULL, newFactoryClient);
    } else if (commandId == CMD_CONNECT_BACK_FACTORY) {
        connect_new_factory(args);
    } else if (commandId == CMD_SEND_SENSOR_DATA) {
        int factId;
        double temperature, humidity, pressure;
        sscanf(args, "%d %lf %lf %lf", &factId, &temperature, &humidity, &pressure);
        printf("Sensor data from fact_ID=%d: T=%lf H=%lf P=%lf\n", factId, temperature, humidity, pressure);
    } else if (commandId == CMD_TRIGGER_ALARM) {
        int fact_id_alarm;
        sscanf(args, "%d", &fact_id_alarm);
        printf("[!] Alarm triggered because of factory ID %d\n.", fact_id_alarm);
        trigger_alarm(fact_id_alarm);
    }
}

void broadcast_sensor_data(SensorData sensorData) {
    sprintf(cmd_args, "%d %lf %lf %lf", fact_ID, sensorData.temperature, sensorData.humidity, sensorData.pressure);
    host_node * host = host_list;
    while (host->next != NULL){
        host = host->next;
        send_command_to_server(CMD_SEND_SENSOR_DATA, cmd_args, NULL, host->host);
    }
}

// TODO: replace with production version
void read_sensor_data(SensorData * data) {
    data->pressure = 1.34;
    data->temperature = 20.4;
    data->humidity = 99;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Factory takes exactly 1 command line argument: the dashboard's IP address\n");
        exit(-1);
    }

    // start tcp server
    accept_tcp_connections_non_blocking(handle_command, &server_thread);

    // connect to the dashboard
    const char * dashboardAddr = argv[1];
    printf("Starting factory! Dashboard IP address: %s\n", dashboardAddr);

    ClientThreadData * dashboardClient;
    connect_to_tcp_server(dashboardAddr, &dashboardClient);
    initialize_host_list(&host_list);
    push_host(host_list, 0, dashboardClient);

    char ip_address[20], response[20];
    get_ip_address(ip_address);
    // send init_new_factory command
    send_command_to_server(CMD_INIT_NEW_FACTORY, ip_address, response, dashboardClient);

    // get our new factory ID
    sscanf(response, "%d", &fact_ID);
    printf("Successfully connected to the dashboard. This factory now has ID=%d\n", fact_ID);

    while(1) {
        SensorData sensorData;
        read_sensor_data(&sensorData);
        broadcast_sensor_data(sensorData);
        sleep(5);
    }

    close_all_connections(host_list);
    free_host_list(host_list);

    return 0;
}