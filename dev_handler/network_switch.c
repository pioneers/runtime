#include <string.h>
#include <stdlib.h>

int main (int argc, char** argv) {
    if (strcmp(argv[1], "network 1") == 0) {
        system("sudo nmcli d wifi connect pioneers password pieisgreat");
    } else if (strcmp(argv[1], "network 2") == 0) {
        system("sudo nmcli d wifi connect Motherbase password pieisgreat");
    }
    return 0;
}