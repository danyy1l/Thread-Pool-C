/**
 * @file types.h
 * @author Danyyil Shykerynets
 * @brief Global data types definition
 * * This file contains the alias for fixed size types (stdint)
 * @version 1.0
 * @date 2026-02-09
 */

#ifndef _TYPES_H
#define _TYPES_H

#include <stdint.h>
#include <sys/types.h>

/** @name Whole and floating data types
 **@{*/
typedef int32_t i32;  /**< Signed integer 32 bit. [-2^31, 2^31 - 1] */
typedef int64_t i64;  /**< Signed integer 64 bit. [-2^63, 2^63 - 1] */
typedef uint32_t u32; /**< Unsigned integer 32 bit. [0, 2^32 - 1] */
typedef uint64_t u64; /**< Unsigned integer 64 bit. [0, 2^64 - 1] */
typedef float f32;    /**< Float 32 bit. 7 dig. precision */
typedef double f64;   /**< Float 64 bit. 15 dig. precision */
/**@}*/

/** @name Error control */
/**@{*/
#define ERR -1    /**< Standard return error code */
#define OK !(ERR) /**< Success code (0) */

/** @name Status flags */
/**@{*/
#define FOUND 1 /**< Represents the success in finding solution of a thread */
/**@}*/

/** @name General utility */
/**@{*/
/**
 * @brief Returns the minimum of two values. Only number types
 * @param a First value to compare
 * @param b Second value to compare
 * @return Minimum of the two values
 */
#define MIN(a, b) ((a) < (b) ? (a) : (b))
/**@}*/

#endif
