// PdfReader.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "pdf_document.h"

int _tmain(int argc, _TCHAR* argv[])
{
	PdfDocument pdf;
	// Google C++ v4.45.pdf pdf_reference_1-7.pdf
	const char* name = "F:\\pdf_reference_1-7.pdf";
	pdf.load(name);
	return 0;
}

