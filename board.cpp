//===========================================================//
//
// Maverick Chess Engine
// Copyright 2013-2015 Steve Maughan
//
//===========================================================//

#include <stdlib.h>
#include <stdio.h>

#if defined(_WIN32)
#include <windows.h>
#include <process.h>
#include <conio.h>
#else
#include <unistd.h>
#endif

#include <string.h>
#include <math.h>
#include <assert.h>

#include "defs.h"
#include "data.h"
#include "procs.h"
#include "bittwiddle.h"


void update_in_check(struct t_board *board, t_chess_square from_square, t_chess_square to_square, t_chess_color color) {

    t_chess_color opponent = OPPONENT(color);
    t_chess_square king_square = board->king_square[color];
    t_chess_square potential_attacker;
    t_chess_piece piece = board->square[to_square];
    t_bitboard b;

    board->in_check = 0;

    // See of to_square holds a suitable attacker
    if (can_move[to_square][king_square] & PIECEMASK(piece)) {
        switch (PIECETYPE(piece))
        {
        case KNIGHT:
            board->in_check = 1;
            board->check_attacker = to_square;
            break;
        case BISHOP:
            if ((between[to_square][king_square] & board->all_pieces) == 0) {
                board->in_check = 1;
                board->check_attacker = to_square;
            }
            break;
        case ROOK:
            if ((between[to_square][king_square] & board->all_pieces) == 0) {
                board->in_check = 1;
                board->check_attacker = to_square;
            }
            break;
        case QUEEN:
            if ((between[to_square][king_square] & board->all_pieces) == 0) {
                board->in_check = 1;
                board->check_attacker = to_square;
            }
            break;
        case PAWN:
            board->in_check = 1;
            board->check_attacker = to_square;
            break;
        case KING:
            // never delivers check!!
            break;
        }
    }
    // Discovered Check
    switch (can_move[from_square][king_square] & (PIECEMASK(BISHOP) | PIECEMASK(ROOK))) {
    case PIECEMASK(BISHOP):
        b = (xray[king_square][from_square] & (board->pieces[opponent][BISHOP] | board->pieces[opponent][QUEEN]) & ~SQUARE64(to_square));
        while (b) {
            potential_attacker = bitscan(b);
            if ((between[king_square][potential_attacker] & board->all_pieces) == 0) {
                board->in_check++;
                board->check_attacker = potential_attacker;
                return;
            }
            b &= (b - 1);
        }
        break;
    case PIECEMASK(ROOK):
        b = (xray[king_square][from_square] & (board->pieces[opponent][ROOK] | board->pieces[opponent][QUEEN]) & ~SQUARE64(to_square));
        while (b) {
            potential_attacker = bitscan(b);
            if ((between[king_square][potential_attacker] & board->all_pieces) == 0) {
                board->in_check++;
                board->check_attacker = potential_attacker;
                return;
            }
            b &= (b - 1);
        }
        break;
    }
}

BOOL is_pinned(struct t_board *board, t_chess_square square, t_chess_square pinned_to) {

    t_bitboard b = xray[pinned_to][square];

    if (b) {
        t_chess_color to_move = COLOR(board->square[square]);
        t_chess_color opponent = OPPONENT(to_move);
        t_bitboard attacker;
        t_chess_square s;

        board->all_pieces ^= SQUARE64(square);
        //-- Bishop
        attacker = (b & bishop_rays[pinned_to]);
        if (attacker) {
            attacker &= (board->pieces[opponent][BISHOP] | board->pieces[opponent][QUEEN]);
            while (attacker) {
                s = bitscan(attacker);
                if ((between[pinned_to][s] & board->all_pieces) == 0) {
                    board->all_pieces ^= SQUARE64(square);
                    return TRUE;
                }
                attacker &= (attacker - 1);
            }
        }
        //-- Rooks
        attacker = (b & rook_rays[pinned_to]);
        if (attacker) {
            attacker &= (board->pieces[opponent][ROOK] | board->pieces[opponent][QUEEN]);
            while (attacker) {
                s = bitscan(attacker);
                if ((between[pinned_to][s] & board->all_pieces) == 0) {
                    board->all_pieces ^= SQUARE64(square);
                    return TRUE;
                }
                attacker &= (attacker - 1);
            }
        }
        board->all_pieces ^= SQUARE64(square);
    }
    return FALSE;
}

BOOL is_in_check(struct t_board *board, t_chess_color color) {
    return is_square_attacked(board, board->king_square[color], OPPONENT(color));
}

BOOL is_square_attacked(struct t_board *board, t_chess_square square, t_chess_color color) {

    // Pawns
    if (pawn_attackers[color][square] & board->pieces[color][PAWN])
        return TRUE;
    // Knight
    if (knight_mask[square] & board->pieces[color][KNIGHT])
        return TRUE;
    // King
    if (king_mask[square] & board->pieces[color][KING])
        return TRUE;

    t_bitboard b;
    t_chess_square s;
    // Bishop & Queen Diagonals
    b = bishop_rays[square] & (board->pieces[color][BISHOP] | board->pieces[color][QUEEN]);
    while (b) {
        s = bitscan(b);
        if ((between[square][s] & board->all_pieces) == 0)
            return TRUE;
        b &= (b - 1);
    }
    // Rooks & Queen Horizonatal & Vertical
    b = rook_rays[square] & (board->pieces[color][ROOK] | board->pieces[color][QUEEN]);
    while (b) {
        s = bitscan(b);
        if ((between[square][s] & board->all_pieces) == 0)
            return TRUE;
        b &= (b - 1);
    }
    // No Attacks!
    return FALSE;
}

int attack_count(struct t_board *board, t_chess_square square, t_chess_color color) {
    int count;
    t_bitboard b;
    t_chess_square s;

    // Pawns
    count = popcount(pawn_attackers[color][square] & board->pieces[color][PAWN]);
    // Knight
    count += popcount(knight_mask[square] & board->pieces[color][KNIGHT]);
    // King
    count += popcount(king_mask[square] & board->pieces[color][KING]);
    // Bishop & Queen Diagonals
    b = bishop_rays[square] & (board->pieces[color][BISHOP] | board->pieces[color][QUEEN]);
    while (b) {
        s = bitscan(b);
        if ((between[square][s] & board->all_pieces) == 0)
            count++;
        b &= (b - 1);
    }
    // Rooks & Queen Horizonatal & Vertical
    b = rook_rays[square] & (board->pieces[color][ROOK] | board->pieces[color][QUEEN]);
    while (b) {
        s = bitscan(b);
        if ((between[square][s] & board->all_pieces) == 0)
            count++;
        b &= (b - 1);
    }
    // No Attacks!
    return count;
}

t_chess_square who_is_attacking_square(struct t_board *board, t_chess_square square, t_chess_color color) {

    t_bitboard b;
    // Pawns
    b = pawn_attackers[color][square] & board->pieces[color][PAWN];
    if (b)
        return bitscan(b);
    // Knight
    b = knight_mask[square] & board->pieces[color][KNIGHT];
    if (b)
        return bitscan(b);
    // King
    b = king_mask[square] & board->pieces[color][KING];
    if (b)
        return bitscan(b);

    t_chess_square s;
    // Bishop & Queen Diagonals
    b = bishop_rays[square] & (board->pieces[color][BISHOP] | board->pieces[color][QUEEN]);
    while (b) {
        s = bitscan(b);
        if ((between[square][s] & board->all_pieces) == 0)
            return s;
        b &= (b - 1);
    }
    // Rooks & Queen Horizonatal & Vertical
    b = rook_rays[square] & (board->pieces[color][ROOK] | board->pieces[color][QUEEN]);
    while (b) {
        s = bitscan(b);
        if ((between[square][s] & board->all_pieces) == 0)
            return s;
        b &= (b - 1);
    }
    // No Attacks!
    return -1;
}

// Initialize the board variable
void init_board(struct t_board *board)
{
    int i;

    board->pieces[WHITE] = board->piecelist;
    board->pieces[BLACK] = board->piecelist + 8;

    for (i = 0; i <= MAXPLY; i++) {
        if (i == 0)
            board->pv_data[i].previous_pv = NULL;
        else
            board->pv_data[i].previous_pv = &board->pv_data[i - 1];
        if (i < MAXPLY)
            board->pv_data[i].next_pv = &board->pv_data[i + 1];
        else
            board->pv_data[i].next_pv = &board->pv_data[i + 1];
        board->pv_data[i].best_line_length = 0;
        board->pv_data[i].killer1 = NULL;
        board->pv_data[i].killer2 = NULL;
        board->pv_data[i].check_killer1 = NULL;
        board->pv_data[i].check_killer2 = NULL;
        board->pv_data[i].in_check = FALSE;
        init_eval(board->pv_data[i].eval);
    }
}

// Add a piece to the board!
void add_piece(struct t_board *board, t_chess_piece piece, t_chess_square target_square)
{
    t_chess_color color = COLOR(piece);
    t_bitboard square64 = SQUARE64(target_square);

    assert((board->piecelist[piece] & square64) == 0);
    board->piecelist[piece] |= square64;
    assert((board->occupied[color] & square64) == 0);
    board->occupied[color] |= square64;
    assert((board->all_pieces & square64) == 0);
    board->all_pieces |= square64;

    board->square[target_square] = piece;

    board->hash ^= hash_value[piece][target_square];

    switch (PIECETYPE(piece)) {
    case PAWN:
        board->pawn_hash ^= hash_value[piece][target_square];
        board->material_hash ^= material_hash_values[piece][popcount(board->piecelist[piece])];
        break;
    case KING:
        board->king_square[color] = target_square;
        board->pawn_hash ^= hash_value[piece][target_square];
        break;
    default:
        board->material_hash ^= material_hash_values[piece][popcount(board->piecelist[piece])];
    }
}

// Remove a piece from the board
void remove_piece(struct t_board *board, t_chess_square target_square)
{
    t_bitboard square64 = SQUARE64(target_square);
    t_chess_piece piece = board->square[target_square];
    t_chess_color color = COLOR(piece);

    board->piecelist[piece] ^= square64;;
    board->occupied[color] ^= square64;
    board->all_pieces ^= square64;

    board->square[target_square] = BLANK;
    board->hash ^= hash_value[piece][target_square];

    switch (PIECETYPE(piece)) {
    case PAWN:
        board->pawn_hash ^= hash_value[piece][target_square];
        board->material_hash ^= material_hash_values[piece][popcount(board->piecelist[piece]) + 1];
        break;
    case KING:
        board->king_square[color] = -1;
        board->hash ^= hash_value[piece][target_square];
        break;
    default:
        board->material_hash ^= material_hash_values[piece][popcount(board->piecelist[piece]) + 1];
    }

}

// Clear board
void clear_board(struct t_board *board)
{
    int i;
    int c;
    for (c = WHITE; c <= BLACK; c++) {
        board->king_square[c] = 0;
        board->occupied[c] = 0;
        for (i = KNIGHT; i <= KING; i++) {
            board->pieces[c][i] = 0;
        }
    }
    for (i = 0; i < 64; i++)
        board->square[i] = 0;

    board->all_pieces = 0;
    board->castling = 0;
    board->check_attacker = 0;
    board->ep_square = 0;
    board->fifty_move_count = 0;
    board->hash = 0;
    board->pawn_hash = 0;
    board->to_move = WHITE;
}

void new_game(struct t_board *board)
{
    set_fen(board, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -");
}

t_chess_square name_to_index(char *name)
{
    t_chess_square index1, index2;

    index1 = name[1] - '1';
    index2 = name[0] - 'a';
    return (8 * index1 + index2);
}

char *move_as_str(struct t_move_record *move)
{
    static char s[10];

    t_chess_square from_square, to_square;
    t_chess_piece promote;

    if (move == NULL) {
        s[0] = '0';
        s[1] = '0';
        s[2] = '0';
        s[3] = '0';
        s[4] = 0;
        return s;
    }

    from_square = move->from_square;
    to_square = move->to_square;

    //-- Handle Chess960
    if (move->move_type == MOVE_CASTLE && uci.options.chess960) {
        to_square = castle[move->index].rook_from;
    }

    if (from_square == to_square) {
        s[0] = '0';
        s[1] = '0';
        s[2] = '0';
        s[3] = '0';
        s[4] = 0;
        return s;
    }

    promote = move->promote_to;

    s[0] = 'a' + (from_square & 7);
    s[1] = '1' + (from_square >> 3);
    s[2] = 'a' + (to_square & 7);
    s[3] = '1' + (to_square >> 3);

    switch (promote) {
    case BLANK :
        s[4] = 0;
        break;
    case WHITEPAWN :
        s[4]='p';
        break;
    case WHITEKNIGHT :
        s[4]='n';
        break;
    case WHITEBISHOP :
        s[4]='b';
        break;
    case WHITEROOK :
        s[4]='r';
        break;
    case WHITEQUEEN :
        s[4]='q';
        break;
    case WHITEKING :
        s[4]='k';
        break;
    case BLACKPAWN :
        s[4]='p';
        break;
    case BLACKKNIGHT :
        s[4]='n';
        break;
    case BLACKBISHOP :
        s[4]='b';
        break;
    case BLACKROOK :
        s[4]='r';
        break;
    case BLACKQUEEN :
        s[4]='q';
        break;
    case BLACKKING :
        s[4]='k';
        break;
    }
    s[5] = 0;

    return s;
}

t_chess_square kingside_rook(struct t_board *board, t_chess_color color) {

    t_chess_square s = H1 + 56 * color;

    while ((board->square[s] != PIECEINDEX(color, ROOK)) && (board->square[s] != PIECEINDEX(color, KING)) && (s != A1 + 56 * color))
    {
        s--;
    }

    if (board->square[s] == PIECEINDEX(color, ROOK))
        return s;
    else
        return -1;
}

t_chess_square queenside_rook(struct t_board *board, t_chess_color color) {

    t_chess_square s = A1 + 56 * color;

    while ((board->square[s] != PIECEINDEX(color, ROOK)) && (board->square[s] != PIECEINDEX(color, KING)) && (s != H1 + 56 * color))
    {
        s++;
    }

    if (board->square[s] == PIECEINDEX(color, ROOK))
        return s;
    else
        return -1;
}

BOOL integrity(struct t_board *board) {
    int i, p_count, b_count;
    t_chess_color c;
    t_bitboard b;

    //-- Link between squares and bitboards
    for (i = 0; i < 64; i++) {
        if (board->square[i] != BLANK)
            if ((board->piecelist[board->square[i]] & SQUARE64(i)) == 0)
                return FALSE;
    }

    //-- Color bitboards match up
    for (c = WHITE; c <= BLACK; c++) {
        b = 0;
        p_count = 0;
        for (i = KNIGHT; i <= KING; i++) {
            b |= board->pieces[c][i];
            p_count += (piece_count_value[i] * popcount(board->pieces[c][i]));
        }

        if (b != board->occupied[c])
            return FALSE;
        if (board->square[board->king_square[c]] != PIECEINDEX(c, KING))
            return FALSE;
        if (SQUARE64(board->king_square[c]) != board->pieces[c][KING])
            return FALSE;
        if (popcount(board->pieces[c][KING]) != 1)
            return FALSE;
    }

    //--  All pieces match up with occupancy
    if ((board->occupied[WHITE] | board->occupied[BLACK]) != board->all_pieces)
        return FALSE;

    //-- Castling rights
    for (i = 0; i < 4; i++) {
        if (board->castling & ((uchar)1 << i)) {
            if (board->square[castle[i].king_from] != PIECEINDEX((i >> 1), KING))
                return FALSE;
            if (board->square[castle[i].rook_from] != PIECEINDEX((i >> 1), ROOK))
                return FALSE;
        }
    }

    //-- Hashing
    if (board->pawn_hash != calc_pawn_hash(board))
        return FALSE;
    if (board->hash != calc_board_hash(board))
        return FALSE;
    if (board->material_hash != calc_material_hash(board))
        return FALSE;

    //-- In check and shouldn't be!
    if (attack_count(board, board->king_square[OPPONENT(board->to_move)], board->to_move) != 0)
        return FALSE;

    //-- In Check Count
    if (board->in_check != attack_count(board, board->king_square[board->to_move], OPPONENT(board->to_move))) {
        return FALSE;
    }

    return TRUE;
}

//-- Flip board
void flip_board(struct t_board *board) {
    t_chess_piece piece, swap_piece;
    t_chess_color to_move = board->to_move;
    t_chess_color opponent = OPPONENT(to_move);
    t_chess_color color;
    t_bitboard t;
    int i;

    //-- Flip Pieces
    for (piece = KNIGHT; piece <= KING; piece++) {
        t = board->pieces[WHITE][piece];
        board->pieces[WHITE][piece] = 0;
        for (i = A1; i <= H8; i++) {
            if (is_bit_set(board->pieces[BLACK][piece], i))
                board->pieces[WHITE][piece] |= SQUARE64(FLIP64(i));
        }
        board->pieces[BLACK][piece] = 0;
        for (i = A1; i <= H8; i++) {
            if (is_bit_set(t, i))
                board->pieces[BLACK][piece] |= SQUARE64(FLIP64(i));
        }
    }

    //-- Swap in check attacker
    if (board->in_check)
        board->check_attacker = FLIP64(board->check_attacker);
    else
        board->check_attacker = 0;

    //-- Bitboard piece swaps
    board->all_pieces = 0;
    for (color = WHITE; color <= BLACK; color++) {
        board->king_square[color] = bitscan(board->pieces[color][KING]);
        board->occupied[color] = 0;
        for (piece = KNIGHT; piece <= KING; piece ++) {
            board->occupied[color] |= board->pieces[color][piece];
        }
        board->all_pieces |= board->occupied[color];
    }

    //-- Squares
    for (i = A1; i <= H4; i++) {
        piece = board->square[i];
        swap_piece = board->square[FLIP64(i)];
        if (piece)
            board->square[FLIP64(i)] = FLIPPIECECOLOR(piece);
        else
            board->square[FLIP64(i)] = BLANK;
        if (swap_piece)
            board->square[i] = FLIPPIECECOLOR(swap_piece);
        else
            board->square[i] = BLANK;
    }

    //-- Castling rights
    board->castling = (((board->castling & 3) << 2) | (board->castling >> 2));
    if (board->chess960) {
        for (i = 0; i < 4; i++) {
            color = (i / 2);
            if (board->castling & (uchar(1) << i)) {
                if (i & uchar(1))
                    init_960_castling(board, board->king_square[color], queenside_rook(board, color));
                else
                    init_960_castling(board, board->king_square[color], kingside_rook(board, color));
            }
        }
    }

    board->to_move = opponent;

    board->hash = calc_board_hash(board);
    board->pawn_hash = calc_pawn_hash(board);
    board->material_hash = calc_material_hash(board);

    assert(integrity(board));
}

void init_can_move() {
    t_chess_piece piece;
    t_chess_square from;
    t_chess_square to;
    int i, j;

    for (i = A1; i <= H8; i++)
        for (j = A1; j <= H8; j++)
            can_move[i][j] = 0;

    for (i = 4; i < GLOBAL_MOVE_COUNT; i++) {
        from = xmove_list[i].from_square;
        to = xmove_list[i].to_square;
        piece = xmove_list[i].piece;
        if (PIECETYPE(piece) != PAWN || xmove_list[i].captured != BLANK)
            can_move[from][to] |= PIECEMASK(piece);
    }
}

void init_perft_pv_data() {
    int i;
    for (i = 0; i < 100; i++) {
        perft_pv_data[i].index = i;
        strcpy(perft_pv_data[i].fen,"");
        perft_pv_data[i].move = NULL;
    }
}
