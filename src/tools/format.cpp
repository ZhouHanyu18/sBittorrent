#include "tools/format.h"

namespace tools
{
	namespace format
	{

		wstring AsciiToUnicode(const string& str) {
			// 预算-缓冲区中宽字节的长度    
			int unicodeLen = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, nullptr, 0);
			// 给指向缓冲区的指针变量分配内存    
			wchar_t *pUnicode = (wchar_t*)malloc(sizeof(wchar_t)*unicodeLen);
			// 开始向缓冲区转换字节    
			MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, pUnicode, unicodeLen);
			wstring ret_str = pUnicode;
			free(pUnicode);
			return ret_str;
		}
		string UnicodeToAscii(const wstring& wstr) {
			// 预算-缓冲区中多字节的长度    
			int ansiiLen = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
			// 给指向缓冲区的指针变量分配内存    
			char *pAssii = (char*)malloc(sizeof(char)*ansiiLen);
			// 开始向缓冲区转换字节    
			WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, pAssii, ansiiLen, nullptr, nullptr);
			string ret_str = pAssii;
			free(pAssii);
			return ret_str;
		}
		wstring Utf8ToUnicode(const string& str) {
			// 预算-缓冲区中宽字节的长度    
			int unicodeLen = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
			// 给指向缓冲区的指针变量分配内存    
			wchar_t *pUnicode = (wchar_t*)malloc(sizeof(wchar_t)*unicodeLen);
			// 开始向缓冲区转换字节    
			MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, pUnicode, unicodeLen);
			wstring ret_str = pUnicode;
			free(pUnicode);
			return ret_str;
		}
		string UnicodeToUtf8(const wstring& wstr) {
			// 预算-缓冲区中多字节的长度    
			int ansiiLen = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
			// 给指向缓冲区的指针变量分配内存    
			char *pAssii = (char*)malloc(sizeof(char)*ansiiLen);
			// 开始向缓冲区转换字节    
			WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, pAssii, ansiiLen, nullptr, nullptr);
			string ret_str = pAssii;
			free(pAssii);
			return ret_str;
		}
		string AsciiToUtf8(const string& str) {
			return UnicodeToUtf8(AsciiToUnicode(str));
		}
		string Utf8ToAscii(const string& str) {
			return UnicodeToAscii(Utf8ToUnicode(str));
		}
		
		// string 与 Int 互转  
		int StringToInt(const string& str) {
			return atoi(str.c_str());
		}
		string IntToString(int i) {
			char ch[1024];
			memset(ch, 0, 1024);
			sprintf_s(ch, sizeof(ch), "%d", i);
			return ch;
		}
		string IntToString(char i) {
			char ch[1024];
			memset(ch, 0, 1024);
			sprintf_s(ch, sizeof(ch), "%c", i);
			return ch;
		}
		string IntToString(double i) {
			char ch[1024];
			memset(ch, 0, 1024);
			sprintf_s(ch, sizeof(ch), "%f", i);
			return ch;
		}

	}
}