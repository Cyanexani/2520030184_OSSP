#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <termios.h>
#include <unistd.h>

#define BUF_SIZE 256
#define MAX_HISTORY 50
#define PROMPT "hist> "

static struct termios original;
static int raw_mode = 0;
static char *history[MAX_HISTORY];
static int history_count = 0;

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

/* \r goes back to column 0 and \033[K clears the rest of the line */
static void redraw(const char *buf) {
    printf("\r\033[K" PROMPT "%s", buf);
    fflush(stdout);
}

static void add_history(const char *line) {
    if (history_count > 0 && strcmp(history[history_count - 1], line) == 0)
        return;
    if (history_count == MAX_HISTORY) {
        free(history[0]);
        memmove(history, history + 1, (MAX_HISTORY - 1) * sizeof(char *));
        history_count--;
    }
    history[history_count++] = strdup(line);
}

int main(void) {
    char buf[BUF_SIZE] = "";
    int len = 0;
    int pos = 0;
    unsigned char c;

    if (isatty(STDIN_FILENO))
        enable_raw_mode();
    printf("Up and Down arrows browse history, 'history' lists it, 'exit' quits.\n");
    redraw(buf);

    while (read(STDIN_FILENO, &c, 1) == 1) {
        if (c == 27) {
            unsigned char seq[2];
            if (read(STDIN_FILENO, &seq[0], 1) != 1 || read(STDIN_FILENO, &seq[1], 1) != 1)
                break;
            if (seq[0] != '[')
                continue;
            if (seq[1] == 'A' && pos > 0) {
                pos--;
                snprintf(buf, sizeof(buf), "%s", history[pos]);
            } else if (seq[1] == 'B' && pos < history_count) {
                pos++;
                if (pos == history_count)
                    buf[0] = '\0';
                else
                    snprintf(buf, sizeof(buf), "%s", history[pos]);
            }
            len = strlen(buf);
            redraw(buf);
        } else if (c == '\r' || c == '\n') {
            printf("\n");
            if (len > 0) {
                if (strcmp(buf, "exit") == 0)
                    break;
                if (strcmp(buf, "history") == 0) {
                    for (int i = 0; i < history_count; i++)
                        printf("%4d  %s\n", i + 1, history[i]);
                } else {
                    printf("you entered: %s\n", buf);
                }
                add_history(buf);
            }
            len = 0;
            buf[0] = '\0';
            pos = history_count;
            redraw(buf);
        } else if (c == 127 || c == 8) {
            if (len > 0) {
                buf[--len] = '\0';
                redraw(buf);
            }
        } else if (c == 4) {
            if (len == 0)
                break;
        } else if (c >= 32 && c < 127 && len < BUF_SIZE - 1) {
            buf[len++] = c;
            buf[len] = '\0';
            putchar(c);
            fflush(stdout);
        }
    }

    printf("\n");
    for (int i = 0; i < history_count; i++)
        free(history[i]);
    return 0;
}
