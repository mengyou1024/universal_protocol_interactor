#include "universalProtocolInteractor.h"
#include <fcntl.h>
#include <mqueue.h>
#include <pthread.h>
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
#    define DBG_TAG    "upi"
#    define DBG_LVL    DBG_LOG
#    include <rtdbg.h>
#    define LOG LOG_D
#else
#    include <stdio.h>
#    include <string.h>
#    define upi_printf printf
#    define upi_malloc malloc
#    define upi_free   free
#    define upi_calloc calloc
#    define upi_memset memset
#    define upi_memcpy memcpy
#    define LOG(FMT, ...)                                                                                                                  \
        do {                                                                                                                               \
            upi_printf("[%s:%d] " FMT "\r\n", __func__, __LINE__, ##__VA_ARGS__);                                           \
        } while (0)
#endif

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

typedef struct {
    const upi_urm_rom *rom;\
    intr_msg_t msg;
}urm_thread_param, *urm_thread_param_t;

static void *interactorRUMCallback(void *param) {
    urm_thread_param_t urm_param = (urm_thread_param_t)param;
    
    urm_param->rom->callback(urm_param->msg->buf, urm_param->msg->len);

    upi_free(urm_param->msg->buf);
    upi_free(urm_param->msg);
    upi_free(urm_param);
    return NULL;
}

static bool inURMROM(_interactor_t intr, intr_msg_t msg) {
    for (upi_urm_rom *rom = (upi_urm_rom_t)&__upi_urm_start; rom < (upi_urm_rom_t)&__upi_urm_end; ++rom) {
        if (rom->filter != 0 && rom->callback != 0) {
            if (rom->filter((interactor_t)intr, msg->buf, msg->len) == true) {
                urm_thread_param_t upi_param = upi_malloc(sizeof(urm_thread_param));
                upi_param->rom = rom;
                upi_param->msg = msg;
                pthread_t tid;
                int ret = pthread_create(&tid, NULL, interactorRUMCallback, upi_param);
                LOG("create thread tid: %ul, ret: %d", tid, ret);
                pthread_detach(tid);
                return true;
            }
        }
    }
    return false;
}

static void *intercatorReadThread(void *param) {
    _interactor_t intr = (_interactor_t)param;
    // sleep(1);
    while (1) {
        ssize_t size = read(intr->fd, intr->rx_buf, intr->rx_buf_size);
        LOG("read.size: %d", size);
        if (size > 0) {
            intr_msg_t msg = upi_malloc(sizeof(intr_msg));
            msg->buf = upi_malloc(size);
            msg->len = size;
            if (msg->buf != NULL) {
                upi_memcpy(msg->buf, intr->rx_buf, msg->len);
                if (!inURMROM(intr, msg)) {
                    int ret = mq_send(intr->rx_mq, (const char *)msg, sizeof(intr_msg), 0);
                    upi_free(msg);
                }
            }
        }
        LOG("read thread running.");
        sleep(1);
        lseek(intr->fd, 0, SEEK_SET);
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
        ret->fd          = open(filename, O_RDWR);
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
        if (pthread_create(&ret->rx_thread, NULL, intercatorReadThread, ret) != 0) {
            goto __exit;
        }
        ++count;
    }
    return ret;
__exit:
    LOG("intr creat error.");
    intercatorDelete(ret);
    return NULL;
}

void intercatorDelete(interactor_t intr) {
    if (intr != NULL) {
        _interactor_t inter = (_interactor_t)intr;
        if (inter->fd > 0) {
            close(inter->fd);
        } else {
            LOG("fd:%d open failed.", inter->fd);
        }
        pthread_mutex_destroy(&inter->lock);
        if (inter->rx_buf != 0) {
            upi_free(inter->rx_buf);
        } else {
            LOG("low memory.");
        }
        if (inter->rx_thread != 0) {
            pthread_cancel(inter->rx_thread);
        }else {
            LOG("read thread creat error.");
        }
        pthread_join(inter->rx_thread, NULL);
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
