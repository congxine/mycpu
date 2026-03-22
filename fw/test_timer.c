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

static unsigned int timer_ms(void) {
    volatile unsigned int *t = (volatile unsigned int *)TIMER_LOW_ADDR;
    return *t;
}

static void print_digit(int x) {
    putchar_uart((char)('0' + x));
}

int main(void) {
    unsigned int next_tick;
    int remain = 5;

    puts_uart("Timer countdown demo\r\n");
    puts_uart("Press q to quit early.\r\n");

    puts_uart("start: ");
    print_digit(remain);
    puts_uart("\r\n");

    next_tick = timer_ms() + 1000u;

    while (remain > 0) {
        int ch = getchar_nonblock();
        if (ch == 'q') {
            puts_uart("quit\r\n");
            return 0;
        }

        if ((int)(timer_ms() - next_tick) >= 0) {
            remain--;
            puts_uart("tick: ");
            print_digit(remain);
            puts_uart("\r\n");
            next_tick += 1000u;
        }
    }

    puts_uart("done\r\n");
    return 0;
}
