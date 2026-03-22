#define UART_TX_ADDR 0x10000000u

static void putchar_uart(char c) {
    volatile unsigned char *uart = (volatile unsigned char *)UART_TX_ADDR;
    *uart = (unsigned char)c;
}

static void puts_uart(const char *s) {
    while (*s) {
        putchar_uart(*s++);
    }
}

static int run_test(const char *name, int (*fn)(void)) {
    puts_uart("[PIPE] ");
    puts_uart(name);
    puts_uart(" ... ");

    if (fn()) {
        puts_uart("OK\n");
        return 1;
    } else {
        puts_uart("FAIL\n");
        return 0;
    }
}

static int test_forward_ex_ex(void) {
    volatile int a = 5;
    volatile int b = 7;
    volatile int c = a + b;
    volatile int d = c + 3;
    volatile int e = d + c;
    return (c == 12) && (d == 15) && (e == 27);
}

static int test_forward_chain(void) {
    volatile int x = 1;
    x = x + 2;
    x = x + 3;
    x = x + 4;
    x = x + 5;
    return x == 15;
}

static int test_load_use_stall(void) {
    volatile int mem = 21;
    volatile int a = mem;
    volatile int b = a + 21;
    return (a == 21) && (b == 42);
}

static int test_store_after_compute(void) {
    volatile int a = 10;
    volatile int b = 20;
    volatile int c = a + b;
    volatile int mem = 0;
    mem = c;
    return mem == 30;
}

static int test_branch_flush_taken(void) {
    volatile int x = 0;
    volatile int y = 0;

    if (1) {
        x = 42;
    } else {
        y = 99;
    }

    return (x == 42) && (y == 0);
}

static int test_branch_flush_not_taken(void) {
    volatile int x = 0;
    volatile int y = 0;

    if (0) {
        x = 99;
    } else {
        y = 42;
    }

    return (x == 0) && (y == 42);
}

static int plus_one(int x) {
    return x + 1;
}

static int test_jal_jalr(void) {
    int (*fn)(int) = plus_one;
    volatile int v = fn(41);
    return v == 42;
}

static int test_x0_is_zero(void) {
    register int zero_reg asm("x0");
    (void)zero_reg;
    return zero_reg == 0;
}

int main(void) {
    int ok = 1;

    ok &= run_test("forward_ex_ex",         test_forward_ex_ex);
    ok &= run_test("forward_chain",         test_forward_chain);
    ok &= run_test("load_use_stall",        test_load_use_stall);
    ok &= run_test("store_after_compute",   test_store_after_compute);
    ok &= run_test("branch_flush_taken",    test_branch_flush_taken);
    ok &= run_test("branch_flush_not_taken",test_branch_flush_not_taken);
    ok &= run_test("jal_jalr",              test_jal_jalr);
    ok &= run_test("x0_is_zero",            test_x0_is_zero);

    if (ok) {
        puts_uart("\nPIPELINE TESTS PASSED\n");
        return 0;
    } else {
        puts_uart("\nPIPELINE TESTS FAILED\n");
        return 1;
    }
}
