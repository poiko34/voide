#include <termios.h>
#include <unistd.h>
#include "init.h"

#define CHAR_CAPACITY 10

static struct termios orig_termios;

void enableRawMode() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    struct termios raw = orig_termios;

    // Включаем "сырой" ввод по полной:
    // IXON — отключает Ctrl+S и Ctrl+Q (управление потоком)
    raw.c_iflag &= ~(IXON | ICRNL); 
    
    // OPOST — отключает автоматическое добавление \r к \n при выводе
    raw.c_oflag &= ~(OPOST);

    // ISIG — отключает Ctrl+C и Ctrl+Z
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
}

void disableRawMode() {
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
}