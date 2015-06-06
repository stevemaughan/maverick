//===========================================================//
//
// Maverick Chess Engine
// Copyright 2013-2015 Steve Maughan
//
//===========================================================//

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "defs.h"
#include "data.h"
#include "procs.h"

BOOL is_futile(struct t_pv_data *pv, struct t_pv_data *next_pv, int depth, t_chess_value alpha, t_chess_value beta){

	//-- Too far from Horizon
	if (depth > 4)
		return FALSE;

	//-- Legal Move Count
	if (pv->legal_moves_played <= 1)
		return FALSE;

	//-- In Check before or after move
	if (next_pv->in_check || pv->in_check)
		return FALSE;

	//-- Mate Possibilities
	if (alpha <= -MAX_CHECKMATE || beta >= MAX_CHECKMATE)
		return FALSE;

	////-- Capture
	struct t_move_record *move = pv->current_move;
	//if (move->captured && depth > 0)
	//	return FALSE;

	//-- Pawn Push
	if (PIECETYPE(move->piece) == PAWN && COLOR_RANK(COLOR(move->piece), move->to_square) >= 6)
		return FALSE;

	//-- Killer Moves
	if (move == pv->killer1 || move == pv->killer2)
		return FALSE;

	//-- Could score surpass alpha?
	return (pv->eval->static_score + see_piece_value[move->captured] + futility_margin[depth] <= alpha);

}
