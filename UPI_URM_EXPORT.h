/**
 * @file UPI_URM_EXPORT.h
 * @author  ()
 * @brief
 * @version 0.0.1
 * @date 2023-02-03
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

typedef void (*upi_urm_callback_func)(uint8_t *buf, size_t size);
typedef bool (*upi_urm_filter_func)(interactor_t intr, uint8_t *buf, size_t size);

typedef struct {
    upi_urm_filter_func   filter;
    upi_urm_callback_func callback;
} upi_urm_rom, *upi_urm_rom_t;

#define upi_urm_export(filter, callback)                                                                                                   \
    __attribute__((used)) const static upi_urm_rom __upi_urm_rom_##filter##_##callback                                                     \
        __attribute__((section(".upi_urm"))) = {filter, callback}

#ifdef __GNUC__
extern size_t __upi_urm_start, __upi_urm_end;
#else
extern size_t upi_urm$$Base;
extern size_t upi_urm$$Limit;
#    define __upi_urm_start upi_urm$$Base
#    define __upi_urm_end   upi_urm$$Limit
#endif

#ifdef __cplusplus
}
#endif
