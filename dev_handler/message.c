/*
Constructs, encodes, and decodes the appropriate messages asked for by dev_handler.c
Previously hibikeMessage.py
*/
#include <stdio.h>
#include <jsonc/json.h> // Import devices.json
#include "message.h"

// Import devices.json
// Reference: https://progur.com/2018/12/how-to-parse-json-in-c.html
// json-C Reference: http://json-c.github.io/json-c/json-c-0.13/doc/html/json__object_8h.html
int import_json(void) {
    // Read devices.json into a buffer
    FILE* fp;
    fp = fopen("devices.json", "r'");
    if (fp = NULL) {
        return -1
    }
    char buffer[__fbufsize(fp)]; // Calculates the size of the file https://stackoverflow.com/questions/28311201/buffer-size-in-file-i-o
    fread(buffer, __fbufsize(fp), 1, fp);
    fclose(fp);

    struct json_object* parsed_json = json_tokener_parse(buffer);
    struct json_object* dev_type;
    struct json_object* dev_name;
    struct json_object* dev_params;

    if (json_object_object_get_ex(parsed_json, "dev_type", &dev_type) == 0 ||
        json_object_object_get_ex(parsed_json, "dev_name", &dev_name) == 0 ||
        json_object_object_get_ex(parsed_json, "dev_params", &dev_params) == 0) {
        perror("Unable to open devices.json");
        exit(5)
    }
    printf("dev_type: %d\n", json_object_get_string(dev_type));
    printf("dev_name: %d\n", json_object_get_string(dev_name));
    printf("dev_params: %d\n", json_object_get_string(dev_params));
}


int main(void) {
    import_json();

}
