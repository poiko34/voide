#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "init.c"

#define CHAR_CAPACITY 10
#define ARROW_UP 65
#define ARROW_DOWN 66
#define ARROW_RIGHT 67
#define ARROW_LEFT 68
#define BACKSPACE 127
#define ENTER 13
#define SAVE 19

struct winsize ws;
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

void tui(Document* doc, const char* filename, Cursor* curs) {
    char* name = "voide 1.0";
    int header_height = 1;
    int footer_height = 3;
    int view_height = ws.ws_row - header_height - footer_height;

    if (curs->point_y >= curs->scroll_y + view_height) {
        curs->scroll_y = curs->point_y - view_height + 1;
    }
    if (curs->point_y < curs->scroll_y) {
        curs->scroll_y = curs->point_y;
    }

    if (curs->point_x >= curs->scroll_x + ws.ws_col) {
        curs->scroll_x = curs->point_x - ws.ws_col + 5;
    }
    if (curs->point_x < curs->scroll_x) {
        curs->scroll_x = curs->point_x;
    }

    printf("\033[1;1H\x1b[J");

    int pad_left = (ws.ws_col - (int)strlen(filename) - (int)strlen(name)) / 2;
    printf("\x1b[47;30m  %s%*s%s%*s\x1b[0m", name, pad_left - 2, "", filename, ws.ws_col - pad_left - (int)strlen(filename) - (int)strlen(name), "");

    for (int i = 0; i < view_height; i++) {
        int doc_idx = i + curs->scroll_y;
        if (doc_idx >= doc->count) break;

        printf("\033[%d;1H", i + 2);
        
        LINE* line = &doc->items[doc_idx];
        if (line->len > (size_t)curs->scroll_x) {
            char* visible_part = line->chars + curs->scroll_x;
            int visible_len = (int)line->len - curs->scroll_x;

            if (visible_len > ws.ws_col) {
                printf("%.*s\x1b[1;33m>\x1b[0m", ws.ws_col - 1, visible_part);
            } else {
                printf("%s", visible_part);
            }
        }
        
        if (curs->scroll_x > 0 && doc_idx == curs->point_y) {
            printf("\033[%d;1H\x1b[1;33m$\x1b[0m", i + 2);
        }
    }

    printf("\033[%d;1H\x1b[47;30m^C\x1b[0m Exit", ws.ws_row - 1);
    printf("\033[%d;1H\x1b[47;30m^S\x1b[0m Save", ws.ws_row);

    printf("\033[%d;%dH", (curs->point_y - curs->scroll_y) + 2, (curs->point_x - curs->scroll_x) + 1);
}

void message(char* mess, Cursor* curs) {
    size_t mess_len = strlen(mess) + 4;
    int pad_left = (ws.ws_col - mess_len) / 2;
    printf("\033[%d;1H", ws.ws_row - 2);
    if(mess[0] == '\0') {
        printf("\x1b[0m%*s", ws.ws_col, "");
    } else {
        printf("%*s\x1b[47;30m  %s  \x1b[0m", pad_left, "", mess);
    }
    printf("\033[%d;%dH", curs->point_y+2, curs->point_x+1);
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

int save_to_file(const char* filename, const Document* doc) {
    FILE* fp = fopen(filename, "w");
    if (fp == NULL) {
        perror("Error opening file for saving");
        return 1;
    }

    for (size_t i = 0; i < doc->count; i++) {
        if (doc->items[i].len > 0) {
            fwrite(doc->items[i].chars, sizeof(char), doc->items[i].len, fp);
        }
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

int main(int argc, char *argv[]) {
    if(argc < 2) {
        printf("main <filename>\r\n");
        return 1;
    }
    enableRawMode();
    
    setvbuf(stdout, NULL, _IONBF, 0);
	ioctl(STDIN_FILENO, TIOCGWINSZ, &ws);
    char program_is_work = 1;
    char is_saved = 1;
    
    Cursor cursor = {0, 0, 0};
    Document doc;
    if (init_doc(&doc) != 0) {
        fprintf(stderr, "Fatal: memory allocation failed\n");
        clean_exit(&doc);
        disableRawMode();
        return 1;
    }

    {
        int status = load_from_file(argv[1], &doc);
        if(status == 2) {
            tui(&doc ,argv[1], &cursor);
            message("File not founded", &cursor);
        } else if(status != 0) {
            perror(argv[1]);
            return 1;
        } else {
            tui(&doc ,argv[1], &cursor);
        }
    }
    
    while(program_is_work) {
        int ch = fgetc(stdin);
        switch (ch)
        {
        case 3:
            if(is_saved == 0) {
                message("Exit without saving? - N to deny, Y to exit", &cursor);
                while(1) {
                    int next_ch = fgetc(stdin);
                    if(next_ch == 89) {
                        program_is_work = 0;
                        printf("\033[1;1H\x1b[J");
                        break;
                    } else if(next_ch == 78) {
                        message("", &cursor);
                        break;
                    } else {
                        printf("\a");
                        fflush(stdout);
                    }
                }
            } else {
                program_is_work = 0;
                printf("\033[1;1H\x1b[J");
            }
            break;
        case 27:
            int next_ch = fgetc(stdin);
            if(next_ch == 91) {
                int next_next_ch = fgetc(stdin);
                if(next_next_ch == ARROW_UP) {
                    if(cursor.point_y > 0) {
                        if(doc.items[cursor.point_y - 1].len < cursor.point_x)
                            cursor.point_x = doc.items[cursor.point_y - 1].len;
                        cursor.point_y--;
                    }
                    tui(&doc, argv[1], &cursor);
                }
                if(next_next_ch == ARROW_DOWN) {
                    if(doc.count > cursor.point_y+1) {
                        if(doc.items[cursor.point_y + 1].len < cursor.point_x)
                            cursor.point_x = doc.items[cursor.point_y + 1].len;
                        cursor.point_y++;
                    }
                    tui(&doc, argv[1], &cursor);
                }
                if(next_next_ch == ARROW_RIGHT) {
                    if(doc.items[cursor.point_y].len > cursor.point_x) {
                        cursor.point_x++;
                    } else if (doc.count > cursor.point_y + 1) {
                        cursor.point_x = 0;
                        cursor.point_y++;
                    }
                    tui(&doc, argv[1], &cursor);
                }
                if(next_next_ch == ARROW_LEFT) {
                    if(cursor.point_x > 0) {
                        cursor.point_x--;
                    } else if(cursor.point_y > 0) {
                        cursor.point_x = doc.items[cursor.point_y - 1].len;
                        cursor.point_y--;
                    }
                    tui(&doc, argv[1], &cursor);
                }
            }
            break;
        case BACKSPACE:
            del_char(&doc, &cursor);
            tui(&doc, argv[1], &cursor);
            is_saved = 0;
            break;
        case ENTER:
            int new_l = new_line(&doc, &cursor);
            if(new_l == 0) {
                tui(&doc, argv[1], &cursor);
            }
            is_saved = 0;
            break;
        case SAVE:
            if(save_to_file(argv[1], &doc) == 0) {
                is_saved = 1;
                message("Saved!", &cursor);
            }
            break;
        default:
            if(add_char(&doc, &cursor, ch) == 0) {
                tui(&doc, argv[1], &cursor);
                is_saved = 0;
            }
            break;
        }
    }
    
    clean_exit(&doc);
    disableRawMode();
    return 0;
}

