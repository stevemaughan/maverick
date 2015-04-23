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

void set_fen(struct t_board *board, char *epd)
{

    int r, c, i, l;
    char s;
    int count;
    char fen[1024], tms[1024], ep[1024], castle[1024];

    clear_board(board);

    board->chess960 = FALSE;

    fen[0] = 0;
    tms[0] = 0;
    ep[0] = 0;
    castle[0] = 0;

    board->hash = 0;
    board->pawn_hash = 0;
    board->material_hash = 0;

    count = word_count(epd);
    if (count >= 0) strcpy(fen, word_index(0, epd));
    if (count >= 1) strcpy(tms, word_index(1, epd));
    if (count >= 2) strcpy(castle, word_index(2, epd));
    if (count >= 3) strcpy(ep, word_index(3, epd));

    l = (int)strlen(fen);
    r = 7;
    c = 0;
    for (i = 0; i < l; i++)
    {
        s = fen[i];
        switch (s)
        {
        case '1' :
            c++;
            break;
        case '2' :
            c += 2;
            break;
        case '3' :
            c += 3;
            break;
        case '4' :
            c += 4;
            break;
        case '5' :
            c += 5;
            break;
        case '6' :
            c += 6;
            break;
        case '7' :
            c += 7;
            break;
        case '8' :
            c += 8;
            break;
        case 'B' :
            add_piece(board, WHITEBISHOP, (r * 8) + c);
            c++;
            break;
        case 'N' :
            add_piece(board, WHITEKNIGHT, (r * 8) + c);
            c++;
            break;
        case 'R' :
            add_piece(board, WHITEROOK, (r * 8) + c);
            c++;
            break;
        case 'P' :
            add_piece(board, WHITEPAWN, (r * 8) + c);
            c++;
            break;
        case 'Q' :
            add_piece(board, WHITEQUEEN, (r * 8) + c);
            c++;
            break;
        case 'K' :
            add_piece(board, WHITEKING, (r * 8) + c);
            c++;
            break;
        case 'b' :
            add_piece(board, BLACKBISHOP, (r * 8) + c);
            c++;
            break;
        case 'n' :
            add_piece(board, BLACKKNIGHT, (r * 8) + c);
            c++;
            break;
        case 'r' :
            add_piece(board, BLACKROOK, (r * 8) + c);
            c++;
            break;
        case 'p' :
            add_piece(board, BLACKPAWN, (r * 8) + c);
            c++;
            break;
        case 'q' :
            add_piece(board, BLACKQUEEN, (r * 8) + c);
            c++;
            break;
        case 'k' :
            add_piece(board, BLACKKING, (r * 8) + c);
            c++;
            break;
        case '/' :
            c = 0;
            r--;
        }
    }

    /* Decide who is to move */
    if (strlen(tms)>0)
    {
        if (toupper(tms[0])=='W')
            board->to_move = WHITE;
        else if (toupper(tms[0])=='B')
            board->to_move = BLACK;
        else
            board->to_move = WHITE;
    }
    else
    {
        if (is_in_check(board, WHITE))
            board->to_move = WHITE;
        else if (is_in_check(board, BLACK))
            board->to_move = BLACK;
        else
            board->to_move = WHITE;
    }

    board->in_check = attack_count(board, board->king_square[board->to_move], OPPONENT(board->to_move));
    board->check_attacker = who_is_attacking_square(board, board->king_square[board->to_move], OPPONENT(board->to_move));

    //-- decide castling rights --//
    board->castling_squares_changed = FALSE;

    board->castling = 0;
    if (strlen(castle) > 0)
    {

        // Is this a Chess960 game?
        if (board->chess960) {

            if (strchr(castle, 'K') != NULL) {
                init_960_castling(board, board->king_square[WHITE], kingside_rook(board, WHITE));
            }
            if (strchr(castle, 'Q') != NULL) {
                init_960_castling(board, board->king_square[WHITE], queenside_rook(board, WHITE));
            }
            if (strchr(castle, 'k') != NULL) {
                init_960_castling(board, board->king_square[BLACK], kingside_rook(board, BLACK));
            }
            if (strchr(castle, 'q') != NULL) {
                init_960_castling(board, board->king_square[BLACK], queenside_rook(board, BLACK));
            }
        }

        // It's probably a regular chess game
        else {
            if ((strchr(castle, 'K') != NULL) && (board->square[4] == WHITEKING) && (board->square[7] == WHITEROOK))
                init_960_castling(board, E1, H1);
            if ((strchr(castle, 'Q') != NULL) && (board->square[4] == WHITEKING) && (board->square[0] == WHITEROOK))
                init_960_castling(board, E1, A1);
            if ((strchr(castle, 'k') != NULL) && (board->square[60] == BLACKKING) && (board->square[63] == BLACKROOK))
                init_960_castling(board, E8, H8);
            if ((strchr(castle, 'q') != NULL) && (board->square[60] == BLACKKING) && (board->square[56] == BLACKROOK))
                init_960_castling(board, E8, A8);

            // ...but then again it's Chess960 if it has these characters in the castling rights
            for (s = 'A'; s <= 'H'; s++)
                if (strchr(castle, s) != NULL) {
                    init_960_castling(board, board->king_square[WHITE], s - 'A');
                    board->chess960 = TRUE;
                    uci.options.chess960 = TRUE;
                }

            //  .. or these characters
            for (s = 'a'; s <= 'h'; s++)
                if (strchr(castle, s) != NULL) {
                    init_960_castling(board, board->king_square[BLACK], A8 + s - 'a');
                    board->chess960 = TRUE;
                    uci.options.chess960 = TRUE;
                }
        }
    }

    //-- Recalculate changes to castling rights if necessary
    if (board->castling_squares_changed)
        init_directory_castling_delta();

    /* en-passant */
    board->ep_square = 0;
    if (ep[0] != '-')
    {
        if (board->to_move ==  WHITE) {
            i = ep[0] - 'a' + 40;
        }
        else {
            i = ep[0] - 'a' + 16;
        }
        assert(board->to_move >= 0 && board->to_move < 2);
        assert(i >= 0 && i < 64);
        if (pawn_attackers[board->to_move][i] & board->pieces[board->to_move][PAWN]) {
            board->ep_square = SQUARE64(i);
            board->hash ^= ep_hash[COLUMN(i)];
        }
    }

    //--Hash
    board->hash ^= castle_hash[board->castling];
    if (board->to_move == WHITE) {
        board->hash ^= white_to_move_hash;
        board->pawn_hash ^= white_to_move_hash;
    }

    assert(board->hash == calc_board_hash(board));
    assert(board->pawn_hash == calc_pawn_hash(board));

    //// Reset draw variables
    board->fifty_move_count = 0;
    draw_stack_count = 0;
    draw_stack[0] = board->hash;

    //// Evaluate Position
    //board->static_value = evaluate(board);
}

char *get_fen(struct t_board *board)
{
    static char s[100];
    int r, c, i, n;

    i=0;
    r=7;
    while (r >= 0) {
        c = 0;
        while (c <= 7) {
            n=0;
            while ((c <= 7) && (board->square[(r * 8) + c] == BLANK)) {
                n++;
                c++;
            }
            if (n) {
                s[i]='0' + n;
                i++;
            }
            else {
                switch (board->square[(r * 8) + c]) {
                case WHITEPAWN :
                    s[i]='P';
                    break;
                case WHITEKNIGHT :
                    s[i]='N';
                    break;
                case WHITEBISHOP :
                    s[i]='B';
                    break;
                case WHITEROOK :
                    s[i]='R';
                    break;
                case WHITEQUEEN :
                    s[i]='Q';
                    break;
                case WHITEKING :
                    s[i]='K';
                    break;
                case BLACKPAWN :
                    s[i]='p';
                    break;
                case BLACKKNIGHT :
                    s[i]='n';
                    break;
                case BLACKBISHOP :
                    s[i]='b';
                    break;
                case BLACKROOK :
                    s[i]='r';
                    break;
                case BLACKQUEEN :
                    s[i]='q';
                    break;
                case BLACKKING :
                    s[i]='k';
                    break;
                }
                i++;
                c++;
            }
            if ((c > 7) && (r > 0)) {
                s[i]='/';
                i++;
            }
        }
        r--;
    }
    /*To Move*/
    s[i] = ' ';
    i++;
    if (board->to_move == WHITE) {
        s[i]='w';
    }
    else {
        s[i]='b';
    }
    i++;
    s[i]=' ';
    i++;

    //-- Castling
    if (board->castling) {

        if (board->chess960) {
            if (board->castling & WHITE_CASTLE_OO) {
                s[i] = 'A' + castle[0].rook_from;
                i++;
            }
            if (board->castling & WHITE_CASTLE_OOO) {
                s[i] = 'A' + castle[1].rook_from;
                i++;
            }
            if (board->castling & BLACK_CASTLE_OO) {
                s[i] = 'a' + castle[2].rook_from;
                i++;
            }
            if (board->castling & BLACK_CASTLE_OOO) {
                s[i] = 'a' + castle[3].rook_from;
                i++;
            }
        }

        else {

            if (board->castling & WHITE_CASTLE_OO) {
                s[i] = 'K';
                i++;
            }
            if (board->castling & WHITE_CASTLE_OOO) {
                s[i] = 'Q';
                i++;
            }
            if (board->castling & BLACK_CASTLE_OO) {
                s[i] = 'k';
                i++;
            }
            if (board->castling & BLACK_CASTLE_OOO) {
                s[i] = 'q';
                i++;
            }
        }
    }
    else {
        s[i] = '-';
        i++;
    }
    s[i] = ' ';
    i++;
    /*en-passant*/
    if (board->ep_square) {
        s[i] = 'a'+(bitscan(board->ep_square) & 7);
        i++;
    }
    else {
        s[i] = '-';
        i++;
    }

    s[i] = 0;
    return s;
}
