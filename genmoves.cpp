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

void generate_legal_moves(struct t_board *board, struct t_move_list *move_list)
{
    if (board->in_check)
        generate_evade_check(board, move_list);
    else {
        struct t_move_list xmove_list[1];
        struct t_undo undo[1];

        generate_moves(board, xmove_list);
        move_list->count = 0;
        move_list->pinned_pieces = xmove_list->pinned_pieces;
        for (int i = 0; i < xmove_list->count; i++)
        {
            if (make_move(board, xmove_list->pinned_pieces, xmove_list->move[i], undo)) {
                move_list->move[move_list->count++] = xmove_list->move[i];
                unmake_move(board, undo);
            }
        }
    }
    move_list->imove = move_list->count;
}

void generate_moves(struct t_board *board, struct t_move_list *move_list) {

    t_bitboard _all_pieces = board->all_pieces;

    struct t_move_record *move;
    t_chess_color to_move = board->to_move;
    t_chess_color opponent = OPPONENT(to_move);
    t_bitboard not_occupied_to_move = ~board->occupied[to_move];

    t_bitboard moves = 0;
    t_bitboard source_piece;
    // Types of pawn move
    t_bitboard double_push;
    t_bitboard pawn_promotions;

    t_chess_piece piece;
    t_chess_piece captured;
    t_chess_piece promote_to;
    t_chess_square from_square, to_square;
    int castle_index;
    int forward;
    int piece_color = (to_move * 8);

    move_list->count = 0;

    //+---------------------------------+
    //| Find the Pinned Pieces          |
    //+---------------------------------+
    t_bitboard pinner, pinned;
    move_list->pinned_pieces = 0;
    //-- Bishops & Queens
    assert(to_move >= 0 && to_move < 2);
    assert(board->king_square[to_move] >= 0 && board->king_square[to_move] < 64);
    assert(board->pieces[WHITE] == board->piecelist);
    assert(board->pieces[BLACK] == board->piecelist + 8);
    t_bitboard b = bishop_rays[board->king_square[to_move]] & (board->pieces[opponent][BISHOP] | board->pieces[opponent][QUEEN]);
    while (b) {
        pinner = bitscan_reset(&b);
        pinned = between[board->king_square[to_move]][pinner] & _all_pieces;
        if (popcount(pinned) == 1)
            move_list->pinned_pieces |= (pinned & board->occupied[to_move]);
    }
    //--Rooks & Queens
    b = rook_rays[board->king_square[to_move]] & (board->pieces[opponent][ROOK] | board->pieces[opponent][QUEEN]);
    while (b) {
        pinner = bitscan_reset(&b);
        pinned = between[board->king_square[to_move]][pinner] & _all_pieces;
        if (popcount(pinned) == 1)
            move_list->pinned_pieces |= (pinned & board->occupied[to_move]);
    }

    //+---------------------------------+
    //| Castling Moves                  |
    //+---------------------------------+

    if (board->castling && !board->in_check) {

        //-- Kingside O-O
        castle_index = (to_move * 2);
        if ((board->castling >> castle_index) & (uchar)1) {
            move = &xmove_list[castle_index];
            if (!(_all_pieces & castle[castle_index].possible)) {
                move_list->move[move_list->count++] = &xmove_list[castle_index];
            }
        }
        //-- Queenside O-O-O
        castle_index++;
        if ((board->castling >> castle_index) & (uchar)1) {
            move = &xmove_list[castle_index];
            if (!(_all_pieces & castle[castle_index].possible)) {
                move_list->move[move_list->count++] = &xmove_list[castle_index];
            }
        }
    }

    //+---------------------------------+
    //| Pawn Moves                      |
    //+---------------------------------+
    // find the direction of a pawn push
    forward = 8 - 16 * to_move;
    piece = PAWN + piece_color;
    // All pawn pushes
    moves = (((board->piecelist[piece] << 8) >> (16 * to_move)) & ~(_all_pieces));
    // Pawn priomotions
    pawn_promotions = moves & rank_mask[to_move][EIGHTH_RANK];
    // Take out Pawn Promotions
    moves ^= pawn_promotions;
    // Double moves
    double_push = (moves & rank_mask[to_move][THIRD_RANK]);
    // Spool the moves to the move list
    while (moves) {
        to_square = bitscan_reset(&moves);
        from_square = to_square - forward;
        move_list->move[move_list->count++] = move_directory[from_square][to_square][piece];
    }
    // Double Moves
    double_push = (((double_push << 8) >> (16 * to_move)) & ~(_all_pieces));
    while (double_push) {
        to_square = bitscan_reset(&double_push);
        from_square = to_square - (forward * 2);
        move_list->move[move_list->count++] = move_directory[from_square][to_square][piece];
    }
    // Pawn promotions
    while (pawn_promotions) {
        to_square = bitscan_reset(&pawn_promotions);
        from_square = to_square - forward;
        for (promote_to = 0; promote_to <= 3; promote_to++)
            move_list->move[move_list->count++] = move_directory[from_square][to_square][piece] + promote_to;
    }
    // Pawn captures
    moves = (((board->piecelist[piece] & B8H1) << 7) >> (16 * to_move)) & (board->occupied[opponent] | board->ep_square);
    pawn_promotions = (moves & rank_mask[to_move][EIGHTH_RANK]);
    moves ^= pawn_promotions;
    while (moves) {
        to_square = bitscan_reset(&moves);
        from_square = to_square - forward + 1;
        captured = PIECETYPE(board->square[to_square]);
        move_list->move[move_list->count++] = move_directory[from_square][to_square][piece] + PIECETYPE(captured);
    }
    while (pawn_promotions) {
        to_square = bitscan_reset(&pawn_promotions);
        from_square = to_square - forward + 1;
        captured = PIECETYPE(board->square[to_square]);
        for (promote_to = 0; promote_to <= 3; promote_to++)
            move_list->move[move_list->count++] = move_directory[from_square][to_square][piece] + (4 * captured) + promote_to;
    }
    // Other direction
    moves = (((board->piecelist[piece] & A8G1) << 9) >> (16 * to_move)) & (board->occupied[opponent] | board->ep_square);
    pawn_promotions = (moves & rank_mask[to_move][EIGHTH_RANK]);
    moves ^= pawn_promotions;
    while (moves) {
        to_square = bitscan_reset(&moves);
        from_square = to_square - forward - 1;
        captured = PIECETYPE(board->square[to_square]);
        move_list->move[move_list->count++] = move_directory[from_square][to_square][piece] + PIECETYPE(captured);
    }
    while (pawn_promotions) {
        to_square = bitscan_reset(&pawn_promotions);
        from_square = to_square - forward - 1;
        captured = PIECETYPE(board->square[to_square]);
        for (promote_to = 0; promote_to <= 3; promote_to++)
            move_list->move[move_list->count++] = move_directory[from_square][to_square][piece] + (4 * captured) + promote_to;
    }

    //+---------------------------------+
    //| Knight Moves                    |
    //+---------------------------------+
    piece = KNIGHT + piece_color;
    source_piece = board->piecelist[piece] & ~move_list->pinned_pieces;
    while (source_piece) {
        from_square = bitscan_reset(&source_piece);
        moves = (knight_mask[from_square] & not_occupied_to_move);
        while (moves) {
            to_square = bitscan_reset(&moves);
            captured = PIECETYPE(board->square[to_square]);
            move_list->move[move_list->count++] = move_directory[from_square][to_square][piece] + captured;
        }
    }

    //+---------------------------------+
    //| King Moves                      |
    //+---------------------------------+
    piece = KING + piece_color;
    from_square = board->king_square[to_move];
    moves = (king_mask[from_square] & not_occupied_to_move);
    while (moves) {
        to_square = bitscan_reset(&moves);
        captured = PIECETYPE(board->square[to_square]);
        move_list->move[move_list->count++] = move_directory[from_square][to_square][piece] + captured;
    }

    //+---------------------------------+
    //| Rook Moves                      |
    //+---------------------------------+
    piece = ROOK + piece_color;
    source_piece = board->piecelist[piece];
    while (source_piece) {
        from_square = bitscan_reset(&source_piece);
        moves = rook_magic_moves[from_square][((rook_magic[from_square].mask & _all_pieces) * rook_magic[from_square].magic) >> 52];
        moves &= not_occupied_to_move;
        while (moves) {
            to_square = bitscan_reset(&moves);
            captured = PIECETYPE(board->square[to_square]);
            move_list->move[move_list->count++] = move_directory[from_square][to_square][piece] + captured;
        }
    }

    //+---------------------------------+
    //| Bishop Moves                    |
    //+---------------------------------+
    piece = BISHOP + piece_color;
    source_piece = board->piecelist[piece];
    while (source_piece) {
        from_square = bitscan_reset(&source_piece);
        moves = bishop_magic_moves[from_square][((bishop_magic[from_square].mask & _all_pieces) * bishop_magic[from_square].magic) >> 55];
        moves &= not_occupied_to_move;
        while (moves) {
            to_square = bitscan_reset(&moves);
            captured = PIECETYPE(board->square[to_square]);
            move_list->move[move_list->count++] = move_directory[from_square][to_square][piece] + captured;
        }
    }

    //+---------------------------------+
    //| Queen Moves                     |
    //+---------------------------------+
    piece = QUEEN + piece_color;
    source_piece = board->piecelist[piece];
    while (source_piece) {
        from_square = bitscan_reset(&source_piece);
        moves = rook_magic_moves[from_square][((rook_magic[from_square].mask & _all_pieces) * rook_magic[from_square].magic) >> 52];
        moves &= not_occupied_to_move;
        while (moves) {
            to_square = bitscan_reset(&moves);
            captured = PIECETYPE(board->square[to_square]);
            move_list->move[move_list->count++] = move_directory[from_square][to_square][piece] + captured;
        }
        moves = bishop_magic_moves[from_square][((bishop_magic[from_square].mask & _all_pieces) * bishop_magic[from_square].magic) >> 55];
        moves &= not_occupied_to_move;
        while (moves) {
            to_square = bitscan_reset(&moves);
            captured = PIECETYPE(board->square[to_square]);
            move_list->move[move_list->count++] = move_directory[from_square][to_square][piece] + captured;
        }
    }
    //-- Set the move index to the count
    move_list->imove = move_list->count;
}

//---------------------------------------------------------------------------------//
// Generates "obvious" checks - no castling, en-passant, discovered or promotions
//---------------------------------------------------------------------------------//
void generate_quiet_checks(struct t_board *board, struct t_move_list *move_list) {

    t_bitboard _all_pieces = board->all_pieces;

    t_chess_color to_move = board->to_move;
    t_chess_color opponent = OPPONENT(to_move);
    t_bitboard not_occupied_to_move = ~board->occupied[to_move];

    t_bitboard moves = 0;
    t_bitboard source_piece;
    // Types of pawn move
    t_bitboard double_push;

    t_chess_piece piece;
    t_chess_square from_square, to_square;
    int forward;
    int piece_color = (to_move * 8);

    //+------------------------------------------------------+
    //| Calculate check_rays - squares which will give check |
    //+------------------------------------------------------+
    from_square = board->king_square[opponent];
    t_bitboard bishop_check_rays = ~_all_pieces & bishop_magic_moves[from_square][((bishop_magic[from_square].mask & _all_pieces) * bishop_magic[from_square].magic) >> 55];
    t_bitboard rook_check_rays = ~_all_pieces & rook_magic_moves[from_square][((rook_magic[from_square].mask & _all_pieces) * rook_magic[from_square].magic) >> 52];
    t_bitboard queen_check_rays = rook_check_rays | bishop_check_rays;

    //--DOES NOT ZERO THE MOVE LIST!!
    //move_list->count = 0;

    //+---------------------------------+
    //| Find the Pinned Pieces          |
    //+---------------------------------+

    //--ASSUME PINNED PIECES HAVE ALREADY BEEN CALCULATED!!

    //+---------------------------------+
    //| Castling Moves                  |
    //+---------------------------------+

    //--IGNORE CASTLING MOVES!!

    //+---------------------------------+
    //| Pawn Moves                      |
    //+---------------------------------+
    // find the direction of a pawn push
    forward = 8 - 16 * to_move;
    piece = PAWN + piece_color;
    // All pawn pushes
    moves = ((pawn_attackers[to_move][board->king_square[opponent]] & ~_all_pieces) >> 8) << (16 * to_move);
    double_push = moves & ~_all_pieces & rank_mask[to_move][THIRD_RANK];
    double_push = ((double_push >> 8) << (16 * to_move)) & board->piecelist[piece];
    moves &= board->piecelist[piece];

    // Spool the moves to the move list
    while (moves) {
        from_square = bitscan_reset(&moves);
        to_square = from_square + forward;
        move_list->move[move_list->count++] = move_directory[from_square][to_square][piece];
    }
    // Double Moves
    while (double_push) {
        from_square = bitscan_reset(&double_push);
        to_square = from_square + (forward * 2);
        move_list->move[move_list->count++] = move_directory[from_square][to_square][piece];
    }

    //+---------------------------------+
    //| Knight Moves                    |
    //+---------------------------------+
    t_bitboard knight_check_rays = knight_mask[board->king_square[opponent]] & ~_all_pieces;
    piece = KNIGHT + piece_color;
    source_piece = board->piecelist[piece] & ~move_list->pinned_pieces;
    while (source_piece) {
        from_square = bitscan_reset(&source_piece);
        moves = (knight_mask[from_square] & not_occupied_to_move) & knight_check_rays;
        while (moves) {
            to_square = bitscan_reset(&moves);
            move_list->move[move_list->count++] = move_directory[from_square][to_square][piece];
        }
    }

    //+---------------------------------+
    //| King Moves                      |
    //+---------------------------------+

    //--NOT RELEVANT!

    //+---------------------------------+
    //| Rook Moves                      |
    //+---------------------------------+
    piece = ROOK + piece_color;
    source_piece = board->piecelist[piece];
    while (source_piece) {
        from_square = bitscan_reset(&source_piece);
        moves = rook_magic_moves[from_square][((rook_magic[from_square].mask & _all_pieces) * rook_magic[from_square].magic) >> 52];
        moves &= not_occupied_to_move & rook_check_rays;
        while (moves) {
            to_square = bitscan_reset(&moves);
            move_list->move[move_list->count++] = move_directory[from_square][to_square][piece];
        }
    }

    //+---------------------------------+
    //| Bishop Moves                    |
    //+---------------------------------+
    piece = BISHOP + piece_color;
    source_piece = board->piecelist[piece];
    while (source_piece) {
        from_square = bitscan_reset(&source_piece);
        moves = bishop_magic_moves[from_square][((bishop_magic[from_square].mask & _all_pieces) * bishop_magic[from_square].magic) >> 55];
        moves &= not_occupied_to_move & bishop_check_rays;
        while (moves) {
            to_square = bitscan_reset(&moves);
            move_list->move[move_list->count++] = move_directory[from_square][to_square][piece];
        }
    }

    //+---------------------------------+
    //| Queen Moves                     |
    //+---------------------------------+
    piece = QUEEN + piece_color;
    source_piece = board->piecelist[piece];
    while (source_piece) {
        from_square = bitscan_reset(&source_piece);
        moves = rook_magic_moves[from_square][((rook_magic[from_square].mask & _all_pieces) * rook_magic[from_square].magic) >> 52];
        moves &= not_occupied_to_move & queen_check_rays;
        while (moves) {
            to_square = bitscan_reset(&moves);
            move_list->move[move_list->count++] = move_directory[from_square][to_square][piece];
        }
        moves = bishop_magic_moves[from_square][((bishop_magic[from_square].mask & _all_pieces) * bishop_magic[from_square].magic) >> 55];
        moves &= not_occupied_to_move & queen_check_rays;
        while (moves) {
            to_square = bitscan_reset(&moves);
            move_list->move[move_list->count++] = move_directory[from_square][to_square][piece];
        }
    }
    //-- Set the move index to the count
    move_list->imove = move_list->count;
}

void generate_captures(struct t_board *board, struct t_move_list *move_list) {

    t_bitboard _all_pieces = board->all_pieces;

    t_chess_color to_move = board->to_move;
    t_chess_color opponent = OPPONENT(to_move);
    t_bitboard not_occupied_to_move = ~board->occupied[to_move];

    t_bitboard moves = 0;
    t_bitboard source_piece;
    // Types of pawn move
    t_bitboard pawn_promotions;

    t_chess_piece piece;
    t_chess_piece captured;
    t_chess_square from_square, to_square;
    int forward;
    int piece_color = (to_move * 8);

    move_list->count = 0;

    //+---------------------------------+
    //| Find the Pinned Pieces          |
    //+---------------------------------+
    t_bitboard pinner, pinned;
    move_list->pinned_pieces = 0;
    //-- Bishops & Queens
    t_bitboard b = bishop_rays[board->king_square[to_move]] & (board->pieces[opponent][BISHOP] | board->pieces[opponent][QUEEN]);
    while (b) {
        pinner = bitscan_reset(&b);
        pinned = between[board->king_square[to_move]][pinner] & _all_pieces;
        if (popcount(pinned) == 1)
            move_list->pinned_pieces |= (pinned & board->occupied[to_move]);
    }
    //--Rooks & Queens
    b = rook_rays[board->king_square[to_move]] & (board->pieces[opponent][ROOK] | board->pieces[opponent][QUEEN]);
    while (b) {
        pinner = bitscan_reset(&b);
        pinned = between[board->king_square[to_move]][pinner] & _all_pieces;
        if (popcount(pinned) == 1)
            move_list->pinned_pieces |= (pinned & board->occupied[to_move]);
    }

    //+---------------------------------+
    //| Pawn Moves                      |
    //+---------------------------------+
    // find the direction of a pawn push
    forward = 8 - 16 * to_move;
    piece = PAWN + piece_color;
    // All pawn pushes
    moves = (((board->piecelist[piece] << 8) >> (16 * to_move)) & ~(_all_pieces));
    // Pawn priomotions
    pawn_promotions = moves & rank_mask[to_move][EIGHTH_RANK];
    // Take out Pawn Promotions
    moves ^= pawn_promotions;
    // Pawn promotions
    while (pawn_promotions) {
        to_square = bitscan_reset(&pawn_promotions);
        from_square = to_square - forward;
        move_list->move[move_list->count++] = move_directory[from_square][to_square][piece] + 3;
    }
    // Pawn captures
    moves = (((board->piecelist[piece] & B8H1) << 7) >> (16 * to_move)) & (board->occupied[opponent] | board->ep_square);
    pawn_promotions = (moves & rank_mask[to_move][EIGHTH_RANK]);
    moves ^= pawn_promotions;
    while (moves) {
        to_square = bitscan_reset(&moves);
        from_square = to_square - forward + 1;
        captured = PIECETYPE(board->square[to_square]);
        move_list->move[move_list->count++] = move_directory[from_square][to_square][piece] + PIECETYPE(captured);
    }
    while (pawn_promotions) {
        to_square = bitscan_reset(&pawn_promotions);
        from_square = to_square - forward + 1;
        captured = PIECETYPE(board->square[to_square]);
        move_list->move[move_list->count++] = move_directory[from_square][to_square][piece] + (4 * captured) + 3;
    }
    // Other direction
    moves = (((board->piecelist[piece] & A8G1) << 9) >> (16 * to_move)) & (board->occupied[opponent] | board->ep_square);
    pawn_promotions = (moves & rank_mask[to_move][EIGHTH_RANK]);
    moves ^= pawn_promotions;
    while (moves) {
        to_square = bitscan_reset(&moves);
        from_square = to_square - forward - 1;
        captured = PIECETYPE(board->square[to_square]);
        move_list->move[move_list->count++] = move_directory[from_square][to_square][piece] + PIECETYPE(captured);
    }
    while (pawn_promotions) {
        to_square = bitscan_reset(&pawn_promotions);
        from_square = to_square - forward - 1;
        captured = PIECETYPE(board->square[to_square]);
        move_list->move[move_list->count++] = move_directory[from_square][to_square][piece] + (4 * captured) + 3;
    }

    //+---------------------------------+
    //| Knight Moves                    |
    //+---------------------------------+
    piece = KNIGHT + piece_color;
    source_piece = board->piecelist[piece] & ~move_list->pinned_pieces;
    while (source_piece) {
        from_square = bitscan_reset(&source_piece);
        moves = (knight_mask[from_square] & board->occupied[opponent]);
        while (moves) {
            to_square = bitscan_reset(&moves);
            captured = PIECETYPE(board->square[to_square]);
            move_list->move[move_list->count++] = move_directory[from_square][to_square][piece] + captured;
        }
    }

    //+---------------------------------+
    //| King Moves                      |
    //+---------------------------------+
    piece = KING + piece_color;
    from_square = board->king_square[to_move];
    moves = (king_mask[from_square] & board->occupied[opponent]);
    while (moves) {
        to_square = bitscan_reset(&moves);
        captured = PIECETYPE(board->square[to_square]);
        move_list->move[move_list->count++] = move_directory[from_square][to_square][piece] + captured;
    }

    //+---------------------------------+
    //| Rook Moves                      |
    //+---------------------------------+
    piece = ROOK + piece_color;
    source_piece = board->piecelist[piece];
    while (source_piece) {
        from_square = bitscan_reset(&source_piece);
        moves = rook_magic_moves[from_square][((rook_magic[from_square].mask & _all_pieces) * rook_magic[from_square].magic) >> 52];
        moves &= board->occupied[opponent];
        while (moves) {
            to_square = bitscan_reset(&moves);
            captured = PIECETYPE(board->square[to_square]);
            move_list->move[move_list->count++] = move_directory[from_square][to_square][piece] + captured;
        }
    }

    //+---------------------------------+
    //| Bishop Moves                    |
    //+---------------------------------+
    piece = BISHOP + piece_color;
    source_piece = board->piecelist[piece];
    while (source_piece) {
        from_square = bitscan_reset(&source_piece);
        moves = bishop_magic_moves[from_square][((bishop_magic[from_square].mask & _all_pieces) * bishop_magic[from_square].magic) >> 55];
        moves &= board->occupied[opponent];
        while (moves) {
            to_square = bitscan_reset(&moves);
            captured = PIECETYPE(board->square[to_square]);
            move_list->move[move_list->count++] = move_directory[from_square][to_square][piece] + captured;
        }
    }

    //+---------------------------------+
    //| Queen Moves                     |
    //+---------------------------------+
    piece = QUEEN + piece_color;
    source_piece = board->piecelist[piece];
    while (source_piece) {
        from_square = bitscan_reset(&source_piece);
        moves = rook_magic_moves[from_square][((rook_magic[from_square].mask & _all_pieces) * rook_magic[from_square].magic) >> 52];
        moves &= board->occupied[opponent];
        while (moves) {
            to_square = bitscan_reset(&moves);
            captured = PIECETYPE(board->square[to_square]);
            move_list->move[move_list->count++] = move_directory[from_square][to_square][piece] + captured;
        }
        moves = bishop_magic_moves[from_square][((bishop_magic[from_square].mask & _all_pieces) * bishop_magic[from_square].magic) >> 55];
        moves &= board->occupied[opponent];
        while (moves) {
            to_square = bitscan_reset(&moves);
            captured = PIECETYPE(board->square[to_square]);
            move_list->move[move_list->count++] = move_directory[from_square][to_square][piece] + captured;
        }
    }
    //-- Set the move index to the count
    move_list->imove = move_list->count;
}

void generate_evade_check(struct t_board *board, struct t_move_list *move_list) {

    t_bitboard _all_pieces = board->all_pieces;
    t_chess_piece piece, captured, promote_to;
    t_chess_square from_square, to_square;
    t_bitboard moves;
    t_bitboard pawn_promotions, double_push, interpose;
    t_chess_color to_move = board->to_move;
    t_chess_color opponent = OPPONENT(to_move);
    int piece_color = (to_move * 8);

    assert(board->in_check);
    //-- Reset move count
    move_list->count = 0;

    //+---------------------------------+
    //| King Moves                      |
    //+---------------------------------+
    piece = KING + piece_color;
    from_square = board->king_square[to_move];
    moves = (king_mask[from_square] & ~board->occupied[to_move]);
    board->all_pieces ^= board->pieces[to_move][KING];
    while (moves) {
        to_square = bitscan_reset(&moves);
        if (!is_square_attacked(board, to_square, opponent)) {
            captured = PIECETYPE(board->square[to_square]);
            move_list->move[move_list->count++] = move_directory[from_square][to_square][piece] + captured;
        }
    }
    board->all_pieces ^= board->pieces[to_move][KING];

    // if Double Check then exit!
    if (board->in_check > 1) {
        move_list->imove = move_list->count;
        return;
    }

    //+---------------------------------+
    //| Find the Pinned Pieces          |
    //+---------------------------------+
    t_bitboard pinner, pinned;
    move_list->pinned_pieces = 0;
    //-- Bishops & Queens
    t_bitboard b = bishop_rays[board->king_square[to_move]] & (board->pieces[opponent][BISHOP] | board->pieces[opponent][QUEEN]);
    while (b) {
        pinner = bitscan_reset(&b);
        pinned = between[board->king_square[to_move]][pinner] & _all_pieces;
        if (popcount(pinned) == 1)
            move_list->pinned_pieces |= (pinned & board->occupied[to_move]);
    }
    //--Rooks & Queens
    b = rook_rays[board->king_square[to_move]] & (board->pieces[opponent][ROOK] | board->pieces[opponent][QUEEN]);
    while (b) {
        pinner = bitscan_reset(&b);
        pinned = between[board->king_square[to_move]][pinner] & _all_pieces;
        if (popcount(pinned) == 1)
            move_list->pinned_pieces |= (pinned & board->occupied[to_move]);
    }
    pinned = move_list->pinned_pieces;

    //-- PAWN Moves
    piece = PAWN + piece_color;
    //-- Pawn captures attacker
    moves = (pawn_attackers[to_move][board->check_attacker] & board->pieces[to_move][PAWN] & ~pinned);
    // Pawn priomotions
    pawn_promotions = moves & rank_mask[to_move][SEVENTH_RANK];
    // Take out Pawn Promotions
    moves ^= pawn_promotions;
    while (moves) {
        captured = board->square[board->check_attacker];
        assert(captured != BLANK);
        assert(COLOR(captured) == opponent);
        from_square = bitscan_reset(&moves);
        move_list->move[move_list->count++] = move_directory[from_square][board->check_attacker][piece] + PIECETYPE(captured);
    }
    while (pawn_promotions) {
        from_square = bitscan_reset(&pawn_promotions);
        to_square = board->check_attacker;
        captured = PIECETYPE(board->square[to_square]);
        for (promote_to = 0; promote_to <= 3; promote_to++) {
            move_list->move[move_list->count++] = move_directory[from_square][to_square][piece] + (4 * captured) + promote_to;
        }
    }

    //-- e.p. pawn capture (rare case!)
    if (board->ep_square) {
        if (PIECETYPE(board->square[board->check_attacker]) == PAWN) {
            to_square = bitscan(board->ep_square);
            moves = (pawn_attackers[to_move][to_square] & board->pieces[to_move][PAWN]);
            do {
                from_square = bitscan_reset(&moves);
                board->all_pieces ^= (SQUARE64(from_square) | SQUARE64((to_square - 8) + (16 * to_move)));
                board->pieces[opponent][PAWN] ^= SQUARE64(board->check_attacker);
                if (!is_square_attacked(board, board->king_square[to_move], opponent))
                    move_list->move[move_list->count++] = move_directory[from_square][to_square][piece];
                board->all_pieces ^= (SQUARE64(from_square) | SQUARE64((to_square - 8) + (16 * to_move)));
                board->pieces[opponent][PAWN] ^= SQUARE64(board->check_attacker);
            } while (moves);
        }
    }

    //-- Set the squares to interpose
    interpose = between[board->king_square[to_move]][board->check_attacker];

    if (interpose) {
        //-- Pawn interposes
        // All pawn pushes
        moves = board->piecelist[piece] & ~pinned;
        moves = (((moves << 8) >> (16 * to_move)) & ~(board->all_pieces));
        // Pawn priomotions
        pawn_promotions = moves & rank_mask[to_move][EIGHTH_RANK] & interpose;
        // Take out Pawn Promotions
        moves ^= pawn_promotions;
        // Double moves
        double_push = (moves & rank_mask[to_move][THIRD_RANK]);
        moves &= interpose;
        // Spool the moves to the move list
        while (moves) {
            to_square = bitscan_reset(&moves);
            from_square = to_square - 8 + 16 * to_move;
            move_list->move[move_list->count++] = move_directory[from_square][to_square][piece];
        }
        // Double Moves
        double_push = (((double_push << 8) >> (16 * to_move)) & interpose);
        while (double_push) {
            to_square = bitscan_reset(&double_push);
            from_square = to_square - 16  + 32 * to_move;
            move_list->move[move_list->count++] = move_directory[from_square][to_square][piece];
        }
        // Pawn promotions
        while (pawn_promotions) {
            to_square = bitscan_reset(&pawn_promotions);
            from_square = to_square - 8  + 16 * to_move;
            for (promote_to = 0; promote_to <= 3; promote_to++) {
                move_list->move[move_list->count++] = move_directory[from_square][to_square][piece] + promote_to;
            }
        }
    }

    //-- Interpose or Capture by a piece other than a pawn
    interpose |= SQUARE64(board->check_attacker);

    //+---------------------------------+
    //| Knight Moves                    |
    //+---------------------------------+
    piece = KNIGHT + piece_color;
    t_bitboard source_piece = board->piecelist[piece] & ~pinned;
    while (source_piece) {
        from_square = bitscan(source_piece);
        moves = (knight_mask[from_square] & ~board->occupied[to_move] & interpose);
        while (moves) {
            to_square = bitscan(moves);
            captured = PIECETYPE(board->square[to_square]);
            move_list->move[move_list->count++] = move_directory[from_square][to_square][piece] + captured;
            moves &= (moves - 1);
        }
        source_piece &= (source_piece - 1);
    }

    //+---------------------------------+
    //| Rook Moves                      |
    //+---------------------------------+
    piece = ROOK + piece_color;
    source_piece = board->piecelist[piece] & ~pinned;
    while (source_piece) {
        from_square = bitscan(source_piece);
        moves = rook_magic_moves[from_square][((rook_magic[from_square].mask & board->all_pieces) * rook_magic[from_square].magic) >> 52];
        moves &= (~board->occupied[to_move] & interpose);
        while (moves) {
            to_square = bitscan(moves);
            captured = PIECETYPE(board->square[to_square]);
            move_list->move[move_list->count++] = move_directory[from_square][to_square][piece] + captured;
            moves &= (moves - 1);
        }
        source_piece &= (source_piece - 1);
    }

    //+---------------------------------+
    //| Bishop Moves                    |
    //+---------------------------------+
    piece = BISHOP + piece_color;
    source_piece = board->piecelist[piece] & ~pinned;
    while (source_piece) {
        from_square = bitscan(source_piece);
        moves = bishop_magic_moves[from_square][((bishop_magic[from_square].mask & board->all_pieces) * bishop_magic[from_square].magic) >> 55];
        moves &= (~board->occupied[to_move] & interpose);
        while (moves) {
            to_square = bitscan(moves);
            captured = PIECETYPE(board->square[to_square]);
            move_list->move[move_list->count++] = move_directory[from_square][to_square][piece] + captured;
            moves &= (moves - 1);
        }
        source_piece &= (source_piece - 1);
    }

    //+---------------------------------+
    //| Queen Moves                     |
    //+---------------------------------+
    piece = QUEEN + piece_color;
    source_piece = board->piecelist[piece] & ~pinned;
    while (source_piece) {
        from_square = bitscan(source_piece);
        moves = rook_magic_moves[from_square][((rook_magic[from_square].mask & board->all_pieces) * rook_magic[from_square].magic) >> 52];
        moves &= (~board->occupied[to_move] & interpose);
        while (moves) {
            to_square = bitscan(moves);
            captured = PIECETYPE(board->square[to_square]);
            move_list->move[move_list->count++] = move_directory[from_square][to_square][piece] + captured;
            moves &= (moves - 1);
        }
        moves = bishop_magic_moves[from_square][((bishop_magic[from_square].mask & board->all_pieces) * bishop_magic[from_square].magic) >> 55];
        moves &= (~board->occupied[to_move] & interpose);
        while (moves) {
            to_square = bitscan(moves);
            captured = PIECETYPE(board->square[to_square]);
            move_list->move[move_list->count++] = move_directory[from_square][to_square][piece] + captured;
            moves &= (moves - 1);
        }
        source_piece &= (source_piece - 1);
    }
    //-- Set the move index to the count
    move_list->imove = move_list->count;
}

void generate_no_capture_no_checks(struct t_board *board, struct t_move_list *move_list) {

    t_bitboard _all_pieces = board->all_pieces;

    struct t_move_record *move;
    t_chess_color to_move = board->to_move;
    t_chess_color opponent = OPPONENT(to_move);
    t_bitboard not_occupied_to_move = ~board->occupied[to_move];

    t_bitboard moves = 0;
    t_bitboard source_piece;
    // Types of pawn move
    t_bitboard double_push;
    t_bitboard pawn_promotions;

    t_chess_piece piece;
    t_chess_piece captured;
    t_chess_piece promote_to;
    t_chess_square from_square, to_square;
    int castle_index;
    int forward;
    int piece_color = (to_move * 8);

    //+---------------------------------+
    //| Find the Pinned Pieces          |
    //+---------------------------------+

    //-- ASSUME ALREADY CALCULATED

    //+------------------------------------------------------+
    //| Calculate check_rays - squares which will give check |
    //+------------------------------------------------------+
    from_square = board->king_square[opponent];
    t_bitboard bishop_check_rays = ~_all_pieces & bishop_magic_moves[from_square][((bishop_magic[from_square].mask & _all_pieces) * bishop_magic[from_square].magic) >> 55];
    t_bitboard rook_check_rays = ~_all_pieces & rook_magic_moves[from_square][((rook_magic[from_square].mask & _all_pieces) * rook_magic[from_square].magic) >> 52];
    t_bitboard queen_check_rays = bishop_check_rays | rook_check_rays;
    t_bitboard knight_check_rays = knight_mask[board->king_square[opponent]] & ~_all_pieces;

    bishop_check_rays = ~bishop_check_rays;
    rook_check_rays = ~rook_check_rays;
    queen_check_rays = ~queen_check_rays;
    knight_check_rays = ~knight_check_rays;

    //+---------------------------------+
    //| Castling Moves                  |
    //+---------------------------------+

    if (board->castling && !board->in_check) {

        //-- Kingside O-O
        castle_index = (to_move * 2);
        if ((board->castling >> castle_index) & (uchar)1) {
            move = &xmove_list[castle_index];
            if (!(_all_pieces & castle[castle_index].possible)) {
                move_list->move[move_list->count++] = &xmove_list[castle_index];
            }
        }
        //-- Queenside O-O-O
        castle_index++;
        if ((board->castling >> castle_index) & (uchar)1) {
            move = &xmove_list[castle_index];
            if (!(_all_pieces & castle[castle_index].possible)) {
                move_list->move[move_list->count++] = &xmove_list[castle_index];
            }
        }
    }

    //+---------------------------------+
    //| Pawn Moves                      |
    //+---------------------------------+
    // find the direction of a pawn push
    forward = 8 - 16 * to_move;
    piece = PAWN + piece_color;
    // All pawn pushes
    moves = (((board->piecelist[piece] << 8) >> (16 * to_move)) & ~(_all_pieces));
    // Pawn priomotions
    pawn_promotions = moves & rank_mask[to_move][EIGHTH_RANK];
    // Take out Pawn Promotions
    moves ^= pawn_promotions;
    // Double moves
    double_push = (moves & rank_mask[to_move][THIRD_RANK]);
    moves &= ~pawn_attackers[to_move][board->king_square[opponent]];
    // Spool the moves to the move list
    while (moves) {
        to_square = bitscan_reset(&moves);
        from_square = to_square - forward;
        move_list->move[move_list->count++] = move_directory[from_square][to_square][piece];
    }
    // Double Moves
    double_push = (((double_push << 8) >> (16 * to_move)) & ~(_all_pieces));
    double_push &= ~pawn_attackers[to_move][board->king_square[opponent]];
    while (double_push) {
        to_square = bitscan_reset(&double_push);
        from_square = to_square - (forward * 2);
        move_list->move[move_list->count++] = move_directory[from_square][to_square][piece];
    }
    // Pawn promotions
    while (pawn_promotions) {
        to_square = bitscan_reset(&pawn_promotions);
        from_square = to_square - forward;
        for (promote_to = 0; promote_to <= 2; promote_to++)
            move_list->move[move_list->count++] = move_directory[from_square][to_square][piece] + promote_to;
    }
    moves = (((board->piecelist[piece] & B8H1) << 7) >> (16 * to_move)) & (board->occupied[opponent] | board->ep_square);
    pawn_promotions = (moves & rank_mask[to_move][EIGHTH_RANK]);
    while (pawn_promotions) {
        to_square = bitscan_reset(&pawn_promotions);
        from_square = to_square - forward + 1;
        captured = PIECETYPE(board->square[to_square]);
        for (promote_to = 0; promote_to <= 2; promote_to++)
            move_list->move[move_list->count++] = move_directory[from_square][to_square][piece] + (4 * captured) + promote_to;
    }
    // Other direction
    moves = (((board->piecelist[piece] & A8G1) << 9) >> (16 * to_move)) & (board->occupied[opponent] | board->ep_square);
    pawn_promotions = (moves & rank_mask[to_move][EIGHTH_RANK]);
    while (pawn_promotions) {
        to_square = bitscan_reset(&pawn_promotions);
        from_square = to_square - forward - 1;
        captured = PIECETYPE(board->square[to_square]);
        for (promote_to = 0; promote_to <= 2; promote_to++)
            move_list->move[move_list->count++] = move_directory[from_square][to_square][piece] + (4 * captured) + promote_to;
    }

    //+---------------------------------+
    //| Knight Moves                    |
    //+---------------------------------+
    piece = KNIGHT + piece_color;
    source_piece = board->piecelist[piece] & ~move_list->pinned_pieces;;
    while (source_piece) {
        from_square = bitscan_reset(&source_piece);
        moves = (knight_mask[from_square] & ~_all_pieces & knight_check_rays);
        while (moves) {
            to_square = bitscan_reset(&moves);
            captured = PIECETYPE(board->square[to_square]);
            move_list->move[move_list->count++] = move_directory[from_square][to_square][piece] + captured;
        }
    }

    //+---------------------------------+
    //| King Moves                      |
    //+---------------------------------+
    piece = KING + piece_color;
    from_square = board->king_square[to_move];
    moves = (king_mask[from_square] & ~_all_pieces);
    while (moves) {
        to_square = bitscan_reset(&moves);
        captured = PIECETYPE(board->square[to_square]);
        move_list->move[move_list->count++] = move_directory[from_square][to_square][piece] + captured;
    }

    //+---------------------------------+
    //| Rook Moves                      |
    //+---------------------------------+
    piece = ROOK + piece_color;
    source_piece = board->piecelist[piece];
    while (source_piece) {
        from_square = bitscan_reset(&source_piece);
        moves = rook_magic_moves[from_square][((rook_magic[from_square].mask & _all_pieces) * rook_magic[from_square].magic) >> 52];
        moves &= ~_all_pieces & rook_check_rays;
        while (moves) {
            to_square = bitscan_reset(&moves);
            captured = PIECETYPE(board->square[to_square]);
            move_list->move[move_list->count++] = move_directory[from_square][to_square][piece] + captured;
        }
    }

    //+---------------------------------+
    //| Bishop Moves                    |
    //+---------------------------------+
    piece = BISHOP + piece_color;
    source_piece = board->piecelist[piece];
    while (source_piece) {
        from_square = bitscan_reset(&source_piece);
        moves = bishop_magic_moves[from_square][((bishop_magic[from_square].mask & _all_pieces) * bishop_magic[from_square].magic) >> 55];
        moves &= ~_all_pieces & bishop_check_rays;
        while (moves) {
            to_square = bitscan_reset(&moves);
            captured = PIECETYPE(board->square[to_square]);
            move_list->move[move_list->count++] = move_directory[from_square][to_square][piece] + captured;
        }
    }

    //+---------------------------------+
    //| Queen Moves                     |
    //+---------------------------------+
    piece = QUEEN + piece_color;
    source_piece = board->piecelist[piece];
    while (source_piece) {
        from_square = bitscan_reset(&source_piece);
        moves = rook_magic_moves[from_square][((rook_magic[from_square].mask & _all_pieces) * rook_magic[from_square].magic) >> 52];
        moves &= ~_all_pieces & queen_check_rays;
        while (moves) {
            to_square = bitscan_reset(&moves);
            captured = PIECETYPE(board->square[to_square]);
            move_list->move[move_list->count++] = move_directory[from_square][to_square][piece] + captured;
        }
        moves = bishop_magic_moves[from_square][((bishop_magic[from_square].mask & _all_pieces) * bishop_magic[from_square].magic) >> 55];
        moves &= ~_all_pieces & queen_check_rays;
        while (moves) {
            to_square = bitscan_reset(&moves);
            captured = PIECETYPE(board->square[to_square]);
            move_list->move[move_list->count++] = move_directory[from_square][to_square][piece] + captured;
        }
    }
    //-- Set the move index to the count
    move_list->imove = move_list->count;
}

void generate_quiet_moves(struct t_board *board, struct t_move_list *move_list) {

    t_bitboard _all_pieces = board->all_pieces;

    struct t_move_record *move;
    t_chess_color to_move = board->to_move;
    t_chess_color opponent = OPPONENT(to_move);
    t_bitboard not_occupied_to_move = ~board->occupied[to_move];

    t_bitboard moves = 0;
    t_bitboard source_piece;
    // Types of pawn move
    t_bitboard double_push;
    t_bitboard pawn_promotions;

    t_chess_piece piece;
    t_chess_piece captured;
    t_chess_piece promote_to;
    t_chess_square from_square, to_square;
    int castle_index;
    int forward;
    int piece_color = (to_move * 8);

    //+---------------------------------+
    //| Find the Pinned Pieces          |
    //+---------------------------------+

    //-- ASSUME ALREADY CALCULATED

    //+---------------------------------+
    //| Castling Moves                  |
    //+---------------------------------+

    if (board->castling && !board->in_check) {

        //-- Kingside O-O
        castle_index = (to_move * 2);
        if ((board->castling >> castle_index) & (uchar)1) {
            move = &xmove_list[castle_index];
            if (!(_all_pieces & castle[castle_index].possible)) {
                move_list->move[move_list->count++] = &xmove_list[castle_index];
            }
        }
        //-- Queenside O-O-O
        castle_index++;
        if ((board->castling >> castle_index) & (uchar)1) {
            move = &xmove_list[castle_index];
            if (!(_all_pieces & castle[castle_index].possible)) {
                move_list->move[move_list->count++] = &xmove_list[castle_index];
            }
        }
    }

    //+---------------------------------+
    //| Pawn Moves                      |
    //+---------------------------------+
    // find the direction of a pawn push
    forward = 8 - 16 * to_move;
    piece = PAWN + piece_color;
    // All pawn pushes
    moves = (((board->piecelist[piece] << 8) >> (16 * to_move)) & ~(_all_pieces));
    // Pawn priomotions
    pawn_promotions = moves & rank_mask[to_move][EIGHTH_RANK];
    // Take out Pawn Promotions
    moves ^= pawn_promotions;
    // Double moves
    double_push = (moves & rank_mask[to_move][THIRD_RANK]);
    // Spool the moves to the move list
    while (moves) {
        to_square = bitscan_reset(&moves);
        from_square = to_square - forward;
        move_list->move[move_list->count++] = move_directory[from_square][to_square][piece];
    }
    // Double Moves
    double_push = (((double_push << 8) >> (16 * to_move)) & ~(_all_pieces));
    while (double_push) {
        to_square = bitscan_reset(&double_push);
        from_square = to_square - (forward * 2);
        move_list->move[move_list->count++] = move_directory[from_square][to_square][piece];
    }
    // Pawn promotions
    while (pawn_promotions) {
        to_square = bitscan_reset(&pawn_promotions);
        from_square = to_square - forward;
        for (promote_to = 0; promote_to <= 2; promote_to++)
            move_list->move[move_list->count++] = move_directory[from_square][to_square][piece] + promote_to;
    }
    moves = (((board->piecelist[piece] & B8H1) << 7) >> (16 * to_move)) & (board->occupied[opponent] | board->ep_square);
    pawn_promotions = (moves & rank_mask[to_move][EIGHTH_RANK]);
    while (pawn_promotions) {
        to_square = bitscan_reset(&pawn_promotions);
        from_square = to_square - forward + 1;
        captured = PIECETYPE(board->square[to_square]);
        for (promote_to = 0; promote_to <= 2; promote_to++)
            move_list->move[move_list->count++] = move_directory[from_square][to_square][piece] + (4 * captured) + promote_to;
    }
    // Other direction
    moves = (((board->piecelist[piece] & A8G1) << 9) >> (16 * to_move)) & (board->occupied[opponent] | board->ep_square);
    pawn_promotions = (moves & rank_mask[to_move][EIGHTH_RANK]);
    while (pawn_promotions) {
        to_square = bitscan_reset(&pawn_promotions);
        from_square = to_square - forward - 1;
        captured = PIECETYPE(board->square[to_square]);
        for (promote_to = 0; promote_to <= 2; promote_to++)
            move_list->move[move_list->count++] = move_directory[from_square][to_square][piece] + (4 * captured) + promote_to;
    }

    //+---------------------------------+
    //| Knight Moves                    |
    //+---------------------------------+
    piece = KNIGHT + piece_color;
    source_piece = board->piecelist[piece] & ~move_list->pinned_pieces;;
    while (source_piece) {
        from_square = bitscan_reset(&source_piece);
        moves = (knight_mask[from_square] & ~_all_pieces);
        while (moves) {
            to_square = bitscan_reset(&moves);
            captured = PIECETYPE(board->square[to_square]);
            move_list->move[move_list->count++] = move_directory[from_square][to_square][piece] + captured;
        }
    }

    //+---------------------------------+
    //| King Moves                      |
    //+---------------------------------+
    piece = KING + piece_color;
    from_square = board->king_square[to_move];
    moves = (king_mask[from_square] & ~_all_pieces);
    while (moves) {
        to_square = bitscan_reset(&moves);
        captured = PIECETYPE(board->square[to_square]);
        move_list->move[move_list->count++] = move_directory[from_square][to_square][piece] + captured;
    }

    //+---------------------------------+
    //| Rook Moves                      |
    //+---------------------------------+
    piece = ROOK + piece_color;
    source_piece = board->piecelist[piece];
    while (source_piece) {
        from_square = bitscan_reset(&source_piece);
        moves = rook_magic_moves[from_square][((rook_magic[from_square].mask & _all_pieces) * rook_magic[from_square].magic) >> 52];
        moves &= ~_all_pieces;
        while (moves) {
            to_square = bitscan_reset(&moves);
            captured = PIECETYPE(board->square[to_square]);
            move_list->move[move_list->count++] = move_directory[from_square][to_square][piece] + captured;
        }
    }

    //+---------------------------------+
    //| Bishop Moves                    |
    //+---------------------------------+
    piece = BISHOP + piece_color;
    source_piece = board->piecelist[piece];
    while (source_piece) {
        from_square = bitscan_reset(&source_piece);
        moves = bishop_magic_moves[from_square][((bishop_magic[from_square].mask & _all_pieces) * bishop_magic[from_square].magic) >> 55];
        moves &= ~_all_pieces;
        while (moves) {
            to_square = bitscan_reset(&moves);
            captured = PIECETYPE(board->square[to_square]);
            move_list->move[move_list->count++] = move_directory[from_square][to_square][piece] + captured;
        }
    }

    //+---------------------------------+
    //| Queen Moves                     |
    //+---------------------------------+
    piece = QUEEN + piece_color;
    source_piece = board->piecelist[piece];
    while (source_piece) {
        from_square = bitscan_reset(&source_piece);
        moves = rook_magic_moves[from_square][((rook_magic[from_square].mask & _all_pieces) * rook_magic[from_square].magic) >> 52];
        moves &= ~_all_pieces;
        while (moves) {
            to_square = bitscan_reset(&moves);
            captured = PIECETYPE(board->square[to_square]);
            move_list->move[move_list->count++] = move_directory[from_square][to_square][piece] + captured;
        }
        moves = bishop_magic_moves[from_square][((bishop_magic[from_square].mask & _all_pieces) * bishop_magic[from_square].magic) >> 55];
        moves &= ~_all_pieces;
        while (moves) {
            to_square = bitscan_reset(&moves);
            captured = PIECETYPE(board->square[to_square]);
            move_list->move[move_list->count++] = move_directory[from_square][to_square][piece] + captured;
        }
    }
    //-- Set the move index to the count
    move_list->imove = move_list->count;
}

BOOL move_list_integrity(struct t_board *board, struct t_move_list *move_list) {
    int i;
    struct t_move_record *move;
    struct t_move_list moves[1];
    for (i = 0; i < move_list->count; i++) {
        move = move_list->move[i];
        if (board->square[move->from_square] != move->piece) {
            write_board(board, "board.txt");
            generate_moves(board, moves);
            write_move_list(moves, "move-list.txt");
            return FALSE;
        }
        if (((board->square[move->to_square] != BLANK) && (board->square[move->to_square] != move->captured)) && !(board->chess960 && move->move_type == MOVE_CASTLE))
            return FALSE;
    }
    return TRUE;
}

