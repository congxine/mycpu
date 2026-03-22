#define UART_TX_ADDR    0x10000000u
#define KBD_STATUS_ADDR 0x10000010u
#define KBD_DATA_ADDR   0x10000014u
#define TIMER_LOW_ADDR  0x10000018u

#define MAZE_W 17
#define MAZE_H 8
#define LEVEL_COUNT 2
#define PATROL_MAX 4
#define ALERT_TURNS 8

#define STATE_PLAYING 0
#define STATE_WIN     1
#define STATE_LOSE    2

static void putchar_uart(char c) {
    volatile unsigned char *uart = (volatile unsigned char *)UART_TX_ADDR;
    *uart = (unsigned char)c;
}

static void puts_uart(const char *s) {
    while (*s) {
        putchar_uart(*s++);
    }
}

static int getchar_nonblock(void) {
    volatile unsigned int *status = (volatile unsigned int *)KBD_STATUS_ADDR;
    volatile unsigned int *data   = (volatile unsigned int *)KBD_DATA_ADDR;

    if (*status) {
        return (int)(*data & 0xffu);
    }
    return -1;
}

static int getchar_blocking(void) {
    for (;;) {
        int ch = getchar_nonblock();
        if (ch >= 0) {
            return ch;
        }
    }
}

static unsigned int timer_ms(void) {
    volatile unsigned int *t = (volatile unsigned int *)TIMER_LOW_ADDR;
    return *t;
}

static unsigned int uabs32(int x) {
    if (x >= 0) return (unsigned int)x;
    return (unsigned int)(-(x + 1)) + 1u;
}

static void print_uint_dec(unsigned int x) {
    static const unsigned int pow10[] = {
        1000000000u, 100000000u, 10000000u, 1000000u, 100000u,
        10000u, 1000u, 100u, 10u, 1u
    };

    int started = 0;
    int i;

    for (i = 0; i < 10; ++i) {
        unsigned int d = pow10[i];
        int digit = 0;

        while (x >= d) {
            x -= d;
            digit++;
        }

        if (digit != 0 || started || d == 1u) {
            putchar_uart((char)('0' + digit));
            started = 1;
        }
    }
}

static void print_int_dec(int x) {
    if (x < 0) {
        putchar_uart('-');
        print_uint_dec(uabs32(x));
    } else {
        print_uint_dec((unsigned int)x);
    }
}

static void clear_screen(void) {
    puts_uart("\x1b[2J\x1b[H");
}

static int abs_diff(int a, int b) {
    if (a >= b) return a - b;
    return b - a;
}

static int manhattan(int x1, int y1, int x2, int y2) {
    return abs_diff(x1, x2) + abs_diff(y1, y2);
}

static const char levels[LEVEL_COUNT][MAZE_H][MAZE_W + 1] = {
    {
        "#################",
        "#S   ##   #   EG#",
        "# ##    # # ##  #",
        "#   #### #  #   #",
        "###      ## # ###",
        "#   ####    #   #",
        "# #      #### # #",
        "#################"
    },
    {
        "#################",
        "#S #      E#   G#",
        "#  # ##### # # ##",
        "## # #   # # #  #",
        "#    # # #   ## #",
        "# #### # ####   #",
        "#              ##",
        "#################"
    }
};

static const int patrol_counts[LEVEL_COUNT] = { 4, 4 };

static const int patrol_x[LEVEL_COUNT][PATROL_MAX] = {
    { 14, 11, 13, 11 },
    { 10,  6, 13,  8 }
};

static const int patrol_y[LEVEL_COUNT][PATROL_MAX] = {
    {  1,  1,  5,  5 },
    {  1,  1,  6,  6 }
};

static char level_buf[MAZE_H][MAZE_W + 1];

static void copy_level(int idx) {
    int y, x;
    for (y = 0; y < MAZE_H; ++y) {
        for (x = 0; x <= MAZE_W; ++x) {
            level_buf[y][x] = levels[idx][y][x];
        }
    }
}

static void find_entities(int *px, int *py, int *ex, int *ey) {
    int y, x;

    *px = 1;
    *py = 1;
    *ex = MAZE_W - 2;
    *ey = 1;

    for (y = 0; y < MAZE_H; ++y) {
        for (x = 0; x < MAZE_W; ++x) {
            if (level_buf[y][x] == 'S') {
                *px = x;
                *py = y;
                level_buf[y][x] = ' ';
            } else if (level_buf[y][x] == 'E') {
                *ex = x;
                *ey = y;
                level_buf[y][x] = ' ';
            }
        }
    }
}

static int enemy_can_step(int nx, int ny) {
    char tile;

    if (nx < 0 || nx >= MAZE_W || ny < 0 || ny >= MAZE_H) {
        return 0;
    }

    tile = level_buf[ny][nx];
    if (tile == '#') return 0;
    if (tile == 'G') return 0;

    return 1;
}

static int has_clear_row(int ex, int ey, int px, int py) {
    int x0, x1, x;

    if (ey != py) return 0;

    if (ex < px) {
        x0 = ex + 1;
        x1 = px - 1;
    } else {
        x0 = px + 1;
        x1 = ex - 1;
    }

    for (x = x0; x <= x1; ++x) {
        if (level_buf[ey][x] == '#') {
            return 0;
        }
    }

    return 1;
}

static int has_clear_col(int ex, int ey, int px, int py) {
    int y0, y1, y;

    if (ex != px) return 0;

    if (ey < py) {
        y0 = ey + 1;
        y1 = py - 1;
    } else {
        y0 = py + 1;
        y1 = ey - 1;
    }

    for (y = y0; y <= y1; ++y) {
        if (level_buf[y][ex] == '#') {
            return 0;
        }
    }

    return 1;
}

static int enemy_detects_player(int ex, int ey, int px, int py) {
    if (manhattan(ex, ey, px, py) <= 5) {
        return 1;
    }

    if (has_clear_row(ex, ey, px, py)) {
        return 1;
    }

    if (has_clear_col(ex, ey, px, py)) {
        return 1;
    }

    return 0;
}

static void step_enemy_toward(int *ex, int *ey,
                              int target_x, int target_y,
                              int prev_ex, int prev_ey) {
    static const int dxs[4] = { 1, -1, 0, 0 };
    static const int dys[4] = { 0, 0, 1, -1 };

    int best_x = *ex;
    int best_y = *ey;
    int best_score = 0x7fffffff;
    int found = 0;
    int i;

    for (i = 0; i < 4; ++i) {
        int nx = *ex + dxs[i];
        int ny = *ey + dys[i];
        int score;

        if (!enemy_can_step(nx, ny)) {
            continue;
        }

        score = manhattan(nx, ny, target_x, target_y);

        if (nx == prev_ex && ny == prev_ey) {
            score += 2;
        }

        if (!found || score < best_score) {
            best_score = score;
            best_x = nx;
            best_y = ny;
            found = 1;
        }
    }

    if (found) {
        *ex = best_x;
        *ey = best_y;
    }
}

static void enemy_take_turn(int level_idx,
                            int *ex, int *ey,
                            int px, int py,
                            int *patrol_idx,
                            int *alert_left,
                            int *last_seen_x,
                            int *last_seen_y,
                            int *prev_ex,
                            int *prev_ey) {
    int target_x;
    int target_y;
    int seen = enemy_detects_player(*ex, *ey, px, py);

    if (seen) {
        *last_seen_x = px;
        *last_seen_y = py;
        *alert_left = ALERT_TURNS;
    }

    *prev_ex = *ex;
    *prev_ey = *ey;

    if (*alert_left > 0) {
        target_x = *last_seen_x;
        target_y = *last_seen_y;
        step_enemy_toward(ex, ey, target_x, target_y, *prev_ex, *prev_ey);

        if (*ex == target_x && *ey == target_y) {
            *alert_left = 0;
        } else if (!seen) {
            *alert_left = *alert_left - 1;
        }
        return;
    }

    if (*ex == patrol_x[level_idx][*patrol_idx] &&
        *ey == patrol_y[level_idx][*patrol_idx]) {
        *patrol_idx = *patrol_idx + 1;
        if (*patrol_idx >= patrol_counts[level_idx]) {
            *patrol_idx = 0;
        }
    }

    target_x = patrol_x[level_idx][*patrol_idx];
    target_y = patrol_y[level_idx][*patrol_idx];
    step_enemy_toward(ex, ey, target_x, target_y, *prev_ex, *prev_ey);

    if (*ex == patrol_x[level_idx][*patrol_idx] &&
        *ey == patrol_y[level_idx][*patrol_idx]) {
        *patrol_idx = *patrol_idx + 1;
        if (*patrol_idx >= patrol_counts[level_idx]) {
            *patrol_idx = 0;
        }
    }
}

static void draw_game(int level_idx, int px, int py, int ex, int ey,
                      int turns, unsigned int elapsed_ms, int alert_left) {
    int y, x;

    clear_screen();
    puts_uart("RV32I Maze Game 4.0\r\n");
    puts_uart("w a s d move | r restart | q quit\r\n");
    puts_uart("Reach G, avoid E\r\n");
    puts_uart("Level: ");
    print_int_dec(level_idx + 1);
    puts_uart("/");
    print_int_dec(LEVEL_COUNT);
    puts_uart("   Turns: ");
    print_int_dec(turns);
    puts_uart("   Time(ms): ");
    print_uint_dec(elapsed_ms);
    puts_uart("\r\nEnemy: ");
    if (alert_left > 0) {
        puts_uart("CHASE");
    } else {
        puts_uart("PATROL");
    }
    puts_uart("\r\n\r\n");

    for (y = 0; y < MAZE_H; ++y) {
        for (x = 0; x < MAZE_W; ++x) {
            char ch = level_buf[y][x];

            if (x == px && y == py) {
                ch = 'P';
            } else if (x == ex && y == ey) {
                ch = 'E';
            }

            putchar_uart(ch);
        }
        puts_uart("\r\n");
    }
}

static int try_move_player(int *px, int *py, int dx, int dy) {
    int nx = *px + dx;
    int ny = *py + dy;
    char tile;

    if (nx < 0 || nx >= MAZE_W || ny < 0 || ny >= MAZE_H) {
        return 0;
    }

    tile = level_buf[ny][nx];
    if (tile == '#') {
        return 0;
    }

    *px = nx;
    *py = ny;

    if (tile == 'G') {
        return 2;
    }

    return 1;
}

static void draw_level_clear(int level_idx, int turns, unsigned int elapsed_ms) {
    clear_screen();
    puts_uart("Level clear!\r\n\r\n");
    puts_uart("Level: ");
    print_int_dec(level_idx + 1);
    puts_uart("\r\nTurns: ");
    print_int_dec(turns);
    puts_uart("\r\nTime(ms): ");
    print_uint_dec(elapsed_ms);
    puts_uart("\r\n\r\n");

    if (level_idx + 1 < LEVEL_COUNT) {
        puts_uart("Press n for next level, r to replay, q to quit.\r\n");
    } else {
        puts_uart("All levels cleared!\r\n");
        puts_uart("Press r to play again, q to quit.\r\n");
    }
}

static void draw_game_over(int level_idx, int turns, unsigned int elapsed_ms) {
    clear_screen();
    puts_uart("Caught by enemy!\r\n\r\n");
    puts_uart("Level: ");
    print_int_dec(level_idx + 1);
    puts_uart("\r\nTurns: ");
    print_int_dec(turns);
    puts_uart("\r\nTime(ms): ");
    print_uint_dec(elapsed_ms);
    puts_uart("\r\n\r\n");
    puts_uart("Press r to retry, q to quit.\r\n");
}

static void draw_title(void) {
    clear_screen();
    puts_uart("RV32I Maze Game 4.0\r\n");
    puts_uart("===================\r\n\r\n");
    puts_uart("Controls:\r\n");
    puts_uart("  w a s d : move\r\n");
    puts_uart("  r       : restart current level\r\n");
    puts_uart("  q       : quit\r\n\r\n");
    puts_uart("Enemy AI:\r\n");
    puts_uart("  PATROL -> follows a route\r\n");
    puts_uart("  CHASE  -> spots you and keeps hunting for a while\r\n\r\n");
    puts_uart("Press any key to start.\r\n");
}

static void start_level(int level_idx,
                        int *px, int *py,
                        int *ex, int *ey,
                        int *turns,
                        unsigned int *level_start_ms,
                        int *state,
                        int *patrol_idx,
                        int *alert_left,
                        int *last_seen_x,
                        int *last_seen_y,
                        int *prev_ex,
                        int *prev_ey) {
    copy_level(level_idx);
    find_entities(px, py, ex, ey);
    *turns = 0;
    *level_start_ms = timer_ms();
    *state = STATE_PLAYING;
    *patrol_idx = 0;
    *alert_left = 0;
    *last_seen_x = *px;
    *last_seen_y = *py;
    *prev_ex = *ex;
    *prev_ey = *ey;
    draw_game(level_idx, *px, *py, *ex, *ey, *turns, 0, *alert_left);
}

int main(void) {
    int level_idx = 0;
    int px, py;
    int ex, ey;
    int turns = 0;
    int state = STATE_PLAYING;
    int patrol_idx = 0;
    int alert_left = 0;
    int last_seen_x = 0;
    int last_seen_y = 0;
    int prev_ex = 0;
    int prev_ey = 0;
    unsigned int level_start_ms = 0;

    draw_title();
    getchar_blocking();

    start_level(level_idx,
                &px, &py, &ex, &ey,
                &turns, &level_start_ms, &state,
                &patrol_idx, &alert_left,
                &last_seen_x, &last_seen_y,
                &prev_ex, &prev_ey);

    for (;;) {
        int ch = getchar_blocking();

        if (state == STATE_PLAYING) {
            int recognized = 1;
            int rc = 0;

            if (ch == 'q' || ch == 'Q') {
                puts_uart("\r\nbye\r\n");
                return 0;
            } else if (ch == 'r' || ch == 'R') {
                start_level(level_idx,
                            &px, &py, &ex, &ey,
                            &turns, &level_start_ms, &state,
                            &patrol_idx, &alert_left,
                            &last_seen_x, &last_seen_y,
                            &prev_ex, &prev_ey);
                continue;
            } else if (ch == 'w' || ch == 'W') {
                rc = try_move_player(&px, &py, 0, -1);
            } else if (ch == 's' || ch == 'S') {
                rc = try_move_player(&px, &py, 0, 1);
            } else if (ch == 'a' || ch == 'A') {
                rc = try_move_player(&px, &py, -1, 0);
            } else if (ch == 'd' || ch == 'D') {
                rc = try_move_player(&px, &py, 1, 0);
            } else {
                recognized = 0;
            }

            if (!recognized) {
                continue;
            }

            turns++;

            if (px == ex && py == ey) {
                state = STATE_LOSE;
                draw_game_over(level_idx, turns, timer_ms() - level_start_ms);
                continue;
            }

            if (rc == 2) {
                state = STATE_WIN;
                draw_level_clear(level_idx, turns, timer_ms() - level_start_ms);
                continue;
            }

            enemy_take_turn(level_idx,
                            &ex, &ey,
                            px, py,
                            &patrol_idx,
                            &alert_left,
                            &last_seen_x,
                            &last_seen_y,
                            &prev_ex,
                            &prev_ey);

            if (px == ex && py == ey) {
                state = STATE_LOSE;
                draw_game_over(level_idx, turns, timer_ms() - level_start_ms);
                continue;
            }

            draw_game(level_idx, px, py, ex, ey,
                      turns, timer_ms() - level_start_ms, alert_left);
        } else if (state == STATE_WIN) {
            if (ch == 'q' || ch == 'Q') {
                puts_uart("\r\nbye\r\n");
                return 0;
            }

            if (level_idx + 1 < LEVEL_COUNT) {
                if (ch == 'n' || ch == 'N') {
                    level_idx++;
                    start_level(level_idx,
                                &px, &py, &ex, &ey,
                                &turns, &level_start_ms, &state,
                                &patrol_idx, &alert_left,
                                &last_seen_x, &last_seen_y,
                                &prev_ex, &prev_ey);
                } else if (ch == 'r' || ch == 'R') {
                    start_level(level_idx,
                                &px, &py, &ex, &ey,
                                &turns, &level_start_ms, &state,
                                &patrol_idx, &alert_left,
                                &last_seen_x, &last_seen_y,
                                &prev_ex, &prev_ey);
                }
            } else {
                if (ch == 'r' || ch == 'R') {
                    level_idx = 0;
                    start_level(level_idx,
                                &px, &py, &ex, &ey,
                                &turns, &level_start_ms, &state,
                                &patrol_idx, &alert_left,
                                &last_seen_x, &last_seen_y,
                                &prev_ex, &prev_ey);
                }
            }
        } else {
            if (ch == 'q' || ch == 'Q') {
                puts_uart("\r\nbye\r\n");
                return 0;
            } else if (ch == 'r' || ch == 'R') {
                start_level(level_idx,
                            &px, &py, &ex, &ey,
                            &turns, &level_start_ms, &state,
                            &patrol_idx, &alert_left,
                            &last_seen_x, &last_seen_y,
                            &prev_ex, &prev_ey);
            }
        }
    }
}
