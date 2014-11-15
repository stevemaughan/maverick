//===========================================================//
//
// Maverick Chess Engine
// Copyright 2013-2014 Steve Maughan
//
//===========================================================//

#if defined(MINGW64)

static inline t_chess_square bitscan(t_bitboard b) {
    t_bitboard index;
__asm__("bsfq %1, %0": "=r"(index): "rm"(b) );
    return (t_chess_square)index;
}

static inline void prefetch(struct t_move_record *move) {
    __builtin_prefetch(move, 0, 3);
}

#if defined (POPCOUNT)

static inline int popcount(t_bitboard b)
{
    return __builtin_popcountll(b);
}

#else

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

#endif

static inline t_chess_square bitscan_reset(t_bitboard *b)
{
    //t_bitboard c = *b ^ (*b - 1);
    //*b &= (*b - 1);
    //unsigned int folded = (int) (c ^ (c >> 32));
    //return bitscan_table[folded * 0x78291ACF >> 26];
    //t_chess_square s = bitscan(*b);
    //*b ^= SQUARE64(s);
    //return s;
    t_bitboard index;
__asm__("bsfq %1, %0": "=r"(index): "rm"(*b) );
    *b &= (*b - 1);
    return (t_chess_square)index;
}

#else

inline void prefetch(struct t_move_record *move) {

}

static inline t_chess_square bitscan(t_bitboard b)
{
    b ^= (b - 1);
    unsigned int folded = (int) (b ^ (b >> 32));
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
    unsigned int folded = (int) (c ^ (c >> 32));
    return bitscan_table[folded * 0x78291ACF >> 26];
}

#endif

static inline BOOL is_bit_set(t_bitboard b, int i) {
    return ((SQUARE64(i) & b) != 0);
}
