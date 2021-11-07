#include "tcp.h"

void handle_command(int commandId, char * args, char * response) {
    if (commandId == 0) {
        int a, b;
        sscanf(args, "%d %d", &a, &b);
        printf("arguments: a=%d b=%d\n", a, b);
        strcpy(response, "Command 0, sensor data");
    }
    else if (commandId == 1) {
        strcpy(response, "Command 1, actuator data");
    }
}

int main(void) {
    int o, cmd;
    char buffer[MAX_BUFFER_SIZE];

    printf("Choose:\n 1- Server\t2- Client\n");
    scanf("%d", &o);

    if(o == 1){
        // server
        accept_tcp_connections(handle_command);

        // non blocking version:
        /*
        pthread_t accept_thread;
        accept_tcp_connections_non_blocking(handle_command, &accept_thread);
        printf("NON BLOCKING! :D\n");
        while (1);
         */
    } else {
        // client
        ClientThreadData data;
        connect_to_tcp_server("127.0.0.1", &data);
        while (1) {
            char arguments[MAX_ARGS_BUFFER_SIZE];

            printf("Command to send:");
            scanf("%d", &cmd);
            printf("arguments: ");
            sprintf(arguments, "1 2");
            send_command_to_server(cmd, arguments, buffer, &data);

            // exit
            if(cmd == -1)
                break;

            printf("Server response: %s\n", buffer);
        }
        pthread_join(data.interact_server_thread, NULL);
    }

    return 0;
}