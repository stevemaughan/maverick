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
#include <time.h>
#include <assert.h>

#include "defs.h"
#include "data.h"
#include "procs.h"
#include "bittwiddle.h"

HANDLE thread_handle;
unsigned threadID;

char input_string[UCI_BUFFER_SIZE];

unsigned __stdcall engine_loop(void* pArguments)
{
    uci.engine_state = UCI_ENGINE_WAITING;
    uci.stop = FALSE;
    while (!uci.quit) {
        if (uci.engine_state == UCI_ENGINE_START_THINKING){
            root_search(position);
            uci.stop = FALSE;
            uci.engine_state = UCI_ENGINE_WAITING;
        }
        else {
            Sleep(1);
        }
    }
    _endthreadex(0);
    return 0;
}

void create_uci_engine_thread()
{
    thread_handle = (HANDLE)_beginthreadex( NULL, 0, &engine_loop, NULL, 0, &threadID );
    SetThreadPriority(thread_handle, THREAD_PRIORITY_NORMAL); // needed for Fritz GUI! :-))
}

void listen_for_uci_input()
{

    char *p;

	//-- Create a log file if in debug mode
	if (uci.debug)
		write_log("Maverick's Log File", "maverick-log.txt", TRUE, TRUE);

    // Ensure that listening thread has been started
    while (!uci.quit) {
        fgets(input_string, sizeof(input_string), stdin);

        //-- Remove the "\n" character
        if ((p = strchr(input_string, '\n')) != NULL)
            *p = '\0';

		//-- Create a log file if in debug mode
		if (uci.debug)
			write_log(input_string, "maverick-log.txt", TRUE, FALSE);

		/*===============================================================*/
        /* QUIT Command
        /*===============================================================*/
        if (!strcmp(input_string, "quit") || !strcmp(input_string, "QUIT")) {
            uci_stop();
            uci.quit = TRUE;
        }
        /*===============================================================*/
        /* UCI Command
        /*===============================================================*/
        if (!strcmp(input_string, "uci") || !strcmp(input_string, "UCI"))
            uci_set_mode();
        /*===============================================================*/
        /* ISREADY Command
        /*===============================================================*/
        if (!strcmp(input_string, "isready") || !strcmp(input_string, "ISREADY"))
            uci_isready();
        /*===============================================================*/
        /* GO Command
        /*===============================================================*/
        if ((index_of("go", input_string)==0) || (index_of("GO", input_string)==0))
            uci_go(input_string);
        /*===============================================================*/
        /* Position Command
        /*===============================================================*/
        if ((index_of("position", input_string)==0) || (index_of("POSITION", input_string)==0))
            uci_position(position, input_string);
        /*===============================================================*/
        /* Stop Command
        /*===============================================================*/
        if ((index_of("stop", input_string)==0) || (index_of("STOP", input_string)==0))
            uci_stop();
        /*===============================================================*/
        /* Ponder Hit Command
        /*===============================================================*/
        if ((index_of("ponderhit", input_string)==0) || (index_of("PONDERHIT", input_string)==0))
            uci_ponderhit();
		/*===============================================================*/
		/* New Game
		/*===============================================================*/
		if ((index_of("ucinewgame", input_string) == 0) || (index_of("UCINEWGAME", input_string) == 0))
			uci_new_game(position);
		/*===============================================================*/
        /* Set Options Command
        /*===============================================================*/
        if ((index_of("setoption", input_string)==0) || (index_of("SETOPTION", input_string)==0)) {
            uci_setoption(input_string);
        }
		/*===============================================================*/
		/* Set Options Command
		/*===============================================================*/
		if ((index_of("debug", input_string) == 0) || (index_of("DEBUG", input_string) == 0)) {
			uci_set_debug(input_string);
		}

        /*===============================================================*/
        /* TEST Command
        /*===============================================================*/
        if (!strcmp(input_string, "test") || !strcmp(input_string, "TEST"))
            test_procedure();
        if (!strcmp(input_string, "testperft") || !strcmp(input_string, "TESTPERFT"))
            test_perft();
        if (!strcmp(input_string, "testbook") || !strcmp(input_string, "TESTBOOK"))
            test_book();

    }
    WaitForSingleObject(thread_handle, INFINITE);
    CloseHandle(thread_handle);
}

void send_command(char *t)
{
    size_t i = strlen(t);
    if (i > 0)
    {
        if (t[i-1] == '\n')
            t[i-1] = '\0';
        puts(t);
        fflush(stdout);
		if (uci.debug)
			write_log(t, "maverick-log.txt", TRUE, TRUE);

    }
}

void uci_set_author()
{
    sprintf(engine_name, "Maverick %s", ENGINE_VERSION);
    strcpy(engine_author, "Steve Maughan");
}

void uci_set_mode()
{
    char s[1024];

    // Send the UCI Options
    strcpy(s,"id name ");
    strcat(s, engine_name);
    send_command(s);

    strcpy(s,"id author ");
    strcat(s, engine_author);
    send_command(s);

    strcpy(s,"option name Hash type spin default 64 min 2 max 1024");
    uci.options.hash_table_size = 2;
    send_command(s);

    strcpy(s,"option name Ponder type check default true");
    send_command(s);

    if (book_count() > 0) {
        strcpy(s, "option name OwnBook type check default true");
        send_command(s);
        strcpy(s, book_string());
        send_command(s);
    }
    else {
        uci.opening_book.use_own_book = FALSE;
        strcpy(uci.opening_book.filename, "");
        uci.opening_book.f = NULL;
    }

    strcpy(s,"option name UCI_ShowCurrLine type check default false");
     send_command(s);
    uci.options.current_line = FALSE;

    strcpy(s,"option name EvalTest type check default false");
    uci.options.eval_test = FALSE;
    send_command(s);

    sprintf(s, "option name UCI_EngineAbout type string default Maverick (v%s) by Steve Maughan www.chessprogramming.net", ENGINE_VERSION);
    send_command(s);

	strcpy(s, "option name Show Search Statistics type check default true");
	uci.options.show_search_statistics = TRUE;
	send_command(s);

	strcpy(s, "option name Smart Book type check default false");
	uci.options.smart_book = FALSE;
	send_command(s);

	strcpy(s, "uciok");
    send_command(s);
}

void uci_isready()
{
    char s[1024];

    if (!uci.engine_initialized)
        init_engine(position);

    strcpy(s, "readyok");
    send_command(s);
}

void uci_stop()
{
    if (uci.engine_state != UCI_ENGINE_THINKING) {
        send_info("ERROR - I can't stop because I'm not thinking!");
		uci_send_state("After Stop");
    }
    uci.stop = TRUE;
    while (uci.engine_state != UCI_ENGINE_WAITING)
        Sleep(1);
}

void uci_go(char *s)
{
	while (uci.engine_state != UCI_ENGINE_WAITING)
		Sleep(1);
    search_start_time = time_now();
    last_display_update = search_start_time;
    set_uci_level(s, position->to_move);
    uci.engine_state = UCI_ENGINE_START_THINKING;
    while (uci.engine_state == UCI_ENGINE_START_THINKING)
        Sleep(1);
}

void uci_position(struct t_board *board, char *s)
{
	while (uci.engine_state != UCI_ENGINE_WAITING)
		Sleep(1);

	static char str[UCI_BUFFER_SIZE];
    int i, n, c;

    if (!strcmp(word_index(1, s), "startpos") || !strcmp(word_index(1, s), "STARTPOS")) {
        new_game(board);
    }
    else {
        strcpy(str,word_index(2,s));
        strcat(str," ");
        strcat(str,word_index(3,s));
        strcat(str," ");
        strcat(str,word_index(4,s));
        strcat(str," ");
        strcat(str,word_index(5,s));
        set_fen(board, str);
    }
    n = index_of("moves", s);
    if (n <= -1)
        n = index_of("MOVES", s);
    if (n > 0) {
        c = word_count(s);
        for (i = n + 1; i < c; i++) {
            make_game_move(board, word_index(i, s));
        }
    }
}

void uci_ponderhit()
{
	if (uci.level.ponder == FALSE){
		send_info("ERROR - I'm not pondering!");
		uci_send_state("After False PonderHit");
	}
	uci.level.ponder = FALSE;
}

void uci_check_status(struct t_board *board, int ply)
{
    unsigned long t1;
    t1 = time_now();
	if ((!uci.level.ponder) && (!uci.level.infinite) && (!uci.level.depth) && (!uci.level.mate) && (!uci.level.nodes) && (t1 - search_start_time >= abort_move_time)){
		static char s[1024];
		sprintf(s, "Abort **NOW** : Abort Time = %d, Search Time = %d, Nodes = %I64d", abort_move_time, t1 - search_start_time, nodes + qnodes);
		send_info(s);
		uci.stop = TRUE;
	}
    else {
        message_update_count++;
        if (t1 - last_display_update > 1000) {
            //Sleep(0);
            do_uci_hash_full();
            do_uci_send_nodes();
            if (uci.options.current_line)
            	uci_current_line(board, ply);
            last_display_update = t1;
			if (message_update_count > 200){
				message_update_mask = (message_update_mask << 1) + 1;
			}
			else {
				if (message_update_count < 100)
					message_update_mask = (message_update_mask >> 1);
			}
            message_update_count = 0;
        }
    }
}

/*=======================================================*/
/*	UCI Options Management
/*=======================================================*/
void uci_setoption(char *s)
{

    if ((index_of("Hash", s) == 2) || (index_of("hash", s) == 2) || (index_of("HASH", s) == 2)) {
        set_hash(number_index(4, s));
		return;
    }

    if ((index_of("OwnBook", s) == 2) || (index_of("ownbook", s) == 2) || (index_of("OWNBOOK", s) == 2)) {
        if (!strcmp(word_index(4, s), "true") || !strcmp(word_index(4, s), "TRUE"))
            set_own_book(TRUE);
        else
            set_own_book(FALSE);
		return;
    }

	if (((index_of("Opening", s) == 2) || (index_of("opening", s) == 2) || (index_of("OPENING", s) == 2)) && ((index_of("Book", s) == 3) || (index_of("book", s) == 3) || (index_of("BOOK", s) == 3))) {
		set_opening_book(leftstr(s, 5));
		return;
    }

	if ((index_of("UCI_ShowCurrLine", s) == 2) || (index_of("UCI_SHOWCURRLINE", s) == 2) || (index_of("uci_showcurrline", s) == 2)){
        if (!strcmp(word_index(4,s),"true") || !strcmp(word_index(4,s),"TRUE"))
        	uci.options.current_line = TRUE;
        else
        	uci.options.current_line = FALSE;
		return;
    }

	if ((index_of("EvalTest", s) == 2) || (index_of("EVALTEST", s) == 2) || (index_of("evaltest", s) == 2)){
		if (!strcmp(word_index(4, s), "true") || !strcmp(word_index(4, s), "TRUE"))
			uci.options.eval_test = TRUE;
		else
			uci.options.eval_test = FALSE;
		return;
	}

	if((index_of("Statistics", s) == 4) || (index_of("statistics", s) == 4) || (index_of("STATISTICS", s) == 4)){
        if (!strcmp(word_index(6, s), "true") || !strcmp(word_index(6, s), "TRUE"))
        	uci.options.show_search_statistics = TRUE;
        else
        	uci.options.show_search_statistics = FALSE;
		return;
    }

	if ((index_of("Smart", s) == 2) || (index_of("smart", s) == 2) || (index_of("SMART", s) == 2)){
		if (!strcmp(word_index(4, s), "true") || !strcmp(word_index(4, s), "TRUE"))
			uci.options.smart_book = TRUE;
		else
			uci.options.smart_book = FALSE;
		return;
	}


}

/*=======================================================*/
/*	UCI Events
/*=======================================================*/
void do_uci_new_pv(struct t_board *board, int score, int depth)
{
	//-- Don' waist bandwidth
		if (depth < 2)
			return;

    static char pv[2048];
    static char s[2048];

    int i, v;
    unsigned long t;

    t = time_now() - search_start_time;

    if (score >= MAX_CHECKMATE) {
        v = ((CHECKMATE - score + 1) >> 1);
        sprintf(s, "info score mate %d time %ld depth %d seldepth %d nodes %I64d pv ", v, t, depth, deepest, nodes + qnodes);
    }
    else if (score <= -MAX_CHECKMATE) {
        v = ((-score - CHECKMATE) >> 1);
        sprintf(s, "info score mate %d time %ld depth %d seldepth %d nodes %I64d pv ", v, t, depth, deepest, nodes + qnodes);
    }
    else {
        sprintf(s, "info score cp %d time %ld depth %d seldepth %d nodes %I64d pv ", score, t, depth, deepest, nodes + qnodes);
    }

    pv[0] = 0;
    for (i = 0; i < board->pv_data[0].best_line_length; i++) {
        if (i>0)
            strcat(pv," ");
        strcat(pv, move_as_str(board->pv_data[0].best_line[i]));
    }

    strcat(s, pv);
    send_command(s);
    return;
}

void do_uci_fail_high(struct t_board *board, int score, int depth)
{
	//-- Don' waist bandwidth
	if (depth < 4)
		return;

    static char pv[2048];
    static char s[2048];

    int v;
    unsigned long t;

	assert(score > -CHECKMATE && score < CHECKMATE);

    t = time_now() - search_start_time;

    if (score >= MAX_CHECKMATE){
    	v = ((CHECKMATE - score + 1) >> 1);
    	sprintf(s, "info score mate %d lowerbound time %ld depth %d seldepth %d nodes %I64d pv ", v, t, depth, deepest, nodes + qnodes);
    }
    else if (score <= -MAX_CHECKMATE){
    	v = ((-score - CHECKMATE) >> 1);
    	sprintf(s, "info score mate %d lowerbound time %ld depth %d seldepth %d nodes %I64d pv ", v, t, depth, deepest, nodes + qnodes);
    }
    else{
    	sprintf(s, "info score cp %d lowerbound time %ld depth %d seldepth %d nodes %I64d pv ", score, t, depth, deepest, nodes + qnodes);
    }
    strcpy(pv,move_as_str(board->pv_data[0].current_move));
    strcat(s,pv);
    send_command(s);
}

void do_uci_fail_low(struct t_board *board, int score, int depth)
{
    static char pv[2048];
    static char s[2048];

    int v;
    unsigned long t;

    if (depth < 3 || uci.stop)  return;

    t = time_now() - search_start_time;

    if (score >= MAX_CHECKMATE){
    	v = ((CHECKMATE - score + 1) >> 1);
    	sprintf(s, "info score mate %d upperbound time %ld depth %d seldepth %d nodes %I64d pv ", v, t, depth, deepest, nodes + qnodes);
    }
    else if (score <= -MAX_CHECKMATE){
    	v = ((-score - CHECKMATE) >> 1);
    	sprintf(s, "info score mate %d upperbound time %ld depth %d seldepth %d nodes %I64d pv ", v, t, depth, deepest, nodes + qnodes);
    }
    else{
    	sprintf(s, "info score cp %d upperbound time %ld depth %d seldepth %d nodes %I64d pv ", score, t, depth, deepest, nodes + qnodes);
    }
    strcpy(pv,move_as_str(board->pv_data[0].current_move));
    strcat(s,pv);
    send_command(s);
}

void do_uci_send_nodes()
{
    static char s[64];

    t_nodes n;
    unsigned long t;

    n = nodes + qnodes;
    t = time_now();
    if (t > search_start_time)
        sprintf(s,"info nodes %I64d nps %I64d\n", n, 1000 * n / (t - search_start_time));
    else
        sprintf(s,"info nodes %I64d nps %I64d\n", n, (long long) 0);
    send_command(s);
}

void do_uci_consider_move(struct t_board *board, int depth)
{
    static char s[64];
    unsigned long t1;

    t1 = time_now();
    if (t1 - 300 > search_start_time) {
        sprintf(s,"info currmove %s currmovenumber %d depth %d seldepth %d\n", move_as_str(board->pv_data[0].current_move), board->pv_data[0].legal_moves_played, depth, deepest);
        send_command(s);
    }
}

void do_uci_hash_full()
{
    static char s[64];

    sprintf(s,"info hashfull %I64d\n", (1000 * hash_full) / (hash_mask + HASH_ATTEMPTS));
    send_command(s);
}

void do_uci_bestmove(struct t_board *board)
{
    static char s[64];

    strcpy(s, "bestmove ");
    strcat(s, move_as_str(board->pv_data[0].best_line[0]));
    if (board->pv_data[0].best_line_length > 1) {
        strcat(s, " ponder ");
        strcat(s, move_as_str(board->pv_data[0].best_line[1]));
    }
    send_command(s);
}

void do_uci_depth()
{
    static char s[64];

    sprintf(s,"info depth %d seldepth %d\0", search_ply, deepest);
    send_command(s);
}

void uci_current_line(struct t_board *board, int ply)
{
    static char s[1024];
    int i;

    strcpy(s, "info currline");
    for(i = 0; i < ply; i++){
    	strcat(s, " ");
    	strcat(s, move_as_str(board->pv_data[i].current_move));
    }
    send_command(s);
}

BOOL is_search_complete(struct t_board *board, int score, int ply, struct t_move_list *move_list)
{
    int s;

    //-- Maximum search depth
    if (ply >= MAXPLY)
        return TRUE;

    //-- Found a Mate */
    if (score >= MAX_CHECKMATE) {
        s = (CHECKMATE - score);
        if (s <= ply)
            return TRUE;
    }
    if (score <= -MAX_CHECKMATE) {
        s = (score + CHECKMATE);
        if (s <= ply)
            return TRUE;
    }

    if ((uci.level.infinite) || (uci.level.ponder))
        return FALSE;

    if (uci.level.depth > 0)
        return (uci.level.depth <= ply);

    if (uci.level.nodes > 0)
        return (uci.level.nodes < nodes + qnodes);

    if (uci.level.mate > 0) {
        if (score >= MAX_CHECKMATE && ((CHECKMATE - score + 1) >> 1) <= uci.level.mate)
            return TRUE;
        else
            return FALSE;
    }

    unsigned long t1 = time_now();
    if (uci.level.movetime > 0) {
        if (search_start_time + uci.level.movetime <= t1)
            return TRUE;
        else
            return FALSE;
    }

    if (move_list->count == 1)
    {
        if (early_move_time <= t1 - search_start_time)
            return TRUE;
    };

    /* Normal Move */
	if (target_move_time < (t1 - search_start_time) * 1.5 || abort_move_time < (t1 - search_start_time) * 2.0)
        return TRUE;

    return FALSE;
}

void set_uci_time_to_move(t_chess_color color)
{

    static char s[2048];
	long lag = 0;
	t_chess_color opponent = OPPONENT(color);

    /* Infinity */
    if (uci.level.infinite || uci.level.mate || uci.level.nodes) {
        early_move_time = 1;
        target_move_time = 1;
        abort_move_time = 1;
        return;
    }

    /* Fixed Time per Move */
    if (uci.level.movetime > 0) {
        early_move_time = uci.level.movetime * 10;
        target_move_time = uci.level.movetime * 10;
        abort_move_time = uci.level.movetime;
        return;
    }

	int x_togo;

    /* Set time for x number of moves */
    if (uci.level.movestogo > 0) {
		x_togo = uci.level.movestogo;
        target_move_time = (uci.level.time[color] - lag * x_togo) / (x_togo + 1);
		if (target_move_time < lag)
			target_move_time = lag;

		//-- Modified Logistic Formula
		double delta = 3;
		double half = 10;
		double a = (log((double) delta - (double) 1) - log((double) 1)) / (half - (double) 1);
		double b = -half * a - log((double) 1);
		double m = delta / (1 + exp(-((double) x_togo * a + b)));
		abort_move_time = int(target_move_time * m);

	}
	else{
		/* Game in x + inc */
		if (uci.level.movestogo == 0)
			x_togo = (35 - 5 * uci.level.ponder);
		else
			x_togo = 35;
		target_move_time = uci.level.tinc[color] + (uci.level.time[color] - uci.level.tinc[color] - lag * x_togo) / x_togo;
		if (target_move_time < lag)
			target_move_time = lag;

		//-- Find a suitable abort time
		abort_move_time = target_move_time + (uci.level.time[color] - uci.level.tinc[color]) / 10;
		if (abort_move_time < lag)
			abort_move_time = lag;
	}


	//-- Check to make sure this makes sense
	if (uci.level.time[color] - abort_move_time < 100)
		abort_move_time = target_move_time;

	//-- Early move time is when there is only one move
	early_move_time = (t_chess_time) (target_move_time / 5);


	sprintf(s, "info string Move = %d in Time = %d: Early = %d, Target = %d, Abort = %d", uci.level.movestogo, uci.level.time[color], early_move_time, target_move_time, abort_move_time);
    send_command(s);
}

void set_uci_level(char *s, t_chess_color color)
{
    int i;
    double d;
	t_chess_color opponent = OPPONENT(color);

    uci.level.tinc[WHITE] = 0;
    uci.level.tinc[BLACK] = 0;
    uci.level.time[WHITE] = 0;
    uci.level.time[BLACK] = 0;
    uci.level.depth = 0;
    uci.level.infinite = 0;
    uci.level.mate = 0;
    uci.level.movestogo = 0;
    uci.level.movetime = 0;
    uci.level.nodes = 0;
    uci.level.ponder = FALSE;

    i = index_of("ponder", s);
    if (i>=0)
        uci.level.ponder = TRUE;
    i = index_of("PONDER", s);
    if (i>=0)
        uci.level.ponder = TRUE;

    i = index_of("wtime",s);
    if (i>=0)
        uci.level.time[WHITE] = number_index(i + 1, s);
    i = index_of("WTIME",s);
    if (i>=0)
        uci.level.time[WHITE] = number_index(i + 1, s);
    i = index_of("winc",s);
    if (i>=0)
        uci.level.tinc[WHITE] = number_index(i + 1, s);
    i = index_of("WINC",s);
    if (i>=0)
        uci.level.tinc[WHITE] = number_index(i + 1, s);

    i = index_of("btime",s);
    if (i>=0)
        uci.level.time[BLACK] = number_index(i + 1, s);
    i = index_of("BTIME",s);
    if (i>=0)
        uci.level.time[BLACK] = number_index(i + 1, s);
    i = index_of("binc",s);
    if (i>=0)
        uci.level.tinc[BLACK] = number_index(i + 1, s);
    i = index_of("BINC",s);
    if (i>=0)
        uci.level.tinc[BLACK] = number_index(i + 1, s);

    i = index_of("movestogo",s);
    if (i>=0)
        uci.level.movestogo = number_index(i + 1, s);
    i = index_of("MOVESTOGO",s);
    if (i>=0)
        uci.level.movestogo = number_index(i + 1, s);

    i = index_of("depth",s);
    if (i >= 0)
        uci.level.depth = number_index(i + 1, s);
    i = index_of("DEPTH",s);
    if (i >= 0)
        uci.level.depth = number_index(i + 1, s);

	//-- Fixed 5 ply test mode
	if (uci.options.eval_test){
		uci.level.depth = 5;
	}

    i = index_of("nodes",s);
    if (i>=0)
        uci.level.nodes = number_index(i + 1, s);
    i = index_of("NODES",s);
    if (i>=0)
        uci.level.nodes = number_index(i + 1, s);

    i = index_of("mate",s);
    if (i>=0)
        uci.level.mate = number_index(i + 1, s);
    i = index_of("MATE",s);
    if (i>=0)
        uci.level.mate = number_index(i + 1, s);

    i = index_of("movetime",s);
    if (i>=0)
        uci.level.movetime = number_index(i + 1, s);
    i = index_of("MOVETIME",s);
    if (i>=0)
        uci.level.movetime = number_index(i + 1, s);

    i = index_of("infinite", s);
    if (i>=0)
        uci.level.infinite = TRUE;
    i = index_of("INFINITE", s);
    if (i>=0)
        uci.level.infinite = TRUE;

    set_uci_time_to_move(color);

}

void uci_send_state(char *c)
{
    static char s[2048];
    static char t[2048];
    static char st[1024];

    switch (uci.engine_state) {
    case UCI_ENGINE_WAITING:
        strcpy(s, "Waiting");
        break;
	case UCI_ENGINE_THINKING:
		strcpy(s, "Thinking");
		break;
	case UCI_ENGINE_START_THINKING:
		strcpy(s, "Start Thinking");
		break;
	}

    if (uci.stop)
        strcpy(st, "TRUE");
    else
        strcpy(st, "FALSE");


    sprintf(t, "info string %s Engine State = %s, STOP = %s\n",c, s, st);
    send_command(t);
}

void do_uci_show_stats()
{
    if (uci.options.show_search_statistics){

		static char s[2048];
		static char t[2048];
		double n, f = 0, h = 0;

    	/* nodes */
    	n = 100 * (double)qnodes / (qnodes + nodes);

    	/* hash performance */
    	strcpy(s, "info string ");
    	if (hash_probes){
    		h = (double)hash_hits;
    		h = (100 * h / hash_probes );
    	}

    	/* move order */
    	if (cutoffs){
    		f = first_move_cutoffs;
    		f = (100 * f / cutoffs );
    	}
		sprintf(s, "info string QNodes = %3.1f%%, Hash Hits = %3.1f%%, Move Order = %3.1f%%\n", n, h, f);
		send_command(s);
    }
}

void send_info(char *s)
{
    static char t[2048];
    strcpy(t, "info string ");
    strncat(t, s, sizeof t - 12);
    send_command(t);
}

void uci_new_game(struct t_board *board)
{
	if (uci.engine_state != UCI_ENGINE_WAITING)
		uci_stop();

    clear_hash();
    //clear_pv(board);
    clear_history();
	//write_log("", "maverick-log.txt", FALSE, FALSE);
}

void uci_set_debug(char *s)
{
	if (index_of("on", s) >= 0 || index_of("ON", s) >= 0)
		uci.debug = TRUE;
	else
		uci.debug = FALSE;
}

//void uci_set_predicted_hash(struct t_board *board)
//{
//	int i, n, q;
//	struct t_undo					undo[2];
//	struct t_move_list		move_list[1];
//
//	uci.thinking.predicted_hash = 0;
//	uci.thinking.obvious_move.move = 0;
//
//	if (board->pv[0].best_line_length >= 3){
//		for(i = 0; i < 2; i++){
//			make_move(board, board->pv[0].best_line[i], &(undo[i]));
//			assert(board_integrity(board));
//		}
//		uci.thinking.predicted_hash = board->hash;
//		if (board->pv[0].best_line[1].captured && board->pv[0].best_line[1].to == board->pv[0].best_line[2].to){
//			gen_legal_moves(board, move_list);
//			n = 0;
//			q = 0;
//			for(i = 0; i < move_list->count; i++){
//				if (move_list->move[i].to == board->pv[0].best_line[1].to)
//				{
//					if (see_positive(board, move_list->move[i])){
//						n++;
//						if (move_list->move[i].move == board->pv[0].best_line[2].move)
//							q++;
//					}
//				}
//			}
//			if (n == 1 && q == 1)
//				uci.thinking.obvious_move.move = board->pv[0].best_line[2].move;
//		}
//		for(i = 1; i >= 0; i--){
//			undo_move(board, &(undo[i]));
//			assert(board_integrity(board));
//		};
//	}
//	uci.thinking.predicted_time = uci.level.time[board->to_move] - (time_now() - search_start_time) + uci.level.tinc[board->to_move];
//};

void init_engine(struct t_board *board)
{
    if (!uci.engine_initialized) {
		hash_age = 1;

		srand(time(NULL));

		message_update_mask = 32767;

		uci.debug = FALSE;
        //initialize stuff
        init_eval_function();
        init_board(board);
        init_hash();
        init_pawn_hash();
        init_bitboards();
        init_move_directory();
        init_magic();
        init_can_move();
		init_material_hash();
        uci.engine_initialized = TRUE;
    }
};
