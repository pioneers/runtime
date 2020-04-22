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
#include <stdio_ext.h> // For size_t __fbufsize(FILE* file_pointer)
#include "message.h"

// Import devices.json
// Reference: https://progur.com/2018/12/how-to-parse-json-in-c.html
// json-C Reference: http://json-c.github.io/json-c/json-c-0.13/doc/html/json__object_8h.html
int import_json() {
    // Read devices.json into a buffer
    FILE* fp = fopen("test.json", "r");
    if (fp == NULL) {
        return -1;
    }
    fseek(fp, 0L, SEEK_END);    //seek the end of the file
    int sz = ftell(fp);         //ask for position of file
    rewind(fp);                 //reset file pointer back to beginning
    char buffer[sz];
    fread(buffer, sz, 1, fp);
    fclose(fp);

    struct json_tokener* token = json_tokener_new();
    struct json_object* parsed_json = json_tokener_parse_ex(token, buffer, sz);

    if (parsed_json == NULL) {
        perror("Couldn't parse json file\n");
    }
    enum json_tokener_error error = json_tokener_get_error(token);
    const char* error_str = json_tokener_error_desc(error);
    printf("Tokener Error: %s\n", error_str);

    enum json_type type = json_object_get_type(parsed_json);

    /*
    struct json_object* first_device = json_object_array_get_idx(parsed_json, 0);
    if (first_device == NULL) {
        perror("Couldn't get first device from file.\n");
    }
    struct json_object** dev_name;
    if (json_object_object_get_ex(first_device, "name", dev_name)) { // Reads the "name" key of the first_device into dev_name
        perror("Unable to find key in first_device.\n");
    }
    printf("First Device name: %s\n", json_object_get_string(*dev_name));
    */
    return 0;
}

int main(void) {
    import_json();

}
