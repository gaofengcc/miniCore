/**
 * @file epx_types.h
 * @brief Base types and error codes for EpxCore.
 */

#ifndef EPX_TYPES_H
#define EPX_TYPES_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t epx_bool_t;
#define EPX_TRUE   1
#define EPX_FALSE  0

typedef int32_t epx_err_t;

#define EPX_OK                 0
#define EPX_ERR                 (-1)
#define EPX_ERR_PARAM           (-2)
#define EPX_ERR_NOMEM           (-3)
#define EPX_ERR_TIMEOUT         (-4)
#define EPX_ERR_BUSY            (-5)
#define EPX_ERR_IO              (-6)
#define EPX_ERR_NOT_SUPPORTED   (-7)
#define EPX_ERR_QUEUE_FULL      (-8)

/** Millisecond timestamp (monotonic). */
typedef uint64_t epx_time_t;

/* Wire layout: packed struct for RPC/cross-device payload, avoid alignment mismatch. */
#if defined(__GNUC__) || defined(__clang__)
#define EPX_PACKED __attribute__((packed))
#else
#define EPX_PACKED
#endif

#ifdef __cplusplus
}
#endif

#endif /* EPX_TYPES_H */
