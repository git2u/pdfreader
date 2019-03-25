
#ifndef __PDF_DOC_H
#define __PDF_DOC_H
#include <stdio.h>
#include "pdf_token.h"
#include "pdf_element.h"

class PdfDocument
{
public:
	PdfDocument();
	~PdfDocument();

	bool load(const char* name);
private:
	bool isValidFileHeader(FILE* fp);
	bool isValidFileTrailer(FILE* fp);
	bool parseEncryptObject(FILE* fp);
	bool parseXRef(FILE* fp, long offset);
	bool parseXRefTrailer(FILE* fp, long offset, PdfXRefTrailer& xrefTrailer);
	int  parseXRefTable(FILE* fp, long offset, PdfXRefTable& xrefTable);
	PDF_TOKEN nextToken(FILE* fp, PdfToken& token);
private:
	long m_nXRefOff;
	bool m_fPrev;
	int m_nFileSize;
	int m_nMajor;
	int m_nMinor;
	vector<PdfXRef> m_xrefs;
};
#endif