#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif
typedef void *interactor_t;

/**
 * @brief unsolicited request messages filter
 * 
 * @param intr interactor object 
 * @param buf recv buf pointer
 * @param size recv length
 * 
 * @note when the filter func return ture , interactor will execute callback function on task queue
 * 
 */
typedef bool (*upi_urm_filter_func)(interactor_t intr, uint8_t *buf, size_t size);


/**
 * @brief unsolicited request messages callback func type
 * 
 * @param buf recv buf pointer
 * @param size recv length
 */
typedef void (*upi_urm_callback_func)(uint8_t *buf, size_t size);

typedef struct {
    upi_urm_filter_func   filter;
    upi_urm_callback_func callback;
} upi_urm_rom, *upi_urm_rom_t;

#ifdef __GNUC__
#    define upi_urm_export(filter, callback)                                                                                               \
        __attribute__((used)) const static upi_urm_rom __upi_urm_rom_##filter##_##callback                                                 \
            __attribute__((section(".upi_urm"))) = {filter, callback}
extern size_t __upi_urm_start, __upi_urm_end;
#else
#    define upi_urm_export(filter, callback)                                                                                               \
        __attribute__((used)) const static upi_urm_rom __upi_urm_rom_##filter##_##callback                                                 \
            __attribute__((section("upi_urm"))) = {filter, callback}
extern size_t upi_urm$$Base;
extern size_t upi_urm$$Limit;
#    define __upi_urm_start upi_urm$$Base
#    define __upi_urm_end   upi_urm$$Limit
#endif

/**
 * @brief export a pair of filter and callback
 * 
 * @param filter filter function # upi_urm_callback_func
 * @param callback callback function
 * 
 */
#define UPI_URM_EXPORT(filter, callback) upi_urm_export(filter, callback)

#ifdef __cplusplus
}
#endif
