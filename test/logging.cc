#include "../ccutil.h"

#include <gtest/gtest.h>

const char kLogName[] = "basic";
const char kThreadLogName[] = "threading";

class LoggingThread : public utils::Thread {
public:
    LoggingThread(int id): utils::Thread(id) {}

    virtual int RunImpl() OVERRIDE {
        log_printf(kThreadLogName, LOG_DEBUG, "thread %d begin logging...", id_);
        for (int i = 0; i < 1000; ++i)
            log_printf(kThreadLogName, LOG_DEBUG, "This is thread %d", id_);

        log_printf(kThreadLogName, LOG_DEBUG, "thread %d finish logging", id_);
        return 0;
    }
};

TEST(Logging, Basic) {
    ASSERT_TRUE(log_open(kLogName, "basic.log", 0, 0));
    ASSERT_TRUE(path_is_file("basic.log"));

    log_printf0(kLogName, "%s", "something");
    
    EXPECT_FALSE(log_close("basi"));
    EXPECT_TRUE(log_close(kLogName));

    std::string content = utils::ReadFile("basic.log");
    EXPECT_TRUE(content.substr(3) == "something");
}

int line_counter(char *line, size_t len, size_t nline, void *arg) {
    int *counts = (int*)arg;
    if (len > 0)
        (*counts)++;
    return 1;
}

TEST(Logging, Threading) {
    const int kThreads = 4;
    ASSERT_TRUE(log_open(kThreadLogName, "threading.log", 0, 0));

    LoggingThread* threads[16];
    for (int i = LOG_WARNING; i < LOG_WARNING + kThreads; i++) {
        threads[i] = new LoggingThread(i);
        threads[i]->Run();
    }

    for (int i = LOG_WARNING; i < LOG_WARNING + kThreads; i++)
        threads[i]->Join();

    EXPECT_TRUE(log_close(kThreadLogName));

    // 检查写入的日志行数
    int lines = 0;
    FILE* fp = xfopen("threading.log", "r");
    IGNORE_RESULT(foreach_line(fp, line_counter, &lines));
    EXPECT_EQ(kThreads * 2 + kThreads * 1000, lines);
}

static const char kMsgDebug[] = "debug log will not log to system log!!!";
static const char kMsgInfo[] = "info log will not log to system log!!!";
static const char kMsgNotice[] = "notice log will log to system log!!!";
static const char kMsgWarning[] = "warning log will log to system log!!!";
static const char kMsgError[] = "error log will log to system log!!!";

static int debug_found;
static int info_found;
static int notice_found;
static int warning_found;
static int error_found;

static int system_log(char *line, size_t len, size_t nline, void *arg) {
    if (strstr(line, kMsgDebug))
        debug_found = 1;
    if (strstr(line, kMsgInfo))
        info_found = 1;
    if (strstr(line, kMsgNotice))
        notice_found = 1;
    if (strstr(line, kMsgWarning))
        warning_found = 1;
    if (strstr(line, kMsgError))
        error_found = 1;
    return 1;
}

TEST(Logging, Default) {
    log_debug("%s", kMsgDebug);
    log_info("%s", kMsgInfo);
    log_notice("%s", kMsgNotice);
    log_warning("%s", kMsgWarning);
#ifdef NDEBUG
    log_error("%s", kMsgError);
#endif

#if defined(OS_MACOSX)
    FILE *fp = xfopen("/var/log/system.log", "r");
    if (!fp)
        fp = xfopen("/var/log/messages", "r");

    ASSERT_TRUE(fp != NULL);
    ASSERT_TRUE(foreach_line(fp, system_log, NULL));
    EXPECT_FALSE(debug_found);
    EXPECT_FALSE(info_found);
    EXPECT_TRUE(notice_found);
    EXPECT_TRUE(warning_found);
#ifdef NDEBUG
    EXPECT_TRUE(error_found);
#endif

#endif
}
