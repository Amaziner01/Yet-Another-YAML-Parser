#ifndef YET_ANOTHET_YAML_PARSER
#define YET_ANOTHET_YAML_PARSER

#ifdef __cplusplus
extern "C" {
#endif


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef DEBUG

    static int total_allocated = 0;
    static int total_freed = 0;

    void *
    y_alloc( size_t size ) {
        total_allocated++;
        return malloc(size);
    }

    int
    y_free( void *ptr ) {

        if ( ptr ) {
            total_freed++;
            free(ptr);
            return 1;
        }

        return 0;
    }

#else

    #define y_alloc malloc

    #define y_free free

#endif


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
};

typedef struct yaml_file yaml_file_t;


/* Lexical Analisys Functions */
#define IS_NUMBER(x) ( (x <= '9') && (x >= '0') || (x == '.') )

#define IS_SYMBOL(x) ( ((x <= 'z') && (x >= 'a')) || ((x <= 'Z') && (x >= 'A')) || (x == '_') || (x == '$') || (x == '-') ) 

#define IS_BLANK(x) ( (x == '\t') || (x == ' ') )

#define IS_NEWLINE(x) ( (x == '\n') || (x == '\r') )

#define IS_QUOTE(x) ( (x == '\'') || (x == '\"') )

#define SKIP_BLANK(c) while ( IS_BLANK(*c) ) c++

#define SKIP_NEWLINE(c) while ( IS_NEWLINE(*c) ) c++


void
yaml_insert_node( struct yaml_file *file, enum yaml_type type, char *label, char *data ) {
    if ( !file ) return;

    struct yaml_node *node = (struct yaml_node*)y_alloc(sizeof(struct yaml_node));

    /* Set Values */
    node->type = type;
    node->label = label;
    node->next = NULL;

    /* Set data */
    switch ( type ) {
        case NUMBER: {
            node->value.num = atof(data);
        } break;

        case STRING: {
            node->value.str = data;
        } break;

        default: {
            y_free(node);
            return;
        } break;
    }
    
    if (!file->tail) {
        file->tail = node;
        file->head = file->tail;
        return;
    }

    file->tail->next = node;
    file->tail = node;

    return;
}

yaml_file_t *
yaml_open( const char *path ) {

    FILE *fstream;
    size_t fsize;
    char *fcursor;
    yaml_file_t *yfile;

    char *lbeg;
    char *dbeg;
    
    fstream = fopen(path, "rb");

    if ( !fstream ) {
        fprintf(stderr, "Couldn't find \"%s\"\n", path);
        return NULL;
    }

    fseek(fstream, 0, SEEK_END);
    fsize = ftell(fstream);
    fseek(fstream, 0, SEEK_SET);

    yfile = (yaml_file_t*)y_alloc(sizeof(yaml_file_t));

    yfile->name = path;
    yfile->head = NULL;
    yfile->tail = NULL;

    yfile->fbuffer = (char*)y_alloc(fsize + 1);
    fread(yfile->fbuffer, fsize, 1, fstream);
    yfile->fbuffer[fsize - 1] = '\0';

    fclose(fstream);
    fcursor = yfile->fbuffer;

    

    enum yaml_type stype;

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
                }
                else {
                    /* Handle "No space after colon" */
                    fprintf(stderr, "No space after colon\n");
                    break;
                }
            }

            /* INSERT DATA */
            /* printf("Label: %s, Data: %s\n", lbeg, dbeg); */
            yaml_insert_node(yfile, stype, lbeg, dbeg);

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

char *
yaml_get_string( yaml_file_t *yfile, char *label ) {
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

double
yaml_get_number( yaml_file_t *yfile, char *label ) {
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

void
yaml_close( yaml_file_t *yfile ) {
    if ( !yfile ) return;

    /* Delete nodes */
    struct yaml_node *node = yfile->head;
    while ( node ) {
        yfile->head = node->next;

        /* Free node */
        y_free(node);
        node = yfile->head;
    }

    y_free(yfile->fbuffer);
    y_free(yfile);
    yfile = NULL;
}

#ifdef __cplusplus
}
#endif

#endif  /* YET_ANOTHET_YAML_PARSER */