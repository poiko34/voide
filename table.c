#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "table.h"

int hash(const char word[], size_t capacity) {
    int hash_val = 0;

    for(size_t i = 0; i < capacity; i++) {
        unsigned char current = (unsigned char)word[i];
        unsigned char next = (unsigned char)word[(i + 1) % capacity];
        
        hash_val = (hash_val << 5) + (current ^ (next + (int)i));
    }

    return hash_val;
}

void init_table(Tables* tab) {
    tab->tables = malloc(10 * sizeof(HTable));
    tab->count = 10;
    for(int i = 0; i < 10; i++) {
        tab->tables[i].full_hash = 0;
        tab->tables[i].name = NULL;
        tab->tables[i].next = NULL;
    }
}

void free_table(Tables* tab) {
    for (size_t i = 0; i < tab->count; i++) {
        free(tab->tables[i].name);
        
        HTable* current = tab->tables[i].next;
        while (current != NULL) {
            HTable* to_free = current;
            current = current->next;
            
            free(to_free->name);
            free(to_free);
        }
        
        tab->tables[i].next = NULL;
        tab->tables[i].name = NULL;
    }
    
    free(tab->tables);
}

int new_table(Tables* tab, char* name, char color[3]) {
    unsigned int hash_val = hash(name, strlen(name));
    int idx = hash_val % tab->count;

    if (tab->tables[idx].name == NULL) { 
        tab->tables[idx].full_hash = hash_val;
        tab->tables[idx].name = strdup(name);
        for(int i = 0; i < 3; i++) tab->tables[idx].rgb[i] = color[i];
        tab->tables[idx].next = NULL;
        return 0;
    }

    HTable* current = &tab->tables[idx];
    while (current != NULL) {
        if (current->full_hash == hash_val && strcmp(current->name, name) == 0) {
            for(int i = 0; i < 3; i++) current->rgb[i] = color[i];
            return 0;
        }
        if (current->next == NULL) break;
        current = current->next;
    }

    HTable* new_node = malloc(sizeof(HTable));
    new_node->full_hash = hash_val;
    new_node->name = strdup(name);
    for(int i = 0; i < 3; i++) new_node->rgb[i] = color[i];
    new_node->next = NULL;

    current->next = new_node;
    return 0;
}

int get_color(Tables* tab, char* name, size_t capacity, char color[3]) {
    int hash_val = hash(name, capacity);
    int idx = hash_val % tab->count;

    if(tab->tables[idx].full_hash == 0) return 1;

    if(strcmp(name, tab->tables[idx].name) != 0) {
        HTable* current = tab->tables[idx].next;
        while(1) {
            if(current != NULL) {
                if(strcmp(name, current->name) != 0) {
                    current = current->next;
                    continue;
                } 
            } else return 1;
            break;
            }
        for(int i = 0; i < 3; i++) color[i] = current->rgb[i];
        return 0;
    }
    for(int i = 0; i < 3; i++) color[i] = tab->tables[idx].rgb[i];
    return 0;
}

int import_theme(char* filename, Tables* tab) {
    FILE* fp = fopen(filename, "r");
    if(fp == NULL) {
        perror(filename);
        return 1;
    }
    char buffer[512];
    while(fgets(buffer, 512, fp) != NULL) {
        unsigned char rgb[3];
        char name[100];
        int hex;
        if(sscanf(buffer, "%99s #%x", name, &hex) == 2) {
            rgb[0] = (hex >> 16) & 0xFF;
            rgb[1] = (hex >> 8) & 0xFF;
            rgb[2] = hex & 0xFF;

            // printf("name: %s, hex code: R:%d G:%d B:%d\n", name, rgb[0], rgb[1], rgb[2]);
            new_table(tab, name, rgb);
        }
    }
    fclose(fp);

    return 0;
}

// int main() {
//     Tables tab;
//     init_table(&tab);
//     import_theme("theme.cfg", &tab);

//     {
//         unsigned char tpm_rgb[3];
//         get_color(&tab, "int", strlen("int"), tpm_rgb);
//         // printf("%d %d %d\n", tpm_rgb[0], tpm_rgb[1], tpm_rgb[2]);
//     }

//     free_table(&tab);
//     return 0;
// }