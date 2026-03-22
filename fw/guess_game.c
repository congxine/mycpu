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

static int streq(const char *a, const char *b) {
    while (*a && *b) {
        if (*a != *b) return 0;
        a++;
        b++;
    }
    return *a == 0 && *b == 0;
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

static unsigned int next_rand(unsigned int x) {
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return x;
}

static unsigned int urem32(unsigned int num, unsigned int den) {
    unsigned int r = 0;
    int i;

    for (i = 31; i >= 0; --i) {
        r = (r << 1) | ((num >> i) & 1u);
        if (r >= den) {
            r -= den;
        }
    }

    return r;
}

int main(void) {
    char line[64];
    unsigned int seed = timer_ms() ^ 0x12345678u;

    puts_uart("RV32I Guess Number Game\r\n");
    puts_uart("Guess a number between 1 and 100.\r\n");
    puts_uart("Type q to quit.\r\n\r\n");

    for (;;) {
        int tries = 0;
        int guessed = 0;
        int target;
        int value;
        const char *p;

        seed = next_rand(seed ^ timer_ms());
        target = (int)urem32(seed, 100u) + 1;

        puts_uart("New round started!\r\n");

        while (!guessed) {
            puts_uart("guess> ");
            read_line(line, (int)sizeof(line));

            if (streq(line, "q")) {
                puts_uart("bye\r\n");
                return 0;
            }

            p = line;
            if (!parse_int32(&p, &value)) {
                puts_uart("please enter a valid number\r\n");
                continue;
            }

            skip_spaces(&p);
            if (*p != '\0') {
                puts_uart("please enter a valid number\r\n");
                continue;
            }

            tries++;

            if (value < target) {
                puts_uart("too small\r\n");
            } else if (value > target) {
                puts_uart("too big\r\n");
            } else {
                puts_uart("correct! tries = ");
                print_int_dec(tries);
                puts_uart("\r\n\r\n");
                guessed = 1;
            }
        }

        puts_uart("Press Enter to play again, or q to quit.\r\n");
        puts_uart("> ");
        read_line(line, (int)sizeof(line));
        if (streq(line, "q")) {
            puts_uart("bye\r\n");
            return 0;
        }
        puts_uart("\r\n");
    }
}
