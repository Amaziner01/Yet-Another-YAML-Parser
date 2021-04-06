#include <stdio.h>

#define DEBUG
#include "yayp.h"
#include "test.h"


int
main(void) {

    /*    
    yaml_file_t *file = yaml_open("basic.yml");

    printf("%f\n", yaml_get_number(file, "Truthfullness"));
    printf("%f\n", yaml_get_number(file, "cool1"));
    printf("%f\n", yaml_get_number(file, "french-hens"));

    yaml_close(file);
    */

    Test(1000);
    
    #ifdef DEBUG
        printf("\nAllocated: %i, Freed: %i\n", total_allocated, total_freed);
    #endif
}