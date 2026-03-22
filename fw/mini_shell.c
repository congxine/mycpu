#define UART_TX_ADDR    0x10000000u
#define KBD_STATUS_ADDR 0x10000010u
#define KBD_DATA_ADDR   0x10000014u
#define TIMER_LOW_ADDR  0x10000018u

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

static void skip_spaces(const char **p) {
    while (**p == ' ' || **p == '\t') {
        (*p)++;
    }
}

static int streq(const char *a, const char *b) {
    while (*a && *b) {
        if (*a != *b) return 0;
        a++;
        b++;
    }
    return *a == 0 && *b == 0;
}

static int starts_with(const char *s, const char *prefix) {
    while (*prefix) {
        if (*s != *prefix) return 0;
        s++;
        prefix++;
    }
    return 1;
}

static unsigned int uabs32(int x) {
    if (x >= 0) return (unsigned int)x;
    return (unsigned int)(-(x + 1)) + 1u;
}

static int parse_int32(const char **p, int *out) {
    int neg = 0;
    unsigned int value = 0;
    int saw_digit = 0;

    skip_spaces(p);

    if (**p == '+') {
        (*p)++;
    } else if (**p == '-') {
        neg = 1;
        (*p)++;
    }

    while (**p >= '0' && **p <= '9') {
        unsigned int digit = (unsigned int)(**p - '0');
        value = (value << 3) + (value << 1) + digit;
        (*p)++;
        saw_digit = 1;
    }

    if (!saw_digit) {
        return 0;
    }

    *out = neg ? -(int)value : (int)value;
    return 1;
}

static int mul32(int a, int b) {
    unsigned int ua = uabs32(a);
    unsigned int ub = uabs32(b);
    unsigned int result = 0;
    int neg = ((a < 0) ^ (b < 0));

    while (ub != 0) {
        if (ub & 1u) result += ua;
        ua <<= 1;
        ub >>= 1;
    }

    return neg ? -(int)result : (int)result;
}

static unsigned int udiv32(unsigned int num, unsigned int den) {
    unsigned int q = 0;
    unsigned int r = 0;
    int i;

    for (i = 31; i >= 0; --i) {
        r = (r << 1) | ((num >> i) & 1u);
        if (r >= den) {
            r -= den;
            q |= (1u << i);
        }
    }
    return q;
}

static int div32(int a, int b, int *ok) {
    unsigned int ua, ub, uq;
    int neg;

    if (b == 0) {
        *ok = 0;
        return 0;
    }

    ua = uabs32(a);
    ub = uabs32(b);
    uq = udiv32(ua, ub);
    neg = ((a < 0) ^ (b < 0));

    *ok = 1;
    return neg ? -(int)uq : (int)uq;
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

static int read_line(char *buf, int max_len) {
    int n = 0;

    for (;;) {
        int ch = getchar_blocking();

        if (ch == '\r' || ch == '\n') {
            puts_uart("\r\n");
            buf[n] = '\0';
            return n;
        }

        if ((ch == 8 || ch == 127) && n > 0) {
            n--;
            puts_uart("\b \b");
            continue;
        }

        if (ch >= 32 && ch <= 126) {
            if (n + 1 < max_len) {
                buf[n++] = (char)ch;
                putchar_uart((char)ch);
            }
        }
    }
}

static void cmd_help(void) {
    puts_uart("commands:\r\n");
    puts_uart("  help              - show this help\r\n");
    puts_uart("  echo TEXT         - print text\r\n");
    puts_uart("  add A B           - add two ints\r\n");
    puts_uart("  sub A B           - subtract two ints\r\n");
    puts_uart("  mul A B           - multiply two ints\r\n");
    puts_uart("  div A B           - divide two ints\r\n");
    puts_uart("  time              - show ms since boot\r\n");
    puts_uart("  count N           - countdown seconds\r\n");
    puts_uart("  clear             - clear screen\r\n");
    puts_uart("  q                 - quit\r\n");
}

static void cmd_echo(const char *line) {
    const char *p = line + 4;
    skip_spaces(&p);
    puts_uart(p);
    puts_uart("\r\n");
}

static int parse_two_ints(const char *p, int *a, int *b) {
    if (!parse_int32(&p, a)) return 0;
    if (!parse_int32(&p, b)) return 0;
    skip_spaces(&p);
    return *p == '\0';
}

static void cmd_add(const char *line) {
    int a, b;
    if (!parse_two_ints(line + 3, &a, &b)) {
        puts_uart("usage: add A B\r\n");
        return;
    }
    puts_uart("= ");
    print_int_dec(a + b);
    puts_uart("\r\n");
}

static void cmd_sub(const char *line) {
    int a, b;
    if (!parse_two_ints(line + 3, &a, &b)) {
        puts_uart("usage: sub A B\r\n");
        return;
    }
    puts_uart("= ");
    print_int_dec(a - b);
    puts_uart("\r\n");
}

static void cmd_mul(const char *line) {
    int a, b;
    if (!parse_two_ints(line + 3, &a, &b)) {
        puts_uart("usage: mul A B\r\n");
        return;
    }
    puts_uart("= ");
    print_int_dec(mul32(a, b));
    puts_uart("\r\n");
}

static void cmd_div(const char *line) {
    int a, b, ok;
    if (!parse_two_ints(line + 3, &a, &b)) {
        puts_uart("usage: div A B\r\n");
        return;
    }
    puts_uart("= ");
    print_int_dec(div32(a, b, &ok));
    if (!ok) {
        puts_uart("error: divide by zero\r\n");
        return;
    }
    puts_uart("\r\n");
}

static void cmd_time(void) {
    puts_uart("ms = ");
    print_uint_dec(timer_ms());
    puts_uart("\r\n");
}

static void cmd_count(const char *line) {
    const char *p = line + 5;
    int n;
    unsigned int next_tick;

    if (!parse_int32(&p, &n) || n < 0) {
        puts_uart("usage: count N\r\n");
        return;
    }

    skip_spaces(&p);
    if (*p != '\0') {
        puts_uart("usage: count N\r\n");
        return;
    }

    puts_uart("countdown start\r\n");
    next_tick = timer_ms() + 1000u;

    while (n > 0) {
        int ch = getchar_nonblock();
        if (ch == 'q') {
            puts_uart("countdown interrupted\r\n");
            return;
        }

        if ((int)(timer_ms() - next_tick) >= 0) {
            n--;
            print_int_dec(n);
            puts_uart("\r\n");
            next_tick += 1000u;
        }
    }

    puts_uart("done\r\n");
}

static void cmd_clear(void) {
    puts_uart("\x1b[2J\x1b[H");
}

int main(void) {
    char line[128];

    puts_uart("RV32I mini shell\r\n");
    puts_uart("type 'help' for commands\r\n\r\n");

    for (;;) {
        puts_uart("rv32i> ");
        read_line(line, (int)sizeof(line));

        if (line[0] == '\0') {
            continue;
        }

        if (streq(line, "q")) {
            puts_uart("bye\r\n");
            return 0;
        } else if (streq(line, "help")) {
            cmd_help();
        } else if (streq(line, "time")) {
            cmd_time();
        } else if (streq(line, "clear")) {
            cmd_clear();
        } else if (starts_with(line, "echo ")) {
            cmd_echo(line);
        } else if (starts_with(line, "add ")) {
            cmd_add(line);
        } else if (starts_with(line, "sub ")) {
            cmd_sub(line);
        } else if (starts_with(line, "mul ")) {
            cmd_mul(line);
        } else if (starts_with(line, "div ")) {
            cmd_div(line);
        } else if (starts_with(line, "count ")) {
            cmd_count(line);
        } else {
            puts_uart("unknown command\r\n");
        }
    }
}
