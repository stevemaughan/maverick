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

#if defined(_WIN32)
#include <windows.h>
#else
#include <unistd.h>
#endif

#include "defs.h"
#include "data.h"
#include "procs.h"

void root_search(struct t_board *board)
{

    //-- Declare local variables
    struct t_undo undo[1];
    struct t_pv_data *pv = board->pv_data;
    t_chess_value e;

    //-- What type of chess are we playing?
    if (board->chess960)
        send_info("Playing Chess960 / FRC");

    assert(uci.options.chess960 == board->chess960);

    //-- Generate moves
    struct t_move_list move_list[1];
    generate_legal_moves(board, move_list);

    //-- Dummy move for PV (in case there is no search)
    board->pv_data[0].best_line[0] = move_list->move[0];
    board->pv_data[0].best_line_length = 1;

    //-- Reset Move Scores
    reset_move_list_scores(move_list);

    //-- Record pre-search state
    nodes = 0;
    qnodes = 0;

    hash_age++;
    hash_hits = 0;
    hash_full = 0;
    hash_probes = 0;

    cutoffs = 0;
    first_move_cutoffs = 0;

    pv->node_type = node_pv;

    search_ply = 0;
    deepest = 0;
    message_update_count = 0;
    search_start_time = time_now();
    search_start_draw_stack_count = draw_stack_count;
	t_chess_value best_score = -CHESS_INFINITY;

    //-- Write the whole tree to a file!
    //write_tree(board, NULL, FALSE, "tree.txt");

    //-- Start the thinking!
    uci.engine_state = UCI_ENGINE_THINKING;

    //-- See if we can play a book move
    if (uci.opening_book.use_own_book && !uci.level.infinite) {
        struct t_move_record *move;

        //-- Probe the opening book
        move = probe_book(board);
        if (move != NULL) {
            board->pv_data[0].best_line[0] = move;
            board->pv_data[0].best_line_length = 1;
            send_info("Maverick Book Move!");
            while (uci.level.ponder && !uci.stop)
                Sleep(1);
            do_uci_bestmove(board);
            return;
        }
    }

    //-- Age the history scores
    age_history_scores();

    //-- Iternate round until finished searching
    do {

        //-- Reset the search parameters for this iteration
        int i = 0;
        search_ply += 1;
        pv->legal_moves_played = 0;
		pv->mate_threat = 0;
        t_chess_value alpha = -CHESS_INFINITY;
        t_chess_value beta = +CHESS_INFINITY;
        best_score = -CHESS_INFINITY;

        //-- Loop around for each move
        while ((i < move_list->count) && !uci.stop) {

            //-- Record the nodes at the start of the search
            start_nodes = nodes + qnodes;

            //-- Make the move on the board
            pv->current_move = move_list->move[i];
            pv->legal_moves_played++;
            make_move(board, move_list->pinned_pieces, pv->current_move, undo);

            //-- Tell the GUI
            do_uci_consider_move(board, search_ply);

            //-- Evaluate the new position
            evaluate(board, board->pv_data[1].eval);

			//-- Extend for checks
			if (board->in_check && see_safe(board, pv->current_move->to_square, 0))
                pv->reduction = 0;
            else
                pv->reduction = 1;

            //-- Call alpha-beta recursively
            e = -alphabeta(board, 1, search_ply - pv->reduction, -beta, -alpha);

            //-- Test for best move so far!
            if ((e > alpha) && !uci.stop) {

                //-- See if we need to re-search
                if ((e >= beta) && !uci.stop) {

                    //-- Show fail high
                    do_uci_fail_high(board, e, search_ply);

                    //-- Research with wider bounds
                    e = -alphabeta(board, 1, search_ply - pv->reduction, -CHESS_INFINITY, -alpha);
                }

                //-- If still better than everything else then update best move!
                if (e > alpha && !uci.stop) {

                    alpha = e;
                    new_best_move(move_list, i);

                    //-- Update the Principle Variation
                    update_best_line(board, 0);
                }

                //-- Posibly Update the Best Score
                if (e > best_score)
                    best_score = e;

                //-- Tell the GUI!
                if (!uci.stop)
                    do_uci_new_pv(board, best_score, search_ply);
            }

            //-- Update the late move's with the number of nodes searched
            update_move_value(pv->current_move, move_list, nodes + qnodes - start_nodes);

            //-- Undo the move
            unmake_move(board, undo);

            //-- Update beta for zero width
            beta = alpha + 1;

            i++;
        }

        //-- Sort moves based on Node count
        if (move_list->count > 1)
            qsort_moves(move_list, 1, move_list->count - 1);

    } while (!is_search_complete(board, best_score, search_ply, move_list) && !uci.stop);

    //-- Snooze while still in ponder mode
    while (((uci.level.infinite) || (uci.level.ponder)) && !uci.stop)
        Sleep(1);

    //-- Send the latest PV
    if (!uci.stop)
        do_uci_new_pv(board, best_score, search_ply);

    //-- Send the GUI all of the search details
    do_uci_hash_full();
    do_uci_send_nodes();
    do_uci_bestmove(board);
    do_uci_show_stats();

}

