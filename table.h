#pragma once
#include <stdio.h>

typedef struct HTable {
    unsigned int full_hash;
    char* name;
    unsigned char rgb[3];
    struct HTable* next;
} HTable;

typedef struct {
    HTable* tables;
    size_t count;
} Tables;

int get_color(Tables* tab, char* name, size_t capacity, char color[3]);
int import_theme(char* filename, Tables* tab);
int new_table(Tables* tab, char* name, char color[3]);
void init_table(Tables* tab);
void free_table(Tables* tab);