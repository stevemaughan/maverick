//===========================================================//
//
// Maverick Chess Engine
// Copyright 2013-2015 Steve Maughan
//
//===========================================================//


//-- Pawns
#define MG_DOUBLE_PAWN					-10
#define EG_DOUBLE_PAWN					-25

#define MG_ISOLATED_PAWN				-15
#define EG_ISOLATED_PAWN				-25

//-- Piece Values
#define MG_PAWN_VALUE					100
#define EG_PAWN_VALUE					120

#define MG_KNIGHT_VALUE					350
#define EG_KNIGHT_VALUE					330

#define MG_BISHOP_VALUE					350
#define EG_BISHOP_VALUE					360

#define MG_ROOK_VALUE					525
#define EG_ROOK_VALUE					550

#define MG_QUEEN_VALUE					1000
#define EG_QUEEN_VALUE					1050

//-- Bishop Pair
#define MG_BISHOP_PAIR					25
#define EG_BISHOP_PAIR					65

//-- Mobility
#define ROOK_MOBILITY_MIDDLEGAME		5
#define ROOK_MOBILITY_ENDGAME			10

//-- Connected Knights
#define MG_CONNECTED_KNIGHTS			10
#define EG_CONNECTED_KNIGHTS			15

#define MG_ROOK_BEHIND_PASSED_PAWN		5
#define EG_ROOK_BEHIND_PASSED_PAWN		15

//-- Rook on the 7th
#define MG_ROOK_ON_7TH					10
#define EG_ROOK_ON_7TH					5

//-- Rooks on open file
#define MG_ROOK_ON_OPEN_FILE			2
#define MG_ROOK_ON_SEMI_OPEN_FILE		1

//-- Pawn Shield
#define MG_INTACT_PAWN_SHIELD			15
#define MG_PAWN_SHIELD_WRECKED			-45
#define MG_F6_PAWN_WEDGE				25
#define MG_BACKWARD_PAWN				-5