#include "../ccutil.h"

#include <cstdlib>
#include <gtest/gtest.h>

namespace {

const int kThreadCount = 10;

int THREAD_CALLTYPE ExitCodeThreadProc(void *arg) {
    int i = (int)(size_t)arg;

    msleep(100);

    if (i % 2) {
        uthread_exit(i * 2);
        NOT_REACHED();
        return -1;
    }

    return i;
}

thread_tls_t g_tls[kThreadCount];

int THREAD_CALLTYPE LocalStorageThreadProc(void *arg) {
    int i = (int)(size_t)arg;
    char buf[16];
    void *p;

    EXPECT_TRUE(thread_tls_create(&g_tls[i]));

    snprintf(buf, sizeof(buf), "%d", i);
    EXPECT_TRUE(thread_tls_set(g_tls[i], xstrdup(buf)));

    msleep(10);

    EXPECT_TRUE(thread_tls_get(g_tls[i], &p));
    EXPECT_STREQ(buf, (char*)p);
    xfree(p);

    EXPECT_TRUE(thread_tls_free(g_tls[i]));
    return 0;
}

thread_once_t g_once;
int g_once_count;

int OnceInitFunc() {
    ++g_once_count;
    return 0;
}

int THREAD_CALLTYPE OnceInitThreadProc(void* arg) {
    thread_once(&g_once, OnceInitFunc);

    return 0;
}

}

TEST(Thread, ExitCode) {
    uthread_t threads[kThreadCount];
    for (int i = 0; i < kThreadCount; i++) {
        EXPECT_TRUE(uthread_create(threads + i, ExitCodeThreadProc, 
            (void*)(size_t)i, i % 2 ? 8192 : 0));
    }

    for (int i = 0; i < kThreadCount; i++) {
        int exit_code;
        uthread_join(threads[i], &exit_code);
        EXPECT_EQ((i % 2 ? i * 2 : i), exit_code);
    }
}

TEST(Thread, LocalStorage) {
    uthread_t threads[kThreadCount];
    for (int i = 0; i < kThreadCount; i++) {
        EXPECT_TRUE(uthread_create(threads + i, LocalStorageThreadProc,
            (void*)(size_t)i, 0));
    }
    for (int i = 0; i < kThreadCount; i++) {
        uthread_join(threads[i], NULL);
    }
}

TEST(Thread, OnceInit) {
    uthread_t threads[kThreadCount];

    thread_once_init(&g_once);
    
    for (int i = 0; i < kThreadCount; i++) {
        EXPECT_TRUE(uthread_create(threads + i, OnceInitThreadProc,
            (void*)(size_t)i, 0));
    }
    for (int i = 0; i < kThreadCount; i++) {
        uthread_join(threads[i], NULL);
    }

    EXPECT_EQ(1, g_once_count);
}
