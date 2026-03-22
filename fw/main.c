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

int main(void) {
    puts_uart("Hello RV32I!\n");
    return 0;
}
