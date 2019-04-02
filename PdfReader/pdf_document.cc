
#include "pdf_document.h"
#include "pdf_parser.h"
#include <stdio.h>
#include <string.h>
#include <cctype>
#include <string>
#include <vector>

using std::vector;
PdfDocument::PdfDocument()
	: m_nMajor(0)
	, m_nMinor(0)
	, m_nFileSize(0)
	, m_nXRefOff(0)
{
}

PdfDocument::~PdfDocument()
{

}

#define SHORT_BUFFER_LENGTH 16
#define FILE_MIN_SIZE 1024
#define MAGIC_XREF 0x66657278
bool PdfDocument::load(const char* name)
{
	bool bres = false;
	FILE* fp = fopen(name, "rb");
	if (fp) {
		do {
			fseek(fp, 0, SEEK_END);
			int size = ftell(fp);
			if (size <= FILE_MIN_SIZE)
				break;
			this->m_nFileSize = size;
			if (!this->isValidFileHeader(fp))
				break;
			if (!this->isValidFileTrailer(fp))
				break;
			if (this->parseEncryptObject(fp))
				break;
			
		} while (0);
		fclose(fp);
	}
	return bres;
}

bool PdfDocument::isValidFileHeader(FILE* fp)
{
	//检查%PDF
	//FileHeader必须出现在文档前1024字节 %!PS-Adobe-N.n PDF-M.m
	bool bres = false;
	char buf[SHORT_BUFFER_LENGTH] = { 0 };
	fseek(fp, 0, SEEK_SET);
	fread(buf, 4, 1, fp);
	do {
		if (memcmp(buf, "%PDF", 4) != 0)
			break;
		if (getc(fp) != '-')
			break;
		int major = pdfParseInt(fp);
		if (getc(fp) != '.')
			break;
		int minor = pdfParseInt(fp);
		if (major <= 0)
			break;
		this->m_nMajor = major;
		this->m_nMinor = minor;
		bres = true;
	} while (0);
	return bres;
}

bool PdfDocument::isValidFileTrailer(FILE* fp)
{
	fseek(fp, -FILE_MIN_SIZE, SEEK_END);
	int pos0 = 0;
	int pos1 = 0;
	int state = 0;
	char c = 0;
	for (int n = 0; n < FILE_MIN_SIZE;) {
		switch (state) {
		case 0:
			c = getc(fp);
			n++;
			if (c == '\r')
				state = 1;
			else if (c == '\n')
				state = 2;
			break;
		case 1:
			c = getc(fp);
			n++;
			if (c == '\n')
				state = 2;
			break;
		case 2:
			pos0 = pos1;
			pos1 = ftell(fp);
			state = 0;
			break;
		default:
			break;
		}
	}

	if (this->m_nFileSize < pos1)
		return false;
	fseek(fp, pos1, SEEK_SET);
	if (getc(fp) != '%'
		|| getc(fp) != '%'
		|| getc(fp) != 'E'
		|| getc(fp) != 'O'
		|| getc(fp) != 'F'
		)
		return false;

	//查找最后一个startxref
	if (this->m_nFileSize < pos0)
		return false;
	fseek(fp, pos0, SEEK_SET);
	int nStartXrefOffset = pdfParseInt(fp);
	if (this->m_nFileSize < nStartXrefOffset)
		return false;
	this->m_nXRefOff = nStartXrefOffset;
	return true;
}

int PdfDocument::parseXRefTable(FILE* fp, long xrefOffset, PdfXRefTable& pdfXRefTable)
{
	fseek(fp, xrefOffset, SEEK_SET);
	PdfToken token;
	PDF_TOKEN type = PDF_TOKEN_NONE;
	int nres = 0;
	int state = 0;
	int internalState = 0;

	int nXrefIndex = 0;
	int nXrefCount = 0;

	pdfXRefTable.xrefOffset = xrefOffset;
	PdfXRefEntry pdfXRefEntry;
	PdfXRefItem pdfXRefItem;

	bool fstop = false;
	bool ferror = false;
	bool fSkipNext = true;
	type = nextToken(fp, token);
	for (; !fstop && !ferror;) {
		if (!fSkipNext) {
			type = nextToken(fp, token);
		}
		fSkipNext = false;
		switch (state) {
		case 0:
			if (PDF_TOKEN_NEWLINE_SEPARATE == type || PDF_TOKEN_SPACE_SEPARATE == type) {
				if (internalState == 0) {
					state = 0;
				}
				else if (internalState == 1) {
					internalState = 0;
					state = 1;
				}
			}
			else if (PDF_TOKEN_IDENTIFIER == type && token.s == "xref") {
				internalState++;
				//printf("xref\n");
			}
			else {
				state = -1;
			}
			break;
		case 1: // xref index count
			if (PDF_TOKEN_NEWLINE_SEPARATE == type || PDF_TOKEN_SPACE_SEPARATE == type) {
				if (internalState == 0 || internalState == 1) {
					state = 1;
				}
				else if (internalState == 2) {
					internalState = 0;
					state = 2;
				}
			}
			else if (PDF_TOKEN_NUM == type) {
				//printf("%s ", token.s.c_str());
				internalState++;
				if (internalState == 1) {
					pdfXRefEntry.reset();
					nXrefIndex = atoi(token.s.c_str());
					pdfXRefEntry.index = nXrefIndex;
				}
				else if (internalState == 2) {
					//printf("\n");
					nXrefCount = atoi(token.s.c_str());
					pdfXRefEntry.count = nXrefCount;
				}
			}
			else {
				state = -1;
			}
			break;
		case 2: //判断count个数 
			if (nXrefCount == 0)
				state = 6;
			else {
				internalState = 0;
				state = 3;
			}
			fSkipNext = true;
			break;
		case 3: // xref entry
			if (PDF_TOKEN_NEWLINE_SEPARATE == type || PDF_TOKEN_SPACE_SEPARATE == type) {
				if (internalState == 3) {
					internalState = 0;
					//printf("\n");
					nXrefCount--;
					if (nXrefCount == 0) {
						state = 6;
						pdfXRefTable.vecEntries.push_back(pdfXRefEntry);
					}
				}
				else if (internalState > 3) {
					state = -1;
				}
				else {
					state = 3;
				}
			}
			else if (PDF_TOKEN_NUM == type) {
				//printf("%s ", token.s.c_str());
				internalState++;
				if (internalState == 1) {
					pdfXRefItem.reset();
					pdfXRefItem.strOffset = token.s;
				}
				else if (internalState == 2) {
					pdfXRefItem.strGenerationNum = token.s;
				}
			}
			else if (PDF_TOKEN_IDENTIFIER == type) {
				if (internalState++ == 2) {
					if (token.s == "f" || token.s == "n") {
						pdfXRefItem.chInUse = token.s[0];
						pdfXRefEntry.entries.push_back(pdfXRefItem);
					}
				}
				else {
					state = -1;
				}
			}
			else {
				state = -1;
			}
			break;
		case 4:
			break;
		case 5:
			break;
		case 6: 
			if (PDF_TOKEN_NEWLINE_SEPARATE == type || PDF_TOKEN_SPACE_SEPARATE == type) {
				state = 6;
			}
			else if (PDF_TOKEN_KEYWORD == type) {
				if (PDF_KEYWORD_TRAILER == token.keyword) { //end of
					fstop = true;
					nres = ftell(fp) - 7;
				}
			}
			else if (PDF_TOKEN_NUM == type) { // next sub entry
				state = 1;
				internalState = 0;
				fSkipNext = true;
			}
			else {
				state = -1;
				fSkipNext = true;
			}
			break;
		case -1:
			ferror = true;
			fstop = true;
			pdfXRefTable.reset();
			break;
		default:
			break;
		}
	}
	return nres;
}

PDF_TOKEN PdfDocument::nextToken(FILE* fp, PdfToken& token)
{
	int state = 0;
	bool foundOne = false;
	bool fSkipNext = false;
	PDF_TOKEN type = PDF_TOKEN_NONE;
	string lex;
	token.clear();
	char ch = getc(fp);
	while (!foundOne) {
		switch (state) {
		case 0:
			if (isLetter(ch)) {
				lex.push_back(ch);
				state = 1;
				break;
			}
			else if (isdigit(ch)){
				lex.push_back(ch);
				state = 2;
				break;
			}
			switch (ch) {
			case '\n': state = 3; fSkipNext = true; break;
			case '\r': state = 3; break;
			case '<': state = 4; break;
			case '>': state = 5; break;
			case '/': state = 6; break;
			case '[': state = 7; break;
			case ']': state = 8; break;
			case ' ': state = 9; break;
			case -1:
				type = PDF_TOKEN_EOF;
				foundOne = true;
				break;
			default:
				type = PDF_TOKEN_BAD;
				foundOne = true;
				break;
			}
			break;
		case 1:
			if (isLetter(ch) || isdigit(ch)) {
				lex.push_back(ch);
				state = 1;
				break;
			}
			else {
				auto search = keywords.find(lex);
				if (search != keywords.end()) {
					foundOne = true;
					type = PDF_TOKEN_KEYWORD;
					token.s = lex;
					token.keyword = (PDF_KEYWORD)search->second;
				}
				if (!foundOne) {
					token.s = lex;
					type = PDF_TOKEN_IDENTIFIER;
					foundOne = true;
				}
				ungetc(ch, fp);
			}
			break;
		case 2:
			//数字
			if (isdigit(ch)){
				lex.push_back(ch);
				state = 2;
			}
			else if (isLetter(ch)) {
				lex.push_back(ch);
				state = 1;
			}
			else {
				type = PDF_TOKEN_NUM;
				foundOne = true;
				token.s = lex;
				ungetc(ch, fp);
			}
			break;
		case 3:
			//换行
			if (ch == '\n') {
				type = PDF_TOKEN_NEWLINE_SEPARATE;
			}
			else {
				type = PDF_TOKEN_BAD;
			}
			foundOne = true;
			break;
		case 4:
			if (ch == '<') {
				type = PDF_TOKEN_DOUBLE_LEFT_ANGLE_BRACKETS;
			}
			else {
				type = PDF_TOKEN_LEFT_ANGLE_BRACKETS;
				ungetc(ch, fp);
			}
			foundOne = true;
			break;
		case 5:
			if (ch == '>') {
				type = PDF_TOKEN_DOUBLE_RIGHT_ANGLE_BRACKETS;
			}
			else {
				type = PDF_TOKEN_RIGHT_ANGLE_BRACKETS;
				ungetc(ch, fp);
			}
			foundOne = true;
			break;
		case 6: //name
			if (isWhiteSpace(ch) || isPdfDelimiter(ch))  {
				auto search = keywords.find(lex);
				if (search != keywords.end()) {
					type = PDF_TOKEN_KEYWORD;
					token.keyword = (PDF_KEYWORD)search->second;
				}
				else {
					type = PDF_TOKEN_NAME;
				}
				token.s = lex;
				foundOne = true;
				ungetc(ch, fp);
			}
			else {
				lex.push_back(ch);
				state = 6;
			}
			break;
		case 7:
			ungetc(ch, fp);
			type = PDF_TOKEN_LEFT_BRACKETS;
			foundOne = true;
			break;
		case 8:
			ungetc(ch, fp);
			type = PDF_TOKEN_RIGHT_BRACKETS;
			foundOne = true;
			break;
		case 9:
			ungetc(ch, fp);
			type = PDF_TOKEN_SPACE_SEPARATE;
			foundOne = true;
			break;
		default:
			type = PDF_TOKEN_BAD;
			foundOne = true;
			break;
		}
		if (!foundOne && !fSkipNext) {
			ch = getc(fp);
		}
		fSkipNext = false;
	}
	token.token = type;
	return type;
}

bool PdfDocument::parseEncryptObject(FILE* fp)
{
	long nextXRefOffset = this->m_nXRefOff;
	do {
		if (!this->parseXRef(fp, nextXRefOffset))
			break;
	} while (nextXRefOffset != 0);
	return true;
}

void PdfDocument::printXRefTrailer(PdfXRefTable& xrefTable, PdfXRefTrailer& xrefTrailer) const
{
	printf("xref offset = %d\n", xrefTable.xrefOffset);
	printf("Size: %d\n", xrefTrailer.size);
	printf("Prev: %d\n", xrefTrailer.prev);
	printf("Root: %s %s %c\n", xrefTrailer.root.strObj.c_str(), xrefTrailer.root.strIndex.c_str(), xrefTrailer.root.r);
	printf("ID: %s %s\n", xrefTrailer.id[0].c_str(), xrefTrailer.id[1].c_str());
	printf("Encrypt: %s %s %c\n", xrefTrailer.encrypt.strObj.c_str(), xrefTrailer.encrypt.strIndex.c_str(), xrefTrailer.encrypt.r);
	printf("Info:%s %s %c\n", xrefTrailer.info.strObj.c_str(), xrefTrailer.info.strIndex.c_str(), xrefTrailer.info.r);
	printf("XRefStm: %d\n", xrefTrailer.xrefstm);
}

long PdfDocument::findEncryptObjectOffset(PdfXRefTable& xrefTable, PdfXRefTrailer& xrefTrailer) const
{
	int obj = atoi(xrefTrailer.encrypt.strObj.c_str());
	auto begiter = xrefTable.vecEntries.begin();
	auto enditer = xrefTable.vecEntries.end();
	int index = 0;
	for (; begiter != enditer; ++begiter) {
		if (begiter->count < 1)
			continue;
		if (begiter->index <= obj && obj < (begiter->count + begiter->index )) {
			index = obj - begiter->index;
			PdfXRefItem& item = begiter->entries[index];
			int x = 0;
		}
	}
	return 0;
}

bool PdfDocument::parseXRef(FILE* fp, long& nextOffset)
{
	long offset = nextOffset;
	nextOffset = 0;
	if (0 == offset) {
		return false;
	}

	PdfXRefTable xrefTable;
	PdfXRefTrailer xrefTrailer;
	int trailerOffset = this->parseXRefTable(fp, offset, xrefTable);
	if (0 == trailerOffset)
		return false;
	if (!this->parseXRefTrailer(fp, trailerOffset, xrefTrailer))
		return false;
	nextOffset = xrefTrailer.prev;
	this->printXRefTrailer(xrefTable, xrefTrailer);

	if (!xrefTrailer.encrypt.empty() && !xrefTable.empty()) {
		long encryptObjOffset = this->findEncryptObjectOffset(xrefTable, xrefTrailer);
		if (0 != encryptObjOffset) {
			printf("find encrypt obj offset!\n");
		}
	}
	return true;
}

bool PdfDocument::parseXRefTrailer(FILE* fp, long trailerOffset, PdfXRefTrailer& xrefTrailer)
{
	if (trailerOffset == 0)
		return false;
	fseek(fp, trailerOffset, SEEK_SET);
	PdfToken token;
	PDF_TOKEN type = PDF_TOKEN_NONE;
	std::vector<int> stackObject;
	std::vector<int> internalStackObject;
	int state = 0;
	int internalState = 0;

	bool fstop = false;
	bool ferror = false;
	bool fSkipNext = false;
	
	type = nextToken(fp, token);
	for (;!fstop;) {
		switch (state) {
		case 0:
			if (PDF_TOKEN_NEWLINE_SEPARATE == type || PDF_TOKEN_SPACE_SEPARATE == type)
				state = 0;
			else if (PDF_TOKEN_KEYWORD == type) {
				if (PDF_KEYWORD_TRAILER == token.keyword)
					state = 0;
				else {
					fSkipNext = true;
					state = 1;
				}
			}
			else if (PDF_TOKEN_DOUBLE_LEFT_ANGLE_BRACKETS == type) {
				state = 0;
				stackObject.push_back(PDF_STACK_DOUBLE_ANGLE_BRACKETS);
			}
			else if (PDF_TOKEN_DOUBLE_RIGHT_ANGLE_BRACKETS == type) {
				//结束
				stackObject.pop_back();
				if (stackObject.empty()) {
					fstop = true;
				}
				state = 0;
			}
			else
				state = -1;
			break;
		case 1:
			if (PDF_TOKEN_KEYWORD == type) {
				state = 2;
				fSkipNext = true;
			}
			else {
				state = -1;
			}
			break;
		case 2:
			switch (token.keyword) {
			case PDF_KEYWORD_SIZE:
				state = 3;
				break;
			case PDF_KEYWORD_ROOT:
				state = 4;
				internalState = 0;
				break;
			case PDF_KEYWORD_ID:
				state = 5;
				internalStackObject.clear();
				break;
			case PDF_KEYWORD_PREV:
				state = 6;
				break;
			case PDF_KEYWORD_ENCRYPT:
				state = 7;
				internalState = 0;
				break;
			case PDF_KEYWORD_INFO:
				state = 8;
				internalState = 0;
				break;
			case PDF_KEYWORD_XREFSTM:
				state = 9;
				break;
			default:
				state = -1;
				break;
			}
			break;
		case 3: //keyword size
			if (PDF_TOKEN_NEWLINE_SEPARATE == type || PDF_TOKEN_SPACE_SEPARATE == type)
				state = 3;
			else if (PDF_TOKEN_NUM == type) {
				state = 3;
				xrefTrailer.size = atoi(token.s.c_str());
			}
			else {
				fSkipNext = true;
				state = 0;
			}
			break;
		case 4: // keyword root
			if (PDF_TOKEN_NEWLINE_SEPARATE == type || PDF_TOKEN_SPACE_SEPARATE == type) {
				state = 4;
			}
			else if (PDF_TOKEN_NUM == type) {
				state = 4;
				internalState++;
				if (internalState == 1) {
					xrefTrailer.root.strObj = token.s;
				}
				else if (internalState == 2) {
					xrefTrailer.root.strIndex = token.s;
				}
			}
			else if (PDF_TOKEN_IDENTIFIER == type) {
				if (token.s == "R") {
					xrefTrailer.root.r = 'R';
					state = 0;
				}
			}
			else {
				fSkipNext = true;
				state = 0;
			}
			break;
		case 5: // id array
			switch (type) {
			case PDF_TOKEN_NEWLINE_SEPARATE:
			case PDF_TOKEN_SPACE_SEPARATE:
				state = 5;
				break;
			case PDF_TOKEN_LEFT_BRACKETS:
				state = 5;
				internalStackObject.push_back(PDF_STACK_BRACKETS);
				break;
			case PDF_TOKEN_RIGHT_BRACKETS:
				state = 5;
				internalStackObject.pop_back();
				break;
			case PDF_TOKEN_LEFT_ANGLE_BRACKETS:
				state = 5;
				internalStackObject.push_back(PDF_STACK_ANGLE_BRACKETS);
				break;
			case PDF_TOKEN_RIGHT_ANGLE_BRACKETS:
				state = 5;
				internalStackObject.pop_back();
				break;
			case PDF_TOKEN_IDENTIFIER:
				state = 5;
				xrefTrailer.id.push_back(token.s);
				break;
			default:
				if (!internalStackObject.empty())
					state = -1;
				else 
					state = 0;
				fSkipNext = true;
				break;
			}
			break;
		case 6: // prev
			if (PDF_TOKEN_NEWLINE_SEPARATE == type || PDF_TOKEN_SPACE_SEPARATE == type)
				state = 6;
			else if (PDF_TOKEN_NUM == type) {
				state = 6;
				xrefTrailer.prev = atoi(token.s.c_str());
			}
			else {
				state = 0;
				fSkipNext = true;
			}
			break;
		case 7: // encrypt
			if (PDF_TOKEN_NEWLINE_SEPARATE == type || PDF_TOKEN_SPACE_SEPARATE == type)
				state = 7;
			else if (PDF_TOKEN_NUM == type) {
				state = 7;
				internalState++;
				if (1 == internalState) {
					xrefTrailer.encrypt.strObj = token.s;
				}
				else if (2 == internalState) {
					xrefTrailer.encrypt.strIndex = token.s;
				}
			}
			else if (PDF_TOKEN_IDENTIFIER == type) {
				if (token.s == "R") {
					state = 0;
					xrefTrailer.encrypt.r = 'R';
				}
			}
			else {
				state = 0;
				fSkipNext = true;
			}
			break;
		case 8:	// info
			if (PDF_TOKEN_NEWLINE_SEPARATE == type || PDF_TOKEN_SPACE_SEPARATE == type)
				state = 8;
			else if (PDF_TOKEN_NUM == type) {
				state = 8;
				internalState++;
				if (internalState == 1) {
					xrefTrailer.info.strObj = token.s;
				}
				else if (internalState == 2) {
					xrefTrailer.info.strIndex = token.s;
				}
			}
			else if (PDF_TOKEN_IDENTIFIER == type) {
				if (token.s == "R") {
					state = 0;
					xrefTrailer.info.r = 'R';
				}
			}
			else {
				fSkipNext = true;
				state = 0;
			}
			break;
		case 9: // xrefstm
			if (PDF_TOKEN_NEWLINE_SEPARATE == type || PDF_TOKEN_SPACE_SEPARATE == type)
				state = 9;
			else if (PDF_TOKEN_NUM == type) {
				state = 9;
				int size = atoi(token.s.c_str());
				xrefTrailer.xrefstm = atoi(token.s.c_str());
			}
			else {
				fSkipNext = true;
				state = 0;
			}
			break;
		case -1:
			xrefTrailer.reset();
			ferror = true;
			fstop = true;
			break;
		default:
			state = -1;
			fSkipNext = true;
			break;
		}
		if (!fstop && !fSkipNext) {
			type = nextToken(fp, token);
		}
		fSkipNext = false;
	}
	if (ferror || !stackObject.empty())
		return false;
	return true;
}
