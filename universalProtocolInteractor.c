#include "universalProtocolInteractor.h"
#include <mqueue.h>
#include <pthread.h>
#include <sys/fcntl.h>
#include <sys/time.h>
#include <unistd.h>

#define MQ_NAME_LEN  (8)
#define MQ_MAX_ITEMS (10)

#ifdef __RTTHREAD__
#    include <rtthread.h>
#    define upi_printf rt_kprintf
#    define upi_malloc rt_malloc
#    define upi_free   rt_fee
#    define upi_calloc rt_calloc
#    define upi_memset rt_memset
#    define upi_memcpy rt_memcpy
#else
#    include <stdio.h>
#    include <string.h>
#    define upi_printf printf
#    define upi_malloc malloc
#    define upi_free   free
#    define upi_calloc calloc
#    define upi_memset memset
#    define upi_memcpy memcpy
#endif

#define LOG(FMT, ...)                                                                                                                      \
    do {                                                                                                                                   \
        upi_printf("[%s] [%s:%d] " FMT "\r\n", __FILE__, __func__, __LINE__, ##__VA_ARGS__);                                               \
    } while (0)

typedef struct interactor {
    pthread_mutex_t lock;
    int             fd;
    pthread_t       rx_thread;
    uint8_t        *rx_buf;
    size_t          rx_buf_size;
    mqd_t           rx_mq;
    char            mq_name[MQ_NAME_LEN];
} interactor, *_interactor_t;

typedef struct intr_msg {
    uint8_t *buf;
    size_t   len;
} intr_msg, *intr_msg_t;

void *hello(void *args) {
    while (1) {
        LOG("hello world.");
        usleep(1000 * 100);
    }
}

static void *intercatorReadThread(void *param) {
    _interactor_t intr = (_interactor_t)param;
    while (1) {
        ssize_t size = read(intr->fd, intr->rx_buf, intr->rx_buf_size);
        if (size > 0) {
            intr_msg msg;
            msg.buf = upi_malloc(size);
            msg.len = size;
            if (msg.buf != NULL) {
                upi_memcpy(msg.buf, intr->rx_buf, msg.len);
                int ret = mq_send(intr->rx_mq, (const char *)&msg, sizeof(intr_msg), 0);
                LOG("send ret: %d", ret);
            }
        }
        sleep(1);
    }
}

interactor_t intercatorCreate(const char *filename, size_t rx_buf_size, size_t rx_thread_stack_size) {
    _interactor_t ret   = (_interactor_t)upi_malloc(sizeof(interactor));
    static int    count = 0;
    if (ret != NULL) {
        upi_memset(ret, 0, sizeof(interactor));
        if (pthread_mutex_init(&ret->lock, NULL) != 0) {
            goto __exit;
        }
        ret->fd = open(filename, O_RDWR);
        if (pthread_create(&ret->rx_thread, NULL, intercatorReadThread, ret) != 0) {
            goto __exit;
        }
        ret->rx_buf_size = rx_buf_size;
        ret->rx_buf      = upi_malloc(rx_buf_size);
        if (ret->rx_buf == NULL) {
            goto __exit;
        }
        snprintf(ret->mq_name, MQ_NAME_LEN, "/intr%02d", count);
        struct mq_attr _mq_attr;
        _mq_attr.mq_flags   = 0;
        _mq_attr.mq_maxmsg  = MQ_MAX_ITEMS;
        _mq_attr.mq_msgsize = sizeof(intr_msg);
        ret->rx_mq          = mq_open(ret->mq_name, O_RDWR | O_CREAT, 066, &_mq_attr);
        if (ret->rx_mq <= 0) {
            goto __exit;
        }
        ++count;
    }
    return ret;
__exit:
    intercatorDelete(ret);
    return NULL;
}

void intercatorDelete(interactor_t intr) {
    if (intr != NULL) {
        _interactor_t inter = (_interactor_t)intr;
        if (inter->fd > 0) {
            close(inter->fd);
        }
        pthread_mutex_destroy(&inter->lock);
        if (inter->rx_buf != 0) {
            upi_free(inter->rx_buf);
        }
        pthread_cancel(inter->rx_thread);
        mq_close(inter->rx_mq);
        mq_unlink(inter->mq_name);
        upi_free(inter);
    }
}

bool intercatorRequest(interactor_t intr, void *req, size_t req_len, void *resp, size_t *resp_len, uint32_t timeout_ms) {
    if (intr != NULL) {
        _interactor_t  _intr = (_interactor_t)intr;
        struct timeval tv;
        gettimeofday(&tv, NULL);
        struct timespec ts;
        uint64_t        nsec = (uint64_t)(tv.tv_usec) * 1000 + (uint64_t)(timeout_ms)*1000 * 1000;
        ts.tv_sec            = tv.tv_sec + (nsec / 1000000000ULL);
        ts.tv_nsec           = nsec % 1000000000ULL;
        intr_msg msg;
        ssize_t  ret = mq_timedreceive(_intr->rx_mq, (char *)&msg, sizeof(intr_msg), 0, &ts);
        if (ret <= 0) {
            LOG("request error.");
        } else {
            LOG("Request: %.*s", (int)msg.len, (char *)msg.buf);
            upi_free(msg.buf);
        }
    }
    return false;
}
