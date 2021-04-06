#include <stdio.h>

#include "yayp.h"
#include "test.h"


int
main(void) {

    yaml_file_t *file = yaml_open("basic.yml");

    printf("%i\n", yaml_get_bool(file, "likes_vim"));
    printf("%f\n", yaml_get_number(file, "cool1"));
    printf("%s\n", yaml_get_string(file, "name"));

    yaml_close(file);
}