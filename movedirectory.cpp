//===========================================================//
//
// Maverick Chess Engine
// Copyright 2013-2014 Steve Maughan
//
//===========================================================//

#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <process.h>
#include <conio.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include "defs.h"
#include "data.h"
#include "procs.h"
#include "bittwiddle.h"

t_move_record *lookup_move(struct t_board *board, char *move_string) {

    t_chess_square from_square = name_to_index(move_string);
    t_chess_square to_square = name_to_index(move_string + 2);
    t_chess_piece piece = board->square[from_square];
    t_chess_piece captured = PIECETYPE(board->square[to_square]);
    t_chess_color color = COLOR(piece);
    t_chess_piece promote_to = 0;

    if (PAWN == PIECETYPE(piece) && ((color == WHITE && RANK(to_square) == 7) || (color == BLACK && RANK(to_square) == 0))) {
        if (move_string[4] == 'q')
            promote_to = QUEEN;
        else if (move_string[4] == 'b')
            promote_to = BISHOP;
        else if (move_string[4] == 'r')
            promote_to = ROOK;
        else if (move_string[4] == 'n')
            promote_to = KNIGHT;

        return move_directory[from_square][to_square][piece] + (4 * captured) + promote_to - 1;

    }
    else
        return move_directory[from_square][to_square][piece] + captured;
}

void configure_castling(int *i)
{
    struct t_move_record *move = &xmove_list[*i];

    // White Kingside
    move->captured = BLANK;
    move->from_square = E1;
    move->to_square = G1;
    move->move_type = MOVE_CASTLE;
    move->piece = WHITEKING;
    move->promote_to = BLANK;
    move->castling_delta = (BLACK_CASTLE_OO | BLACK_CASTLE_OOO);
    move_directory[move->from_square][move->to_square][move->piece] = move;
    move_directory[move->from_square][H1][move->piece] = move;
    (*i)++;
    move++;

    // White Queenside
    move->captured = BLANK;
    move->from_square = E1;
    move->to_square = C1;
    move->move_type = MOVE_CASTLE;
    move->piece = WHITEKING;
    move->promote_to = BLANK;
    move->castling_delta = (BLACK_CASTLE_OO | BLACK_CASTLE_OOO);
    move_directory[move->from_square][move->to_square][move->piece] = move;
    move_directory[move->from_square][A1][move->piece] = move;
    (*i)++;
    move++;

    // Black Kingside
    move->captured = BLANK;
    move->from_square = E8;
    move->to_square = G8;
    move->move_type = MOVE_CASTLE;
    move->piece = BLACKKING;
    move->promote_to = BLANK;
    move->castling_delta = (WHITE_CASTLE_OO | WHITE_CASTLE_OOO);
    move_directory[move->from_square][move->to_square][move->piece] = move;
    move_directory[move->from_square][H8][move->piece] = move;
    (*i)++;
    move++;

    // Black Queenside
    move->captured = BLANK;
    move->from_square = E8;
    move->to_square = C8;
    move->move_type = MOVE_CASTLE;
    move->piece = BLACKKING;
    move->promote_to = BLANK;
    move->castling_delta = (WHITE_CASTLE_OO | WHITE_CASTLE_OOO);
    move_directory[move->from_square][move->to_square][move->piece] = move;
    move_directory[move->from_square][A8][move->piece] = move;
    (*i)++;
    move++;

}

void configure_pawn_push(int *i)
{
    int s;
    t_chess_piece promote;
    struct t_move_record *move = &xmove_list[*i];

    // White double pawn push
    for (s = A2; s <= H2; s++) {
        move->captured = BLANK;
        move->from_square = s;
        move->to_square = s + 16;
        move->move_type = MOVE_PAWN_PUSH2;
        move->piece = WHITEPAWN;
        move->promote_to = BLANK;
        move_directory[move->from_square][move->to_square][move->piece] = move;
        (*i)++;
        move++;
    }

    // White non-promoting pawn push
    for (s = A2; s <= H6; s++) {
        move->captured = BLANK;
        move->from_square = s;
        move->to_square = s + 8;
        move->move_type = MOVE_PAWN_PUSH1;
        move->piece = WHITEPAWN;
        move->promote_to = BLANK;
        move_directory[move->from_square][move->to_square][move->piece] = move;
        (*i)++;
        move++;
    }

    // White promotions
    for (s = A7; s <= H7; s++) {
        move_directory[s][s + 8][WHITEPAWN] = move;
        for (promote = WHITEKNIGHT; promote <= WHITEQUEEN; promote++) {
            move->captured = BLANK;
            move->from_square = s;
            move->to_square = s + 8;
            move->move_type = MOVE_PROMOTION;
            move->piece = WHITEPAWN;
            move->promote_to = promote;
            (*i)++;
            move++;
        }
    }

    // Black non-promoting pawn push
    for (s = A7; s <= H7; s++) {
        move->captured = BLANK;
        move->from_square = s;
        move->to_square = s - 16;
        move->move_type = MOVE_PAWN_PUSH2;
        move->piece = BLACKPAWN;
        move->promote_to = BLANK;
        move_directory[move->from_square][move->to_square][move->piece] = move;
        (*i)++;
        move++;
    }

    // Black non-promoting pawn push
    for (s = A3; s <= H7; s++) {
        move->captured = BLANK;
        move->from_square = s;
        move->to_square = s - 8;
        move->move_type = MOVE_PAWN_PUSH1;
        move->piece = BLACKPAWN;
        move->promote_to = BLANK;
        move_directory[move->from_square][move->to_square][move->piece] = move;
        (*i)++;
        move++;
    }

    // Black pawn promotions
    for (s = A2; s <= H2; s++) {
        move_directory[s][s - 8][BLACKPAWN] = move;
        for (promote = BLACKKNIGHT; promote <= BLACKQUEEN; promote++) {
            move->captured = BLANK;
            move->from_square = s;
            move->to_square = s - 8;
            move->move_type = MOVE_PROMOTION;
            move->piece = BLACKPAWN;
            move->promote_to = promote;
            (*i)++;
            move++;
        }
    }

}

void configure_pawn_capture(int *i)
{
    int s;
    t_chess_piece promote, capture;
    struct t_move_record *move = &xmove_list[*i];

    // White non-promoting captures
    for (s = A2; s <= H6; s++) {

        // North East Capture
        if (COLUMN(s) < 7) {
            // ep capture
            if (RANK(s) == 4) {
                move->captured = BLACKPAWN;
                move->from_square = s;
                move->to_square = s + 9;
                move->move_type = MOVE_PxP_EP;
                move->piece = WHITEPAWN;
                move->promote_to = BLANK;
                move_directory[move->from_square][move->to_square][move->piece] = move;
                (*i)++;
                move++;
            }
            for (capture = BLACKKNIGHT; capture <=BLACKPAWN; capture++) {
                move->captured = capture;
                move->from_square = s;
                move->to_square = s + 9;
                if (PIECETYPE(capture) == PAWN) {
                    move->move_type = MOVE_PxPAWN;
                }
                else {
                    move->move_type = MOVE_PxPIECE;
                }
                move->piece = WHITEPAWN;
                move->promote_to = BLANK;
                move_directory[move->from_square][move->to_square][move->piece] = move - PIECETYPE(capture);
                (*i)++;
                move++;
            }

        }

        // North West Capture
        if (COLUMN(s) > 0) {
            // ep capture
            if (RANK(s) == 4) {
                move->captured = BLACKPAWN;
                move->from_square = s;
                move->to_square = s + 7;
                move->move_type = MOVE_PxP_EP;
                move->piece = WHITEPAWN;
                move->promote_to = BLANK;
                move_directory[move->from_square][move->to_square][move->piece] = move;
                (*i)++;
                move++;
            }
            for (capture = BLACKKNIGHT; capture <=BLACKPAWN; capture++) {
                move->captured = capture;
                move->from_square = s;
                move->to_square = s + 7;
                if (PIECETYPE(capture) == PAWN) {
                    move->move_type = MOVE_PxPAWN;
                }
                else {
                    move->move_type = MOVE_PxPIECE;
                }
                move->piece = WHITEPAWN;
                move->promote_to = BLANK;
                move_directory[move->from_square][move->to_square][move->piece] = move - PIECETYPE(capture);
                (*i)++;
                move++;
            }
        }
    }

    // White Capture Promotes
    for (s = A7; s <= H7; s++) {
        if (COLUMN(s) < 7) {
            for (capture = BLACKKNIGHT; capture <= BLACKQUEEN; capture++) {
                move_directory[s][s + 9][WHITEPAWN] = move - (4 * PIECETYPE(capture));
                for (promote = WHITEKNIGHT; promote <= WHITEQUEEN; promote++) {
                    move->captured = capture;
                    move->from_square = s;
                    move->to_square = s + 9;
                    move->move_type = MOVE_CAPTUREPROMOTE;
                    move->piece = WHITEPAWN;
                    move->promote_to = promote;
                    (*i)++;
                    move++;
                }
            }
        }
        if (COLUMN(s) > 0) {
            for (capture = BLACKKNIGHT; capture <= BLACKQUEEN; capture++) {
                move_directory[s][s + 7][WHITEPAWN] = move - (4 * PIECETYPE(capture));
                for (promote = WHITEKNIGHT; promote <= WHITEQUEEN; promote++) {
                    move->captured = capture;
                    move->from_square = s;
                    move->to_square = s + 7;
                    move->move_type = MOVE_CAPTUREPROMOTE;
                    move->piece = WHITEPAWN;
                    move->promote_to = promote;
                    (*i)++;
                    move++;
                }
            }
        }
    }

    // Black non-promoting captures
    for (s = A3; s <= H7; s++) {

        // South East Capture
        if (COLUMN(s) < 7) {
            // ep capture
            if (RANK(s) == 3) {
                move->captured = WHITEPAWN;
                move->from_square = s;
                move->to_square = s - 7;
                move->move_type = MOVE_PxP_EP;
                move->piece = BLACKPAWN;
                move->promote_to = BLANK;
                move_directory[move->from_square][move->to_square][move->piece] = move;
                (*i)++;
                move++;
            }
            for (capture = WHITEKNIGHT; capture <= WHITEPAWN; capture++) {
                move->captured = capture;
                move->from_square = s;
                move->to_square = s - 7;
                if (PIECETYPE(capture) == PAWN) {
                    move->move_type = MOVE_PxPAWN;
                }
                else {
                    move->move_type = MOVE_PxPIECE;
                }
                move->piece = BLACKPAWN;
                move->promote_to = BLANK;
                move_directory[move->from_square][move->to_square][move->piece] = move - PIECETYPE(capture);
                (*i)++;
                move++;
            }
        }

        // North West Capture
        if (COLUMN(s) > 0) {
            // ep capture
            if (RANK(s) == 3) {
                move->captured = WHITEPAWN;
                move->from_square = s;
                move->to_square = s - 9;
                move->move_type = MOVE_PxP_EP;
                move->piece = BLACKPAWN;
                move->promote_to = BLANK;
                move_directory[move->from_square][move->to_square][move->piece] = move;
                (*i)++;
                move++;
            }
            for (capture = WHITEKNIGHT; capture <= WHITEPAWN; capture++) {
                move->captured = capture;
                move->from_square = s;
                move->to_square = s - 9;
                if (PIECETYPE(capture) == PAWN) {
                    move->move_type = MOVE_PxPAWN;
                }
                else {
                    move->move_type = MOVE_PxPIECE;
                }
                move->piece = BLACKPAWN;
                move->promote_to = BLANK;
                move_directory[move->from_square][move->to_square][move->piece] = move - PIECETYPE(capture);
                (*i)++;
                move++;
            }
        }

    }

    // Black Capture Promotes
    for (s = A2; s <= H2; s++) {
        if (COLUMN(s) < 7) {
            for (capture = WHITEKNIGHT; capture <= WHITEQUEEN; capture++) {
                move_directory[s][s - 7][BLACKPAWN] = move - (4 * PIECETYPE(capture));
                for (promote = BLACKKNIGHT; promote <= BLACKQUEEN; promote++) {
                    move->captured = capture;
                    move->from_square = s;
                    move->to_square = s - 7;
                    move->move_type = MOVE_CAPTUREPROMOTE;
                    move->piece = BLACKPAWN;
                    move->promote_to = promote;
                    (*i)++;
                    move++;
                }
            }
        }
        if (COLUMN(s) > 0) {
            for (capture = WHITEKNIGHT; capture <= WHITEQUEEN; capture++) {
                move_directory[s][s - 9][BLACKPAWN] = move - (4 * PIECETYPE(capture));
                for (promote = BLACKKNIGHT; promote <= BLACKQUEEN; promote++) {
                    move->captured = capture;
                    move->from_square = s;
                    move->to_square = s - 9;
                    move->move_type = MOVE_CAPTUREPROMOTE;
                    move->piece = BLACKPAWN;
                    move->promote_to = promote;
                    (*i)++;
                    move++;
                }
            }
        }
    }

}

void configure_piece_moves(int *i)
{
    int delta, d;
    t_chess_square s, target;
    t_chess_color color, opponent;
    t_chess_piece p, move_piece, capture;
    struct t_move_record *move = &xmove_list[*i];

    for (color = WHITE; color <= BLACK; color++) {
        opponent = OPPONENT(color);
        for (s = A1; s <= H8; s++) {
            for (p = KNIGHT; p <= KING; p++) {
                if (PIECETYPE(p) != PAWN) {
                    move_piece = p + (color * 8);
                    if (slider[p]) {
                        d = 0;
                        for (delta = direction[p][d]; (delta = direction[p][d]) != 0; d++) {
                            target = x88_SQUARE(s) + delta;
                            while (x88_ONBOARD(target)) {
                                move->captured = BLANK;
                                move->from_square = s;
                                move->to_square = x88_TO_64(target);
                                move->move_type = MOVE_PIECE_MOVE;
                                move->piece = move_piece;
                                move->promote_to = BLANK;
                                move_directory[move->from_square][move->to_square][move->piece] = move;
                                (*i)++;
                                move++;
                                for (capture = KNIGHT; capture <= QUEEN; capture++) {
                                    move->captured = capture + (opponent * 8);
                                    move->from_square = s;
                                    move->to_square = x88_TO_64(target);
                                    move->move_type = MOVE_PIECExPIECE;
                                    move->piece = move_piece;
                                    move->promote_to = BLANK;
                                    (*i)++;
                                    move++;
                                }
                                if ((x88_TO_64(target) >= A2) && (x88_TO_64(target) <= H7)) {
                                    move->captured = PAWN + (opponent * 8);
                                    move->from_square = s;
                                    move->to_square = x88_TO_64(target);
                                    move->move_type = MOVE_PIECExPAWN;
                                    move->piece = move_piece;
                                    move->promote_to = BLANK;
                                    (*i)++;
                                    move++;
                                }
                                target += delta;
                            }
                        }
                    }
                    else
                    {
                        d = 0;
                        for (delta = direction[p][d]; (delta = direction[p][d]) != 0; d++) {
                            target = x88_SQUARE(s) + delta;
                            if (x88_ONBOARD(target)) {
                                move->captured = BLANK;
                                move->from_square = s;
                                move->to_square = x88_TO_64(target);
                                if (PIECETYPE(move_piece) == KING)
                                    move->move_type = MOVE_KING_MOVE;
                                else
                                    move->move_type = MOVE_PIECE_MOVE;
                                move->piece = move_piece;
                                move->promote_to = BLANK;
                                move_directory[move->from_square][move->to_square][move->piece] = move;
                                (*i)++;
                                move++;
                                for (capture = KNIGHT; capture <= QUEEN; capture++) {
                                    move->captured = capture + (opponent * 8);
                                    move->from_square = s;
                                    move->to_square = x88_TO_64(target);
                                    if (PIECETYPE(move_piece) == KING)
                                        move->move_type = MOVE_KINGxPIECE;
                                    else
                                        move->move_type = MOVE_PIECExPIECE;
                                    move->piece = move_piece;
                                    move->promote_to = BLANK;
                                    (*i)++;
                                    move++;
                                }
                                if ((x88_TO_64(target) >= A2) && (x88_TO_64(target) <= H7)) {
                                    move->captured = PAWN + (opponent * 8);
                                    move->from_square = s;
                                    move->to_square = x88_TO_64(target);
                                    if (PIECETYPE(move_piece) == KING)
                                        move->move_type = MOVE_KINGxPAWN;
                                    else
                                        move->move_type = MOVE_PIECExPAWN;
                                    move->piece = move_piece;
                                    move->promote_to = BLANK;
                                    (*i)++;
                                    move++;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

void init_move_directory()
{
    int i, j;
    t_chess_square f, t;
    t_chess_piece p;
    t_chess_color color, opponent;
    static char s[64];
    struct t_move_record *move;
    uchar not_mask;

    // initialize the global move lists
    for (f = A1; f <= H8; f++) {
        for (t = A1; t <= H8; t++) {
            for (p = BLANK; p <= BLACKKING; p++) {
                move_directory[f][t][p] = NULL;
            }
        }
    }

    i = 0;
    configure_castling(&i);
    configure_pawn_push(&i);
    configure_pawn_capture(&i);
    configure_piece_moves(&i);

    //-- Fill in the data
    for (i = 0, move = &xmove_list[0]; i < GLOBAL_MOVE_COUNT; i++, move++) {
        move->history = 0;
        move->index = i;
        move->from_to_bitboard = SQUARE64(move->from_square) | SQUARE64(move->to_square);
        // castling mask
        move->castling_delta = (WHITE_CASTLE_OO | WHITE_CASTLE_OOO | BLACK_CASTLE_OO | BLACK_CASTLE_OOO);
        for (j = 0; j < 4; j++) {
            not_mask = (15 ^ ((uchar)1 << j));
            if ((move->from_square == castle[j].king_from) | (move->to_square == castle[j].king_from))
                move->castling_delta &= not_mask;
            if ((move->from_square == castle[j].rook_from) | (move->to_square == castle[j].rook_from))
                move->castling_delta &= not_mask;
        }
        move->capture_mask = 0;
        if (move->captured && (move->move_type != MOVE_PxP_EP))
            move->capture_mask = SQUARE64(move->to_square);

        color = COLOR(move->piece);
        opponent = OPPONENT(color);

        move->hash_delta = white_to_move_hash;
        move->pawn_hash_delta = white_to_move_hash;

        if (move->captured)
		{
			assert(move->piece >= 0 && move->piece < 15);
            move->mvvlva = see_piece_value[move->captured] * 100 + (see_piece_value[QUEEN] - see_piece_value[move->piece]);
		}
        else
		{
            move->mvvlva = 0;
		}
		assert(move->piece >= 0 && move->piece < 16);
		assert(move->from_square >= 0 && move->from_square < 64);
		assert(move->to_square >= 0 && move->to_square < 64);
        switch (move->move_type)
        {
        case MOVE_CASTLE:
            j = move->index;
		    assert( move->index >= 0 &&  move->index < 4);
            move->hash_delta ^= hash_value[move->piece][move->from_square] ^ hash_value[move->piece][move->to_square];
            move->hash_delta ^= hash_value[castle[j].rook_piece][castle[j].rook_from] ^ hash_value[castle[j].rook_piece][castle[j].rook_to];
			move->pawn_hash_delta ^= hash_value[move->piece][move->from_square] ^ hash_value[move->piece][move->to_square];
            break;
        case MOVE_PAWN_PUSH1:
            move->hash_delta ^= hash_value[move->piece][move->from_square] ^ hash_value[move->piece][move->to_square];
            move->pawn_hash_delta ^= hash_value[move->piece][move->from_square] ^ hash_value[move->piece][move->to_square];
            break;
        case MOVE_PAWN_PUSH2:
            move->hash_delta ^= hash_value[move->piece][move->from_square] ^ hash_value[move->piece][move->to_square];
            move->pawn_hash_delta ^= hash_value[move->piece][move->from_square] ^ hash_value[move->piece][move->to_square];
            break;
        case MOVE_PxPAWN:
			assert(PIECEINDEX(opponent, PAWN) >= 0 && PIECEINDEX(opponent, PAWN) < 64);
            move->hash_delta ^= hash_value[move->piece][move->from_square] ^ hash_value[move->piece][move->to_square] ^ hash_value[PIECEINDEX(opponent, PAWN)][move->to_square];
            move->pawn_hash_delta ^= hash_value[move->piece][move->from_square] ^ hash_value[move->piece][move->to_square] ^ hash_value[PIECEINDEX(opponent, PAWN)][move->to_square];
            break;
        case MOVE_PxPIECE:
			assert(move->captured >= 0 && move->captured < 16);
            move->hash_delta ^= hash_value[move->piece][move->from_square] ^ hash_value[move->piece][move->to_square] ^ hash_value[move->captured][move->to_square];
            move->pawn_hash_delta ^= hash_value[move->piece][move->from_square] ^ hash_value[move->piece][move->to_square];
            break;
        case MOVE_PxP_EP:
            move->hash_delta ^= hash_value[move->piece][move->from_square] ^ hash_value[move->piece][move->to_square] ^ hash_value[PIECEINDEX(opponent, PAWN)][(move->to_square - 8) + 16 * color];
            move->pawn_hash_delta ^= hash_value[move->piece][move->from_square] ^ hash_value[move->piece][move->to_square] ^ hash_value[PIECEINDEX(opponent, PAWN)][(move->to_square - 8) + 16 * color];
            break;
        case MOVE_PROMOTION:
			assert(move->promote_to >= 0 && move->promote_to < 16);
            move->hash_delta ^= hash_value[move->piece][move->from_square] ^ hash_value[move->promote_to][move->to_square];
            move->pawn_hash_delta ^= hash_value[move->piece][move->from_square];
            break;
        case MOVE_CAPTUREPROMOTE:
			assert(move->captured >= 0 && move->captured < 16);
			assert(move->promote_to >= 0 && move->promote_to < 16);
            move->hash_delta ^= hash_value[move->piece][move->from_square] ^ hash_value[move->promote_to][move->to_square] ^ hash_value[move->captured][move->to_square];
            move->pawn_hash_delta ^= hash_value[move->piece][move->from_square];
            break;
        case MOVE_PIECE_MOVE:
            move->hash_delta ^= hash_value[move->piece][move->from_square] ^ hash_value[move->piece][move->to_square];
            break;
        case MOVE_PIECExPIECE:
            move->hash_delta ^= hash_value[move->piece][move->from_square] ^ hash_value[move->piece][move->to_square] ^ hash_value[move->captured][move->to_square];
            break;
        case MOVE_PIECExPAWN:
            move->hash_delta ^= hash_value[move->piece][move->from_square] ^ hash_value[move->piece][move->to_square] ^ hash_value[move->captured][move->to_square];
            move->pawn_hash_delta ^= hash_value[move->captured][move->to_square];
            break;
        case MOVE_KING_MOVE:
            move->hash_delta ^= hash_value[move->piece][move->from_square] ^ hash_value[move->piece][move->to_square];
			move->pawn_hash_delta ^= hash_value[move->piece][move->from_square] ^ hash_value[move->piece][move->to_square];
			break;
        case MOVE_KINGxPIECE:
            move->hash_delta ^= hash_value[move->piece][move->from_square] ^ hash_value[move->piece][move->to_square] ^ hash_value[move->captured][move->to_square];
			move->pawn_hash_delta ^= hash_value[move->piece][move->from_square] ^ hash_value[move->piece][move->to_square];
			break;
        case MOVE_KINGxPAWN:
            move->hash_delta ^= hash_value[move->piece][move->from_square] ^ hash_value[move->piece][move->to_square] ^ hash_value[move->captured][move->to_square];
            move->pawn_hash_delta ^= hash_value[move->captured][move->to_square];
			move->pawn_hash_delta ^= hash_value[move->piece][move->from_square] ^ hash_value[move->piece][move->to_square];
			break;
        }
    }
}

void clear_history()
{
	struct t_move_record *move = &xmove_list[0];

	for (int i = 0; i < GLOBAL_MOVE_COUNT; i++, move++) {
		move->history = 0;
	}
}