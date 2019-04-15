#ifndef FORMAT_H
#define FORMAT_H

#include <iostream>  
#include <string>
#include <Windows.h>

using namespace std;


namespace tools
{
	namespace format
	{
		//**************string******************//  
		// ASCII��Unicode��ת  
		wstring AsciiToUnicode(const string& str);
		string  UnicodeToAscii(const wstring& wstr);
		// UTF8��Unicode��ת  
		wstring Utf8ToUnicode(const string& str);
		string  UnicodeToUtf8(const wstring& wstr);
		// ASCII��UTF8��ת  
		string  AsciiToUtf8(const string& str);
		string  Utf8ToAscii(const string& str);
		
		/************string-int***************/
		// string ת Int  
		int StringToInt(const string& str);
		string IntToString(int i);
		string IntToString(char i);
		string IntToString(double i);
	}
}

#endif //FORMAT_H