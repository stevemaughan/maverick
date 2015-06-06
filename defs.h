//===========================================================//
// Maverick Chess Engine
// Copyright 2013-2015 Steve Maughan
//===========================================================//

//===========================================================//
// Define Version
//===========================================================//
#define ENGINE_NAME							"Maverick"
#define VERSION_NUMBER						"2.0 alpha"

#if defined(_WIN32) && !defined(_WIN64)
#define ENGINE_VERSION					VERSION_NUMBER " x86"
#elif defined(_WIN64) && defined(NOPOPCOUNT)
#define ENGINE_VERSION					VERSION_NUMBER " x64 np"
#elif defined(_WIN64) && defined(_DEBUG)
#define ENGINE_VERSION					VERSION_NUMBER " x64 Debug"
#elif defined(_WIN64)
#define ENGINE_VERSION					VERSION_NUMBER " x64"
#elif defined(__arm__)
#define ENGINE_VERSION					VERSION_NUMBER
#elif defined(__linux__)
#define ENGINE_VERSION					VERSION_NUMBER
#endif

#define ENGINE_AUTHOR						"Steve Maughan"
#include <cassert>

//===========================================================//
// Primitive Logic
//===========================================================///
typedef int BOOL;
#define TRUE								1
#define FALSE								0
#ifndef min
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

//===========================================================//
// Basic Chess Constants
//===========================================================//
#define CHESS_INFINITY						99999999
#define CHECKMATE							10000000
#define MAXPLY								127
#define MAX_MOVES							1024
#define MAX_CHECKMATE						(CHECKMATE - 2 * MAXPLY)

#define WHITE								0
#define BLACK								1

#define BLANK								0
#define KNIGHT								1
#define BISHOP								2
#define ROOK								3
#define QUEEN								4
#define PAWN								5
#define KING								6

#define WHITEKNIGHT							1
#define WHITEBISHOP							2
#define WHITEROOK							3
#define WHITEQUEEN							4
#define WHITEPAWN							5
#define WHITEKING							6

#define BLACKKNIGHT							9
#define BLACKBISHOP							10
#define BLACKROOK							11
#define BLACKQUEEN							12
#define BLACKPAWN							13
#define BLACKKING							14

#define KINGSIDE							0
#define QUEENSIDE							1

#define WHITE_CASTLE_OO						1
#define WHITE_CASTLE_OOO					2
#define BLACK_CASTLE_OO						4
#define BLACK_CASTLE_OOO					8

#define MIDDLEGAME							0
#define ENDGAME								1

//===========================================================//
// Ranks
//===========================================================//
#define FIRST_RANK							0
#define SECOND_RANK							1
#define THIRD_RANK							2
#define FOURTH_RANK							3
#define FIFTH_RANK							4
#define SIXTH_RANK							5
#define SEVENTH_RANK						6
#define EIGHTH_RANK							7

//===========================================================//
// Types of Moves
//===========================================================//
typedef enum movetypes {
    MOVE_CASTLE,
    MOVE_PAWN_PUSH1,
    MOVE_PAWN_PUSH2,
    MOVE_PxPAWN,
    MOVE_PxPIECE,
    MOVE_PxP_EP,
    MOVE_PROMOTION,
    MOVE_CAPTUREPROMOTE,
    MOVE_PIECE_MOVE,
    MOVE_PIECExPIECE,
    MOVE_PIECExPAWN,
    MOVE_KING_MOVE,
    MOVE_KINGxPIECE,
    MOVE_KINGxPAWN
} t_chess_move_type;

#define GLOBAL_MOVE_COUNT					43764

//===========================================================//
// Types
//===========================================================//
typedef unsigned long long					t_bitboard;
typedef long long unsigned int				t_hash;
typedef long long unsigned int				t_nodes;
typedef unsigned char						t_chess_piece;
typedef unsigned char						t_chess_square;
typedef unsigned char						t_chess_color;
typedef unsigned char						uchar;
typedef long long unsigned int				t_magic;
typedef unsigned int						t_piece_mask;
typedef int									t_chess_value;
typedef long long int						t_history_value;
typedef signed long							t_chess_time;

//===========================================================//
// UCI Engine States
//===========================================================//
typedef enum enginestates {
    UCI_ENGINE_WAITING,
    UCI_ENGINE_THINKING,
    UCI_ENGINE_START_THINKING
} t_uci_engine_state;

//===========================================================//
// UCI Constants
//===========================================================//
#define UCI_BUFFER_SIZE						4096

//===========================================================//
// General Macros
//===========================================================//
#define OPPONENT(x)							((x) ^ (t_chess_color)1)
#define COLUMN(s)							((s) & (uchar)7)
#define RANK(s)								((s) >> 3)
#define SQUARE64(s)							((t_bitboard)1 << (s))
#define COLOR(x)							((x) >> 3)
#define PIECETYPE(x)						((x) & (uchar)7)
#define PIECEINDEX(color, piece)			(((int)(color) << 3) | piece)
#define x88_SQUARE(x)						((16 * RANK(x)) + COLUMN(x))
#define x88_ONBOARD(x)						(((x) & 136) == 0)
#define x88_TO_64(x)						((8 * ((x) >> 4)) + COLUMN(x))
#define FLIP64(x)							((7 - RANK(x)) * 8 + COLUMN(x))
#define FLIPPIECECOLOR(x)					PIECEINDEX(OPPONENT(COLOR(x)), PIECETYPE(x))
#define PIECEMASK(x)						((t_piece_mask)1 << (x))
#define SQUARECOLOR(x)						((x & 1) ^ (RANK(x) & 1) ^ 1)
#define PROMOTION_SQUARE(color, square)		(56 * (1 - color) + COLUMN(square))
#define COLOR_RANK(color, square)			(COLOR(color) ? (7 - RANK(square)) : RANK(square))
#define CAN_CASTLE(color, castle_flag)		(castle_flag & (((uchar)3) << (color * 2)))

//===========================================================//
// Global Move List Record Structure
//===========================================================//
struct t_move_record
{
    t_chess_move_type						move_type;
    t_chess_piece							piece;
    t_chess_piece							captured;
    t_chess_square							from_square;
    t_chess_square							to_square;
    t_bitboard								from_to_bitboard;
    t_bitboard								capture_mask;
    t_chess_piece							promote_to;
    t_hash									hash_delta;
    t_hash									pawn_hash_delta;
    uchar									castling_delta;
    int										index;
    t_chess_value							history;
    int										mvvlva;
    struct t_move_record					*refutation;
};

struct t_move_list
{
    int										count;							// Number of moves (this doesn't change)
    int										imove;							// This starts at "count" and is decremented as the moves are played
    struct t_move_record					*current_move;					// The move last played
    BOOL									current_move_see_positive;		// True if the current capture is NOT see negative
    struct t_move_record					*hash_move;						// The hash move
    t_bitboard								pinned_pieces;					// A bitboard which stores the position of pinned pieces
    struct t_move_record					*move[256];						// The moves!
    signed long long						value[256];						// Notional values for all of the moves
};

struct t_undo
{
    struct t_move_record					*move;
    uchar									castling;
    t_bitboard								ep_square;
    t_chess_square							attacker;
    uchar									fifty_move_count;
    uchar									in_check;
    t_hash									hash;
    t_hash									pawn_hash;
    t_hash									material_hash;
};

struct t_castle_record
{
    t_bitboard								possible;			// the squares which must be empty for castling to be possible
    t_bitboard								not_attacked;		// the squares which must not be attacked
    t_chess_square							king_from;			// position of the king before castling (relevant for Chess960)
    t_chess_square							rook_from;			// position of the rook before castling (relevant for Chess960)
    t_chess_square							rook_to;			// position of the rook after castling
    t_bitboard								rook_from_to;		// from and to bitboard of rook
    t_chess_piece							rook_piece;			// white or black rook
    uchar									mask;
};

//===========================================================//
// Evaluation Structure
//===========================================================//
struct t_chess_eval
{
    struct t_pawn_hash_record				*pawn_evaluation;
    int										game_phase;
    t_chess_value							middlegame;
    t_chess_value							endgame;
    t_chess_value							static_score;
    t_bitboard								attacklist[15];
    t_bitboard								*attacks[2];
    t_bitboard								king_zone[2];
    int										king_attack_count[2];
    int										king_attack_pressure[2];
    t_chess_value							king_attack[2];				// The pressure against the opponents king - so king_attack[WHITE] measures the pressure against the black king
};

//===========================================================//
// Hash Records
//===========================================================//
#define HASH_ATTEMPTS						4

typedef enum hash_bound {
    HASH_LOWER,
    HASH_EXACT,
    HASH_UPPER
} t_hash_bound;

struct t_hash_record
{
    t_hash									key;
    t_hash_bound							bound;
    int										depth;
    int										age;
    t_chess_value							score;
    struct t_move_record					*move;
};

struct t_pawn_hash_record
{
    t_hash									key;
    t_chess_value							middlegame;
    t_chess_value							endgame;
    t_chess_value							king_pawn_endgame_score;
    t_bitboard								forward_squares[2];
    t_bitboard								backward_squares[2];
    t_bitboard								attacks[2];
    t_bitboard								passed[2];
    t_bitboard								candidate_passed[2];
    t_bitboard								potential_outpost[2];
    t_bitboard								double_pawns[2];
    t_bitboard								backward[2];
    t_bitboard								isolated[2];
    t_bitboard								blocked[2];
    t_bitboard								weak[2];
    t_bitboard								open_file;
    t_bitboard								semi_open_file[2];
    t_chess_value							king_pressure[2];
    int										pawn_count[2];
    int										semi_open_double_pawns[2]; // number of double pawns on semi open files
};

struct t_material_hash_record
{
    t_hash									key;
    void									(*eval_endgame)(struct t_board *board, struct t_chess_eval *eval);
};

//===========================================================//
// PV Data Structures
//===========================================================//
enum t_node_type
{
    node_cut,
    node_super_cut,
    node_pv,
    node_lite_all,
    node_super_all,
    node_all
};

struct t_perft_pv_data
{
    uchar									index;
    char									fen[100];
    struct t_move_record					*move;
    struct t_move_list						move_list[1];
};

struct t_pv_data
{
    struct t_chess_eval						eval[1];
    int										reduction;
	BOOL									extension;
	int										mate_threat;
    struct t_move_record					*current_move;
    struct t_move_record					*killer1;
    struct t_move_record					*killer2;
    struct t_move_record					*check_killer1;
	struct t_move_record					*check_killer2;
	struct t_move_record					*null_refutation;
	BOOL									in_check;
    int										legal_moves_played;
    int										best_line_length;
    struct t_move_record					*best_line[MAXPLY + 1];
    struct t_pv_data						*previous_pv;
    struct t_pv_data						*next_pv;
    t_node_type								node_type;
};


//===========================================================//
// Chess Board Structure
//===========================================================//
struct t_board
{
    t_bitboard								piecelist[15];
    t_bitboard								*pieces[2];
    t_bitboard								all_pieces;
    t_bitboard								occupied[2];
    t_chess_color							to_move;
    t_hash									hash;
    t_hash									pawn_hash;
    t_hash									material_hash;
    BOOL									chess960;
    uchar									castling;
    t_bitboard								ep_square;
    uchar									king_square[2];
    uchar									in_check;
    t_chess_square							check_attacker;
    t_chess_square							square[64];
    uchar									fifty_move_count;
    struct t_pv_data						pv_data[MAXPLY + 2];
    BOOL									castling_squares_changed;
};

//===========================================================//
// Search Constants
//===========================================================//
#define NULL_REDUCTION						3

//===========================================================//
// UCI Interface
//===========================================================//
struct t_level
{
    int										ponder;
    int										infinite;
    t_chess_time							time[2];
    t_chess_time							tinc[2];
    t_nodes									nodes;
    int										depth;
    int										movestogo;
    t_chess_time							movetime;
    int										mate;
};

struct t_uci_options
{
    int										hash_table_size;
    int										pawn_hash_table_size;
    BOOL									current_line;
    BOOL									show_search_statistics;
    BOOL									chess960;
	BOOL									futility_pruning;
};

struct t_uci_thinking
{
    t_hash									predicted_hash;
    unsigned long							predicted_time;
    BOOL									predicted_hash_hit;
    BOOL									fail_high_last_ply;
};

struct t_uci_opening_book
{
    BOOL									use_own_book;
    char									filename[FILENAME_MAX];
    FILE									*f;
    int										book_size;
};

struct t_uci
{
    t_uci_engine_state						engine_state;
    BOOL									stop;
    BOOL									quit;
    struct t_uci_opening_book				opening_book;
    struct t_level							level;
    struct t_uci_options					options;
    struct t_uci_thinking					thinking;
    BOOL									debug;
    BOOL									engine_initialized;
};

struct t_book_move {
    t_hash									key;
    int										move;
    int										weight;
    int										n;
    int										learn;
};

//===========================================================//
// "Magics"
//===========================================================//
struct t_magic_structure
{
    t_bitboard								mask;
    t_magic									magic;
};

//===========================================================//
// Move Ordering
//===========================================================//
#define MAX_CHESS_INT						2147483647
#define MOVE_ORDER_HASH						(MAX_CHESS_INT >> 1)
#define MOVE_ORDER_CAPTURE					(MAX_CHESS_INT >> 2)
#define MOVE_ORDER_KILLER1					(MAX_CHESS_INT >> 3)
#define MOVE_ORDER_KILLER2					(MAX_CHESS_INT >> 4)
#define MOVE_ORDER_KILLER3					(MAX_CHESS_INT >> 7)
#define MOVE_ORDER_KILLER4					(MAX_CHESS_INT >> 8)
#define MOVE_ORDER_ETC						(MAX_CHESS_INT >> 5)
#define MOVE_ORDER_REFUTATION				(MAX_CHESS_INT >> 6)

//===========================================================//
// Multi-PV
//===========================================================//
struct t_pv_record {
    t_chess_value							score;
    int										pv_length;
    struct t_move_record					*move[MAXPLY];
};

struct t_multi_pv {
    int										count;
    struct t_pv_record						pv[128];
};

//===========================================================//
// Squares
//===========================================================//
#define A1 0
#define B1 1
#define C1 2
#define D1 3
#define E1 4
#define F1 5
#define G1 6
#define H1 7
#define A2 8
#define B2 9
#define C2 10
#define D2 11
#define E2 12
#define F2 13
#define G2 14
#define H2 15
#define A3 16
#define B3 17
#define C3 18
#define D3 19
#define E3 20
#define F3 21
#define G3 22
#define H3 23
#define A4 24
#define B4 25
#define C4 26
#define D4 27
#define E4 28
#define F4 29
#define G4 30
#define H4 31
#define A5 32
#define B5 33
#define C5 34
#define D5 35
#define E5 36
#define F5 37
#define G5 38
#define H5 39
#define A6 40
#define B6 41
#define C6 42
#define D6 43
#define E6 44
#define F6 45
#define G6 46
#define H6 47
#define A7 48
#define B7 49
#define C7 50
#define D7 51
#define E7 52
#define F7 53
#define G7 54
#define H7 55
#define A8 56
#define B8 57
#define C8 58
#define D8 59
#define E8 60
#define F8 61
#define G8 62
#define H8 63

#define B8H1									0xfefefefefefefefe
#define A8G1									0x7f7f7f7f7f7f7f7f

#define BITBOARD_KINGSIDE						0xE0E0E0E0E0E0E0E0
#define BITBOARD_QUEENSIDE						0x0707070707070707

#define BITBOARDS_CENTER						0x00003C3C3C3C0000
#define BITBOARDS_RING							0x007E424242427E00
#define BITBOARDS_EDGE							0xff818181818181FF

#define INDEX_CHECK(index, array)  assert((index) >= 0 &&  (index) < sizeof array / sizeof array[0]);

//================================================================
// UCI Information Strings
//================================================================
#if _WIN32
#define NODE_FORMAT								"%I64u"
#else
#define NODE_FORMAT								"%llu"
#endif

#define INFO_STRING_ABORT						"Abort **NOW** : Abort Time = %d, Search Time = %d, Nodes = " NODE_FORMAT
#define INFO_STRING_CHECKMATE					"info score mate %d time %ld depth %d seldepth %d nodes " NODE_FORMAT " pv "
#define INFO_STRING_SCORE						"info score cp %d time %ld depth %d seldepth %d nodes " NODE_FORMAT " pv "
#define INFO_STRING_FAIL_HIGH_MATE				"info score mate %d lowerbound time %ld depth %d seldepth %d nodes " NODE_FORMAT " pv "
#define INFO_STRING_FAIL_HIGH_SCORE				"info score cp %d lowerbound time %ld depth %d seldepth %d nodes " NODE_FORMAT " pv "
#define INFO_STRING_FAIL_LOW_MATE				"info score mate %d upperbound time %ld depth %d seldepth %d nodes " NODE_FORMAT " pv "
#define INFO_STRING_FAIL_LOW_SCORE				"info score cp %d upperbound time %ld depth %d seldepth %d nodes " NODE_FORMAT " pv "
#define INFO_STRING_SEND_NODES					"info nodes " NODE_FORMAT " nps " NODE_FORMAT "\n"
#define INFO_STRING_SEND_HASH_FULL				"info hashfull " NODE_FORMAT "\n"
#define INFO_STRING_PERFT_SPEED					"Total Nodes: " NODE_FORMAT " in %d milliseconds = nps " NODE_FORMAT "\n"
#define INFO_STRING_PERFT_NODES					"Total Nodes: " NODE_FORMAT "\n"

