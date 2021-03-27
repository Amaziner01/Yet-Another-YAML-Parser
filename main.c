#include <stdio.h>
#include <stdlib.h>

#include <windows.h>

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
    void *value;

    struct yaml_node *next;
};

struct yaml_file {
    const char *name;
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

struct yaml_node *
yaml_insert_node( yaml_file_t *this, enum yaml_type type, char *label, char *data ) {
    struct yaml_node *node = malloc(sizeof(struct yaml_node));

    /* Set Values */
    node->type = type;
    node->label = label;
    node->next = NULL;

    /* Set data */
    switch ( type ) {
        case NUMBER: {
            node->value = malloc(sizeof(double));
            *(double*)node = atof(data);
        } break;

        case STRING: {
            int len = strlen(data);
            node->value = malloc(len + 1);
            strncpy(node->value, data, len);
        } break;

        default: {
            free(node);
            return NULL;
        } break;
    }
    
    if (!this->tail) {
        this->tail = node;
        this->head = this->tail;

        return node;
    }

    this->tail->next = node;
    this->tail = node;

    return node;
}

yaml_file_t *
yaml_open( const char *path ) {

    FILE *fstream;
    char *fbuffer;
    size_t fsize;
    char *fcursor;
    yaml_file_t *yfile;

    char *lbeg;
    size_t lsize;
    char *dbeg;
    size_t dsize;
    
    fstream = fopen(path, "rb");

    if ( !fstream ) {
        fprintf(stderr, "Couldn't find \"%s\"\n", path);
        return NULL;
    }

    fseek(fstream, 0, SEEK_END);
    fsize = ftell(fstream);
    fseek(fstream, 0, SEEK_SET);

    fbuffer = malloc(fsize + 1);
    fread(fbuffer, fsize, 1, fstream);
    fbuffer[fsize - 1] = '\0';

    fclose(fstream);
    fcursor = fbuffer;

    yfile = malloc(sizeof(yaml_file_t));

    yfile->name = path;
    yfile->head = NULL;
    yfile->tail = NULL;

    /* Parsing */
    while (*fcursor != '\0') {

        if ( IS_SYMBOL(*fcursor) ) {
            lbeg = fcursor;
            lsize = 0;

            while ( IS_SYMBOL(*fcursor) || IS_BLANK(*fcursor) || IS_NUMBER(*fcursor) ) {
                fcursor++;
                lsize++;
            }

            /* Printing word */

            if (*fcursor == ':') {
                *fcursor = '\0';
                fcursor++;

                if ( IS_BLANK(*fcursor) ) {
                    SKIP_BLANK(fcursor);
                    dsize = 0;

                    if ( IS_QUOTE(*fcursor) ) {
                        char quote = *fcursor;
                        fcursor++;
                        dbeg = fcursor;

                        while ( !IS_QUOTE(*fcursor) ) {
                            fcursor++;
                            dsize++;
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
                        // TODO: This is temporary
                        dbeg = fcursor;
                    }
                    else {
                        dbeg = fcursor;
                    }

                    /* NULL Termination */
                    while ( !IS_NEWLINE(*fcursor) ) {
                        fcursor++;
                        dsize++;
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
            printf("Label: %s, Data: %s\n", lbeg, dbeg);
            yaml_insert_node(yfile, )

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

    free(fbuffer);
    return 
}


#include <time.h>

static double
get_time(void)
{
    double time = (double)(clock() / 1000.0);
    return time;
}

int
main(void) {
    double begin, end;

    begin = get_time();
    yaml_file_t *file = yaml_open("basic.yml");
    end = get_time();

    printf("Time Elapsed: %f s", end - begin);

    getchar();
}