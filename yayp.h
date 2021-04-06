#ifndef YET_ANOTHET_YAML_PARSER
#define YET_ANOTHET_YAML_PARSER

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define inline_ static

#define IS_NUMBER(x) ( (x <= '9') && (x >= '0') || (x == '.') )

#define IS_SYMBOL(x) ( ((x <= 'z') && (x >= 'a')) || ((x <= 'Z') && (x >= 'A')) || (x == '_') || (x == '$') || (x == '-') ) 

#define IS_BLANK(x) ( (x == '\t') || (x == ' ') )

#define IS_NEWLINE(x) ( (x == '\n') || (x == '\r') )

#define IS_QUOTE(x) ( (x == '\'') || (x == '\"') )

#define SKIP_BLANK(c) while ( IS_BLANK(*c) ) c++

#define SKIP_NEWLINE(c) while ( IS_NEWLINE(*c) ) c++


/* MEMPOOL STRUCTS */

struct yml_alloc_header {
    struct yml_alloc_header *next;
    unsigned size;
    unsigned char freed;
};

typedef struct yml_alloc_header yml_alloc_header_t;

struct yml_mempool {
    void *mem_block;
    unsigned mem_size;
    unsigned mem_used_bytes;
};

typedef struct yml_mempool yml_mempool_t;


/* YAML STRUCTS */

enum yaml_type {
    NUMBER = 0,
    STRING,
    BIN,
    NUMBER_ARRAY,
    STRING_ARRAY,
    NODE_PARENT,
};

struct yaml_node {
    enum yaml_type type;
    char *label;

    union {
        unsigned char boolean;
        double num;
        char *str;
    } value;

    struct yaml_node *next;
};

struct yaml_file {
    const char *name;
    char *fbuffer;
    struct yaml_node *head;
    struct yaml_node *tail;
    yml_mempool_t *mempool;
};

typedef struct yaml_file yaml_file_t;


/* MEMPOOL FUNCS */

inline_  yml_mempool_t *
create_mempool( unsigned size ) {
    yml_mempool_t *m = (yml_mempool_t*)malloc(sizeof(yml_mempool_t)); 
    if (!m) return NULL;

    m->mem_block = malloc(size);
    if (!m->mem_block) {
        free(m);
        return NULL;
    }

    memset(m->mem_block, 0, size);

    m->mem_used_bytes = 0;
    m->mem_size = size;

    return m;
}

inline_  void
terminate_mempool( yml_mempool_t *yml_mempool ) {
    if (!yml_mempool) return;

    free(yml_mempool->mem_block);
    free(yml_mempool);
}

inline_  void *
alloc( unsigned size, yml_mempool_t *yml_mempool ) {
    if (!yml_mempool) return NULL;
    if (size == 0) return NULL;
    

    yml_alloc_header_t *last_header = (yml_alloc_header_t*)yml_mempool->mem_block;
    yml_alloc_header_t *yml_alloc_header;

    int final_size = size;
    
    if (!last_header->size) {
        yml_alloc_header = last_header;
        goto assignment;
    }

    while (last_header->next) {
        if (last_header->freed) {
            if (last_header->size >= size) {
                yml_alloc_header = last_header;
                goto assignment; 
            } 
        }

        last_header = last_header->next;
    }

    final_size += sizeof(yml_alloc_header_t);
    yml_alloc_header =
        (yml_alloc_header_t*)((unsigned long long)last_header + sizeof(yml_alloc_header_t) + last_header->size);

    last_header->next = yml_alloc_header;
    yml_alloc_header->next = NULL;

assignment:
    yml_mempool->mem_used_bytes += final_size;

    if (yml_mempool->mem_used_bytes + final_size > yml_mempool->mem_size) {
        last_header->next = NULL;
        fprintf(stderr, "Not enough memory to allocate", NULL);
    }

    yml_alloc_header->freed = 0;
    yml_alloc_header->size = size;

    return (void*)((unsigned long long)yml_alloc_header + sizeof(yml_alloc_header_t));
}

inline_ void
release( void *ptr, yml_mempool_t *yml_mempool ) {
    if(!ptr) return;
    if (!yml_mempool) return;

    yml_alloc_header_t *yml_alloc_header = 
        (yml_alloc_header_t*)((unsigned long long)ptr - sizeof(yml_alloc_header_t));
    
    yml_alloc_header->freed = 1;
}


/* YAML FILE FUNCS */

inline_ void
yaml_insert_string_node( struct yaml_file *file, char *label, char *data ) {
    if ( !file ) return;

    struct yaml_node *node = (struct yaml_node*)alloc(sizeof(struct yaml_node), file->mempool);

    /* Set Values */
    node->type = STRING;
    node->label = label;
    node->next = NULL;
    node->value.str = data;
    
    if (!file->tail) {
        file->tail = node;
        file->head = file->tail;
        return;
    }

    file->tail->next = node;
    file->tail = node;

    return;
}

inline_ void
yaml_insert_number_node( struct yaml_file *file, char *label, double data ) {
    if ( !file ) return;

    struct yaml_node *node = (struct yaml_node*)alloc(sizeof(struct yaml_node), file->mempool);

    /* Set Values */
    node->type = NUMBER;
    node->label = label;
    node->next = NULL;
    node->value.num = data;
    
    if (!file->tail) {
        file->tail = node;
        file->head = file->tail;
        return;
    }

    file->tail->next = node;
    file->tail = node;

    return;
}

inline_ void
yaml_insert_bool_node( struct yaml_file *file, char *label, unsigned char data ) {
    if ( !file ) return;

    struct yaml_node *node = (struct yaml_node*)alloc(sizeof(struct yaml_node), file->mempool);

    /* Set Values */
    node->type = BIN;
    node->label = label;
    node->next = NULL;
    node->value.boolean = data;
    
    if (!file->tail) {
        file->tail = node;
        file->head = file->tail;
        return;
    }

    file->tail->next = node;
    file->tail = node;

    return;
}

inline_ yaml_file_t *
yaml_open( const char *path ) {

    FILE *fstream;
    size_t fsize;
    char *fcursor;
    yaml_file_t *yfile;

    char *lbeg;
    char *dbeg;

    const char *falses[] = {"false", "no", "FALSE", "False"};
    const char *trues[] = {"true", "yes", "TRUE", "True"};
    
    fstream = fopen(path, "rb");

    if ( !fstream ) {
        fprintf(stderr, "Couldn't find \"%s\"\n", path);
        return NULL;
    }

    fseek(fstream, 0, SEEK_END);
    fsize = ftell(fstream);
    fseek(fstream, 0, SEEK_SET);

    yfile = (yaml_file_t*)malloc(sizeof(yaml_file_t));

    yfile->name = path;
    yfile->head = NULL;
    yfile->tail = NULL;

    yfile->fbuffer = (char*)malloc(fsize + 1);
    fread(yfile->fbuffer, fsize, 1, fstream);
    yfile->fbuffer[fsize - 1] = '\0';

    fclose(fstream);
    fcursor = yfile->fbuffer;

    yfile->mempool = create_mempool(2048);
    if (!yfile->mempool) {
        free(yfile);
        return NULL;
    }

    enum yaml_type stype;
    unsigned char bval;

    /* Parsing */
    while (*fcursor != '\0') {

        if ( IS_SYMBOL(*fcursor) ) {
            lbeg = fcursor;

            while ( IS_SYMBOL(*fcursor) || IS_BLANK(*fcursor) || IS_NUMBER(*fcursor) ) {
                fcursor++;
            }

            /* Printing word */

            if (*fcursor == ':') {
                *fcursor = '\0';
                fcursor++;

                if ( IS_BLANK(*fcursor) ) {
                    SKIP_BLANK(fcursor);

                    if ( IS_QUOTE(*fcursor) ) {
                        char quote = *fcursor;
                        fcursor++;
                        dbeg = fcursor;

                        stype = STRING;

                        while ( !IS_QUOTE(*fcursor) ) {
                            fcursor++;
                        }

                        if (*fcursor != quote) {
                            /* Handle "Non matching quote" */
                            fprintf(stderr, "Non matching quote\n");
                            break;
                        }

                        *fcursor = '\0';
                        fcursor++;

                    }
                    else if ( IS_NUMBER(*fcursor) ) {
                        /* TODO: This is temporary */
                        dbeg = fcursor;
                        stype = NUMBER;
                    }
                    else {
                        dbeg = fcursor;
                        stype = STRING;                        
                    }

                    /* NULL Termination */
                    while ( !IS_NEWLINE(*fcursor) ) {
                        fcursor++;
                    }

                    short offset = *fcursor == '\r' ? 2 : 1;
                    *fcursor = '\0';
                    fcursor += offset;

                    for (int i = 0; i < 4; i++) {
                        if ( !strcmp(dbeg, trues[i]) ) {
                            stype = BIN;
                            bval = 1;
                            goto node;  
                        }
                    }

                    for (int i = 0; i < 4; i++) {
                        if ( !strcmp(dbeg, falses[i]) ) {
                            stype = BIN;
                            bval = 0;
                            goto node;  
                        }
                    }

                }
                else {
                    /* Handle "No space after colon" */
                    fprintf(stderr, "No space after colon\n");
                    break;
                }
            }

            /* INSERT DATA */
            /* printf("Label: %s, Data: %s\n", lbeg, dbeg); */

node:

            switch ( stype ) {
                case NUMBER: {
                    yaml_insert_number_node(yfile, lbeg, atof(dbeg));
                } break;

                case STRING: {
                    yaml_insert_string_node(yfile, lbeg, dbeg);
                } break;

                case BIN: {
                    yaml_insert_bool_node(yfile, lbeg, bval);
                } break;

                default: break;
            }

        }
        else if ( IS_BLANK(*fcursor) ) {
            SKIP_BLANK(fcursor);
        }
        else if ( IS_NEWLINE(*fcursor) ) {
            SKIP_NEWLINE(fcursor);
        }
        else {
            /* Handle "Label first char is not a symbol" */
            fprintf(stderr, "Label's first char is not a symbol\n");
            break;
        }
    }
    return yfile;
}

inline_ char *
yaml_get_string( yaml_file_t *yfile, const char *label ) {
    if ( !yfile ) return NULL;

    struct yaml_node *node = yfile->head;
    while ( node ) {
        if (!strcmp(label, node->label)) {
            if (node->type == STRING) {
                return node->value.str;
            }
            fprintf(stderr, "This label is not a string\n");
            return NULL;
        }

        node = node->next;
    }
    fprintf(stderr, "Label not found.\n");
    return NULL;  
}

inline_ double
yaml_get_number( yaml_file_t *yfile, const char *label ) {
    if ( !yfile ) return 0.0;

    struct yaml_node *node = yfile->head;
    while ( node ) {
        if (!strcmp(label, node->label)) {
            if (node->type == NUMBER) {
                return node->value.num;
            }
            fprintf(stderr, "This label is not a number\n");
            return 0.0;
        }

        node = node->next;
    }
    fprintf(stderr, "Label not found.\n");
    return 0.0;  
}

inline_ unsigned char
yaml_get_bool( yaml_file_t *yfile, const char *label ) {
    if ( !yfile ) return 0;

    struct yaml_node *node = yfile->head;
    while ( node ) {
        if (!strcmp(label, node->label)) {
            if (node->type == BIN) {
                return node->value.boolean;
            }
            fprintf(stderr, "This label is not a bool\n");
            return 0;
        }

        node = node->next;
    }
    fprintf(stderr, "Label not found.\n");
    return 0;  
}

inline_ void
yaml_close( yaml_file_t *yfile ) {
    if ( !yfile ) return;

    /* Delete nodes */
    struct yaml_node *node = yfile->head;
    while ( node ) {
        yfile->head = node->next;

        /* Free node */
        release(node, yfile->mempool);
        node = yfile->head;
    }

    free(yfile->fbuffer);
    terminate_mempool(yfile->mempool);
    free(yfile);
    yfile = NULL;
}


#ifdef __cplusplus
}
#endif

#endif  /* YET_ANOTHET_YAML_PARSER */