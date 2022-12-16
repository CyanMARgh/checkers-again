#include <cstdint>
#include <vector>
#include <stack>
#include <cstdio>
#include <ncurses.h>
#include <cstdlib>
#include <string>
#include <vector>

typedef uint8_t u8;
typedef int32_t s32;
typedef uint32_t u32;

std::string log_text = "";

struct ivec2 {
	s32 x = 0, y = 0;
};
ivec2 operator+(ivec2 a, ivec2 b) {
	return {a.x + b.x, a.y + b.y};
}
ivec2& operator+=(ivec2& a, ivec2 b) {
	a.x += b.x, a.y += b.y;
	return a;
}
ivec2 operator-(ivec2 a, ivec2 b) {
	return {a.x - b.x, a.y - b.y};
}
ivec2& operator-=(ivec2& a, ivec2 b) {
	a.x -= b.x, a.y -= b.y;
	return a;
}


typedef u8 Cell;
namespace Cells {
	const Cell
		NONE = 0,
		X    = 1,
		O    = 2;
}
enum class TTT_State {
	X_STEP,
	X_LOSE,
	X_DRAW,

	O_STEP,
	O_LOSE,
	O_DRAW
};
struct Action {
	ivec2 pos = {0, 0};
	Cell symbol = Cells::NONE;
};
const s32 board_size = 10, line_size = 5;
struct TTT_Board {	
	Cell cells[board_size][board_size];
	TTT_State state;
	u32 placed;
	float value = 0.;

	std::stack<Action> history;

	Cell& at(ivec2 pos) {
		return cells[pos.y][pos.x];
	}

	void reset() {
		for(u32 y = 0; y < board_size; y++) {
			for(u32 x = 0; x < board_size; x++) {
				cells[y][x] = Cells::NONE;
			}			
		}
		placed = 0;
		state = TTT_State::X_STEP;
		history = {};
		value = 0.;
	}

	TTT_Board() {
		reset();
	}

	// static float seq_cost(u32 l) {
	// 	return l >= line_size ? 1.e9 : l * l;
	// }
	// float find_val_part(Cell cell) {
	// 	for()
	// }
	float find_val() {
		// -> float
		// sum of (line_length_i^2) - same for crosses
		// +-inf, if there are line_legth_i >= line_size
		// TODO search only near to last placed sign

		Cell c = (state == TTT_State::X_STEP) ? Cells::O : Cells::X;
		// searching only for enemy
		// hor
		for(u32 y0 = 0; y0 < board_size; y0++) {
			for(u32 x0 = 0; x0 < board_size - line_size + 1; x0++) {				
				bool ok = true;
				for(u32 di = 0; di < line_size; di++) {
					if(cells[y0][x0 + di] != c) {
						ok = false; break;
					}
				}
				if(ok) return -1.;
			}
		}
		// vert
		for(u32 y0 = 0; y0 < board_size - line_size + 1; y0++) {
			for(u32 x0 = 0; x0 < board_size; x0++) {				
				bool ok = true;
				for(u32 di = 0; di < line_size; di++) {
					if(cells[y0 + di][x0] != c) {
						ok = false; break;
					}
				}
				if(ok) return -1;
			}
		}
		// diagonal
		for(u32 y0 = 0; y0 < board_size - line_size + 1; y0++) {
			for(u32 x0 = 0; x0 < board_size - line_size + 1; x0++) {				
				bool ok = true;
				for(u32 di = 0; di < line_size; di++) {
					if(cells[y0 + di][x0 + di] != c) {
						ok = false; break;
					}
				}
				if(ok) return -1;
			}
		}
		return 0;
	}
	void apply(Action a) {
		cells[a.pos.y][a.pos.x] = a.symbol;	placed++;
		state = (state == TTT_State::O_STEP) ? TTT_State::X_STEP : TTT_State::O_STEP;

		value = find_val();
		if(value < 0) {
			state = (state == TTT_State::O_STEP) ? TTT_State::O_LOSE : TTT_State::X_LOSE;
		} else if(placed == board_size * board_size) {
			state = (state == TTT_State::O_STEP) ? TTT_State::O_DRAW : TTT_State::X_DRAW;
		}
		// log_text = "value = " + std::to_string(value);
		history.push(a);
	}
	void undo(Action a) {
		cells[a.pos.y][a.pos.x] = Cells::NONE;
		state =
			(state == TTT_State::X_STEP || state == TTT_State::X_LOSE || state == TTT_State::X_DRAW) ?
				TTT_State::O_STEP
			:
				TTT_State::X_STEP;
		placed--;
		value = 0;
	}
	void undo() {
		if(history.empty()) {
			log_text = "can't undo!";
			return;
		}
		Action a = history.top();
		history.pop();
		undo(a);
	}
	// should return Step / []Action
	std::pair<bool, Action> get_step(ivec2 pos) {
		if(at(pos) == Cells::NONE && !is_final()) {
			return {true, {pos, state == TTT_State::X_STEP ? Cells::X : Cells::O}};
		} else {
			return {false, {}};
		}
	}	
	std::vector<Action> get_all_steps() {
		std::vector<Action> result;
		Cell symbol;
		if(state == TTT_State::X_STEP) {
			symbol = Cells::X;
		} else if(state == TTT_State::O_STEP) {
			symbol = Cells::O;
		} else {
			return {};
		}
		for(s32 y = 0; y < board_size; y++) {
			for(s32 x = 0; x < board_size; x++) {
				if(cells[y][x] == Cells::NONE) {
					result.push_back(Action{{x, y}, symbol});
				}
			}
		}
		return result;
	}
	bool is_final() {
		return state != TTT_State::X_STEP && state != TTT_State::O_STEP;
	}
	std::pair<float, Action> find_val_and_best_step(u32 depth) {
		//TODO alpha-beta, cache
		if(depth == 0 || is_final()) {
			return {value, {}};
		} else {
			auto steps = get_all_steps();
			// log_text = "(s = " + std::to_string(steps.size()) + ") ";
			Action best;
			float best_val = -1.e10;
			for(auto s : steps) {
				apply(s);
				float val = -find_val_and_best_step(depth - 1).first;
				log_text += std::to_string(val) + " ";
				if(val > best_val) {
					best_val = val;
					best = s;
				}
				undo();
			}
			// log_text = " -> " + std::to_string(steps.size());
			return {best_val, best};
		}
		log_text = "invalid state!";
	}
};
s32 clamp(s32 x, s32 a, s32 b) {
	return x < a ? a : x > b ? b : x;
}

struct TTT_Board_UI {
	TTT_Board board;
	ivec2 cursor = {0, 0};
	void move_cursor(ivec2 v) {
		cursor += v;
		cursor.x = clamp(cursor.x, 0, board_size - 1);
		cursor.y = clamp(cursor.y, 0, board_size - 1);
	}
	bool click() {
		auto [ok, action] = board.get_step(cursor);
		if(ok) {
			board.apply(action);
		}
		return ok;
	}
};
void draw_board(TTT_Board_UI* board_ui, WINDOW* visual_board) {
	char symbols[28] = "         \\ / X / \\/-\\| |\\-/";
	for(u32 y = 0; y < board_size * 3; y++) {
		for(u32 x = 0; x < board_size * 3; x++) {
			if(
					y / 3 == board_ui->cursor.y
				&&	x / 3 == board_ui->cursor.x
			) {
				wattr_on(visual_board, COLOR_PAIR(2), nullptr);
			} else {
				wattr_on(visual_board, COLOR_PAIR(1), nullptr);
			}

			u32 I = board_ui->board.cells[y / 3][x / 3];
			mvwaddch(visual_board, y + 1, x + 1, symbols[
				I * 9
				+ (y % 3) * 3
				+ x % 3
			]);
		}			
	}	
}
void init_ncurses() {
	initscr();
	cbreak();
	noecho();
	curs_set(0);
	set_escdelay(25);

	if(!has_colors()) {
		exit(-1);
	}
	start_color();

	init_pair(1, COLOR_WHITE, COLOR_BLACK);
	init_pair(2, COLOR_WHITE, COLOR_GREEN);
}

void clear_line(WINDOW *win, u32 row, u32 length) {
	wmove(win, row, 1);
	for(u32 i = 0; i < length - 2; i++) { waddch(win, ' '); }
}

int main() {
	init_ncurses();

	ivec2 max_size;
	getmaxyx(stdscr, max_size.y, max_size.x);
	WINDOW *log_win = newwin(max_size.y, 30, 0, max_size.x - 30);
	u32 win_board_size = board_size * 3 + 2;
	WINDOW *ttt_win = newwin(win_board_size, win_board_size, (max_size.y - win_board_size) / 2, (max_size.x - win_board_size - 30) / 2);
	WINDOW *end_win = newwin(5, win_board_size + 10, (max_size.y - win_board_size) / 2 + win_board_size, (max_size.x - win_board_size - 30) / 2 - 5);

	TTT_Board_UI ttt_board_ui;

	box(log_win, 0, 0);
	box(ttt_win, 0, 0);
	box(end_win, 0, 0);

	keypad(ttt_win, true);

	s32 input = -1;
	while(1) {
		mvwprintw(log_win, 1, 1, "winsize.x  = %4d", max_size.x);
		mvwprintw(log_win, 2, 1, "winsize.y  = %4d", max_size.y);
		mvwprintw(log_win, 3, 1, "cursor.x   = %4d", ttt_board_ui.cursor.x);
		mvwprintw(log_win, 4, 1, "cursor.y   = %4d", ttt_board_ui.cursor.y);
		mvwprintw(log_win, 5, 1, "input code = %4d", input);
		clear_line(log_win, 6, 30);
		mvwprintw(log_win, 6, 1, "%s", log_text.c_str());

		draw_board(&ttt_board_ui, ttt_win);

		{
			auto s = ttt_board_ui.board.state;
			const char* msg_str = 
				s == TTT_State::X_STEP ?
					"crosses step"
				: s == TTT_State::X_LOSE ?
					"zeros won!"
				: s == TTT_State::X_DRAW ? 
					"draw! (x)"
				: s == TTT_State::O_STEP ?
					"zeros step"
				: s == TTT_State::O_LOSE ? 
					"crosses won!"
				:
					"draw! (o)";
			clear_line(end_win, 2, win_board_size + 10);
			mvwprintw(end_win, 2, 2, "%s", msg_str);
		}

		refresh();
		wrefresh(log_win);
		wrefresh(ttt_win);
		wrefresh(end_win);

		input = wgetch(ttt_win);
		if(input == 27) {
			break; 
		} else if(input == 10) {
			auto [] = ttt_board_ui.click();
					log_text = "value = " + std::to_string(ttt_board_ui.board.find_val_and_best_step(2).first);
		} else if(input == 'R' || input == 'r') {
			ttt_board_ui.board.reset();
		} else if (input == 'U' || input == 'u') {
			ttt_board_ui.board.undo();
		} else if(input == 'a' || input == 'A' && !ttt_board_ui.board.is_final()) {			
			auto [val, act] = ttt_board_ui.board.find_val_and_best_step(3);
			ttt_board_ui.board.apply(act);
			// log_text = "Ai:";
		} else {
			ivec2 delta = 
				input == KEY_UP    ? ivec2{ 0, -1} :
				input == KEY_DOWN  ? ivec2{ 0,  1} :
				input == KEY_LEFT  ? ivec2{-1,  0} :
				input == KEY_RIGHT ? ivec2{ 1,  0} :
				ivec2{0, 0};
				ttt_board_ui.move_cursor(delta);
		}
	}

	delwin(log_win);
	delwin(ttt_win);
	delwin(end_win);

	endwin();
	return 0;
}