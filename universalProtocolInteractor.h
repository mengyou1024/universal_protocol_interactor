#pragma once

#include "UPI_URM_EXPORT.h"
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief creator an interactor 
 * 
 * @param filename virtual file path of device
 * @param rx_buf_size max recv length
 * @param t_attr recv thread attribue
 * @return interactor_t interactor object 
 */
interactor_t interactorCreate(const char *filename, size_t rx_buf_size, pthread_attr_t *t_attr) ;

/**
 * @brief delete an interactor
 * 
 * @param intr interactor object 
 */
void interactorDelete(interactor_t intr);

/**
 * @brief send a request with interactor
 * 
 * @param intr interactor object 
 * @param req request buf
 * @param req_len request length
 * @param resp response buf
 * @param resp_len reponse length
 * @param timeout_ms timeout in ms
 * @return true succed
 * @return false failed
 */
bool interactorRequest(interactor_t intr, void *req, size_t req_len, void *resp, size_t *resp_len, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif
