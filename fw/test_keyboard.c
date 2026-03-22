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

int main(void) {
    puts_uart("Keyboard echo demo\r\n");
    puts_uart("Type keys, press q to quit.\r\n> ");

    while (1) {
        int ch = getchar_nonblock();
        if (ch < 0) {
            continue;
        }

        if (ch == 'q') {
            puts_uart("\r\nquit\r\n");
            return 0;
        }

        if (ch == '\r' || ch == '\n') {
            puts_uart("\r\n> ");
            continue;
        }

        putchar_uart((char)ch);
    }
}
