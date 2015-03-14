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
#include <Windows.h>

#include "defs.h"
#include "data.h"
#include "procs.h"
#include "bittwiddle.h"

inline BOOL can_do_null_move(struct t_board *board, struct t_pv_data *pv, int ply, t_chess_value alpha, t_chess_value beta){

	t_chess_color color = board->to_move;
	int piece_count = popcount(board->occupied[color] ^ board->pieces[color][PAWN]);

	return !board->in_check && pv->eval->static_score >= beta && piece_count > 3;
}

t_chess_value alphabeta(struct t_board *board, int ply, int depth, t_chess_value alpha, t_chess_value beta) {

	t_chess_value e;

    //-- Should we call qsearch?
    if (depth <= 0 && !board->in_check) {
        e = qsearch_plus(board, ply, depth, alpha, beta);
		assert(e >= -CHECKMATE && e <= CHECKMATE);
		return e;
    }

    //-- Increment the nodes
    nodes++;

	//-- see if we need to update stats */
	if ((nodes & message_update_mask) == 0)
		uci_check_status(board, ply);

	//-- Local Principle Variation variable
    struct t_pv_data *pv = &(board->pv_data[ply]);

    //-- Has the maximum depth been reached
    if (ply >= MAXPLY || uci.stop) {
        pv->best_line_length = ply;
		assert(pv->eval->static_score >= -CHECKMATE && pv->eval->static_score <= CHECKMATE);
        return pv->eval->static_score;
    }

    /* check to see if this is a repeated position or draw by 50 moves */
    if (repetition_draw(board)) {
        pv->best_line_length = ply;
        return 0;
    }

	//-- Mate Distance Pruning
	if (CHECKMATE - ply <= alpha){
		assert(alpha >= -CHECKMATE && alpha <= CHECKMATE);
		return alpha;
	}
	else if (-CHECKMATE + ply >= beta){
		assert(beta >= -CHECKMATE && beta <= CHECKMATE);
		return beta;
	}

	//-- Declare local variables
    struct t_pv_data *next_pv = &(board->pv_data[ply + 1]);
    int reduction;

    t_chess_value					best_score = -CHECKMATE;
    t_chess_value					a = alpha;
    t_chess_value					b = beta;

    //-- Probe Hash
	struct t_move_record *hash_move = NULL;
	t_hash_record *hash_record = probe(board->hash);

	//-- Has there been a match?
	if (hash_record != NULL){

		//-- Could it make a cut-off?
		if (hash_record->depth >= depth){

			//-- Get the score from the hash table
			t_chess_value hash_score = get_hash_score(hash_record, ply);

			//-- Score in hash table is at least as good as beta
			if (hash_record->bound != HASH_UPPER && hash_score >= beta){
				hash_record->age = hash_age;
				assert(hash_score >= -CHECKMATE && hash_score <= CHECKMATE);
				return hash_score;
			}

			//-- Score is worse than alpha
			if (hash_record->bound != HASH_LOWER && hash_score <= alpha){
				hash_record->age = hash_age;
				assert(hash_score >= -CHECKMATE && hash_score <= CHECKMATE);
				return hash_score;
			}

			//-- Score is more accurate
			if (hash_record->bound == HASH_EXACT){
				hash_record->age = hash_age;
				pv->best_line_length = ply;
				update_best_line_from_hash(board, ply);
				assert(hash_score >= -CHECKMATE && hash_score <= CHECKMATE);
				return hash_score;
			}
		}

		//-- Store the hash move for further use!
		hash_move = hash_record->move;
	}

    //-- Null Move
    t_undo undo[1];
	if (can_do_null_move(board, pv, ply, alpha, beta)){

		//-- Make the changes on the board
		make_null_move(board, undo);

		//-- Store the move in the PV data
		pv->current_move = NULL;

		//-- Evaluate the new board position
		evaluate(board, next_pv->eval);

		//-- Find the new score
		e = -alphabeta(board, ply + 1, depth - NULL_REDUCTION - 1, -beta, -beta + 1);

		//-- undo the null move
		unmake_null_move(board, undo);

		//-- is it good enough for a cut-off?
		if (e >= beta){
			poke(board->hash, e, ply, depth, HASH_LOWER, NULL);
			assert(e >= -CHECKMATE && e <= CHECKMATE);
			return e;
		}
	}

    //-- Internal Iterative Deepening!
	if (alpha + 1 != beta && hash_move == NULL && depth > 4 && !uci.stop){

		//-- Search with reduced depth
		e = alphabeta(board, ply, depth - 4, alpha, beta);

		//-- If still no move then search with -INFINITY bound
		if (e <= alpha)
			e = alphabeta(board, ply, depth - 4, -CHESS_INFINITY, beta);

		//-- Probe the hash
		hash_record = probe(board->hash);

		//-- Set the hash move
		if (hash_record != NULL)
			hash_move = hash_record->move;
	}

    //-- Generate All Moves
    struct t_move_list moves[1];
	moves->hash_move = hash_move;

    if (board->in_check) {
        generate_evade_check(board, moves);

        // Are we in checkmate?
        if (moves->count == 0) {
            pv->best_line_length = ply;
			return -CHECKMATE + ply;
        }
		order_evade_check(board, moves, ply);
    }
    else {
        generate_moves(board, moves);
		order_moves(board, moves, ply);
    }

    //-- Enhanced Transposition Cutoff?
	if (depth > 4 && !uci.stop){
		BOOL fail_low;
		while (simple_make_next_move(board, moves, undo)){

			//-- Is the opponent in check?
			if (board->in_check)
				reduction = 0;
			else
				reduction = 1;

			//-- Simple version of alpha_beta for tips of search
			e = -alphabeta_tip(board, ply + 1, depth - reduction, -beta, &fail_low);

			//-- Take the move back
			unmake_move(board, undo);

			//-- Is it good enough for a cutoff?
			if (e >= beta){
				poke(board->hash, e, ply, depth, HASH_LOWER, moves->current_move);
				assert(e >= -CHECKMATE && e <= CHECKMATE);
				return e;
			}

			//-- Is it going to enhance the move ordering?
			if (fail_low){
				moves->value[moves->imove] += MOVE_ORDER_ETC;
				assert(moves->move[moves->imove] == moves->current_move);
			}

		}
		//-- Reset the real move count for the "proper" search
		moves->imove = moves->count;
	}

    //-- Order the moves

	//-- Create the list of "bad" captures
	struct t_move_list bad_moves[1];
	bad_moves->count = 0;
	bad_moves->imove = 0;

	//-- Reset the move count (must be after IID)
	pv->legal_moves_played = 0;

    //-- Play moves
    while (!uci.stop && make_next_move(board, moves, bad_moves, undo)) {

        //-- Increment the "legal_moves_played" counter
        pv->legal_moves_played++;
        pv->current_move = moves->current_move;

        //-- Evaluate the new board position
        evaluate(board, next_pv->eval);

        //-- Calculate reduction
        if (board->in_check)
            reduction = 0;
        else
            reduction = 1;

        //-- Search the next ply at reduced depth
        e = -alphabeta(board, ply + 1, depth - reduction, -b, -a);

        //-- Is a research required?
		if (alpha + 1 != beta && e > a && a + 1 == b)
            e = -alphabeta(board, ply + 1, depth - reduction, -beta, -a);

        unmake_move(board, undo);

        //-- Is it good enough to cut-off?
        if (e >= beta) {
			if (board->in_check)
				update_check_killers(pv, depth);
			else
				update_killers(pv, depth);

			//-- Record the cutoff
			cutoffs++;
			if (pv->legal_moves_played == 1)
				first_move_cutoffs++;

			//-- Store in the hash table
			poke(board->hash, e, ply, depth, HASH_LOWER, pv->current_move);
			assert(e >= -CHECKMATE && e <= CHECKMATE);
			return e;
        }

        //-- Is it the best so far?
        if (e > best_score) {
            best_score = e;

            //-- Does it improve upon alpha (i.e. is it part of the PV)?
            if (e > a) {
                a = e;

                //-- Update the Principle Variation
                update_best_line(board, ply);
            }
        }

        // Reset the zero width window
        b = a + 1;
    }

    //-- Is it a draw
    if (pv->legal_moves_played == 0) {
        pv->best_line_length = ply;
        return 0;
    }

	//-- Update Hash
	if (best_score > alpha)
		poke(board->hash, best_score, ply, depth, HASH_EXACT, pv->best_line[ply]);
	else
		poke(board->hash, best_score, ply, depth, HASH_UPPER, NULL);

    // Return Best Score found
	assert(best_score >= -CHECKMATE && best_score <= CHECKMATE);
	return best_score;

}


t_chess_value qsearch_plus(struct t_board *board, int ply, int depth, t_chess_value alpha, t_chess_value beta) {

	//-- Principle Variation
	struct t_pv_data *pv = &(board->pv_data[ply]);

	//-- Has the maximum depth been reached
	if (ply >= MAXPLY || uci.stop)
		return pv->eval->static_score;

	//-- Increment the node count
	qnodes++;

	/* check to see if this is a repeated position or draw by 50 moves */
	if (repetition_draw(board)) {
		pv->best_line_length = ply;
		return 0;
	}

	//-- Mate Distance Pruning
	if (CHECKMATE - ply <= alpha){
		return alpha;
	}
	else if (-CHECKMATE + ply >= beta){
		return beta;
	}

	//-- Next Principle Variation
	struct t_pv_data *next_pv = &(board->pv_data[ply + 1]);

	//-- Define the local variables
	pv->legal_moves_played = 0;
	t_chess_value best_score;
	t_chess_value a = alpha;
	t_chess_value b = beta;
	t_chess_value e;


	// Declare local variables
	t_undo undo[1];

	//-- Generate All Moves
	struct t_move_list moves[1];
	moves->hash_move = NULL;

	//-----------------------------------------------
	//-- Handling In-Check (e.g. Cannot Stand-Pat)
	//-----------------------------------------------
	if (board->in_check) {

		best_score = -CHECKMATE;

		//-- Generate moves which get out of check
		generate_evade_check(board, moves);

		// Are we in checkmate?
		if (moves->count == 0) {
			pv->best_line_length = ply;
			return -CHECKMATE + ply;
		}

		t_chess_value(*q_search)(struct t_board *board, int ply, int depth, t_chess_value alpha, t_chess_value beta);

		if (moves->count == 1)
			q_search = &qsearch_plus;
		else{
			q_search = &qsearch;
			//-- Order the moves
			order_evade_check(board, moves, ply);
		}

		//-- Play moves
		while (make_next_best_move(board, moves, undo)) {

			//-- Increment the "legal_moves_played" counter
			pv->legal_moves_played++;
			pv->current_move = moves->current_move;

			//-- Evaluate the new board position
			evaluate(board, next_pv->eval);

			//-- More than one move out of check so just use "vanilla" qsearch
			e = -q_search(board, ply + 1, depth - 1, -b, -a);

			//-- Is a research required?
			if (alpha + 1 != beta && e > a && a + 1 == b)
				e = -q_search(board, ply + 1, depth - 1, -beta, -a);

			unmake_move(board, undo);

			//-- Is it good enough to cut-off?
			if (e >= beta){
				update_check_killers(pv, 0);
				poke(board->hash, e, ply, depth, HASH_LOWER, pv->current_move);
				return e;
			}

			//-- Is it the best so far?
			if (e > best_score) {
				best_score = e;

				//-- Does it improve upon alpha (i.e. is it part of the PV)?
				if (e > a) {
					a = e;

					//-- Update the Principle Variation
					update_best_line(board, ply);
				}
			}

			// Reset the zero width window
			b = a + 1;

		}
	}
	else {
		//--------------------------------------------------------
		//-- Normal moves handled differently (e.g. can stand-pat)
		//--------------------------------------------------------

		//-- Does stand-pat cause a cut-off?
		e = pv->eval->static_score;
		if (e >= beta)
			return e;

		//-- Does the current value raise alpha?
		best_score = e;
		if (e > alpha) {
			a = e;
			pv->best_line_length = ply;
		}

		//-- Generate all captures
		generate_captures(board, moves);

		//-- Order the moves
		order_captures(board, moves);

		//-- Play *ALL* captures
		while (make_next_best_move(board, moves, undo)) {

			//-- Increment the "legal_moves_played" counter
			pv->legal_moves_played++;
			pv->current_move = moves->current_move;

			//-- Evaluate the new board position
			evaluate(board, next_pv->eval);

			//-- Search the next ply at reduced depth
			e = -qsearch(board, ply + 1, depth - 1, -b, -a);

			//-- Is a research required?
			if (alpha + 1 != beta && e > a && a + 1 == b)
				e = -qsearch(board, ply + 1, depth - 1, -beta, -a);

			unmake_move(board, undo);

			//-- Is it good enough to cut-off?
			if (e >= beta){
				return e;
			}

			//-- Is it the best so far?
			if (e > best_score) {
				best_score = e;

				//-- Does it improve upon alpha (i.e. is it part of the PV)?
				if (e > a) {
					a = e;
					update_best_line(board, ply);
				}
			}

			// Reset the zero width window
			b = a + 1;

		}

		//-- Now Try the checks!
		generate_quiet_checks(board, moves);

		//-- Order the moves
		order_moves(board, moves, ply);

		//-- Play moves
		while (make_next_best_move(board, moves, undo)) {

			//-- Increment the "legal_moves_played" counter
			pv->legal_moves_played++;
			pv->current_move = moves->current_move;

			//-- Evaluate the new board position
			evaluate(board, next_pv->eval);

			//-- Search the next ply at reduced depth
			e = -qsearch_plus(board, ply + 1, depth - 1, -b, -a);

			//-- Is a research required?
			if (alpha + 1 != beta && e > a && a + 1 == b)
				e = -qsearch_plus(board, ply + 1, depth - 1, -beta, -a);

			unmake_move(board, undo);

			//-- Is it good enough to cut-off?
			if (e >= beta){
				poke(board->hash, e, ply, depth, HASH_LOWER, pv->current_move);
				update_killers(pv, 0);
				return e;
			}

			//-- Is it the best so far?
			if (e > best_score) {
				best_score = e;

				//-- Does it improve upon alpha (i.e. is it part of the PV)?
				if (e > a) {
					a = e;
					update_best_line(board, ply);
				}
			}

			// Reset the zero width window
			b = a + 1;

		}
	}

	//-- Update Hash
	if (best_score > alpha)
		poke(board->hash, best_score, ply, depth, HASH_EXACT, pv->best_line[ply]);

	// Return Best Score found
	return best_score;
}

t_chess_value qsearch(struct t_board *board, int ply, int depth, t_chess_value alpha, t_chess_value beta) {

	//-- Principle Variation
	struct t_pv_data *pv = &(board->pv_data[ply]);

	//-- Has the maximum depth been reached
    if (ply >= MAXPLY || uci.stop)
        return pv->eval->static_score;

    //-- Increment the node count
    qnodes++;

    //-- Is this the deepest?
    if (ply > deepest) {
        deepest = ply;
		do_uci_depth();
		//if (ply == MAXPLY)
		//	write_path(board, ply - 1, "path.txt");
    }

	//-- Mate Distance Pruning
	if (CHECKMATE - ply <= alpha){
		return alpha;
	}
	else if (-CHECKMATE + ply >= beta){
		return beta;
	}

	//-- PV of Next Ply
    struct t_pv_data *next_pv = &(board->pv_data[ply + 1]);

    //-- Define the local variables
	pv->legal_moves_played = 0;
    t_chess_value best_score;
    t_chess_value a = alpha;
    t_chess_value b = beta;
    t_chess_value e;


    // Declare local variables
    t_undo undo[1];

    //-- Generate All Moves
    struct t_move_list moves[1];
	moves->hash_move = NULL;

    //-----------------------------------------------
    //-- Handling In-Check (e.g. Cannot Stand-Pat)
    //-----------------------------------------------
    if (board->in_check) {

        best_score = -CHECKMATE;

        //-- Generate moves which get out of check
        generate_evade_check(board, moves);

        // Are we in checkmate?
        if (moves->count == 0) {
            pv->best_line_length = ply;
            return -CHECKMATE + ply;
        }

        //-- Order the moves
        order_evade_check(board, moves, ply);

        //-- Play moves
		while (make_next_best_move(board, moves, undo)) {

            //-- Increment the "legal_moves_played" counter
            pv->legal_moves_played++;
            pv->current_move = moves->current_move;

            //-- Evaluate the new board position
            evaluate(board, next_pv->eval);

            //-- Search the next ply at reduced depth
            e = -qsearch(board, ply + 1, depth - 1, -b - 1, -a);

            //-- Is a research required?
			if (alpha + 1 != beta && e > a && a + 1 == b)
				e = -qsearch(board, ply + 1, depth - 1, -beta, -a);

            unmake_move(board, undo);

            //-- Is it good enough to cut-off?
			if (e >= beta){
				update_check_killers(pv, 0);
				return e;
			}

            //-- Is it the best so far?
            if (e > best_score) {
                best_score = e;

                //-- Does it improve upon alpha (i.e. is it part of the PV)?
                if (e > a) {
                    a = e;

                    //-- Update the Principle Variation
                    update_best_line(board, ply);
                }
            }

            // Reset the zero width window
            b = a + 1;

        }

    }
    else {
        //--------------------------------------------------------
        //-- Normal moves handled differently (e.g. can stand-pat)
        //--------------------------------------------------------

        //-- Does stand-pat cause a cutoff?
        e = pv->eval->static_score;
        if (e >= beta)
            return e;

        //-- Does the current value raise alpha?
        best_score = e;
        if (e > alpha) {
            a = e;
            pv->best_line_length = ply;
        }

        //-- Generate all captures
        generate_captures(board, moves);

        //-- Order the moves
        order_captures(board, moves);

        //-- Play moves
        while (make_next_see_positive_move(board, moves, 0, undo)) {

            //-- Increment the "legal_moves_played" counter
            pv->legal_moves_played++;
            pv->current_move = moves->current_move;

            //-- Evaluate the new board position
            evaluate(board, next_pv->eval);

            //-- Search the next ply at reduced depth
            e = -qsearch(board, ply + 1, depth - 1, -b, -a);

            //-- Is a research required?
			if (alpha + 1 != beta && e > a && a + 1 == b)
				e = -qsearch(board, ply + 1, depth - 1, -beta, -a);

            unmake_move(board, undo);

            //-- Is it good enough to cut-off?
            if (e >= beta)
                return e;

            //-- Is it the best so far?
            if (e > best_score) {
                best_score = e;

                //-- Does it improve upon alpha (i.e. is it part of the PV)?
                if (e > a) {
                    a = e;
                    update_best_line(board, ply);
                }
            }

            // Reset the zero width window
            b = a + 1;

        }
    }

    // Return Best Score found
    return best_score;

}

t_chess_value alphabeta_tip(struct t_board *board, int ply, int depth, t_chess_value alpha, BOOL *fail_low) {

	//-- Set the default of Fail Low
	*fail_low = FALSE;

	//-- Has the maximum depth been reached
	if (ply >= MAXPLY) {
		return 0;
	}

	/* check to see if this is a repeated position or draw by 50 moves */
	if (repetition_draw(board)) {
		return 0;
	}

	//-- Probe Hash
	struct t_move_record *hash_move = NULL;
	t_hash_record *hash_record = probe(board->hash);

	//-- Has there been a match?
	if (hash_record != NULL){

		//-- Get the score from the hash table
		t_chess_value hash_score = get_hash_score(hash_record, ply);

		//-- Score is worse than alpha
		if (hash_record->bound != HASH_LOWER && hash_score <= alpha){
			*fail_low = TRUE;
			if (hash_record->depth >= depth)
				return hash_score;
		}
	}

	// Return Infinity
	return +CHESS_INFINITY;

}
