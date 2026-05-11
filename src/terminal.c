/* Terminal control via raw POSIX (termios + ANSI escapes). */

#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include "maxlife.h"

static struct termios g_orig;
static bool           g_have_orig = false;
static volatile sig_atomic_t g_quit = 0;

static void on_signal(int sig)
{
    (void)sig;
    g_quit = 1;
}

bool ml_term_init(void)
{
    if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO)) return false;
    if (tcgetattr(STDIN_FILENO, &g_orig) != 0) return false;
    g_have_orig = true;

    struct termios raw = g_orig;
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_iflag &= ~(IXON | INPCK | ISTRIP);
    raw.c_cc[VMIN]  = 0;
    raw.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) != 0) {
        g_have_orig = false;
        return false;
    }
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_signal;
    sigaction(SIGINT,  &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    atexit(ml_term_restore);
    return true;
}

void ml_term_restore(void)
{
    if (g_have_orig) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_orig);
        g_have_orig = false;
    }
    fputs("\x1b[?25h\x1b[?1049l\x1b[0m", stdout);
    fflush(stdout);
}

bool ml_term_size_get(int *w, int *h)
{
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) != 0 || ws.ws_col == 0) {
        if (w) *w = 80;
        if (h) *h = 24;
        return false;
    }
    if (w) *w = ws.ws_col;
    if (h) *h = ws.ws_row;
    return true;
}

bool ml_term_quit_pressed(void)
{
    if (g_quit) return true;
    if (!isatty(STDIN_FILENO)) return false;
    char buf[16];
    ssize_t n = read(STDIN_FILENO, buf, sizeof(buf));
    if (n <= 0) return false;
    for (ssize_t i = 0; i < n; ++i) {
        char c = buf[i];
        if (c == 'q' || c == 'Q' || c == '\x03' || c == '\x1b') return true;
    }
    return false;
}

void ml_term_enter_alt_screen(void) { fputs("\x1b[?1049h", stdout); fflush(stdout); }
void ml_term_exit_alt_screen(void)  { fputs("\x1b[?1049l", stdout); fflush(stdout); }
void ml_term_hide_cursor(void)      { fputs("\x1b[?25l",  stdout); fflush(stdout); }
void ml_term_show_cursor(void)      { fputs("\x1b[?25h",  stdout); fflush(stdout); }
void ml_term_home(void)             { fputs("\x1b[H",     stdout); fflush(stdout); }
void ml_term_clear_screen(void)     { fputs("\x1b[2J\x1b[H", stdout); fflush(stdout); }
