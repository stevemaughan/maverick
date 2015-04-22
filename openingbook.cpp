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
#include <shlwapi.h>
#else
#include <unistd.h>
#endif

#include <iostream>

#include "defs.h"
#include "data.h"
#include "procs.h"


#if defined(__arm__) || defined(__linux__) 

void close_book()
{

}

void set_own_book(BOOL value)
{

}

void set_opening_book(char *book)
{


}

int book_count()
{

}

char *book_string()
{


}

unsigned long long read_integer(FILE *file, int size)
{


}

void read_book_move(int index, struct t_book_move *book_move)
{

}

struct t_move_record *decode_move(struct t_board *board, unsigned int move)
{

}

t_move_record *probe_book(struct t_board *board)
{


}

int book_move_count(struct t_board *board){


}

void fill_opening_move_list(struct t_board *board, struct t_move_list *move_list){


}

#else

void close_book()
{
    if (uci.opening_book.f != NULL)
        fclose(uci.opening_book.f);
}

void set_own_book(BOOL value)
{
    uci.opening_book.use_own_book = value;
}

void set_opening_book(char *book)
{
	TCHAR file_path[2048] = { 0 };
	TCHAR file_to_open[2048] = { 0 };

	GetModuleFileName(0, file_path, 2048);
	PathRemoveFileSpec(file_path);
	PathAddBackslash(file_path);
	strcpy(file_to_open, file_path);

	strncpy(uci.opening_book.filename, book, sizeof uci.opening_book.filename);
	strtok(uci.opening_book.filename, "\n");

    if (uci.opening_book.f != NULL) {
        fclose(uci.opening_book.f);
    }

	strcat(file_to_open, uci.opening_book.filename);

    uci.opening_book.f = fopen(file_to_open, "rb");
    fseek(uci.opening_book.f, -16, SEEK_END);
    uci.opening_book.book_size = ftell(uci.opening_book.f) / 16;

}

int book_count()
{
    int count = 0;

	TCHAR file_path[2048] = { 0 };

	GetModuleFileName(0, file_path, 2048);
	PathRemoveFileSpec(file_path);
	PathAddBackslash(file_path);

	CHAR szSearch[2048] = { 0 };

	strcpy(szSearch, file_path);
	strcat(szSearch, "*.bin");

    HANDLE hFind = NULL;
    WIN32_FIND_DATAA FindFileData;

    hFind = FindFirstFileA(szSearch, &FindFileData);
    if (hFind == INVALID_HANDLE_VALUE) return count;
    do {
        count++;
    } while (FindNextFileA(hFind, &FindFileData));

    FindClose(hFind);
    return count;
}

char *book_string()
{

    static char s[1024] = { 0 };
    static char d[1024] = { 0 };
    static char b[1024] = { 0 };

    LARGE_INTEGER biggest;

	TCHAR file_path[2048] = { 0 };

	GetModuleFileName(0, file_path, 2048);
	PathRemoveFileSpec(file_path);
	PathAddBackslash(file_path);

	CHAR szSearch[2048] = { 0 };

	strcpy(szSearch, file_path);
	strcat(szSearch, "*.bin");

    HANDLE hFind = NULL;
    WIN32_FIND_DATAA FindFileData;
    LARGE_INTEGER size;

    biggest.QuadPart = 0;

    hFind = FindFirstFileA(szSearch, &FindFileData);
    if (hFind == INVALID_HANDLE_VALUE) return s;
    do {
        strcat(s, " var ");
        strcat(s, FindFileData.cFileName);

        size.HighPart = FindFileData.nFileSizeHigh;
        size.LowPart = FindFileData.nFileSizeLow;

        if (size.QuadPart > biggest.QuadPart) {
            biggest.QuadPart = FindFileData.nFileSizeHigh;
            strcpy(d, FindFileData.cFileName);
        }

    } while (FindNextFileA(hFind, &FindFileData));
    FindClose(hFind);

    //-- Find any Maverick opening books (...and give them preference)
    biggest.QuadPart = 0;

	strcpy(szSearch, file_path);
	strcpy(szSearch, "Maverick*.bin");
    hFind = FindFirstFileA(szSearch, &FindFileData);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            size.HighPart = FindFileData.nFileSizeHigh;
            size.LowPart = FindFileData.nFileSizeLow;

            if (size.QuadPart > biggest.QuadPart) {
                biggest.QuadPart = FindFileData.nFileSizeHigh;
                strcpy(d, FindFileData.cFileName);
            }

        } while (FindNextFileA(hFind, &FindFileData));
        FindClose(hFind);
    }

    strcpy(b, "option name Opening Book type combo default ");
    strcat(b, d);
    strcat(b, s);

    strcpy(uci.opening_book.filename, d);
    uci.opening_book.f = fopen(uci.opening_book.filename, "rb");
    fseek(uci.opening_book.f, -16, SEEK_END);
    uci.opening_book.book_size = ftell(uci.opening_book.f) / 16;

    return b;

}

unsigned long long read_integer(FILE *file, int size)
{

    unsigned long long n;
    int i;
    int b;

    n = 0;

    for (i = 0; i < size; i++)
    {
        b = fgetc(file);
        n = (n << 8) | b;
    }

    return n;
}

void read_book_move(int index, struct t_book_move *book_move)
{
    fseek(uci.opening_book.f, index * 16, SEEK_SET);

    book_move->key = read_integer(uci.opening_book.f, 8);
    book_move->move = read_integer(uci.opening_book.f, 2);
    book_move->weight = read_integer(uci.opening_book.f, 2);
    book_move->n = read_integer(uci.opening_book.f, 2);
    book_move->learn = read_integer(uci.opening_book.f, 2);
}

struct t_move_record *decode_move(struct t_board *board, unsigned int move)
{
    int from_square = (move >> 6) & 077;
    int from_rank = (from_square >> 3) & 0x7;
    int from_file = from_square & 0x7;
    from_square = (8 * from_rank) + from_file;

    int to_square = move & 077;
    int to_rank = (to_square >> 3) & 0x7;
    int to_file = to_square & 0x7;
    to_square = (8 * to_rank) + to_file;

    int promote = (move >> 12) & 0x7;
    t_chess_piece piece = board->square[from_square];
    t_chess_piece captured = BLANK;
    if (board->square[to_square] != BLANK && COLOR(piece) != COLOR(board->square[to_square]))
        captured = PIECETYPE(board->square[to_square]);

    if (promote)
        return move_directory[from_square][to_square][piece] + (4 * captured) + promote - 1;
    else
        return move_directory[from_square][to_square][piece] + captured;
}

t_move_record *probe_book(struct t_board *board)
{
    //-- The Key we're looking for!
    t_hash key = board->hash;

    //-- Used to get the data from the file
    struct t_book_move		book_move[1];

    //-- Not open?
    if (uci.opening_book.f == NULL)
        return NULL;

    //-- List of possible moves
    struct t_move_list	moves[1];
    moves->count = 0;

    //-- First entry
    int first = 0;

    //-- Last entry
    int last = uci.opening_book.book_size;

    //-- Binary search (which cleverly finds the lowest entry with the same key)
    while (first < last) {

        int middle = (first + last) / 2;
        read_book_move(middle, book_move);

        if (key <= book_move->key) {
            last = middle;
        }
        else {
            first = middle + 1;
        }
    }

    assert(first == last);
    read_book_move(first, book_move);

    //-- return NULL value if we cannot find the move
    if (book_move->key != key)
        return NULL;

    //-- local storage
    struct t_move_record *move;

    //-- Add all suitable moves to the list
    int i = 0;
    int sum = 0;
    do {
        move = decode_move(board, book_move->move);
        moves->move[i] = move;
        moves->value[i] = book_move->weight;
        sum += book_move->weight;
        i++;
        if (first + i < uci.opening_book.book_size)
            read_book_move(first + i, book_move);
    } while (book_move->key == key && first + i < uci.opening_book.book_size);
    moves->count = i;

    //-- Generate random move
    int random = rand() % sum;

    //-- Select the best move
    sum = 0;
    for (i = 0; i < moves->count; i++) {
        sum += moves->value[i];
        if (random <= sum)
            return moves->move[i];
    }

    return moves->move[0];

}

int book_move_count(struct t_board *board){

	//-- The Key we're looking for!
	t_hash key = board->hash;

	//-- Used to get the data from the file
	struct t_book_move		book_move[1];

	//-- Not open?
	if (uci.opening_book.f == NULL)
		return 0;

	//-- First entry
	int first = 0;

	//-- Last entry
	int last = uci.opening_book.book_size;

	//-- Binary search (which cleverly finds the lowest entry with the same key)
	while (first < last) {

		int middle = (first + last) / 2;
		read_book_move(middle, book_move);

		if (key <= book_move->key) {
			last = middle;
		}
		else {
			first = middle + 1;
		}
	}

	assert(first == last);
	read_book_move(first, book_move);

	//-- return 0 value if we cannot find the move
	if (book_move->key != key)
		return 0;

	//-- Count all suitable moves to the list
	int i = 0;
	do {
		i++;
		if (first + i < uci.opening_book.book_size)
			read_book_move(first + i, book_move);
	} while (book_move->key == key && first + i < uci.opening_book.book_size);

	//-- Return the move count
	return i;
}

void fill_opening_move_list(struct t_board *board, struct t_move_list *move_list){

	//-- The Key we're looking for!
	t_hash key = board->hash;

	//-- Used to get the data from the file
	struct t_book_move		book_move[1];

	//-- Not open?
	if (uci.opening_book.f == NULL)
		return;

	//-- List of possible moves
	struct t_move_list	moves[1];
	moves->count = 0;

	//-- First entry
	int first = 0;

	//-- Last entry
	int last = uci.opening_book.book_size;

	//-- Binary search (which cleverly finds the lowest entry with the same key)
	while (first < last) {

		int middle = (first + last) / 2;
		read_book_move(middle, book_move);

		if (key <= book_move->key) {
			last = middle;
		}
		else {
			first = middle + 1;
		}
	}

	assert(first == last);
	read_book_move(first, book_move);

	//-- return NULL value if we cannot find the move
	if (book_move->key != key)
		return;

	//-- local storage
	struct t_move_record *move;

	//-- Add all suitable moves to the list
	int i = 0;
	int sum = 0;
	do {
		move = decode_move(board, book_move->move);
		moves->move[i] = move;
		moves->value[i] = book_move->weight;
		sum += book_move->weight;
		i++;
		if (first + i < uci.opening_book.book_size)
			read_book_move(first + i, book_move);
	} while (book_move->key == key && first + i < uci.opening_book.book_size);
	moves->count = i;


	move_list->count = 0;
	int n = 0;
	do{

		//-- Generate random move
		int random = rand() % sum;

		//-- Select the best move
		int value_sum = 0;
		for (i = 0; i < moves->count; i++) {
			value_sum += moves->value[i];
			if (random <= value_sum){
				if (!is_move_in_list(moves->move[i], move_list)){
					move_list->move[move_list->count] = moves->move[i];
					move_list->count++;
				}
				break;
			}
		}
		n++;

	}while (move_list->count * 4 < moves->count * 3 && n < moves->count * 5);

}

#endif