
#ifndef __PDF_TOKEN_H
#define __PDF_TOKEN_H

#include <string>
#include <unordered_map>

using std::string;
using std::unordered_map;

enum PDF_TOKEN
{
	PDF_TOKEN_NONE = 0,
	PDF_TOKEN_IDENTIFIER = 1,
	PDF_TOKEN_KEYWORD = 2,					// keyword
	PDF_TOKEN_NAME = 3,						// '/'
	PDF_TOKEN_LEFT_ANGLE_BRACKETS = 4,		// '<'
	PDF_TOKEN_RIGHT_ANGLE_BRACKETS = 5,		// '>'
	PDF_TOKEN_DOUBLE_LEFT_ANGLE_BRACKETS = 6,	// '<<'
	PDF_TOKEN_DOUBLE_RIGHT_ANGLE_BRACKETS = 7,	// '>>'
	PDF_TOKEN_LEFT_BRACKETS = 8,			// '['
	PDF_TOKEN_RIGHT_BRACKETS = 9,			// ']'
	PDF_TOKEN_NEWLINE_SEPARATE = 10,		//换行分隔符
	PDF_TOKEN_SPACE_SEPARATE = 11,			//空格分隔符
	PDF_TOKEN_NUM = 12,						//数字
	PDF_TOKEN_EOF = 98,
	PDF_TOKEN_BAD = 99,
};

enum PDF_STACK
{
	PDF_STACK_NONE = 0,
	PDF_STACK_ANGLE_BRACKETS = 1,			// <>
	PDF_STACK_DOUBLE_ANGLE_BRACKETS = 2,	// <<>>
	PDF_STACK_BRACKETS = 3,					// []
};

enum PDF_KEYWORD
{
	PDF_KEYWORD_NONE = 0,
	PDF_KEYWORD_TRAILER = 1,				// "trailer"
	PDF_KEYWORD_SIZE = 2,					// "Size"
	PDF_KEYWORD_ROOT = 3,					// "Root"
	PDF_KEYWORD_PREV = 4,					// "Prev"
	PDF_KEYWORD_ENCRYPT = 5,				// "Encrypt"
	PDF_KEYWORD_INFO = 6,					// "Info"
	PDF_KEYWORD_ID = 7,						// "ID"
	PDF_KEYWORD_XREFSTM = 8,				// "XRefStm"
};

const int kNumberOfKeywords = 7;
const unordered_map<string, int> keywords = {
	{ "trailer", PDF_KEYWORD_TRAILER },
	{ "Size", PDF_KEYWORD_SIZE },
	{ "Root", PDF_KEYWORD_ROOT },
	{ "Prev", PDF_KEYWORD_PREV },
	{ "Encrypt", PDF_KEYWORD_ENCRYPT },
	{ "Info", PDF_KEYWORD_INFO },
	{ "ID", PDF_KEYWORD_ID },
	{ "XRefStm", PDF_KEYWORD_XREFSTM },
};

struct PdfToken
{
	PDF_TOKEN token;
	PDF_KEYWORD keyword;
	std::string s;

	PdfToken()
		: token(PDF_TOKEN_NONE)
		, keyword(PDF_KEYWORD_NONE)
		, s() {	}

	void clear(){
		token = PDF_TOKEN_NONE;
		keyword = PDF_KEYWORD_NONE;
		s.clear();
	}
};



#endif