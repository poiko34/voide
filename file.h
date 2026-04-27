#pragma once
#include <stdio.h>
#include <errno.h>
#include "char.h"

int load_from_file(char* filename, Document* doc);
int save_to_file(const char* filename, const Document* doc);
void clean_exit(Document* doc);
int init_doc(Document* doc);