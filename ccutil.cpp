#include "ccutil.h"

#include <algorithm>
#include <cctype>
#include <errno.h>
#include <stdarg.h>

namespace utils{

//////////////////////////////////////////////////////////////////////////
// 字符串相关

template<typename STR, typename CH>
STR &TrimLeftCharT(STR &s, CH ch)
{
    size_t pe = 0;
    while (pe < s.length() && s.at(pe) == ch)
        pe ++;

    if (pe >= s.length())
        return s.erase(0, s.length());

    return s.erase(0, pe);
}

template<typename STR>
STR &TrimLeftStrT(STR &s, const STR &m)
{
    size_t pe = 0;
    while (pe < s.length() && m.find(s.at(pe)) != m.npos)
        pe ++;

    if (pe >= s.length())
        return s.erase(0, s.length());

    return s.erase(0, pe);
}

std::string &TrimLeft(std::string &s, char ch)
{
    return TrimLeftCharT(s, ch);
}

std::wstring &TrimLeft(std::wstring ws, wchar_t wc)
{
    return TrimLeftCharT(ws, wc);
}

std::string &TrimLeft(std::string &s, const std::string &m)
{
    return TrimLeftStrT(s, m);
}

std::wstring &TrimLeft(std::wstring &ws, const std::wstring &wm)
{
    return TrimLeftStrT(ws, wm);
}

template<typename STR, typename CH>
STR &TrimRightCharT(STR &s, CH ch)
{
    size_t pe = s.length() - 1;
    while (pe < s.length() && s.at(pe) == ch)
        pe --;

    if (pe >= s.length())
        return s.erase(0, s.length());

    return s.erase(pe+1, s.length()-1-pe);
}

template<typename STR>
STR &TrimRightStrT(STR &s, const STR &m)
{
    size_t pe = s.length() - 1;
    while (pe < s.length() && m.find(s.at(pe)) != m.npos)
        pe --;

    if (pe >= s.length())
        return s.erase(0, s.length());

    return s.erase(pe+1, s.length()-1-pe);
}

std::string &TrimRight(std::string &s, char ch)
{
    return TrimRightCharT(s, ch);
}

std::wstring &TrimRight(std::wstring ws, wchar_t wc)
{
    return TrimRightCharT(ws, wc);
}

std::string &TrimRight(std::string &s, const std::string &m)
{
    return TrimRightStrT(s, m);
}

std::wstring &TrimRight(std::wstring &ws, const std::wstring &wm)
{
    return TrimRightStrT(ws, wm);
}

template<typename STR, typename CH>
STR &TrimCharT(STR &s, CH c)
{
    TrimLeftCharT(s, c);

    return TrimRightCharT(s, c);
}

template<typename STR>
STR &TrimStrT(STR &s, const STR &m)
{
    TrimLeftStrT(s, m);

    return TrimRightStrT(s, m);
}

std::string &Trim(std::string &s, char c)
{
    return TrimCharT(s, c);
}

std::wstring &Trim(std::wstring &ws, wchar_t wc)
{
    return TrimCharT(ws, wc);
}

std::string &Trim(std::string &s, const std::string &m)
{
    return TrimStrT(s, m);
}

std::wstring &Trim(std::wstring &ws, const std::wstring &wm)
{
    return TrimStrT(ws, wm);
}

std::string &TrimWS(std::string &s)
{
    return Trim(s, " \r\n\t\v");
}

std::wstring &TrimWS(std::wstring &ws)
{
    return Trim(ws, L" \r\n\t\v");
}

template<typename charT>
struct char_ignore_case_equal {
    char_ignore_case_equal() {}
    bool operator() (charT ch1, charT ch2) {
        return std::toupper(ch1) == std::toupper(ch2);
    }
};

template<typename T>
size_t FindIgnoreCaseT(const T& str1, const T& str2, size_t start_pos)
{
    typename T::const_iterator it = std::search(
        str1.begin() + start_pos, str1.end(), 
        str2.begin(), str2.end(), 
        char_ignore_case_equal<typename T::value_type>());
    
    if (it != str1.end())
        return static_cast<size_t>(it - str1.begin());
    
    return std::string::npos;
}

size_t StrCaseStr(const std::string& haystack, const std::string& needle, size_t start_pos)
{
    return FindIgnoreCaseT(haystack, needle, start_pos);
}

size_t StrCaseStr(const std::wstring& haystack, const std::wstring& needle, size_t start_pos)
{
    return FindIgnoreCaseT(haystack, needle, start_pos);
}

template<typename STR>
STR &ReplaceT(STR &s, const STR &olds, const STR &news, bool ignore_case)
{
    if (!ignore_case) {
        size_t pos, pos0 = 0;
        while ((pos = s.find(olds, pos0)) != s.npos) {
            s.replace(pos, olds.length(), news);
            pos0 = pos + news.length();
        }
    } else {
        size_t pos, pos0 = 0;
        while ((pos = StrCaseStr(s, olds, pos0)) != s.npos) {
            s.replace(pos, olds.length(), news);
            pos0 = pos + news.length();
        }
    }

    return s;
}

std::string &Replace(std::string &s, const std::string &olds, const std::string &news, bool ignore_case)
{
    return ReplaceT(s, olds, news, ignore_case);
}

std::wstring &Replace(std::wstring &s, const std::wstring &olds, const std::wstring &news, bool ignore_case)
{
    return ReplaceT(s, olds, news, ignore_case);
}

template<typename STR, typename CH>
STR &EndWithCharT(STR &s, CH ch)
{
    if (!s.empty() && s.at(s.length() - 1) != ch)
        s += ch;

    return s;
}

template<typename STR>
STR &EndWithStrT(STR &s, const STR &m)
{
    if (!m.empty() 
        && s.length() >= m.length() 
        && s.substr(s.length() - m.length()) != m)
        s += m;

    return s;
}

std::string &EndWith(std::string &s, char c)
{
    return EndWithCharT(s, c);
}

std::wstring &EndWith(std::wstring &ws, wchar_t wc)
{
    return EndWithCharT(ws, wc);
}

std::string &EndWith(std::string &s, const std::string &m)
{
    return EndWithStrT(s, m);
}

std::wstring &EndWith(std::wstring &ws, const std::wstring &wm)
{
    return EndWithStrT(ws, wm);
}

template<typename STR, typename CH>
STR &StartWithCharT(STR &s, CH ch)
{
    if (!s.empty() && s.at(0) != ch)
        s.insert(0, 1, ch);

    return s;
}

template<typename STR>
STR &StartWithStrT(STR &s, const STR &m)
{
    if (!m.empty() 
        && s.length() >= m.length() 
        && s.substr(0, m.length()) != m)
        s.insert(0, m);

    return s;
}

std::string &StartWith(std::string &s, char c)
{
    return StartWithCharT(s, c);
}

std::wstring &StartWith(std::wstring &ws, wchar_t wc)
{
    return StartWithCharT(ws, wc);
}

std::string &StartWith(std::string &s, const std::string &m)
{
    return StartWithStrT(s, m);
}

std::wstring &StartWith(std::wstring &ws, const std::wstring &wm)
{
    return StartWithStrT(ws, wm);
}

// 转换字符串为全大/小写形式
std::string &UpperCase(std::string &s)
{
    std::transform(s.begin(), s.end(), s.begin(), toupper);
    return s;
}

std::wstring &UpperCase(std::wstring &ws)
{
    std::transform(ws.begin(), ws.end(), ws.begin(), towupper);
    return ws;
}

std::string &LowerCase(std::string &s)
{
    std::transform(s.begin(), s.end(), s.begin(), tolower);
    return s;
}

std::wstring &LowerCase(std::wstring &ws)
{
    std::transform(ws.begin(), ws.end(), ws.begin(), towlower);
    return ws;
}

// 见cutil的xasprintf函数
std::string FormatString(const char* format, ...)
{
    char stack_buf[512];
    int mem_length = sizeof(stack_buf);

    errno = 0;

    va_list ap;
    va_start (ap, format);
    int result = xvsnprintf(stack_buf, sizeof(stack_buf), format, ap);
    va_end(ap);

    if (result >= 0 && result < mem_length)
        return std::string(stack_buf, result);

    while (true) {
        if (result < 0)
        {
            if (errno != 0
#ifdef OS_WIN
                && errno != ERANGE
#else
                && errno != EOVERFLOW
#endif
                )
                return "";

            mem_length <<= 1;
        }
        else
            mem_length = result + 1;

        if (mem_length > 32 * 1024 * 1024)
            return "";

        std::vector<char> mem_buf(mem_length);
        errno = 0;

        va_list ap;
        va_start (ap, format);
        result = xvsnprintf(&mem_buf[0], mem_length, format, ap);
        va_end(ap);

        if (result >= 0 && result < mem_length)
            return std::string(&mem_buf[0], result);
    }
}

bool Split(const std::string &s, const std::string &sep, string_vec* vstr, bool allow_null)
{
    if(s.empty() || sep.empty())   
        return false;   

    std::string str = s;
    Trim(str, ' ');

    vstr->clear();

    size_t found = 0;   
    do {
        found = str.find(sep,0);
        if(found!=std::string::npos)   
        {   
            if(found != 0 || allow_null)
                vstr->push_back(str.substr(0,found));
            str = str.erase(0,found + sep.length());   
            TrimLeft(str,' '); 
        }   
        else  
        {
            vstr->push_back(str);
            break;
        }   
    } while(str.length());

    return true;  
}

// 转义HTML特殊字符
std::string EscapeHTML(const std::string &str)
{
    std::string s = str;

    Replace(s, "&",  "&amp;");
    Replace(s, "\"", "&quot;");
    Replace(s, "\'", "&apos;");
    Replace(s, "<",  "&lt;");
    Replace(s, ">",  "&gt;");

    return s;
}

//////////////////////////////////////////////////////////////////////////
// 字符编码

// 字符串是纯ASCII编码
bool IsASCII(const std::string &input)
{
    return is_ascii(input.c_str(), input.length()) == 1;
}

// 字符串是否是UTF-8编码
bool IsUTF8(const std::string &input)
{
    return is_utf8(input.c_str(), input.length()) == 1;
}

// 字符串是否是GB2312编码
bool IsGB2312(const std::string &input)
{
    return is_gb2312(input.c_str(), input.length()) == 1;
}

// 字符串是否是GBK编码的
bool IsGBK(const std::string &input)
{
    return is_gbk(input.c_str(), input.length()) == 1;
}

// 探测字符串的字符集(ASCII,UTF-8,GB2312,GBK,GB18030)
std::string GetCharset(const std::string &str, bool ascii)
{
    std::string s;
    char buf[MAX_CHARSET];
    
    if (!get_charset(str.c_str(), str.length(), buf, MAX_CHARSET, ascii ? 1 : 0))
        return "";
    
    s = buf;
    return s;
}

// 探测文件字符集
// 成功返回文件的字符集（如"UTF-8", "GBK")
// 失败返回空字符串
std::string GetFileCharset(const std::string &path, double &probability, int maxline)
{
    char buf[MAX_CHARSET];

    if (get_file_charset(path.c_str(), buf, MAX_CHARSET, &probability, maxline))
        return buf;
    
    return "";
}

// 获取UTF-8字符串的字符数
size_t UTF8Length(const std::string &utf8)
{
    return utf8_len(utf8.c_str());
}

// 按照最大字节数截取UTF-8字符串
std::string UTF8Trim(const std::string &utf8, size_t max_bytes)
{
    size_t len = utf8_trim(utf8.c_str(), NULL, max_bytes);
    return len > 0 ? utf8.substr(0, len) : "";
}

// 缩略UTF8字符串
// 失败返回空字符串
std::string& UTF8Abbr(std::string &utf8, size_t max_bytes, 
    size_t last_reserved_words)
{
    char *p = (char*)xmalloc(max_bytes + 1);

    if (utf8_abbr(utf8.c_str(), p, max_bytes, last_reserved_words) > 0)
        utf8 = p;
    else
        utf8.clear();
 
    xfree(p);
    return utf8;
}

// 将宽字符串转换为多字节字符串
std::string WstringTostring(const std::wstring& ws)
{
    std::string s;

    char *buf = wcs_to_mbcs(ws.c_str());
    if (!buf)
        return "";

    s = buf;
    xfree(buf);

    return s;
}

// 将多字节字符串转换为宽字符串
std::wstring stringToWstring(const std::string& s)
{
    std::wstring ws;

    wchar_t *buf = mbcs_to_wcs(s.c_str());
    if (!buf)
        return L"";

    ws = buf;
    xfree(buf);

    return ws;
}

// 宽字符串转换为UTF-8字符串
std::string WstringToUTF8string(const std::wstring &ws, bool bStrict)
{
    std::string utf8;

    char *s = wcs_to_utf8(ws.c_str(), bStrict ? 1 : 0);
    if (!s)
        return "";

    utf8 = s;
    xfree(s);

    return utf8;
}

// UTF-8字符串转换为宽字符串
std::wstring UTF8stringToWstring(const std::string &s, bool bStrict)
{
    std::wstring ws;

    wchar_t *w = utf8_to_wcs(s.c_str(), bStrict ? 1 : 0);
    if (!w)
        return L"";

    ws = w;
    xfree(w);

    return ws;
}

// 多字节字符串转换为UTF-8字符串
std::string stringToUTF8string(const std::string& s)
{
    return WstringToUTF8string(stringToWstring(s));
}

// UTF-8字符串转换为本地多字节字符串
std::string UTF8stringTostring(const std::string& s)
{
    return WstringTostring(UTF8stringToWstring(s));
}

std::string UTF7stringToUTF8string(const std::string &s7)
{
    UTF8 *utf8 = utf7_to_utf8((const UTF7*)s7.c_str());
    if (utf8)
    {
        std::string s8((char*)utf8);
        xfree(utf8);
        return s8;
    }

    return std::string();
}

std::string    UTF8stringToUTF7string(const std::string &s8)
{
    UTF7 *utf7 = utf8_to_utf7((const UTF8*)s8.c_str());
    if (utf7)
    {
        std::string s7((char*)utf7);
        xfree(utf7);
        return s7;
    }

    return std::string();
}

#ifdef _LIBICONV_H

// 返回一个转换过编码之后的字符串
std::string ConvertToCharset(const std::string &from, const std::string &to, const std::string &input, bool strict)
{
    char buf[1024], *output;
    size_t len;

    if (from == to)
        return input;

    output = buf;
    len = sizeof(buf);
    if (!convert_to_charset(from.c_str(), to.c_str(), input.c_str(), input.length(), &output, &len, (strict ? 1 : 0)))
        return "";

    std::string s(output, len);

    if (output != buf)
        xfree(output);
    
    return s;
}

#endif /* _LIBICONV_H */

//////////////////////////////////////////////////////////////////////////
// 文件系统

// 是否是绝对路径
bool IsAbsolutePath(const std::string &path)
{
    return is_absolute_path(path.c_str()) == 1;
}

// 是否是根路径（/或C:\）
bool IsRootPath(const std::string &path)
{
    return is_root_path(path.c_str()) == 1;
}

// 返回路径的文件名或最底层目录名
std::string PathFindFileName(const std::string &path)    
{
    return path_find_file_name(path.c_str());
}

// 返回文件的扩展名，目录返回NULL
std::string PathFindExtension(const std::string &path)
{
    return path_find_extension(path.c_str());
}

std::string PathFindDirectory(const std::string &path)
{
    char buf[MAX_PATH+1];
    std::string s;

    if (!path_find_directory(path.c_str(), buf, sizeof(buf)))
        return "";

    s = buf;
    return s;
}

bool PathFileExists(const std::string &path)
{
    return path_file_exists(path.c_str()) > 0;
}

bool PathIsFile(const std::string &path)
{
    return path_is_file(path.c_str()) > 0;
}

bool PathIsDirectory(const std::string &path)
{
    return path_is_directory(path.c_str()) > 0;
}

// 获取可用的文件路径
std::string& UniqueFile(std::string &path, bool create_now)
{
    char buf[MAX_PATH] = {0};

    if (unique_file(path.c_str(), buf, sizeof(buf), (create_now?1:0)))
        path = buf;

    return path;
}

// 获取可用的目录路径
std::string& UniqueDir(std::string &path, bool create_now)
{
    char buf[MAX_PATH] = {0};

    if (unique_dir(path.c_str(), buf, sizeof(buf), (create_now ? 1 : 0)))
        path = buf;

    return path;
}

// 将路径中的非法字符替换为%HH的形式
std::string PathEscape(const std::string &path, int platform, bool reserve_separator)
{
    std::string s;
    char *p = path_escape(path.c_str(), platform, (reserve_separator ? 1 : 0));
    if (!p)
        return "";

    s = p;
    xfree(p);

    return s;
}

// 将路径组成元素（目录名或文件名）合法化
// 注意必须是UTF-8编码！
std::string& PathComponentLegalize(std::string &component, int platform, size_t max_length)
{
    if (component.empty())
        return component;

    char* p = (char*)xstrdup(component.c_str()); 
    path_illegal_blankspace(p, platform, 0);
    component = p;
    xfree(p);

    utils::TrimWS(component);

    if (platform == PATH_WINDOWS) {
        // Windows会忽略末尾的'.'和' '
        utils::TrimRight(component, " .");
    }

    if (max_length)
        utils::UTF8Abbr(component, max_length);

    return component;
}

// 合法化路径名
// 使用当前编译时平台的路径分隔符
std::string& PathLegalize(std::string& path, int platform, size_t max_length)
{
    if (path.empty())
        return path;

    if (platform == PATH_WINDOWS) {
        if (max_length > MAX_PATH_WIN)
            max_length = MAX_PATH_WIN;
    } else if (max_length > MAX_PATH)
        max_length = MAX_PATH;

    bool prefix_slash = IS_PATH_SEP(path.at(0));
    bool suffix_slash = IS_PATH_SEP(path.at(path.length() - 1));

    std::vector<std::string> components;
    std::string component;

    Split(path, PATH_SEP_STR, &components, false);
    size_t ncomp = components.size();
    
    path.clear();
    if (prefix_slash)
        path += PATH_SEP_STR;

    for (size_t i = 0; i < ncomp; i++) {
        component = components[i];

        PathComponentLegalize(component, platform, (max_length - path.size()) / 2);
        if (component.empty())
            continue;

        path += component;

        if (i != ncomp - 1 || suffix_slash)
            path += PATH_SEP_STR;

        if (max_length && path.length() > max_length)
            break;
    }

    return path;
}

std::string AbsolutePath(const std::string& relative)
{
    char buf[MAX_PATH];
    if (!absolute_path(relative.c_str(), buf, MAX_PATH))
        return 0;

    return buf;
}

// 构造相对路径
std::string RelativePath(const std::string& src, const std::string& dst)
{
    char link[MAX_PATH+1];

    if (src.empty() || dst.empty())
        return "";

    if (!relative_path(src.c_str(), dst.c_str(), link, MAX_PATH+1))
        return "";

    return link;
}

bool CopyFile(const std::string &src, const std::string &dst, bool bOverWrite /* = false */)
{
    return copy_file(src.c_str(), dst.c_str(), bOverWrite ? 1: 0) == 1;
}

bool MoveFile(const std::string &src, const std::string &dst, bool bOverWrite /* = false */)
{
    return move_file(src.c_str(), dst.c_str(), bOverWrite ? 1: 0) == 1;
}

bool DeleteFile(const std::string &file)
{
    return delete_file(file.c_str()) == 1;
}

bool CreateDirectory(const std::string &path)
{
    return create_directory(path.c_str()) == 1;
}

bool CreateDirectories(const std::string &path)
{
    return create_directories(path.c_str()) == 1;
}

bool DeleteDirectory(const std::string &dir)
{
    return delete_directory(dir.c_str()) == 1;
}

bool DeleteEmptyDirectories(const std::string &dir)
{
    return delete_empty_directories(dir.c_str()) == 1;
}

bool DeleteDirectories(const std::string &dir, delete_dir_cb func, void *arg)
{
    return delete_directories(dir.c_str(), func, arg) == 1;
}

bool CopyDirectories(const std::string &srcdir, const std::string &dstdir,
                        copy_dir_cb func, void *arg)
{
    return copy_directories(srcdir.c_str(), dstdir.c_str(), func, arg) == 1;
}

int64_t FileSize(const std::string &path)
{
    return file_size(path.c_str());
}

std::string ReadFile(const std::string& path, size_t max_size /* = 0 */) 
{
    std::string content;
    file_mem* fm = read_file_mem(path.c_str(), max_size);
    if (!fm)
        return content;

    content.assign(fm->content, fm->length);

    free_file_mem(fm);
    return content;
}

bool WriteFile(const std::string& path, const std::string& content)
{
    return write_mem_file(path.c_str(), content.data(), content.length()) == 1;
}

// 获取可读性强的文件长度值，如3bytes, 10KB, 1.5MB, 3GB
std::string FileSizeReadable(int64_t size)
{
    std::string s;
    char buf[64];

    file_size_readable(size, buf, sizeof(buf));

    s = buf;
    return s;
}

//////////////////////////////////////////////////////////////////////////
// 更多

// 获取日期时间字符串，如"2012-06-06 16:07:32"
std::string DateTimeStr(time_t t)
{
    std::string s;
    s = datetime_str(t);

    return s;
}

// 获取当前时间字符串
std::string CurrentTime()
{
    return DateTimeStr(time(NULL));
}

// 获取日期字符串，如"2012-06-06"
std::string DateStr(time_t t)
{
    std::string s;
    s = date_str(t);

    return s;

}

// 获取时间字符串，如"16:07:32"
std::string TimeStr(time_t t)
{
    std::string s;
    s = time_str(t);

    return s;
}

// 获取可读性强的时间差字符串
std::string TimeSpanReadable(int64_t seconds, int cutoff)
{
    char buf[TIME_SPAN_BUFSIZE];

    time_span_readable(seconds, cutoff, buf, sizeof(buf));

    return buf;
}

std::string GetExecutePath()
{
    return get_execute_path();
}

std::string GetExecuteName()
{
    return get_execute_name();
}

// 获取进程起始目录（exe所在目录）
std::string GetExecuteDir()
{
    return get_execute_dir();
}

// 获取进程当前工作目录
std::string GetCurrentDir()
{
    return get_current_dir();
}

// 设置进程当前工作目录
bool SetCurrentDir(const std::string &dir)
{
    return set_current_dir(dir.c_str()) == 1;
}

// 获取当前用户的主目录
std::string GetHomeDir()
{
    return get_home_dir();
}

std::string GetAppDataDir()
{
    return get_app_data_dir();
}

std::string GetTempDir()
{
    return get_temp_dir();
}

std::string GetTempFile(const std::string& prefix)
{
    return get_temp_file(prefix.c_str());
}

// 创建进程
bool CreateProcess(const std::string& commandline, bool show, bool wait)
{
    process_t proc = process_create(commandline.c_str(), show ? 1 : 0);
    if (proc == INVALID_PROCESS)
        return false;

    if (wait)
        IGNORE_RESULT(process_wait(proc, INFINITE, NULL));

    return true;
}

#define NUM_TO_STR(t,N,f) \
    std::string N##ToStr(t i){\
        std::string s;\
        char buf[25];\
        snprintf(buf,sizeof(buf), f, i);\
        s = buf;\
        return s;\
    }\
    t StrTo##N(const std::string &s){\
        t i = 0;\
        if (!sscanf(s.c_str(), f, &i))\
            return 0;\
        return i;\
    }

// 字符<=>串数字
NUM_TO_STR(int, Int, "%d");
NUM_TO_STR(uint, UInt, "%u");
NUM_TO_STR(size_t, Size, "%" PRIuS);
NUM_TO_STR(int64_t, Int64, "%" PRId64);
NUM_TO_STR(uint64_t, UInt64, "%" PRIu64);
NUM_TO_STR(double, Double, "%lf");

#undef NUM_TO_STR

// 以十六进制的形式查看缓冲区的内容
std::string HexDump(const void *buf, int len)
{
    std::string s;
    char *p;

    p = hexdump(buf, len);
    if (!p)
        return "";

    s = p;
    xfree(p);

    return s;
}

//////////////////////////////////////////////////////////////////////////

ScopedWalkDir::ScopedWalkDir(const std::string& dir)
    : dir_(dir),
    ctx_(NULL) {
}

ScopedWalkDir::~ScopedWalkDir() {
    if (ctx_)
        walk_dir_end(ctx_);
}

bool ScopedWalkDir::Next() {
    if (!ctx_) {
        ctx_ = walk_dir_begin(dir_.c_str());
        if (!ctx_)
            return false;

        return true;
    }

    return walk_dir_next(ctx_) == 1;
}

std::string ScopedWalkDir::Name() {
    const char* name = walk_entry_name(ctx_);
    if (!name)
        return "";

    return name;
}

std::string ScopedWalkDir::Path() {
    char buf[MAX_PATH];
    if (!walk_entry_path(ctx_, buf, MAX_PATH))
        return "";

    return buf;
}

//////////////////////////////////////////////////////////////////////////
// 线程类

Thread::Thread(int id)
    :id_(id),
    thread_(INVALID_THREAD),
    once_(NULL),
    once_func_(NULL)
{
}

Thread::~Thread()
{
    Join();
}

static int 
THREAD_CALLTYPE thread_helper(void *arg)
{
  return ((Thread*)arg)->RunImpl();
}

bool Thread::Run()
{
    return uthread_create(&thread_, thread_helper, this, 0) == 1;
}

void Thread::SetOnce(thread_once_t *once, thread_once_func once_func)
{
    once_ = once;
    once_func_ = once_func;
}

int Thread::Join()
{
    int ret = -1;
    if (thread_ != INVALID_THREAD)
    {
        ret = uthread_join(thread_, NULL);
        thread_ = INVALID_THREAD;
    }

    return ret;
}

bool Thread::Once()
{
    if (!once_ || !once_func_)
        return false;

    return thread_once(once_, once_func_) == 1;
}

}; //namespace utils

//////////////////////////////////////////////////////////////////////////
// 内存调试

#if (defined DBG_MEM) && (!defined COMPILER_MSVC || _MSC_VER > MSVC6)

#ifdef DBG_MEM_RT

void memrt_msg_cpp(int error,
    const char* file, const char* func, int line,
    void* address, size_t size, int method)
{
    switch(error)
    {
    case MEMRTE_NOFREE:
        __memrt_printf("[MEMORY] {%s %s %d} Memory not deleted at %p size %u method %d.\n",
            file, func, line, address ,size, method);
        break;
    case MEMRTE_UNALLOCFREE:
        __memrt_printf("[MEMORY] {%s %s %d} Memory unexceptly deleted at %p.\n", 
            file, func, line, address);
        break;
    case MEMRTE_MISRELIEF:
        __memrt_printf("[MEMORY] {%s %s %d} Memory not reliefed properly at %p, use free() instead.\n", 
            file, func, line, address);
        break;
    default:
        break;
    }
}

#endif

void * operator new(size_t size, const char *file, const char *func, int line)
{
    void *ptr = ::operator new(size);

    if (!ptr)
        log_dprintf(LOG_FATAL, "{%s %s %d} New exhausted.\n", path_find_file_name(file), func, line);
    else
    {
#ifdef DBG_MEM_LOG
        log_dprintf(LOG_DEBUG, "{%s %s %d} New at %p size %" PRIuS ".\n", path_find_file_name(file), func, line, ptr, size);
#endif
#ifdef DBG_MEM_RT
        __memrt_alloc(MEMRT_NEW, ptr, size, file, func, line, NULL, memrt_msg_cpp);
#endif
        g_xalloc_count++;
    }

    return(ptr);
};

void* operator new[](size_t size, const char *file, const char *func, int line)
{
    void *ptr = ::operator new(size);

    if (!ptr)
        log_dprintf(LOG_FATAL, "{%s %s %d} New[] exhausted.\n", path_find_file_name(file), func, line);
    else
    {
#ifdef DBG_MEM_LOG
        log_dprintf(LOG_DEBUG, "{%s %s %d} New[] at %p size %" PRIuS ".\n", path_find_file_name(file), func, line, ptr, size);
#endif
#ifdef DBG_MEM_RT
        __memrt_alloc(MEMRT_NEW_ARRAY, ptr, size, file, func, line, NULL, memrt_msg_cpp);
#endif
        g_xalloc_count++;
    }

    return(ptr);
}

// 但是以下delete函数仅当对象构造函数抛出异常时才会调用
void operator delete(void* ptr, const char* file, const char* func, int line)
{
    log_dprintf(LOG_ERROR, "{%s %s %d} Thrown on initializing object at %p.\n", path_find_file_name(file), func, line, ptr);

    ::operator delete(ptr);
}

void operator delete[](void* pointer, const char* file, const char* func, int line)
{
    operator delete(pointer, file, func, line);
}

void operator delete(void* pointer, const std::nothrow_t&)
{
    operator delete(pointer, "Unknown","Unknown", 0);
}

void operator delete[](void* pointer, const std::nothrow_t&)
{
    operator delete(pointer, std::nothrow);
}

#endif // DBG_MEM

/*
 * vim: et ts=4 sw=4
 */
