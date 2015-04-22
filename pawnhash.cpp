//===========================================================//
//
// Maverick Chess Engine
// Copyright 2013-2015 Steve Maughan
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

#ifndef min
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

struct t_pawn_hash_record *lookup_pawn_hash(struct t_board *board, struct t_chess_eval *eval)
{
    t_chess_color color;

    // Look-up in pawn hash table
    struct t_pawn_hash_record *pawn_record = &pawn_hash[board->pawn_hash & pawn_hash_mask];
    
	// See if already exists
    if (pawn_record->key == board->pawn_hash) {
        for (color = WHITE; color <= BLACK; color++) {
            eval->attacks[color][PAWN] = pawn_record->attacks[color];
			eval->king_attack_pressure[color] = pawn_record->king_pressure[color];
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
		pawn_record->king_pressure[color] = 0;
		opponent = OPPONENT(color);
        p = board->pieces[color][PAWN];
		b = (((p & B8H1) << 7) >> (16 * color)) | (((p & A8G1) << 9) >> (16 * color));
		pawn_record->attacks[color] = b;
    }
	
	//-- Blocked
	for (color = WHITE; color <= BLACK; color++) {
		t_bitboard stops = (board->pieces[color][PAWN] << 8) >> (16 * color);
		stops &= (board->piecelist[WHITEPAWN] | board->piecelist[BLACKPAWN]);
		pawn_record->blocked[color] = (stops >> 8) << (16 * color);
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

			middlegame += bonus / 2 ;
			endgame += bonus;

			//-- Connected passed pawn bonus
			if (connected_pawn_mask[s] & pawn_record->passed[color]){
				middlegame += bonus / 5;
				endgame += (2 * bonus) / 3;
			}
		}
    }

	// Backward
	for (color = WHITE; color <= BLACK; color++) {

		t_bitboard stops = (board->pieces[color][PAWN] << 8) >> (16 * color);

		b = (stops & (pawn_record->attacks[OPPONENT(color)] | board->piecelist[WHITEPAWN] | board->piecelist[BLACKPAWN]) & ~fwd_attacks[color]);
		b = (b >> 8) << (16 * color);
		pawn_record->backward[color] = b;
		pawn_record->weak[color] = b;

		int penalty = popcount(b) * MG_BACKWARD_PAWN * (1 - color * 2);

		middlegame += penalty;
		endgame += penalty;
	}

    // double (first type of weak pawns)
    for (color = WHITE; color <= BLACK; color++) {
		b = (board->pieces[color][PAWN] & pawn_record->backward_squares[color]);
		pawn_record->double_pawns[color] = b;
		pawn_record->weak[color] = b;

		int penalty = -37 * popcount(b) * (1 - 2 * color);

		middlegame += penalty;
		endgame += penalty;

		//-- Double and backward!!!!
		b &= pawn_record->backward[color];
		penalty = -10 * popcount(b) * (1 - 2 * color);

		middlegame += penalty;
		endgame += penalty;

		//-- Calculate the double pawns on semi open files
		pawn_record->semi_open_double_pawns[color] = popcount(b & ~pawn_record->forward_squares[OPPONENT(color)]);
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

		int count = popcount(b);
		middlegame += isolated_pawn[MIDDLEGAME][count] * (1 - color * 2);
		endgame += isolated_pawn[ENDGAME][count] * (1 - color * 2);
    }
    

    //- Piece Square values
    for (color = WHITE; color <= BLACK; color++) {

		//-- King
		middlegame += piece_square_table[PIECEINDEX(color, KING)][MIDDLEGAME][board->king_square[color]] * (1 - color * 2);
		endgame += piece_square_table[PIECEINDEX(color, KING)][ENDGAME][board->king_square[color]] * (1 - color * 2);

		//-- Pawns
        b = board->pieces[color][PAWN];
        while (b) {
            s = bitscan_reset(&b);
            middlegame += piece_square_table[PIECEINDEX(color, PAWN)][MIDDLEGAME][s] * (1 - color * 2);
            endgame += piece_square_table[PIECEINDEX(color, PAWN)][ENDGAME][s] * (1 - color * 2);
        }
    }

	//-- Potential Outposts
	for (color = WHITE; color <= BLACK; color++)
		pawn_record->potential_outpost[color] = candidate_outposts[color] & pawn_record->attacks[color] & ~fwd_attacks[OPPONENT(color)];

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

	//-- Pawn Storm 
	eval_pawn_storm(board, pawn_record);

	//-- Pawn Shield
	eval_pawn_shelter(board, pawn_record);

	//-- Calculate the score if it were a king & pawn endgame
	evaluate_king_pawn_endgame(board, pawn_record);

    //-- Transfer key Bitboard to the Board structure
    for (color = WHITE; color <= BLACK; color++) {
        eval->attacks[color][PAWN] = pawn_record->attacks[color];
		eval->king_attack_pressure[color] += pawn_record->king_pressure[color];
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
			pawn_hash[i].potential_outpost[c] = 0;
            pawn_hash[i].double_pawns[c] = 0;
			pawn_hash[i].semi_open_double_pawns[c] = 0;
            pawn_hash[i].attacks[c] = 0;
            pawn_hash[i].weak[c] = 0;
			pawn_hash[i].semi_open_file[c] = 0;
			pawn_hash[i].pawn_count[c] = 0;
			pawn_hash[i].king_pressure[c] = 0;
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

//-- Evaluate pawn storm when castling on oppossite sides of the board
void eval_pawn_storm(struct t_board *board, struct t_pawn_hash_record *pawn_record)
{
	t_bitboard w = 0, b = 0;

	if (board->castling)
	{
		return;
	}
	//-- White castled kingside and Black castled queenside
	else if ((board->pieces[WHITE][KING] & BITBOARD_KINGSIDE) && (board->pieces[BLACK][KING] & BITBOARD_QUEENSIDE)){
		w = (board->piecelist[WHITEPAWN] & BITBOARD_QUEENSIDE);
		b = (board->piecelist[BLACKPAWN] & BITBOARD_KINGSIDE);
	}
	else if ((board->pieces[WHITE][KING] & BITBOARD_QUEENSIDE) && (board->pieces[BLACK][KING] & BITBOARD_KINGSIDE)){
		w = (board->piecelist[WHITEPAWN] & BITBOARD_KINGSIDE);
		b = (board->piecelist[BLACKPAWN] & BITBOARD_QUEENSIDE);
	}
	else
		return;

	int s;
	t_chess_value middlegame = 0;

	//-- White bonus for advancing pawns 
	if (w){
		while (w){
			s = bitscan_reset(&w);
			middlegame += pawn_storm[RANK(s)];
		}
	}

	//-- Black bonus for advancing pawns
	if (b)
	{
		while (b)
		{
			s = bitscan_reset(&b);
			middlegame -= pawn_storm[7 - RANK(s)];
		}
	}

	//-- Store the data in pawn record
	pawn_record->middlegame += middlegame;

}

//-- Evaluate Pawn Shelter
void eval_pawn_shelter(struct t_board *board, struct t_pawn_hash_record *pawn_record)
{

	//- Do this for black and white
	for (t_chess_color color = WHITE; color <= BLACK; color++){

		t_chess_value middlegame = 0;
		t_chess_color opponent = OPPONENT(color);

		for (int castle_side = KINGSIDE; castle_side <= QUEENSIDE; castle_side++){

			//-- Has the king castled kingside?
			if (board->pieces[color][KING] & king_castle_squares[color][castle_side]){

				//-- Bonus if pawn shield is in tact
				if ((board->pieces[color][PAWN] & intact_pawns[color][castle_side]) == intact_pawns[color][castle_side]){
					middlegame += MG_INTACT_PAWN_SHIELD;
				}

				//-- Panalty if there isn't a decent pawn shield
				else if (!(board->pieces[color][PAWN] & wrecked_pawns[color][castle_side])){
					middlegame += MG_PAWN_SHIELD_WRECKED;
					pawn_record->king_pressure[color] += 50;
				}

				//-- See if opponent has a F6 wedge
				if (board->pieces[opponent][PAWN] & pawn_wedge_mask[opponent][castle_side]){
					middlegame -= MG_F6_PAWN_WEDGE;
					pawn_record->king_pressure[color] += 50;
				}
			}

		}

		////-- Extra penalty for isolated square on the same side as the king
		//if (BITBOARD_KINGSIDE & board->pieces[color][KING])
		//	middlegame += popcount(pawn_record->isolated[color] & BITBOARD_KINGSIDE) * -15;

		//else if (BITBOARD_QUEENSIDE & board->pieces[color][KING])
		//	middlegame += popcount(pawn_record->isolated[color] & BITBOARD_QUEENSIDE) * -15;

		pawn_record->middlegame += middlegame * (1 - 2 * color);
	}
}

void evaluate_king_pawn_endgame(struct t_board *board, struct t_pawn_hash_record *pawn_record)
{

	t_chess_value score = 0;
	t_chess_square s;

	int unstoppable_rank[2] = { 0, 0 };

	for (t_chess_color color = WHITE; color <= BLACK; color++) {

		t_chess_color opponent = OPPONENT(color);
		t_bitboard king_zone = king_mask[board->king_square[color]];

		//-- Value of king
		score += piece_square_table[PIECEINDEX(color, KING)][ENDGAME][board->king_square[color]] * (1 - color * 2);

		//-- Calculate the different types of pawns
		t_bitboard unstoppables = pawn_record->passed[color] & cannot_catch_pawn_mask[opponent][board->to_move][board->king_square[opponent]];
		t_bitboard passed_pawns = pawn_record->passed[color] ^ unstoppables;
		t_bitboard other_pawns = board->pieces[color][PAWN] ^ unstoppables ^ passed_pawns;
		t_bitboard weak_pawns = pawn_record->weak[color] & other_pawns;

		//-- Evaluate the unstoppables
		while (unstoppables){
			s = bitscan_reset(&unstoppables);

			int rank = RANK(s);
			if (color)
				rank = 7 - rank;

			unstoppable_rank[color] = max(unstoppable_rank[color], rank);
			score += passed_pawn_bonus[color][s] + piece_square_table[PIECEINDEX(color, PAWN)][ENDGAME][s] * (1 - color * 2);
		}

		//-- ...which leaves the other passed pawns
		while (passed_pawns){
			s = bitscan_reset(&passed_pawns);

			//-- Does the king protect the path to promotion?
			if ((forward_squares[color][s] & king_zone) == forward_squares[color][s])
			{
				int rank = RANK(s);
				if (color)
					rank = 7 - rank;

				unstoppable_rank[color] = max(unstoppable_rank[color], rank);
				score += passed_pawn_bonus[color][s] + piece_square_table[PIECEINDEX(color, PAWN)][ENDGAME][s] * (1 - color * 2);
			}
			else
				score += passed_pawn_bonus[color][s] + piece_square_table[PIECEINDEX(color, PAWN)][ENDGAME][s] * (1 - color * 2);
		}

		//-- Evaluate the rest of the pawns
		while (other_pawns){
			s = bitscan_reset(&other_pawns);
			score += piece_square_table[PIECEINDEX(color, PAWN)][ENDGAME][s] * (1 - color * 2);
		}

		//-- Penalize weak pawns
		score += popcount(weak_pawns) * -20 * (1 - color * 2);
	}

	//-- Race to promotion!
	if (unstoppable_rank[WHITE] || unstoppable_rank[BLACK]){

		//-- Does White Win?
		if ((unstoppable_rank[WHITE] + (board->to_move == WHITE)) > (unstoppable_rank[BLACK] + (board->to_move == BLACK)))
			score += (300 + 20 * unstoppable_rank[WHITE]);
		else
			score -= (300 + 20 * unstoppable_rank[BLACK]);
	}

	//-- Calculate the score from the perspective of the side to move
	pawn_record->king_pawn_endgame_score = score * (1 - board->to_move * 2);
}