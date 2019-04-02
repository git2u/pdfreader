
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
	bool parseXRef(FILE* fp, long& nextOffset);
	bool parseXRefTrailer(FILE* fp, long trailerOffset, PdfXRefTrailer& xrefTrailer);
	int  parseXRefTable(FILE* fp, long xrefOffset, PdfXRefTable& xrefTable);
	PDF_TOKEN nextToken(FILE* fp, PdfToken& token);
	void printXRefTrailer(PdfXRefTable& pdfXRefTable, PdfXRefTrailer& xrefTrailer) const ;
	long findEncryptObjectOffset(PdfXRefTable& pdfXRefTable, PdfXRefTrailer& xrefTrailer) const;
private:
	long m_nXRefOff;
	int m_nFileSize;
	int m_nMajor;
	int m_nMinor;
	vector<PdfXRef> m_xrefs;
};
#endif