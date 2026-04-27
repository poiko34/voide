#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <termios.h>
#include <unistd.h>
#include "init.h"
#include "table.h"
#include "char.h"
#include "file.h"
#include "ui.h"

#define ARROW_UP 65
#define ARROW_DOWN 66
#define ARROW_RIGHT 67
#define ARROW_LEFT 68
#define BACKSPACE 127
#define ENTER 13
#define SAVE 19
#define TAB 9

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
    Tables tab;
    init_table(&tab);
    import_theme("theme.cfg", &tab);
    if (init_doc(&doc) != 0) {
        fprintf(stderr, "Fatal: memory allocation failed\n");
        clean_exit(&doc);
        free_table(&tab);
        disableRawMode();
        return 1;
    }

    {
        int status = load_from_file(argv[1], &doc);
        if(status == 2) {
            printf("\033[1;1H\x1b[J");
            tui(&doc ,argv[1], &cursor, &tab);
            message("File not founded", &cursor);
        } else if(status != 0) {
            perror(argv[1]);
            return 1;
        } else {
            printf("\033[1;1H\x1b[J");
            tui(&doc ,argv[1], &cursor, &tab);
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
                    tui(&doc, argv[1], &cursor, &tab);
                }
                if(next_next_ch == ARROW_DOWN) {
                    if(doc.count > cursor.point_y+1) {
                        if(doc.items[cursor.point_y + 1].len < cursor.point_x)
                            cursor.point_x = doc.items[cursor.point_y + 1].len;
                        cursor.point_y++;
                    }
                    tui(&doc, argv[1], &cursor, &tab);
                }
                if(next_next_ch == ARROW_RIGHT) {
                    if(doc.items[cursor.point_y].len > cursor.point_x) {
                        cursor.point_x++;
                    } else if (doc.count > cursor.point_y + 1) {
                        cursor.point_x = 0;
                        cursor.point_y++;
                    }
                    tui(&doc, argv[1], &cursor, &tab);
                }
                if(next_next_ch == ARROW_LEFT) {
                    if(cursor.point_x > 0) {
                        cursor.point_x--;
                    } else if(cursor.point_y > 0) {
                        cursor.point_x = doc.items[cursor.point_y - 1].len;
                        cursor.point_y--;
                    }
                    tui(&doc, argv[1], &cursor, &tab);
                }
            }
            break;
        case BACKSPACE:
            del_char(&doc, &cursor);
            tui(&doc, argv[1], &cursor, &tab);
            is_saved = 0;
            break;
        case ENTER:
            new_line(&doc, &cursor);
            tui(&doc, argv[1], &cursor, &tab);
            is_saved = 0;
            break;
        case SAVE:
            if(save_to_file(argv[1], &doc) == 0) {
                is_saved = 1;
                message("Saved!", &cursor);
            }
            break;
        case TAB:
            for(int i = 0; i < 4; i++)
                if(add_char(&doc, &cursor, 32) == 0) {
                    tui(&doc, argv[1], &cursor, &tab);
                    is_saved = 0;
                }
            break;
        default:
            if(add_char(&doc, &cursor, ch) == 0) {
                tui(&doc, argv[1], &cursor, &tab);
                is_saved = 0;
            }
            break;
        }
    }
    
    clean_exit(&doc);
    free_table(&tab);
    disableRawMode();
    return 0;
}