//===========================================================//
//
// Maverick Chess Engine
// Copyright 2013 Steve Maughan
//
//===========================================================//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "defs.h"
#include "eval.h"
#include "data.h"
#include "procs.h"
#include "bittwiddle.h"

struct t_pawn_hash_record *lookup_pawn_hash(struct t_board *board, struct t_chess_eval *eval)
{
    t_chess_color color;

    // Look-up in pawn hash table
    struct t_pawn_hash_record *pawn_record = &pawn_hash[board->pawn_hash & pawn_hash_mask];
    // See if already exists
    if (pawn_record->key == board->pawn_hash) {
        for (color = WHITE; color <= BLACK; color++) {
            eval->attacks[color][PAWN] = pawn_record->attacks[color];
        }
        return pawn_record;
    }

    // Create all of the goodies!!
    t_chess_color opponent;
    t_chess_square s;
    t_bitboard b, w, p;
    t_chess_value middlegame = 0;
    t_chess_value endgame = 0;
	t_chess_value bonus;
    t_bitboard fwd_attacks[2];
    t_bitboard bkw_attacks[2];
    int c;

    pawn_record->key = board->pawn_hash;

    // forward bitmaps
    w = board->piecelist[WHITEPAWN];
	pawn_record->pawn_count[WHITE] = popcount(w);
    pawn_record->forward_squares[WHITE] = (w << 8) | (w << 16) | (w << 24) | (w << 32) | (w << 40) | (w << 48);
    b = board->piecelist[BLACKPAWN];
	pawn_record->pawn_count[BLACK] = popcount(b);
    pawn_record->forward_squares[BLACK] = (b >> 8) | (b >> 16) | (b >> 24) | (b >> 32) | (b >> 40) | (b >> 48);

    // backward bitmaps
    w = board->piecelist[WHITEPAWN];
    pawn_record->backward_squares[WHITE] = (w >> 8) | (w >> 16) | (w >> 24) | (w >> 32) | (w >> 40) | (w >> 48);

    b = board->piecelist[BLACKPAWN];
    pawn_record->backward_squares[BLACK] = (b << 8) | (b << 16) | (b << 24) | (b << 32) | (b << 40) | (b << 48);

    //--Forward & Backward attacks
    for (color = WHITE; color <= BLACK; color++) {
        fwd_attacks[color] = ((A8G1 & pawn_record->forward_squares[color]) << 1) | ((B8H1 & pawn_record->forward_squares[color]) >> 1);
        bkw_attacks[color] = ((A8G1 & (pawn_record->backward_squares[color] | board->pieces[color][PAWN])) << 1) | ((B8H1 & (pawn_record->forward_squares[color] | board->pieces[color][PAWN])) >> 1);
    }

    // attacks
    for (color = WHITE; color <= BLACK; color++) {
        p = board->pieces[color][PAWN];
        pawn_record->attacks[color] = (((p & B8H1) << 7) >> (16 * color)) | (((p & A8G1) << 9) >> (16 * color));
    }

    // passed
    for (color = WHITE; color <= BLACK; color++) {
        opponent = OPPONENT(color);
        pawn_record->passed[color] = board->pieces[color][PAWN] & ~(fwd_attacks[opponent] | pawn_record->forward_squares[opponent] | pawn_record->backward_squares[color]);

		//-- Add the basic passed pawn bonus
		b = pawn_record->passed[color];
		while (b){
			s = bitscan_reset(&b);
			bonus = passed_pawn_bonus[color][s];
			middlegame += bonus;
			endgame += bonus;

			//-- Connected passed pawn bonus
			if (connected_pawn_mask[s] & pawn_record->passed[color]){

				//** Tested 2013-09-14
				// Base = /1, /1
				// New1 = 3/2, 3/2
				// New2 = 4/3, 4/3
				// New3 = 3/4, 3/4
				// New4 = /2, /2 (BEST! +25 ELO)
				// So connected bonus must be less than the bonus for the passed pawn
				// New1 = /3, /3
				// New2 = /3, /2
				// New3 = /4, /2 (BEST)
				// New4 = /4, /3
				// Try even more options
				// New1 = /4, 2/3
				// New2 = /5, /2
				// New3 = /4, /2
				// New4 = /5, 2/3
				middlegame += bonus / 5;
				endgame += (2 * bonus) / 3;
			}
		}
    }

    // double (first type of weak pawns)
    for (color = WHITE; color <= BLACK; color++) {
		b = (board->pieces[color][PAWN] & pawn_record->backward_squares[color]);
		pawn_record->double_pawns[color] = b;
		pawn_record->weak[color] = b;

		while (b){
			s = bitscan_reset(&b);
			middlegame += double_pawn_penalty[MIDDLEGAME][COLUMN(s)] * (1 - 2 * color);
			endgame += double_pawn_penalty[ENDGAME][COLUMN(s)] * (1 - 2 * color);
		}
    }

    // Isolated Pawns
    for (color = WHITE; color <= BLACK; color++) {
        b = 0;
        p = board->pieces[color][PAWN];
        for (c = 0; c < 8; c++) {
            if (!(p & neighboring_file[c]))
                b |= (p & column_mask[c]);
        }
        pawn_record->isolated[color] = b;
        pawn_record->weak[color] |= b;
    }
    int count = popcount(pawn_record->isolated[WHITE]) - popcount(pawn_record->isolated[BLACK]);

    middlegame += count * MG_ISOLATED_PAWN;
    endgame += count * EG_ISOLATED_PAWN;

    // Backward

    //--TO DO!!

    //- Piece Square values
    for (color = WHITE; color <= BLACK; color++) {
        b = board->pieces[color][PAWN];
        while (b) {
            s = bitscan_reset(&b);
            middlegame += piece_square_table[PIECEINDEX(color, PAWN)][MIDDLEGAME][s] * (1 - color * 2);
            endgame += piece_square_table[PIECEINDEX(color, PAWN)][ENDGAME][s] * (1 - color * 2);
        }
    }

	//-- Open file
	t_bitboard pawn_file[2];
	for (color = WHITE; color <= BLACK; color++) {
		pawn_file[color] = board->pieces[color][PAWN] | pawn_record->forward_squares[color] | pawn_record->backward_squares[color];
	}
	pawn_record->open_file = ~(pawn_file[WHITE] | pawn_file[BLACK]);

	//-- Semi Open File
	for (color = WHITE; color <= BLACK; color++) {
		pawn_record->semi_open_file[color] = (pawn_file[WHITE] ^ pawn_file[BLACK]) & pawn_file[OPPONENT(color)];
	}

    //-- Transfer to Pawn Record
    pawn_record->middlegame = middlegame;
    pawn_record->endgame = endgame;

    //-- Transfer key Bitboard to the Board structure
    for (color = WHITE; color <= BLACK; color++) {
        eval->attacks[color][PAWN] = pawn_record->attacks[color];
    }

    return pawn_record;

}

void init_pawn_hash()
{
    pawn_hash = NULL;
    set_pawn_hash(8);
}

void destroy_pawn_hash()
{
    if (uci.engine_initialized)
        free(pawn_hash);
}

void set_pawn_hash(unsigned int size)
{
    t_hash i;

    if (uci.options.pawn_hash_table_size == size) return;

    i = 1;
    while ((i << 1) * sizeof(struct t_pawn_hash_record) <= size * 1024 * 1024)
        (i <<= 1);

    free(pawn_hash);
    pawn_hash = (struct t_pawn_hash_record*)malloc(i * sizeof(struct t_pawn_hash_record));
	assert(pawn_hash);
    pawn_hash_mask = i - 1;
    for (i = 0; i <= pawn_hash_mask; i++) {
        pawn_hash[i].key = 0;
        pawn_hash[i].endgame = 0;
        pawn_hash[i].middlegame = 0;
		pawn_hash[i].open_file = 0;
        for (t_chess_color c = WHITE; c <= BLACK; c++) {
            pawn_hash[i].backward[c] = 0;
            pawn_hash[i].blocked[c] = 0;
            pawn_hash[i].forward_squares[c] = 0;
            pawn_hash[i].backward_squares[c] = 0;
            pawn_hash[i].isolated[c] = 0;
            pawn_hash[i].passed[c] = 0;
            pawn_hash[i].candidate_passed[c] = 0;
            pawn_hash[i].double_pawns[c] = 0;
            pawn_hash[i].attacks[c] = 0;
            pawn_hash[i].weak[c] = 0;
			pawn_hash[i].semi_open_file[c] = 0;
			pawn_hash[i].pawn_count[c] = 0;
        }
    }
    uci.options.pawn_hash_table_size = size;
}

t_hash calc_pawn_hash(struct t_board *board) {
    t_hash zobrist = 0;
    t_chess_square s;
    t_chess_color color;
    t_bitboard b;

	//-- To Move
	if (board->to_move == WHITE)
		zobrist = white_to_move_hash;

    //-- Pieces
    for (color = WHITE; color <= BLACK; color++) {
		zobrist ^= hash_value[PIECEINDEX(color, KING)][board->king_square[color]];
        b = board->pieces[color][PAWN];
        while (b) {
            s = bitscan_reset(&b);
            zobrist ^= hash_value[PIECEINDEX(color, PAWN)][s];
        }
    }

    return zobrist;
}
