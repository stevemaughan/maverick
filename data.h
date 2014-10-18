//===========================================================//
//
// Maverick Chess Engine
// Copyright 2013 Steve Maughan
//
//===========================================================//

// UCI
extern struct t_uci uci;
extern char engine_author[30];
extern char engine_name[30];

// Board Position
extern struct t_board position[1];

// Global Move Directory
extern struct t_move_record xmove_list[GLOBAL_MOVE_COUNT];
extern struct t_move_record *move_directory[64][64][15];

// Principle Variation Data
extern struct t_perft_pv_data perft_pv_data[MAXPLY + 1];
extern struct t_multi_pv multi_pv[1];

// Global Search Variables
extern int search_ply;
extern unsigned long cutoffs;
extern unsigned long first_move_cutoffs;

extern int message_update_count;
extern t_nodes message_update_mask;
extern long last_display_update;
extern long search_start_time;
extern int deepest;
extern int most_captures;
extern t_nodes nodes;
extern t_nodes qnodes;
extern t_nodes start_nodes;
extern t_chess_time early_move_time;
extern t_chess_time target_move_time;
extern t_chess_time abort_move_time;

// Perft
extern t_nodes global_nodes;
extern long perft_start_time;
extern long perft_end_time;

// Aspiration Window
extern const t_chess_value aspiration_window[2][6];

// x88 Constants
extern const BOOL slider[15];
extern const char direction[15][10];
extern const char piece_count_value[15];

// SEE Values
extern const int see_piece_value[15];

// Castling records
extern struct t_castle_record castle[4];

/* Repetition Variables */
extern t_hash draw_stack[MAX_MOVES];
extern int draw_stack_count;
extern int search_start_draw_stack_count;

// Bitboards
extern t_bitboard between[64][64];									// squares between any two squares on the board (*not* including start and finish)
extern t_bitboard line[64][64];										// squares between any two squares on the board (*including* start and finish)
extern t_bitboard bishop_rays[64];									// bishop moves on an empty board from each square
extern t_bitboard rook_rays[64];									// rook moves on an empty board from each square
extern t_bitboard queen_rays[64];
extern t_bitboard rank_mask[2][8];
extern t_bitboard knight_mask[64];									// knight moves from each square
extern t_bitboard king_mask[64];									// king moves from each square
extern t_bitboard pawn_attackers[2][64];							// Squares of pawns of color which will attach a given square e.g. pawn_attackers[WHITE][E4] = D3 and F3
extern t_bitboard xray[64][64];
extern t_bitboard neighboring_file[8];
extern t_bitboard column_mask[8];
extern t_bitboard square_rank_mask[64];								// Mask the squares on the same rank
extern t_bitboard square_column_mask[64];							// Mask the squares on the same column
extern t_bitboard forward_squares[2][64];							// Mask of the squares infront of a give square, moving in the direction of a pawn
extern t_bitboard connected_pawn_mask[64];							// Mask for connected pawn for a given square

extern const t_chess_square bitscan_table[64];

//  Can move
extern t_piece_mask can_move[64][64];

// Piece Square Tables
extern t_chess_value piece_value[2][8];
extern t_chess_value piece_square_table[16][2][64];

extern const t_chess_value pawn_pst[2][64];
extern const t_chess_value knight_pst[2][64];
extern const t_chess_value bishop_pst[2][64];
extern const t_chess_value rook_pst[2][64];
extern const t_chess_value queen_pst[2][64];
extern const t_chess_value king_pst[2][64];

extern const t_chess_value lone_king[64];
extern const t_chess_value bishop_knight_corner[2][64];
extern const t_chess_color square_color[64];

extern const t_chess_value passed_pawn_bonus[2][64];

//-- Mobility
extern const t_chess_value horizontal_rook_mobility[2][8];
extern const t_chess_value vertical_rook_mobility[2][8];

extern const t_chess_value bishop_mobility[2][16];

extern const t_chess_value double_pawn_penalty[2][8];

// Magics
extern t_bitboard rook_magic_moves[64][4096];
extern t_bitboard bishop_magic_moves[64][512];
extern const struct t_magic_structure rook_magic[64];
extern const struct t_magic_structure bishop_magic[64];

// Hash Table
extern struct t_pawn_hash_record *pawn_hash;
extern t_hash pawn_hash_mask;

extern struct t_hash_record *hash_table;
extern t_hash hash_mask;
extern t_nodes hash_probes;
extern t_nodes hash_hits;
extern t_nodes hash_full;
extern int hash_age;

extern struct t_material_hash_record *material_hash;
extern t_hash material_hash_mask;
extern t_hash material_hash_values[16][10];

extern t_hash hash_value[16][64];
extern t_hash castle_hash[16];
extern t_hash ep_hash[8];
extern t_hash white_to_move_hash;
extern const t_hash polyglot_random[781];
