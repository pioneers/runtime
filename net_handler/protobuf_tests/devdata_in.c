#include <stdio.h>
#include <stdlib.h>
#include "../pbc_gen/device.pb-c.h"
#define MAX_MSG_SIZE 1024

static size_t read_buffer(unsigned max_length, uint8_t* out) {
    size_t cur_len = 0;
    size_t nread;
    while ((nread = fread(out + cur_len, 1, max_length - cur_len, stdin)) != 0) {
        cur_len += nread;
        if (cur_len == max_length) {
            fprintf(stderr, "max message length exceeded\n");
            exit(1);
        }
    }
    return cur_len;
}

int main() {
    DevData* dev_data;

    // Read packed message from standard-input.
    uint8_t buf[MAX_MSG_SIZE];
    size_t msg_len = read_buffer(MAX_MSG_SIZE, buf);

    // Unpack the message using protobuf-c.
    dev_data = dev_data__unpack(NULL, msg_len, buf);
    if (dev_data == NULL) {
        fprintf(stderr, "error unpacking incoming message\n");
        exit(1);
    }

    // display the message's fields.
    printf("Received:\n");
    for (int i = 0; i < dev_data->n_devices; i++) {
        printf("Device No. %d: ", i);
        printf("\ttype = %s, uid = %lu, itype = %d\n", dev_data->devices[i]->name, dev_data->devices[i]->uid, dev_data->devices[i]->type);
        printf("\tParams:\n");
        for (int j = 0; j < dev_data->devices[i]->n_params; j++) {
            printf("\t\tparam \"%s\" has type ", dev_data->devices[i]->params[j]->name);
            switch (dev_data->devices[i]->params[j]->val_case) {
                case (PARAM__VAL_FVAL):
                    printf("FLOAT with value %f\n", dev_data->devices[i]->params[j]->fval);
                    break;
                case (PARAM__VAL_IVAL):
                    printf("INT with value %d\n", dev_data->devices[i]->params[j]->ival);
                    break;
                case (PARAM__VAL_BVAL):
                    printf("BOOL with value %d\n", dev_data->devices[i]->params[j]->bval);
                    break;
                default:
                    printf("UNKNOWN");
                    break;
            }
        }
    }

    // Free the unpacked message
    dev_data__free_unpacked(dev_data, NULL);
    return 0;
}