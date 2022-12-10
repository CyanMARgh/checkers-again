#include <cstdint>
#include <vector>
#include <stack>
#include <cstdio>
#include <ncurses.h>
#include <cstdlib>
#include <string>

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

	O_STEP,
	O_LOSE,
	DRAW
};
struct Action {
	ivec2 pos;
	Cell symbol;
};
const s32 board_size = 3, line_size = 3;
struct TTT_Board {	
	Cell cells[board_size][board_size];
	TTT_State state;
	u32 placed;

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
	}

	TTT_Board() {
		reset();
	}
	s32 get_val() {
		//search only near to last placed sign

		// hor
		for(u32 y0 = 0; y0 < board_size; y0++) {
			for(u32 x0 = 0; x0 < board_size - line_size + 1; x0++) {				
				bool ok = true;
				for(u32 dx = 0; dx < line_size - 1; dx++) {
					if(cells[y0][x0 + dx] == Cells::NONE || cells[y0][x0 + dx] != cells[y0][x0 + dx + 1]) {
						ok = false; break;
					}
				}
				if(ok) return cells[y0][x0] == Cells::X ? 1 : -1;
			}
		}
		// vert
		for(u32 y0 = 0; y0 < board_size - line_size + 1; y0++) {
			for(u32 x0 = 0; x0 < board_size; x0++) {				
				bool ok = true;
				for(u32 dy = 0; dy < line_size - 1; dy++) {
					if(cells[y0 + dy][x0] == Cells::NONE || cells[y0 + dy][x0] != cells[y0 + dy + 1][x0]) {
						ok = false; break;
					}
				}
				if(ok) return cells[y0][x0] == Cells::X ? 1 : -1;
			}
		}
		// diagonal
		for(u32 y0 = 0; y0 < board_size - line_size + 1; y0++) {
			for(u32 x0 = 0; x0 < board_size - line_size + 1; x0++) {				
				bool ok = true;
				for(u32 di = 0; di < line_size - 1; di++) {
					if(cells[y0 + di][x0 + di] == Cells::NONE || cells[y0 + di][x0 + di] != cells[y0 + di + 1][x0 + di + 1]) {
						ok = false; break;
					}
				}
				if(ok) return cells[y0][x0] == Cells::X ? 1 : -1;
			}
		}
		return 0;
	}
	void apply(Action a) {
		cells[a.pos.y][a.pos.x] = a.symbol;
		s32 val = get_val();
		log_text = "get_val() = " + std::to_string(val);
		state = 
			placed == board_size * board_size ? 
				TTT_State::DRAW
			: val > 0 ?
				TTT_State::O_LOSE
			: val < 0 ?
				TTT_State::X_LOSE
			: state == TTT_State::X_STEP ?
				TTT_State::O_STEP
			: 
				TTT_State::X_STEP;
		placed++;
		history.push(a);
	}
	void undo(Action a) {
		cells[a.pos.y][a.pos.x] = Cells::NONE;
		state =
			(state == TTT_State::X_STEP || state == TTT_State::X_LOSE) ?
				TTT_State::O_STEP
			:
				TTT_State::X_STEP;
		placed--;
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
		if(at(pos) == Cells::NONE && (state == TTT_State::X_STEP || state == TTT_State::O_STEP)) {
			return {true, {pos, state == TTT_State::X_STEP ? Cells::X : Cells::O}};
		} else {
			return {false, {}};
		}
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
	void click() {
		auto [ok, action] = board.get_step(cursor);
		log_text = "(1)";
		if(ok) {
			log_text = "(2)";
			board.apply(action);
		}
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
				s == TTT_State::O_LOSE ?
					"crosses won!"
				: s == TTT_State::X_LOSE ?
					"zeros won!"
				: s == TTT_State::DRAW ? 
					"draw!"
				: s == TTT_State::X_STEP ?
					"crosses step"
				:
					"zeros step";
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
			ttt_board_ui.click();
		} else if(input == 'R' || input == 'r') {
			ttt_board_ui.board.reset();
		} else if (input == 'U' || input == 'u') {
			ttt_board_ui.board.undo();
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