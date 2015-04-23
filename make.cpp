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

inline BOOL is_in_check_after_move(struct t_board *board, struct t_move_record *move) {

    t_chess_color to_move = board->to_move;
    t_chess_square king_square = board->king_square[to_move];
    t_bitboard b;
    t_chess_color opponent = OPPONENT(to_move);
    t_chess_square s;

    if (PIECETYPE(move->piece) == KING) {
        // castling
        if (move->move_type == MOVE_CASTLE) {
            b = castle[move->index].not_attacked;
            while (b) {
                s = bitscan_reset(&b);
                if (is_square_attacked(board, s, opponent))
                    return TRUE;
            };
        }
        // Other King moves
        else
            return is_square_attacked(board, move->to_square, opponent);
    }
    else {
        b = xray[king_square][move->from_square];
        if (b) {
            t_bitboard attacker;
            board->all_pieces ^= move->from_to_bitboard ^ move->capture_mask;
            //-- Bishop
            attacker = (b & bishop_rays[king_square] & ~move->capture_mask);
            if (attacker) {
                attacker &= (board->pieces[opponent][BISHOP] | board->pieces[opponent][QUEEN]);
                while (attacker) {
                    s = bitscan_reset(&attacker);
                    if ((between[king_square][s] & board->all_pieces) == 0) {
                        board->all_pieces ^= move->from_to_bitboard ^ move->capture_mask;
                        return TRUE;
                    }
                }
            }
            //-- Rooks
            attacker = (b & rook_rays[king_square] & ~move->capture_mask);
            if (attacker) {
                attacker &= (board->pieces[opponent][ROOK] | board->pieces[opponent][QUEEN]);
                while (attacker) {
                    s = bitscan_reset(&attacker);
                    if ((between[king_square][s] & board->all_pieces) == 0) {
                        board->all_pieces ^= move->from_to_bitboard ^ move->capture_mask;
                        return TRUE;
                    }
                }
            }
            //-- e.p. (rare case)
            if (move->move_type == MOVE_PxP_EP) {
                board->all_pieces ^= ((SQUARE64(move->to_square) >> 8) << (16 * to_move));
                attacker = (b & rook_rays[king_square]);
                if (attacker) {
                    attacker &= (board->pieces[opponent][ROOK] | board->pieces[opponent][QUEEN]);
                    while (attacker) {
                        s = bitscan_reset(&attacker);
                        if ((between[king_square][s] & board->all_pieces) == 0) {
                            board->all_pieces ^= ((SQUARE64(move->to_square) >> 8) << (16 * to_move));
                            board->all_pieces ^= move->from_to_bitboard ^ move->capture_mask;
                            return TRUE;
                        }
                    }
                }
                board->all_pieces ^= ((SQUARE64(move->to_square) >> 8) << (16 * to_move));
            }
            board->all_pieces ^= move->from_to_bitboard ^ move->capture_mask;
        }
    }
    return FALSE;
}

void make_null_move(struct t_board *board, struct t_undo *undo) {

    // Transfer everthing required to the Undo record
    undo->ep_square = board->ep_square;
    undo->fifty_move_count = board->fifty_move_count;
    undo->hash = board->hash;
    undo->pawn_hash = board->pawn_hash;

    //-- Make the necessary changes
    board->to_move = OPPONENT(board->to_move);
    board->hash ^= white_to_move_hash;
    board->pawn_hash ^= white_to_move_hash;
    if (board->ep_square) {
        board->hash ^= ep_hash[COLUMN(bitscan(board->ep_square))];
        board->ep_square = 0;
    }
    board->fifty_move_count++;
    draw_stack[++draw_stack_count] = board->hash;
}

void unmake_null_move(struct t_board *board, struct t_undo *undo) {

    //-- Transfer everything back
    board->ep_square = undo->ep_square;
    board->fifty_move_count = undo->fifty_move_count;
    board->hash = undo->hash;
    board->pawn_hash = undo->pawn_hash;
    board->to_move = OPPONENT(board->to_move);
    draw_stack_count--;
}

BOOL make_move(struct t_board *board, t_bitboard pinned, struct t_move_record *move, struct t_undo *undo) {

    t_chess_color			color				= board->to_move;
    t_chess_color			opponent			= OPPONENT(color);
    t_chess_square			from				= move->from_square;
    t_chess_square			to					= move->to_square;
    t_chess_piece			piece				= move->piece;
    t_chess_piece			captured			= move->captured;
    t_chess_square			ep_capture;
    struct t_castle_record	*castle_move;

    assert(move->captured != WHITEKING && move->captured != BLACKKING);
    assert(integrity(board));

    if (!board->in_check) {
        switch (move->move_type) {
        case MOVE_CASTLE:
            if (board->chess960) {
                castle_move = &castle[move->index];
                if ((castle_move->rook_from_to & pinned) || is_in_check_after_move(board, move))
                    return FALSE;
            }
            else
            {
                if (is_in_check_after_move(board, move))
                    return FALSE;
            }
            break;
        case MOVE_PxP_EP:
            if (is_in_check_after_move(board, move))
                return FALSE;
            break;
        case MOVE_KING_MOVE:
            if (is_in_check_after_move(board, move))
                return FALSE;
            break;
        case MOVE_KINGxPIECE:
            if (is_in_check_after_move(board, move))
                return FALSE;
            break;
        case MOVE_KINGxPAWN:
            if (is_in_check_after_move(board, move))
                return FALSE;
            break;
        default:
            if (SQUARE64(move->from_square) & pinned) {
                if (is_in_check_after_move(board, move))
                    return FALSE;
            }
            else
                assert(!is_in_check_after_move(board, move));
            break;
        }
    }
    //-- Write whole tree to file
    //write_tree(board, move, TRUE, "tree.txt");

    // Transfer everthing required to the Undo record
    undo->castling					= board->castling;
    undo->ep_square					= board->ep_square;
    undo->attacker					= board->check_attacker;
    undo->fifty_move_count			= board->fifty_move_count;
    undo->in_check					= board->in_check;
    undo->move						= move;
    undo->hash						= board->hash;
    undo->pawn_hash					= board->pawn_hash;
    undo->material_hash				= board->material_hash;

    // Move on board
    board->square[from] = BLANK;

    board->to_move = opponent;
    board->castling &= move->castling_delta;

    // Update Hash
    board->hash ^= move->hash_delta;
    board->hash ^= castle_hash[board->castling ^ undo->castling];
    board->pawn_hash ^= move->pawn_hash_delta;
    if (board->ep_square)
        board->hash ^= ep_hash[COLUMN(bitscan(board->ep_square))];

    switch (move->move_type)
    {
    case MOVE_CASTLE:
        castle_move		= &castle[move->index];
        // Update bitboards
        board->piecelist[piece] ^= (move->from_to_bitboard);
        board->pieces[color][ROOK] ^= castle_move->rook_from_to;
        board->occupied[color] ^= (move->from_to_bitboard ^ castle_move->rook_from_to);
        board->all_pieces ^= (move->from_to_bitboard ^ castle_move->rook_from_to);
        // Move on board
        board->square[castle_move->rook_from] = BLANK;
        board->square[castle_move->rook_to] = castle_move->rook_piece;
        board->square[to] = piece;
        // Update flags
        board->fifty_move_count++;
        // Update Check flag
        update_in_check(board, from, castle_move->rook_to, opponent);
        // Update ep flag
        board->ep_square = 0;
        // King square
        board->king_square[color] = to;
        // Update draw stack with new hash value
        draw_stack[++draw_stack_count] = board->hash;
        assert(integrity(board));
        return TRUE;
    case MOVE_PAWN_PUSH1:
        // Update bitboards
        board->piecelist[piece] ^= move->from_to_bitboard;
        board->occupied[color] ^= move->from_to_bitboard;
        board->all_pieces ^= move->from_to_bitboard;
        // Move on board
        board->square[to] = piece;
        // Update flags
        board->fifty_move_count = 0;
        // Update Check flag
        update_in_check(board, from, to, opponent);
        // Update ep flag
        board->ep_square = 0;
        // Update draw stack with new hash value
        draw_stack[++draw_stack_count] = board->hash;
        assert(integrity(board));
        return TRUE;
    case MOVE_PAWN_PUSH2:
        // Update bitboards
        board->piecelist[piece] ^= move->from_to_bitboard;
        board->occupied[color] ^= move->from_to_bitboard;
        board->all_pieces ^= move->from_to_bitboard;
        // Move on board
        board->square[to] = piece;
        // Update flags
        board->fifty_move_count = 0;
        // Update Check flag
        update_in_check(board, from, to, opponent);
        // e.p flag
        if (pawn_attackers[opponent][(from + to) >> 1] & board->pieces[opponent][PAWN]) {
            board->ep_square = SQUARE64((from + to) >> 1);
            board->hash ^= ep_hash[COLUMN((from + to) >> 1)];
        }
        else
            board->ep_square = 0;
        // Update draw stack with new hash value
        draw_stack[++draw_stack_count] = board->hash;
        assert(integrity(board));
        return TRUE;
    case MOVE_PxPAWN:
        // Move on board
        board->square[to] = piece;
        // Update Material Hash
        board->material_hash ^= material_hash_values[captured][popcount(board->piecelist[captured])];
        // Update bitboards
        board->piecelist[piece] ^= move->from_to_bitboard;
        board->occupied[color] ^= move->from_to_bitboard;
        board->all_pieces ^= SQUARE64(from);
        board->piecelist[captured] ^= SQUARE64(to);
        board->occupied[opponent] ^= SQUARE64(to);
        // Update flags
        board->fifty_move_count = 0;
        // Update Check flag
        update_in_check(board, from, to, opponent);
        // e.p flag
        board->ep_square = 0;
        // Update draw stack with new hash value
        draw_stack[++draw_stack_count] = board->hash;
        assert(integrity(board));
        return TRUE;
    case MOVE_PxPIECE:
        // Move on board
        board->square[to] = piece;
        // Update Material Hash
        board->material_hash ^= material_hash_values[captured][popcount(board->piecelist[captured])];
        // Update bitboards
        board->piecelist[piece] ^= move->from_to_bitboard;
        board->occupied[color] ^= move->from_to_bitboard;
        board->all_pieces ^= SQUARE64(from);
        board->piecelist[captured] ^= SQUARE64(to);
        board->occupied[opponent] ^= SQUARE64(to);
        // Update flags
        board->fifty_move_count = 0;
        // Update Check flag
        update_in_check(board, from, to, opponent);
        // e.p flag
        board->ep_square = 0;
        // Update draw stack with new hash value
        draw_stack[++draw_stack_count] = board->hash;
        assert(integrity(board));
        return TRUE;
    case MOVE_PxP_EP:
        // Move on board
        board->square[to] = piece;
        ep_capture	= ((to - 8) + 16 * color);
        board->square[ep_capture] = BLANK;
        // Update Material Hash
        board->material_hash ^= material_hash_values[captured][popcount(board->piecelist[captured])];
        // Update bitboards
        board->piecelist[piece] ^= move->from_to_bitboard;
        board->occupied[color] ^= move->from_to_bitboard;
        board->all_pieces ^= (move->from_to_bitboard ^ SQUARE64(ep_capture));
        board->piecelist[captured] ^= SQUARE64(ep_capture);
        board->occupied[opponent] ^= SQUARE64(ep_capture);
        // Update flags
        board->fifty_move_count = 0;
        // Update Check flag
        if (board->in_check = attack_count(board, board->king_square[opponent], color))
            board->check_attacker = who_is_attacking_square(board, board->king_square[opponent], color);
        // e.p flag
        board->ep_square = 0;
        // Update draw stack with new hash value
        draw_stack[++draw_stack_count] = board->hash;
        assert(integrity(board));
        return TRUE;
    case MOVE_PROMOTION:
        // Move on board
        board->square[to] = move->promote_to;
        // Update Material Hash
        board->material_hash ^= material_hash_values[piece][popcount(board->piecelist[piece])];
        board->material_hash ^= material_hash_values[move->promote_to][popcount(board->piecelist[move->promote_to]) + 1];
        // Update bitboards
        board->piecelist[piece] ^= SQUARE64(from);
        board->piecelist[move->promote_to] ^= SQUARE64(to);
        board->occupied[color] ^= move->from_to_bitboard;
        board->all_pieces ^= move->from_to_bitboard;
        // Update flags
        board->fifty_move_count = 0;
        // Update Check flag
        update_in_check(board, from, to, opponent);
        // e.p flag
        board->ep_square = 0;
        // Update draw stack with new hash value
        draw_stack[++draw_stack_count] = board->hash;
        assert(integrity(board));
        return TRUE;
    case MOVE_CAPTUREPROMOTE:
        // Move on board
        board->square[to] = move->promote_to;
        // Update Material Hash
        board->material_hash ^= material_hash_values[piece][popcount(board->piecelist[piece])];
        board->material_hash ^= material_hash_values[captured][popcount(board->piecelist[captured])];
        board->material_hash ^= material_hash_values[move->promote_to][popcount(board->piecelist[move->promote_to]) + 1];
        // Update bitboards
        board->all_pieces ^= SQUARE64(from);
        board->piecelist[piece] ^= SQUARE64(from);
        board->piecelist[move->promote_to] ^= SQUARE64(to);
        board->piecelist[captured] ^= SQUARE64(to);
        board->occupied[opponent] ^= SQUARE64(to);
        board->occupied[color] ^= move->from_to_bitboard;
        // Update flags
        board->fifty_move_count = 0;
        // Update Check flag
        update_in_check(board, from, to, opponent);
        // e.p flag
        board->ep_square = 0;
        // Update draw stack with new hash value
        draw_stack[++draw_stack_count] = board->hash;
        assert(integrity(board));
        return TRUE;
    case MOVE_PIECE_MOVE:
        // Update bitboards
        board->piecelist[piece] ^= move->from_to_bitboard;
        board->occupied[color] ^= move->from_to_bitboard;
        board->all_pieces ^= move->from_to_bitboard;
        // Move on board
        board->square[to] = piece;
        // Update flags
        board->fifty_move_count++;
        // Update Check flag
        update_in_check(board, from, to, opponent);
        // Update ep flag
        board->ep_square = 0;
        // Update draw stack with new hash value
        draw_stack[++draw_stack_count] = board->hash;
        assert(integrity(board));
        return TRUE;
    case MOVE_PIECExPIECE:
        // Update Material Hash
        board->material_hash ^= material_hash_values[captured][popcount(board->piecelist[captured])];
        // Update bitboards
        board->piecelist[piece] ^= move->from_to_bitboard;
        board->occupied[color] ^= move->from_to_bitboard;
        board->all_pieces ^= SQUARE64(from);
        board->piecelist[captured] ^= SQUARE64(to);
        board->occupied[opponent] ^= SQUARE64(to);
        // Move on board
        board->square[to] = piece;
        // Update flags
        board->fifty_move_count = 0;
        // Update Check flag
        update_in_check(board, from, to, opponent);
        // Update ep flag
        board->ep_square = 0;
        // Update draw stack with new hash value
        draw_stack[++draw_stack_count] = board->hash;
        assert(integrity(board));
        return TRUE;
    case MOVE_PIECExPAWN:
        // Update Material Hash
        board->material_hash ^= material_hash_values[captured][popcount(board->piecelist[captured])];
        // Update bitboards
        board->piecelist[piece] ^= move->from_to_bitboard;
        board->occupied[color] ^= move->from_to_bitboard;
        board->all_pieces ^= SQUARE64(from);
        board->piecelist[captured] ^= SQUARE64(to);
        board->occupied[opponent] ^= SQUARE64(to);
        // Move on board
        board->square[to] = piece;
        // Update flags
        board->fifty_move_count = 0;
        // Update Check flag
        update_in_check(board, from, to, opponent);
        // Update ep flag
        board->ep_square = 0;
        // Update draw stack with new hash value
        draw_stack[++draw_stack_count] = board->hash;
        return TRUE;
    case MOVE_KING_MOVE:
        // Update bitboards
        board->piecelist[piece] ^= move->from_to_bitboard;
        board->occupied[color] ^= move->from_to_bitboard;
        board->all_pieces ^= move->from_to_bitboard;
        // Move on board
        board->square[to] = piece;
        // Update flags
        board->fifty_move_count++;
        // Update Check flag
        update_in_check(board, from, to, opponent);
        // Update ep flag
        board->ep_square = 0;
        // Update King Position
        board->king_square[color] = to;
        // Update draw stack with new hash value
        draw_stack[++draw_stack_count] = board->hash;
        assert(integrity(board));
        return TRUE;
    case MOVE_KINGxPIECE:
        // Update Material Hash
        board->material_hash ^= material_hash_values[captured][popcount(board->piecelist[captured])];
        // Update bitboards
        board->piecelist[piece] ^= move->from_to_bitboard;
        board->occupied[color] ^= move->from_to_bitboard;
        board->all_pieces ^= SQUARE64(from);
        board->piecelist[captured] ^= SQUARE64(to);
        board->occupied[opponent] ^= SQUARE64(to);
        // Move on board
        board->square[to] = piece;
        // Update flags
        board->fifty_move_count = 0;
        // Update Check flag
        update_in_check(board, from, to, opponent);
        // Update ep flag
        board->ep_square = 0;
        // Update King Position
        board->king_square[color] = to;
        // Update draw stack with new hash value
        draw_stack[++draw_stack_count] = board->hash;
        assert(integrity(board));
        return TRUE;
    case MOVE_KINGxPAWN:
        // Update Material Hash
        board->material_hash ^= material_hash_values[captured][popcount(board->piecelist[captured])];
        // Update bitboards
        board->piecelist[piece] ^= move->from_to_bitboard;
        board->occupied[color] ^= move->from_to_bitboard;
        board->all_pieces ^= SQUARE64(from);
        board->piecelist[captured] ^= SQUARE64(to);
        board->occupied[opponent] ^= SQUARE64(to);
        // Move on board
        board->square[to] = piece;
        // Update flags
        board->fifty_move_count = 0;
        // Update Check flag
        update_in_check(board, from, to, opponent);
        // Update ep flag
        board->ep_square = 0;
        // Update King Position
        board->king_square[color] = to;
        // Update draw stack with new hash value
        draw_stack[++draw_stack_count] = board->hash;
        assert(integrity(board));
        return TRUE;
    }
    assert(FALSE);
    return FALSE;
}

void unmake_move(struct t_board *board, struct t_undo *undo) {

    struct t_move_record *move		= undo->move;
    t_chess_square from				= move->from_square;
    t_chess_square to				= move->to_square;
    t_chess_piece piece				= move->piece;
    t_chess_color color				= OPPONENT(board->to_move);
    t_chess_piece captured;
    t_chess_piece promote;
    t_chess_color opponent;
    t_chess_square ep_capture;
    struct t_castle_record *castle_move;

    assert(integrity(board));

    //-- copy back to the board
    board->fifty_move_count			= undo->fifty_move_count;
    board->check_attacker			= undo->attacker;
    board->ep_square				= undo->ep_square;
    board->in_check					= undo->in_check;
    board->castling					= undo->castling;
    board->hash						= undo->hash;
    board->pawn_hash				= undo->pawn_hash;
    board->material_hash			= undo->material_hash;

    board->square[from] = piece;
    board->to_move = color;

    draw_stack_count--;

    switch (move->move_type)
    {
    case MOVE_CASTLE:
        castle_move		= &castle[move->index];
        //-- Squares
        if (to != from) // Test only necessary for Chess960
            board->square[to] = BLANK;
        if (castle_move->rook_to != from)
            board->square[castle_move->rook_to] = BLANK;
        board->square[castle_move->rook_from] = castle_move->rook_piece;
        //-- Bitboards
        board->all_pieces ^= (move->from_to_bitboard ^ castle_move->rook_from_to);
        board->occupied[color] ^= (move->from_to_bitboard ^ castle_move->rook_from_to);
        board->piecelist[piece] ^= move->from_to_bitboard;
        board->piecelist[castle_move->rook_piece] ^= castle_move->rook_from_to;
        //-- King Position
        board->king_square[color] = from;
        assert(integrity(board));
        return;
    case MOVE_PAWN_PUSH1:
        //-- Squares
        board->square[to] = BLANK;
        //-- Bitboards
        board->all_pieces ^= move->from_to_bitboard;
        board->occupied[color] ^= move->from_to_bitboard;
        board->piecelist[piece] ^= move->from_to_bitboard;
        assert(integrity(board));
        return;
    case MOVE_PAWN_PUSH2:
        //-- Squares
        board->square[to] = BLANK;
        //-- Bitboards
        board->all_pieces ^= move->from_to_bitboard;
        board->occupied[color] ^= move->from_to_bitboard;
        board->piecelist[piece] ^= move->from_to_bitboard;
        return;
    case MOVE_PxPAWN:
        captured = move->captured;
        opponent = OPPONENT(color);
        //-- Squares
        board->square[to] = captured;
        //-- Bitboards
        board->all_pieces ^= SQUARE64(from);
        board->occupied[color] ^= move->from_to_bitboard;
        board->piecelist[piece] ^= move->from_to_bitboard;
        board->piecelist[captured] ^= SQUARE64(to);
        board->occupied[opponent] ^= SQUARE64(to);
        assert(integrity(board));
        return;
    case MOVE_PxPIECE:
        captured = move->captured;
        opponent = OPPONENT(color);
        //-- Squares
        board->square[to] = captured;
        //-- Bitboards
        board->all_pieces ^= SQUARE64(from);
        board->occupied[color] ^= move->from_to_bitboard;
        board->piecelist[piece] ^= move->from_to_bitboard;
        board->piecelist[captured] ^= SQUARE64(to);
        board->occupied[opponent] ^= SQUARE64(to);
        assert(integrity(board));
        return;
    case MOVE_PxP_EP:
        captured = move->captured;
        opponent = OPPONENT(color);
        //-- Squares
        board->square[to] = BLANK;
        ep_capture	= ((to - 8) + 16 * color);
        board->square[ep_capture] = PIECEINDEX(opponent, PAWN);
        // Update bitboards
        board->piecelist[piece] ^= move->from_to_bitboard;
        board->occupied[color] ^= move->from_to_bitboard;
        board->all_pieces ^= (move->from_to_bitboard ^ SQUARE64(ep_capture));
        board->piecelist[captured] ^= SQUARE64(ep_capture);
        board->occupied[opponent] ^= SQUARE64(ep_capture);
        assert(integrity(board));
        return;
    case MOVE_PROMOTION:
        promote = move->promote_to;
        //-- Squares
        board->square[to] = BLANK;
        //-- Bitboards
        board->all_pieces ^= move->from_to_bitboard;
        board->occupied[color] ^= move->from_to_bitboard;
        board->piecelist[piece] ^= SQUARE64(from);
        board->piecelist[promote] ^= SQUARE64(to);
        assert(integrity(board));
        return;
    case MOVE_CAPTUREPROMOTE:
        opponent = OPPONENT(color);
        promote = move->promote_to;
        captured = move->captured;
        //-- Squares
        board->square[to] = captured;
        //-- Bitboards
        board->all_pieces ^= SQUARE64(from);
        board->piecelist[piece] ^= SQUARE64(from);
        board->occupied[color] ^= move->from_to_bitboard;
        board->occupied[opponent] ^= SQUARE64(to);
        board->piecelist[promote] ^= SQUARE64(to);
        board->piecelist[captured] ^= SQUARE64(to);
        assert(integrity(board));
        return;
    case MOVE_PIECE_MOVE:
        //-- Squares
        board->square[to] = BLANK;
        //-- Bitboards
        board->all_pieces ^= move->from_to_bitboard;
        board->occupied[color] ^= move->from_to_bitboard;
        board->piecelist[piece] ^= move->from_to_bitboard;
        assert(integrity(board));
        return;
    case MOVE_PIECExPIECE:
        captured = move->captured;
        opponent = OPPONENT(color);
        //-- Squares
        board->square[to] = captured;
        //-- Bitboards
        board->all_pieces ^= SQUARE64(from);
        board->occupied[color] ^= move->from_to_bitboard;
        board->piecelist[piece] ^= move->from_to_bitboard;
        board->piecelist[captured] ^= SQUARE64(to);
        board->occupied[opponent] ^= SQUARE64(to);
        assert(integrity(board));
        return;
    case MOVE_PIECExPAWN:
        captured = move->captured;
        opponent = OPPONENT(color);
        //-- Squares
        board->square[to] = captured;
        //-- Bitboards
        board->all_pieces ^= SQUARE64(from);
        board->occupied[color] ^= move->from_to_bitboard;
        board->piecelist[piece] ^= move->from_to_bitboard;
        board->piecelist[captured] ^= SQUARE64(to);
        board->occupied[opponent] ^= SQUARE64(to);
        assert(integrity(board));
        return;
    case MOVE_KING_MOVE:
        //-- Squares
        board->square[to] = BLANK;
        //-- Bitboards
        board->all_pieces ^= move->from_to_bitboard;
        board->occupied[color] ^= move->from_to_bitboard;
        board->piecelist[piece] ^= move->from_to_bitboard;
        //-- King location
        board->king_square[color] = from;
        assert(integrity(board));
        return;
    case MOVE_KINGxPIECE:
        captured = move->captured;
        opponent = OPPONENT(color);
        //-- Squares
        board->square[to] = captured;
        //-- Bitboards
        board->all_pieces ^= SQUARE64(from);
        board->occupied[color] ^= move->from_to_bitboard;
        board->piecelist[piece] ^= move->from_to_bitboard;
        board->piecelist[captured] ^= SQUARE64(to);
        board->occupied[opponent] ^= SQUARE64(to);
        //-- King location
        board->king_square[color] = from;
        assert(integrity(board));
        return;
    case MOVE_KINGxPAWN:
        captured = move->captured;
        opponent = OPPONENT(color);
        //-- Squares
        board->square[to] = captured;
        //-- Bitboards
        board->all_pieces ^= SQUARE64(from);
        board->occupied[color] ^= move->from_to_bitboard;
        board->piecelist[piece] ^= move->from_to_bitboard;
        board->piecelist[captured] ^= SQUARE64(to);
        board->occupied[opponent] ^= SQUARE64(to);
        //-- King location
        board->king_square[color] = from;
        assert(integrity(board));
        return;
    }
    assert(FALSE);
}


int legal_move_count(struct t_board *board, struct t_move_list *move_list) {
    int i;
    int n = 0;
    for (i = move_list->count - 1; i >= 0; i--) {

        switch (move_list->move[i]->move_type) {
        case MOVE_CASTLE:
            if (board->chess960) {
                struct t_castle_record *castle_move;
                castle_move = &castle[move_list->move[i]->index];
                if (!((castle_move->rook_from_to & move_list->pinned_pieces) || is_in_check_after_move(board, move_list->move[i])))
                    n++;
            }
            else
            {
                if (!is_in_check_after_move(board, move_list->move[i]))
                    n++;
            }
            break;
        case MOVE_PxP_EP:
            if (!is_in_check_after_move(board, move_list->move[i]))
                n++;
            break;
        case MOVE_KING_MOVE:
            if (!is_in_check_after_move(board, move_list->move[i]))
                n++;
            break;
        case MOVE_KINGxPIECE:
            if (!is_in_check_after_move(board, move_list->move[i]))
                n++;
            break;
        case MOVE_KINGxPAWN:
            if (!is_in_check_after_move(board, move_list->move[i]))
                n++;
            break;
        default:
            if (move_list->move[i]->from_to_bitboard & move_list->pinned_pieces) {
                if (!is_in_check_after_move(board, move_list->move[i]))
                    n++;
            }
            else
                n++;
            break;
        }

    }
    return n;
}

void make_game_move(struct t_board *board, char *s)
{
    struct t_move_record	*move;
    struct t_undo			undo[1];

    move = lookup_move(board, s);
    make_move(board, 0, move, undo);

}


BOOL make_next_see_positive_move(struct t_board *board, struct t_move_list *move_list, t_chess_value see_margin, struct t_undo *undo) {

    struct t_move_record *move;
    int ibest;
    t_chess_value best_value;

    //-- Test for no more moves
    while (move_list->imove > 0) {

        //-- Decrement the move-played index
        move_list->imove--;

        //-- Initialize the best move
        ibest = move_list->imove;
        best_value = move_list->value[ibest];

        //-- Loop round and find the best move
        for (int i = move_list->imove - 1; i >= 0; i--) {
            if (move_list->value[i] > best_value) {
                best_value = move_list->value[i];
                ibest = i;
            }
        }

        //-- Store the best move
        move = move_list->move[ibest];

        //-- Remove the move from the move list
        move_list->move[ibest] = move_list->move[move_list->imove];
        move_list->value[ibest] = move_list->value[move_list->imove];

        //-- Test to ensure if move is SEE positive
        if (see(board, move, see_margin)) {

            //-- Make move on board
            if (make_move(board, move_list->pinned_pieces, move, undo)) {
                move_list->current_move = move;
                return TRUE;
            }
        }
    }

    //-- No more moves!
    return FALSE;
}

BOOL make_next_best_move(struct t_board *board, struct t_move_list *move_list, struct t_undo *undo) {

    int ibest;
    t_chess_value best_value;

    //-- Test for no more moves
    while (move_list->imove > 0) {

        //-- Decrement the move-played index
        move_list->imove--;

        //-- Initialize the best move
        ibest = move_list->imove;
        best_value = move_list->value[ibest];

        //-- Loop round and find the best move
        for (int i = move_list->imove - 1; i >= 0; i--) {
            if (move_list->value[i] > best_value) {
                best_value = move_list->value[i];
                ibest = i;
            }
        }

        //-- Store the move
        move_list->current_move = move_list->move[ibest];

        //-- Remove the move from the move list
        move_list->move[ibest] = move_list->move[move_list->imove];
        move_list->value[ibest] = move_list->value[move_list->imove];

        //-- Make move on board
        if (make_move(board, move_list->pinned_pieces, move_list->current_move, undo)) {
            return TRUE;
        }
    }

    //-- No more moves!
    return FALSE;
}

BOOL make_next_move(struct t_board *board, struct t_move_list *move_list, struct t_move_list *bad_move_list, struct t_undo *undo) {

    int ibest;
    t_chess_value best_value;
    struct t_move_record *move;

    assert(bad_move_list != NULL);

    //-- Test for no more moves in the "normal" move list
    while (move_list->imove > 0) {

        //-- Decrement the move-played index
        move_list->imove--;

        //-- Initialize the best move
        ibest = move_list->imove;
        best_value = move_list->value[ibest];

        //-- Loop round and find the best move
        for (int i = move_list->imove - 1; i >= 0; i--) {
            if (move_list->value[i] > best_value) {
                best_value = move_list->value[i];
                ibest = i;
            }
        }
        move = move_list->move[ibest];

        //-- Remove the move from the move list
        move_list->move[ibest] = move_list->move[move_list->imove];
        move_list->value[ibest] = move_list->value[move_list->imove];

        //-- Is the move a SEE positive capture or a hash move
        if (!move->captured || move == move_list->hash_move || see(board, move, 0)) {

            //-- Store the move
            move_list->current_move = move;
            move_list->current_move_see_positive = TRUE;

            //-- Make move on board
            if (make_move(board, move_list->pinned_pieces, move, undo))
                return TRUE;
        }
        else {

            //-- Transfer the move over the other "bad" move list
            int j = bad_move_list->count;
            bad_move_list->move[j] = move;
            bad_move_list->value[j] = best_value;
            bad_move_list->imove = ++bad_move_list->count;
        }
    }

    //-- Now see if there are any moves in the "bad" move list
    while (bad_move_list->imove > 0) {

        //-- Decrement the move-played index
        bad_move_list->imove--;

        //-- Initialize the best move
        ibest = bad_move_list->imove;
        best_value = bad_move_list->value[ibest];

        //-- Loop round and find the best move
        for (int i = bad_move_list->imove - 1; i >= 0; i--) {
            if (bad_move_list->value[i] > best_value) {
                best_value = bad_move_list->value[i];
                ibest = i;
            }
        }
        move = bad_move_list->move[ibest];

        assert(move->captured);

        //-- Remove the move from the move list
        bad_move_list->move[ibest] = bad_move_list->move[bad_move_list->imove];
        bad_move_list->value[ibest] = bad_move_list->value[bad_move_list->imove];

        //-- Store the move
        move_list->current_move = move;
        move_list->current_move_see_positive = FALSE;

        //-- Make move on board
        if (make_move(board, move_list->pinned_pieces, move, undo))
            return TRUE;
    }

    //-- No more moves!
    return FALSE;
}

BOOL simple_make_next_move(struct t_board *board, struct t_move_list *move_list, struct t_undo *undo)
{
    struct t_move_record *move;

    //-- Now see if there are any moves in move list
    while (move_list->imove > 0) {

        //-- Decrement the move-played index
        move_list->imove--;

        move = move_list->move[move_list->imove];
        move_list->current_move = move;

        //-- Make move on board
        if (make_move(board, move_list->pinned_pieces, move, undo))
            return TRUE;
    }

    //-- No more moves!
    return FALSE;
}

BOOL is_move_legal(struct t_board *board, struct t_move_record *move)
{
    struct t_move_list move_list[1];
    struct t_undo undo[1];

    if (board->in_check)
        generate_evade_check(board, move_list);
    else
        generate_moves(board, move_list);

    for (int i = move_list->count - 1; i >= 0; i--) {
        if (move_list->move[i] == move) {
            if (make_move(board, move_list->pinned_pieces, move, undo)) {
                unmake_move(board, undo);
                return TRUE;
            }
            else
                return FALSE;
        }
    }
    return FALSE;
}