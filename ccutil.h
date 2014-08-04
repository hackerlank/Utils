/**
 *
 * @file: ccutil.h
 * @desc: C++���ߺ�����
 *		  ��ຯ����������Ӧc�����ķ�װ
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

#include "cutil.h"

#define NPOS std::string::npos

#define STL_CONTAINS(container, key) \
	(container.find(key) != container.end())

//ע��g++��ʹ�ô˺����ֶ�����C++11֧��
//�����-std=c++11��-std=gnu++11������ѡ��(ǰ�߽�����GNU��չ)
#define STL_FOR_EACH(iter, container) \
	for (auto iter = (container).begin(); iter != (container).end(); ++iter)

#define STL_REVERSE_FOR_EACH(iter, container) \
	for (auto iter = (container).rbegin(); iter != (container).rend(); ++iter)

namespace utils {

//���Ͷ���
typedef std::list<std::string> string_list;
typedef std::vector<std::string> string_vec;

//////////////////////////////////////////////////////////////////////////
//�ַ���

//ȥ����ߵ��ַ����ַ����а����������ַ�
std::string		&TrimLeft(std::string &s, char c);
std::wstring	&TrimLeft(std::wstring &ws, wchar_t wc);
std::string		&TrimLeft(std::string &s, const std::string &m);
std::wstring	&TrimLeft(std::wstring &ws, const std::wstring &wm);

//ȥ���ұߵ��ַ����ַ����а����������ַ�
std::string		&TrimRight(std::string &s, char c);
std::wstring	&TrimRight(std::wstring &ws, wchar_t wc);
std::string		&TrimRight(std::string &s, const std::string &m);
std::wstring	&TrimRight(std::wstring &ws, const std::wstring &wm);

//ȥ�����ߵ��ַ����ַ����а����������ַ�
std::string		&Trim(std::string &s, char c);
std::wstring	&Trim(std::wstring &ws, wchar_t wc);
std::string		&Trim(std::string &s, const std::string &m);
std::wstring	&Trim(std::wstring &ws, const std::wstring &wm);

//ȥ�����ߵĲ��ɴ�ӡ�ַ�(e.g " \r\n\t\v")
std::string		&TrimWS(std::string &s);
std::wstring	&TrimWS(std::wstring &ws);

//���Դ�Сд�������ַ���
//�ҵ��������ַ�������λ�ã�δ�ҵ�����std::string::npos
size_t			StrCaseStr(const std::string& haystack, const std::string& needle, size_t start_pos = 0);
size_t			StrCaseStr(const std::wstring& whaystack, const std::wstring& wneedle, size_t start_pos = 0);

//�滻�ַ���Ϊ���ַ���
std::string		&Replace(std::string &s, const std::string &olds, const std::string &news, bool ignore_case = false);
std::wstring	&Replace(std::wstring &s, const std::wstring &olds, const std::wstring &news, bool ignore_case = false);

//ʹ�ַ�����ָ���ַ�[��]��ͷ
std::string		&StartWith(std::string &s, char c);
std::wstring	&StartWith(std::wstring &ws, wchar_t wc);
std::string		&StartWith(std::string &s, const std::string &m);
std::wstring	&StartWith(std::wstring &ws, const std::wstring &wm);

//ʹ�ַ�����ָ���ַ�[��]��β
std::string		&EndWith(std::string &s, char c);
std::wstring	&EndWith(std::wstring &ws, wchar_t wc);
std::string		&EndWith(std::string &s, const std::string &m);
std::wstring	&EndWith(std::wstring &ws, const std::wstring &wm);

//���ַ���ת��Ϊ��/Сд
//��ע������Դ�ASCII��UTF-8�ַ�����
std::string		&UpperCase(std::string &s);
std::wstring	&UpperCase(std::wstring &ws);
std::string		&LowerCase(std::string &s);
std::wstring	&LowerCase(std::wstring &ws);

//��ʽ���ַ���
std::string		FormatString(const char* format, ...);

//�ָ��ַ���
bool			Split(const std::string &str, const std::string &sep, 
					  string_vec* vector, bool allow_null);

//ת��HTML�������ַ�
std::string		EscapeHTML(const std::string &str);

//////////////////////////////////////////////////////////////////////////
//�ַ�����

bool			IsASCII(const std::string &input);								//�ַ����Ǵ�ASCII����
bool			IsUTF8(const std::string &input);								//�ַ����Ƿ���UTF-8����
bool			IsGB2312(const std::string &input);								//�ַ����Ƿ���GB2312����
bool			IsGBK(const std::string &input);								//�ַ����Ƿ���GBK�����

std::string		GetCharset(const std::string &str, bool ascii = true);			//̽���ַ������ַ���(ASCII,UTF-8,GB2312,GBK,GB18030)
std::string		GetFileCharset(const std::string &path, double &probability, 	//�ж��ļ��ַ��������̽��maxline��
								int maxline = 0);

int				UTF8Length(const std::string &utf8);							//��ȡUTF-8�ַ������ַ���
std::string		UTF8Trim(const std::string &utf8, size_t max_bytes);			//��������ֽ�����ȡUTF-8�ַ���
std::string&	UTF8Abbr(std::string &utf8, size_t max_bytes,					//��дUTF-8�ַ�����ָ����󳤶ȣ�������ǰ�������ַ����м���...��ʾʡ��
						size_t last_reserved_words = 3);						//last_reserved_wordsָ���Ӧ�������ٸ��ַ���ע�ⲻ���ֽڣ�

//���ַ��� <=> ���ֽ��ַ���
std::string		WstringTostring(const std::wstring& ws);						
std::wstring	stringToWstring(const std::string& s);						

//���ַ��� <=> UTF-8 �ַ���
std::string		WstringToUTF8string(const std::wstring &ws, bool bStrict = false); 
std::wstring	UTF8stringToWstring(const std::string &s, bool bStrict = false); 

//���ֽ��ַ��� <=> UTF-8 �ַ���
std::string		stringToUTF8string(const std::string& s);
std::string		UTF8stringTostring(const std::string& s);

//UTF-8 �ַ��� <=> UTF-7 �ַ���
std::string		UTF7stringToUTF8string(const std::string &utf7);
std::string		UTF8stringToUTF7string(const std::string &utf8);

#ifdef _LIBICONV_H

std::string		ConvertToCharset(const std::string &from,						//����һ��ת��������֮����ַ���
					const std::string &to,const std::string &input, bool strict = false);
#endif

//////////////////////////////////////////////////////////////////////////
//�ļ�ϵͳ

bool			IsAbsolutePath(const std::string &path);						//�Ƿ��Ǿ���·��
bool			IsRootPath(const std::string &path);							//�Ƿ��Ǹ�·����/��C:\��

std::string		PathFindFileName(const std::string &path);						//����·�����ļ�������ײ�Ŀ¼��
std::string		PathFindExtension(const std::string &path);						//�����ļ�����չ����Ŀ¼����NULL
std::string		PathFindDirectory(const std::string &path);						//����·����ָĿ¼/�ļ����ϼ�Ŀ¼·��(·������ΪUTF-8����)

bool			PathFileExists(const std::string &path);						//·����ָ�ļ�/Ŀ¼�Ƿ����
bool			PathIsFile(const std::string &path);							//·���Ƿ����ļ�
bool			PathIsDirectory(const std::string &path);						//·���Ƿ�����ЧĿ¼

std::string&	UniqueFile(std::string &path);									//��ȡ���õ��ļ�·��				

std::string&	UniqueDir(std::string &path);									//��ȡ���õ�Ŀ¼·��				

std::string		PathEscape(const std::string &path, 							//��·���еķǷ��ַ��滻Ϊ%HH����ʽ
							int platform = PATH_PLATFORM,
							bool reserve_separator = false);	

std::string&	PathComponentLegalize(std::string &component,					//·��Ԫ�غϷ���
							int platform = PATH_PLATFORM,
							size_t max_length = MAX_PATH / 5);

std::string&	PathLegalize(std::string& path,									//·���Ϸ���
							int platform = PATH_PLATFORM,
							size_t max_length = MAX_PATH);

std::string		AbsolutePath(const std::string& relative);						//��ȡ����ڵ�ǰ����Ŀ¼�ľ���·��
std::string		RelativePath(const std::string& src, const std::string& dst,	//��ȡsrcָ��dst���������
							char sep = PATH_SEP_CHAR);

//�ļ�
bool			CopyFile(const std::string &src, const std::string &dst,		//�����ļ�
							bool bOverWrite = false) WUR;
bool			MoveFile(const std::string &src, const std::string &dst,		//�ƶ��ļ�
							bool bOverWrite = false) WUR;
bool			DeleteFile(const std::string &file) WUR;						//ɾ���ļ�

//Ŀ¼
bool			CreateDirectory(const std::string &dir) WUR;					//��������Ŀ¼
bool			CreateDirectories(const std::string &dir) WUR;					//�ݹ鴴������Ŀ¼

bool			DeleteDirectory(const std::string &dir) WUR;					//ɾ����Ŀ¼
bool			DeleteDirectories(const std::string &dir,
							delete_dir_cb func = NULL, void *arg = NULL) WUR;	//ɾ��һ��Ŀ¼����������������
bool			DeleteEmptyDirectories(const std::string &dir) WUR;				//ɾ��һ��Ŀ¼�µ����в������ļ���Ŀ¼

bool			CopyDirectories(const std::string &srcdir, const std::string &dstdir,
							copy_dir_cb func = NULL, void *arg = NULL) WUR;		// ��srcĿ¼������dstĿ¼�£��÷���ע���copy_directories��

//�ļ�����
int64_t			FileSize(const std::string &path);								//��ȡ�ļ���С
std::string		FileSizeReadable(int64_t size);									//��ȡ�ɶ���ǿ���ļ�����ֵ����3bytes, 10KB, 1.5MB, 3GB

bool			ReadFile(const std::string& path, std::string* content,			//���ļ������ڴ�
						size_t max_size = 0);
bool			WriteFile(const std::string& path, const std::string& content);	//���ڴ�д���ļ�

std::string		GetExecutePath();												//��ȡ����·����(��argv[0])
std::string		GetExecuteName();												//��ȡ�����ļ���
std::string		GetExecuteDir();												//��ȡ������ʼĿ¼��exe����Ŀ¼��

std::string		GetCurrentDir();												//��ȡ���̵�ǰ����Ŀ¼
bool			SetCurrentDir(const std::string &dir) WUR;						//���ý��̵�ǰ����Ŀ¼

std::string		GetHomeDir();													//��ȡ��ǰ�û�����Ŀ¼
std::string	    GetAppDataDir();												//��ȡ����AppDataĿ¼
std::string		GetTempDir();													//��ȡ��ʱĿ¼
std::string		GetTempFile(const std::string& prefix);							//��ȡ��ʱ�ļ���·�������̰߳�ȫ��

//����
bool			CreateProcess(const std::string& commandline,
							bool show = false, bool wait = false);				//�����µĽ���

//����ʱ��
std::string		DateStr(time_t t);												//��ȡ�����ַ�������"2012-06-06"
std::string		TimeStr(time_t t);												//��ȡʱ���ַ�������"16:07:32"
std::string		DateTimeStr(time_t t);											//��ȡ����ʱ���ַ�������"2012-06-06 16:07:32"
std::string		CurrentTime();													//��ȡ��ǰʱ���ַ���
std::string		TimeSpanReadable(int64_t seconds, int cutoff = 3);				//����һ���ɶ���ǿ��ʱ����"1��20Сʱ13��24��"

//����=>�ַ���
std::string		IntToStr(int i);
std::string		UIntToStr(uint i);
std::string		SizeToStr(size_t i);
std::string		Int64ToStr(int64_t i);
std::string		UInt64ToStr(uint64_t i);
std::string		DoubleToStr(double i);

//�ַ���=>����
int				StrToInt(const std::string &s);	
uint			StrToUInt(const std::string &s);
size_t			StrToSize(const std::string &s);
int64_t			StrToInt64(const std::string &s);
uint64_t		StrToUInt64(const std::string &s);
double			StrToDouble(const std::string &s);

//�������
std::string		HexDump(const void *buf, int len);								//����һ�黺������16���������ַ���

std::string		PtrToStr(void *ptr);											//��ָ��ת����ʮ�����Ƶ��ַ���
void*			StrToPtr(const std::string &str);								//������ָ���ʮ�����Ƶ��ַ���ת��Ϊָ��ֵ

//////////////////////////////////////////////////////////////////////////
//��װ��

//��ʱ����װ��
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

//�Զ��رմ򿪵��ļ�
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

//����Ŀ¼
class ScopedWalkDir
{
public:
	ScopedWalkDir(const std::string& dir);

	~ScopedWalkDir();

	bool Next();

	bool IsDotOrDotDot() {return walk_entry_is_dot(ctx_) || walk_entry_is_dotdot(ctx_);}
	bool IsFile() {return walk_entry_is_file(ctx_) == 1;}
	bool IsDirectory() {return walk_entry_is_dir(ctx_) == 1;};
	bool IsRegularFile() {return walk_entry_is_regular(ctx_) == 1;}

	std::string Name();
	std::string Path();

private:
	std::string dir_;
	walk_dir_context* ctx_;
};

//�߳���
//�����߳���ֻ����д RunImpl() ����
//�����Ҫ�߳�һ�γ�ʼ������RunImpl()�����ȵ���Once()����
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

//��������
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

//�Զ�������
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

//������
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

//�Զ�������
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

//��д��
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

//�Զ�����
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

//�Զ�д��
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

//ԭ�ӱ�����
//TODO: ��ӳ��ò���������
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

//�̱߳��ش洢ģ����
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

//����ģʽģ����
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
//�ڴ����

#if defined DBG_MEM && (!defined COMPILER_MSVC || _MSC_VER > MSVC6)

//����ʱ�ڴ����
#ifdef DBG_MEM_RT
enum MEMRT_MTD_CPP{
	MEMRT_NEW = MEMRT_C_END + 1,			/* new */
	MEMRT_NEW_ARRAY,						/* new[] */
	MEMRT_DELETE,							/* delete */
	MEMRT_DELETE_ARRAY,						/* delete[]*/
	MEMRT_CPP_END
};

enum MEMRT_ERR_CPP{
	MEMRTE_NODELETE = MEMRTE_C_END+1,		/* ����δdelete */
	MEMRTE_UNALLOCDEL,						/* û��newȴ��delete */
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
