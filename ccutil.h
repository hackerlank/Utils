/**
 *
 * @file: ccutil.h
 * @desc: C++工具函数库
 *		  许多函数仅仅是相应c函数的封装
 * @auth: liwei (www.leewei.org)
 * @mail: ari.feng@qq.com
 * @date: 2012/4/06
 * @mdfy: 2012/12/21
 *
 */

#ifndef UTILS_CCUTIL_H
#define UTILS_CCUTIL_H

#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <list>

#include "utils/cutil.h"

#define NPOS std::string::npos

#define STL_CONTAINS(container, key) \
	(container.find(key) != container.end())

//注：g++下使用此宏需手动开启C++11支持
//即添加-std=c++11或-std=gnu++11命令行选项(前者禁用了GNU扩展)
#define STL_FOR_EACH(iter, container) \
	for (auto iter = (container).begin(); iter != (container).end(); ++iter)

#define STL_REVERSE_FOR_EACH(iter, container) \
	for (auto iter = (container).rbegin(); iter != (container).rend(); ++iter)

namespace utils {

//类型定义
typedef std::list<std::string> string_list;
typedef std::vector<std::string> string_vec;

//////////////////////////////////////////////////////////////////////////
//字符串

//去除左边的字符或字符串中包含的所有字符
std::string		&TrimLeft(std::string &s, char c);
std::wstring	&TrimLeft(std::wstring &ws, wchar_t wc);
std::string		&TrimLeft(std::string &s, const std::string &m);
std::wstring	&TrimLeft(std::wstring &ws, const std::wstring &wm);

//去除右边的字符或字符串中包含的所有字符
std::string		&TrimRight(std::string &s, char c);
std::wstring	&TrimRight(std::wstring &ws, wchar_t wc);
std::string		&TrimRight(std::string &s, const std::string &m);
std::wstring	&TrimRight(std::wstring &ws, const std::wstring &wm);

//去除两边的字符或字符串中包含的所有字符
std::string		&Trim(std::string &s, char c);
std::wstring	&Trim(std::wstring &ws, wchar_t wc);
std::string		&Trim(std::string &s, const std::string &m);
std::wstring	&Trim(std::wstring &ws, const std::wstring &wm);

//去除两边的不可打印字符(e.g " \r\n\t\v")
std::string		&TrimWS(std::string &s);
std::wstring	&TrimWS(std::wstring &ws);

//忽略大小写查找子字符串
//找到返回子字符串所在位置，未找到返回std::string::npos
size_t			StrCaseStr(const std::string& haystack, const std::string& needle, size_t start_pos = 0);
size_t			StrCaseStr(const std::wstring& whaystack, const std::wstring& wneedle, size_t start_pos = 0);

//替换字符串为新字符串
std::string		&Replace(std::string &s, const std::string &olds, const std::string &news, bool ignore_case = false);
std::wstring	&Replace(std::wstring &s, const std::wstring &olds, const std::wstring &news, bool ignore_case = false);

//使字符串以指定字符[串]开头
std::string		&StartWith(std::string &s, char c);
std::wstring	&StartWith(std::wstring &ws, wchar_t wc);
std::string		&StartWith(std::string &s, const std::string &m);
std::wstring	&StartWith(std::wstring &ws, const std::wstring &wm);

//使字符串以指定字符[串]结尾
std::string		&EndWith(std::string &s, char c);
std::wstring	&EndWith(std::wstring &ws, wchar_t wc);
std::string		&EndWith(std::string &s, const std::string &m);
std::wstring	&EndWith(std::wstring &ws, const std::wstring &wm);

//将字符串转换为大/小写
//（注：仅针对纯ASCII或UTF-8字符串）
std::string		&UpperCase(std::string &s);
std::wstring	&UpperCase(std::wstring &ws);
std::string		&LowerCase(std::string &s);
std::wstring	&LowerCase(std::wstring &ws);

//格式化字符串
std::string		FormatString(const char* format, ...);

//分割字符串
bool			Split(const std::string &str, const std::string &sep, 
					  string_vec* vector, bool allow_null);

//转义HTML的特殊字符
std::string		EscapeHTML(const std::string &str);

//////////////////////////////////////////////////////////////////////////
//字符编码

bool			IsASCII(const std::string &input);								//字符串是纯ASCII编码
bool			IsUTF8(const std::string &input);								//字符串是否是UTF-8编码
bool			IsGB2312(const std::string &input);								//字符串是否是GB2312编码
bool			IsGBK(const std::string &input);								//字符串是否是GBK编码的

std::string		GetCharset(const std::string &str, bool ascii = true);			//探测字符串的字符集(ASCII,UTF-8,GB2312,GBK,GB18030)
std::string		GetFileCharset(const std::string &path, double &probability, 	//判断文件字符集，最多探测maxline行
								int maxline = 0);

int				UTF8Length(const std::string &utf8);							//获取UTF-8字符串的字符数
std::string		UTF8Trim(const std::string &utf8, size_t max_bytes);			//按照最大字节数截取UTF-8字符串
std::string&	UTF8Abbr(std::string &utf8, size_t max_bytes,					//简写UTF-8字符串到指定最大长度，保留最前和最后的字符，中间用...表示省略
						size_t last_reserved_words = 3);						//last_reserved_words指最后应保留多少个字符（注意不是字节）

//宽字符串 <=> 多字节字符串
std::string		WstringTostring(const std::wstring& ws);						
std::wstring	stringToWstring(const std::string& s);						

//宽字符串 <=> UTF-8 字符串
std::string		WstringToUTF8string(const std::wstring &ws, bool bStrict = false); 
std::wstring	UTF8stringToWstring(const std::string &s, bool bStrict = false); 

//多字节字符串 <=> UTF-8 字符串
std::string		stringToUTF8string(const std::string& s);
std::string		UTF8stringTostring(const std::string& s);

//UTF-8 字符串 <=> UTF-7 字符串
std::string		UTF7stringToUTF8string(const std::string &utf7);
std::string		UTF8stringToUTF7string(const std::string &utf8);

#ifdef _LIBICONV_H

std::string		ConvertToCharset(const std::string &from,						//返回一个转换过编码之后的字符串
					const std::string &to,const std::string &input, bool strict = false);
#endif

//////////////////////////////////////////////////////////////////////////
//文件系统

bool			IsAbsolutePath(const std::string &path);						//是否是绝对路径
bool			IsRootPath(const std::string &path);							//是否是根路径（/或C:\）

std::string		PathFindFileName(const std::string &path);						//返回路径的文件名或最底层目录名
std::string		PathFindExtension(const std::string &path);						//返回文件的扩展名，目录返回NULL
std::string		PathFindDirectory(const std::string &path);						//返回路径所指目录/文件的上级目录路径(路径名需为UTF-8编码)

bool			PathFileExists(const std::string &path);						//路径所指文件/目录是否存在
bool			PathIsFile(const std::string &path);							//路径是否是文件
bool			PathIsDirectory(const std::string &path);						//路径是否是有效目录

std::string&	UniqueFile(std::string &path);									//获取可用的文件路径				

std::string&	UniqueDir(std::string &path);									//获取可用的目录路径				

std::string		PathEscape(const std::string &path, 							//将路径中的非法字符替换为%HH的形式
							int platform = PATH_PLATFORM,
							bool reserve_separator = false);	

std::string&	PathComponentLegalize(std::string &component,					//路径元素合法化
							int platform = PATH_PLATFORM,
							size_t max_length = MAX_PATH / 5);

std::string&	PathLegalize(std::string& path,									//路径合法化
							int platform = PATH_PLATFORM,
							size_t max_length = MAX_PATH);

std::string		AbsolutePath(const std::string& relative);						//获取相对于当前工作目录的绝对路径
std::string		RelativePath(const std::string& src, const std::string& dst,	//获取src指定dst的相对链接
							char sep = PATH_SEP_CHAR);

//文件
bool			CopyFile(const std::string &src, const std::string &dst,		//复制文件
							bool bOverWrite = false) WUR;
bool			MoveFile(const std::string &src, const std::string &dst,		//移动文件
							bool bOverWrite = false) WUR;
bool			DeleteFile(const std::string &file) WUR;						//删除文件

//目录
bool			CreateDirectory(const std::string &dir) WUR;					//创建单层目录
bool			CreateDirectories(const std::string &dir) WUR;					//递归创建父层目录

bool			DeleteDirectory(const std::string &dir) WUR;					//删除空目录
bool			DeleteDirectories(const std::string &dir,
							delete_dir_cb func = NULL, void *arg = NULL) WUR;	//删除一个目录及包含的所有内容
bool			DeleteEmptyDirectories(const std::string &dir) WUR;				//删除一个目录下的所有不包含文件的目录

bool			CopyDirectories(const std::string &srcdir, const std::string &dstdir,
							copy_dir_cb func = NULL, void *arg = NULL) WUR;		// 将src目录拷贝到dst目录下（用法及注意见copy_directories）

//文件属性
int64_t			FileSize(const std::string &path);								//获取文件大小
std::string		FileSizeReadable(int64_t size);									//获取可读性强的文件长度值，如3bytes, 10KB, 1.5MB, 3GB

bool			ReadFile(const std::string& path, std::string* content,			//将文件读入内存
						size_t max_size = 0);
bool			WriteFile(const std::string& path, const std::string& content);	//将内存写入文件

std::string		GetExecutePath();												//获取进程路径名(即argv[0])
std::string		GetExecuteName();												//获取进程文件名
std::string		GetExecuteDir();												//获取进程起始目录（exe所在目录）

std::string		GetCurrentDir();												//获取进程当前工作目录
bool			SetCurrentDir(const std::string &dir) WUR;						//设置进程当前工作目录

std::string		GetHomeDir();													//获取当前用户的主目录
std::string	    GetAppDataDir();												//获取电脑AppData目录
std::string		GetTempDir();													//获取临时目录
std::string		GetTempFile(const std::string& prefix);							//获取临时文件的路径（非线程安全）

//进程
bool			CreateProcess(const std::string& commandline,
							bool show = false, bool wait = false);				//创建新的进程

//日期时间
std::string		DateStr(time_t t);												//获取日期字符串，如"2012-06-06"
std::string		TimeStr(time_t t);												//获取时间字符串，如"16:07:32"
std::string		DateTimeStr(time_t t);											//获取日期时间字符串，如"2012-06-06 16:07:32"
std::string		CurrentTime();													//获取当前时间字符串
std::string		TimeSpanReadable(int64_t seconds, int cutoff = 3);				//返回一个可读性强的时间差，如"1天20小时13分24秒"

//数字=>字符串
std::string		IntToStr(int i);
std::string		UIntToStr(uint i);
std::string		SizeToStr(size_t i);
std::string		Int64ToStr(int64_t i);
std::string		UInt64ToStr(uint64_t i);
std::string		DoubleToStr(double i);

//字符串=>数字
int				StrToInt(const std::string &s);	
uint			StrToUInt(const std::string &s);
size_t			StrToSize(const std::string &s);
int64_t			StrToInt64(const std::string &s);
uint64_t		StrToUInt64(const std::string &s);
double			StrToDouble(const std::string &s);

//调试相关
std::string		HexDump(const void *buf, int len);								//返回一块缓冲区的16进行描述字符串

std::string		PtrToStr(void *ptr);											//将指针转换成十六进制的字符串
void*			StrToPtr(const std::string &str);								//将代表指针的十六进制的字符串转换为指针值

//////////////////////////////////////////////////////////////////////////
//封装类

//线程类
//派生线程类只需重写 RunImpl() 函数
//如果需要线程一次初始化，在RunImpl()中首先调用Once()即可
class Thread
{
public:
	Thread(int id = 0);
	virtual ~Thread();

	bool Run();
	int Join();	

	void SetID(int id) {id_ = id;}
	int GetID() {return id_;}

	void SetOnce(thread_once_t *once, thread_once_func once_func);

	virtual int RunImpl() = 0;

protected:
	bool Once();

protected:
	int id_;

private:
	thread_t thread_;

	thread_once_t* once_;
	thread_once_func once_func_;
};

//互斥锁类
class MutexLock
{
public:
	MutexLock(){
		mutex_init(&mlock_);
	}
	~MutexLock(){
		mutex_destroy(&mlock_);
	}
	void Lock(){
		mutex_lock(&mlock_);
	}
	void Unlock(){
		mutex_unlock(&mlock_);
	}
	bool Trylock(){
		return mutex_trylock(&mlock_) == 1;
	}

private:
	mutex_t mlock_;
	DISABLE_COPY_AND_ASSIGN(MutexLock);
};

//自动互斥锁
class AutoMutexLock
{
public:
	AutoMutexLock(MutexLock &lock):lock_(&lock) {
		lock_->Lock();
	}
	~AutoMutexLock(){
		lock_->Unlock();
	}
private:
	MutexLock* lock_;
	DISABLE_COPY_AND_ASSIGN(AutoMutexLock);
};

//自旋锁
class SpinLock
{
public:
	SpinLock(){
		spin_init(&slock_);
	}
	~SpinLock(){
		//nothing to do...
	}
	void Lock(){
		spin_lock(&slock_);
	}
	void Unlock(){
		spin_unlock(&slock_);
	}
	bool Trylock(){
		return spin_trylock(&slock_) == 1;
	}

private:
	spin_lock_t slock_;
	DISABLE_COPY_AND_ASSIGN(SpinLock);
};

//自动自旋锁
class AutoSpinLock
{
public:
	AutoSpinLock(SpinLock &lock):lock_(&lock){
		lock_->Lock();
	}
	~AutoSpinLock(){
		lock_->Unlock();
	}

private:
	SpinLock* lock_;
	DISABLE_COPY_AND_ASSIGN(AutoSpinLock);
};

#ifdef USE_READ_WRITE_LOCK

//读写锁
class ReadWriteLock
{
public:
	ReadWriteLock(){
		rwlock_init(&rwlock_);
	}
	~ReadWriteLock(){
		rwlock_destroy(&rwlock_);
	}
	void LockRead(){
		rwlock_rdlock(&rwlock_);
	}
	void UnlockRead(){
		rwlock_rdunlock(&rwlock_);
	}
	void LockWrite(){
		rwlock_wrlock(&rwlock_);
	}
	void UnlockWrite(){
		rwlock_wrunlock(&rwlock_);
	}

private:
	rwlock_t rwlock_;
	DISABLE_COPY_AND_ASSIGN(ReadWriteLock);
};

//自动读锁
class AutoReadLock
{
public:
	AutoReadLock(ReadWriteLock &lock):lock_(&lock){
		lock_->LockRead();
	}
	~AutoReadLock(){
		lock_->UnlockRead();
	}

private:
	ReadWriteLock*	lock_;
	DISABLE_COPY_AND_ASSIGN(AutoReadLock);
};

//自动写锁
class AutoWriteLock
{
public:
	AutoWriteLock(ReadWriteLock &lock):lock_(&lock){
		lock_->LockWrite();
	}
	~AutoWriteLock(){
		lock_->UnlockWrite();
	}

private:
	ReadWriteLock*	lock_;
	DISABLE_COPY_AND_ASSIGN(AutoWriteLock);
};

#endif

//原子变量类
//TODO: 添加常用操作符重载
class Atomic
{
public:
	Atomic(long i = 0){
		Set(i);
	}
	Atomic(const Atomic &rhs){
		Set(rhs.Get());
	}
	void Set(long i){
		atomic_set(&val_, i);
	}
	long Get() const{
		return atomic_get(&val_);
	}
	void Add(long i){
		atomic_add(i, &val_);
	}
	void Sub(long i){
		atomic_sub(i, &val_);
	}
	void Increment(){
		atomic_inc(&val_);
	}
	void Decrement(){
		atomic_dec(&val_);
	}

private:
	atomic_t val_;
};

//计时器类
class TimeMeter
{
public:
	TimeMeter() {Reset();}

	void Reset() {Start(); Stop();}

	void Start() {time_meter_start(&t_);}
	void Stop() {time_meter_stop(&t_);}

	double ElapsedUS() const {return time_meter_elapsed_us(&t_);}
	double ElapsedMS() const {return time_meter_elapsed_ms(&t_);}
	double ElapsedS() const {return time_meter_elapsed_s(&t_);}

	double ElapseUSTillNow() const {return time_meter_elapsed_us_till_now(&t_);}
	double ElapseMSTillNow() const {return time_meter_elapsed_ms_till_now(&t_);}
	double ElapseSTillNow()  const {return time_meter_elapsed_s_till_now(&t_);}

private:
	mutable time_meter_t t_;
};

//自动关闭打开的文件
class ScopedFILE {
public:
	ScopedFILE(const std::string& path, const std::string& mode) {
		fp_ = xfopen(path.c_str(), mode.c_str());
	}
	virtual ~ScopedFILE() {if (fp_) xfclose(fp_);}

	FILE* get() {return fp_;}

	bool valid() {return fp_ != NULL;}

	size_t Read(void* buffer, size_t size, size_t count) {
		if (!fp_) return 0; 
		return fread(buffer, size, count, fp_);
	}

	size_t Write(const void* buffer, size_t size, size_t count) {
		if (!fp_) return 0;
		return fwrite(buffer, size, count, fp_);
	}

private:
	FILE* fp_;
};

//线程本地存储模板类
template <typename Type>
class ThreadLocalStorage {
public:
	ThreadLocalStorage() {
		if(!thread_tls_create(&tls_)){
			ASSERT(!"tls create failed!");
		}
	}

	~ThreadLocalStorage() {
		if(!thread_tls_free(tls_)){
			ASSERT(!"tls free failed!");
		}
	}

	Type* Get() {
		void *ptr;
		if(!thread_tls_get(tls_, &ptr)){
			ASSERT(!"tls get failed!");
			ptr = NULL;
		}
		return static_cast<Type*>(ptr);
	}

	void Set(Type* ptr) {
		if(!thread_tls_set(tls_, ptr)){
			ASSERT(!"tls set failed!");
		}
	}

private:
	thread_tls_t tls_;
	DISABLE_COPY_AND_ASSIGN(ThreadLocalStorage<Type>);
};

#define TLS ThreadLocalStorage

//单例模式模板类
template <typename Type>
class LazyInstance {
public:
	LazyInstance():instance_(NULL){
	}
	~LazyInstance(){
		if (instance_)
			delete(instance_);
	}

	Type& instance(){
		if (!instance_){
			instance_ = new Type;
		}
		return *instance_;
	}

private:
	Type* instance_;
};

}; //namespace utils

//////////////////////////////////////////////////////////////////////////
//内存调试

#if defined DBG_MEM && (!defined COMPILER_MSVC || _MSC_VER > MSVC6)

//运行时内存调试
#ifdef DBG_MEM_RT
enum MEMRT_MTD_CPP{
	MEMRT_NEW = MEMRT_C_END + 1,			/* new */
	MEMRT_NEW_ARRAY,						/* new[] */
	MEMRT_DELETE,							/* delete */
	MEMRT_DELETE_ARRAY,						/* delete[]*/
	MEMRT_CPP_END
};

enum MEMRT_ERR_CPP{
	MEMRTE_NODELETE = MEMRTE_C_END+1,		/* 对象未delete */
	MEMRTE_UNALLOCDEL,						/* 没有new却被delete */
	MEMRTE_CPP_END
};

void memrt_msg_cpp(int error, struct MEMRT_OPERATE *rtp);

#define DBG_DELETE_MEMRT(p) __memrt_release(MEMRT_DELETE, p, 0, path_find_file_name(__FILE__),__FUNCTION__,__LINE__, NULL, memrt_msg_cpp),
#define DBG_DELETE_ARRAY_MEMRT(p) __memrt_release(MEMRT_DELETE_ARRAY, p, 0, path_find_file_name(__FILE__),__FUNCTION__,__LINE__, NULL, memrt_msg_cpp),

#else
#define DBG_DELETE_MEMRT(p)
#define DBG_DELETE_ARRAY_MEMRT(p)
#endif	//DBG_MEM_RT

#ifdef DBG_MEM_LOG
#define DBG_DELELTE_LOGPRINTF(p) log_dprintf(LOG_DEBUG, "{%s %s %d} Delete at %p.\n",path_find_file_name(__FILE__),__FUNCTION__,__LINE__, p),
#define DBG_DELELTE_ARRAY_LOGPRINTF(p) log_dprintf(LOG_DEBUG, "{%s %s %d} Delete at %p.\n",path_find_file_name(__FILE__),__FUNCTION__,__LINE__, p),
#else
#define DBG_DELELTE_LOGPRINTF(p)
#define DBG_DELELTE_ARRAY_LOGPRINTF(p)
#endif //DBG_MEM_LOG

void* operator new(size_t size, const char *file, const char *func, int line);
void* operator new[](size_t size, const char *file, const char *func, int line);
void  operator delete(void* pointer, const char* file, const char *func, int line);
void  operator delete[](void* pointer, const char* file, const char *func, int line);
#define xnew ::new(__FILE__,__FUNCTION__,__LINE__)

#define xdelete(p) --g_xalloc_count, DBG_DELETE_MEMRT(p) DBG_DELELTE_LOGPRINTF(p) delete p
#define xdelete_array(p) --g_xalloc_count, DBG_DELELTE_ARRAY_LOGPRINTF(p) DBG_DELETE_ARRAY_MEMRT(p) delete[] p

#else  //DGB_MEM
#define xnew new
#define xdelete(p) delete p
#define xdelete_array(p) delete[] p
#endif //DGB_MEM

#endif /* UTILS_CCUTIL_H */

/*
 * vim: set ts=4 sw=4
 */
