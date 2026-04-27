#include <stdlib.h>
#include <string.h>
#include "char.h"

int add_char(Document* doc, Cursor* curs, int ch) {
    LINE* line = &doc->items[curs->point_y];

    if (line->len + 2 > line->capacity) {
        line->capacity *= 2;
        char* tpm_chars = realloc(line->chars, line->capacity);
        if (tpm_chars == NULL) {
            perror("Error realloc add");
            return 1;
        }
        line->chars = tpm_chars;
    }

    if (curs->point_x < line->len) {
        memmove(&line->chars[curs->point_x + 1], 
                &line->chars[curs->point_x], 
                line->len - curs->point_x + 1);
    } else {
        line->chars[line->len + 1] = '\0';
    }

    line->chars[curs->point_x] = (char)ch;
    line->len++;
    curs->point_x++;

    return 0;
}

int del_char(Document* doc, Cursor* curs) {
    if(curs->point_x <= 0 && curs->point_y > 0) {
        size_t new_len = doc->items[curs->point_y-1].len + doc->items[curs->point_y].len;
        if(new_len + 2 >= doc->items[curs->point_y - 1].capacity) {
            char* tpm_chars = realloc(doc->items[curs->point_y - 1].chars, new_len+2);
            if(tpm_chars == NULL) {
                perror("Error realloc chars");
                return 1;
            }
            doc->items[curs->point_y - 1].capacity = new_len+2;
            doc->items[curs->point_y - 1].chars = tpm_chars;
        }
        memcpy(doc->items[curs->point_y-1].chars + doc->items[curs->point_y-1].len, doc->items[curs->point_y].chars, doc->items[curs->point_y].len + 1);
        free(doc->items[curs->point_y].chars);
        if (curs->point_y < doc->count - 1) {
            memmove(&doc->items[curs->point_y], 
                    &doc->items[curs->point_y + 1], 
                    sizeof(LINE) * (doc->count - curs->point_y - 1));
        }
        curs->point_x = doc->items[curs->point_y - 1].len;
        doc->items[curs->point_y - 1].len = new_len;
        curs->point_y--;
        doc->count--;
        return 1;
    }
    if(curs->point_x <= 0 && curs->point_y <= 0) return 1;

    LINE* line = &doc->items[curs->point_y];
    if(line->len > curs->point_x) {
        memmove(&line->chars[curs->point_x - 1], 
                &line->chars[curs->point_x], 
                line->len - curs->point_x + 1);
    } else {
        line->chars[curs->point_x-1] = '\0';
    }
    line->len--;
    curs->point_x--;
    return 0;
}

int new_line(Document* doc, Cursor* curs) {
    if(doc->count+2 > doc->capacity) {
        LINE* tpm_items = realloc(doc->items, doc->capacity * 2 * sizeof(LINE));
        if(tpm_items == NULL) {
            perror("Error realloc add line");
            return 1;
        }
        doc->capacity *= 2;
        doc->items = tpm_items;

        for (size_t i = doc->capacity / 2; i < doc->capacity; i++) {
            doc->items[i].chars = NULL;
            doc->items[i].len = 0;
            doc->items[i].capacity = 0;
        }
    }
    
    if(curs->point_y + 1 < doc->count) {
        memmove(&doc->items[curs->point_y + 2], &doc->items[curs->point_y + 1], sizeof(LINE) * (doc->count - (curs->point_y + 1)));
    }
    
    memset(&doc->items[curs->point_y+1], 0, sizeof(LINE));

    doc->items[curs->point_y+1].chars = malloc(CHAR_CAPACITY);
    if(doc->items[curs->point_y+1].chars == NULL) {
        perror("malloc lines");
        return 2;
    }

    if(doc->items[curs->point_y].len > curs->point_x) {
        size_t tail_len = doc->items[curs->point_y].len - curs->point_x;
        if(tail_len + 1 > CHAR_CAPACITY) {
            char* tpm_chars = realloc(doc->items[curs->point_y+1].chars, tail_len+1);
            if(tpm_chars == NULL) {
                perror("Error realloc chars");
                return 1;
            }
            doc->items[curs->point_y+1].chars = tpm_chars;
            doc->items[curs->point_y+1].capacity = tail_len+1;
        } else {
            doc->items[curs->point_y+1].capacity = CHAR_CAPACITY;
        }
        memcpy(doc->items[curs->point_y+1].chars, &doc->items[curs->point_y].chars[curs->point_x], tail_len + 1);
        doc->items[curs->point_y].chars[curs->point_x] = '\0';
        doc->items[curs->point_y+1].len = doc->items[curs->point_y].len - curs->point_x;
        doc->items[curs->point_y].len = curs->point_x;
    } else {
        doc->items[curs->point_y+1].chars[0] = '\0';
        doc->items[curs->point_y+1].len = 0;
        doc->items[curs->point_y+1].capacity = 10;
    }
    doc->count++;
    curs->point_y++;
    curs->point_x = 0;
    return 0;
}
