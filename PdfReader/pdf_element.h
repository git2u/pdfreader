
#ifndef __PDF_ELEMENT_H
#define __PDF_ELEMENT_H

#include <string>
#include <map>
#include <vector>

using std::map;
using std::vector;
using std::string;

struct PdfEncryptObject
{

};

typedef int PdfXRefInteger;
typedef vector<string> PdfXRefArray;
struct PdfXRefDictionary
{
	string strObj;
	string strIndex;
	char r;

	PdfXRefDictionary() { reset(); }
	void reset() {
		strObj.clear();
		strIndex.clear();
		r = '\0';
	}
	bool empty() {
		return (strObj.empty() || strIndex.empty());
	}
};

struct PdfXRefTrailer
{
	PdfXRefInteger offset;
	PdfXRefInteger size;
	PdfXRefInteger prev;
	PdfXRefInteger xrefstm;
	PdfXRefDictionary root;
	PdfXRefDictionary encrypt;
	PdfXRefDictionary info;
	PdfXRefArray id;

	PdfXRefTrailer() { reset(); }
	void reset() {
		size = 0;
		prev = 0;
		root.reset();
		encrypt.reset();
		info.reset();
		id.clear();
	}
};

struct PdfXRefItem
{
	string strOffset;			//偏移
	string strGenerationNum;	//代号 最大值65535
	char chInUse;
	PdfXRefItem() {
		reset();
	}
	void reset() {
		strOffset.clear();
		strGenerationNum.clear();
		chInUse = '\0';
	}
};

struct PdfXRefEntry
{
	int index;
	int count;
	vector<PdfXRefItem> entries;

	PdfXRefEntry() {
		reset();
	}
	void reset() {
		index = -1;
		count = -1;
		entries.clear();
	}
};

struct PdfXRefTable
{
	PdfXRefInteger xrefOffset;
	vector<PdfXRefEntry> vecEntries;

	PdfXRefTable(){ reset(); }
	void reset() { 
		xrefOffset = 0;
		vecEntries.clear(); 
	}
	bool empty() {
		return vecEntries.empty();
	}
};

struct PdfXRef
{
	PdfXRefTable xrefTable;
	vector<PdfXRefTrailer> trailer;
};

#endif