#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <termios.h>
#include <unistd.h>

#define BUF_SIZE 256

static struct termios original;
static int raw_mode = 0;

static void restore_terminal(void) {
    if (raw_mode)
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &original);
}

static void on_sigint(int sig) {
    (void)sig;
    restore_terminal();
    _exit(130);
}

static void enable_raw_mode(void) {
    struct termios raw;

    if (tcgetattr(STDIN_FILENO, &original) == -1) {
        perror("tcgetattr");
        exit(EXIT_FAILURE);
    }
    raw = original;
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        perror("tcsetattr");
        exit(EXIT_FAILURE);
    }
    raw_mode = 1;
    atexit(restore_terminal);
    signal(SIGINT, on_sigint);
}

int main(void) {
    char buf[BUF_SIZE];
    int len = 0;
    unsigned char c;

    if (isatty(STDIN_FILENO))
        enable_raw_mode();

    printf("Type a line and press Enter. Backspace edits, Ctrl-D or 'exit' quits.\n> ");
    fflush(stdout);

    while (read(STDIN_FILENO, &c, 1) == 1) {
        if (c == '\r' || c == '\n') {
            buf[len] = '\0';
            printf("\nline: \"%s\" (%d bytes)\nhex :", buf, len);
            for (int i = 0; i < len; i++)
                printf(" %02x", (unsigned char)buf[i]);
            printf("\n");
            if (strcmp(buf, "exit") == 0)
                break;
            len = 0;
            printf("> ");
        } else if (c == 127 || c == 8) {
            if (len > 0) {
                len--;
                printf("\b \b");
            }
        } else if (c == 4) {
            if (len == 0)
                break;
        } else if (c >= 32 && c < 127) {
            if (len < BUF_SIZE - 1) {
                buf[len++] = c;
                putchar(c);
            } else {
                putchar('\a');
            }
        }
        fflush(stdout);
    }

    printf("\n");
    return 0;
}
