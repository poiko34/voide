#include "ui.h"
#include "char.h"
#include "table.h"

struct winsize ws;

void tui(Document* doc, const char* filename, Cursor* curs, Tables* tab) {
    char* name = "voide 2.0";
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
            if (visible_len > ws.ws_col) visible_len = ws.ws_col - 1;

            char word_buf[256]; 
            int buf_idx = 0;
            int is_string = 0;   // Для "..."
            int is_include = 0;  // Для <...>
            int last_word_was_define = 0; // Для розового макроса
            int expect_include_path = 0;

            for (int j = 0; j < visible_len; j++) {
                char ch = visible_part[j];

                // 1. Обработка кавычек ""
                if (ch == '"' && !is_include) {
                    is_string = !is_string;
                    printf("\033[38;2;100;200;100m%c", ch); // Зеленый
                    if (!is_string) printf("\033[0m"); 
                    continue;
                }
                if (is_string) {
                    putchar(ch);
                    continue;
                }

                // 2. Обработка угловых скобок <> (только если не в строке)
                if (ch == '<' && expect_include_path && !is_string) {
                    is_include = 1;
                    printf("\033[38;2;100;200;100m<"); 
                    continue;
                }
                if (ch == '>' && is_include) {
                    is_include = 0;
                    printf("%c\033[0m", ch);
                    continue;
                }
                if (is_include) {
                    putchar(ch);
                    continue;
                }

                // 3. Сборка слова
                if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '_') {
                    if (buf_idx < 255) word_buf[buf_idx++] = ch;
                } else {
                    if (buf_idx > 0) {
                        word_buf[buf_idx] = '\0';
                        unsigned char rgb[3];
                        
                        if (last_word_was_define) {
                            printf("\033[38;2;184;86;123m%s\033[0m", word_buf);
                            last_word_was_define = 0;
                        } 
                        else if (get_color(tab, word_buf, buf_idx, rgb) == 0) {
                            // Печатаем слово цветом из таблицы
                            printf("\033[38;2;%d;%d;%dm%s\033[0m", rgb[0], rgb[1], rgb[2], word_buf);
                            
                            // ПРОВЕРЯЕМ ТУТ ЖЕ: не было ли это спецсловом?
                            if (strcmp(word_buf, "define") == 0) {
                                last_word_was_define = 1;
                            }
                            if (strcmp(word_buf, "include") == 0) {
                                expect_include_path = 1;
                            }
                        } 
                        else {
                            printf("%s", word_buf);
                        }
                        buf_idx = 0;
                    }
                    
                    // Красим саму решетку в серый, если это начало директивы
                    if (ch == '#') printf("\033[38;2;160;160;160m#\033[0m");
                    else {
                        char single_char[2] = {ch, '\0'};
                        unsigned char symbol_rgb[3];
                        
                        if (ch == '#') {
                            printf("\033[38;2;160;160;160m#\033[0m");
                        } 
                        else if (get_color(tab, single_char, 1, symbol_rgb) == 0) {
                            printf("\033[38;2;%d;%d;%dm%c\033[0m", symbol_rgb[0], symbol_rgb[1], symbol_rgb[2], ch);
                        } 
                        else {
                            putchar(ch);
                        }
                    }
                }
            }

            // Хвост строки
            if (buf_idx > 0) {
                word_buf[buf_idx] = '\0';
                unsigned char rgb[3];
                if (last_word_was_define) {
                    printf("\033[38;2;184;86;123m%s\033[0m", word_buf);
                } else if (get_color(tab, word_buf, buf_idx, rgb) == 0) {
                    printf("\033[38;2;%d;%d;%dm%s\033[0m", rgb[0], rgb[1], rgb[2], word_buf);
                } else {
                    printf("%s", word_buf);
                }
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