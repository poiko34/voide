#pragma once
#include <sys/ioctl.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include "char.h"
#include "table.h"

extern struct winsize ws;

void tui(Document* doc, const char* filename, Cursor* curs, Tables* tab);
void message(char* mess, Cursor* curs);