#include <stdio.h>

//#define DEBUG
#include "yayp.h"

int
main(void) {
    
    yaml_file_t *file = yaml_open("basic.yml");

    printf("word: %s\n", yaml_get_string(file, "name"));
    printf("word: %s\n", yaml_get_string(file, "job"));
    printf("num: %f\n", yaml_get_number(file, "cool1"));
    printf("num: %f\n", yaml_get_number(file, "golden-rings"));

    yaml_close(file);

    
    #ifdef DEBUG
        printf("\nAllocated: %i, Freed: %i\n", total_allocated, total_freed);
    #endif
}