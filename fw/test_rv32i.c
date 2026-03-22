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

static volatile unsigned int   g_word  = 0;
static volatile unsigned short g_half  = 0;
static volatile unsigned char  g_byte  = 0;
static volatile signed char    g_sbyte = 0;
static volatile signed short   g_shalf = 0;

static int test_add_sub(void) {
    volatile int a = 7;
    volatile int b = 3;
    if ((a + b) != 10) return 0;
    if ((a - b) != 4) return 0;
    return 1;
}

static int test_logic(void) {
    volatile unsigned int x = 0x55aa00ffu;
    volatile unsigned int y = 0x0f0f00f0u;

    if ((x & y) != 0x050a00f0u) return 0;
    if ((x | y) != 0x5faf00ffu) return 0;
    if ((x ^ y) != 0x5aa5000fu) return 0;
    return 1;
}

static int test_shift(void) {
    volatile unsigned int x = 1u;
    volatile int y = -64;

    x = x << 7;
    if (x != 128u) return 0;

    x = x >> 3;
    if (x != 16u) return 0;

    if ((y >> 2) != -16) return 0;
    return 1;
}

static int test_compare(void) {
    volatile int a = -3;
    volatile int b = 2;
    volatile unsigned int ua = 1u;
    volatile unsigned int ub = 7u;

    if (!(a < b)) return 0;
    if (!(b >= a)) return 0;
    if (!(ua < ub)) return 0;
    if (!(ub >= ua)) return 0;
    return 1;
}

static int test_load_store(void) {
    g_word = 0x11223344u;
    if (g_word != 0x11223344u) return 0;

    g_half = 0xabcd;
    if (g_half != 0xabcd) return 0;

    g_byte = 0x5a;
    if (g_byte != 0x5a) return 0;

    g_sbyte = -5;
    if (g_sbyte != -5) return 0;

    g_shalf = -1234;
    if (g_shalf != -1234) return 0;

    return 1;
}

static int test_branch_loop(void) {
    volatile int sum = 0;
    volatile int i = 0;

    for (i = 0; i < 10; ++i) {
        if ((i & 1) == 0) {
            sum += i;
        } else {
            sum -= 1;
        }
    }

    return sum == 15;
}

static int add_one(int x) {
    return x + 1;
}

static int test_call_indirect(void) {
    int (*fn)(int) = add_one;
    return fn(41) == 42;
}

static int run_test(const char *name, int (*fn)(void)) {
    puts_uart("[TEST] ");
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

int main(void) {
    int ok = 1;

    ok &= run_test("add_sub",      test_add_sub);
    ok &= run_test("logic",        test_logic);
    ok &= run_test("shift",        test_shift);
    ok &= run_test("compare",      test_compare);
    ok &= run_test("load_store",   test_load_store);
    ok &= run_test("branch_loop",  test_branch_loop);
    ok &= run_test("call_indirect", test_call_indirect);

    if (ok) {
        puts_uart("\nALL TESTS PASSED\n");
        return 0;
    } else {
        puts_uart("\nTEST FAILED\n");
        return 1;
    }
}
