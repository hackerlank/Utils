#include "../ccutil.h"

#include <gtest/gtest.h>

const char kLogName[] = "basic";
const char kThreadLogName[] = "threading";

class LoggingThread : public utils::Thread {
public:
    LoggingThread(int id): utils::Thread(id) {}

    virtual int RunImpl() OVERRIDE {
        log_printf(kThreadLogName, id_, "thread %d begin logging...", id_);
        for (int i = 0; i < 1000; ++i)
            log_printf(kThreadLogName, id_, "This is thread %d", id_);

        log_printf(kThreadLogName, id_, "thread %d finish logging", id_);
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

TEST(Logging, Default) {
    log_open(NULL, "default.log", 0, 0);

    log_debug("debug!");
    log_info("info!");
    log_warning("warning!");

    EXPECT_TRUE(log_close(NULL));
}
