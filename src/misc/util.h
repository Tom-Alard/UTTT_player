#ifndef UTTT2_UTIL_H
#define UTTT2_UTIL_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "../board/square.h"

#define assume(cond) do { if (!(cond)) __builtin_unreachable(); } while (0)

#define BIT_SET(a,b) ((a) |= (1ULL<<(b)))
#define BIT_CHECK(a,b) ((a) & (1ULL<<(b)))
#define BIT_CHANGE(a,b,c) ((a) = (a) & ~(b) | (-(c) & (b)))
#define ONE_BIT_SET(a) (((a) & ((a)-1)) == 0 && (a) != 0)

void* safe_malloc(size_t size);

void* safe_calloc(size_t size);

void* safe_realloc(void* pointer, size_t size);

void safe_free(void* pointer);

Square toOurNotation(Square rowAndColumn);

Square toGameNotation(Square square);

#endif //UTTT2_UTIL_H
