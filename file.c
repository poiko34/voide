#include <string.h>
#include <stdlib.h>
#include "file.h"
#include "char.h"

int save_to_file(const char* filename, const Document* doc) {
    FILE* fp = fopen(filename, "w");
    if (fp == NULL) {
        perror("Error opening file for saving");
        return 1;
    }

    for (size_t i = 0; i < doc->count; i++) {
        fwrite(doc->items[i].chars, sizeof(char), doc->items[i].len, fp);
        fputc('\n', fp);
    }

    fclose(fp);
    return 0;
}

int load_from_file(char* filename, Document* doc) {
    FILE* fp = fopen(filename, "r");
    if (fp == NULL) {
        if (errno == ENOENT) return 2;
        perror("Error opening file");
        return 1;
    }

    clean_exit(doc);
    doc->count = 0;
    doc->capacity = 10;
    doc->items = calloc(doc->capacity, sizeof(LINE));

    char* buffer = malloc(CHAR_CAPACITY);
    size_t buf_capacity = CHAR_CAPACITY;
    size_t length = 0;
    int ch;

    while (1) {
        ch = fgetc(fp);

        if (ch == '\n' || ch == EOF) {
            if (doc->count >= doc->capacity) {
                doc->capacity *= 2;
                LINE* tmp_items = realloc(doc->items, doc->capacity * sizeof(LINE));
                if (!tmp_items) return 1;
                doc->items = tmp_items;
                
                for (size_t i = doc->count; i < doc->capacity; i++) {
                    doc->items[i].chars = NULL;
                    doc->items[i].len = 0;
                    doc->items[i].capacity = 0;
                }
            }

            doc->items[doc->count].capacity = length + 1;
            if (doc->items[doc->count].capacity < CHAR_CAPACITY) 
                doc->items[doc->count].capacity = CHAR_CAPACITY;
            
            doc->items[doc->count].chars = malloc(doc->items[doc->count].capacity);
            
            if (length > 0) {
                memcpy(doc->items[doc->count].chars, buffer, length);
            }
            doc->items[doc->count].chars[length] = '\0';
            doc->items[doc->count].len = length;
            
            doc->count++;
            length = 0;

            if (ch == EOF) break;
            continue;
        }

        if (length + 2 >= buf_capacity) {
            buf_capacity *= 2;
            char* tmp_buf = realloc(buffer, buf_capacity);
            if (!tmp_buf) return 1;
            buffer = tmp_buf;
        }
        buffer[length++] = (char)ch;
    }

    free(buffer);
    fclose(fp);

    if (doc->count == 0) init_doc(doc);
    
    return 0;
}

void clean_exit(Document* doc) {
    if (doc->items == NULL) return;

    for(size_t i = 0; i < doc->count; i++) {
        if(doc->items[i].chars != NULL) {
            free(doc->items[i].chars);
            doc->items[i].chars = NULL;
        }
    }
    free(doc->items);
    doc->items = NULL;
}

int init_doc(Document* doc) {
    doc->count = 1;
    doc->capacity = 1;
    doc->items = calloc(doc->count, sizeof(LINE));
    if(doc->items == NULL) {
        perror("malloc lines");
        return 1;
    }
    doc->items[0].chars = malloc(CHAR_CAPACITY);
    if(doc->items[0].chars == NULL) {
        perror("malloc lines");
        return 1;
    }
    doc->items[0].len = 0;
    doc->items[0].capacity = CHAR_CAPACITY;
    doc->items[0].chars[0] = '\0';
    return 0;
}