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
#include <Windows.h>

#include "defs.h"
#include "data.h"
#include "procs.h"

void root_search(struct t_board *board)
{

	//-- Declare local variables
	struct t_undo undo[1];
	struct t_pv_data *pv = board->pv_data;
	t_chess_value e;

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

	search_ply = 0;
	deepest = 0;
	message_update_count = 0;
	search_start_time = time_now();
	search_start_draw_stack_count = draw_stack_count;
	t_chess_value best_score;

	//-- Write the whole tree to a file!
	//write_tree(board, NULL, FALSE, "tree.txt");

	//-- Start the thinking!
	uci.engine_state = UCI_ENGINE_THINKING;

	//-- See if we can play a book move
	if (uci.opening_book.use_own_book && !uci.level.infinite) {
		struct t_move_record *move;

		//-- Play a "Smart" Opening book move
		int book_count = book_move_count(board);
		if (uci.options.smart_book && book_count > 3){
			fill_opening_move_list(board, move_list);

			//-- If only one move then play the move immediately
			if (move_list->count == 1){
				move = move_list->move[0];
				if (move != NULL) {
					board->pv_data[0].best_line[0] = move;
					board->pv_data[0].best_line_length = 1;
					send_info("Maverick Smart Book Move!");
					while (uci.level.ponder && !uci.stop)
						Sleep(1);
					do_uci_bestmove(board);
					return;
				}
			}

			//-- Make sure there is limited time to "think"
			if (early_move_time < abort_move_time)
				abort_move_time = early_move_time;
			early_move_time = abort_move_time;
			target_move_time = abort_move_time;
		}

		//-- Probe the opening book as normal
		else{
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
	}

	//-- Age the history scores
	age_history_scores();

	//-- Iternate round until finished searching
	do {

		//-- Reset the search parameters for this iteration
		int i = 0;
		search_ply += 1;
		pv->legal_moves_played = 0;
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

			//-- Call alpha-beta recursively
			e = -alphabeta(board, 1, search_ply - 1, -beta, -alpha);

			//-- Test for best move so far!
			if ((e > alpha) && !uci.stop) {

				//-- See if we need to re-search
				if ((e >= beta) && !uci.stop) {

					//-- Show fail high
					do_uci_fail_high(board, e, search_ply);

					//-- Research with wider bounds
					e = -alphabeta(board, 1, search_ply - 1, -CHESS_INFINITY, -alpha);
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
		qsort_moves(move_list, 1, move_list->count - 1);

	} while (!is_search_complete(board, best_score, search_ply, move_list) && !uci.stop);

	//-- Snooze while still in ponder mode
	while (uci.level.ponder && !uci.stop)
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

void root_search_aspiration(struct t_board *board)
{

    //-- Declare local variables
    struct t_undo undo[1];
    struct t_pv_data *pv = board->pv_data;
    t_chess_value e;

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

    search_ply = 0;
    deepest = 0;
	message_update_count = 0;
	search_start_time = time_now();
    search_start_draw_stack_count = draw_stack_count;
	t_chess_value best_score;

    //-- Write the whole tree to a file!
    //write_tree(board, NULL, FALSE, "tree.txt");

    //-- Start the thinking!
    uci.engine_state = UCI_ENGINE_THINKING;

    //-- See if we can play a book move
    if (uci.opening_book.use_own_book && !uci.level.infinite) {
        struct t_move_record *move;
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

	//-- Evaluate the initial position
	evaluate(board, board->pv_data[0].eval);
	best_score = board->pv_data[0].eval->static_score;

	//-- Flaf to see if the search was volatile?
	BOOL volatile_last_ply = FALSE;

    //-- Iternate round until finished searching
    do {

		//-- Flag to see if the search is volatile
		BOOL volatile_this_ply = FALSE;

        //-- Reset the search parameters for this iteration
        int i = 0;
        search_ply += 1;
        pv->legal_moves_played = 0;

		//-- Reset Aspiration Windows
		t_chess_value alpha = best_score - aspiration_window[volatile_last_ply][1];
		t_chess_value beta = best_score + aspiration_window[volatile_last_ply][1];

		//-- Loop around for each move
        while ((i < move_list->count) && !uci.stop) {

			//-- Reset Fail High / Low
			int fail_high = 0;
			int fail_low = 0;

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

			//-- Start of Aspuiration search loop
			do{

				//-- Call alpha-beta recursively
				e = -alphabeta(board, 1, search_ply - 1, -beta, -alpha);

				//-- Stop?
				if (uci.stop) break;

				//-- Is it a fail low?
				if (e <= alpha){
					if (pv->legal_moves_played > 1)
						break;

					//-- Must be a fail low for the first move :(
					best_score = e;
					fail_low++;
					alpha = best_score - aspiration_window[volatile_last_ply][fail_low];
					if (alpha < -CHESS_INFINITY) alpha = -CHESS_INFINITY;

					//-- Let everyone know there's trouble ahead!
					do_uci_fail_low(board, e, search_ply);

				}
				else{

					//- New PV?
					if (e < beta){

						alpha = e;
						best_score = e;
						new_best_move(move_list, i);

						//-- Update the Principle Variation
						update_best_line(board, 0);
						do_uci_new_pv(board, best_score, search_ply);

						break;
					}

					//-- New Fail High
					else{
						best_score = e;
						fail_high++;
						beta = best_score + aspiration_window[volatile_last_ply][fail_high];
						if (beta > CHESS_INFINITY) beta = CHESS_INFINITY;

						//-- Show fail high
						do_uci_fail_high(board, e, search_ply);

						//-- If two fail highs then search is volatile
						if (fail_high == 3)
							volatile_this_ply = TRUE;
					}
				}

			} while (TRUE);

            //-- Update the late move's with the number of nodes searched
            update_move_value(pv->current_move, move_list, nodes + qnodes - start_nodes);

            //-- Undo the move
            unmake_move(board, undo);

            //-- Update beta for zero width
            beta = alpha + 1;

			//-- Set the volatile flags
			volatile_last_ply = volatile_this_ply;

            i++;
        }

		//-- Sort moves based on Node count
		qsort_moves(move_list, 1, move_list->count - 1);

    } while (!is_search_complete(board, best_score, search_ply, move_list) && !uci.stop);

    //-- Snooze while still in ponder mode
    while (uci.level.ponder && !uci.stop)
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

