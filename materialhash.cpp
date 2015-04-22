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

void init_material_hash()
{
	//--Fill with Polglot hash values!
	int n = 0;
	for (int i = 0; i < 16; i++){
		material_hash_values[i][0] = 0;
		for (int j = 1; j < 10; j++){
			material_hash_values[i][j] = polyglot_random[n++];
		}
	}

	material_hash = NULL;

	material_hash_mask = (t_hash) 7;
	do{
		material_hash_mask <<= 1;
		material_hash_mask++;

		if (material_hash != NULL){
			free(material_hash);
			material_hash = NULL;
		}

		material_hash = (struct t_material_hash_record *)malloc(((material_hash_mask + 1) * sizeof(struct t_material_hash_record)));

	}while (!fill_material_hash());

}

t_hash get_material_hash(const int material [])
{
	t_hash zobrist = 0;
	for (int i = WHITEKNIGHT; i <= BLACKKING; i++){
		for (int j = 1; j <= material[i]; j++){
			zobrist ^= material_hash_values[i][j];
		}
	}
	return zobrist;
}

void clear_array(int a [])
{
	for (int i = 15; i >= 0; i--){
		a[i] = 0;
	}
}

t_hash calc_material_hash(struct t_board *board)
{
	int material[16];
	clear_array(material);
	for (t_chess_color color = WHITE; color <= BLACK; color++){
		for (int piece = KNIGHT; piece <= PAWN; piece++){
			material[PIECEINDEX(color, piece)] = popcount(board->pieces[color][piece]);
		}
	}
	return get_material_hash(material);
}

BOOL fill_material_hash()
{
	//-- Reset all the material hash records
	for (int i = material_hash_mask; i >= 0; i--){
		material_hash[i].key = 0;
		material_hash[i].eval_endgame = NULL;
	}

	//-- Local variables
	t_hash key;
	t_hash index;
	int material[16];

	//-- K vs. k
	clear_array(material);
	key = get_material_hash(material);
	index = key & material_hash_mask;
	if (material_hash[index].key != 0 || material_hash[index].eval_endgame != NULL)
		return FALSE;
	material_hash[index].key = key;
	material_hash[index].eval_endgame = &known_endgame_insufficient_material;

	//-- Q + K vs. k
	clear_array(material);
	material[WHITEQUEEN] = 1;
	key = get_material_hash(material);
	index = key & material_hash_mask;
	if (material_hash[index].key != 0)
		return FALSE;
	material_hash[index].key = key;
	material_hash[index].eval_endgame = &known_endgame_QKvk;

	//-- K vs. q + k
	clear_array(material);
	material[BLACKQUEEN] = 1;
	key = get_material_hash(material);
	index = key & material_hash_mask;
	if (material_hash[index].key != 0)
		return FALSE;
	material_hash[index].key = key;
	material_hash[index].eval_endgame = &known_endgame_Kvqk;

	//-- R + K vs. k
	clear_array(material);
	material[WHITEROOK] = 1;
	key = get_material_hash(material);
	index = key & material_hash_mask;
	if (material_hash[index].key != 0)
		return FALSE;
	material_hash[index].key = key;
	material_hash[index].eval_endgame = &known_endgame_RKvk;

	//-- K vs. r + k
	clear_array(material);
	material[BLACKROOK] = 1;
	key = get_material_hash(material);
	index = key & material_hash_mask;
	if (material_hash[index].key != 0)
		return FALSE;
	material_hash[index].key = key;
	material_hash[index].eval_endgame = &known_endgame_Kvrk;

	//-- B + B + K vs. k
	clear_array(material);
	material[WHITEBISHOP] = 2;
	key = get_material_hash(material);
	index = key & material_hash_mask;
	if (material_hash[index].key != 0)
		return FALSE;
	material_hash[index].key = key;
	material_hash[index].eval_endgame = &known_endgame_BBKvk;

	//-- K vs. b + b + k
	clear_array(material);
	material[BLACKBISHOP] = 2;
	key = get_material_hash(material);
	index = key & material_hash_mask;
	if (material_hash[index].key != 0)
		return FALSE;
	material_hash[index].key = key;
	material_hash[index].eval_endgame = &known_endgame_Kvbbk;

	//-- B + N + K vs. k
	clear_array(material);
	material[WHITEBISHOP] = 1;
	material[WHITEKNIGHT] = 1;
	key = get_material_hash(material);
	index = key & material_hash_mask;
	if (material_hash[index].key != 0)
		return FALSE;
	material_hash[index].key = key;
	material_hash[index].eval_endgame = &known_endgame_BNKvk;

	//-- K vs. b + n + k
	clear_array(material);
	material[BLACKBISHOP] = 1;
	material[BLACKKNIGHT] = 1;
	key = get_material_hash(material);
	index = key & material_hash_mask;
	if (material_hash[index].key != 0)
		return FALSE;
	material_hash[index].key = key;
	material_hash[index].eval_endgame = &known_endgame_Kvbnk;

	//-- K + N vs. k
	clear_array(material);
	material[WHITEKNIGHT] = 1;
	key = get_material_hash(material);
	index = key & material_hash_mask;
	if (material_hash[index].key != 0)
		return FALSE;
	material_hash[index].key = key;
	material_hash[index].eval_endgame = &known_endgame_insufficient_material;

	//-- K vs. k + n
	clear_array(material);
	material[BLACKKNIGHT] = 1;
	key = get_material_hash(material);
	index = key & material_hash_mask;
	if (material_hash[index].key != 0)
		return FALSE;
	material_hash[index].key = key;
	material_hash[index].eval_endgame = &known_endgame_insufficient_material;

	//-- K + N + N vs. k
	clear_array(material);
	material[WHITEKNIGHT] = 2;
	key = get_material_hash(material);
	index = key & material_hash_mask;
	if (material_hash[index].key != 0)
		return FALSE;
	material_hash[index].key = key;
	material_hash[index].eval_endgame = &known_endgame_insufficient_material;

	//-- K vs. k + n + n
	clear_array(material);
	material[BLACKKNIGHT] = 2;
	key = get_material_hash(material);
	index = key & material_hash_mask;
	if (material_hash[index].key != 0)
		return FALSE;
	material_hash[index].key = key;
	material_hash[index].eval_endgame = &known_endgame_insufficient_material;

	//-- K + B vs. k
	clear_array(material);
	material[WHITEBISHOP] = 1;
	key = get_material_hash(material);
	index = key & material_hash_mask;
	if (material_hash[index].key != 0)
		return FALSE;
	material_hash[index].key = key;
	material_hash[index].eval_endgame = &known_endgame_insufficient_material;

	//-- K vs. k + b
	clear_array(material);
	material[BLACKBISHOP] = 1;
	key = get_material_hash(material);
	index = key & material_hash_mask;
	if (material_hash[index].key != 0)
		return FALSE;
	material_hash[index].key = key;
	material_hash[index].eval_endgame = &known_endgame_insufficient_material;

	//-- K + B vs. k + n
	clear_array(material);
	material[WHITEBISHOP] = 1;
	material[BLACKKNIGHT] = 1;
	key = get_material_hash(material);
	index = key & material_hash_mask;
	if (material_hash[index].key != 0)
		return FALSE;
	material_hash[index].key = key;
	material_hash[index].eval_endgame = &known_endgame_insufficient_material;

	//-- K + N vs. k + b
	clear_array(material);
	material[WHITEKNIGHT] = 1;
	material[BLACKBISHOP] = 1;
	key = get_material_hash(material);
	index = key & material_hash_mask;
	if (material_hash[index].key != 0)
		return FALSE;
	material_hash[index].key = key;
	material_hash[index].eval_endgame = &known_endgame_insufficient_material;

	//-- K + R + N vs. k
	clear_array(material);
	material[WHITEKNIGHT] = 1;
	material[WHITEROOK] = 1;
	key = get_material_hash(material);
	index = key & material_hash_mask;
	if (material_hash[index].key != 0)
		return FALSE;
	material_hash[index].key = key;
	material_hash[index].eval_endgame = &known_endgame_KRNvk;

	//-- K vs. k + r + n
	clear_array(material);
	material[BLACKKNIGHT] = 1;
	material[BLACKROOK] = 1;
	key = get_material_hash(material);
	index = key & material_hash_mask;
	if (material_hash[index].key != 0)
		return FALSE;
	material_hash[index].key = key;
	material_hash[index].eval_endgame = &known_endgame_Kvkrn;

	//-- K + R + B vs. k
	clear_array(material);
	material[WHITEBISHOP] = 1;
	material[WHITEROOK] = 1;
	key = get_material_hash(material);
	index = key & material_hash_mask;
	if (material_hash[index].key != 0)
		return FALSE;
	material_hash[index].key = key;
	material_hash[index].eval_endgame = &known_endgame_KRBvk;

	//-- K vs. k + r + b
	clear_array(material);
	material[BLACKBISHOP] = 1;
	material[BLACKROOK] = 1;
	key = get_material_hash(material);
	index = key & material_hash_mask;
	if (material_hash[index].key != 0)
		return FALSE;
	material_hash[index].key = key;
	material_hash[index].eval_endgame = &known_endgame_Kvkrb;

	//-- K + N vs. k  + p
	clear_array(material);
	material[WHITEKNIGHT] = 1;
	material[BLACKPAWN] = 1;
	key = get_material_hash(material);
	index = key & material_hash_mask;
	if (material_hash[index].key != 0)
		return FALSE;
	material_hash[index].key = key;
	material_hash[index].eval_endgame = &known_endgame_KNvkp;

	//-- K + P vs. k + n
	clear_array(material);
	material[WHITEPAWN] = 1;
	material[BLACKKNIGHT] = 1;
	key = get_material_hash(material);
	index = key & material_hash_mask;
	if (material_hash[index].key != 0)
		return FALSE;
	material_hash[index].key = key;
	material_hash[index].eval_endgame = &known_endgame_KPvkn;

	//-- K + B vs. k  + p
	clear_array(material);
	material[WHITEBISHOP] = 1;
	material[BLACKPAWN] = 1;
	key = get_material_hash(material);
	index = key & material_hash_mask;
	if (material_hash[index].key != 0)
		return FALSE;
	material_hash[index].key = key;
	material_hash[index].eval_endgame = &known_endgame_KBvkp;

	//-- K + P vs. k + b
	clear_array(material);
	material[WHITEPAWN] = 1;
	material[BLACKBISHOP] = 1;
	key = get_material_hash(material);
	index = key & material_hash_mask;
	if (material_hash[index].key != 0)
		return FALSE;
	material_hash[index].key = key;
	material_hash[index].eval_endgame = &known_endgame_KPvkb;

	//-- K + B vs. k  + r
	clear_array(material);
	material[WHITEBISHOP] = 1;
	material[BLACKROOK] = 1;
	key = get_material_hash(material);
	index = key & material_hash_mask;
	if (material_hash[index].key != 0)
		return FALSE;
	material_hash[index].key = key;
	material_hash[index].eval_endgame = &known_endgame_KBvkr;

	//-- K + R vs. k + b
	clear_array(material);
	material[WHITEROOK] = 1;
	material[BLACKBISHOP] = 1;
	key = get_material_hash(material);
	index = key & material_hash_mask;
	if (material_hash[index].key != 0)
		return FALSE;
	material_hash[index].key = key;
	material_hash[index].eval_endgame = &known_endgame_KRvkb;
	
	//-- K + N vs. k  + r
	clear_array(material);
	material[WHITEKNIGHT] = 1;
	material[BLACKROOK] = 1;
	key = get_material_hash(material);
	index = key & material_hash_mask;
	if (material_hash[index].key != 0)
		return FALSE;
	material_hash[index].key = key;
	material_hash[index].eval_endgame = &known_endgame_KNvkr;

	//-- K + R vs. k + n
	clear_array(material);
	material[WHITEROOK] = 1;
	material[BLACKKNIGHT] = 1;
	key = get_material_hash(material);
	index = key & material_hash_mask;
	if (material_hash[index].key != 0)
		return FALSE;
	material_hash[index].key = key;
	material_hash[index].eval_endgame = &known_endgame_KRvkn;

	//-- K + N + N vs. k + n
	clear_array(material);
	material[WHITEKNIGHT] = 2;
	material[BLACKKNIGHT] = 1;
	key = get_material_hash(material);
	index = key & material_hash_mask;
	assert(material_hash[index].key != key);
	if (material_hash[index].key != 0)
		return FALSE;
	material_hash[index].key = key;
	material_hash[index].eval_endgame = &known_endgame_insufficient_material;

	//-- K + N vs. k + n + n
	clear_array(material);
	material[WHITEKNIGHT] = 1;
	material[BLACKKNIGHT] = 2;
	key = get_material_hash(material);
	index = key & material_hash_mask;
	assert(material_hash[index].key != key);
	if (material_hash[index].key != 0)
		return FALSE;
	material_hash[index].key = key;
	material_hash[index].eval_endgame = &known_endgame_insufficient_material;

	//-- K + N + N vs. k + n + n
	clear_array(material);
	material[WHITEKNIGHT] = 2;
	material[BLACKKNIGHT] = 2;
	key = get_material_hash(material);
	index = key & material_hash_mask;
	assert(material_hash[index].key != key);
	if (material_hash[index].key != 0)
		return FALSE;
	material_hash[index].key = key;
	material_hash[index].eval_endgame = &known_endgame_insufficient_material;

	//-- K + N + N vs. k + b
	clear_array(material);
	material[WHITEKNIGHT] = 2;
	material[BLACKBISHOP] = 1;
	key = get_material_hash(material);
	index = key & material_hash_mask;
	assert(material_hash[index].key != key);
	if (material_hash[index].key != 0)
		return FALSE;
	material_hash[index].key = key;
	material_hash[index].eval_endgame = &known_endgame_insufficient_material;

	//-- K + B vs. k + n + n
	clear_array(material);
	material[WHITEBISHOP] = 1;
	material[BLACKKNIGHT] = 2;
	key = get_material_hash(material);
	index = key & material_hash_mask;
	assert(material_hash[index].key != key);
	if (material_hash[index].key != 0)
		return FALSE;
	material_hash[index].key = key;
	material_hash[index].eval_endgame = &known_endgame_insufficient_material;

	//-- K + R + B vs. k + r
	clear_array(material);
	material[WHITEBISHOP] = 1;
	material[WHITEROOK] = 1;
	material[BLACKROOK] = 1;
	key = get_material_hash(material);
	index = key & material_hash_mask;
	assert(material_hash[index].key != key);
	if (material_hash[index].key != 0)
		return FALSE;
	material_hash[index].key = key;
	material_hash[index].eval_endgame = &known_endgame_KRBvkr;

	//-- K + R + N vs. k + r
	clear_array(material);
	material[WHITEKNIGHT] = 1;
	material[WHITEROOK] = 1;
	material[BLACKROOK] = 1;
	key = get_material_hash(material);
	index = key & material_hash_mask;
	assert(material_hash[index].key != key);
	if (material_hash[index].key != 0)
		return FALSE;
	material_hash[index].key = key;
	material_hash[index].eval_endgame = &known_endgame_KRNvkr;

	//-- K + R vs. k + r + b
	clear_array(material);
	material[WHITEROOK] = 1;
	material[BLACKROOK] = 1;
	material[BLACKBISHOP] = 1;
	key = get_material_hash(material);
	index = key & material_hash_mask;
	assert(material_hash[index].key != key);
	if (material_hash[index].key != 0)
		return FALSE;
	material_hash[index].key = key;
	material_hash[index].eval_endgame = &known_endgame_KRvkrb;

	//-- K + R vs. k + r + n
	clear_array(material);
	material[WHITEROOK] = 1;
	material[BLACKROOK] = 1;
	material[BLACKKNIGHT] = 1;
	key = get_material_hash(material);
	index = key & material_hash_mask;
	assert(material_hash[index].key != key);
	if (material_hash[index].key != 0)
		return FALSE;
	material_hash[index].key = key;
	material_hash[index].eval_endgame = &known_endgame_KRvkrn;

	//-- K + P vs. k
	clear_array(material);
	material[WHITEPAWN] = 1;
	key = get_material_hash(material);
	index = key & material_hash_mask;
	assert(material_hash[index].key != key);
	if (material_hash[index].key != 0)
		return FALSE;
	material_hash[index].key = key;
	material_hash[index].eval_endgame = &known_endgame_KPvk;

	//-- K vs. k + p
	clear_array(material);
	material[BLACKPAWN] = 1;
	key = get_material_hash(material);
	index = key & material_hash_mask;
	assert(material_hash[index].key != key);
	if (material_hash[index].key != 0)
		return FALSE;
	material_hash[index].key = key;
	material_hash[index].eval_endgame = &known_endgame_Kvkp;

	//-- K + Q vs. k + q + b
	clear_array(material);
	material[WHITEQUEEN] = 1;
	material[BLACKQUEEN] = 1;
	material[BLACKBISHOP] = 1;
	key = get_material_hash(material);
	index = key & material_hash_mask;
	assert(material_hash[index].key != key);
	if (material_hash[index].key != 0)
		return FALSE;
	material_hash[index].key = key;
	material_hash[index].eval_endgame = &known_endgame_KQvkqb;

	//-- K + Q vs. k + q + n
	clear_array(material);
	material[WHITEQUEEN] = 1;
	material[BLACKQUEEN] = 1;
	material[BLACKKNIGHT] = 1;
	key = get_material_hash(material);
	index = key & material_hash_mask;
	assert(material_hash[index].key != key);
	if (material_hash[index].key != 0)
		return FALSE;
	material_hash[index].key = key;
	material_hash[index].eval_endgame = &known_endgame_KQvkqn;

	//-- K + Q + B vs. k + q
	clear_array(material);
	material[WHITEQUEEN] = 1;
	material[WHITEBISHOP] = 1;
	material[BLACKQUEEN] = 1;
	key = get_material_hash(material);
	index = key & material_hash_mask;
	assert(material_hash[index].key != key);
	if (material_hash[index].key != 0)
		return FALSE;
	material_hash[index].key = key;
	material_hash[index].eval_endgame = &known_endgame_KQBvkq;

	//-- K + Q + N vs. k + q
	clear_array(material);
	material[WHITEQUEEN] = 1;
	material[WHITEKNIGHT] = 1;
	material[BLACKQUEEN] = 1;
	key = get_material_hash(material);
	index = key & material_hash_mask;
	assert(material_hash[index].key != key);
	if (material_hash[index].key != 0)
		return FALSE;
	material_hash[index].key = key;
	material_hash[index].eval_endgame = &known_endgame_KQNvkq;

	//-- K + B + N vs. k + b
	clear_array(material);
	material[WHITEKNIGHT] = 1;
	material[WHITEBISHOP] = 1;
	material[BLACKBISHOP] = 1;
	key = get_material_hash(material);
	index = key & material_hash_mask;
	assert(material_hash[index].key != key);
	if (material_hash[index].key != 0)
		return FALSE;
	material_hash[index].key = key;
	material_hash[index].eval_endgame = &known_endgame_KBNvkb;

	//-- K + B + N vs. k + n
	clear_array(material);
	material[WHITEKNIGHT] = 1;
	material[WHITEBISHOP] = 1;
	material[BLACKKNIGHT] = 1;
	key = get_material_hash(material);
	index = key & material_hash_mask;
	assert(material_hash[index].key != key);
	if (material_hash[index].key != 0)
		return FALSE;
	material_hash[index].key = key;
	material_hash[index].eval_endgame = &known_endgame_KBNvkn;

	//-- K + B vs. k + b + n
	clear_array(material);
	material[WHITEBISHOP] = 1;
	material[BLACKBISHOP] = 1;
	material[BLACKKNIGHT] = 1;
	key = get_material_hash(material);
	index = key & material_hash_mask;
	assert(material_hash[index].key != key);
	if (material_hash[index].key != 0)
		return FALSE;
	material_hash[index].key = key;
	material_hash[index].eval_endgame = &known_endgame_KBvkbn;

	//-- K + N vs. k + b + n
	clear_array(material);
	material[WHITEKNIGHT] = 1;
	material[BLACKBISHOP] = 1;
	material[BLACKKNIGHT] = 1;
	key = get_material_hash(material);
	index = key & material_hash_mask;
	assert(material_hash[index].key != key);
	if (material_hash[index].key != 0)
		return FALSE;
	material_hash[index].key = key;
	material_hash[index].eval_endgame = &known_endgame_KNvkbn;

	return TRUE;

}

void destroy_material_hash()
{
	if (uci.engine_initialized){
		free(material_hash);
		material_hash = NULL;
	}
}
