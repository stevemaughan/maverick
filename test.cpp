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
#include "bittwiddle.h"

void test_procedure()
{
    if (!uci.engine_initialized)
        init_engine(position);

    assert(test_bitscan());
	assert(test_bittwiddles());
    assert(test_fen());
    assert(test_genmove());
    assert(test_make_unmake());
    assert(test_hash());
    assert(test_eval());
    assert(test_capture_gen());
    assert(test_check_gen());
    assert(test_alt_move_gen());
    assert(test_see());
    assert(test_position());
	assert(test_hash_table());
	assert(test_ep_capture());
	//assert(test_book());
    test_search();
}

BOOL test_bitscan() {
    int i, j;
    t_bitboard b;
    t_bitboard c;
    for (i = 0; i < 64; i++) {
        b = rand64();
        b |= SQUARE64(0);
        b <<= i;
        j = bitscan(b);
        if (j != i)	return FALSE;
        c = (b & -b);
        if (c != SQUARE64(i)) return FALSE;
    }
    return TRUE;
}

BOOL test_bittwiddles()
{
	BOOL ok = true;

	ok &= SQUARECOLOR(A1) == BLACK;
	ok &= SQUARECOLOR(H1) == WHITE;
	ok &= SQUARECOLOR(A2) == WHITE;
	ok &= SQUARECOLOR(E4) == WHITE;

	return ok;
}

BOOL test_fen() {
    set_fen(position, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -");
    assert(integrity(position));
    set_fen(position, "r5r1/n1q1pP1k/3pPppp/P1pP4/2P4N/R1B5/2Q3PP/7K w - -");
    return (!strcmp(get_fen(position), "r5r1/n1q1pP1k/3pPppp/P1pP4/2P4N/R1B5/2Q3PP/7K w - -"));
}

BOOL test_genmove() {

    BOOL ok = TRUE;

    struct t_move_list moves[1];

    set_fen(position, "r5r1/n1q1pP1k/3pPppp/P1pP4/2P4N/R1B5/2Q3PP/7K w - -");
    assert(integrity(position));
    assert(is_square_attacked(position, E4, WHITE));
    assert(!is_square_attacked(position, A7, WHITE));
    assert(!is_square_attacked(position, F4, BLACK));
    assert(is_square_attacked(position, D8, BLACK));
    generate_moves(position, moves);
    ok = ok && (moves->count == 42);

    flip_board(position);
    generate_moves(position, moves);
    //write_move_list(moves, "movelist.txt");
    ok = ok && (moves->count == 42);

    set_fen(position, "1r2k2r/p1ppqpb1/b3pnp1/3PN3/1pn1P3/2N2Q1p/PPPBBPPP/R4K1R w - -");
    assert(integrity(position));
    generate_moves(position, moves);
    assert(move_list_integrity(position, moves));
    ok = ok && (moves->count == 44);

    set_fen(position, "4q3/3P1P2/b4N2/8/3Q2Bb/2p3B1/1k4N1/4K1Nr w - -");
    assert(integrity(position));
    generate_evade_check(position, moves);
    ok = ok && (moves->count == 18);

    flip_board(position);
    generate_evade_check(position, moves);
    ok = ok && (moves->count == 18);

    set_fen(position, "1r2k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R4K1R w --k- -");
    assert(integrity(position));
    generate_moves(position, moves);
    //write_move_list(moves, "movelist.txt");
    assert(move_list_integrity(position, moves));
    ok = ok && (moves->count == 46);

    flip_board(position);
    generate_moves(position, moves);
    ok = ok && (moves->count == 46);

	// Chess960 Examples
	set_fen(position, "Rr4kr/8/8/8/8/8/PPPP4/R1K5 w Ahb -");
	assert(integrity(position));
	generate_moves(position, moves);
	assert(move_list_integrity(position, moves));
	ok = ok && (moves->count == 18);

	flip_board(position);
	generate_moves(position, moves);
	ok = ok && (moves->count == 18);

	set_fen(position, "1r1kbb1r/1pp2ppp/3npn2/3pN3/1Q3P2/4PN2/2PP2PP/qR1KBB1R w HBhb -");
	assert(integrity(position));
	generate_legal_moves(position, moves);
	assert(move_list_integrity(position, moves));
	ok = ok && (moves->count == 48);

	set_fen(position, "rkrbqnb1/pp2p2p/3p1pp1/2p1nP2/2P1P3/3P2N1/PP4PP/RKRBQNB1 w CAca -");
	assert(integrity(position));
	generate_legal_moves(position, moves);
	write_move_list(moves, "movelist.txt");
	assert(move_list_integrity(position, moves));
	ok = ok && (moves->count == 34);

	flip_board(position);
	generate_legal_moves(position, moves);
	ok = ok && (moves->count == 34);


    return ok;
}

BOOL test_make_unmake() {

    BOOL ok = TRUE;
    int i, j;

    struct t_move_list moves[1];
    struct t_undo undo[1];

    set_fen(position, "rnbqkb1r/ppppp1pp/7n/5p2/4P3/8/PPPP1PPP/RNBQKBNR w KQkq -");
    assert(integrity(position));
    for (j = WHITE; j <= BLACK; j++) {
        generate_moves(position, moves);
        for (i = 0; i < moves->count; i++) {
            assert(integrity(position));
            if (make_move(position, moves->pinned_pieces, moves->move[i], undo)) {
                assert(integrity(position));
                unmake_move(position, undo);
                assert(integrity(position));
            }
        }
        flip_board(position);
    }
    ok = integrity(position);

    set_fen(position, "r3Rbk1/2p2p1p/p2p4/1p1P2q1/8/PBPQ2pP/1P3P1P/3R2K1 b - -");
    assert(integrity(position));
    for (j = WHITE; j <= BLACK; j++) {
        generate_moves(position, moves);
        for (i = 0; i < moves->count; i++) {
            assert(integrity(position));
            if (make_move(position, moves->pinned_pieces, moves->move[i], undo)) {
                assert(integrity(position));
                unmake_move(position, undo);
                assert(integrity(position));
            }
        }
        flip_board(position);
    }
    ok &= integrity(position);

    return ok;
}

BOOL test_perft960() {

	BOOL ok = TRUE;
	int i;
	t_nodes n = 0;

	if (!uci.engine_initialized)
		init_engine(position);

	//--Position 1
	//for (i = 0; i <= 1; i++) {
	//	set_fen(position, "R3rkrR/8/8/8/8/8/8/r3RKRr w EGeg - 0 1");
	//	n = perft(position, 5);
	//	ok &= (n == 12667098);
	//	global_nodes += n;
	//	flip_board(position);
	//}

	//--Position 2
	//for (i = 0; i <= 1; i++) {
		set_fen(position, "qr1kbb1r/1pp2ppp/3npn2/3pN3/1p3P2/4PN2/P1PP2PP/QR1KBB1R w HBhb -");
		n = perft(position, 6);
		ok &= (n == 3206947488);
	//	global_nodes += n;
	//	flip_board(position);
	//}

	if (ok)
		printf("Everything seems Fine - all PERFT 960 scores are correct\n");
	else
		printf("**ERROR** with PERFT 960 scores\n");

	return ok;
}

BOOL test_perft() {

    BOOL ok = TRUE;
    int i;
    t_nodes n = 0;

    if (!uci.engine_initialized)
        init_engine(position);

    global_nodes = 0;
    perft_start_time = time_now();

    //--Position 1
    for (i = 0; i <= 1; i++) {
        set_fen(position, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        n = perft(position, 6);
        ok &= (n == 119060324);
        global_nodes += n;
        flip_board(position);
    }
    //--Position 2
    for (i = 0; i <= 1; i++) {
        set_fen(position, "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -");
        n = perft(position, 5);
        ok &= (n == 193690690);
        global_nodes += n;
        flip_board(position);
    }
    //--Position 3
    for (i = 0; i <= 1; i++) {
        set_fen(position, "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -");
        n = perft(position, 7);
        ok &= (n == 178633661);
        global_nodes += n;
        flip_board(position);
    }
    //--Position 4
    for (i = 0; i <= 1; i++) {
        set_fen(position, "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1");
        n = perft(position, 6);
        ok &= (n == 706045033);
        global_nodes += n;
        flip_board(position);
    }

    perft_end_time = time_now();

    //--Position 5
    for (i = 0; i <= 1; i++) {
        set_fen(position, "1k6/1b6/8/8/7R/8/8/4K2R b K - 0 1");
        ok &= (perft(position, 5) == 1063513);
        flip_board(position);
    }

    //--Illegal ep move #1
    for (i = 0; i <= 1; i++) {
        set_fen(position, "3k4/3p4/8/K1P4r/8/8/8/8 b - - 0 1");
        ok &= (perft(position, 6) == 1134888);
        flip_board(position);
    }

    //--Illegal ep move #2
    for (i = 0; i <= 1; i++) {
        set_fen(position, "8/8/4k3/8/2p5/8/B2P2K1/8 w - - 0 1");
        ok &= (perft(position, 6) == 1015133);
        flip_board(position);
    }

    //--EP Capture Checks Opponent
    for (i = 0; i <= 1; i++) {
        set_fen(position, "8/8/1k6/2b5/2pP4/8/5K2/8 b - d3 0 1");
        ok &= (perft(position, 6) == 1440467);
        flip_board(position);
    }

    //--Short Castling Gives Check
    for (i = 0; i <= 1; i++) {
        set_fen(position, "5k2/8/8/8/8/8/8/4K2R w K - 0 1");
        ok &= (perft(position, 6) == 661072);
        flip_board(position);
    }

    //--Long Castling Gives Check
    for (i = 0; i <= 1; i++) {
        set_fen(position, "3k4/8/8/8/8/8/8/R3K3 w Q - 0 1");
        ok &= (perft(position, 6) == 803711);
        flip_board(position);
    }

    //--Castle Rights
    for (i = 0; i <= 1; i++) {
        set_fen(position, "r3k2r/1b4bq/8/8/8/8/7B/R3K2R w KQkq - 0 1");
        ok &= (perft(position, 4) == 1274206);
        flip_board(position);
    }

    //--Castling Prevented
    for (i = 0; i <= 1; i++) {
        set_fen(position, "r3k2r/8/3Q4/8/8/5q2/8/R3K2R b KQkq - 0 1");
        ok &= (perft(position, 4) == 1720476);
        flip_board(position);
    }

    //--Promote out of Check
    for (i = 0; i <= 1; i++) {
        set_fen(position, "2K2r2/4P3/8/8/8/8/8/3k4 w - - 0 1");
        ok &= (perft(position, 6) == 3821001);
        flip_board(position);
    }

    //--Discovered Check
    for (i = 0; i <= 1; i++) {
        set_fen(position, "8/8/1P2K3/8/2n5/1q6/8/5k2 b - - 0 1");
        ok &= (perft(position, 5) == 1004658);
        flip_board(position);
    }

    //--Promote to give check
    for (i = 0; i <= 1; i++) {
        set_fen(position, "4k3/1P6/8/8/8/8/K7/8 w - - 0 1");
        ok &= (perft(position, 6) == 217342);
        flip_board(position);
    }

    //--Under Promote to give check
    for (i = 0; i <= 1; i++) {
        set_fen(position, "8/P1k5/K7/8/8/8/8/8 w - - 0 1");
        ok &= (perft(position, 6) == 92683);
        flip_board(position);
    }

    //--Self Stalemate
    for (i = 0; i <= 1; i++) {
        set_fen(position, "K1k5/8/P7/8/8/8/8/8 w - - 0 1");
        ok &= (perft(position, 6) == 2217);
        flip_board(position);
    }

    //--Stalemate & Checkmate
    for (i = 0; i <= 1; i++) {
        set_fen(position, "8/k1P5/8/1K6/8/8/8/8 w - - 0 1");
        ok &= (perft(position, 7) == 567584);
        flip_board(position);
    }

    //--Stalemate & Checkmate
    for (i = 0; i <= 1; i++) {
        set_fen(position, "8/8/2k5/5q2/5n2/8/5K2/8 b - - 0 1");
        ok &= (perft(position, 4) == 23527);
        flip_board(position);
    }

    if (ok)
        printf("Everything seems Fine - all PERFT scores are correct\n");
    else
        printf("**ERROR** with PERFT scores\n");

	printf(INFO_STRING_PERFT_SPEED, global_nodes, perft_end_time - perft_start_time, 1000 * global_nodes / (perft_end_time - perft_start_time));
    return ok;
}

BOOL test_hash() {
    BOOL ok = TRUE;

	set_fen(position, "r6b/p1pp1k2/1n6/8/4n3/2P3Pp/P1PBKP1P/R6R w - -");
	ok &= (position->hash == 9349117323807073039);

    set_fen(position, "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1");
    ok &= (position->hash == 0x823c9b50fd114196);

    set_fen(position, "rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 2");
    ok &= (position->hash == 0x756b94461c50fb0);

    set_fen(position, "rnbqkbnr/ppp1pppp/8/3pP3/8/8/PPPP1PPP/RNBQKBNR b KQkq - 0 2");
    ok &= (position->hash == 0x662fafb965db29d4);

    set_fen(position, "rnbqkbnr/ppp1p1pp/8/3pPp2/8/8/PPPP1PPP/RNBQKBNR w KQkq f6 0 3");
    ok &= (position->hash == 0x22a48b5a8e47ff78);

    set_fen(position, "rnbqkbnr/ppp1p1pp/8/3pPp2/8/8/PPPPKPPP/RNBQ1BNR b kq - 0 3");
    ok &= (position->hash == 0x652a607ca3f242c1);

    set_fen(position, "rnbq1bnr/ppp1pkpp/8/3pPp2/8/8/PPPPKPPP/RNBQ1BNR w - - 0 4");
    ok &= (position->hash == 0x00fdd303c946bdd9);

    set_fen(position, "rnbqkbnr/p1pppppp/8/8/PpP4P/8/1P1PPPP1/RNBQKBNR b KQkq c3 0 3");
    ok &= (position->hash == 0x3c8123ea7b067637);

    set_fen(position, "rnbqkbnr/p1pppppp/8/8/P6P/R1p5/1P1PPPP1/1NBQKBNR b Kkq - 0 4");
    ok &= (position->hash == 0x5c3f9b829b279560);

    return ok;

}

BOOL test_capture_gen() {

    BOOL ok = TRUE;
    struct t_move_list moves[1];

    set_fen(position, "8/pppr2pp/3pKp2/2Q3bn/8/b6k/PPP1P2P/3R2n1 w - -");
    generate_captures(position, moves);
    ok &= (moves->count == 12);

    flip_board(position);
    generate_captures(position, moves);
    ok &= (moves->count == 12);

    return ok;

}

BOOL test_check_gen() {

    BOOL ok = TRUE;
    struct t_move_list moves[1];

    set_fen(position, "8/8/2K5/4k1PQ/8/5P2/1N1P4/2R3B1 w - -");
    moves->count = 0;
    generate_quiet_checks(position, moves);
    ok &= (moves->count == 11);

    flip_board(position);
    moves->count = 0;
    generate_quiet_checks(position, moves);
    ok &= (moves->count == 11);

    return ok;

}

BOOL test_alt_move_gen() {

    BOOL ok = TRUE;
    struct t_move_list moves[1], xmoves[2];

    set_fen(position, "5rN1/4P3/1B6/1B3k2/8/4P3/6PP/2RQK2R w K -");
    generate_captures(position, moves);
    generate_quiet_checks(position, moves);
    generate_no_capture_no_checks(position, moves);

    generate_moves(position, xmoves);

    ok &= (equal_move_lists(xmoves, moves));

    flip_board(position);
    generate_captures(position, moves);
    generate_quiet_checks(position, moves);
    generate_no_capture_no_checks(position, moves);

    generate_moves(position, xmoves);
    ok &= (equal_move_lists(xmoves, moves));

    return ok;

}

BOOL test_eval() {

    struct t_chess_eval eval[1];

    t_chess_value v;
    BOOL ok = TRUE;

    init_eval(eval);

	set_fen(position, "1k6/8/8/5PP1/3Pp2p/P7/8/4K3 w - -");
    v = evaluate(position, eval);
    flip_board(position);
    ok &= (v == evaluate(position, eval));

    set_fen(position, "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -");
    v = evaluate(position, eval);
    flip_board(position);
    ok &= (v == evaluate(position, eval));

    set_fen(position, "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1");
    v = evaluate(position, eval);
    flip_board(position);
    ok &= (v == evaluate(position, eval));

    set_fen(position, "1k6/8/8/4b3/8/3B4/K7/8 b - -");
    v = evaluate(position, eval);
    flip_board(position);
    ok &= (v == evaluate(position, eval));

	set_fen(position, "8/8/7k/2R1Q1n1/1pRp4/8/2B5/5K2 w - -");
	v = evaluate(position, eval);
	flip_board(position);
	ok &= (v == evaluate(position, eval));

	set_fen(position, "3K4/6p1/2P5/3P4/7k/8/1B4B1/5N2 w - -");
	v = evaluate(position, eval);
	flip_board(position);
	ok &= (v == evaluate(position, eval));

	set_fen(position, "8/6k1/2p5/7K/N7/2N5/PPPP1PPP/8 w - -");
	v = evaluate(position, eval);
	flip_board(position);
	ok &= (v == evaluate(position, eval));

	set_fen(position, "3b2k1/ppp4p/4P1p1/2n1p2b/2B4B/2PP4/PP2N1PP/4K3 w - -");
	v = evaluate(position, eval);
	flip_board(position);
	ok &= (v == evaluate(position, eval));

	set_fen(position, "5k2/p1pK3p/1p2Q1p1/5Pq1/4P3/8/P5P1/8 b - -");
	v = evaluate(position, eval);
	flip_board(position);
	ok &= (v == evaluate(position, eval));

	set_fen(position, "4rbk1/pp3ppp/2p5/q1n2P2/2PRn2P/2N2Q2/PP2B1P1/1K5R b - -");
	v = evaluate(position, eval);
	flip_board(position);
	ok &= (v == evaluate(position, eval));

	set_fen(position, "2kr1b1r/pppbqpp1/2n5/1B1pP2p/P2Pn3/5N2/1PN2PPP/R1BQ1RK1 b - -");
	v = evaluate(position, eval);
	flip_board(position);
	ok &= (v == evaluate(position, eval));

	set_fen(position, "r1bq1b1r/ppp3pp/4k3/3Np3/2B5/P4Q2/1P1P1PPP/n1BK3R b - -");
	v = evaluate(position, eval);
	flip_board(position);
	ok &= (v == evaluate(position, eval));	
	
	set_fen(position, "r2q1rk1/2pb1p1p/pp1bpPp1/4n1P1/1n2P3/2N1BN1Q/P1P4P/2KR1B1R b - -");
	v = evaluate(position, eval);
	flip_board(position);
	ok &= (v == evaluate(position, eval));		
	
	set_fen(position, "8/6pp/6p1/Pp6/1P6/1PK5/4P1k1/8 w - -");
	v = evaluate(position, eval);
	flip_board(position);
	ok &= (v == evaluate(position, eval));	

	set_fen(position, "8/2k5/1pP5/1P6/K7/8/8/8 w - -");
	v = evaluate(position, eval);
	flip_board(position);
	ok &= (v == evaluate(position, eval));

	set_fen(position, "5k2/8/8/7P/8/7K/8/8 b - -");
	v = evaluate(position, eval);
	flip_board(position);
	ok &= (v == evaluate(position, eval));

	set_fen(position, "6k1/8/7K/7P/8/8/8/8 b - -");
	v = evaluate(position, eval);
	flip_board(position);
	ok &= (v == evaluate(position, eval));

	set_fen(position, "8/6k1/8/3P4/1K6/8/8/8 b - -");
	v = evaluate(position, eval);
	flip_board(position);
	ok &= (v == evaluate(position, eval));

	set_fen(position, "2k5/2P1K3/1p6/1P6/8/8/8/8 b - -");
	v = evaluate(position, eval);
	flip_board(position);
	ok &= (v == evaluate(position, eval));

	set_fen(position, "r2r2k1/ppp4p/4p1p1/2B1Nb2/1PN2P2/3P2P1/P1P4P/5K2 w - -");
	v = evaluate(position, eval);
	flip_board(position);
	ok &= (v == evaluate(position, eval));

	set_fen(position, "1NQ5/k1p1p3/7p/pP2P1P1/2P5/2pq4/1n6/6K1 w - -");
	v = evaluate(position, eval);
	flip_board(position);
	ok &= (v == evaluate(position, eval));

	set_fen(position, "r1b1k3/1pq2pp1/p3p3/4p1Q1/3N2p1/1NP3Pr/PPP3Pb/R1BR3K w q -");
	v = evaluate(position, eval);
	flip_board(position);
	ok &= (v == evaluate(position, eval));

	set_fen(position, "7k/6p1/K3p3/2P5/1P3r2/8/2R5/8 w - -");
	v = evaluate(position, eval);
	flip_board(position);
	ok &= (v == evaluate(position, eval));

	return ok;

}

BOOL test_see() {

    t_move_record *move;
    BOOL ok = TRUE;

    set_fen(position, "2k1K3/7p/6p1/1p2RrR1/8/8/8/8 w - -");
    move = lookup_move(position, "e5b5");
    ok &= see(position, move, 0);

    move = lookup_move(position, "g5f5");
    ok &= see(position, move, 0);

    move = lookup_move(position, "g5g6");
    ok &= !see(position, move, 0);

    flip_board(position);

    move = lookup_move(position, "e4b4");
    ok &= see(position, move, 0);

    move = lookup_move(position, "g4f4");
    ok &= see(position, move, 0);

    move = lookup_move(position, "g4g3");
    ok &= !see(position, move, 0);

    set_fen(position, "k7/4n3/5n2/3p4/8/2N5/6B1/K6Q w - -");
    move = lookup_move(position, "c3d5");
    ok &= see(position, move, 0);

    set_fen(position, "k7/4n3/5n2/3p4/8/2N5/6B1/K7 w - -");
    move = lookup_move(position, "c3d5");
    ok &= !see(position, move, 0);

    set_fen(position, "3k4/b7/1q6/8/3pK1RR/8/2N5/8 w - -");
    move = lookup_move(position, "c2d4");
    ok &= !see(position, move, 0);

    set_fen(position, "3k4/8/1q6/8/3p2R1/7K/2N5/8 w - -");
    move = lookup_move(position, "c2d4");
    ok &= see(position, move, 0);

    set_fen(position, "Q7/p2k4/2pq4/3B4/8/8/6PP/n2Kb3 w - -");
    move = lookup_move(position, "a8c6");
    ok &= !see(position, move, 0);

	////-- Test SEE on a square
	//set_fen(position, "4k3/1ppn4/p5rr/1B3p2/2p1p3/1b3P2/PPP1P1BR/3K3R w - -");
	//ok &= (see_square(position, H6, 0) == TRUE);
	//ok &= (see_square(position, E4, 0) == TRUE);
	//ok &= (see_square(position, A6, 0) == FALSE);
	//ok &= (see_square(position, C4, 0) == FALSE);
	//ok &= (see_square(position, B3, 0) == TRUE);
	//ok &= (see_square(position, F5, 0) == FALSE);


    return ok;
}

BOOL test_position()
{
	uci_position(position, "position startpos moves d2d4 g7g6 g1f3 g8f6 c2c4 f8g7 b1c3 d7d5 d1b3 d5c4 b3c4 e8g8 e2e4 a7a6 e4e5 b7b5 c4b3 f6d7 e5e6 f7e6 f3g5 d7b6 g5e6 c8e6 b3e6 g8h8 c1e3 d8d6 e6d6 e7d6");

    return TRUE;
}

BOOL test_ep_capture()
{
	uci_set_mode();
	uci_isready();

	set_hash(512);
	set_own_book(TRUE);

	uci_position(position, "position fen 2r2bk1/1pNb4/3p3q/p2Pp2n/2P1PpPP/5P2/PPn1Q3/2KR3R b - g3");

	struct t_move_list moves[1];
	t_undo undo[1];

	generate_moves(position, moves);
	for (int i = 0; i < moves->count; i++){
		if (make_move(position, moves->pinned_pieces, moves->move[i], undo))
			unmake_move(position, undo);
	}

	return TRUE;
}

BOOL test_search()
{

    uci_set_mode();
    uci_isready();
	
	set_hash(512);
	set_own_book(TRUE);

	uci_new_game(position);

	uci_position(position, "position fen rbbqnknr/pppppppp/8/8/8/8/PPPPPPPP/RBBQNKNR w HAha - moves c2c4 c7c5 g1f3 e7e5 b1e4 e8d6 d2d3 d6e4 d3e4 g8e7 c1e3 b7b6 d1d3 c8b7 a1d1 b7c6 g2g4 f7f6 h1g1 h7h5 e3d2 h5g4 g1g4 d8c8 g4g2 c8a6 a2a3 b6b5 c4b5 c6b5 d3e3 d7d6 d1c1 b8c7 h2h4 a6b7 b2b4 c7b6 a3a4 b5a4 b4c5 b6c5 e3d3 a4b5 d3c2 a7a5 d2c3 a8c8 c2b2 b5c6");
	uci_go("go depth 6");

	while (uci.engine_state != UCI_ENGINE_WAITING)
		Sleep(1);

	uci_new_game(position);

	uci_setoption("setoption name UCI_Chess960 value true");

	uci_position(position, "position fen qrknrnbb/pppppppp/8/8/8/8/PPPPPPPP/QRKNRNBB w EBeb -");
	uci_go("go depth 6");

	while (uci.engine_state != UCI_ENGINE_WAITING)
		Sleep(1);

	uci_new_game(position);
	uci_position(position, "position fen r2q1rk1/2pb1p1p/pp1bpPp1/4n1P1/1n2P3/2N1BN1Q/P1P4P/2KR1B1R b - - moves");
	uci_go("go depth 12");

	while (uci.engine_state != UCI_ENGINE_WAITING)
		Sleep(1);
	
	return TRUE;
}

void test_bench()
{

	uci_set_mode();
	uci_isready();

	set_hash(512);
	set_own_book(FALSE);

	uci_new_game(position);

	global_nodes = 0;

	t_chess_time start_time = time_now();

	uci_position(position, "position fen 8/8/8/4k2K/1R3p2/8/6r1/8 w - -");
	uci_go("go depth 20");

	while (uci.engine_state != UCI_ENGINE_WAITING)
		Sleep(1);
	global_nodes += nodes + qnodes;

	uci_position(position, "position fen 1rq5/p3kp2/2Bp1p2/1P2p1r1/QP3n2/2P5/5PPP/R4RK1 b - -");
	uci_go("go depth 12");

	while (uci.engine_state != UCI_ENGINE_WAITING)
		Sleep(1);
	global_nodes += nodes + qnodes;

	uci_position(position, "position fen 1NQ5/k1p1p3/7p/pP2P1P1/2P5/2pq4/1n6/6K1 w - -");
	uci_go("go depth 12");

	while (uci.engine_state != UCI_ENGINE_WAITING)
		Sleep(1);
	global_nodes += nodes + qnodes;

	uci_position(position, "position fen 2kr3r/pp1q1ppp/5n2/1Nb5/2Pp1B2/7Q/P4PPP/1R3RK1 w - -");
	uci_go("go depth 16");

	while (uci.engine_state != UCI_ENGINE_WAITING)
		Sleep(1);
	global_nodes += nodes + qnodes;

	uci_position(position, "position fen 8/5p2/pk2p3/4P2p/2b1pP1P/P3P2B/8/7K w - -");
	uci_go("go depth 24");

	while (uci.engine_state != UCI_ENGINE_WAITING)
		Sleep(1);
	global_nodes += nodes + qnodes;

	uci_position(position, "position fen 5rk1/2p4p/2p4r/3P4/4p1b1/1Q2NqPp/PP3P1K/R4R2 b - -");
	uci_go("go depth 16");

	while (uci.engine_state != UCI_ENGINE_WAITING)
		Sleep(1);
	global_nodes += nodes + qnodes;

	t_chess_time end_time = time_now();

	if (end_time > start_time)
		printf(INFO_STRING_PERFT_SPEED, global_nodes, end_time - start_time, 1000 * global_nodes / (end_time - start_time));
	
}

BOOL test_book()
{
    struct t_move_record *move;

    uci_set_mode();
    uci_isready();
    uci_position(position, "position startpos moves e2e4 c7c5 g1f3");

    search_start_time = time_now();

    for (int i = 0; i < 1000; i++)
        move = probe_book(position);

    search_start_time =  time_now() - search_start_time;

    char s[1024];
    sprintf(s, "Time = %d", search_start_time);

    send_info(s);
    return TRUE;
}

BOOL test_hash_table()
{
	t_move_list moves[1];
	t_hash_record *h;
	BOOL ok = TRUE;

	set_fen(position, "8/pppr2pp/3pKp2/2Q3bn/8/b6k/PPP1P2P/3R2n1 w - -");
	generate_captures(position, moves);

	poke(position->hash, 1, 2, 2, HASH_LOWER, moves->move[0]);

	h = probe(position->hash);

	if (h != NULL){
		ok &= h->bound == HASH_LOWER;
		ok &= h->depth == 2;
		ok &= h->score == 1;
		ok &= h->move == moves->move[0];
		ok &= h->key == position->hash;
	}

	return ok;
}
