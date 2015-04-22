//===========================================================//
//
// Maverick Chess Engine
// Copyright 2013-2015 Steve Maughan
//
//===========================================================//

#if defined(_WIN32)
#include <windows.h>
#include <process.h>
#include <conio.h>
#else
#include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#include "defs.h"
#include "data.h"
#include "procs.h"
#include "bittwiddle.h"

// ----------------------------------------------------------//
// UCI Interface Variables
// ----------------------------------------------------------//
struct t_uci uci;
char engine_author[30];
char engine_name[30];

// ----------------------------------------------------------//
// Chess Board
// ----------------------------------------------------------//
struct t_board position[1];

// ----------------------------------------------------------//
// Global Move List Variables
// ----------------------------------------------------------//
struct t_move_record xmove_list[GLOBAL_MOVE_COUNT];
struct t_move_record *move_directory[64][64][15];

// ----------------------------------------------------------//
// Principle Variation Stack
// ----------------------------------------------------------//
struct t_perft_pv_data perft_pv_data[MAXPLY + 1];
struct t_multi_pv multi_pv[1];

// ----------------------------------------------------------//
// Global Search Variables
// ----------------------------------------------------------//
int search_ply;
unsigned long cutoffs;
unsigned long first_move_cutoffs;

int message_update_count;
t_nodes message_update_mask;
long last_display_update;
long search_start_time;
int deepest;
int most_captures;
t_nodes nodes;
t_nodes qnodes;
t_nodes start_nodes;
t_chess_time early_move_time;
t_chess_time target_move_time;
t_chess_time abort_move_time;

// ----------------------------------------------------------//
// Perft Global Variables
// ----------------------------------------------------------//
t_nodes global_nodes;
long perft_start_time;
long perft_end_time;

// ----------------------------------------------------------//
// Aspiration Window
// ----------------------------------------------------------//
const t_chess_value aspiration_window[2][6] = {{1, 25, 150, CHESS_INFINITY, CHESS_INFINITY, CHESS_INFINITY}, { 1, 100, CHESS_INFINITY, CHESS_INFINITY, CHESS_INFINITY, CHESS_INFINITY }};

// ----------------------------------------------------------//
// Direction of moving pieces
// ----------------------------------------------------------//
const BOOL slider[15] = {FALSE, FALSE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, TRUE, TRUE, FALSE, FALSE};
const char direction[15][10] = {
    {0},
    {33, 31, 18, 14, -14, -18, -31, -33, 0},				// White Knight
    {17, 15, -15, -17, 0},								// White Bishop
    {-16, -1, 1, 16, 0},									// White Rook
    {17, 16, 15, 1, -1, -15, -16, -17, 0},				// White Queen
    {15, 17, 0},											// White Pawn
    {17, 16, 15, 1, -1, -15, -16, -17, 0},				// White King
    {0},
    {0},
    {-33, -31, -18, -14, 14, 18, 31, 33, 0},				// Black Knight
    {-17, -15, 15, 17, 0},								// Black Bishop
    {16, 1, -1, -16, 0},									// Black Rook
    {-17, -16, -15, -1, 1, 15, 16, 17, 0},				// Black Queen
    {-15, -17, 0},										// Black Pawn
    {-17, -16, -15, -1, 1, 15, 16, 17, 0}					// Black King
};
const char piece_count_value[15] = {0, 2, 3, 5, 9, 0, 0, 0, 0, 2, 3, 5, 9, 0, 0};

// ----------------------------------------------------------//
// Static Exchange Evaluation
// ----------------------------------------------------------//
const int see_piece_value[15] = {0, 350, 350, 500, 900, 100, 10000, 0, 0, 350, 350, 500, 900, 100, 10000};

// ----------------------------------------------------------//
// Castling Data
// ----------------------------------------------------------//
struct t_castle_record castle[4];

// ----------------------------------------------------------//
// Repitition Variables
// ----------------------------------------------------------//
t_hash draw_stack[MAX_MOVES];
int draw_stack_count;
int search_start_draw_stack_count;

// ----------------------------------------------------------//
// Bitboards
// ----------------------------------------------------------//
t_bitboard between[64][64];									// squares between any two squares on the board (*not* including start and finish)
t_bitboard line[64][64];									// squares between any two squares on the board (*including* start and finish)
t_bitboard bishop_rays[64];									// bishop moves on an empty board from each square
t_bitboard rook_rays[64];									// rook moves on an empty board from each square
t_bitboard queen_rays[64];									// queen moves on an empty board from each square
t_bitboard knight_mask[64];									// knight moves from each square
t_bitboard king_mask[64];									// king moves from each square
t_bitboard rank_mask[2][8];									// mask of each rank indexed by color
t_bitboard pawn_attackers[2][64];							// for each square it shows the squares from which a pawn could attack
t_bitboard xray[64][64];									// Going from then to, these are the squares the other side of the "to" square
t_bitboard neighboring_file[8];								// Adjacent files set to "1"
t_bitboard column_mask[8];									// Mask out columns
t_bitboard square_rank_mask[64];							// Mask the squares on the same rank
t_bitboard square_column_mask[64];							// Mask the squares on the same column
t_bitboard forward_squares[2][64];							// Mask of squares in front of give square in direction of pawn
t_bitboard connected_pawn_mask[64];							// Mask for connected pawn for a given square

const t_chess_square bitscan_table[64] = {
    63, 30,  3, 32, 59, 14, 11, 33, 60, 24, 50,  9, 55, 19, 21, 34,
    61, 29,  2, 53, 51, 23, 41, 18, 56, 28,  1, 43, 46, 27,  0, 35,
    62, 31, 58,  4,  5, 49, 54,  6, 15, 52, 12, 40,  7, 42, 45, 16,
    25, 57, 48, 13, 10, 39,  8, 44, 20, 47, 38, 22, 17, 37, 36, 26
};

// ----------------------------------------------------------//
// Can Move Mask
// ----------------------------------------------------------//
t_piece_mask can_move[64][64];

// ----------------------------------------------------------//
// Piece Tables
// ----------------------------------------------------------//
t_chess_value piece_value[2][8];

t_chess_value piece_square_table[16][2][64];

const t_chess_value pawn_pst[2][64] = {
    {0, 0, 0, 0, 0, 0, 0, 0,
        5, 5, 7, -20, -20, 7, 5, 5,
        5, 5, -5, 4, 4, -5, 3, 3,
        0, 0, 5, 20, 20, 5, 0, 0,
        5, 10, 15, 25, 25, 15, 10, 5,
        10, 20, 35, 30, 30, 35, 20, 10,
        40, 40, 40, 40, 40, 40, 40, 40,
        0, 0, 0, 0, 0, 0, 0, 0},

    {0, 0, 0, 0, 0, 0, 0, 0,
     5, -10, -20, -25, -25, -20, -10, 5,
     5, -10, -20, -25, -25, -20, -10, 5,
     10, -5, -15, -20, -20, -15, -5, 10,
     18, 2, -8, -15, -15, -8, 2, 18,
     30, 14, 1, -10, -10, 1, 14, 30,
     45, 30, 16, 5, 5, 16, 30, 45,
     0, 0, 0, 0, 0, 0, 0, 0}
};

const t_chess_value knight_pst[2][64] = {
    {-69, -19, -24, -14, -14, -24, -19, -69,
        -54, -39, -9, 11, 11, -9, -39, -54,
        -39, 1, 31, 21, 21, 31, 1, -39,
        -39, 11, 41, 36, 36, 41, 11, -39,
        -39, 41, 51, 51, 51, 51, 41, -39,
        -39, 46, 61, 71, 71, 61, 46, -39,
        -39, 21, 41, 41, 41, 41, 21, -39,
        -59, -39, -29, -29, -29, -29, -39, -59},

    {-63, -53, -43, -43, -43, -43, -53, -63,
     -53, -43, 18, 28, 28, 18, -43, -53,
     -43, 18, 48, 38, 38, 48, 18, -43,
     -43, 38, 58, 68, 68, 58, 38, -43,
     -43, 38, 73, 78, 78, 73, 38, -43,
     -43, 28, 78, 73, 73, 78, 28, -43,
     -53, -43, 38, 48, 48, 38, -43, -53,
     -63, -53, -43, -43, -43, -43, -53, -63}
};

const t_chess_value bishop_pst[2][64] = {
    {-30, -25, -20, -20, -20, -20, -25, -30,
        -28, 11, 6, 1, 1, 6, 11, -28,
        -25, 6, 16, 11, 11, 16, 6, -25,
        1, 1, 16, 21, 21, 16, 1, 1,
        1, 21, 21, 26, 26, 21, 21, 1,
        1, 11, 21, 26, 26, 21, 11, 1,
        -10, 11, 1, 1, 1, 1, 11, -10,
        -20, -18, -16, -14, -14, -16, -18, -20},

    {-38, -18, -8, 2, 2, -8, -18, -38,
     -18, -8, 2, 7, 7, 2, -8, -18,
     -8, 2, 10, 12, 12, 10, 2, -8,
     2, 12, 16, 20, 20, 16, 12, 2,
     2, 12, 17, 22, 22, 17, 12, 2,
     -8, 2, 20, 22, 22, 20, 2, -8,
     -18, -8, 0, 12, 12, 0, -8, -18,
     -38, -18, -8, 2, 2, -8, -18, -38}
};

const t_chess_value rook_pst[2][64] = {
    {-8, -6, 2, 7, 7, 2, -6, -8,
        -8, -6, 2, 7, 7, 2, -6, -8,
        -8, -6, 6, 7, 7, 6, -6, -8,
        -8, -6, 6, 7, 7, 6, -6, -8,
        -8, -6, 6, 8, 8, 6, -6, -8,
        -8, -6, 6, 10, 10, 6, -6, -8,
        2, 2, 7, 12, 12, 7, 2, 2,
        -8, -6, 2, 7, 7, 2, -6, -8},

    {0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0}
};

const t_chess_value queen_pst[2][64] = {
    {-26, -16, -6, 4, 4, -6, -16, -26,
        -16, -11, -1, 4, 4, -1, -11, -16,
        -6, -6, -1, 4, 4, -1, -6, -6,
        4, 4, 4, 4, 4, 4, 4, 4,
        4, 4, 4, 4, 4, 4, 4, 4,
        4, 4, 4, 4, 4, 4, 4, 4,
        4, 4, 4, 4, 4, 4, 4, 4,
        4, 4, 4, 4, 4, 4, 4, 4},

    {-46, -41, -31, -26, -26, -31, -41, -46,
     -31, -26, -16, -6, -6, -16, -26, -31,
     -16, -1, 14, 24, 24, 14, -1, -16,
     -6, 9, 24, 34, 34, 24, 9, -6,
     -6, 9, 24, 34, 34, 24, 9, -6,
     -6, 9, 24, 34, 34, 24, 9, -6,
     -16, 4, 19, 29, 29, 19, 4, -16,
     -26, -6, -1, 4, 4, -1, -6, -26}
};

const t_chess_value king_pst[2][64] = {
    {-20, -10, 0, -20, -3, -25, 0, -20,
        -30, -30, -35, -40, -40, -35, -30, -30,
        -40, -40, -45, -50, -50, -45, -40, -40,
        -50, -50, -55, -60, -60, -55, -50, -50,
        -55, -55, -60, -70, -70, -60, -55, -55,
        -55, -55, -60, -70, -70, -60, -55, -55,
        -55, -55, -60, -70, -70, -60, -55, -55,
        -55, -55, -60, -70, -70, -60, -55, -55},

    {-30, -25, -15, -10, -10, -15, -25, -30,
     -15, -10, 0, 10, 10, 0, -10, -15,
     0, 15, 30, 40, 40, 30, 15, 0,
     10, 25, 40, 50, 50, 40, 25, 10,
     10, 25, 40, 50, 50, 40, 25, 10,
     10, 25, 40, 50, 50, 40, 25, 10,
     0, 20, 35, 45, 45, 35, 20, 0,
     -10, 10, 15, 20, 20, 15, 10, -10}
};

const t_chess_value lone_king[64]{
	123, 83, 43, 3, 3, 43, 83, 200,
	83, -17, -27, -37, -37, -27, -17, 160,
	43, -27, -47, -57, -57, -47, -27, 120,
		3, -37, -57, -77, -77, -57, -37, 80,
		3, -37, -57, -77, -77, -57, -37, 80,
		43, -27, -47, -57, -57, -47, -27, 120,
		83, -17, -27, -37, -37, -27, -17, 160,
		123, 83, 43, 3, 3, 43, 83, 200
};

const t_chess_value bishop_knight_corner[2][64]{

		{4, 24, 44, 64, 84, 104, 124, 144,
		24, -56, -46, -36, -26, -16, -6, 124,
		44, -36, -116, -96, -76, -56, -16, 104,
		64, -16, -96, -136, -136, -76, -26, 84,
		84, 4, -76, -136, -136, -96, -36, 64,
		104, 44, -56, -76, -96, -116, -46, 44,
		124, -6, -16, -26, -36, -46, -56, 24,
		144, 124, 104, 84, 64, 44, 24, 4},

		{ 144, 124, 104, 84, 64, 44, 24, 4,
		124, -6, -16, -26, -36, -46, -56, 24,
		104, -16, -56, -76, -96, -116, -36, 44,
		84, -26, -76, -136, -136, -96, -16, 64,
		64, -36, -96, -136, -136, -76, 4, 84,
		44, -46, -116, -96, -76, -56, 44, 104,
		24, -56, -46, -36, -26, -16, -6, 124,
		4, 24, 44, 64, 84, 104, 124, 144}

};

const t_chess_color square_color[64]{
		BLACK, WHITE, BLACK, WHITE, BLACK, WHITE, BLACK, WHITE,
		WHITE, BLACK, WHITE, BLACK, WHITE, BLACK, WHITE, BLACK,
		BLACK, WHITE, BLACK, WHITE, BLACK, WHITE, BLACK, WHITE,
		WHITE, BLACK, WHITE, BLACK, WHITE, BLACK, WHITE, BLACK,
		BLACK, WHITE, BLACK, WHITE, BLACK, WHITE, BLACK, WHITE,
		WHITE, BLACK, WHITE, BLACK, WHITE, BLACK, WHITE, BLACK,
		BLACK, WHITE, BLACK, WHITE, BLACK, WHITE, BLACK, WHITE,
		WHITE, BLACK, WHITE, BLACK, WHITE, BLACK, WHITE, BLACK
};

const t_chess_value passed_pawn_bonus[2][64]{
		{0, 0, 0, 0, 0, 0, 0, 0,
		15, 14, 12, 10, 10, 12, 14, 15,
		30, 28, 26, 25, 25, 26, 28, 30,
		50, 45, 40, 35, 35, 40, 45, 50,
		75, 70, 65, 60, 60, 65, 70, 75,
		105, 97, 90, 80, 80, 90, 97, 105,
		145, 130, 115, 100, 100, 115, 130, 145,
		0, 0, 0, 0, 0, 0, 0, 0},

		{ 0, 0, 0, 0, 0, 0, 0, 0,
		-145, -130, -115, -100, -100, -115, -130, -145,
		-105, -97, -90, -80, -80, -90, -97, -105,
		-75, -70, -65, -60, -60, -65, -70, -75,
		-50, -45, -40, -35, -35, -40, -45, -50,
		-30, -28, -26, -25, -25, -26, -28, -30,
		-15, -14, -12, -10, -10, -12, -14, -15,
		0, 0, 0, 0, 0, 0, 0, 0 }

};

const t_chess_value rook_behind_passed[2][2]{ // [Color][MG / EG]
	{5, 15},
	{-5, -15}
};

// ----------------------------------------------------------//
// Pawn Evaluation
// ----------------------------------------------------------//
const t_chess_value isolated_pawn[2][9]{
	{0, -15, -30, -45, -60, -75, -90, -105, -120},
	{ 0, -25, -50, -75, -100, -125, -150, -175, -200}
};

t_bitboard cannot_catch_pawn_mask[2][2][64];	// [color][to_move][king_square], so [WHITE][BLACK][E4] would return a mask of all squares where the white king on E4 cannot catch black pawns, when it is black to move!

const t_bitboard candidate_outposts[2]{SQUARE64(C5) | SQUARE64(D5) | SQUARE64(E5) | SQUARE64(F5) | SQUARE64(C6) | SQUARE64(D6) | SQUARE64(E6) | SQUARE64(F6), SQUARE64(C3) | SQUARE64(D3) | SQUARE64(E3) | SQUARE64(F3) | SQUARE64(C4) | SQUARE64(D4) | SQUARE64(E4) | SQUARE64(F4)};
const t_bitboard color_square_mask[2]{0x55AA55AA55AA55AA, 0xAA55AA55AA55AA55};

// ----------------------------------------------------------//
// Mobility
// ----------------------------------------------------------//
const t_chess_value horizontal_rook_mobility[2][8]{
	{-4, -2, 0, 2, 4, 6, 8, 10},
	{ -6, -3, 0, 3, 6, 9, 12, 15}
};

const t_chess_value vertical_rook_mobility[2][8]{
	{-6, -3, 0, 3, 6, 9, 12, 15},
	{-15, -5, 0, 5, 10, 15, 20, 25}
};

const t_chess_value trapped_rook[2][16]{
	{-30, -15, -5, 0, 5, 10, 14, 15, 15, 15, 15, 15, 15, 15, 15, 15},
	{-100, -50, -10, 0, 10, 7, 24, 28, 30, 30, 30, 30, 30, 30, 30, 30 }
};

const t_chess_value bishop_mobility[2][16]{
	{-50, -40, -10, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26},
	{-90, -50, -20, 0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48}
};

const t_chess_value queen_mobility[2][32]{
	{ -150, -100, -50, -20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{-50, -30, 0, 2, 5, 8, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10}
};

const t_chess_value knight_mobility[2][9]{
	{-30, -10, -2, 0, 2, 4, 6, 6, 6},
	{-25, -7, -1, 0, 1, 2, 3, 3, 2}
};

// ----------------------------------------------------------//
// King Safety
// ----------------------------------------------------------//
t_bitboard king_castle_squares[2][2];
t_bitboard intact_pawns[2][2];
t_bitboard wrecked_pawns[2][2];
t_bitboard pawn_wedge_mask[2][2];

const t_chess_value pawn_storm[8] {0, -50, -48, -30, -10, 0, 0, 0};
t_bitboard king_zone[64];

const t_chess_value king_safety[8] {0, 0, 64, 96, 113, 120, 124, 128};


// ----------------------------------------------------------//
// Magics
// ----------------------------------------------------------//
t_bitboard rook_magic_moves[64][4096];
const struct t_magic_structure rook_magic[64] = {
    {0x000101010101017e, 0xe580008110204000},	{0x000202020202027c, 0x0160002008011000},	{0x000404040404047a, 0x3520080020000400},
    {0x0008080808080876, 0x6408080448002002},	{0x001010101010106e, 0x1824080004020400},	{0x002020202020205e, 0x9e04088101020400},
    {0x004040404040403e, 0x8508008001000200},	{0x008080808080807e, 0x5a000040a4840201},	{0x0001010101017e00, 0x0172800180504000},
    {0x0002020202027c00, 0x0094200020483000},	{0x0004040404047a00, 0x0341400410000800},	{0x0008080808087600, 0x007e040020041200},
    {0x0010101010106e00, 0x0042104404000800},	{0x0020202020205e00, 0x00cc042090008400},	{0x0040404040403e00, 0x00a88081000a0200},
    {0x0080808080807e00, 0x006c810180840100},	{0x00010101017e0100, 0x0021150010208000},	{0x00020202027c0200, 0x0000158060004000},
    {0x00040404047a0400, 0x0080bc2808402000},	{0x0008080808760800, 0x00113e1000200800},	{0x00101010106e1000, 0x0002510001010800},
    {0x00202020205e2000, 0x0008660814001200},	{0x00404040403e4000, 0x0000760400408080},	{0x00808080807e8000, 0x0004158180800300},
    {0x000101017e010100, 0x00c0826880004000},	{0x000202027c020200, 0x0000803901002800},	{0x000404047a040400, 0x0020005d88000400},
    {0x0008080876080800, 0x0000136a18001000},	{0x001010106e101000, 0x0000053200100a00},	{0x002020205e202000, 0x0004301aa0010400},
    {0x004040403e404000, 0x0001085c80048200},	{0x008080807e808000, 0x000000be04040100},	{0x0001017e01010100, 0x0001042142401000},
    {0x0002027c02020200, 0x0140300036282000},	{0x0004047a04040400, 0x0009802008801000},	{0x0008087608080800, 0x0000201870841000},
    {0x0010106e10101000, 0x0002020926000a00},	{0x0020205e20202000, 0x0004040078800200},	{0x0040403e40404000, 0x0002000250900100},
    {0x0080807e80808000, 0x00008001b9800100},	{0x00017e0101010100, 0x0240001090418000},	{0x00027c0202020200, 0x0000100008334000},
    {0x00047a0404040400, 0x0060086000234800},	{0x0008760808080800, 0x0088088011121000},	{0x00106e1010101000, 0x000204000e1d2800},
    {0x00205e2020202000, 0x00020082002a0400},	{0x00403e4040404000, 0x0006021050270a00},	{0x00807e8080808000, 0x0000610002548080},
    {0x007e010101010100, 0x0401410280201a00},	{0x007c020202020200, 0x0001408100104e00},	{0x007a040404040400, 0x0020100980084480},
    {0x0076080808080800, 0x000a004808286e00},	{0x006e101010101000, 0x0000100120024600},	{0x005e202020202000, 0x0002000902008e00},
    {0x003e404040404000, 0x0001010800807200},	{0x007e808080808000, 0x0002050400295a00},	{0x7e01010101010100, 0x004100a080004257},
    {0x7c02020202020200, 0x000011008048c005},	{0x7a04040404040400, 0x8040441100086001},	{0x7608080808080800, 0x8014400c02002066},
    {0x6e10101010101000, 0x00080220010a010e},	{0x5e20202020202000, 0x0002000810041162},	{0x3e40404040404000, 0x00010486100110e4},
    {0x7e80808080808000, 0x000100020180406f}
};

t_bitboard bishop_magic_moves[64][512];
const struct t_magic_structure bishop_magic[64] = {
    {0x0040201008040200, 0x00a08800240c0040},	{0x0000402010080400, 0x0020085008088000},	{0x0000004020100a00, 0x0080440306000000},
    {0x0000000040221400, 0x000c520480000000},	{0x0000000002442800, 0x0024856000000000},	{0x0000000204085000, 0x00091430a0000000},
    {0x0000020408102000, 0x0009081820c00000},	{0x0002040810204000, 0x0005100400609000},	{0x0020100804020000, 0x0000004448220400},
    {0x0040201008040000, 0x00001030010c0480},	{0x00004020100a0000, 0x00003228120a0000},	{0x0000004022140000, 0x00004c2082400000},
    {0x0000000244280000, 0x0000108d50000000},	{0x0000020408500000, 0x0000128821100000},	{0x0002040810200000, 0x00000a0410245000},
    {0x0004081020400000, 0x0000030180208800},	{0x0010080402000200, 0x0050000820500c00},	{0x0020100804000400, 0x0008010050180200},
    {0x004020100a000a00, 0x0150008a00200500},	{0x0000402214001400, 0x000e000505090000},	{0x0000024428002800, 0x0014001239200000},
    {0x0002040850005000, 0x0006000540186000},	{0x0004081020002000, 0x0001000603502000},	{0x0008102040004000, 0x0005000104480c00},
    {0x0008040200020400, 0x0011400048880200},	{0x0010080400040800, 0x0008080020304800},	{0x0020100a000a1000, 0x00241800b4000c00},
    {0x0040221400142200, 0x0020480010820040},	{0x0002442800284400, 0x011484002e822000},	{0x0004085000500800, 0x000c050044822000},
    {0x0008102000201000, 0x0010090020401800},	{0x0010204000402000, 0x0004008004a10c00},	{0x0004020002040800, 0x0044214001005000},
    {0x0008040004081000, 0x0008406000188200},	{0x00100a000a102000, 0x0002043000120400},	{0x0022140014224000, 0x0010280800120a00},
    {0x0044280028440200, 0x0040120a00802080},	{0x0008500050080400, 0x0018282700021000},	{0x0010200020100800, 0x0008400c00090c00},
    {0x0020400040201000, 0x0024200440050400},	{0x0002000204081000, 0x004420104000a000},	{0x0004000408102000, 0x0020085060000800},
    {0x000a000a10204000, 0x00090428d0011800},	{0x0014001422400000, 0x00000a6248002c00},	{0x0028002844020000, 0x000140820a000400},
    {0x0050005008040200, 0x0140900848400200},	{0x0020002010080400, 0x000890400c000200},	{0x0040004020100800, 0x0008240492000080},
    {0x0000020408102000, 0x0010104980400000},	{0x0000040810204000, 0x0001480810300000},	{0x00000a1020400000, 0x0000060410a80000},
    {0x0000142240000000, 0x0000000270150000},	{0x0000284402000000, 0x000000808c540000},	{0x0000500804020000, 0x00002010202c8000},
    {0x0000201008040200, 0x0020a00810210000},	{0x0000402010080400, 0x0080801804828000},	{0x0002040810204000, 0x0002420010c09000},
    {0x0004081020400000, 0x0000080828180400},	{0x000a102040000000, 0x0000000890285800},	{0x0014224000000000, 0x0000000008234900},
    {0x0028440200000000, 0x00000000a0244500},	{0x0050080402000000, 0x0000022080900300},	{0x0020100804020000, 0x0000005002301100},
    {0x0040201008040200, 0x0080881014040040}
};

// ----------------------------------------------------------//
// Hash Table Data & Polyglot Random Numbers
// ----------------------------------------------------------//
struct t_pawn_hash_record *pawn_hash;
t_hash pawn_hash_mask;

struct t_material_hash_record *material_hash;
t_hash material_hash_mask;
t_hash material_hash_values[16][10];

struct t_hash_record *hash_table;
t_hash hash_mask;
t_nodes hash_probes;
t_nodes hash_hits;
t_nodes hash_full;
int hash_age;

t_hash hash_value[16][64];
t_hash castle_hash[16];
t_hash ep_hash[8];
t_hash white_to_move_hash;

const t_hash polyglot_random[781] = {
    t_hash(0x9D39247E33776D41), t_hash(0x2AF7398005AAA5C7), t_hash(0x44DB015024623547), t_hash(0x9C15F73E62A76AE2),
    t_hash(0x75834465489C0C89), t_hash(0x3290AC3A203001BF), t_hash(0x0FBBAD1F61042279), t_hash(0xE83A908FF2FB60CA),
    t_hash(0x0D7E765D58755C10), t_hash(0x1A083822CEAFE02D), t_hash(0x9605D5F0E25EC3B0), t_hash(0xD021FF5CD13A2ED5),
    t_hash(0x40BDF15D4A672E32), t_hash(0x011355146FD56395), t_hash(0x5DB4832046F3D9E5), t_hash(0x239F8B2D7FF719CC),
    t_hash(0x05D1A1AE85B49AA1), t_hash(0x679F848F6E8FC971), t_hash(0x7449BBFF801FED0B), t_hash(0x7D11CDB1C3B7ADF0),
    t_hash(0x82C7709E781EB7CC), t_hash(0xF3218F1C9510786C), t_hash(0x331478F3AF51BBE6), t_hash(0x4BB38DE5E7219443),
    t_hash(0xAA649C6EBCFD50FC), t_hash(0x8DBD98A352AFD40B), t_hash(0x87D2074B81D79217), t_hash(0x19F3C751D3E92AE1),
    t_hash(0xB4AB30F062B19ABF), t_hash(0x7B0500AC42047AC4), t_hash(0xC9452CA81A09D85D), t_hash(0x24AA6C514DA27500),
    t_hash(0x4C9F34427501B447), t_hash(0x14A68FD73C910841), t_hash(0xA71B9B83461CBD93), t_hash(0x03488B95B0F1850F),
    t_hash(0x637B2B34FF93C040), t_hash(0x09D1BC9A3DD90A94), t_hash(0x3575668334A1DD3B), t_hash(0x735E2B97A4C45A23),
    t_hash(0x18727070F1BD400B), t_hash(0x1FCBACD259BF02E7), t_hash(0xD310A7C2CE9B6555), t_hash(0xBF983FE0FE5D8244),
    t_hash(0x9F74D14F7454A824), t_hash(0x51EBDC4AB9BA3035), t_hash(0x5C82C505DB9AB0FA), t_hash(0xFCF7FE8A3430B241),
    t_hash(0x3253A729B9BA3DDE), t_hash(0x8C74C368081B3075), t_hash(0xB9BC6C87167C33E7), t_hash(0x7EF48F2B83024E20),
    t_hash(0x11D505D4C351BD7F), t_hash(0x6568FCA92C76A243), t_hash(0x4DE0B0F40F32A7B8), t_hash(0x96D693460CC37E5D),
    t_hash(0x42E240CB63689F2F), t_hash(0x6D2BDCDAE2919661), t_hash(0x42880B0236E4D951), t_hash(0x5F0F4A5898171BB6),
    t_hash(0x39F890F579F92F88), t_hash(0x93C5B5F47356388B), t_hash(0x63DC359D8D231B78), t_hash(0xEC16CA8AEA98AD76),
    t_hash(0x5355F900C2A82DC7), t_hash(0x07FB9F855A997142), t_hash(0x5093417AA8A7ED5E), t_hash(0x7BCBC38DA25A7F3C),
    t_hash(0x19FC8A768CF4B6D4), t_hash(0x637A7780DECFC0D9), t_hash(0x8249A47AEE0E41F7), t_hash(0x79AD695501E7D1E8),
    t_hash(0x14ACBAF4777D5776), t_hash(0xF145B6BECCDEA195), t_hash(0xDABF2AC8201752FC), t_hash(0x24C3C94DF9C8D3F6),
    t_hash(0xBB6E2924F03912EA), t_hash(0x0CE26C0B95C980D9), t_hash(0xA49CD132BFBF7CC4), t_hash(0xE99D662AF4243939),
    t_hash(0x27E6AD7891165C3F), t_hash(0x8535F040B9744FF1), t_hash(0x54B3F4FA5F40D873), t_hash(0x72B12C32127FED2B),
    t_hash(0xEE954D3C7B411F47), t_hash(0x9A85AC909A24EAA1), t_hash(0x70AC4CD9F04F21F5), t_hash(0xF9B89D3E99A075C2),
    t_hash(0x87B3E2B2B5C907B1), t_hash(0xA366E5B8C54F48B8), t_hash(0xAE4A9346CC3F7CF2), t_hash(0x1920C04D47267BBD),
    t_hash(0x87BF02C6B49E2AE9), t_hash(0x092237AC237F3859), t_hash(0xFF07F64EF8ED14D0), t_hash(0x8DE8DCA9F03CC54E),
    t_hash(0x9C1633264DB49C89), t_hash(0xB3F22C3D0B0B38ED), t_hash(0x390E5FB44D01144B), t_hash(0x5BFEA5B4712768E9),
    t_hash(0x1E1032911FA78984), t_hash(0x9A74ACB964E78CB3), t_hash(0x4F80F7A035DAFB04), t_hash(0x6304D09A0B3738C4),
    t_hash(0x2171E64683023A08), t_hash(0x5B9B63EB9CEFF80C), t_hash(0x506AACF489889342), t_hash(0x1881AFC9A3A701D6),
    t_hash(0x6503080440750644), t_hash(0xDFD395339CDBF4A7), t_hash(0xEF927DBCF00C20F2), t_hash(0x7B32F7D1E03680EC),
    t_hash(0xB9FD7620E7316243), t_hash(0x05A7E8A57DB91B77), t_hash(0xB5889C6E15630A75), t_hash(0x4A750A09CE9573F7),
    t_hash(0xCF464CEC899A2F8A), t_hash(0xF538639CE705B824), t_hash(0x3C79A0FF5580EF7F), t_hash(0xEDE6C87F8477609D),
    t_hash(0x799E81F05BC93F31), t_hash(0x86536B8CF3428A8C), t_hash(0x97D7374C60087B73), t_hash(0xA246637CFF328532),
    t_hash(0x043FCAE60CC0EBA0), t_hash(0x920E449535DD359E), t_hash(0x70EB093B15B290CC), t_hash(0x73A1921916591CBD),
    t_hash(0x56436C9FE1A1AA8D), t_hash(0xEFAC4B70633B8F81), t_hash(0xBB215798D45DF7AF), t_hash(0x45F20042F24F1768),
    t_hash(0x930F80F4E8EB7462), t_hash(0xFF6712FFCFD75EA1), t_hash(0xAE623FD67468AA70), t_hash(0xDD2C5BC84BC8D8FC),
    t_hash(0x7EED120D54CF2DD9), t_hash(0x22FE545401165F1C), t_hash(0xC91800E98FB99929), t_hash(0x808BD68E6AC10365),
    t_hash(0xDEC468145B7605F6), t_hash(0x1BEDE3A3AEF53302), t_hash(0x43539603D6C55602), t_hash(0xAA969B5C691CCB7A),
    t_hash(0xA87832D392EFEE56), t_hash(0x65942C7B3C7E11AE), t_hash(0xDED2D633CAD004F6), t_hash(0x21F08570F420E565),
    t_hash(0xB415938D7DA94E3C), t_hash(0x91B859E59ECB6350), t_hash(0x10CFF333E0ED804A), t_hash(0x28AED140BE0BB7DD),
    t_hash(0xC5CC1D89724FA456), t_hash(0x5648F680F11A2741), t_hash(0x2D255069F0B7DAB3), t_hash(0x9BC5A38EF729ABD4),
    t_hash(0xEF2F054308F6A2BC), t_hash(0xAF2042F5CC5C2858), t_hash(0x480412BAB7F5BE2A), t_hash(0xAEF3AF4A563DFE43),
    t_hash(0x19AFE59AE451497F), t_hash(0x52593803DFF1E840), t_hash(0xF4F076E65F2CE6F0), t_hash(0x11379625747D5AF3),
    t_hash(0xBCE5D2248682C115), t_hash(0x9DA4243DE836994F), t_hash(0x066F70B33FE09017), t_hash(0x4DC4DE189B671A1C),
    t_hash(0x51039AB7712457C3), t_hash(0xC07A3F80C31FB4B4), t_hash(0xB46EE9C5E64A6E7C), t_hash(0xB3819A42ABE61C87),
    t_hash(0x21A007933A522A20), t_hash(0x2DF16F761598AA4F), t_hash(0x763C4A1371B368FD), t_hash(0xF793C46702E086A0),
    t_hash(0xD7288E012AEB8D31), t_hash(0xDE336A2A4BC1C44B), t_hash(0x0BF692B38D079F23), t_hash(0x2C604A7A177326B3),
    t_hash(0x4850E73E03EB6064), t_hash(0xCFC447F1E53C8E1B), t_hash(0xB05CA3F564268D99), t_hash(0x9AE182C8BC9474E8),
    t_hash(0xA4FC4BD4FC5558CA), t_hash(0xE755178D58FC4E76), t_hash(0x69B97DB1A4C03DFE), t_hash(0xF9B5B7C4ACC67C96),
    t_hash(0xFC6A82D64B8655FB), t_hash(0x9C684CB6C4D24417), t_hash(0x8EC97D2917456ED0), t_hash(0x6703DF9D2924E97E),
    t_hash(0xC547F57E42A7444E), t_hash(0x78E37644E7CAD29E), t_hash(0xFE9A44E9362F05FA), t_hash(0x08BD35CC38336615),
    t_hash(0x9315E5EB3A129ACE), t_hash(0x94061B871E04DF75), t_hash(0xDF1D9F9D784BA010), t_hash(0x3BBA57B68871B59D),
    t_hash(0xD2B7ADEEDED1F73F), t_hash(0xF7A255D83BC373F8), t_hash(0xD7F4F2448C0CEB81), t_hash(0xD95BE88CD210FFA7),
    t_hash(0x336F52F8FF4728E7), t_hash(0xA74049DAC312AC71), t_hash(0xA2F61BB6E437FDB5), t_hash(0x4F2A5CB07F6A35B3),
    t_hash(0x87D380BDA5BF7859), t_hash(0x16B9F7E06C453A21), t_hash(0x7BA2484C8A0FD54E), t_hash(0xF3A678CAD9A2E38C),
    t_hash(0x39B0BF7DDE437BA2), t_hash(0xFCAF55C1BF8A4424), t_hash(0x18FCF680573FA594), t_hash(0x4C0563B89F495AC3),
    t_hash(0x40E087931A00930D), t_hash(0x8CFFA9412EB642C1), t_hash(0x68CA39053261169F), t_hash(0x7A1EE967D27579E2),
    t_hash(0x9D1D60E5076F5B6F), t_hash(0x3810E399B6F65BA2), t_hash(0x32095B6D4AB5F9B1), t_hash(0x35CAB62109DD038A),
    t_hash(0xA90B24499FCFAFB1), t_hash(0x77A225A07CC2C6BD), t_hash(0x513E5E634C70E331), t_hash(0x4361C0CA3F692F12),
    t_hash(0xD941ACA44B20A45B), t_hash(0x528F7C8602C5807B), t_hash(0x52AB92BEB9613989), t_hash(0x9D1DFA2EFC557F73),
    t_hash(0x722FF175F572C348), t_hash(0x1D1260A51107FE97), t_hash(0x7A249A57EC0C9BA2), t_hash(0x04208FE9E8F7F2D6),
    t_hash(0x5A110C6058B920A0), t_hash(0x0CD9A497658A5698), t_hash(0x56FD23C8F9715A4C), t_hash(0x284C847B9D887AAE),
    t_hash(0x04FEABFBBDB619CB), t_hash(0x742E1E651C60BA83), t_hash(0x9A9632E65904AD3C), t_hash(0x881B82A13B51B9E2),
    t_hash(0x506E6744CD974924), t_hash(0xB0183DB56FFC6A79), t_hash(0x0ED9B915C66ED37E), t_hash(0x5E11E86D5873D484),
    t_hash(0xF678647E3519AC6E), t_hash(0x1B85D488D0F20CC5), t_hash(0xDAB9FE6525D89021), t_hash(0x0D151D86ADB73615),
    t_hash(0xA865A54EDCC0F019), t_hash(0x93C42566AEF98FFB), t_hash(0x99E7AFEABE000731), t_hash(0x48CBFF086DDF285A),
    t_hash(0x7F9B6AF1EBF78BAF), t_hash(0x58627E1A149BBA21), t_hash(0x2CD16E2ABD791E33), t_hash(0xD363EFF5F0977996),
    t_hash(0x0CE2A38C344A6EED), t_hash(0x1A804AADB9CFA741), t_hash(0x907F30421D78C5DE), t_hash(0x501F65EDB3034D07),
    t_hash(0x37624AE5A48FA6E9), t_hash(0x957BAF61700CFF4E), t_hash(0x3A6C27934E31188A), t_hash(0xD49503536ABCA345),
    t_hash(0x088E049589C432E0), t_hash(0xF943AEE7FEBF21B8), t_hash(0x6C3B8E3E336139D3), t_hash(0x364F6FFA464EE52E),
    t_hash(0xD60F6DCEDC314222), t_hash(0x56963B0DCA418FC0), t_hash(0x16F50EDF91E513AF), t_hash(0xEF1955914B609F93),
    t_hash(0x565601C0364E3228), t_hash(0xECB53939887E8175), t_hash(0xBAC7A9A18531294B), t_hash(0xB344C470397BBA52),
    t_hash(0x65D34954DAF3CEBD), t_hash(0xB4B81B3FA97511E2), t_hash(0xB422061193D6F6A7), t_hash(0x071582401C38434D),
    t_hash(0x7A13F18BBEDC4FF5), t_hash(0xBC4097B116C524D2), t_hash(0x59B97885E2F2EA28), t_hash(0x99170A5DC3115544),
    t_hash(0x6F423357E7C6A9F9), t_hash(0x325928EE6E6F8794), t_hash(0xD0E4366228B03343), t_hash(0x565C31F7DE89EA27),
    t_hash(0x30F5611484119414), t_hash(0xD873DB391292ED4F), t_hash(0x7BD94E1D8E17DEBC), t_hash(0xC7D9F16864A76E94),
    t_hash(0x947AE053EE56E63C), t_hash(0xC8C93882F9475F5F), t_hash(0x3A9BF55BA91F81CA), t_hash(0xD9A11FBB3D9808E4),
    t_hash(0x0FD22063EDC29FCA), t_hash(0xB3F256D8ACA0B0B9), t_hash(0xB03031A8B4516E84), t_hash(0x35DD37D5871448AF),
    t_hash(0xE9F6082B05542E4E), t_hash(0xEBFAFA33D7254B59), t_hash(0x9255ABB50D532280), t_hash(0xB9AB4CE57F2D34F3),
    t_hash(0x693501D628297551), t_hash(0xC62C58F97DD949BF), t_hash(0xCD454F8F19C5126A), t_hash(0xBBE83F4ECC2BDECB),
    t_hash(0xDC842B7E2819E230), t_hash(0xBA89142E007503B8), t_hash(0xA3BC941D0A5061CB), t_hash(0xE9F6760E32CD8021),
    t_hash(0x09C7E552BC76492F), t_hash(0x852F54934DA55CC9), t_hash(0x8107FCCF064FCF56), t_hash(0x098954D51FFF6580),
    t_hash(0x23B70EDB1955C4BF), t_hash(0xC330DE426430F69D), t_hash(0x4715ED43E8A45C0A), t_hash(0xA8D7E4DAB780A08D),
    t_hash(0x0572B974F03CE0BB), t_hash(0xB57D2E985E1419C7), t_hash(0xE8D9ECBE2CF3D73F), t_hash(0x2FE4B17170E59750),
    t_hash(0x11317BA87905E790), t_hash(0x7FBF21EC8A1F45EC), t_hash(0x1725CABFCB045B00), t_hash(0x964E915CD5E2B207),
    t_hash(0x3E2B8BCBF016D66D), t_hash(0xBE7444E39328A0AC), t_hash(0xF85B2B4FBCDE44B7), t_hash(0x49353FEA39BA63B1),
    t_hash(0x1DD01AAFCD53486A), t_hash(0x1FCA8A92FD719F85), t_hash(0xFC7C95D827357AFA), t_hash(0x18A6A990C8B35EBD),
    t_hash(0xCCCB7005C6B9C28D), t_hash(0x3BDBB92C43B17F26), t_hash(0xAA70B5B4F89695A2), t_hash(0xE94C39A54A98307F),
    t_hash(0xB7A0B174CFF6F36E), t_hash(0xD4DBA84729AF48AD), t_hash(0x2E18BC1AD9704A68), t_hash(0x2DE0966DAF2F8B1C),
    t_hash(0xB9C11D5B1E43A07E), t_hash(0x64972D68DEE33360), t_hash(0x94628D38D0C20584), t_hash(0xDBC0D2B6AB90A559),
    t_hash(0xD2733C4335C6A72F), t_hash(0x7E75D99D94A70F4D), t_hash(0x6CED1983376FA72B), t_hash(0x97FCAACBF030BC24),
    t_hash(0x7B77497B32503B12), t_hash(0x8547EDDFB81CCB94), t_hash(0x79999CDFF70902CB), t_hash(0xCFFE1939438E9B24),
    t_hash(0x829626E3892D95D7), t_hash(0x92FAE24291F2B3F1), t_hash(0x63E22C147B9C3403), t_hash(0xC678B6D860284A1C),
    t_hash(0x5873888850659AE7), t_hash(0x0981DCD296A8736D), t_hash(0x9F65789A6509A440), t_hash(0x9FF38FED72E9052F),
    t_hash(0xE479EE5B9930578C), t_hash(0xE7F28ECD2D49EECD), t_hash(0x56C074A581EA17FE), t_hash(0x5544F7D774B14AEF),
    t_hash(0x7B3F0195FC6F290F), t_hash(0x12153635B2C0CF57), t_hash(0x7F5126DBBA5E0CA7), t_hash(0x7A76956C3EAFB413),
    t_hash(0x3D5774A11D31AB39), t_hash(0x8A1B083821F40CB4), t_hash(0x7B4A38E32537DF62), t_hash(0x950113646D1D6E03),
    t_hash(0x4DA8979A0041E8A9), t_hash(0x3BC36E078F7515D7), t_hash(0x5D0A12F27AD310D1), t_hash(0x7F9D1A2E1EBE1327),
    t_hash(0xDA3A361B1C5157B1), t_hash(0xDCDD7D20903D0C25), t_hash(0x36833336D068F707), t_hash(0xCE68341F79893389),
    t_hash(0xAB9090168DD05F34), t_hash(0x43954B3252DC25E5), t_hash(0xB438C2B67F98E5E9), t_hash(0x10DCD78E3851A492),
    t_hash(0xDBC27AB5447822BF), t_hash(0x9B3CDB65F82CA382), t_hash(0xB67B7896167B4C84), t_hash(0xBFCED1B0048EAC50),
    t_hash(0xA9119B60369FFEBD), t_hash(0x1FFF7AC80904BF45), t_hash(0xAC12FB171817EEE7), t_hash(0xAF08DA9177DDA93D),
    t_hash(0x1B0CAB936E65C744), t_hash(0xB559EB1D04E5E932), t_hash(0xC37B45B3F8D6F2BA), t_hash(0xC3A9DC228CAAC9E9),
    t_hash(0xF3B8B6675A6507FF), t_hash(0x9FC477DE4ED681DA), t_hash(0x67378D8ECCEF96CB), t_hash(0x6DD856D94D259236),
    t_hash(0xA319CE15B0B4DB31), t_hash(0x073973751F12DD5E), t_hash(0x8A8E849EB32781A5), t_hash(0xE1925C71285279F5),
    t_hash(0x74C04BF1790C0EFE), t_hash(0x4DDA48153C94938A), t_hash(0x9D266D6A1CC0542C), t_hash(0x7440FB816508C4FE),
    t_hash(0x13328503DF48229F), t_hash(0xD6BF7BAEE43CAC40), t_hash(0x4838D65F6EF6748F), t_hash(0x1E152328F3318DEA),
    t_hash(0x8F8419A348F296BF), t_hash(0x72C8834A5957B511), t_hash(0xD7A023A73260B45C), t_hash(0x94EBC8ABCFB56DAE),
    t_hash(0x9FC10D0F989993E0), t_hash(0xDE68A2355B93CAE6), t_hash(0xA44CFE79AE538BBE), t_hash(0x9D1D84FCCE371425),
    t_hash(0x51D2B1AB2DDFB636), t_hash(0x2FD7E4B9E72CD38C), t_hash(0x65CA5B96B7552210), t_hash(0xDD69A0D8AB3B546D),
    t_hash(0x604D51B25FBF70E2), t_hash(0x73AA8A564FB7AC9E), t_hash(0x1A8C1E992B941148), t_hash(0xAAC40A2703D9BEA0),
    t_hash(0x764DBEAE7FA4F3A6), t_hash(0x1E99B96E70A9BE8B), t_hash(0x2C5E9DEB57EF4743), t_hash(0x3A938FEE32D29981),
    t_hash(0x26E6DB8FFDF5ADFE), t_hash(0x469356C504EC9F9D), t_hash(0xC8763C5B08D1908C), t_hash(0x3F6C6AF859D80055),
    t_hash(0x7F7CC39420A3A545), t_hash(0x9BFB227EBDF4C5CE), t_hash(0x89039D79D6FC5C5C), t_hash(0x8FE88B57305E2AB6),
    t_hash(0xA09E8C8C35AB96DE), t_hash(0xFA7E393983325753), t_hash(0xD6B6D0ECC617C699), t_hash(0xDFEA21EA9E7557E3),
    t_hash(0xB67C1FA481680AF8), t_hash(0xCA1E3785A9E724E5), t_hash(0x1CFC8BED0D681639), t_hash(0xD18D8549D140CAEA),
    t_hash(0x4ED0FE7E9DC91335), t_hash(0xE4DBF0634473F5D2), t_hash(0x1761F93A44D5AEFE), t_hash(0x53898E4C3910DA55),
    t_hash(0x734DE8181F6EC39A), t_hash(0x2680B122BAA28D97), t_hash(0x298AF231C85BAFAB), t_hash(0x7983EED3740847D5),
    t_hash(0x66C1A2A1A60CD889), t_hash(0x9E17E49642A3E4C1), t_hash(0xEDB454E7BADC0805), t_hash(0x50B704CAB602C329),
    t_hash(0x4CC317FB9CDDD023), t_hash(0x66B4835D9EAFEA22), t_hash(0x219B97E26FFC81BD), t_hash(0x261E4E4C0A333A9D),
    t_hash(0x1FE2CCA76517DB90), t_hash(0xD7504DFA8816EDBB), t_hash(0xB9571FA04DC089C8), t_hash(0x1DDC0325259B27DE),
    t_hash(0xCF3F4688801EB9AA), t_hash(0xF4F5D05C10CAB243), t_hash(0x38B6525C21A42B0E), t_hash(0x36F60E2BA4FA6800),
    t_hash(0xEB3593803173E0CE), t_hash(0x9C4CD6257C5A3603), t_hash(0xAF0C317D32ADAA8A), t_hash(0x258E5A80C7204C4B),
    t_hash(0x8B889D624D44885D), t_hash(0xF4D14597E660F855), t_hash(0xD4347F66EC8941C3), t_hash(0xE699ED85B0DFB40D),
    t_hash(0x2472F6207C2D0484), t_hash(0xC2A1E7B5B459AEB5), t_hash(0xAB4F6451CC1D45EC), t_hash(0x63767572AE3D6174),
    t_hash(0xA59E0BD101731A28), t_hash(0x116D0016CB948F09), t_hash(0x2CF9C8CA052F6E9F), t_hash(0x0B090A7560A968E3),
    t_hash(0xABEEDDB2DDE06FF1), t_hash(0x58EFC10B06A2068D), t_hash(0xC6E57A78FBD986E0), t_hash(0x2EAB8CA63CE802D7),
    t_hash(0x14A195640116F336), t_hash(0x7C0828DD624EC390), t_hash(0xD74BBE77E6116AC7), t_hash(0x804456AF10F5FB53),
    t_hash(0xEBE9EA2ADF4321C7), t_hash(0x03219A39EE587A30), t_hash(0x49787FEF17AF9924), t_hash(0xA1E9300CD8520548),
    t_hash(0x5B45E522E4B1B4EF), t_hash(0xB49C3B3995091A36), t_hash(0xD4490AD526F14431), t_hash(0x12A8F216AF9418C2),
    t_hash(0x001F837CC7350524), t_hash(0x1877B51E57A764D5), t_hash(0xA2853B80F17F58EE), t_hash(0x993E1DE72D36D310),
    t_hash(0xB3598080CE64A656), t_hash(0x252F59CF0D9F04BB), t_hash(0xD23C8E176D113600), t_hash(0x1BDA0492E7E4586E),
    t_hash(0x21E0BD5026C619BF), t_hash(0x3B097ADAF088F94E), t_hash(0x8D14DEDB30BE846E), t_hash(0xF95CFFA23AF5F6F4),
    t_hash(0x3871700761B3F743), t_hash(0xCA672B91E9E4FA16), t_hash(0x64C8E531BFF53B55), t_hash(0x241260ED4AD1E87D),
    t_hash(0x106C09B972D2E822), t_hash(0x7FBA195410E5CA30), t_hash(0x7884D9BC6CB569D8), t_hash(0x0647DFEDCD894A29),
    t_hash(0x63573FF03E224774), t_hash(0x4FC8E9560F91B123), t_hash(0x1DB956E450275779), t_hash(0xB8D91274B9E9D4FB),
    t_hash(0xA2EBEE47E2FBFCE1), t_hash(0xD9F1F30CCD97FB09), t_hash(0xEFED53D75FD64E6B), t_hash(0x2E6D02C36017F67F),
    t_hash(0xA9AA4D20DB084E9B), t_hash(0xB64BE8D8B25396C1), t_hash(0x70CB6AF7C2D5BCF0), t_hash(0x98F076A4F7A2322E),
    t_hash(0xBF84470805E69B5F), t_hash(0x94C3251F06F90CF3), t_hash(0x3E003E616A6591E9), t_hash(0xB925A6CD0421AFF3),
    t_hash(0x61BDD1307C66E300), t_hash(0xBF8D5108E27E0D48), t_hash(0x240AB57A8B888B20), t_hash(0xFC87614BAF287E07),
    t_hash(0xEF02CDD06FFDB432), t_hash(0xA1082C0466DF6C0A), t_hash(0x8215E577001332C8), t_hash(0xD39BB9C3A48DB6CF),
    t_hash(0x2738259634305C14), t_hash(0x61CF4F94C97DF93D), t_hash(0x1B6BACA2AE4E125B), t_hash(0x758F450C88572E0B),
    t_hash(0x959F587D507A8359), t_hash(0xB063E962E045F54D), t_hash(0x60E8ED72C0DFF5D1), t_hash(0x7B64978555326F9F),
    t_hash(0xFD080D236DA814BA), t_hash(0x8C90FD9B083F4558), t_hash(0x106F72FE81E2C590), t_hash(0x7976033A39F7D952),
    t_hash(0xA4EC0132764CA04B), t_hash(0x733EA705FAE4FA77), t_hash(0xB4D8F77BC3E56167), t_hash(0x9E21F4F903B33FD9),
    t_hash(0x9D765E419FB69F6D), t_hash(0xD30C088BA61EA5EF), t_hash(0x5D94337FBFAF7F5B), t_hash(0x1A4E4822EB4D7A59),
    t_hash(0x6FFE73E81B637FB3), t_hash(0xDDF957BC36D8B9CA), t_hash(0x64D0E29EEA8838B3), t_hash(0x08DD9BDFD96B9F63),
    t_hash(0x087E79E5A57D1D13), t_hash(0xE328E230E3E2B3FB), t_hash(0x1C2559E30F0946BE), t_hash(0x720BF5F26F4D2EAA),
    t_hash(0xB0774D261CC609DB), t_hash(0x443F64EC5A371195), t_hash(0x4112CF68649A260E), t_hash(0xD813F2FAB7F5C5CA),
    t_hash(0x660D3257380841EE), t_hash(0x59AC2C7873F910A3), t_hash(0xE846963877671A17), t_hash(0x93B633ABFA3469F8),
    t_hash(0xC0C0F5A60EF4CDCF), t_hash(0xCAF21ECD4377B28C), t_hash(0x57277707199B8175), t_hash(0x506C11B9D90E8B1D),
    t_hash(0xD83CC2687A19255F), t_hash(0x4A29C6465A314CD1), t_hash(0xED2DF21216235097), t_hash(0xB5635C95FF7296E2),
    t_hash(0x22AF003AB672E811), t_hash(0x52E762596BF68235), t_hash(0x9AEBA33AC6ECC6B0), t_hash(0x944F6DE09134DFB6),
    t_hash(0x6C47BEC883A7DE39), t_hash(0x6AD047C430A12104), t_hash(0xA5B1CFDBA0AB4067), t_hash(0x7C45D833AFF07862),
    t_hash(0x5092EF950A16DA0B), t_hash(0x9338E69C052B8E7B), t_hash(0x455A4B4CFE30E3F5), t_hash(0x6B02E63195AD0CF8),
    t_hash(0x6B17B224BAD6BF27), t_hash(0xD1E0CCD25BB9C169), t_hash(0xDE0C89A556B9AE70), t_hash(0x50065E535A213CF6),
    t_hash(0x9C1169FA2777B874), t_hash(0x78EDEFD694AF1EED), t_hash(0x6DC93D9526A50E68), t_hash(0xEE97F453F06791ED),
    t_hash(0x32AB0EDB696703D3), t_hash(0x3A6853C7E70757A7), t_hash(0x31865CED6120F37D), t_hash(0x67FEF95D92607890),
    t_hash(0x1F2B1D1F15F6DC9C), t_hash(0xB69E38A8965C6B65), t_hash(0xAA9119FF184CCCF4), t_hash(0xF43C732873F24C13),
    t_hash(0xFB4A3D794A9A80D2), t_hash(0x3550C2321FD6109C), t_hash(0x371F77E76BB8417E), t_hash(0x6BFA9AAE5EC05779),
    t_hash(0xCD04F3FF001A4778), t_hash(0xE3273522064480CA), t_hash(0x9F91508BFFCFC14A), t_hash(0x049A7F41061A9E60),
    t_hash(0xFCB6BE43A9F2FE9B), t_hash(0x08DE8A1C7797DA9B), t_hash(0x8F9887E6078735A1), t_hash(0xB5B4071DBFC73A66),
    t_hash(0x230E343DFBA08D33), t_hash(0x43ED7F5A0FAE657D), t_hash(0x3A88A0FBBCB05C63), t_hash(0x21874B8B4D2DBC4F),
    t_hash(0x1BDEA12E35F6A8C9), t_hash(0x53C065C6C8E63528), t_hash(0xE34A1D250E7A8D6B), t_hash(0xD6B04D3B7651DD7E),
    t_hash(0x5E90277E7CB39E2D), t_hash(0x2C046F22062DC67D), t_hash(0xB10BB459132D0A26), t_hash(0x3FA9DDFB67E2F199),
    t_hash(0x0E09B88E1914F7AF), t_hash(0x10E8B35AF3EEAB37), t_hash(0x9EEDECA8E272B933), t_hash(0xD4C718BC4AE8AE5F),
    t_hash(0x81536D601170FC20), t_hash(0x91B534F885818A06), t_hash(0xEC8177F83F900978), t_hash(0x190E714FADA5156E),
    t_hash(0xB592BF39B0364963), t_hash(0x89C350C893AE7DC1), t_hash(0xAC042E70F8B383F2), t_hash(0xB49B52E587A1EE60),
    t_hash(0xFB152FE3FF26DA89), t_hash(0x3E666E6F69AE2C15), t_hash(0x3B544EBE544C19F9), t_hash(0xE805A1E290CF2456),
    t_hash(0x24B33C9D7ED25117), t_hash(0xE74733427B72F0C1), t_hash(0x0A804D18B7097475), t_hash(0x57E3306D881EDB4F),
    t_hash(0x4AE7D6A36EB5DBCB), t_hash(0x2D8D5432157064C8), t_hash(0xD1E649DE1E7F268B), t_hash(0x8A328A1CEDFE552C),
    t_hash(0x07A3AEC79624C7DA), t_hash(0x84547DDC3E203C94), t_hash(0x990A98FD5071D263), t_hash(0x1A4FF12616EEFC89),
    t_hash(0xF6F7FD1431714200), t_hash(0x30C05B1BA332F41C), t_hash(0x8D2636B81555A786), t_hash(0x46C9FEB55D120902),
    t_hash(0xCCEC0A73B49C9921), t_hash(0x4E9D2827355FC492), t_hash(0x19EBB029435DCB0F), t_hash(0x4659D2B743848A2C),
    t_hash(0x963EF2C96B33BE31), t_hash(0x74F85198B05A2E7D), t_hash(0x5A0F544DD2B1FB18), t_hash(0x03727073C2E134B1),
    t_hash(0xC7F6AA2DE59AEA61), t_hash(0x352787BAA0D7C22F), t_hash(0x9853EAB63B5E0B35), t_hash(0xABBDCDD7ED5C0860),
    t_hash(0xCF05DAF5AC8D77B0), t_hash(0x49CAD48CEBF4A71E), t_hash(0x7A4C10EC2158C4A6), t_hash(0xD9E92AA246BF719E),
    t_hash(0x13AE978D09FE5557), t_hash(0x730499AF921549FF), t_hash(0x4E4B705B92903BA4), t_hash(0xFF577222C14F0A3A),
    t_hash(0x55B6344CF97AAFAE), t_hash(0xB862225B055B6960), t_hash(0xCAC09AFBDDD2CDB4), t_hash(0xDAF8E9829FE96B5F),
    t_hash(0xB5FDFC5D3132C498), t_hash(0x310CB380DB6F7503), t_hash(0xE87FBB46217A360E), t_hash(0x2102AE466EBB1148),
    t_hash(0xF8549E1A3AA5E00D), t_hash(0x07A69AFDCC42261A), t_hash(0xC4C118BFE78FEAAE), t_hash(0xF9F4892ED96BD438),
    t_hash(0x1AF3DBE25D8F45DA), t_hash(0xF5B4B0B0D2DEEEB4), t_hash(0x962ACEEFA82E1C84), t_hash(0x046E3ECAAF453CE9),
    t_hash(0xF05D129681949A4C), t_hash(0x964781CE734B3C84), t_hash(0x9C2ED44081CE5FBD), t_hash(0x522E23F3925E319E),
    t_hash(0x177E00F9FC32F791), t_hash(0x2BC60A63A6F3B3F2), t_hash(0x222BBFAE61725606), t_hash(0x486289DDCC3D6780),
    t_hash(0x7DC7785B8EFDFC80), t_hash(0x8AF38731C02BA980), t_hash(0x1FAB64EA29A2DDF7), t_hash(0xE4D9429322CD065A),
    t_hash(0x9DA058C67844F20C), t_hash(0x24C0E332B70019B0), t_hash(0x233003B5A6CFE6AD), t_hash(0xD586BD01C5C217F6),
    t_hash(0x5E5637885F29BC2B), t_hash(0x7EBA726D8C94094B), t_hash(0x0A56A5F0BFE39272), t_hash(0xD79476A84EE20D06),
    t_hash(0x9E4C1269BAA4BF37), t_hash(0x17EFEE45B0DEE640), t_hash(0x1D95B0A5FCF90BC6), t_hash(0x93CBE0B699C2585D),
    t_hash(0x65FA4F227A2B6D79), t_hash(0xD5F9E858292504D5), t_hash(0xC2B5A03F71471A6F), t_hash(0x59300222B4561E00),
    t_hash(0xCE2F8642CA0712DC), t_hash(0x7CA9723FBB2E8988), t_hash(0x2785338347F2BA08), t_hash(0xC61BB3A141E50E8C),
    t_hash(0x150F361DAB9DEC26), t_hash(0x9F6A419D382595F4), t_hash(0x64A53DC924FE7AC9), t_hash(0x142DE49FFF7A7C3D),
    t_hash(0x0C335248857FA9E7), t_hash(0x0A9C32D5EAE45305), t_hash(0xE6C42178C4BBB92E), t_hash(0x71F1CE2490D20B07),
    t_hash(0xF1BCC3D275AFE51A), t_hash(0xE728E8C83C334074), t_hash(0x96FBF83A12884624), t_hash(0x81A1549FD6573DA5),
    t_hash(0x5FA7867CAF35E149), t_hash(0x56986E2EF3ED091B), t_hash(0x917F1DD5F8886C61), t_hash(0xD20D8C88C8FFE65F),
    t_hash(0x31D71DCE64B2C310), t_hash(0xF165B587DF898190), t_hash(0xA57E6339DD2CF3A0), t_hash(0x1EF6E6DBB1961EC9),
    t_hash(0x70CC73D90BC26E24), t_hash(0xE21A6B35DF0C3AD7), t_hash(0x003A93D8B2806962), t_hash(0x1C99DED33CB890A1),
    t_hash(0xCF3145DE0ADD4289), t_hash(0xD0E4427A5514FB72), t_hash(0x77C621CC9FB3A483), t_hash(0x67A34DAC4356550B),
    t_hash(0xF8D626AAAF278509),
};
