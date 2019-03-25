
#ifndef __PDF_PARSER_H
#define __PDF_PARSER_H

#include <stdio.h>

int pdfParseInt(FILE* fp);
bool isLetter(char ch);
bool isWhiteSpace(const char ch);
bool isPdfDelimiter(const char ch);
void skipWhiteSpace(FILE* fp);

#endif