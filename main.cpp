//===========================================================//
//
// Maverick Chess Engine
// Copyright 2013-2015 Steve Maughan
//
// Started March 1st 2013
//
//===========================================================//

#include <stdio.h>
#include <stdlib.h>
#include <process.h>

#include "defs.h"
#include "data.h"
#include "procs.h"
#include "bittwiddle.h"



//============================================================//
// To Do...
//
// Counter Move for move ordering

int main(int argc, char *argv[])
{
    setbuf(stdout, NULL);
    setbuf(stdin, NULL);
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stdin, NULL, _IONBF, 0);

	uci.engine_initialized = FALSE;
    uci.quit = FALSE;
	uci.stop = FALSE;

    init_engine(position);
	set_fen(position, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -");

    create_uci_engine_thread();

    uci_set_author();
    listen_for_uci_input();

    destroy_pawn_hash();
	destroy_material_hash();
	destroy_hash();

    close_book();

    return TRUE;
}
