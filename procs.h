//===========================================================//
//
// Maverick Chess Engine
// Copyright 2013-2015 Steve Maughan
//
//===========================================================//

// uci.c
void create_uci_engine_thread();
void listen_for_uci_input();
unsigned __stdcall engine_loop(void* pArguments);
void uci_set_author();
void uci_set_mode();
void uci_isready();
void send_command(char *t);
BOOL is_search_complete(struct t_board *board, int score, int ply, struct t_move_list *move_list);
void uci_go(char *s);
void set_uci_level(char *s, t_chess_color color);
void set_uci_time_to_move(t_chess_color color);
void do_uci_new_pv(struct t_board *board, int score, int depth);
void uci_position(struct t_board *board, char *s);
void do_uci_bestmove(struct t_board *board);
void do_uci_consider_move(struct t_board *board, int depth);
void do_uci_hash_full();
void do_uci_send_nodes();
void uci_current_line(struct t_board *board, int ply);
void uci_stop();
void do_uci_depth();
void do_uci_fail_high(struct t_board *board, int score, int depth);
void do_uci_fail_low(struct t_board *board, int score, int depth);
void uci_ponderhit();
void uci_check_status(struct t_board *board, int ply);
void uci_setoption(char *s);
void uci_current_line(struct t_board *board);
void do_uci_show_stats();
void send_info(char *s);
void uci_new_game(struct t_board *board);
void uci_set_predicted_hash(struct t_board *board);
void init_engine(struct t_board *board);
void uci_send_state(char *c);
void uci_set_debug(char *s);

// utils.c
unsigned long time_now();
int index_of(char *substr, char *s);
int number_index(int index, char *s);
char *word_index(int index, char *s);
int word_count(char *s);
t_hash rand64();
void qsort_moves(struct t_move_list *move_list, int first, int last);
char *leftstr(char *s, int index);

// board.c
void update_in_check(struct t_board *board, t_chess_square from_square, t_chess_square to_square, t_chess_color color);
BOOL is_pinned(struct t_board *board, t_chess_square square, t_chess_square pinned_to);
t_chess_square who_is_attacking_square(struct t_board *board, t_chess_square square, t_chess_color color);
int attack_count(struct t_board *board, t_chess_square square, t_chess_color color);
BOOL is_in_check(struct t_board *board, t_chess_color color);
//BOOL is_in_check_after_move(struct t_board *board, struct t_move_record *move);
BOOL is_square_attacked(struct t_board *board, t_chess_square square, t_chess_color color);
void init_board(struct t_board *board);
void add_piece(struct t_board *board, t_chess_piece piece, t_chess_square target_square);
void clear_board(struct t_board *board);
void new_game(struct t_board *board);
t_chess_square name_to_index(char *name);
char *move_as_str(struct t_move_record *move);
t_chess_square kingside_rook(struct t_board *board, t_chess_color color);
t_chess_square queenside_rook(struct t_board *board, t_chess_color color);
BOOL integrity(struct t_board *board);
void init_can_move();
void init_perft_pv_data();

// movedirectory.c
void init_move_directory();
t_move_record *lookup_move(struct t_board *board, char *move_string);
void configure_castling();
void configure_pawn_push(int *i);
void configure_pawn_capture(int *i);
void configure_piece_moves(int *i);
void init_directory_castling_delta();
void clear_history();
void init_960_castling(struct t_board *board, t_chess_square king_square, t_chess_square rook_square);

// fen.c
void set_fen(struct t_board *board, char *epd);
char *get_fen(struct t_board *board);

//--bitboard.c
void init_bitboards();
void init_magic();
void init_unstoppable_pawn_mask();

t_bitboard index_to_bitboard(t_bitboard mask, int index);
t_bitboard create_rook_mask(t_chess_square s);
t_bitboard create_rook_attacks(t_chess_square s, t_bitboard mask);
t_bitboard create_bishop_mask(t_chess_square s);
t_bitboard create_bishop_attacks(t_chess_square s, t_bitboard mask);
void flip_board(struct t_board *board);

//--Hash Table
t_hash calc_board_hash(struct t_board *board);
void init_hash();
void destroy_hash();
void set_hash(unsigned int size);
void poke(t_hash hash_key, t_chess_value score, int ply, int depth, t_hash_bound bound, struct t_move_record *move);
void poke_draw(t_hash hash_key);
struct t_hash_record *probe(t_hash hash_key);
void clear_hash();
t_chess_value get_hash_score(struct t_hash_record *hash_record, int ply);

//--Pawn Hash Table Routines
void set_pawn_hash(unsigned int size);
void init_pawn_hash();
void destroy_pawn_hash();
struct t_pawn_hash_record *lookup_pawn_hash(struct t_board *board, struct t_chess_eval *eval);
t_hash calc_pawn_hash(struct t_board *board);
void eval_pawn_shelter(struct t_board *board, struct t_pawn_hash_record *pawn_record);
void eval_pawn_storm(struct t_board *board, struct t_pawn_hash_record *pawn_record);
void evaluate_king_pawn_endgame(struct t_board *board, struct t_pawn_hash_record *pawn_record);

//-- Static Exchange Evaluation (see.cpp)
BOOL see(struct t_board *board, struct t_move_record *move, t_chess_value threshold);
BOOL see_square(struct t_board *board, t_chess_square to_square, t_chess_value threshold);

//-- Root Search (root.c)
void root_search(struct t_board *board);

//-- Search.c
t_chess_value alphabeta(struct t_board *board, int ply, int depth, t_chess_value alpha, t_chess_value beta);
t_chess_value qsearch_plus(struct t_board *board, int ply, int depth, t_chess_value alpha, t_chess_value beta);
t_chess_value qsearch(struct t_board *board, int ply, int depth, t_chess_value alpha, t_chess_value beta);
t_chess_value alphabeta_tip(struct t_board *board, int ply, int depth, t_chess_value alpha, BOOL *fail_low);

//--Generate Moves
void generate_legal_moves(struct t_board *board, struct t_move_list *move_list);
void generate_moves(struct t_board *board, struct t_move_list *move_list);
void generate_evade_check(struct t_board *board, struct t_move_list *move_list);
void generate_captures(struct t_board *board, struct t_move_list *move_list);
void generate_quiet_checks(struct t_board *board, struct t_move_list *move_list);
void generate_no_capture_no_checks(struct t_board *board, struct t_move_list *move_list);
void generate_quiet_moves(struct t_board *board, struct t_move_list *move_list);

//-- Move List Routines (movelist.cpp)
void reset_move_list_scores(struct t_move_list *move_list);
void new_best_move(struct t_move_list *move_list, int i);
void update_move_value(struct t_move_record *move, struct t_move_list *move_list, t_nodes n);

//-- Order Moves (moveorder.cpp)
void order_moves(struct t_board *board, struct t_move_list *move_list, int ply);
void order_captures(struct t_board *board, struct t_move_list *move_list);
void order_evade_check(struct t_board *board, struct t_move_list *move_list, int ply);
void age_history_scores();
void update_killers(struct t_pv_data *pv, int depth);
void update_check_killers(struct t_pv_data *pv, int depth);
BOOL pv_not_resolved(int legal_moves_played, t_chess_value e, t_chess_value alpha, t_chess_value beta);

//-- Principle Variation (pv.cpp)
void update_best_line(struct t_board *board, int ply);
void update_best_line_from_hash(struct t_board *board, int ply);

//-- Move List Manipulation
BOOL move_list_integrity(struct t_board *board, struct t_move_list *move_list);
int legal_move_count(struct t_board *board, struct t_move_list *move_list);
BOOL equal_move_lists(struct t_move_list *move_list1, struct t_move_list *move_list2);
BOOL is_move_in_list(struct t_move_record *move, struct t_move_list *move_list);

//--Make Moves (make.c)
BOOL make_move(struct t_board *board, t_bitboard pinned, struct t_move_record *move, struct t_undo *undo);
void unmake_move(struct t_board *board, struct t_undo *undo);
void make_game_move(struct t_board *board, char *s);
BOOL make_next_see_positive_move(struct t_board *board, struct t_move_list *move_list, t_chess_value see_margin, struct t_undo *undo);
BOOL make_next_best_move(struct t_board *board, struct t_move_list *move_list, struct t_undo *undo);
void make_null_move(struct t_board *board, struct t_undo *undo);
void unmake_null_move(struct t_board *board, struct t_undo *undo);
BOOL make_next_move(struct t_board *board, struct t_move_list *move_list, struct t_move_list *bad_move_list, struct t_undo *undo);
BOOL simple_make_next_move(struct t_board *board, struct t_move_list *move_list, struct t_undo *undo);
BOOL is_move_legal(struct t_board *board, struct t_move_record *move);

//--Evaluate the Board (eval.cpp)
t_chess_value evaluate(struct t_board *board, struct t_chess_eval *eval);
inline t_chess_value calc_evaluation(struct t_board *board, struct t_chess_eval *eval);
inline void calc_game_phase(struct t_board *board, struct t_chess_eval *eval);
inline void calc_pawn_value(struct t_board *board, struct t_chess_eval *eval);
inline void calc_piece_value(struct t_board *board, struct t_chess_eval *eval);
inline void calc_passed_pawns(struct t_board *board, struct t_chess_eval *eval);
inline void calc_king_safety(struct t_board *board, struct t_chess_eval *eval);
t_chess_value calc_king_pawn_endgame(struct t_board *board, struct t_chess_eval *eval);
inline BOOL known_ending(struct t_board *board, t_chess_value *score);
void init_eval_function();
void init_eval(struct t_chess_eval *eval);

//-- Know Endgames
void known_endgame_insufficient_material(struct t_board *board, struct t_chess_eval *eval);
void known_endgame_QKvk(struct t_board *board, struct t_chess_eval *eval);
void known_endgame_RKvk(struct t_board *board, struct t_chess_eval *eval);
void known_endgame_BBKvk(struct t_board *board, struct t_chess_eval *eval);
void known_endgame_BNKvk(struct t_board *board, struct t_chess_eval *eval);
void known_endgame_Kvqk(struct t_board *board, struct t_chess_eval *eval);
void known_endgame_Kvrk(struct t_board *board, struct t_chess_eval *eval);
void known_endgame_Kvbbk(struct t_board *board, struct t_chess_eval *eval);
void known_endgame_Kvbnk(struct t_board *board, struct t_chess_eval *eval);
void known_endgame_KRNvk(struct t_board *board, struct t_chess_eval *eval);
void known_endgame_Kvkrn(struct t_board *board, struct t_chess_eval *eval);
void known_endgame_KRBvk(struct t_board *board, struct t_chess_eval *eval);
void known_endgame_Kvkrb(struct t_board *board, struct t_chess_eval *eval);
void known_endgame_KNvkp(struct t_board *board, struct t_chess_eval *eval);
void known_endgame_KPvkn(struct t_board *board, struct t_chess_eval *eval);
void known_endgame_KBvkp(struct t_board *board, struct t_chess_eval *eval);
void known_endgame_KPvkb(struct t_board *board, struct t_chess_eval *eval);
void known_endgame_KRvkn(struct t_board *board, struct t_chess_eval *eval);
void known_endgame_KNvkr(struct t_board *board, struct t_chess_eval *eval);
void known_endgame_KRvkb(struct t_board *board, struct t_chess_eval *eval);
void known_endgame_KBvkr(struct t_board *board, struct t_chess_eval *eval);
void known_endgame_KRBvkr(struct t_board *board, struct t_chess_eval *eval);
void known_endgame_KRvkrb(struct t_board *board, struct t_chess_eval *eval);
void known_endgame_KRNvkr(struct t_board *board, struct t_chess_eval *eval);
void known_endgame_KRvkrn(struct t_board *board, struct t_chess_eval *eval);
void known_endgame_KPvk(struct t_board *board, struct t_chess_eval *eval);
void known_endgame_Kvkp(struct t_board *board, struct t_chess_eval *eval);

//-- Material Hash Routines (materialhash.cpp)
BOOL fill_material_hash();
void init_material_hash();
void destroy_material_hash();
t_hash calc_material_hash(struct t_board *board);

//--Write to Disc
void write_board(struct t_board *board, char filename[1024]);
void write_move_list(struct t_move_list *move_list, char filename[1024]);
void write_path(struct t_board *board, int ply, char filename[1024]);
void write_tree(struct t_board *board, struct t_move_record *move, BOOL append, char filename[1024]);
void write_log(char *s, char *filename, BOOL append, BOOL send);

//--Test Routines
void test_procedure();
BOOL test_fen();
BOOL test_bitscan();
BOOL test_bittwiddles();
BOOL test_genmove();
BOOL test_make_unmake();
BOOL test_perft();
BOOL test_hash();
BOOL test_eval();
BOOL test_capture_gen();
BOOL test_check_gen();
BOOL test_alt_move_gen();
BOOL test_see();
BOOL test_position();
BOOL test_search();
BOOL test_book();
BOOL test_hash_table();
BOOL test_ep_capture();
BOOL test_perft960();

//--Perft
t_nodes perft(struct t_board *board, int depth);
t_nodes do_perft(struct t_board *board, int depth);

//-- Opening Book (openingbook.c)
int book_count();
char *book_string();
void close_book();
void set_own_book(BOOL value);
void set_opening_book(char *book);
t_move_record *probe_book(struct t_board *board);
int book_move_count(struct t_board *board);
void fill_opening_move_list(struct t_board *board, struct t_move_list *move_list);

//-- Draw Routines (draw.cpp)
BOOL repetition_draw(struct t_board *board);
BOOL is_checkmate(struct t_board *board);
BOOL is_stalemate(struct t_board *board);
