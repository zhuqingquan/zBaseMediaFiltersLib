// libtext.cpp : Defines the exported functions for the DLL application.
//

#include <stdio.h>
#include <stdarg.h>
#include <algorithm>  
#include <functional>
#include <iostream>
#include <fstream>
#include <Windows.h>
#include "TextHelper.h"

using namespace std;

namespace zUtils
{

vector<wstring> split(const wstring &str, const wstring &delimiter)
{
	vector<wstring> ret;
	size_t last = 0;
	size_t index = str.find_first_of(delimiter,last);
	while ( index != string::npos )
	{
		ret.push_back(str.substr(last,index-last));
		last = index + 1;
		index = str.find_first_of(delimiter,last);
	}
	if ( index - last > 0 )
	{
		ret.push_back(str.substr(last ,index - last));
	}
	return ret;
}

wstring format(wstring fmt, ...)
{
	wchar_t buffer[4096] = {0};

	va_list valist;
	va_start(valist, fmt);
	_vsnwprintf(buffer, sizeof(buffer)/sizeof(wchar_t), fmt.c_str(), valist);
	va_end(valist);

	return wstring(buffer);
}

string format(string fmt, ...)
{
	char buffer[4096] = {0};

	va_list valist;
	va_start(valist, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt.c_str(), valist);
	va_end(valist);

	return string(buffer);
}

wstring Int64ToWString(__int64 v)
{
	wchar_t buf[128] = {0};
	swprintf(buf, 128, L"%lld", v);
	return wstring(buf);
}

__int64 wstringToInt64(const wstring& str)
{
	__int64 v = 0;
	swscanf(str.c_str(), L"%lld", &v);
	return v;
}

wstring IntToWString(int v)
{
	wchar_t buf[128] = {0};
	swprintf(buf, 128, L"%d", v);
	return wstring(buf);
}

wstring UIntToWString(unsigned int v)
{
	wchar_t buf[128] = {0};
	swprintf(buf, 128, L"%u", v);
	return wstring(buf);
}

unsigned int wstringToUInt(const wstring& str)
{
	unsigned int v = 0;
	swscanf(str.c_str(), L"%u", &v);
	return v;
}

 bool wstringToUInt( const wstring& str, unsigned int* outResult )
{
	if(str.size()>=1024)
	{
		return false;
	}
	unsigned int v = 0;
	wchar_t tail[1024] = {0};
	int varc = swscanf(str.c_str(), L"%u%1024s", &v, tail);
	if(varc==1)
	{
		*outResult = v;
		return true;
	}
	return false;
}

int wstringToInt(const wstring& str)
{
	int v = 0;
	swscanf(str.c_str(), L"%d", &v);
	return v;
}

wstring string2wstring(const string &str)
{
	int n = MultiByteToWideChar(CP_ACP, 0, (LPCSTR)str.c_str(), -1, (LPWSTR)NULL, 0);
	wstring wstr;
	wstr.resize(n-1,0);
	MultiByteToWideChar(CP_ACP, 0, (LPCSTR)str.c_str(), -1, (LPWSTR)wstr.c_str(), n);
	return wstr;
}

string wstring2string(const wstring &wstr)
{
	int n = WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)wstr.c_str(), -1, (LPSTR)NULL, 0, NULL, NULL);
	string str;
	str.resize(n-1,0);
	WideCharToMultiByte(CP_ACP,0,(LPCWSTR)wstr.c_str(),-1,(LPSTR)str.c_str(),n,NULL,NULL);
	return str;
}

string UTF8_To_string(const string & str)
{
	int nwLen = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);

	wchar_t * pwBuf = new wchar_t[nwLen + 1];//һ��Ҫ��1����Ȼ�����β��
	memset(pwBuf, 0, nwLen * 2 + 2);

	MultiByteToWideChar(CP_UTF8, 0, str.c_str(), str.length(), pwBuf, nwLen);

	int nLen = WideCharToMultiByte(CP_ACP, 0, pwBuf, -1, NULL, NULL, NULL, NULL);

	char * pBuf = new char[nLen + 1];
	memset(pBuf, 0, nLen + 1);

	WideCharToMultiByte(CP_ACP, 0, pwBuf, nwLen, pBuf, nLen, NULL, NULL);

	string retStr = pBuf;

	delete []pBuf;
	delete []pwBuf;

	pBuf = NULL;
	pwBuf = NULL;

	return retStr;
} 

string string_To_UTF8(const string & str)
{
	int nwLen = ::MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, NULL, 0);

	wchar_t * pwBuf = new wchar_t[nwLen + 1];//һ��Ҫ��1����Ȼ�����β��
	ZeroMemory(pwBuf, nwLen * 2 + 2);

	::MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.length(), pwBuf, nwLen);

	int nLen = ::WideCharToMultiByte(CP_UTF8, 0, pwBuf, -1, NULL, NULL, NULL, NULL);

	char * pBuf = new char[nLen + 1];
	ZeroMemory(pBuf, nLen + 1);

	::WideCharToMultiByte(CP_UTF8, 0, pwBuf, nwLen, pBuf, nLen, NULL, NULL);

	string retStr(pBuf);

	delete []pwBuf;
	delete []pBuf;

	pwBuf = NULL;
	pBuf  = NULL;

	return retStr;
} 

string wstring_To_UTF8(const wstring & str)
{
	int nwLen = ::WideCharToMultiByte(CP_UTF8, 0, str.c_str(), -1, NULL, 0, NULL, NULL);
	char * pBuf = new char[nwLen + 1];
	ZeroMemory(pBuf, nwLen + 1);

	::WideCharToMultiByte(CP_UTF8, 0, str.c_str(), nwLen, pBuf, nwLen, NULL, NULL);

	string retStr(pBuf);
	delete []pBuf;
	pBuf  = NULL;
	return retStr;
} 

wstring UTF8_To_wstring(const string & str)
{
	int nwLen = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);

	wchar_t * pwBuf = new wchar_t[nwLen + 1];//һ��Ҫ��1����Ȼ�����β��
	memset(pwBuf, 0, nwLen * 2 + 2);

	MultiByteToWideChar(CP_UTF8, 0, str.c_str(), str.length(), pwBuf, nwLen);

	wstring retStr = pwBuf;

	delete []pwBuf;

	pwBuf = NULL;

	return retStr;
} 

bool contains(const wstring& str, const wstring& substr)
{
	std::size_t found = str.find(substr);
	if (found==std::string::npos)
		return false;
	else
		return true;
}

bool startWith(const wstring& str, const wstring& with)
{
	return str.compare(0, with.size(), with)==0 ? true : false; 
}

bool endWith(const wstring& str, const wstring& with)
{
	if(str.size()<with.size())
		return false;
	return str.compare(str.size()-with.size(), with.size(), with)==0 ? true : false;  ;
}

wstring trim(const wstring& str)
{
	wstring ret = str;
	ret.erase(0, ret.find_first_not_of(' '));
	ret.erase(ret.find_last_not_of(' ')+1);
	return ret;
}

wstring trimLeft(const wstring& str, const wstring& trimstr)
{
	wstring ret = str;
	ret.erase(0, ret.find_first_not_of(trimstr.c_str()));
	return ret;
}

wstring trimRight(const wstring& str, const wstring& trimstr)
{
	wstring ret = str;
	ret.erase(ret.find_last_not_of(trimstr.c_str())+1);
	return ret;
}

wstring toUpper(wstring str)
{
	transform(str.begin(),str.end(),str.begin(),::toupper);
	return str;
}

wstring toLower(wstring str)
{
	transform(str.begin(),str.end(),str.begin(),::tolower);
	return str;
}

wstring replace(const wstring& str, const wstring& search, const wstring& replace)
{
	wstring subject = str;
	size_t pos = 0;
    while ((pos = subject.find(search, pos)) != std::string::npos) {
         subject.replace(pos, search.length(), replace);
         pos += replace.length();
    }
    return subject;

}

wstring startWString(const wstring& str, const int num)
{
	wstring tmp = L"";
	if ((int)str.length() > num)
	{
		tmp.append(str.begin(), str.begin() + num);
	}
	else
	{
		tmp.append(str.begin(), str.end());
	}
	return tmp;
}

wstring endWString(const wstring& str, const int num)
{
	wstring tmp = L"";
	if ((int)str.length() > num)
	{
		tmp.append(str.end() - num, str.end());
	}
	else
	{
		tmp.append(str.begin(), str.end());
	}
	return tmp;
}

wstring readFromFile(wstring path)
{
	string str;
	//ifstream fs(libtext::wstring2string(path).c_str());
	ifstream fs(path.c_str());
	int a = fs.get();
	int b = fs.get();
	int c = fs.get();
	if(a==0xEF && b==0xBB && c==0xBF)
	{
		str = std::string((std::istreambuf_iterator<char>(fs)), (std::istreambuf_iterator<char>()));
	}
	wstring ret = zUtils::UTF8_To_wstring(str);
	return ret;
}

void writeToFile(wstring str, wstring path)
{
	//ofstream fs(libtext::wstring2string(path).c_str());
	ofstream fs(path.c_str());
	unsigned char bom[] = { 0xEF,0xBB,0xBF };
    fs.write((char*)bom, sizeof(bom));
	string text = zUtils::wstring_To_UTF8(str);
	fs.write(text.c_str(), text.size());
	fs.close();
}

string string_to_URLEncode2(const string& str)
{
	//����Ҫ�󣬱�����ת��Ϊutf8����urlencode
	string utfstr = zUtils::string_To_UTF8(str);

	static char hex[] = "0123456789ABCDEF";  
	string dst;  

	for (size_t i = 0; i < utfstr.size() && utfstr[i] != 0; i++)  
	{  
		unsigned char ch = utfstr[i];  
		if (isalnum(ch) || ch == '.')  
		{  
			dst += ch;  
		}  
		else
		{  
			unsigned char c = static_cast<unsigned char>(utfstr[i]);  
			dst += '%';  
			dst += hex[c >> 4];  
			dst += hex[c % 16];  
		}  
	}  
	return dst;
}

char **strlist_split(const char *str, char split_ch, bool include_empty)
{
	const char *cur_str = str;
	const char *next_str;
	char *      out = NULL;
	size_t      count = 0;
	size_t      total_size = 0;

	if (str) {
		char **table;
		char *offset;
		size_t cur_idx = 0;
		size_t cur_pos = 0;

		next_str = strchr(str, split_ch);

		while (next_str) {
			size_t size = next_str - cur_str;

			if (size || include_empty) {
				++count;
				total_size += size + 1;
			}

			cur_str = next_str + 1;
			next_str = strchr(cur_str, split_ch);
		}

		if (*cur_str || include_empty) {
			++count;
			total_size += strlen(cur_str) + 1;
		}

		/* ------------------ */

		cur_pos    = (count + 1) * sizeof(char *);
		total_size += cur_pos;
		out        = (char*)malloc(total_size);
		offset     = out + cur_pos;
		table      = (char **)out;

		/* ------------------ */

		next_str = strchr(str, split_ch);
		cur_str  = str;

		while (next_str) {
			size_t size = next_str - cur_str;

			if (size || include_empty) {
				table[cur_idx++] = offset;
				strncpy(offset, cur_str, size);
				offset[size] = 0;
				offset += size + 1;
			}

			cur_str = next_str + 1;
			next_str = strchr(cur_str, split_ch);
		}

		if (*cur_str || include_empty) {
			table[cur_idx++] = offset;
			strcpy(offset, cur_str);
		}

		table[cur_idx] = NULL;
	}

	return (char**)out;
}

void strlist_free(char **strlist)
{
	free(strlist);
}

static const char *astrblank = "";
static const wchar_t *wstrblank = L"";

int astrcmpi(const char *str1, const char *str2)
{
	if (!str1)
		str1 = astrblank;
	if (!str2)
		str2 = astrblank;

	do {
		char ch1 = (char)toupper(*str1);
		char ch2 = (char)toupper(*str2);

		if (ch1 < ch2)
			return -1;
		else if (ch1 > ch2)
			return 1;
	} while (*str1++ && *str2++);

	return 0;
}
}
