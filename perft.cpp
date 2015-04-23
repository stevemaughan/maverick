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
#include "bittwiddle.h"

t_nodes perft(struct t_board *board, int depth) {

    struct t_move_list move_list[1];
    struct t_undo undo[1];

    t_nodes total_nodes = 0;
    t_nodes move_nodes = 0;

    unsigned long start = time_now();

    int i;

    if (board->in_check)
        generate_evade_check(board, move_list);
    else
        generate_moves(board, move_list);

    for (i = move_list->count - 1; i >= 0; i--) {
        if (make_move(board, move_list->pinned_pieces, move_list->move[i], undo)) {
            move_nodes = 0;
            if (depth > 1)
                move_nodes += do_perft(board, depth - 1);
            printf(move_as_str(move_list->move[i]));
            printf(" = %llu\n", move_nodes);
            unmake_move(board, undo);
            total_nodes += move_nodes;
        }
    }

    unsigned long finish = time_now();

    if (finish == start)
        printf(INFO_STRING_PERFT_NODES, total_nodes);
    else
        printf(INFO_STRING_PERFT_SPEED, (unsigned long long) total_nodes, (int) finish - start, 1000 * total_nodes / (finish - start));

    return total_nodes;
}

t_nodes do_perft(struct t_board *board, int depth)
{
    struct t_move_list move_list[1];
    struct t_move_list bad_move_list[1];
    bad_move_list->count = 0;
    bad_move_list->imove = 0;
    struct t_undo undo[1];

    t_nodes nodes = 0;
    int i;

    assert(integrity(board));
    if (board->in_check) {
        generate_evade_check(board, move_list);
        if (depth == 1) return move_list->count;
    }
    else {

        //generate_captures(board, move_list);
        //generate_quiet_moves(board, move_list);
        generate_moves(board, move_list);
        //if (!equal_move_lists(move_list, xmove_list)){
        //	write_board(board, "board.txt");
        //	write_move_list(xmove_list, "all.txt");
        //	write_move_list(move_list, "inc.txt");
        //}
        if (depth == 1) return legal_move_count(board, move_list);
    }

    for (i = move_list->count - 1; i >= 0; i--) {
        assert(lookup_move(board, move_as_str(move_list->move[i])) == move_list->move[i]);
        if (make_next_move(board, move_list, bad_move_list, undo)) {
            assert(integrity(board));

            //nodes++;
            if (depth > 1)
                nodes += do_perft(board, depth - 1);
            else
                nodes++;
            unmake_move(board, undo);
            assert(integrity(board));
        }
        else
            assert(integrity(board));
    }

    return nodes;

}

