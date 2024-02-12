#include "main.h"
#include "macro_utils.h"
#include "ansi.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <time.h>
#include <io.h>
#include <fcntl.h>

static Vector2UInt32 grid_size = { GRID_SIZE_X_MED, GRID_SIZE_Y_MED };
static Cell *grid = &((Cell[GRID_SIZE_X_MAX][GRID_SIZE_Y_MAX]) { 0 })[0][0];
static CellStatus *grid_status = &((CellStatus[GRID_SIZE_X_MAX][GRID_SIZE_Y_MAX]) { 0 })[0][0];

static Vector2UInt32 grid_buffer_size = { GRID_BUFFER_SIZE_X_MED, GRID_BUFFER_SIZE_Y_MED };
static wchar_t *grid_buffer = &((wchar_t[GRID_BUFFER_SIZE_X_MAX][GRID_BUFFER_SIZE_Y_MAX]) { 0 })[0][0];

static uint32_t mine_count_initial = MINE_COUNT_MED;
static uint32_t rewind_mine_count_initial = MINE_COUNT_MED / MINES_PER_REWIND_MINE;

int main()
{
#if !defined(_WIN32) && !defined(_WIN64)
    /*
        Just as any other developer might do in this scenario, we explicitly mark the game
        as Windows-exclusive because we prefer not to invest effort in making it portable.

         - Regards.
    */

    return 1;
#endif

    if (game_welcome() == -1)
        return -1;

main_menu:
    while (!game_main_menu())
    {
game_process:
        while (true)
        {
            uint32_t cell_count = grid_initialize();
            uint32_t gird_buffer_size_total = grid_buffer_initialize();
            Vector2UInt32 selected = { 0 };
            int32_t flag_count = mine_count_initial + rewind_mine_count_initial;
            TimeSpan timer = { 0 };
            bool init = true;

            wprintf(ANSI_CURSOR_POSITION_RESET() ANSI_CLEAR_FORWARD());

            while (true)
            {
                grid_print(flag_count, timer);

                uint32_t control = grid_scan_position(&selected, flag_count), flag = control == 'f' && !init;

                if (init)
                {
                    grid_generate(selected);
                    grid_generate_rewind_mines(selected);
                    init = false;
                    timer.start = time(NULL);
                }

                timer.end = time(NULL);

                switch (control)
                {
                    case 'd':
                        CONDITIONAL_GOTO(game_lose(true, flag_count, timer), game_process, main_menu);
                    case 'r':
                        goto game_process;
                    case 'q':
                        goto main_menu;
                }

                if (flag)
                {
                    grid_select_flag(selected, &flag_count);
                    continue;
                }

                timer.end = time(NULL);

                if (grid_select(&selected, &cell_count, &flag_count))
                {
                    uint32_t action = game_lose(false, flag_count, timer);

                    if (action != 2)
                        CONDITIONAL_GOTO(action, game_process, main_menu);

                    grid_undo_last_move(selected);
                }

                if (cell_count == mine_count_initial + rewind_mine_count_initial)
                    CONDITIONAL_GOTO(game_win(flag_count, timer), game_process, main_menu);
            }
        }
    }

    game_terminate();

    return 0;
}

int32_t console_scan_input(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    char type;
    int32_t variable_count = 0, c = getchar();

    if (c == EOF)
        return 0;

    while (true)
    {
        while (c == ' ' || c == '\t')
            c = getchar();

        if (c == '\n')
            return variable_count;

        ++variable_count;

        if (!(type = *format++))
            break;

        switch (type)
        {
            uint32_t length;
            uint32_t ui;
            char chr;

            case 'u':
                if ((ui = c - '0') > 9u)
                {
                    wprintf(INVALID_NUMBER_FORMAT);
                    goto error;
                }

                for (length = 1; (c = getchar()) - '0' <= 9u; length += (ui != 0))
                    ui = ui * 10 + c - '0';

                if (c != ' ' && c != '\t' && c != '\n')
                {
                    wprintf(INVALID_NUMBER_FORMAT);
                    goto error;
                }

                if (length > 6)
                {
                    wprintf(ARGUMENT_TOO_LONG);
                    goto error;
                }

                *va_arg(args, uint32_t*) = ui;
                break;

            case 'c':
                chr = c;
                if (c != 26 && (c = getchar()) != ' ' && c != '\t' && c != '\n')
                {
                    wprintf(ARGUMENT_TOO_LONG);
                    goto error;
                }

                *va_arg(args, char*) = chr;
                break;

            case 'O':
                chr = c;
                if ((ui = c - '0') > 9u)
                {
                    if (c != 26 && (c = getchar()) != ' ' && c != '\t' && c != '\n')
                    {
                        wprintf(ARGUMENT_TOO_LONG);
                        goto error;
                    }

                    *va_arg(args, uint32_t*) = chr;
                    *va_arg(args, uint32_t*) = -1;
                    break;
                }

                for (length = 1; (c = getchar()) - '0' <= 9u; length += (ui != 0))
                    ui = ui * 10 + c - '0';

                if (c != ' ' && c != '\t' && c != '\n')
                {
                    wprintf(INVALID_NUMBER_FORMAT);
                    goto error;
                }

                if (length > 6)
                {
                    wprintf(ARGUMENT_TOO_LONG);
                    goto error;
                }

                *va_arg(args, uint32_t*) = -1;
                *va_arg(args, uint32_t*) = ui;
                break;

            case 'l':
                while (c != '\n' && c != 26)
                    c = getchar();
                break;

            default:
                wprintf(UNKNOWN_CONVERSION_SPECIFIER);
                goto error;
        }
    }

    if (c != '\n')
    {
        wprintf(TOO_MANY_ARGUMENTS);
error:
        while (c != '\n' && c != 26)
            c = getchar();

        va_end(args);
        return -variable_count;
    }

    va_end(args);
    return variable_count;
}

int32_t game_welcome(void)
{
    uint64_t seed = (uint64_t)time(NULL);
    srand((uint32_t)(seed ^ (seed >> 32)));
    if (_setmode(_fileno(stdout), _O_U16TEXT) == -1)
        return -1;
    wprintf(ANSI_TEXT_WRAP_ENABLE() ANSI_CURSOR_DISABLE());

    for (uint32_t bar = 0; bar <= 23 + 21 + 25; ++bar, _sleep(25))
        wprintf(ANSI_CURSOR_POSITION_RESET() L"\n"
            L"    " ANSI_SET_CL(FG_BORDER, BG_GRASS, L"┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓" L"\n")
            L"    " ANSI_SET_CL(FG_BORDER, BG_GRASS, L"┃ " ANSI_SET_BG(BG_ROCK, L"💣"), L"═════════════════════════════════" ANSI_SET_BG(BG_ROCK, L"🚩"), L" ┃" L"\n")
            L"    " ANSI_SET_CL(FG_BORDER, BG_GRASS, L"┃ ║                                   ║ ┃" L"\n")
            L"    " ANSI_SET_CL(FG_BORDER, BG_GRASS, L"┃ ║   " ANSI_SET_BG(BG_SOIL, L"                             "), L"   ║ ┃" L"\n")
            L"    " ANSI_SET_CL(FG_BORDER, BG_GRASS, L"┃ ║   " ANSI_SET_CL(ANSI_FG_BLACK, BG_SOIL, L"   %.*s%.*s%.*s   "),   L"   ║ ┃" L"\n")
            L"    " ANSI_SET_CL(FG_BORDER, BG_GRASS, L"┃ ║   " ANSI_SET_CL(ANSI_FG_BLACK, BG_SOIL, L"    %.*s%.*s%.*s    "), L"   ║ ┃" L"\n")
            L"    " ANSI_SET_CL(FG_BORDER, BG_GRASS, L"┃ ║   " ANSI_SET_CL(ANSI_FG_BLACK, BG_SOIL, L"  %.*s%.*s%.*s  "),     L"   ║ ┃" L"\n")
            L"    " ANSI_SET_CL(FG_BORDER, BG_GRASS, L"┃ ║   " ANSI_SET_BG(BG_SOIL, L"                             "), L"   ║ ┃" L"\n")
            L"    " ANSI_SET_CL(FG_BORDER, BG_GRASS, L"┃ ║                                   ║ ┃" L"\n")
            L"    " ANSI_SET_CL(FG_BORDER, BG_GRASS, L"┃ " ANSI_SET_BG(BG_ROCK, L"💥"), L"═════════════════════════════════" ANSI_SET_BG(BG_ROCK, L"🌑"), L" ┃" L"\n")
            L"    " ANSI_SET_CL(FG_BORDER, BG_GRASS, L"┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛" L"\n"),
            (bar >= 0) ? (bar > 23) ? 0 : (23 - bar) / 2 : 11, L"           ",
            (bar >= 0) ? (bar > 23) ? 23 : bar : 0, L"Welcome, fellow player!",
            (bar >= 0) ? (bar > 23) ? 0 : (24 - bar) / 2 : 12, L"            ",
            (bar >= 23) ? (bar > 23 + 21) ? 0 : (23 + 21 - bar) / 2 : 10, L"          ",
            (bar >= 23) ? (bar > 23 + 21) ? 21 : bar - 23 : 0, L"We hope you enjoy it.",
            (bar >= 23) ? (bar > 23 + 21) ? 0 : (24 + 21 - bar) / 2 : 11, L"           ",
            (bar >= 23 + 21) ? (bar > 23 + 21 + 25) ? 0 : (23 + 21 + 25 - bar) / 2 : 12, L"            ",
            (bar >= 23 + 21) ? (bar > 23 + 21 + 25) ? 25 : bar - 23 - 21 : 0, L"Press enter to proceed...",
            (bar >= 23 + 21) ? (bar > 23 + 21 + 25) ? 0 : (24 + 21 + 25 - bar) / 2 : 13, L"             ");

    wprintf(ANSI_CURSOR_ENABLE() L"\n"
        ISSUE(L"If you notice strange bugs with texts, please make sure the size of your console is big enough.") L"\n"
        L"\n"
        PROMPT(L""));

    console_scan_input("l");
    return 0;
}

void game_loading_screen(void)
{
    static const wchar_t *const symbols = L"-\\|/-\\|/";

    wprintf(ANSI_CURSOR_POSITION_RESET() ANSI_CLEAR_FORWARD() ANSI_CURSOR_DISABLE());

    for (uint32_t bar = 0; bar <= 62 + 6 * 3; ++bar, _sleep(8))
        wprintf(ANSI_CURSOR_POSITION_RESET() L"\n"
            ANSI_SET_MD(ANSI_MD_SLOW_BLINK, ANSI_SET_MD(ANSI_MD_REVERSE_VIDEO, ANSI_SET_CL(ANSI_FG_BLACK, ANSI_BG_BLACK,
                L"    ", ANSI_SET_CL(FG_FILL, BG_ROCK, L"┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓" L"\n"),
                L"    ", ANSI_SET_CL(FG_FILL, BG_ROCK, L"┃ ╔══════════════════════════════════════════════════════════════════╗ ┃" L"\n"),
                L"    ", ANSI_SET_CL(FG_FILL, BG_ROCK, L"┃ ║  %-62.*s  ║ ┃" L"\n"),
                L"    ", ANSI_SET_CL(FG_FILL, BG_ROCK, L"┃ ║  %-62.*s  ║ ┃" L"\n"),
                L"    ", ANSI_SET_CL(FG_FILL, BG_ROCK, L"┃ ║  %-62.*s  ║ ┃" L"\n"),
                L"    ", ANSI_SET_CL(FG_FILL, BG_ROCK, L"┃ ║  %-62.*s  ║ ┃" L"\n"),
                L"    ", ANSI_SET_CL(FG_FILL, BG_ROCK, L"┃ ║  %-62.*s  ║ ┃" L"\n"),
                L"    ", ANSI_SET_CL(FG_FILL, BG_ROCK, L"┃ ║  %-62.*s  ║ ┃" L"\n"),
                L"    ", ANSI_SET_CL(FG_FILL, BG_ROCK, L"┃ ╚══════════════════════════════════════════════════════════════════╝ ┃" L"\n"),
                L"    ", ANSI_SET_CL(FG_FILL, BG_ROCK, L"┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛" L"\n"))))
            L"\n"
            ISSUE(L"<{=[%c]=[%c]=[%c]=[%c]=[%c]=[%c]=[%c]=[%c]=}>"),
            bar >= 0 * 5 ? bar >= 62 + 0 * 5 ? 62 : bar - 0 * 5 : 0, L" __  __  _                                                    ",
            bar >= 1 * 5 ? bar >= 62 + 1 * 5 ? 62 : bar - 1 * 5 : 0, L"|  \\/  |(_) _ __   ___  _____      _____  ___ _ __   ___ _ __ ",
            bar >= 2 * 5 ? bar >= 62 + 2 * 5 ? 62 : bar - 2 * 5 : 0, L"| \\  / | _ | '_ \\ / _ \\/ __\\ \\ /\\ / / _ \\/ _ \\ '_ \\ / _ \\ '__|",
            bar >= 3 * 5 ? bar >= 62 + 3 * 5 ? 62 : bar - 3 * 5 : 0, L"| |\\/| || || | | |  __/\\__ \\\\ V  V /  __/  __/ |_) |  __/ |   ",
            bar >= 4 * 5 ? bar >= 62 + 4 * 5 ? 62 : bar - 4 * 5 : 0, L"|_|  |_||_||_| |_|\\___||___/ \\_/\\_/ \\___|\\___| .__/ \\___|_|   ",
            bar >= 5 * 5 ? bar >= 62 + 5 * 5 ? 62 : bar - 5 * 5 : 0, L"                                             |_|              ",
            symbols[((bar - 0 * 2) >> 2) & 3],
            symbols[((bar - 1 * 2) >> 2) & 3],
            symbols[((bar - 2 * 2) >> 2) & 3], 
            symbols[((bar - 3 * 2) >> 2) & 3],
            symbols[((bar - 4 * 2) >> 2) & 3],
            symbols[((bar - 5 * 2) >> 2) & 3],
            symbols[((bar - 6 * 2) >> 2) & 3],
            symbols[((bar - 7 * 2) >> 2) & 3]);

    _sleep(200);
}

bool game_main_menu(void)
{
    game_loading_screen();
    
    wprintf(ANSI_CURSOR_ENABLE() ANSI_CURSOR_MOVE_BEGINNING_OF_PREVIOUS() ANSI_CLEAR_FORWARD() L"\n"
        INFO(
            L"Menu options:" L"\n",
            L" " OPTION(L"`P`") L" - Start the game." L"\n",
            L" " OPTION(L"`S`") L" - Choose the size of the grid." L"\n",
            L" " OPTION(L"`M`") L" - Choose the number of mines." L"\n",
            L" " OPTION(L"`R`") L" - Choose the number of rewind mines." L"\n",
            L" " OPTION(L"`Q`") L" - Quit the game." L"\n"
            L"\n")
        ISSUE(L"Enter one of the menu options from above.")
        ANSI_CURSOR_POSITION_SAVE());

prompt:
    wprintf(ANSI_CURSOR_POSITION_RESTORE() ANSI_CLEAR_FORWARD() L"\n" PROMPT(L""));

    while (true)
    {
        char op;
        int32_t code = console_scan_input("c", &op);

        if (code <= 0)
        {
            if (code == 0)
                wprintf(TOO_FEW_ARGUMENTS);

            continue;
        }

        switch (op | 32)
        {
            case 'p':
                return false;
            case 's':
                game_scan_size();
                goto prompt;
            case 'm':
                game_scan_mine_count();
                goto prompt;
            case 'r':
                game_scan_rewind_mine_count();
                goto prompt;
            case 'q':
                return true;
            default:
                wprintf(INVALID_OPTION);
                continue;
        }
    }
}

void game_scan_size(void)
{
    wprintf(L"\n" 
        INFO(
            L"Current dimensions: " NUMBER(L"%u") L"x" NUMBER(L"%u") L"\n",
            L"Current mine density: " NUMBER(L"%.1f") L"%%" "\n",
            L" " OPTION(L"` `") L" - Cancel" L"\n",
            L" " OPTION(L"`E`") L" - Play in the easiest mode, " NUMBER(WCS(GRID_SIZE_X_MIN)) L"x" NUMBER(WCS(GRID_SIZE_Y_MIN)) L"\n",
            L" " OPTION(L"`M`") L" - Play in the medium mode, " NUMBER(WCS(GRID_SIZE_X_MED)) L"x" NUMBER(WCS(GRID_SIZE_Y_MED)) L"\n",
            L" " OPTION(L"`H`") L" - Play in the hardest mode, " NUMBER(WCS(GRID_SIZE_X_MAX)) L"x" NUMBER(WCS(GRID_SIZE_Y_MAX)) L"\n",
            L" " OPTION(L"`n m`") L" - Set to custom dimensions, within [(" NUMBER(WCS(GRID_SIZE_X_MIN)) L", " NUMBER(WCS(GRID_SIZE_Y_MIN)) L"), (" NUMBER(WCS(GRID_SIZE_X_MAX)) L", " NUMBER(WCS(GRID_SIZE_Y_MAX)) L")]" L"\n"
            L"\n")
        ISSUE(L"Enter one of the options from above." L"\n")
        PROMPT(L""),
        grid_size.x, grid_size.y, MINE_DENSITY(mine_count_initial, grid_size.x, grid_size.y));

    while (true)
    {
        uint32_t option, size_x, size_y;
        int32_t code = console_scan_input("Ou", &option, &size_x, &size_y);

        if (code <= 0)
        {
            if (code == 0)
                return;

            continue;
        }

        if (code == 1)
        {
            if (option == -1)
            {
                wprintf(TOO_FEW_ARGUMENTS);
                continue;
            }

            switch (option | 32)
            {
                case 'e':
                    grid_size = (Vector2UInt32) { GRID_SIZE_X_MIN, GRID_SIZE_Y_MIN };
                    goto adjust_dependencies;
                case 'm':
                    grid_size = (Vector2UInt32) { GRID_SIZE_X_MED, GRID_SIZE_Y_MED };
                    goto adjust_dependencies;
                case 'h':
                    grid_size = (Vector2UInt32) { GRID_SIZE_X_MAX, GRID_SIZE_Y_MAX };
                    goto adjust_dependencies;
                default:
                    wprintf(INVALID_OPTION);
                    continue;
            }
        }

        if (size_x == -1)
        {
            wprintf(TOO_MANY_ARGUMENTS);
            continue;
        }

        if (size_x < GRID_SIZE_X_MIN || GRID_SIZE_X_MAX < size_x ||
            size_y < GRID_SIZE_Y_MIN || GRID_SIZE_Y_MAX < size_y)
        {
            wprintf(VALUE_OUT_OF_RANGE);
            continue;
        }

        grid_size = (Vector2UInt32) { size_x, size_y };
        goto adjust_dependencies;
    }

adjust_dependencies:
    grid_buffer_size = (Vector2UInt32) { grid_size.x * 4 + 1, grid_size.y * 2 + 1 };

    float mine_density = MINE_DENSITY(mine_count_initial, grid_size.x, grid_size.y);

    if (MINE_DENSITY_MIN <= mine_density && mine_density <= MINE_DENSITY_MAX)
        return;

    int ratio = mine_density < MINE_DENSITY_MIN ? 10 : 4;
    mine_count_initial = grid_size.x * grid_size.y / ratio;

    rewind_mine_count_initial = min(rewind_mine_count_initial, mine_count_initial / MINES_PER_REWIND_MINE);
}

void game_scan_mine_count(void)
{
    wprintf(L"\n"
        INFO(
            L"Current mine count: " NUMBER(L"%u") L"\n",
            L"Current mine density: " NUMBER("%.1f") L"%%" L"\n",
            L" " OPTION(L"` `") L" - Cancel" L"\n",
            L" " OPTION(L"`E`") L" - Play in the easiest mode, " NUMBER(WCS(MINE_COUNT_MIN)) L"\n",
            L" " OPTION(L"`M`") L" - Play in the medium mode, " NUMBER(WCS(MINE_COUNT_MED)) L"\n",
            L" " OPTION(L"`H`") L" - Play in the hardest mode, " NUMBER(WCS(MINE_COUNT_MAX)) L"\n",
            L" " OPTION(L"`n`") L" - Set to a custom number of mines, within [" NUMBER(WCS(MINE_COUNT_MIN)) L", " NUMBER(WCS(MINE_COUNT_MAX)) L"]" L"\n"
            L"\n")
        ISSUE(L"Enter one of the options from above." L"\n")
        PROMPT(L""),
        mine_count_initial, MINE_DENSITY(mine_count_initial, grid_size.x, grid_size.y));

    while (true)
    {
        uint32_t option, mine_count;
        int32_t code = console_scan_input("O", &option, &mine_count);

        if (code <= 0)
        {
            if (code == 0)
                return;

            continue;
        }

        if (mine_count == -1)
            switch (option | 32)
            {
                case 'e':
                    mine_count_initial = MINE_COUNT_MIN;
                    goto adjust_dependencies;
                case 'm':
                    mine_count_initial = MINE_COUNT_MED;
                    goto adjust_dependencies;
                case 'h':
                    mine_count_initial = MINE_COUNT_MAX;
                    goto adjust_dependencies;
                default:
                    wprintf(INVALID_OPTION);
                    continue;
            }

        if (mine_count < MINE_COUNT_MIN || MINE_COUNT_MAX < mine_count)
        {
            wprintf(VALUE_OUT_OF_RANGE);
            continue;
        }

        mine_count_initial = mine_count;
        goto adjust_dependencies;
    }

adjust_dependencies:
    rewind_mine_count_initial = min(rewind_mine_count_initial, mine_count_initial / MINES_PER_REWIND_MINE);

    float mine_density = MINE_DENSITY(mine_count_initial, grid_size.x, grid_size.y);

    if (MINE_DENSITY_MIN <= mine_density && mine_density <= MINE_DENSITY_MAX)
        return;

    uint32_t ratio = mine_density < MINE_DENSITY_MIN ? 10 : 4;
    float coefficient = sqrtf(mine_count_initial * ratio / (float)(grid_size.x * grid_size.y));
    grid_size = (Vector2UInt32) { (uint32_t)ceilf(grid_size.x * coefficient), (uint32_t)floorf(grid_size.y * coefficient) };
    grid_buffer_size = (Vector2UInt32) { grid_size.x * 4 + 1, grid_size.y * 2 + 1 };
}

void game_scan_rewind_mine_count(void)
{
    wprintf(L"\n"
        INFO(
            L"Current rewind mine count: " NUMBER(L"%u") L"\n",
            L" " OPTION(L"` `") L" - Cancel" L"\n",
            L" " OPTION(L"`D`") L" - Disable rewind mines" L"\n",
            L" " OPTION(L"`n`") L" - Set to a custom number of rewind mines allowable for the current mine count, within [" NUMBER(L"0") L", " NUMBER(L"%u") L"]" L"\n"
            L"\n")
        ISSUE(L"Enter one of the options from above." L"\n")
        PROMPT(L""),
        rewind_mine_count_initial, mine_count_initial / MINES_PER_REWIND_MINE);

    while (true)
    {
        uint32_t option, rewind_mine_count;
        int32_t code = console_scan_input("O", &option, &rewind_mine_count);

        if (code <= 0)
        {
            if (code == 0)
                return;

            continue;
        }

        if (rewind_mine_count == -1)
            switch (option | 32)
            {
                case 'd':
                    rewind_mine_count_initial = 0;
                    return;
                default:
                    wprintf(INVALID_OPTION);
                    continue;
            }

        if (mine_count_initial / MINES_PER_REWIND_MINE < rewind_mine_count)
        {
            wprintf(VALUE_OUT_OF_RANGE);
            continue;
        }

        rewind_mine_count_initial = rewind_mine_count;
        return;
    }
}

bool game_win(int32_t flag_count, TimeSpan timer)
{
    grid_print(flag_count, timer);
    _sleep(500);

    grid_clear_field(flag_count, timer);

    wprintf(L"\n"
        INFO(
            L"You did it! All the mines in the field have been cleared." L"\n",
            L" " OPTION(L"` `") L" - Switch back to the main menu" L"\n",
            L" " OPTION(L"`R`") L" - Restart the game to play again" L"\n"
            L"\n")
        ISSUE(L"Enter one of the options from above." L"\n")
        PROMPT(L"")
        ANSI_CURSOR_ENABLE());

    while (true)
    {
        char option = 0;
        int32_t code = console_scan_input("c", &option);

        if (code <= 0)
        {
            if (code == 0)
                return false;

            continue;
        }

        if ((option | 32) != 'r')
        {
            wprintf(INVALID_OPTION);
            continue;
        }

        return true;
    }
}

uint32_t game_lose(bool declared_defeat, int32_t flag_count, TimeSpan timer)
{
    grid_print(flag_count, timer);

    if (!declared_defeat)
    {
        game_flash();

        if (!grid_scan_acceptance())
            return 2;
    }

    grid_reveal_all_mines(flag_count, timer);

    wprintf(L"\n"
        ERROR(L"Mission failed! %s" L"\n")
        INFO(
            L" " OPTION(L"` `") L" - Switch back to the main menu" L"\n",
            L" " OPTION(L"`R`") L" - Restart the game to play again" L"\n"
            L"\n")
        ISSUE(L"Enter one of the options from above." L"\n")
        PROMPT(L"")
        ANSI_CURSOR_ENABLE(),
        declared_defeat ? L"You gave up the field." : L"The field has been obliterated by a mine.");

    while (true)
    {
        char option = 0;
        int32_t code = console_scan_input("c", &option);

        if (code <= 0)
        {
            if (code == 0)
                return 0;

            continue;
        }

        if ((option | 32) != 'r')
        {
            wprintf(INVALID_OPTION);
            continue;
        }

        return 1;
    }
}

void game_flash(void)
{
    for (uint32_t i = 0; i < 11; _sleep(40))
        wprintf(++i & 1 ? L"\033[?5l" : L"\033[?5h");
}

void game_terminate(void)
{
    wprintf(ANSI_CURSOR_POSITION_RESET() ANSI_CLEAR_FORWARD() ANSI_CURSOR_DISABLE());

    for (uint32_t bar = 0; bar <= 19; ++bar, _sleep(25))
        wprintf(ANSI_CURSOR_POSITION_RESET() L"\n"
            L"    " ANSI_SET_CL(FG_BORDER, BG_GRASS, L"┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓" L"\n")
            L"    " ANSI_SET_CL(FG_BORDER, BG_GRASS, L"┃ " ANSI_SET_BG(BG_ROCK, L"💣"), L"═══════════════════════════" ANSI_SET_BG(BG_ROCK, L"🚩"), L" ┃" L"\n")
            L"    " ANSI_SET_CL(FG_BORDER, BG_GRASS, L"┃ ║                             ║ ┃" L"\n")
            L"    " ANSI_SET_CL(FG_BORDER, BG_GRASS, L"┃ ║   " ANSI_SET_BG(BG_SOIL, L"                       "), L"   ║ ┃" L"\n")
            L"    " ANSI_SET_CL(FG_BORDER, BG_GRASS, L"┃ ║   " ANSI_SET_CL(ANSI_FG_BLACK, BG_SOIL, L"  %.*s%.*s%.*s  "), L"   ║ ┃" L"\n")
            L"    " ANSI_SET_CL(FG_BORDER, BG_GRASS, L"┃ ║   " ANSI_SET_BG(BG_SOIL, L"                       "), L"   ║ ┃" L"\n")
            L"    " ANSI_SET_CL(FG_BORDER, BG_GRASS, L"┃ ║                             ║ ┃" L"\n")
            L"    " ANSI_SET_CL(FG_BORDER, BG_GRASS, L"┃ " ANSI_SET_BG(BG_ROCK, L"💥"), L"═══════════════════════════" ANSI_SET_BG(BG_ROCK, L"🌑"), L" ┃" L"\n")
            L"    " ANSI_SET_CL(FG_BORDER, BG_GRASS, L"┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛" L"\n"),
            (19 - bar) / 2, L"         ",
            bar, L"Thanks for playing!",
            (20 - bar) / 2, L"          ");
}

uint32_t grid_initialize(void)
{
    uint32_t cell_count = grid_size.x * grid_size.y;
    memset(grid, CELL_0, sizeof *grid * cell_count);
    memset(grid_status, STATUS_UNREVEALED, sizeof *grid_status * cell_count);
    return cell_count;
}

void grid_generate(Vector2UInt32 first_move)
{
    for (uint32_t mine_count = mine_count_initial; mine_count;)
    {
        Vector2Int32 random = { RANDOM_INT(grid_size.x), RANDOM_INT(grid_size.y) };

        if ((int32_t)first_move.x - 1 <= random.x && random.x <= (int32_t)first_move.x + 1 &&
            (int32_t)first_move.y - 1 <= random.y && random.y <= (int32_t)first_move.y + 1 ||
            grid[random.y * grid_size.x + random.x] == CELL_MINE)
            continue;

        grid[random.y * grid_size.x + random.x] = CELL_MINE;
        --mine_count;

        for (int32_t y = random.y - 1; y <= random.y + 1; ++y)
            for (int32_t x = random.x - 1; x <= random.x + 1; ++x)
                if ((uint32_t)grid_get_checked(x, y) < CELL_OUT)
                    ++grid[y * grid_size.x + x];
    }
}

void grid_generate_rewind_mines(Vector2UInt32 first_move)
{
    for (uint32_t rewind_mine_count = rewind_mine_count_initial; rewind_mine_count;)
    {
        Vector2Int32 random = { RANDOM_INT(grid_size.x), RANDOM_INT(grid_size.y) };

        if (random.x < 2 || grid_size.x - 3 < random.x ||
            random.y < 2 || grid_size.y - 3 < random.y ||
            (int32_t)first_move.x - 1 <= random.x && random.x <= (int32_t)first_move.x + 1 &&
            (int32_t)first_move.y - 1 <= random.y && random.y <= (int32_t)first_move.y + 1 ||
            grid[random.y * grid_size.x + random.x] == CELL_MINE ||
            grid[random.y * grid_size.x + random.x] == CELL_REWIND_MINE)
            continue;

        grid[random.y * grid_size.x + random.x] = CELL_REWIND_MINE;
        --rewind_mine_count;
    }
}

void grid_rewind(Vector2UInt32 selected, uint32_t *cell_count, int32_t *flag_count)
{
    uint32_t local_mine_count = 0;
    grid_status[selected.y * grid_size.x + selected.x] = STATUS_REVEALED;

    for (int32_t y = selected.y - 2; y <= selected.y + 2; ++y)
        for (int32_t x = selected.x - 2; x <= selected.x + 2; ++x)
        {
            if (grid_status[y * grid_size.x + x] == STATUS_FLAGGED)
            {
                grid_status[y * grid_size.x + x] = STATUS_UNREVEALED;
                ++*flag_count;
            }

            if (grid_status[y * grid_size.x + x] == STATUS_REVEALED &&
                grid[y * grid_size.x + x] != CELL_REWIND_MINE)
            {
                grid_status[y * grid_size.x + x] = STATUS_UNREVEALED;
                ++*cell_count;
            }

            if (grid[y * grid_size.x + x] == CELL_MINE)
            {
                ++local_mine_count;
                grid[y * grid_size.x + x] = CELL_1;
            
                for (int32_t around_y = y - 1; around_y <= y + 1; ++around_y)
                    for (int32_t around_x = x - 1; around_x <= x + 1; ++around_x)
                    {
                        Cell cell = grid_get_checked(around_x, around_y);
                
                        if ((uint32_t)cell < CELL_OUT)
                            --grid[around_y * grid_size.x + around_x];
                        else if (cell == CELL_MINE)
                            ++grid[y * grid_size.x + x];
                    }
            }
        }

    while (local_mine_count)
    {
        Vector2Int32 random = { selected.x - 2 + RANDOM_INT(5), selected.y - 2 + RANDOM_INT(5) };
    
        if ((uint32_t)grid[random.y * grid_size.x + random.x] >= CELL_OUT)
            continue;
    
        grid[random.y * grid_size.x + random.x] = CELL_MINE;
        --local_mine_count;
    
        for (int32_t y = random.y - 1; y <= random.y + 1; ++y)
            for (int32_t x = random.x - 1; x <= random.x + 1; ++x)
                if ((uint32_t)grid_get_checked(x, y) < CELL_OUT)
                    ++grid[y * grid_size.x + x];
    }
}

uint32_t grid_buffer_initialize(void)
{
    for (uint32_t y = 0; y < grid_buffer_size.y; ++y)
        for (uint32_t x = 0; x < grid_buffer_size.x; ++x)
            grid_buffer[y * grid_buffer_size.x + x] = grid_buffer_get_character(x, y);

    return grid_buffer_size.x * grid_buffer_size.y * (uint32_t)(sizeof *grid_buffer);
}

Cell grid_get_checked(uint32_t x, uint32_t y)
{
    return x >= grid_size.x || y >= grid_size.y ? CELL_OUT : grid[y * grid_size.x + x];
}

CellStatus grid_status_get_checked(uint32_t x, uint32_t y)
{
    return x >= grid_size.x || y >= grid_size.y ? STATUS_OUT : grid_status[y * grid_size.x + x];
}

wchar_t grid_buffer_get_character(uint32_t x, uint32_t y)
{
    if (x & 3)
    {
        if (y & 1)
            return L' ';

        if (y == 0 || y == grid_buffer_size.y - 1)
            return (x & 3) == 2 ? '0' + (x / 4) % 10 : L'━';

        return L'═';
    }

    if (y & 1)
        return x == 0 || x == grid_buffer_size.x - 1 ? '0' + (y / 2) % 10 : L'║';

    if (y == 0)
        return x ? x == grid_buffer_size.x - 1 ? L'┓' : L'┳' : L'┏';

    if (y == grid_buffer_size.y - 1)
        return x ? x == grid_buffer_size.x - 1 ? L'┛' : L'┻' : L'┗';

    return x ? x == grid_buffer_size.x - 1 ? L'┫' : L'╬' : L'┣';
}

void grid_print(int32_t flag_count, TimeSpan timer)
{
    static const wchar_t *const number_colors[] = { L"", FG_CELL_1, FG_CELL_2, FG_CELL_3, FG_CELL_4, FG_CELL_5, FG_CELL_6, FG_CELL_7, FG_CELL_8 };
    static const wchar_t *const flower_colors[] = { FG_FLOWER_1, FG_FLOWER_2, FG_FLOWER_3, FG_FLOWER_4 };
    static const wchar_t flower_pool[] = L"   ✽✽✿✿❁❁❃❃❋❋✤✤";

    const wchar_t *buffer = grid_buffer;
    double seconds = difftime(timer.end, timer.start);

    wprintf(ANSI_CURSOR_POSITION_RESET() ANSI_RESET_ALL()
        L"\n"
        L"    " L"🚩 %-4i" L"        " L"⏰ %04u" L"\n"
        L"\n"
        L"    " ANSI_CREATE_ATTRS(FG_BORDER, BG_GRASS),
        flag_count, seconds < 10'000 ? (uint32_t)seconds : 9'999);

    for (const wchar_t *length = buffer + grid_buffer_size.x - 1; buffer < length; buffer += 4)
        wprintf(L"%.2s" 
            ANSI_CREATE_ATTRS(ANSI_FG_BLACK, BG_ROCK) L"%c"
            ANSI_CREATE_ATTRS(FG_BORDER, BG_GRASS) L"%c",
            buffer, buffer[2], buffer[3]);

    wprintf(L"%c" L"\n", (buffer++)[0]);

    for (size_t y = (size_t)grid_buffer_size.y - 1, index = -1; ; y -= 2)
    {
        wprintf(ANSI_RESET_ALL() L"    " ANSI_CREATE_ATTRS(ANSI_FG_BLACK, BG_ROCK) L"%c", (buffer++)[0]);

        for (const wchar_t *length = buffer + grid_buffer_size.x - 4; ;)
        {
            Cell cell = grid[++index];

            switch (grid_status[index])
            {
                static uint32_t mangle_bits(uint32_t value);
                static bool mangle_to_boolean(uint32_t value);

                case STATUS_UNREVEALED:
                    wprintf(ANSI_CREATE_ATTRS(FG_BORDER, BG_GRASS) L"   ");
                    break;

                case STATUS_REVEALED:
                    switch (cell)
                    {
                        case CELL_0:
                            wprintf(ANSI_CREATE_ATTRS(FG_BORDER, BG_SOIL) L"   ");
                            break;
                        case CELL_MINE:
                            wprintf(mangle_to_boolean(index) ? ANSI_CREATE_ATTR(BG_ROCK) L" 💣" : ANSI_CREATE_ATTR(BG_ROCK) L"💣 ");
                            break;
                        case CELL_REWIND_MINE:
                            wprintf(mangle_to_boolean(index) ? ANSI_CREATE_ATTR(BG_ROCK) L" 🌑" : ANSI_CREATE_ATTR(BG_ROCK) L"🌑 ");
                            break;
                        default:
                            wprintf(ANSI_CREATE_ATTRS(L"%s", BG_SOIL) L" %c ", number_colors[cell], '0' + cell);
                    }

                    break;

                case STATUS_FLAGGED:
                    wprintf(mangle_to_boolean(index) ? ANSI_CREATE_ATTR(BG_GRASS) L" 🚩" : ANSI_CREATE_ATTR(BG_GRASS) L"🚩 ");
                    break;

                case STATUS_EXPLODED:
                    wprintf(mangle_to_boolean(index) ? ANSI_CREATE_ATTR(BG_ROCK) L" 💥" : ANSI_CREATE_ATTR(BG_ROCK) L"💥 ");
                    break;

                case STATUS_WATER:
                    wprintf(ANSI_CREATE_ATTR(BG_WATER) L"   ");
                    break;

                case STATUS_CLEARED:
                    uint32_t left_flower   = mangle_bits(index + 7)                               % (sizeof flower_pool / sizeof *flower_pool - 1);
                    uint32_t middle_flower = mangle_bits((index + 7) * index + 13)                % (sizeof flower_pool / sizeof *flower_pool - 1);
                    uint32_t right_flower  = mangle_bits(((index + 7) * index + 13) * index + 19) % (sizeof flower_pool / sizeof *flower_pool - 1);

                    uint32_t left_color   = (middle_flower ^ right_flower ) % (sizeof flower_colors / sizeof *flower_colors);
                    uint32_t middle_color = (left_flower   ^ right_flower ) % (sizeof flower_colors / sizeof *flower_colors);
                    uint32_t right_color  = (left_flower   ^ middle_flower) % (sizeof flower_colors / sizeof *flower_colors);

                    wprintf(ANSI_CREATE_ATTR(BG_GRASS) 
                        ANSI_CREATE_ATTR(L"%s") L"%c" ANSI_CREATE_ATTR(L"%s") L"%c" ANSI_CREATE_ATTR(L"%s") L"%c",
                        flower_colors[left_color],   flower_pool[left_flower],
                        flower_colors[middle_color], flower_pool[middle_flower],
                        flower_colors[right_color],  flower_pool[right_flower]);

                    break;

                default:
                    wprintf(ANSI_CREATE_ATTRS(FG_BORDER, BG_GRASS) L"###");
            }

            if ((buffer += 4) > length + 1)
                break;

            wprintf(ANSI_CREATE_ATTRS(FG_BORDER, BG_GRASS) L"%c", buffer[-1]);
        }

        wprintf(ANSI_CREATE_ATTRS(FG_FILL, BG_ROCK) L"%c" L"\n", buffer[-1]);

        if (y <= 2)
            break;

        wprintf(ANSI_RESET_ALL() L"    " ANSI_CREATE_ATTRS(FG_BORDER, BG_GRASS) L"%.*s\n", grid_buffer_size.x, buffer);
        buffer += grid_buffer_size.x;
    }

    wprintf(ANSI_RESET_ALL() L"    " ANSI_CREATE_ATTRS(FG_BORDER, BG_GRASS));

    for (size_t length = buffer + grid_buffer_size.x - 1; buffer < length; buffer += 4)
        wprintf(L"%.2s"
            ANSI_CREATE_ATTRS(ANSI_FG_BLACK, BG_ROCK) L"%c"
            ANSI_CREATE_ATTRS(FG_BORDER, BG_GRASS) L"%c",
            buffer, buffer[2], buffer[3]);

    wprintf(L"%c" L"\n" ANSI_RESET_ALL() ANSI_CLEAR_FORWARD(), buffer[0]);
}

uint32_t grid_scan_position(Vector2UInt32 *selected, int32_t flag_count)
{
    wprintf(L"\n"
        INFO(
            L" " OPTION(L"`D`") L" - Declare defeat and reveal all the mines" L"\n",
            L" " OPTION(L"`R`") L" - Restart the game to start over" L"\n",
            L" " OPTION(L"`Q`") L" - Quit and switch back to the main menu" L"\n",
            L" " OPTION(L"`x y`") L" - Reveal the cell at the coordinates, within [(" NUMBER(L"0") L", " NUMBER(L"0") L"), (" NUMBER(L"%u") L", " NUMBER(L"%u") L")]" L"\n",
            L" " OPTION(L"`x y F`") L" - Flag or unflag the cell at the coordinates, within [(" NUMBER(L"0") L", " NUMBER(L"0") L"), (" NUMBER(L"%u") L", " NUMBER(L"%u") L")]" L"\n"
            L"\n")
        ISSUE(L"Enter one of the options from above." L"\n")
        PROMPT(L""),
        grid_size.x - 1, grid_size.y - 1, grid_size.x - 1, grid_size.y - 1);

    while (true)
    {
        uint32_t option, x, y;
        char flag;
        int32_t code = console_scan_input("Ouc", &option, &x, &y, &flag);

        if (code <= 0)
        {
            if (code == 0)
                wprintf(TOO_FEW_ARGUMENTS);

            continue;
        }

        if (code == 1)
        {
            if (option == -1)
            {
                wprintf(TOO_FEW_ARGUMENTS);
                continue;
            }

            switch (option |= 32)
                case 'd':
                case 'r':
                case 'q':
                    return option;

            wprintf(INVALID_OPTION);
            continue;
        }

        if (x == -1)
        {
            wprintf(TOO_MANY_ARGUMENTS);
            continue;
        }

        if (grid_size.x <= x || grid_size.y <= y)
        {
            wprintf(COORDINATES_OUT_OF_GRID);
            continue;
        }

        *selected = (Vector2UInt32) { x, y };

        if (code == 3)
        {
            if ((flag | 32) != 'f')
            {
                wprintf(INVALID_OPTION);
                continue;
            }

            if (grid_status[y * grid_size.x + x] == STATUS_REVEALED)
            {
                uint32_t around_revealed = 0, around_out = 0;

                for (int32_t around_y = y - 1; around_y <= (int32_t)y + 1; ++around_y)
                    for (int32_t around_x = x - 1; around_x <= (int32_t)x + 1; ++around_x)
                    {
                        CellStatus status = grid_status_get_checked(around_x, around_y);
                        around_revealed += status == STATUS_REVEALED;
                        around_out      += status == STATUS_OUT;
                    }

                if (around_revealed + around_out == 9 || grid[y * grid_size.x + x] == CELL_REWIND_MINE)
                {
                    wprintf(CELL_ALREADY_OPENED);
                    continue;
                }

                if (around_revealed + around_out + grid[y * grid_size.x + x] != 9)
                {
                    wprintf(AUTO_FLAG_FAILURE);
                    continue;
                }
            }

            return 'f';
        }

        if (grid_status[y * grid_size.x + x] == STATUS_REVEALED)
        {
            uint32_t around_flagged = 0, around_unrevealed = 0;

            for (int32_t around_y = y - 1; around_y <= (int32_t)y + 1; ++around_y)
                for (int32_t around_x = x - 1; around_x <= (int32_t)x + 1; ++around_x)
                {
                    CellStatus status = grid_status_get_checked(around_x, around_y);
                    around_flagged    += status == STATUS_FLAGGED;
                    around_unrevealed += status == STATUS_UNREVEALED;
                }

            if (around_unrevealed == 0 || grid[y * grid_size.x + x] == CELL_REWIND_MINE)
            {
                wprintf(CELL_ALREADY_OPENED);
                continue;
            }

            if (around_flagged != grid[y * grid_size.x + x])
            {
                wprintf(AUTO_REVEAL_FAILURE);
                continue;
            }
        }

        return 's';
    }
}

bool grid_scan_acceptance(void)
{
    wprintf(ANSI_CURSOR_POSITION_SAVE() L"\n"
        ERROR("You stumbled upon a mine! Do you accept your defeat?" L"\n")
        INFO(
            L" " OPTION(L"` `") L" - Gracefully accept your mistake" L"\n",
            L" " OPTION(L"`Z`") L" - Shamelessly undo the last move" L"\n"
            L"\n")
        PROMPT(L""));

    while (true)
    {
        char option;
        int32_t code = console_scan_input("c", &option);

        if (code <= 0)
        {
            if (code == 0)
            {
                wprintf(ANSI_CURSOR_POSITION_RESTORE() ANSI_CLEAR_FORWARD());
                return true;
            }

            continue;
        }

        if ((option | 32) != 'z')
        {
            wprintf(INVALID_OPTION);
            continue;
        }

        wprintf(ANSI_CURSOR_POSITION_RESTORE() ANSI_CLEAR_FORWARD());
        return false;
    }
}

bool grid_select(Vector2UInt32 *selected, uint32_t *cell_count, int32_t *flag_count)
{
    switch (grid_status[selected->y * grid_size.x + selected->x])
    {
        case STATUS_UNREVEALED:
            switch (grid[selected->y * grid_size.x + selected->x])
            {
                case CELL_MINE:
                    grid_status[selected->y * grid_size.x + selected->x] = STATUS_EXPLODED;
                    return true;

                case CELL_REWIND_MINE:
                    grid_rewind(*selected, cell_count, flag_count);
                    return false;

                default:
                    grid_status[selected->y * grid_size.x + selected->x] = STATUS_REVEALED;
                    --*cell_count;
                    grid_select_cascade(*selected, cell_count);
                    return false;
            }

        case STATUS_REVEALED:
            for (int32_t y = selected->y - 1; y <= (int32_t)selected->y + 1; ++y)
                for (int32_t x = selected->x - 1; x <= (int32_t)selected->x + 1; ++x)
                    if (grid_status_get_checked(x, y) == STATUS_UNREVEALED &&
                        grid[y * grid_size.x + x] == CELL_MINE)
                    {
                        grid_status[y * grid_size.x + x] = STATUS_EXPLODED;
                        *selected = (Vector2UInt32) { x, y };
                        return true;
                    }

            for (int32_t y = selected->y - 1; y <= (int32_t)selected->y + 1; ++y)
                for (int32_t x = selected->x - 1; x <= (int32_t)selected->x + 1; ++x)
                    if (grid_status_get_checked(x, y) == STATUS_UNREVEALED &&
                        grid[y * grid_size.x + x] == CELL_REWIND_MINE)
                    {
                        grid_rewind((Vector2UInt32) { x, y }, cell_count, flag_count);
                        return false;
                    }

            for (int32_t y = selected->y - 1; y <= (int32_t)selected->y + 1; ++y)
                for (int32_t x = selected->x - 1; x <= (int32_t)selected->x + 1; ++x)
                    if (grid_status_get_checked(x, y) == STATUS_UNREVEALED &&
                        (uint32_t)grid[y * grid_size.x + x] < CELL_OUT)
                    {
                        grid_status[y * grid_size.x + x] = STATUS_REVEALED;
                        --*cell_count;
                        grid_select_cascade((Vector2UInt32) { x, y }, cell_count);
                    }

            return false;

        case STATUS_FLAGGED:
            grid_status[selected->y * grid_size.x + selected->x] = STATUS_UNREVEALED;
            ++*flag_count;
            return false;

        default:
            return false;
    }
}

void grid_select_cascade(Vector2UInt32 selected, int32_t *cell_count)
{
    static Vector2Int32 stack[GRID_SIZE_MAX];

    int32_t top = 0;
    stack[top] = (Vector2Int32) { selected.x, selected.y };

    while (top >= 0)
    {
        Vector2Int32 current = stack[top--];

        if (grid[current.y * grid_size.x + current.x] == CELL_0)
            for (int32_t y = current.y - 1; y <= current.y + 1; ++y)
                for (int32_t x = current.x - 1; x <= current.x + 1; ++x)
                    if (grid_status_get_checked(x, y) == STATUS_UNREVEALED &&
                        grid[y * grid_size.x + x] != CELL_REWIND_MINE)
                    {
                        grid_status[y * grid_size.x + x] = STATUS_REVEALED;
                        --*cell_count;
                        stack[++top] = (Vector2Int32) { x, y };
                    }
    }
}

void grid_select_flag(Vector2UInt32 selected, int32_t *flag_count)
{
    switch (grid_status[selected.y * grid_size.x + selected.x])
    {
        case STATUS_UNREVEALED:
            grid_status[selected.y * grid_size.x + selected.x] = STATUS_FLAGGED;
            --*flag_count;
            break;

        case STATUS_FLAGGED:
            grid_status[selected.y * grid_size.x + selected.x] = STATUS_UNREVEALED;
            ++*flag_count;
            break;

        case STATUS_REVEALED:
            bool unflag = true;

            for (int32_t y = selected.y - 1; y <= (int32_t)selected.y + 1; ++y)
                for (int32_t x = selected.x - 1; x <= (int32_t)selected.x + 1; ++x)
                    if (grid_status_get_checked(x, y) == STATUS_UNREVEALED)
                    {
                        unflag = false;
                        break;
                    }

            if (unflag)
            {
                for (int32_t y = selected.y - 1; y <= (int32_t)selected.y + 1; ++y)
                    for (int32_t x = selected.x - 1; x <= (int32_t)selected.x + 1; ++x)
                        if (grid_status_get_checked(x, y) == STATUS_FLAGGED)
                        {
                            grid_status[y * grid_size.x + x] = STATUS_UNREVEALED;
                            ++*flag_count;
                        }
            }
            else
            {
                for (int32_t y = selected.y - 1; y <= (int32_t)selected.y + 1; ++y)
                    for (int32_t x = selected.x - 1; x <= (int32_t)selected.x + 1; ++x)
                        if (grid_status_get_checked(x, y) == STATUS_UNREVEALED)
                        {
                            grid_status[y * grid_size.x + x] = STATUS_FLAGGED;
                            --*flag_count;
                        }
            }

            break;
    }
}

void grid_undo_last_move(Vector2UInt32 last_move)
{
    grid_status[last_move.y * grid_size.x + last_move.x] = STATUS_UNREVEALED;
}

void grid_clear_field(int32_t flag_count, TimeSpan timer)
{
    static Vector2UInt32 mines[MINE_COUNT_MAX];

    int32_t top = 0;

    wprintf(ANSI_CURSOR_DISABLE());

    for (int32_t y = 0; y < grid_size.y; ++y)
    {
        for (int32_t x = 0; x < grid_size.x; ++x)
            if ((uint32_t)grid[y * grid_size.x + x] < CELL_OUT)
                grid_status[y * grid_size.x + x] = STATUS_WATER;
            else
                mines[top++] = (Vector2UInt32) { x, y };

        grid_print(flag_count, timer);
        _sleep(50);

        uint32_t flags_or_rewind_mines = 0;
        for (int32_t x = 0; x < grid_size.x; ++x)
            if (grid_status[y * grid_size.x + x] == STATUS_FLAGGED ||
                grid[y * grid_size.x + x] == CELL_REWIND_MINE)
            {
                grid_status[y * grid_size.x + x] = STATUS_UNREVEALED;
                ++flags_or_rewind_mines;
            }

        if (flags_or_rewind_mines)
        {
            grid_print(flag_count, timer);
            _sleep(10 * flags_or_rewind_mines);
        }
    }

    for (uint32_t i = 0; i < top; ++i)
    {
        uint32_t j = i + rand() / (RAND_MAX / (top - i) + 1);
        Vector2UInt32 t = mines[j];
        mines[j] = mines[i];
        mines[i] = t;
    }

    for (int32_t step = mine_count_initial / 10 + 1; top > 0; _sleep(50))
    {
        do
        {
            Vector2UInt32 mine = mines[--top];
            grid_status[mine.y * grid_size.x + mine.x] = STATUS_CLEARED;
        }
        while (top % step);

        grid_print(flag_count, timer);
    }
}

void grid_reveal_all_mines(int32_t flag_count, TimeSpan timer)
{
    static Vector2UInt32 mines[MINE_COUNT_MAX];

    wprintf(ANSI_CURSOR_DISABLE());

    int32_t top = 0;

    for (int32_t y = 0; y < grid_size.y; ++y)
        for (int32_t x = 0; x < grid_size.x; ++x)
        {
            Cell cell = grid[y * grid_size.x + x];
            if ((uint32_t)grid[y * grid_size.x + x] >= CELL_OUT && grid_status[y * grid_size.x + x] != STATUS_EXPLODED)
                mines[top++] = (Vector2UInt32) { x, y };
        }

    for (uint32_t i = 0; i < top; ++i)
    {
        uint32_t j = i + rand() / (RAND_MAX / (top - i) + 1);
        Vector2UInt32 t = mines[j];
        mines[j] = mines[i];
        mines[i] = t;
    }

    for (int32_t step = (mine_count_initial + rewind_mine_count_initial) / 10 + 1; top > 0; _sleep(200))
    {
        do
        {
            Vector2UInt32 mine = mines[--top];
            grid_status[mine.y * grid_size.x + mine.x] = STATUS_REVEALED;
        }
        while (top % step);

        grid_print(flag_count, timer);
    }
}

static uint32_t mangle_bits(uint32_t value)
{
    uint32_t lfsr = value;
    lfsr ^= lfsr >> 7;
    lfsr ^= lfsr << 9;
    lfsr ^= lfsr >> 13;

    uint32_t pop_count = value;
    pop_count = (pop_count & 0x55555555u) + ((pop_count >> 1) & 0x55555555u);
    pop_count = (pop_count & 0x33333333u) + ((pop_count >> 2) & 0x33333333u);
    pop_count = (pop_count & 0x0f0f0f0fu) + ((pop_count >> 4) & 0x0f0f0f0fu);
    pop_count = (pop_count & 0x00ff00ffu) + ((pop_count >> 8) & 0x00ff00ffu);
    pop_count = (pop_count & 0x0000ffffu) + ((pop_count >> 16) & 0x0000ffffu);

    return value ^ lfsr ^ pop_count;
}

static bool mangle_to_boolean(uint32_t value)
{
    return mangle_bits(value) & 1;
}
