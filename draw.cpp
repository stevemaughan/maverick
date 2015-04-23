//===========================================================//
//
// Maverick Chess Engine
// Copyright 2013-2015 Steve Maughan
//
//===========================================================//

#include <assert.h>
#include <stdio.h>

#include "defs.h"
#include "data.h"
#include "procs.h"

BOOL repetition_draw(struct t_board *board)
{
    int i;
    int reps = 0;

    if (board->fifty_move_count >= 4) {
        if (board->fifty_move_count >= 100) return !is_checkmate(board);
        i = 4;
        do
        {
            if (draw_stack[draw_stack_count - i] == board->hash) {
                if (TRUE || draw_stack_count - i > search_start_draw_stack_count)
                    return TRUE;
                reps++;
                if (reps == 2)
                    return TRUE;
            }
            i += 2;
        } while (board->fifty_move_count >= i);
    }
    return FALSE;
}

BOOL is_checkmate(struct t_board *board)
{
    if (!board->in_check)
        return FALSE;

    struct t_move_list moves[1];
    generate_evade_check(board, moves);

    if (moves->count > 0)
        return FALSE;

    return TRUE;
}

BOOL is_stalemate(struct t_board *board)
{
    struct t_move_list moves[1];
    generate_legal_moves(board, moves);

    if (moves->count > 0)
        return FALSE;

    return TRUE;
}
