#include "network/tcp.h"
#include "interfaces/network_commands.h"
#include "utils/host_list.h"
#include "utils/sensor_history.h"
#include "network/connection.h"
#include "interfaces/peripherals.h"
#include "utils/sensor_data_utils.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

host_node * host_list;
pthread_t server_thread;

int host_ID = -1;
char cmd_args[MAX_ARGS_BUFFER_SIZE];

void handle_command(int commandId, char * args, char * response, int connfd, char * client_ip) {
    if (commandId == CMD_ANNOUNCE_NEW_HOST) {
        ClientThreadData * factory_client = connect_new_factory(args, host_list);
        // get data file from this factory
        send_command_to_server(CMD_SEND_SENSOR_HISTORY_FILE, NULL, NULL, factory_client);
        receive_sensor_history_file(factory_client);
        // print file to test
        SensorData * data = malloc(100 * sizeof(SensorData));
        int lines = read_sensor_data_from_file(data, 100, factory_client->host_id);
        printf("Read %d data lines\n", lines);
        for (int i = 0; i < lines; ++i) {
            printf("Ts=%ld T=%lf H=%lf P=%lf\n", data[i].time, data[i].temperature, data[i].humidity, data[i].pressure);
        }
        free(data);
    } else if (commandId == CMD_SEND_SENSOR_DATA) {
        int factId;
        SensorData data;
        sensor_data_from_command(args, &factId, &data);
        // TODO
    }
}

int main(int argc, char **argv) {
//    setbuf(stdout, NULL);
    if (argc != 2) {
        printf("ML Module takes exactly 1 command line argument: the dashboard's IP address\n");
        exit(-1);
    }

    // start tcp server
    accept_tcp_connections_non_blocking(handle_command, &server_thread);

    // connect to the dashboard
    const char * dashboardAddr = argv[1];
    printf("Starting ML module! Dashboard IP address: %s\n", dashboardAddr);

    connect_to_dashboard(dashboardAddr, &host_list, &host_ID, 0);

    while(1) {

    }

    close_all_connections(host_list);
    free_host_list(host_list);

    return 0;
}