#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv) {
    if (strcmp(argv[1], "network 1") == 0) {
        system("sudo nmcli d wifi connect pioneers password pieisgreat");

    } else if (strcmp(argv[1], "network 2") == 0) {
        char* router_name = malloc(sizeof(char) * 32);
        char* router_password = malloc(sizeof(char) * 32);
        char* total_command = malloc(sizeof(char) * 128);
        FILE* team_router = fopen("../dev_handler/teamrouter.txt", "r");
        char curr_char;
        int curr_spot = 0;

        while ((curr_char = fgetc(team_router)) != '\n') {  // This while loop reads router name from teamrouter.txt
            router_name[curr_spot] = curr_char;
            curr_spot++;
        }
        router_name[curr_spot] = '\0';

        curr_spot = 0;
        for (int i = 0; i < 8; i++) {
            curr_char = fgetc(team_router);
            router_password[curr_spot] = curr_char;
            curr_spot++;
        }
        router_password[curr_spot] = '\0';

        sprintf(total_command, "sudo nmcli d wifi connect %s password %s", router_name, router_password);
        system(total_command);

        fclose(team_router);
        free(router_name);
        free(router_password);
        free(total_command);
    }
    return 0;
}
