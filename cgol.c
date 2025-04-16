#include <stdlib.h>   // malloc, free
#include <stdio.h>    // printf
#include <unistd.h>   // usleep
#include <ncurses.h>
#include <argp.h>
#include <string.h>
#include <limits.h> // ULLONG_MAX etc.

/*
 * Todo:
 * History of multiple frames, smarter loop detection?
 * If no history, no loop detection
 */

#define esc 27

#define MICROSECONDS_IN_SECOND 1000000
#define INIT_AMOUNT 12
#define INIT_TICKS 5
#define MAX_TICKS 240

// Settings for argp
const char *ARGP_PROGRAM_VERSION = "cgol 0.1";
const char *ARGP_PROGRAM_BUG_ADDRESS = "<ranta dot lauri at gmail dot com>";
static char doc[] = "CGOL -- Conway's Game of Life in C\
		     \vThis program comes with absolutely no warranty, it's just a hobby project.\n\n\
Colors:\n\
  black   0\n\
  red     1\n\
  green   2\n\
  yellow  3\n\
  blue    4\n\
  magenta 5\n\
  cyan    6\n\
  white   7\n\
  ...\n\n\
Controls:\n\
  c: clear and reset the screen\n\
  f: change color\n\
  F: change color in reverse order\n\
  esc|q|enter: quit\n\n";

static char args_doc[] = "";

static struct argp_option options[] = {
  {"color_list", 'c', "0,1,...,255", 0, "List of colors (D=0,...,7)"},
  {"ticks_limit",'r', "[0-]", 0, "Ticks before reset, 0 means no reset (D=240)"},
  {"init_ticks", 'i', "[0-]", 0, "Ticks to advance before first print (D=5)"},
  {"init_prob",  'p', "[0-100]", 0, "Cell initialization probability in % (D=12)"},
  {"fps",        'f', "[0-]", 0, "Frames per second, 0 means no delay (D=30)"},
  {"char_alive", 'l', "[CHAR]", 0, "Char for the cells (D=█)"},
  { 0 }
};

struct arguments
{
  char *arg1;
  char **strings;
  int fps;
  int color;
  int ticks_limit;
  int init_ticks;
  float init_prob;
  char char_alive;
  char *color_list;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
  struct arguments *arguments = state->input;

  switch (key)
    {
    case 'r':
      arguments->ticks_limit = arg ? atoi (arg) : MAX_TICKS;
      break;
    case 'i':
      arguments->init_ticks = arg ? atoi (arg) : INIT_TICKS;
      break;
    case 'p':
      arguments->init_prob = arg ? atoi (arg) : INIT_AMOUNT;
      break;
    case 'f':
      arguments->fps = arg ? atoi (arg) : 30;
      break;
    case 'l':
      arguments->char_alive = arg ? *arg : '\0';
      break;
    case 'c':
      arguments->color_list = arg;
      break;

    case ARGP_KEY_NO_ARGS:
      break;

    case ARGP_KEY_ARG:
      arguments->arg1 = arg;
      arguments->strings = &state->argv[state->next];
      state->next = state->argc;
      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc };
// End of argp settings

void swap_array(uint_fast8_t **a, uint_fast8_t **b)
{
    uint_fast8_t *tmp = *a;
    *a = *b;
    *b = tmp;
}

void print_matrix(uint_fast8_t **arr, int w, int h, char char_alive)
{
	for (int j = 0; j < h; j++) {
		for (int i = 0; i < w; i++) {
			// This is hacky, would be better to have ' '|A_REVERSE as the default but how?
			if (char_alive != '\0') {
				((*arr)[j * w + i]) ? mvaddch(j,i,char_alive) : mvprintw(j,i," ");
			} else {
				((*arr)[j * w + i]) ? mvaddch(j,i,' '|A_REVERSE) : mvprintw(j,i," ");
			}
		}
	}
	refresh();
}

int release_field(uint_fast8_t **arr)
{
	free(*arr);
	return 0;
}

uint_fast8_t *create_field(int length)
{
	uint_fast8_t *arr;
	arr = calloc(length, sizeof(uint_fast8_t));
	if (!arr) {
		free(arr);
		exit(1);
		}
	return arr;
}

uint_fast8_t *realloc_field(uint_fast8_t **arr, int length)
{
	uint_fast8_t *new_arr = realloc(*arr, sizeof(uint_fast8_t) * length);
	if (!new_arr) {
		free(*arr);
		free(new_arr);
		exit(1);
		}
	return new_arr;
}

static int check_state(uint_fast8_t **arr, int w, int h, int x, int y)
{
	// Matrix
	int k[2][8] = {
		{x-1, x, x+1, x-1, x+1, x-1, x, x+1},
		{y-1, y-1, y-1, y, y, y+1, y+1, y+1}
	};
	int sum = 0;

	// For cell in matrix, if not out of bounds, add to sum
	for (int i = 0; i < 8; i++) {
		if (k[0][i] >= 0 && k[0][i] < w && k[1][i] >= 0 && k[1][i] < h)
			sum += (*arr)[k[1][i] * w + k[0][i]];
	}

	/*
	 * RULES:
	 * If cell is alive, it dies when there are
	 * 2 or 3 neighbors.  A dead cell will
	 * resurrect if there are 3 neighbors.
	 */
	if ((*arr)[y * w + x])
		return (sum == 2 || sum == 3);
	return (sum == 3);
}

int clear_field(uint_fast8_t **arr, int length)
{
	for (int i = 0; i < length; i++) {
		(*arr)[i] = 0;
	}
	return 0;
}

int tick(uint_fast8_t **arr, uint_fast8_t **buf, uint_fast8_t **hist, int w, int h, unsigned long long int *tick_count)
{
	int change_flag = 0;
	// Set new states to buffer
	for (int j = 0; j < h; j++) {
		for (int i = 0; i < w; i++) {
			uint_fast8_t state = check_state(arr, w, h, i, j);
			// Detect if the history frame is equal
			if ((*hist)[j * w + i] != state)
				change_flag = 1;
			(*buf)[j * w + i] = state;
		}
	}
	// Swap
	swap_array(arr, buf);

	// If no change, max out tick count
	if (!change_flag) {
		(*tick_count) = ULLONG_MAX;
	} else {
		(*tick_count)++;
		memcpy(*hist, *buf, w * h * sizeof(uint_fast8_t));
	}
	return 0;
}

void init_field(uint_fast8_t **arr, uint_fast8_t **buf, uint_fast8_t **hist, int w, int h, int probability, int ticks, unsigned long long int *tick_count)
{
	// Add noise
	for (int i = 0; i < w * h; i++) {
		(*arr)[i] = rand() % 100 < probability;
	}
	// Initial ticks
	for (int i = 0; i < ticks; i++) {
		tick(arr, buf, hist, w, h, tick_count);
	}
}

void color_switch(int *clr_idx, int step, int **color_list, int length)
{
	*clr_idx = (*clr_idx + step) % length;
	*clr_idx = *clr_idx < 0 ? *clr_idx + length : *clr_idx;
	init_pair(1, (*color_list)[*clr_idx], -1);
}

void append_color_list(int **list, int *length, int color)
{
	if (!*list) {
		*list = calloc(1, sizeof(int));
	} else {
		*list = realloc(*list, ((*length) + 1) * sizeof(int));
	}
	if (!*list) {
		free(*list);
		exit(1);
	}
	(*list)[*length] = color;
	(*length)++;
}

// For some reason these functions need to be declared
char *strsep(char **stringp, const char *delim);
char *strdup(const char *stringp);
void parse_color_list(char **list_str, int **list, int *length)
{
	if (*list_str) {
		char *token, *str, *tofree;
		tofree = str = strdup(*list_str);
		while ((token = strsep(&str, ","))) {
			append_color_list(list, length, atoi(token));
		}
		free(tofree);
	}
}

int main(int argc, char* argv[])
{
	struct arguments arguments;
	arguments.fps = 30;
	arguments.ticks_limit = MAX_TICKS;
	arguments.init_ticks = INIT_TICKS;
	arguments.init_prob = INIT_AMOUNT;
	arguments.char_alive = '\0';
	arguments.color_list = NULL;
	//arguments.switch_color = 0;
	//arguments.exclude_color = 0;
	// Parse arguments
	argp_parse (&argp, argc, argv, 0, 0, &arguments);
	// Set cli parameters
	int fps = arguments.fps;
	int max_ticks = arguments.ticks_limit;
	int init_ticks = arguments.init_ticks;
	int init_prob = arguments.init_prob;
	char char_alive = arguments.char_alive;
	char *color_list_string = arguments.color_list;
	int color_list_length = 0;
	int *color_list = NULL;
	int clr_idx;


	// Parse color list, if not, init with [0-7], todo better solution?
	parse_color_list(&color_list_string, &color_list, &color_list_length);
	if (!color_list) {
		color_list_length = 8;
		color_list = calloc(color_list_length, sizeof(int));
		for (int i=0; i<color_list_length; i++) {
			color_list[i] = i;
		}
	}
	/*
	// Just debug code for parsing the string
	for (int i=0; i<color_list_length; i++) {
		printf("%d\n",color_list[i]);
	}
	free(color_list);
	exit(1);
	*/

	// clr is the first index
	clr_idx = 0;

	// Initialize ncurses
	initscr();
	raw(); // to not hang the terminal when pressing Ctrl+S
	if (has_colors() == FALSE) {
		endwin();
		printf("Terminal doesn't support color\n");
		exit(1);
	}
	cbreak();
	noecho();
	curs_set(0);
	timeout(0);
	set_escdelay(0);
	start_color();
	use_default_colors();
	keypad(stdscr, TRUE);
	init_pair(1, color_list[clr_idx], -1);
	attron(COLOR_PAIR(1));

	// Initialize fields
	unsigned long long int tick_count = 0;
	int w = 1, h = 1, _w = 1, _h = 1;
	uint_fast8_t *arr_p, *buf, *hist;
	arr_p = create_field(w * h);
	buf = create_field(w * h);
	hist = create_field(w * h);
	for (;;) {
		/*
		 * Init (or reinit if screen resized or c pressed)
		 *
		 * If old dimensions differ, realloc and reinit
		 */
		int c = getch(); // Get input
		getmaxyx(stdscr,_h,_w);
		// If resized
		if (w != _w || h != _h) {
			w = _w;
			h = _h;
			arr_p = realloc_field(&arr_p, w * h);
			buf = realloc_field(&buf, w * h);
			hist = realloc_field(&hist, w * h);
			clear_field(&arr_p, w * h);
			init_field(&arr_p, &buf, &hist, w, h, init_prob, init_ticks, &tick_count);
			tick_count = 0;
		// If cleared or limit reached
		// If max_ticks = 0, never clear automatically
		} else if (c == 'c' || (tick_count > max_ticks && max_ticks > 0)) {
			clear_field(&arr_p, w * h);
			init_field(&arr_p, &buf, &hist, w, h, init_prob, init_ticks, &tick_count);
			tick_count = 0;
			color_switch(&clr_idx, 1, &color_list, color_list_length);
		// Quit if q, enter or esc pressed
		} else if (c == 'q' || c == '\n' || c == esc) {
			break;
		// Change color with f
		} else if (c == 'f') {
			color_switch(&clr_idx, 1, &color_list, color_list_length);
		} else if (c == 'F') {
			color_switch(&clr_idx, -1, &color_list, color_list_length);
		}

		// Handle printing and game logic
		print_matrix(&arr_p, w, h, char_alive);
		tick(&arr_p, &buf, &hist, w, h, &tick_count);
		// Delay frames only if fps is positive
		if (fps > 0)
			usleep(MICROSECONDS_IN_SECOND/fps);
	}

	// Cleanup
	release_field(&arr_p);
	release_field(&buf);
	release_field(&hist);
	free(color_list);
	clear();
	endwin();
	return 0;
}
