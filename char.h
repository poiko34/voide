#pragma once
#include <stdio.h>

#define CHAR_CAPACITY 10

typedef struct {
    char* chars;
    size_t len;
    size_t capacity;
} LINE;
typedef struct {
    LINE* items;
    size_t count;
    size_t capacity;
} Document;
typedef struct {
    int point_x;
    int point_y;
    int scroll_x;
    int scroll_y;
} Cursor;

int add_char(Document* doc, Cursor* curs, int ch);
int del_char(Document* doc, Cursor* curs);
int new_line(Document* doc, Cursor* curs);