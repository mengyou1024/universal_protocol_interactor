/**
 * @file universalProtocolInteractor.h
 * @author djkj6666 (1111111111@qq.com)
 * @brief Universal Protocol Interactor
 * @version 0.0.1
 * @date 2023-02-02
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void *interactor_t;

interactor_t intercatorCreate(const char *filename, size_t rx_buf_size, size_t rx_thread_stack_size);

void intercatorDelete(interactor_t intr);

bool intercatorRequest(interactor_t intr, void *req, size_t req_len, void *resp, size_t *resp_len, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif
