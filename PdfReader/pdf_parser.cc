
#include "pdf_parser.h"

int pdfParseInt(FILE *file)
{
	bool neg = false;
	int i = 0;
	int ch = getc(file);
	bool hasDigit = false;

	if (ch == '-') {
		neg = true;
		ch = getc(file);
	}
	else if (ch == '+')
		ch = getc(file);
	while (ch >= '0' && ch <= '9') {
		hasDigit = true;
		i *= 10;
		i += ch - '0';
		ch = getc(file);
	}
	ungetc(ch, file);

	if (!hasDigit)
		return -1;

	if (neg)
		i *= -1;

	return i;
}

bool isPdfDelimiter(const char ch)
{
	switch (ch) {
	case '(':
	case ')':
	case '<':
	case '>':
	case '[':
	case ']':
	case '{':
	case '}':
	case '/':
	case '%':
		return true;
	default:
		return false;
	}
}

bool isLetter(char c)
{
	return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c == '@');
}

bool isWhiteSpace(const char ch) {
	return (ch == 0x20 || (ch >= 0x09 && ch <= 0x0d) || ch == 0x00);
}

void skipWhiteSpace(FILE* fp)
{
	int ch = 0;
	do {
		ch = getc(fp);
	} while (isWhiteSpace(ch));
	ungetc(ch, fp);
	return;
}
