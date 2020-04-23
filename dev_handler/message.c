/*
Constructs, encodes, and decodes the appropriate messages asked for by dev_handler.c
Previously hibikeMessage.py

Linux: json-c:      sudo apt-get install -y libjson-c-dev
       compile:     gcc -I/usr/include/json-c -L/usr/lib message.c -o message -ljson-c
       // Note that -ljson-c comes AFTER source files: https://stackoverflow.com/questions/31041272/undefined-reference-error-while-using-json-c
Mac:
       compile:     gcc -I/usr/local/include/json-c -L/usr/local/lib/ message.c -o message -ljson-c


*/
#include <stdio.h>
//#include <stdio_ext.h>
#include "message.h"

// Import devices.json
// Reference: https://progur.com/2018/12/how-to-parse-json-in-c.html
// json-C Reference: http://json-c.github.io/json-c/json-c-0.13/doc/html/json__object_8h.html
int import_json() {
    // Read devices.json into a buffer
    FILE* fp = fopen("devices.json", "r");
    if (fp == NULL) {
        return -1;
    }
    // Calculate the length of the file and read it into a buffer
    fseek(fp, 0L, SEEK_END);    //seek the end of the file
    int sz = ftell(fp);         //ask for position of file
    rewind(fp);                 //reset file pointer back to beginning
    char buffer[sz];
    fread(buffer, sz, 1, fp);
    fclose(fp);

    // Parse the json file into parsed_json
    struct json_tokener* token = json_tokener_new();
    struct json_object* parsed_json = json_tokener_parse_ex(token, buffer, sz);

    if (parsed_json == NULL) {
        perror("Couldn't parse json file\n");
    }

    // parsed_json is an array. Each element is a device.
    int num_devs = json_object_array_length(parsed_json);
    for (int i = 0; i < num_devs-1; i++) { // Loop through each device and add it to devs (ignore ExampleDevice)
      struct json_object* dev_i = json_object_array_get_idx(parsed_json, i);
      if (dev_i == NULL) {
          perror("Couldn't get device from file.\n");
      }

      struct json_object* dev_type;
      int dev_type_int;
      struct json_object* dev_name;
      char* dev_name_string;
      struct json_object* dev_params;

      if (!json_object_object_get_ex(dev_i, "type", &dev_type)) { // Reads the "type" key of the first_device into dev_type
          perror("Unable to find type.\n");
      }
      dev_type_int = json_object_get_int(dev_type);

      if (!json_object_object_get_ex(dev_i, "name", &dev_name)) { // Reads the "name" key of the first_device into dev_name
          perror("Unable to find name.\n");
      }
      dev_name_string = json_object_get_string(dev_name);

      // Print stuff to double check
      printf("\nDevice type: %d\n", dev_type_int);
      printf("Device name: %s\n", dev_name_string);

      // Get the array of params
      if (!json_object_object_get_ex(dev_i, "params", &dev_params)) { // Reads the "params" key of the first_device into dev_params
          perror("Unable to find params.\n");
      }
      int num_params = json_object_array_length(dev_params);
      // Initialize an array of params to put in device struct
      param dev_params_struct_array[num_params];

      // Get the j-th parameter
      for (int j = 0; j < num_params; j++) {
        struct json_object* param_j = json_object_array_get_idx(dev_params, j);
        if (!param_j) {
            perror("Unable to find param.\n");
        }

        // Get the info of param_j
        struct json_object* param_num;
        struct json_object* param_name;
        struct json_object* param_type;
        struct json_object* param_read;
        struct json_object* param_write;
        if (!json_object_object_get_ex(param_j, "number", &param_num)) {
          perror("Unable to find param_num\n");
        } else if (!json_object_object_get_ex(param_j, "name", &param_name)) {
          perror("Unable to find param_name\n");
        } else if (!json_object_object_get_ex(param_j, "type", &param_type)) {
          perror("Unable to find param_type\n");
        } else if (!json_object_object_get_ex(param_j, "read", &param_read)) {
          perror("Unable to find param_read\n");
        } else if (!json_object_object_get_ex(param_j, "write", &param_write)) {
          perror("Unable to find param_write\n");
        }

        // Store the param info into a struct
        param parameter = {
          .number = json_object_get_int(param_num),
          .name = json_object_get_string(param_name),
          .type = json_object_get_string(param_type),
          .read = json_object_get_int(param_read),
          .write = json_object_get_int(param_write)
        };
        dev_params_struct_array[j] = parameter;
      }

      dev device = {
        .type = dev_type_int,
        .name = dev_name_string,
        .params = dev_params_struct_array,
        .num_params = num_params
      };
      // Store the device into our array of devices (see message.h)
      devs[device.type] = device;
    }

    return 0;
}

int main(void) {
    import_json();
    dev test_dev = devs[12];
    printf("Type: %d\n", test_dev.type);
    printf("Name: %s\n", test_dev.name);
    for (int i = 0; i < test_dev.num_params; i++) {
      printf("Param %d: %d\n", i, test_dev.params[i].read);
    }
}
