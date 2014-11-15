//===========================================================//
//
// Maverick Chess Engine
// Copyright 2013-2014 Steve Maughan
//
//===========================================================//

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
//#include <intrin.h>

#include "defs.h"
#include "data.h"
#include "procs.h"
#include "bittwiddle.h"

t_bitboard index_to_bitboard(t_bitboard mask, int index)
{
    t_bitboard a = mask;
    t_bitboard b;
    t_bitboard result = 0;
    int n = popcount(mask);
    int i;

    assert(mask != 0);

    for (i = 0; i < n; i++) {
        b = ((a - 1) & a) ^ a;
        a &= (a - 1);
        if (((int)1 << i) & index)
            result |= b;
    }
    return(result);
}

void init_bitboards()
{
    t_chess_square s, target, target64, xtarget;
    int d, delta;
    int i, r, f;
    t_chess_color color;
    t_bitboard b;

	//-- Square Column / Rank Mask
	memset(square_column_mask, 0, sizeof(square_column_mask));
	memset(square_rank_mask, 0, sizeof(square_column_mask));
	for (f = 0; f <= 7; f++){
		for (r = 0; r <= 7; r++){
			for (i = 0; i <= 7; i++){
				square_column_mask[r * 8 + f] |= SQUARE64(i * 8 + f);
				square_rank_mask[r * 8 + f] |= SQUARE64(r * 8 + i);
			}
		}
	}

    // Initialize rank
    for (r = 0; r < 8; r++) {
        rank_mask[WHITE][r] = 0;
        rank_mask[BLACK][r] = 0;
        for (f = 0; f < 8; f++) {
            rank_mask[WHITE][r] |= SQUARE64((r * 8) + f);
            rank_mask[BLACK][r] |= SQUARE64(((7 - r) * 8) + f);
        }
    }

    //-- Column Mask
    for (f = 0; f < 8; f++) {
        column_mask[f] = 0;
        for (r = 0; r < 8; r++)
            column_mask[f] |= SQUARE64(f + r * 8);
    }

    //-- Neighboring file
    for (f = 0; f < 8; f++) {
        neighboring_file[f] = 0;
        if (f > 0) {
            for (r = 0; r < 8; r++)
                neighboring_file[f] |= SQUARE64((f - 1) + r * 8);
        }
        if (f < 7) {
            for (r = 0; r < 8; r++)
                neighboring_file[f] |= SQUARE64((f + 1) + r * 8);
        }
    }

    //-- Initialize Castle Possible
    castle[0].possible = SQUARE64(F1) | SQUARE64(G1);
    castle[1].possible = SQUARE64(D1) | SQUARE64(C1) | SQUARE64(B1);
    castle[2].possible = SQUARE64(F8) | SQUARE64(G8);
    castle[3].possible = SQUARE64(D8) | SQUARE64(C8) | SQUARE64(B8);

    //-- Initialize Castle Attacks
    castle[0].not_attacked = (SQUARE64(F1) | SQUARE64(G1));
    castle[1].not_attacked = (SQUARE64(D1) | SQUARE64(C1));
    castle[2].not_attacked = (SQUARE64(F8) | SQUARE64(G8));
    castle[3].not_attacked = (SQUARE64(D8) | SQUARE64(C8));

    //-- Initialize Rook Square
    castle[0].rook_from = H1;
    castle[1].rook_from = A1;
    castle[2].rook_from = H8;
    castle[3].rook_from = A8;

    castle[0].rook_to	= F1;
    castle[1].rook_to = D1;
    castle[2].rook_to = F8;
    castle[3].rook_to = D8;

    //-- Initialize King Square
    castle[0].king_from = E1;
    castle[1].king_from = E1;
    castle[2].king_from = E8;
    castle[3].king_from = E8;

    for (i = 0; i < 4; i++) {
        castle[i].rook_from_to = (SQUARE64(castle[i].rook_from) | SQUARE64(castle[i].rook_to));
        castle[i].rook_piece = PIECEINDEX((i >> 1), ROOK);
    }
    // Initialize BETWEEN array
    for (s = A1; s <= H8; s++) {
        for (target = A1; target <= H8; target++) {
            between[s][target] = 0;
            line[s][target] = 0;
            xray[s][target] = 0;
        }
    }

    for (s = A1; s <= H8; s++) {

        // Pawn attacks
        b = SQUARE64(s);

        //--White
        pawn_attackers[WHITE][s] = 0;
        if (b & ~(rank_mask[WHITE][FIRST_RANK] | rank_mask[WHITE][SECOND_RANK])) {
            if (COLUMN(s) > 0)
                pawn_attackers[WHITE][s] |= (b >> 9);
            if (COLUMN(s) < 7)
                pawn_attackers[WHITE][s] |= (b >> 7);
        }
        //--Black
        pawn_attackers[BLACK][s] = 0;
        if (b & ~(rank_mask[BLACK][FIRST_RANK] | rank_mask[BLACK][SECOND_RANK])) {
            if (COLUMN(s) > 0)
                pawn_attackers[BLACK][s] |= (b << 7);
            if (COLUMN(s) < 7)
                pawn_attackers[BLACK][s] |= (b << 9);
        }

		//-- in front white
		forward_squares[WHITE][s] = 0;
		forward_squares[WHITE][s] = (b << 8) | (b << 16) | (b << 24) | (b << 32) | (b << 40) | (b << 48) | (b << 56);

		//--in front black
		forward_squares[BLACK][s] = 0;
		forward_squares[BLACK][s] = (b >> 8) | (b >> 16) | (b >> 24) | (b >> 32) | (b >> 40) | (b >> 48) | (b >> 56);

		// Bishop
        line[s][s] = SQUARE64(s);
        d = 0;
        for (delta = direction[BISHOP][d]; (delta = direction[BISHOP][d]) != 0; d++) {
            target = x88_SQUARE(s) + delta;
            while (x88_ONBOARD(target)) {
                target64 = x88_TO_64(target);
                line[s][target64] = (SQUARE64(target64) | line[s][x88_TO_64(target - delta)]);
                if (x88_ONBOARD(target + delta))
                    between[s][x88_TO_64(target + delta)] = (SQUARE64(x88_TO_64(target)) | between[s][x88_TO_64(target)]);
                bishop_rays[s] |= SQUARE64(target64);
                queen_rays[s] |= SQUARE64(target64);
                xray[s][target64] = 0;
                xtarget = target + delta;
                while (x88_ONBOARD(xtarget)) {
                    xray[s][target64] |= SQUARE64(x88_TO_64(xtarget));
                    xtarget += delta;
                }
                target += delta;
            }
        }
        // Rook
        line[s][s] = SQUARE64(s);
        d = 0;
        for (delta = direction[ROOK][d]; (delta = direction[ROOK][d]) != 0; d++) {
            target = x88_SQUARE(s) + delta;
            while (x88_ONBOARD(target)) {
                target64 = x88_TO_64(target);
                line[s][target64] = (SQUARE64(target64) | line[s][x88_TO_64(target - delta)]);
                if (x88_ONBOARD(target + delta))
                    between[s][x88_TO_64(target + delta)] = (SQUARE64(x88_TO_64(target)) | between[s][x88_TO_64(target)]);
                rook_rays[s] |= SQUARE64(target64);
                queen_rays[s] |= SQUARE64(target64);
                xray[s][target64] = 0;
                xtarget = target + delta;
                while (x88_ONBOARD(xtarget)) {
                    xray[s][target64] |= SQUARE64(x88_TO_64(xtarget));
                    xtarget += delta;
                }
                target += delta;
            }
        }
        // Knight
        knight_mask[s] = 0;
        d = 0;
        for (delta = direction[KNIGHT][d]; (delta = direction[KNIGHT][d]) != 0; d++) {
            target = x88_SQUARE(s) + delta;
            if (x88_ONBOARD(target))
                knight_mask[s] |= SQUARE64(x88_TO_64(target));
        }
        // King
        king_mask[s] = 0;
        d = 0;
        for (delta = direction[KING][d]; (delta = direction[KING][d]) != 0; d++) {
            target = x88_SQUARE(s) + delta;
            if (x88_ONBOARD(target))
                king_mask[s] |= SQUARE64(x88_TO_64(target));
        }

		//-- Connected passed pawn mask
		connected_pawn_mask[s] = king_mask[s] & neighboring_file[COLUMN(s)];
    }

	assert(connected_pawn_mask[C2] == (SQUARE64(B1) | SQUARE64(B2) | SQUARE64(B3) | SQUARE64(D1) | SQUARE64(D2) | SQUARE64(D3)));
    assert(pawn_attackers[WHITE][E4] == (SQUARE64(D3) | SQUARE64(F3)));
    assert(pawn_attackers[BLACK][H4] == SQUARE64(G5));
    assert(pawn_attackers[WHITE][A8] == SQUARE64(B7));
    assert(xray[C2][E4] == (SQUARE64(F5) | SQUARE64(G6) | SQUARE64(H7)));
    assert(line[C2][F2] == (SQUARE64(C2) | SQUARE64(D2) | SQUARE64(E2) | SQUARE64(F2)));
    assert(line[D4][G1] == (SQUARE64(D4) | SQUARE64(E3) | SQUARE64(F2) | SQUARE64(G1)));
}

void init_magic()
{
    t_chess_square square;
    t_bitboard mask, and_result;
    int i, index;

    for (square = A1; square <= H8; square++) {
        // Reset
        for (i = 0; i < 4096; i++)
            rook_magic_moves[square][i] = 0;
        for (i = 0; i < 512; i++)
            bishop_magic_moves[square][i] = 0;

        // Rooks
        mask = create_rook_mask(square);
        for (i = 0; i < (int)1 << popcount(mask); i++) {
            and_result = index_to_bitboard(mask, i);
            index = (int)((and_result * rook_magic[square].magic) >> 52);
            assert((rook_magic_moves[square][index] == 0) || (rook_magic_moves[square][index] == create_rook_attacks(square, and_result)));
            rook_magic_moves[square][index] = create_rook_attacks(square, and_result);
        }
        // Bishops
        mask = create_bishop_mask(square);
        for (i = 0; i < (int)1 << popcount(mask); i++) {
            and_result = index_to_bitboard(mask, i);
            index = (int)((and_result * bishop_magic[square].magic) >> 55);
            assert((bishop_magic_moves[square][index] == 0) || (bishop_magic_moves[square][index] == create_bishop_attacks(square, and_result)));
            bishop_magic_moves[square][index] = create_bishop_attacks(square, and_result);
        }
    }
}

t_bitboard create_rook_mask(t_chess_square s)
{
    int s_rank, s_column, r, c;
    t_bitboard b = 0;

    s_rank = RANK(s);
    s_column = COLUMN(s);

    // East
    r = s_rank;
    c = s_column;
    for (c = s_column + 1; c < 7; c++) {
        b |= SQUARE64(r * 8 + c);
    }

    // West
    r = s_rank;
    c = s_column;
    for (c = s_column - 1; c > 0; c--) {
        b |= SQUARE64(r * 8 + c);
    }

    // North
    r = s_rank;
    c = s_column;
    for (r = s_rank + 1; r < 7; r++) {
        b |= SQUARE64(r * 8 + c);
    }

    // South
    r = s_rank;
    c = s_column;
    for (r = s_rank - 1; r > 0; r--) {
        b |= SQUARE64(r * 8 + c);
    }

    return b;
}

t_bitboard create_bishop_mask(t_chess_square s)
{
    int s_rank, s_column, r, c;
    t_bitboard b = 0;

    s_rank = RANK(s);
    s_column = COLUMN(s);

    // North East
    for (c = s_column + 1, r = s_rank + 1; c < 7 && r < 7; c++, r++) {
        b |= SQUARE64(r * 8 + c);
    }
    // North West
    for (c = s_column - 1, r = s_rank + 1; c > 0 && r < 7; c--, r++) {
        b |= SQUARE64(r * 8 + c);
    }
    // South East
    for (c = s_column + 1, r = s_rank - 1; c < 7 && r > 0; c++, r--) {
        b |= SQUARE64(r * 8 + c);
    }
    // South West
    for (c = s_column - 1, r = s_rank - 1; c > 0 && r > 0; c--, r--) {
        b |= SQUARE64(r * 8 + c);
    }

    return b;
}

t_bitboard create_rook_attacks(t_chess_square s, t_bitboard mask)
{
    int s_rank, s_column, r, c;
    t_bitboard b = 0;

    s_rank = RANK(s);
    s_column = COLUMN(s);

    // East
    for (c = s_column + 1, r = s_rank; c <= 7; c++) {
        b |= SQUARE64(r * 8 + c);
        if (mask & SQUARE64(r * 8 + c))
            break;
    }
    // West
    for (c = s_column - 1, r = s_rank; c >= 0; c--) {
        b |= SQUARE64(r * 8 + c);
        if (mask & SQUARE64(r * 8 + c))
            break;
    }
    // North
    for (c = s_column, r = s_rank + 1; r <= 7; r++) {
        b |= SQUARE64(r * 8 + c);
        if (mask & SQUARE64(r * 8 + c))
            break;
    }
    // South
    for (c = s_column, r = s_rank - 1; r >= 0; r--) {
        b |= SQUARE64(r * 8 + c);
        if (mask & SQUARE64(r * 8 + c))
            break;
    }

    return b;
}

t_bitboard create_bishop_attacks(t_chess_square s, t_bitboard mask)
{
    int s_rank, s_column, r, c;
    t_bitboard b = 0;

    s_rank = RANK(s);
    s_column = COLUMN(s);

    // North East
    for (c = s_column + 1, r = s_rank + 1; c <= 7 && r <= 7; c++, r++) {
        b |= SQUARE64(r * 8 + c);
        if (mask & SQUARE64(r * 8 + c))
            break;
    }
    // North West
    for (c = s_column - 1, r = s_rank + 1; c >= 0 && r <= 7; c--, r++) {
        b |= SQUARE64(r * 8 + c);
        if (mask & SQUARE64(r * 8 + c))
            break;
    }
    // South East
    for (c = s_column + 1, r = s_rank - 1; c <= 7 && r >= 0; c++, r--) {
        b |= SQUARE64(r * 8 + c);
        if (mask & SQUARE64(r * 8 + c))
            break;
    }
    // South West
    for (c = s_column - 1, r = s_rank - 1; c >= 0 && r >= 0; c--, r--) {
        b |= SQUARE64(r * 8 + c);
        if (mask & SQUARE64(r * 8 + c))
            break;
    }

    return b;
}
