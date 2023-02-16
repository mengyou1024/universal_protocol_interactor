#include "universalProtocolInteractor.h"
#include <fcntl.h>
#include <mqueue.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>

#define MQ_NAME_LEN    (8)
#define MQ_MAX_ITEMS   (10)

#define INTR_USING_LOG (1)

#ifdef __RTTHREAD__
#    include <rtthread.h>
#    define upi_printf rt_kprintf
#    define upi_malloc rt_malloc
#    define upi_free   rt_free
#    define upi_calloc rt_calloc
#    define upi_memset rt_memset
#    define upi_memcpy rt_memcpy
#    define DBG_TAG    "upi"
#    define DBG_LVL    DBG_LOG
#    include <rtdbg.h>
#    if INTR_USING_LOG
#        define LOG LOG_D
#    else
#        define LOG(...)
#    endif
#else
#    include <string.h>
#    define upi_printf printf
#    define upi_malloc malloc
#    define upi_free   free
#    define upi_calloc calloc
#    define upi_memset memset
#    define upi_memcpy memcpy
#    if INTR_USING_LOG
#        define LOG(FMT, ...)                                                                                                                                  \
            do {                                                                                                                                               \
                upi_printf("[upi:%s] " FMT "\r\n", __func__, ##__VA_ARGS__);                                                                                   \
            } while (0)
#    else
#        define LOG(...)
#    endif
#endif

typedef struct interactor {
    pthread_mutex_t lock;
    int             fd;
    pthread_t       rx_thread;
    uint8_t        *rx_buf;
    size_t          rx_buf_size;
    mqd_t           rx_mq;
    char            mq_name[MQ_NAME_LEN];
    bool            add_to_mq;
} interactor, *_interactor_t;

typedef struct intr_msg {
    uint8_t *buf;
    size_t   len;
} intr_msg, *intr_msg_t;

typedef struct {
    const upi_urm_rom *rom;
    intr_msg_t         msg;
} urm_thread_param, *urm_thread_param_t;

static pthread_t   taskQueueTid = 0;
static size_t      intrCounter  = 0;
static mqd_t       taskMq       = 0;
const static char *taskMqName   = "/intrMq";

static void *interactorTaskQueueThread(void *param) {
    (void)param;
    while (1) {
        urm_thread_param urm_param;
        mq_receive(taskMq, (char *)&urm_param, sizeof(urm_thread_param), 0);
        urm_param.rom->callback(urm_param.msg->buf, urm_param.msg->len);
        upi_free(urm_param.msg->buf);
        upi_free(urm_param.msg);
    }
}

static bool inURMROM(_interactor_t intr, intr_msg_t msg) {
    for (upi_urm_rom *rom = (upi_urm_rom_t)&__upi_urm_start; rom < (upi_urm_rom_t)&__upi_urm_end; ++rom) {
        if (rom->filter != 0 && rom->callback != 0) {
            if (rom->filter((interactor_t)intr, msg->buf, msg->len) == true) {
                urm_thread_param urm_param;
                urm_param.rom = rom;
                urm_param.msg = msg;
                int ret       = mq_send(taskMq, (const char *)&urm_param, sizeof(urm_thread_param), 0);
                LOG("mq: %d, ret: %d", taskMq, ret);
                return true;
            }
        }
    }
    return false;
}

static void *interactorReadThread(void *param) {
    _interactor_t intr = (_interactor_t)param;
    while (1) {
        ssize_t size = read(intr->fd, intr->rx_buf, intr->rx_buf_size);
        LOG("read.size: %d", (int)size);
        if (size > 0) {
            intr_msg_t msg = upi_malloc(sizeof(intr_msg));
            msg->buf       = upi_malloc(size);
            msg->len       = size;
            if (intr->add_to_mq == true) {
                upi_memcpy(msg->buf, intr->rx_buf, msg->len);
                intr->add_to_mq = false;
                int ret         = mq_send(intr->rx_mq, (const char *)msg, sizeof(intr_msg), 0);
                LOG("mq_send ret = %d", ret);
            } else if (msg->buf != NULL) {
                upi_memcpy(msg->buf, intr->rx_buf, msg->len);
                inURMROM(intr, msg);
            }
        }
        LOG("read thread running.");
        sleep(1);
        lseek(intr->fd, 0, SEEK_SET);
    }
}

interactor_t interactorCreate(const char *filename, size_t rx_buf_size, pthread_attr_t *t_attr) {
    _interactor_t ret   = (_interactor_t)upi_malloc(sizeof(interactor));
    static int    count = 0;
    if (ret != NULL) {
        if (intrCounter == 0) {
            struct mq_attr _mq_attr;
            _mq_attr.mq_flags   = 0;
            _mq_attr.mq_maxmsg  = MQ_MAX_ITEMS;
            _mq_attr.mq_msgsize = sizeof(urm_thread_param);
            taskMq              = mq_open(taskMqName, O_RDWR | O_CREAT, 066, &_mq_attr);
            if (taskMq <= 0) {
                LOG("create intr messagequeue error.");
                goto __exit;
            } else {
                if (pthread_create(&taskQueueTid, NULL, interactorTaskQueueThread, NULL) != 0) {
                    LOG("creat taskQueue error.");
                    goto __exit;
                } else {
                    intrCounter++;
                }
            }
        } else {
            intrCounter++;
        }
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
        if (pthread_create(&ret->rx_thread, t_attr, interactorReadThread, ret) != 0) {
            goto __exit;
        }
        ++count;
    }
    return ret;
__exit:
    LOG("intr creat error.");
    interactorDelete(ret);
    return NULL;
}

void interactorDelete(interactor_t intr) {
    if (intr != NULL) {
        _interactor_t _intr = (_interactor_t)intr;
        if (_intr->fd > 0) {
            close(_intr->fd);
        } else {
            LOG("fd:%d open failed.", _intr->fd);
        }
        pthread_mutex_destroy(&_intr->lock);
        if (_intr->rx_buf != 0) {
            upi_free(_intr->rx_buf);
        } else {
            LOG("low memory.");
        }
        if (_intr->rx_thread != 0) {
            pthread_cancel(_intr->rx_thread);
            pthread_join(_intr->rx_thread, NULL);
        } else {
            LOG("read thread creat error.");
        }
        mq_close(_intr->rx_mq);
        mq_unlink(_intr->mq_name);
        upi_free(_intr);

        if (intrCounter > 0) {
            intrCounter--;
        }
        if (intrCounter == 0) {
            if (taskQueueTid != 0) {
                pthread_cancel(taskQueueTid);
                pthread_join(taskQueueTid, NULL);
            }
            if (taskMq > 0) {
                mq_close(taskMq);
                mq_unlink(taskMqName);
            }
        }
    }
}

bool interactorRequest(interactor_t intr, void *req, size_t req_len, void *resp, size_t *resp_len, uint32_t timeout_ms) {
    if (intr != NULL) {
        _interactor_t   _intr = (_interactor_t)intr;
        struct timespec ts;
        ssize_t         ret = -1;
        intr_msg        msg = {0};

        // 1. lock the request
        pthread_mutex_lock(&_intr->lock);
        _intr->add_to_mq = true;
        // 2. send request
        ret = write(_intr->fd, req, req_len);
        if (ret <= 0) {
            goto __unlock;
        }
        // 3. recv request
        clock_gettime(CLOCK_REALTIME, &ts);
        LOG("ts.tv_sec = %d, ts.tv_nsec = %d", (int)ts.tv_sec, (int)ts.tv_nsec);
        ts.tv_sec += (timeout_ms / 1000);
        ts.tv_nsec += ((timeout_ms % 1000) * 1000ULL);
        LOG("ts.tv_sec = %d, ts.tv_nsec = %d", (int)ts.tv_sec, (int)ts.tv_nsec);
        ret = mq_timedreceive(_intr->rx_mq, (char *)&msg, sizeof(intr_msg), 0, &ts);
    __unlock:
        // 4. unlock the request
        pthread_mutex_unlock(&_intr->lock);
        if (ret <= 0) {
            _intr->add_to_mq = false;
            LOG("request error.");
        } else {
            // 5. copy data to user
            if (resp_len != NULL && resp != NULL) {
                if (*resp_len != 0)
                    *resp_len = ((*resp_len) > (msg.len)) ? msg.len : (*resp_len);
                else
                    *resp_len = msg.len;
                upi_memcpy(resp, msg.buf, *resp_len);
            }
            LOG("Request: %.*s", (int)msg.len, (char *)msg.buf);
            upi_free(msg.buf);
            return true;
        }
    }
    return false;
}
