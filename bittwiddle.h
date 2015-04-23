//===========================================================//
//
// Maverick Chess Engine
// Copyright 2013-2015 Steve Maughan
//
//===========================================================//

#include <intrin.h>

#if (defined(_WIN32) && !defined(_WIN64)) || defined(__arm__) || defined(__linux__)

static inline t_chess_square bitscan(t_bitboard b)
{
    b ^= (b - 1);
    unsigned int folded = (int)(b ^ (b >> 32));
    return bitscan_table[folded * 0x78291ACF >> 26];
}

static inline int popcount(t_bitboard b)
{
    int i = 0;
    t_bitboard a = b;
    while (a) {
        i++;
        a &= (a - 1);
    }
    return(i);
}

static inline t_chess_square bitscan_reset(t_bitboard *b)
{
    t_bitboard c = *b ^ (*b - 1);
    *b &= (*b - 1);
    unsigned int folded = (int)(c ^ (c >> 32));
    return bitscan_table[folded * 0x78291ACF >> 26];
}

#elif defined(MINGW64)

static inline t_chess_square bitscan(t_bitboard b) {
    t_bitboard index;
    __asm__("bsfq %1, %0": "=r"(index) : "rm"(b));
    return (t_chess_square)index;
}

static inline t_chess_square bitscan_reset(t_bitboard *b)
{
    t_bitboard index;
    __asm__("bsfq %1, %0": "=r"(index) : "rm"(*b));
    *b &= (*b - 1);
    return (t_chess_square)index;
}

#if defined(NOPOPCOUNT)
static inline int popcount(t_bitboard b)
{
    int i = 0;
    t_bitboard a = b;
    while (a) {
        i++;
        a &= (a - 1);
    }
    return(i);
}
#else
static inline int popcount(t_bitboard b)
{
    return __builtin_popcountll(b);
}
#endif

#else

#pragma intrinsic(_BitScanForward64)

static inline t_chess_square bitscan(t_bitboard b)
{
    unsigned long index;

    _BitScanForward64(&index, b);

    return index;
}

static inline t_chess_square bitscan_reset(t_bitboard *b)
{
    unsigned long index;

    _BitScanForward64(&index, *b);
    *b &= (*b - 1);

    return index;
}

#if NOPOPCOUNT

static inline int popcount(t_bitboard b)
{
    int i = 0;
    t_bitboard a = b;
    while (a) {
        i++;
        a &= (a - 1);
    }
    return(i);
}

#else

#pragma intrinsic(__popcnt64)

static inline int popcount(t_bitboard b)
{
    return __popcnt64(b);
}

#endif

#endif

static inline BOOL is_bit_set(t_bitboard b, int i) {
    return ((SQUARE64(i) & b) != 0);
}
