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

void write_board(struct t_board *board, char filename[1024])
{
    FILE *tfile = NULL;
    int i, r, c;
    char s[1000];

    r = 7;
    c = 0;
    i = 1;
    tfile = fopen(filename, "w");
    while (i >= 0)
    {
        i = (8 * r) + c;
        switch (board->square[i])
        {
        case BLANK :
            fprintf(tfile,". ");
            break;
        case WHITEPAWN :
            fprintf(tfile,"P ");
            break;
        case WHITEKNIGHT :
            fprintf(tfile,"N ");
            break;
        case WHITEBISHOP :
            fprintf(tfile,"B ");
            break;
        case WHITEROOK :
            fprintf(tfile,"R ");
            break;
        case WHITEQUEEN :
            fprintf(tfile,"Q ");
            break;
        case WHITEKING :
            fprintf(tfile,"K ");
            break;
        case BLACKPAWN :
            fprintf(tfile,"p ");
            break;
        case BLACKKNIGHT :
            fprintf(tfile,"n ");
            break;
        case BLACKBISHOP :
            fprintf(tfile,"b ");
            break;
        case BLACKROOK :
            fprintf(tfile,"r ");
            break;
        case BLACKQUEEN :
            fprintf(tfile,"q ");
            break;
        case BLACKKING :
            fprintf(tfile,"k ");
            break;
        }
        c++;
        if (c > 7)
        {
            c = 0;
            r--;
            i = (8 * r) + c;
            fprintf(tfile,"\n");
        }
    }
    //-- FEN
    strcpy(s, get_fen(board));
    fprintf(tfile, s);
    fprintf(tfile, "\n");
    //-- To Move
    if (board->to_move == WHITE)
        strcpy(s, "White to move");
    else
        strcpy(s, "Black to move");
    fprintf(tfile, s);
    fprintf(tfile, "\n");
    //-- In Check
    if (board->in_check) {
        strcpy(s, "In check!");
        fprintf(tfile, s);
        fprintf(tfile, "\n");
    }

    //-- Board Hash
    sprintf(s, "Hash = %llu \n", board->hash);
    fprintf(tfile, s);

    fclose(tfile);
}

void write_move_list(struct t_move_list *move_list, char filename[1024])
{
    FILE *tfile = NULL;
    int i;
    char s[50];

    tfile = fopen(filename, "w");
    for (i = 0; i < move_list->count; i++) {
        sprintf(s, "%d. %s = %lld", i, move_as_str(move_list->move[i]), move_list->value[i]);
        fprintf(tfile, s);
        //fprintf(tfile," ");
        //fprintf(tfile,"%I64d", move_list->value[i]);
        fprintf(tfile,"\n");
    }
    fclose(tfile);
}

void write_path(struct t_board *board, int ply, char filename[1024]) {
    FILE *tfile = NULL;
    int i;
    char s[10];

    tfile = fopen(filename, "w");

    for (i = 0; i < ply; i++) {
        strcpy(s, move_as_str(board->pv_data[i].current_move));
        fprintf(tfile, s);
        fprintf(tfile, "\n");
    }

    fclose(tfile);
}

void write_tree(struct t_board *board, struct t_move_record *move, BOOL append, char filename[1024]) {

    FILE *tfile = NULL;
    static int i;
    char s[UCI_BUFFER_SIZE];

    if (append) {
        tfile = fopen(filename, "a");
        sprintf(s, "%d, %s, ""%llu"", %s", ++i, get_fen(board), board->hash, move_as_str(move));
        fprintf(tfile, s);
        fprintf(tfile, "\n");
    }
    else {
        i = 0;
        tfile = fopen(filename, "w");
        strcpy(s, "Maverick Tree File");
        fprintf(tfile, s);
        fprintf(tfile, "\n");
    }

    fclose(tfile);
}

void write_log(char *s, char *filename, BOOL append, BOOL send)
{
    FILE *tfile = NULL;

    char t[UCI_BUFFER_SIZE];

    if (append) {
        tfile = fopen(filename, "a");
    }
    else {
        tfile = fopen(filename, "w");
    }

    if (send)
        sprintf(t, "Sent     >> %s", s);
    else
        sprintf(t, "Received >> %s", s);
    fprintf(tfile, t);
    fprintf(tfile, "\n");
    fclose(tfile);
}
