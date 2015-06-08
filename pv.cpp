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

void update_best_line(struct t_board *board, int ply)
{

    struct t_pv_data *pv = &(board->pv_data[ply]);
    struct t_pv_data *pvn = &(board->pv_data[ply + 1]);
    int i;

    pv->best_line[ply] = pv->current_move;
    for (i = ply + 1; i < pvn->best_line_length; i++)
        pv->best_line[i] = pvn->best_line[i];
    pv->best_line_length = pvn->best_line_length;
}

void update_best_line_from_hash(struct t_board *board, int ply)
{
    struct t_pv_data *pv = &(board->pv_data[ply]);
    t_move_record *move;
    t_hash_record *hash_record;
    t_undo undo[1];

    hash_record = probe(board->hash);
    if (hash_record != NULL && hash_record->bound == HASH_EXACT && hash_record->move != NULL) {
        move = hash_record->move;
        hash_record->age = hash_age;
        if (is_move_legal(board, move))
            make_move(board, 0, move, undo);
        else
            return;
        pv->best_line[pv->best_line_length++] = move;
        if (!repetition_draw(board))
            update_best_line_from_hash(board, ply);
        unmake_move(board, undo);
    }
}

BOOL pv_not_resolved(int legal_moves_played, t_chess_value e, t_chess_value alpha, t_chess_value beta) {

    if (legal_moves_played == 0)
        return ((e <= alpha) || (e >= beta));

    return (e >= beta);
}


BOOL research_required(struct t_board *board, struct t_multi_pv *mpv, int index, t_chess_value score, t_chess_value alpha, t_chess_value beta, int *fail_high_count, int *fail_low_count) {

    //-- Fail High
    if (score >= beta) {
        *fail_high_count++;
        return TRUE;
    }

    //-- Fail Low
    if (score <= alpha) {
        if (index < mpv->count) {
            *fail_low_count++;
            return TRUE;
        }
        return FALSE;
    }

    //-- Exact PV found (i.e. alpha < score < beta)
    struct t_pv_data *pv = &(board->pv_data[0]);
    struct t_pv_data *pvn = &(board->pv_data[1]);

    mpv->pv[index].score = score;
    mpv->pv[index].pv_length = pvn->best_line_length;
    mpv->pv[index].move[0] = pv->current_move;

    for (int i = 1; i < pvn->best_line_length; i++)
        mpv->pv[index].move[i] = pvn->best_line[i];

    //-- If multi-pv mode then sort lines
    if (mpv->count > 1) {

        //-- Sort the top lines
        //quicksort_variations(mpv, index);
    }
    else {
        update_best_line(board, 0);
        do_uci_bestmove(board);
    }

    return FALSE;
}

void get_bounds(t_chess_value base_score, int moves_played, int fail_high_count, int fail_low_count, t_chess_value *alpha, t_chess_value *beta) {

	//-- First Move, so need to resolve
	if (moves_played == 1){

		//-- Main search has failed high and low - so search +/- INF
		if (fail_high_count * fail_low_count != 0){
			*alpha = -CHECKMATE;
			*beta = CHECKMATE;
		}

		//-- Checkmate found
		else if (base_score >= MAX_CHECKMATE){
			*alpha = base_score - 2;
			*beta = CHECKMATE;
		}

		else if (base_score <= -MAX_CHECKMATE){
			*alpha = -CHECKMATE;
			*beta = base_score + 2;
		}

		//-- Fail High
		else if (fail_high_count){
			if (fail_high_count >= 4)
				*beta = CHECKMATE;
			else
				*beta = base_score + aspiration_window[fail_high_count];
		}

		//-- Fail Low
		else if (fail_low_count){
			if (fail_low_count >= 4)
				*alpha = -CHECKMATE;
			else
				*alpha = base_score - aspiration_window[fail_low_count];
		}

		else{
			*alpha = -CHECKMATE;
			*beta = CHECKMATE;
		}

	}

	//-- Other Moves so only need to resolve fail high
	else{

		//-- Normal move
		if (fail_high_count == 0){
			*alpha = base_score;
			*beta = base_score + 1;
		}

		//-- Fail high
		else if (fail_high_count >= 4)
			*beta = CHECKMATE;
		else
			*beta = base_score + aspiration_window[fail_high_count];
		
	}
}
