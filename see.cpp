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


BOOL see(struct t_board *board, struct t_move_record *move, t_chess_value threshold) {

    t_chess_value see_value = see_piece_value[move->captured];
    t_chess_value trophy_value = see_piece_value[move->piece];

    //-- Is it an obvious winner e.g. PxQ
    if (see_value - trophy_value >= threshold)
        return TRUE;

    //-- Is it an obvious loser?
    if (see_value < threshold)
        return FALSE;

    t_bitboard attacks[2];
    t_chess_color to_move = COLOR(move->piece);
    t_chess_color opponent = OPPONENT(to_move);
    t_chess_square to_square = move->to_square;
    t_chess_square s;
    t_bitboard b;

    //-- Generate Opponent's Pawn Moves
    attacks[opponent] = pawn_attackers[opponent][to_square] & board->pieces[opponent][PAWN];

    //-- See if an opponent's pawn can take and cause a cut-off
    if (attacks[opponent] && see_value - trophy_value + see_piece_value[PAWN] < threshold)
        return FALSE;

    //-- Generate Attacks - pawns first
    attacks[to_move] = pawn_attackers[to_move][to_square] & board->pieces[to_move][PAWN];

    //-- Knights
    attacks[to_move] |= knight_mask[to_square] & board->pieces[to_move][KNIGHT];
    attacks[opponent] |= knight_mask[to_square] & board->pieces[opponent][KNIGHT];

    //-- Kings
    attacks[to_move] |= king_mask[to_square] & board->pieces[to_move][KING];
    attacks[opponent] |= king_mask[to_square] & board->pieces[opponent][KING];

    //-- Bishops
    attacks[to_move] |= bishop_rays[to_square] & (board->pieces[to_move][BISHOP] | board->pieces[to_move][QUEEN]);
    attacks[opponent] |= bishop_rays[to_square] & (board->pieces[opponent][BISHOP] | board->pieces[opponent][QUEEN]);

    //-- Rooks
    attacks[to_move] |= rook_rays[to_square] & (board->pieces[to_move][ROOK] | board->pieces[to_move][QUEEN]);
    attacks[opponent] |= rook_rays[to_square] & (board->pieces[opponent][ROOK] | board->pieces[opponent][QUEEN]);

    t_bitboard _all_pieces = board->all_pieces;

    b = SQUARE64(move->from_square);
    attacks[to_move] ^= b;
    _all_pieces ^= b;

    do {

        if (!attacks[opponent]) {
            trophy_value = 0;
            goto opponent_cut_test;
        }

        if (b = (board->pieces[opponent][PAWN] & attacks[opponent])) {
            see_value -= trophy_value;
            trophy_value = see_piece_value[PAWN];
            b &= -b;
            attacks[opponent] ^= b;
            _all_pieces ^= b;
            goto opponent_cut_test;
        }

        if (b = (board->pieces[opponent][KNIGHT] & attacks[opponent])) {
            see_value -= trophy_value;
            trophy_value = see_piece_value[KNIGHT];
            b &= -b;
            attacks[opponent] ^= b;
            goto opponent_cut_test;
        }

        if (b = (board->pieces[opponent][BISHOP] & attacks[opponent])) {
            do {
                s = bitscan_reset(&b);
                if (!(between[s][to_square] & _all_pieces)) {
                    b = SQUARE64(s);
                    attacks[opponent] ^= b;
                    _all_pieces ^= b;
                    see_value -= trophy_value;
                    trophy_value = see_piece_value[BISHOP];
                    goto opponent_cut_test;
                }
            } while (b);
        }

        if (b = (board->pieces[opponent][ROOK] & attacks[opponent])) {
            do {
                s = bitscan_reset(&b);
                if (!(between[s][to_square] & _all_pieces)) {
                    b = SQUARE64(s);
                    attacks[opponent] ^= b;
                    _all_pieces ^= b;
                    see_value -= trophy_value;
                    trophy_value = see_piece_value[ROOK];
                    goto opponent_cut_test;
                }
            } while (b);
        }

        if (b = (board->pieces[opponent][QUEEN] & attacks[opponent])) {
            do {
                s = bitscan_reset(&b);
                if (!(between[s][to_square] & _all_pieces)) {
                    b = SQUARE64(s);
                    attacks[opponent] ^= b;
                    _all_pieces ^= b;
                    see_value -= trophy_value;
                    trophy_value = see_piece_value[QUEEN];
                    goto opponent_cut_test;
                }
            } while (b);
        }

        if (b = (board->pieces[opponent][KING] & attacks[opponent])) {
            see_value -= trophy_value;
            trophy_value = see_piece_value[KING];
            b &= -b;
            attacks[opponent] ^= b;
        }
        else
            trophy_value = 0;

opponent_cut_test:

        //-- Test Upper Bound
        if (see_value >= threshold)
            return TRUE;

        //-- Test Lower Bound
        if (see_value + trophy_value < threshold)
            return FALSE;

        //-- Now try to recapture!
        if (!attacks[to_move]) {
            trophy_value = 0;
            goto to_move_cut_test;
        }

        if (b = (board->pieces[to_move][PAWN] & attacks[to_move])) {
            see_value += trophy_value;
            trophy_value = see_piece_value[PAWN];
            b &= -b;
            attacks[to_move] ^= b;
            _all_pieces ^= b;
            goto to_move_cut_test;
        }

        if (b = (board->pieces[to_move][KNIGHT] & attacks[to_move])) {
            see_value += trophy_value;
            trophy_value = see_piece_value[KNIGHT];
            b &= -b;
            attacks[to_move] ^= b;
            goto to_move_cut_test;
        }

        if (b = (board->pieces[to_move][BISHOP] & attacks[to_move])) {
            do {
                s = bitscan_reset(&b);
                if (!(between[s][to_square] & _all_pieces)) {
                    b = SQUARE64(s);
                    attacks[to_move] ^= b;
                    _all_pieces ^= b;
                    see_value += trophy_value;
                    trophy_value = see_piece_value[BISHOP];
                    goto to_move_cut_test;
                }
            } while (b);
        }

        if (b = (board->pieces[to_move][ROOK] & attacks[to_move])) {
            do {
                s = bitscan_reset(&b);
                if (!(between[s][to_square] & _all_pieces)) {
                    b = SQUARE64(s);
                    attacks[to_move] ^= b;
                    _all_pieces ^= b;
                    see_value += trophy_value;
                    trophy_value = see_piece_value[ROOK];
                    goto to_move_cut_test;
                }
            } while (b);
        }

        if (b = (board->pieces[to_move][QUEEN] & attacks[to_move])) {
            do {
                s = bitscan_reset(&b);
                if (!(between[s][to_square] & _all_pieces)) {
                    b = SQUARE64(s);
                    attacks[to_move] ^= b;
                    _all_pieces ^= b;
                    see_value += trophy_value;
                    trophy_value = see_piece_value[QUEEN];
                    goto to_move_cut_test;
                }
            } while (b);
        }

        if (b = (board->pieces[to_move][KING] & attacks[to_move])) {
            see_value += trophy_value;
            trophy_value = see_piece_value[KING];
            b &= -b;
            attacks[to_move] ^= b;
        }
        else
            trophy_value = 0;

to_move_cut_test:

        if (see_value - trophy_value >= threshold)
            return TRUE;

        if (see_value < threshold)
            return FALSE;

    } while (TRUE);


    return TRUE;

}