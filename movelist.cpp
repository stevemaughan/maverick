//===========================================================//
//
// Maverick Chess Engine
// Copyright 2013 Steve Maughan
//
//===========================================================//

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "defs.h"
#include "data.h"
#include "procs.h"

BOOL equal_move_lists(struct t_move_list *move_list1, struct t_move_list *move_list2) {

    // Number of moves
    if (move_list1->count != move_list2->count)
        return FALSE;

    // Find each moves in second list
    int i, j;
    for (i = move_list1->count - 1; i >= 0; i--) {
        j = move_list2->count - 1;
        while ((j >= 0) && (move_list1->move[i] != move_list2->move[j]))
            j--;
        if (j < 0)
            return FALSE;
    }
    return TRUE;
}

void reset_move_list_scores(struct t_move_list *move_list) {
    for (int i = 0; i < move_list->count; i++)
        move_list->value[i] = 0;
}

void new_best_move(struct t_move_list *move_list, int i) {

    struct t_move_record *move;
    t_chess_value t;

    move = move_list->move[0];
    move_list->move[0] = move_list->move[i];
    move_list->move[i] = move;

    t = move_list->value[0];
    move_list->value[0] = move_list->value[i];
    move_list->value[i] = t;
}

void update_move_value(struct t_move_record *move, struct t_move_list *move_list, t_nodes n) {

    for (int i = 0; i < move_list->count; i++)
    {
        if (move_list->move[i] == move) {
            move_list->value[i] += n;
            return;
        }
    }
}

BOOL is_move_in_list(struct t_move_record *move, struct t_move_list *move_list){

	for (int i = 0; i < move_list->count; i++)
	{
		if (move_list->move[i] == move) 
			return TRUE;
	}

	return FALSE;

}
