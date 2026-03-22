#define UART_TX_ADDR    0x10000000u
#define KBD_STATUS_ADDR 0x10000010u
#define KBD_DATA_ADDR   0x10000014u

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

static void skip_spaces(const char **p) {
    while (**p == ' ' || **p == '\t') {
        (*p)++;
    }
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
        value = (value << 3) + (value << 1) + digit;  /* value * 10 + digit */
        (*p)++;
        saw_digit = 1;
    }

    if (!saw_digit) {
        return 0;
    }

    if (neg) {
        *out = -(int)value;
    } else {
        *out = (int)value;
    }
    return 1;
}

static unsigned int uabs32(int x) {
    if (x >= 0) {
        return (unsigned int)x;
    }
    return (unsigned int)(-(x + 1)) + 1u;
}

static int mul32(int a, int b) {
    unsigned int ua = uabs32(a);
    unsigned int ub = uabs32(b);
    unsigned int result = 0;
    int neg = ((a < 0) ^ (b < 0));

    while (ub != 0) {
        if (ub & 1u) {
            result += ua;
        }
        ua <<= 1;
        ub >>= 1;
    }

    if (neg) {
        return -(int)result;
    }
    return (int)result;
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
    if (neg) {
        return -(int)uq;
    }
    return (int)uq;
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

static int streq(const char *a, const char *b) {
    while (*a && *b) {
        if (*a != *b) {
            return 0;
        }
        a++;
        b++;
    }
    return *a == 0 && *b == 0;
}

/* return 1=ok, 0=parse error, 2=div zero */
static int eval_expr(const char *line, int *result) {
    const char *p = line;
    int lhs, rhs, ok_div;
    char op;

    if (!parse_int32(&p, &lhs)) {
        return 0;
    }

    skip_spaces(&p);
    op = *p;
    if (op != '+' && op != '-' && op != '*' && op != '/') {
        return 0;
    }
    p++;

    if (!parse_int32(&p, &rhs)) {
        return 0;
    }

    skip_spaces(&p);
    if (*p != '\0') {
        return 0;
    }

    switch (op) {
        case '+':
            *result = lhs + rhs;
            return 1;
        case '-':
            *result = lhs - rhs;
            return 1;
        case '*':
            *result = mul32(lhs, rhs);
            return 1;
        case '/':
            *result = div32(lhs, rhs, &ok_div);
            return ok_div ? 1 : 2;
    }

    return 0;
}

int main(void) {
    char line[128];
    int result;
    int rc;

    puts_uart("RV32I calculator\r\n");
    puts_uart("Enter expressions like: 12+34, 9-5, 6*7, 8/2\r\n");
    puts_uart("Spaces and negative numbers are supported.\r\n");
    puts_uart("Type q to quit.\r\n\r\n");

    for (;;) {
        puts_uart("> ");
        read_line(line, (int)sizeof(line));

        if (line[0] == '\0') {
            continue;
        }

        if (streq(line, "q")) {
            puts_uart("bye\r\n");
            return 0;
        }

        rc = eval_expr(line, &result);
        if (rc == 1) {
            puts_uart("= ");
            print_int_dec(result);
            puts_uart("\r\n");
        } else if (rc == 2) {
            puts_uart("error: divide by zero\r\n");
        } else {
            puts_uart("error: use form a+b\r\n");
        }
    }
}
