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

void order_moves(struct t_board *board, struct t_move_list *move_list, int ply) {

    struct t_move_record *hash_move = move_list->hash_move;
    struct t_move_record *move;

    struct t_move_record *killer1 = board->pv_data[ply].killer1;
    struct t_move_record *killer2 = board->pv_data[ply].killer2;
    struct t_move_record *killer3 = NULL;
    struct t_move_record *killer4 = NULL;


    struct t_move_record *refutation = NULL;
    if (board->pv_data[ply - 1].current_move)
        refutation = board->pv_data[ply - 1].current_move->refutation;

    if (ply > 1) {
        killer3 = board->pv_data[ply - 2].killer1;
        killer4 = board->pv_data[ply - 2].killer2;
    }

    for (int i = move_list->count - 1; i >= 0; i--)
    {
        move = move_list->move[i];
		assert(move);
        if (move == hash_move) {
            move_list->value[i] = MOVE_ORDER_HASH;
        }
        else if (move->captured) {
            move_list->value[i] = move->mvvlva + MOVE_ORDER_CAPTURE;
        }
        else if (move == killer1)
            move_list->value[i] = MOVE_ORDER_KILLER1;
        else if (move == killer2)
            move_list->value[i] = MOVE_ORDER_KILLER2;
        else if (move == refutation)
            move_list->value[i] = MOVE_ORDER_REFUTATION;
        else if (move == killer3)
            move_list->value[i] = MOVE_ORDER_KILLER3;
        else if (move == killer4)
            move_list->value[i] = MOVE_ORDER_KILLER4;
        else
            move_list->value[i] = move->history;
    }
}

void order_evade_check(struct t_board *board, struct t_move_list *move_list, int ply) {

    struct t_move_record *hash_move = move_list->hash_move;
    struct t_move_record *move;
    struct t_move_record *killer1 = board->pv_data[ply].check_killer1;
    struct t_move_record *killer2 = board->pv_data[ply].check_killer2;
    struct t_move_record *killer3 = NULL;
    struct t_move_record *killer4 = NULL;

	struct t_move_record *refutation = NULL;
	if (board->pv_data[ply - 1].current_move)
		refutation = board->pv_data[ply - 1].current_move->refutation;

	if (ply > 1) {
        killer3 = board->pv_data[ply - 2].check_killer1;
        killer4 = board->pv_data[ply - 2].check_killer2;
    }

    for (int i = move_list->count - 1; i >= 0; i--)
    {
        move = move_list->move[i];
		assert(move);
        if (move == hash_move) {
            move_list->value[i] = MOVE_ORDER_HASH;
        }
        else if (move->captured) {
            move_list->value[i] = move->mvvlva + MOVE_ORDER_CAPTURE;
        }
        else if (move == killer1)
            move_list->value[i] = MOVE_ORDER_KILLER1;
        else if (move == killer2)
            move_list->value[i] = MOVE_ORDER_KILLER2;
		else if (move == refutation)
			move_list->value[i] = MOVE_ORDER_REFUTATION;        
		else if (move == killer3)
            move_list->value[i] = MOVE_ORDER_KILLER3;
        else if (move == killer4)
            move_list->value[i] = MOVE_ORDER_KILLER4;
        else
            move_list->value[i] = move->history;
    }
}

void order_captures(struct t_board *board, struct t_move_list *move_list) {

    for (int i = move_list->count - 1; i >= 0; i--)
        move_list->value[i] = move_list->move[i]->mvvlva;

}

void age_history_scores()
{
    for (int i = 0; i < GLOBAL_MOVE_COUNT; i++) {
        xmove_list[i].history /= 16;
    }
}

void update_killers(struct t_pv_data *pv, int depth) {

    if (!pv->current_move->captured && (pv->current_move != pv->killer1)) {
        pv->killer2 = pv->killer1;
        pv->killer1 = pv->current_move;
        pv->current_move->history += (depth * depth);
    }

    if (struct t_pv_data *previous = pv->previous_pv)
        if (previous->current_move)
            previous->current_move->refutation = pv->current_move;
}

void update_check_killers(struct t_pv_data *pv, int depth) {
    if (!pv->current_move->captured && (pv->current_move != pv->check_killer1)) {
        pv->check_killer2 = pv->check_killer1;
        pv->check_killer1 = pv->current_move;
        pv->current_move->history += (depth * depth);
    }

	if (struct t_pv_data *previous = pv->previous_pv)
		if (previous->current_move)
			previous->current_move->refutation = pv->current_move;
}
