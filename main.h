#ifndef __MAIN_H__
#define __MAIN_H__

#include "ansi.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#pragma warning(disable: 4996)

#define GRID_SIZE_X_MIN        12
#define GRID_SIZE_Y_MIN        6
#define GRID_SIZE_MIN          (GRID_SIZE_Y_MIN * GRID_SIZE_X_MIN)
#define GRID_BUFFER_SIZE_X_MIN (GRID_SIZE_X_MIN * 4 + 1)
#define GRID_BUFFER_SIZE_Y_MIN (GRID_SIZE_Y_MIN * 2 + 1)
#define GRID_BUFFER_SIZE_MIN   (GRID_BUFFER_SIZE_Y_MIN * GRID_BUFFER_SIZE_X_MIN)
#define MINE_COUNT_MIN         10

#define GRID_SIZE_X_MED        20
#define GRID_SIZE_Y_MED        10
#define GRID_SIZE_MED          (GRID_SIZE_Y_MED * GRID_SIZE_X_MED)
#define GRID_BUFFER_SIZE_X_MED (GRID_SIZE_X_MED * 4 + 1)
#define GRID_BUFFER_SIZE_Y_MED (GRID_SIZE_Y_MED * 2 + 1)
#define GRID_BUFFER_SIZE_MED   (GRID_BUFFER_SIZE_Y_MED * GRID_BUFFER_SIZE_X_MED)
#define MINE_COUNT_MED         35

#define GRID_SIZE_X_MAX        27
#define GRID_SIZE_Y_MAX        13
#define GRID_SIZE_MAX          (GRID_SIZE_Y_MAX * GRID_SIZE_X_MAX)
#define GRID_BUFFER_SIZE_X_MAX (GRID_SIZE_X_MAX * 4 + 1)
#define GRID_BUFFER_SIZE_Y_MAX (GRID_SIZE_Y_MAX * 2 + 1)
#define GRID_BUFFER_SIZE_MAX   (GRID_BUFFER_SIZE_Y_MAX * GRID_BUFFER_SIZE_X_MAX)
#define MINE_COUNT_MAX         75

#define MINE_DENSITY_MIN       10.0f
#define MINE_DENSITY_MAX       25.0f

#define MINES_PER_REWIND_MINE 14

#define FG_COMMENT ANSI_FG_RGB(130, 170, 255)
#define FG_PROMPT  ANSI_FG_RGB(137, 221, 255)
#define FG_WARNING ANSI_FG_RGB(078, 201, 176)
#define FG_ERROR   ANSI_FG_RGB(255, 102, 102)
#define FG_NUMBER  ANSI_FG_RGB(235, 118, 105)
#define FG_OPTION  ANSI_FG_RGB(255, 203, 107)

#define __PREPEND_INFO_SYMBOL(current)    L"#   " current
#define __PREPEND_PROMPT_SYMBOL(current)  L">   " current
#define __PREPEND_ISSUE_SYMBOL(current)   L"~   " current
#define __PREPEND_ERROR_SYMBOL(current)   L"!   " current
#define INFO(...)       ANSI_SET_FG(FG_COMMENT, SELECT(__PREPEND_INFO_SYMBOL, __VA_ARGS__))
#define PROMPT(...)     ANSI_SET_FG(FG_PROMPT,  SELECT(__PREPEND_PROMPT_SYMBOL, __VA_ARGS__)) ANSI_CREATE_ATTR(FG_PROMPT)
#define ISSUE(...)      ANSI_SET_FG(FG_WARNING, SELECT(__PREPEND_ISSUE_SYMBOL, __VA_ARGS__))
#define ERROR(...)      ANSI_SET_FG(FG_ERROR,   SELECT(__PREPEND_ERROR_SYMBOL, __VA_ARGS__))  ANSI_CREATE_ATTR(FG_PROMPT)
#define NUMBER(context) ANSI_SET_FG(FG_NUMBER, context)
#define OPTION(context) ANSI_SET_FG(FG_OPTION, context)

#define __CLEAR_LOG                  ANSI_CURSOR_MOVE_BEGINNING_OF_PREVIOUS() ANSI_CLEAR_FORWARD()
#define FEATURE_WORK_IN_PROGRESS     __CLEAR_LOG ISSUE(L"This feature is a work in progress, please try again: ")
#define UNKNOWN_CONVERSION_SPECIFIER __CLEAR_LOG ISSUE(L"Unknown conversion specifier. ")
#define TOO_FEW_ARGUMENTS            __CLEAR_LOG ERROR(L"Too few arguments, please try again: ")
#define TOO_MANY_ARGUMENTS           __CLEAR_LOG ERROR(L"Too many arguments, please try again: ")
#define ARGUMENT_TOO_LONG            __CLEAR_LOG ERROR(L"Argument too long, please try again: ")
#define INVALID_NUMBER_FORMAT        __CLEAR_LOG ERROR(L"Invalid number format, please try again: ")
#define INVALID_OPTION               __CLEAR_LOG ERROR(L"Invalid option, please try again: ")
#define VALUE_OUT_OF_RANGE           __CLEAR_LOG ERROR(L"Value out of the provided range, please try again: ")
#define COORDINATES_OUT_OF_GRID      __CLEAR_LOG ERROR(L"Coordinates outside the grid, please try again: ")
#define CELL_ALREADY_OPENED          __CLEAR_LOG ERROR(L"Cell already opened, please try again: ")
#define AUTO_REVEAL_FAILURE          __CLEAR_LOG ERROR(L"Cannot complete auto-revealing, please try again: ")
#define AUTO_FLAG_FAILURE            __CLEAR_LOG ERROR(L"Cannot complete auto-flagging, please try again: ")

#define BG_GRASS    ANSI_BG_RGB(169, 215, 081)
#define BG_SOIL     ANSI_BG_RGB(228, 194, 159)
#define BG_ROCK     ANSI_BG_RGB(189, 189, 189)
#define BG_WATER    ANSI_BG_RGB(118, 205, 244)
#define FG_BORDER   ANSI_FG_RGB(071, 118, 046)
#define FG_FLOWER_1 ANSI_FG_RGB(255, 000, 000)
#define FG_FLOWER_2 ANSI_FG_RGB(072, 102, 237)
#define FG_FLOWER_3 ANSI_FG_RGB(255, 132, 000)
#define FG_FLOWER_4 ANSI_FG_RGB(115, 043, 245)
#define FG_FILL     ANSI_FG_RGB(050, 050, 050)
#define FG_CELL_1   ANSI_FG_RGB(025, 118, 210)
#define FG_CELL_2   ANSI_FG_RGB(058, 142, 061)
#define FG_CELL_3   ANSI_FG_RGB(211, 047, 047)
#define FG_CELL_4   ANSI_FG_RGB(116, 037, 145)
#define FG_CELL_5   ANSI_FG_RGB(255, 143, 002)
#define FG_CELL_6   ANSI_FG_RGB(000, 151, 167)
#define FG_CELL_7   ANSI_FG_RGB(066, 068, 071)
#define FG_CELL_8   ANSI_FG_RGB(167, 156, 146)

#define MINE_DENSITY(mine_count, size_x, size_y) ((mine_count) * 100 / (float)((size_x) * (size_y)))

typedef enum _Cell
{
    CELL_REWIND_MINE = -2,
    CELL_MINE = -1,

    CELL_0 = 0,
    CELL_1 = 1,
    CELL_2 = 2,
    CELL_3 = 3,
    CELL_4 = 4,
    CELL_5 = 5,
    CELL_6 = 6,
    CELL_7 = 7,
    CELL_8 = 8,

    CELL_OUT = 9,
} Cell;

typedef enum _CellStatus
{
    STATUS_UNREVEALED = 0,
    STATUS_FLAGGED = 1,
    STATUS_REVEALED = 2,

    STATUS_EXPLODED = 3,
    STATUS_WATER = 4,
    STATUS_CLEARED = 5,

    STATUS_OUT = 6,
} CellStatus;

typedef struct _Vector2UInt32
{
    uint32_t x, y;
} Vector2UInt32;

typedef struct _Vector2Int32
{
    int32_t x, y;
} Vector2Int32;

typedef struct _TimeSpan
{
    time_t start, end;
} TimeSpan;

int32_t console_scan_input(const char *format, ...);

int32_t game_welcome(void);

void game_loading_screen(void);

bool game_main_menu(void);

void game_scan_size(void);

void game_scan_mine_count(void);

void game_scan_rewind_mine_count(void);

bool game_win(int32_t flag_count, TimeSpan timer);

uint32_t game_lose(bool declared_defeat, int32_t flag_count, TimeSpan timer);

void game_flash(void);

void game_terminate(void);

uint32_t grid_initialize(void);

void grid_generate(Vector2UInt32 first_move);

void grid_generate_rewind_mines(Vector2UInt32 selected);

void grid_rewind(Vector2UInt32 selected, uint32_t *cell_count, int32_t *flag_count);

uint32_t grid_buffer_initialize(void);

Cell grid_get_checked(uint32_t x, uint32_t y);

CellStatus grid_status_get_checked(uint32_t x, uint32_t y);

wchar_t grid_buffer_get_character(uint32_t x, uint32_t y);

void grid_print(int32_t flag_count, TimeSpan timer);

uint32_t grid_scan_position(Vector2UInt32 *selected, int32_t flag_count);

bool grid_scan_acceptance(void);

bool grid_select(Vector2UInt32 *selected, uint32_t *cell_count, int32_t *flag_count);

void grid_select_cascade(Vector2UInt32 selected, int32_t *cell_count);

void grid_select_flag(Vector2UInt32 selected, int32_t *flag_count);

void grid_undo_last_move(Vector2UInt32 last_move);

void grid_clear_field(int32_t flag_count, TimeSpan timer);

void grid_reveal_all_mines(int32_t flag_count, TimeSpan timer);

#endif
