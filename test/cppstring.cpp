#include "cppstring.h"
#include "../ccutil.h"
#include <iostream>
#include <string>

using namespace std;
using namespace ccutil;
using namespace ccutil::charset;

void wstring_string()
{
    string s = UTF8stringTostring("中文UTF-8编码");
    cout << s <<endl;

    wstring ws = stringToWstring(s);
    CU_ASSERT_TRUE(!ws.empty());
    
    string s1 = WstringTostring(ws);
    CU_ASSERT_TRUE(s == s1);
    cout << s1 <<endl;
}

void wstring_utf8string()
{
    string s8 = "中文UTF-8编码";
    
#define HEX_UTF8 "E4 B8 AD E6 96 87 55 54 46 2D 38 E7 BC 96 E7 A0 81 "
#define HEX_UTF16 "2D 4E 87 65 55 00 54 00 46 00 2D 00 38 00 16 7F 01 78 "
#define HEX_UTF32 "2D 4E 00 00 87 65 00 00 55 00 00 00 54 00 00 00 46 00 00 00 2D 00 00 00 38 00 00 00 16 7F 00 00 01 78 00 00 "
#define HEX_GBK "D6 D0 CE C4 55 54 46 2D 38 B1 E0 C2 EB "
//#define HEX_UTF7 "2B 54 69 2B 5A 59 55 54 46 2D 38 2B 66 78 2B 65 41 "

    string hex = HexDump(s8.c_str(), s8.length());
    CU_ASSERT_TRUE(hex == HEX_UTF8);

    wstring ws = UTF8stringToWstring(s8);
    hex = HexDump(ws.c_str(), ws.length()*sizeof(wchar_t));
#ifdef _WIN32
    CU_ASSERT_TRUE(hex == HEX_UTF16);
#else
    CU_ASSERT_TRUE(hex == HEX_UTF32);
#endif

    s8 = WstringToUTF8string(ws);
    hex = HexDump(s8.c_str(), s8.length());
    CU_ASSERT_TRUE(hex == HEX_UTF8);
}

void string_utf8string() {
    string s8 = "中文UTF-8编码";

    string sloc = UTF8stringTostring(s8);
    cout << sloc << endl;

    s8 = stringToUTF8string(sloc);
    string hex = HexDump(s8.c_str(), s8.length());
    CU_ASSERT_TRUE(hex == HEX_UTF8);
}

void string16_utf8string()
{
    string s8 = "中文UTF-8编码";

    string16 s16 = UTF8stringTostring16(s8);
    string hex = HexDump(s16.c_str(), s16.length() * 2);
    CU_ASSERT_TRUE(hex == HEX_UTF16);

    s8 = string16ToUTF8string(s16);
    hex = HexDump(s8.c_str(), s8.length());
    CU_ASSERT_TRUE(hex == HEX_UTF8);
}

void string16_wstring()
{
    string s8 = "中文UTF-8编码";
    wstring ws = UTF8stringToWstring(s8);
    
    string16 s16 = WstringTostring16(ws);
    string hex = HexDump(s16.c_str(), s16.length() * 2);
    CU_ASSERT_TRUE(hex == HEX_UTF16);

    ws = string16ToWstring(s16);
    hex = HexDump(ws.c_str(), ws.length()*sizeof(wchar_t));
#ifdef _WIN32
    CU_ASSERT_TRUE(hex == HEX_UTF16);
#else
    CU_ASSERT_TRUE(hex == HEX_UTF32);
#endif
}

void string16_string()
{
    string s8 = "中文UTF-8编码";

    string s = UTF8stringTostring(s8);
    cout << s << endl;

    string16 s16 = stringTostring16(s);
    string hex = HexDump(s16.c_str(), s16.length() * 2);
    CU_ASSERT_TRUE(hex == HEX_UTF16);

    s = string16Tostring(s16);
    cout << s << endl;
}

void string_conversions()
{
    string s8 = "中文UTF-8编码";

    string gbk = ConvertToCharset("UTF-8", "GBK", s8);
    string hex = HexDump(gbk.c_str(), gbk.length());
    CU_ASSERT_TRUE(hex == HEX_GBK);

    string u16 = ConvertToCharset("GBK", "UTF-16LE", gbk);
    hex = HexDump(u16.c_str(), u16.length());
    CU_ASSERT_TRUE(hex == HEX_UTF16);

    string u32 = ConvertToCharset("UTF-16LE", "UTF-32LE", u16);
    hex = HexDump(u32.c_str(), u32.length());
    CU_ASSERT_TRUE(hex == HEX_UTF32);

    gbk = ConvertToCharset("UTF-32LE", "GBK", u32);
    hex = HexDump(gbk.c_str(), gbk.length());
    CU_ASSERT_TRUE(hex == HEX_GBK);
}
