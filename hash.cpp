//===========================================================//
//
// Maverick Chess Engine
// Copyright 2013-2015 Steve Maughan
//
//===========================================================//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "defs.h"
#include "eval.h"
#include "data.h"
#include "procs.h"
#include "bittwiddle.h"

void destroy_hash()
{
	free(hash_table);
}

void set_hash(unsigned int size)
{
	unsigned long i;

	if (uci.options.hash_table_size == size) return;

	i = 1;
	while ((i + 1) * sizeof(struct t_hash_record) <= size * 1024 * 1024)
		(i <<= 1);

	free(hash_table);
	hash_table = (struct t_hash_record*)malloc(i * sizeof(struct t_hash_record));

	hash_mask = i - HASH_ATTEMPTS;
	clear_hash();
	uci.options.hash_table_size = size;
}

void poke(t_hash hash_key, t_chess_value score, int ply, int depth, t_hash_bound bound, struct t_move_record *move)
{

	struct t_hash_record *h, *best_hash;
	int best_score;
	int h_score;
	int i;
	int poke_score = score;

	//-- Exit if stopping
	if (uci.stop) return;

	/* Don't store draws!! */
	if (score == 0) return;

	/* adjust mate score */
	if (score >= MAX_CHECKMATE){
		//if (bound == HASH_UPPER) return;
		poke_score += ply;
	}
	else
	if (score <= -MAX_CHECKMATE){
		//if (bound == HASH_LOWER) return;
		poke_score -= ply;
	}

	h =	&hash_table[hash_key & hash_mask];

	best_score = -CHESS_INFINITY;
	for (i = HASH_ATTEMPTS; i > 0; i--, h++){

		//-- Do we have a match (always replace the match)
		if (h->key == hash_key){

			if (h->age != hash_age)
				hash_full++;
			h->age = hash_age;

			if (h->depth <= depth || bound != HASH_UPPER){
				h->bound = bound;
				h->score = poke_score;
				h->depth = depth;
				h->key = hash_key;
				h->move = move;
			}
			return;
		}
		else{
			h_score = (h->key == 0) * 4096 + (hash_age - h->age) * 4096 + h->depth * 16 + (h->bound == HASH_EXACT) * 16 + (h->bound == HASH_LOWER);
			if (h_score > best_score){
				best_score = h_score;
				best_hash = h;
			}
		}
	}
	assert(best_hash != NULL);

	if (best_hash->age != hash_age) hash_full++;

	best_hash->age = hash_age;
	best_hash->bound = bound;
	best_hash->depth = depth;
	best_hash->key = hash_key;
	best_hash->score = poke_score;
	best_hash->move = move;
	return;
}

struct t_hash_record *probe(t_hash hash_key)
{
	struct t_hash_record *h;
	int i;

	hash_probes++;

	h = &hash_table[hash_key & hash_mask];

	i = 0;
	do{
		if (h->key == hash_key){
			hash_hits++;
			assert(h->score < CHECKMATE && h->score > -CHECKMATE);
			return h;
		}
		i++;
		h++;
	} while (i < HASH_ATTEMPTS);

	return NULL;
}

t_chess_value get_hash_score(struct t_hash_record *hash_record, int ply)
{
	if (hash_record->score >= MAX_CHECKMATE){
		return hash_record->score - ply;
	}
	else if (hash_record->score <= -MAX_CHECKMATE){
		return hash_record->score + ply;
	}
	return hash_record->score;
}

void poke_draw(t_hash hash_key)
{

	struct t_hash_record *h, *best_hash;
	int best_score;
	int h_score;
	int i;

	//-- Exit if stopping
	if (uci.stop) return;

	h = &hash_table[hash_key & hash_mask];

	best_score = -CHESS_INFINITY;
	for (i = HASH_ATTEMPTS; i > 0; i--, h++){

		//-- Do we have a match (always replace the match)
		if (h->key == hash_key){

			if (h->age != hash_age)
				hash_full++;
			h->age = hash_age;
			h->bound = HASH_EXACT;
			h->score = 0;
			h->depth = MAXPLY;
			h->key = hash_key;
			h->move = NULL;
			return;
		}
		else{
			h_score = (h->key == 0) * 4096 + (hash_age - h->age) * 4096 + h->depth * 16 + (h->bound == HASH_EXACT) * 16 + (h->bound == HASH_LOWER);
			if (h_score > best_score){
				best_score = h_score;
				best_hash = h;
			}
		}
	}
	assert(best_hash != NULL);

	if (best_hash->age != hash_age) hash_full++;

	best_hash->age = hash_age;
	best_hash->bound = HASH_EXACT;
	best_hash->depth = MAXPLY;
	best_hash->key = hash_key;
	best_hash->score = 0;
	best_hash->move = NULL;
	return;
}

void clear_hash()
{
	t_hash i = hash_mask + HASH_ATTEMPTS;
	memset(hash_table, 0, sizeof(t_hash_record) * i);
}

t_hash calc_board_hash(struct t_board *board) {
    t_hash zobrist = 0;
    t_chess_square s;
    t_chess_piece piece;
    t_chess_color color;
    t_bitboard b;

    //--Pieces
    for (color = WHITE; color <= BLACK; color++) {
        for (piece = KNIGHT; piece <= KING; piece++) {
            b = board->pieces[color][piece];
            while (b) {
                s = bitscan(b);
                b &= (b - 1);
                zobrist ^= hash_value[PIECEINDEX(color, piece)][s];
            }
        }
    }

    //--Castling
    zobrist ^= castle_hash[board->castling];

    //--ep
    if (board->ep_square)
        zobrist ^= ep_hash[COLUMN(bitscan(board->ep_square))];

    //-- White to Move
    if (board->to_move == WHITE)
        zobrist ^= white_to_move_hash;

    return zobrist;
}

void init_hash() {
    int i = 0;
    int b;
    int color;
    t_chess_square s;
    t_chess_piece piece;
    t_hash c[4];

    //-- Set all to zero
    for (piece = BLANK; piece < 16; piece++) {
        for (color = WHITE; color <= BLACK; color++) {
            for (s = A1; s <= H8; s++) {
                hash_value[PIECEINDEX(color,piece)][s] = 0;
            }
        }
    }

    // Pawns
    piece = PAWN;
    for (color = BLACK; color >= WHITE; color--) {
        for (s = A1; s <= H8; s++) {
            hash_value[PIECEINDEX(color, piece)][s] = polyglot_random[i++];
        }
    }
    // Knight
    piece = KNIGHT;
    for (color = BLACK; color >= WHITE; color--) {
        for (s = A1; s <= H8; s++) {
            hash_value[PIECEINDEX(color, piece)][s] = polyglot_random[i++];
        }
    }
    // Bishop
    piece = BISHOP;
    for (color = BLACK; color >= WHITE; color--) {
        for (s = A1; s <= H8; s++) {
            hash_value[PIECEINDEX(color, piece)][s] = polyglot_random[i++];
        }
    }
    // Rook
    piece = ROOK;
    for (color = BLACK; color >= WHITE; color--) {
        for (s = A1; s <= H8; s++) {
            hash_value[PIECEINDEX(color, piece)][s] = polyglot_random[i++];
        }
    }
    // Queen
    piece = QUEEN;
    for (color = BLACK; color >= WHITE; color--) {
        for (s = A1; s <= H8; s++) {
            hash_value[PIECEINDEX(color, piece)][s] = polyglot_random[i++];
        }
    }

    // King
    piece = KING;
    for (color = BLACK; color >= WHITE; color--) {
        for (s = A1; s <= H8; s++) {
            hash_value[PIECEINDEX(color, piece)][s] = polyglot_random[i++];
        }
    }

    //-- Castling
    c[0] = polyglot_random[i++];
    c[1] = polyglot_random[i++];
    c[2] = polyglot_random[i++];
    c[3] = polyglot_random[i++];
    for (s = 0; s < 16; s++) {
        castle_hash[s] = 0;
        for (b = 0; b < 4; b++) {
            if (s & ((int)1 << b))
                castle_hash[s] ^= c[b];
        }
    }

    //-- e.n. passant
    for (s = 0; s < 8; s++)
        ep_hash[s] = polyglot_random[i++];

    //-- White's turn
    white_to_move_hash = polyglot_random[i++];

    assert(i == 781);

	hash_table = NULL;
	set_hash(64);
}
