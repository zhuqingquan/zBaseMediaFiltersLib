#pragma once
#ifndef _ZUTILS_TEXT_HELPER_H_
#define _ZUTILS_TEXT_HELPER_H_

#include <string>
#include <vector>

namespace zUtils
{
	using std::string;
	using std::wstring;
	using std::vector;

	vector<wstring> split(const wstring &str, const wstring &delimiter);
	wstring format(wstring fmt, ...);
	string format(string fmt, ...);

	wstring Int64ToWString(__int64 v);
	__int64 wstringToInt64(const wstring& str);
	wstring IntToWString(int v);
	int wstringToInt(const wstring& str);
	wstring UIntToWString(unsigned int v);
	//��wstringת��Ϊunsinged int,�˷����Ὣ�ַ���"12345ddd"ת��Ϊ����12345����ʱ��ʹ����һ��wstringToUInt����
	unsigned int wstringToUInt(const wstring& str);
	//��wstringת��Ϊunsinged int���˷���ת���ַ���"12345ddd"ʱ������ʧ��false
	bool wstringToUInt(const wstring& str, unsigned int* outResult);

	wstring string2wstring(const string &str);
	string wstring2string(const wstring &wstr);
	string UTF8_To_string(const string & str);
	string string_To_UTF8(const string & str);
	wstring UTF8_To_wstring(const string & str);
	string wstring_To_UTF8(const wstring & str);

	bool contains(const wstring& str, const wstring& substr);
	bool startWith(const wstring& str, const wstring& with);
	bool endWith(const wstring& str, const wstring& with);
	wstring trim(const wstring& str);
	wstring trimLeft(const wstring& str, const wstring& trimstr);
	wstring trimRight(const wstring& str, const wstring& trimstr);

	wstring toUpper(wstring str);
	wstring toLower(wstring str);

	wstring replace(const wstring& str, const wstring& src, const wstring& dest);

	wstring startWString(const wstring& str, const int num);
	wstring endWString(const wstring& str, const int num);

	wstring readFromFile(wstring path);
	void writeToFile(wstring str, wstring path);

	string string_to_URLEncode2(const string& str);

	char **strlist_split(const char *str, char split_ch, bool include_empty);
	void strlist_free(char **strlist);
	int astrcmpi(const char *str1, const char *str2);

}

#endif//_ZUTILS_TEXT_HELPER_H_