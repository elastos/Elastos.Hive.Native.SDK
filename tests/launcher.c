/*
 * Copyright (c) 2019 Elastos Foundation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <stdarg.h>
#include <fcntl.h>
#include <assert.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <crystal.h>
#if defined(_WIN32) || defined(_WIN64)

// Undefine Windows defined MOUSE_MOVED for PDCurses
#undef MOUSE_MOVED
#endif
#include <curses.h>

subprocess_t tests;

WINDOW *tests_out_border, *tests_out_win;
WINDOW *tests_log_border, *tests_log_win;

pthread_mutex_t screen_lock = PTHREAD_MUTEX_INITIALIZER;

#define TESTS_OUT       1
#define TESTS_LOG       2

static pthread_t dumper[2] = { 0 };

static void get_layout(int win, int *w, int *h, int *x, int *y)
{
    if (win == TESTS_OUT) {
        *x = 0;
        *y = 0;

        *w = (COLS - 1) / 2;
        *h = LINES;
    } else if (win == TESTS_LOG) {
        *x = COLS - (COLS / 2);
        *y = 0;

        *w = (COLS - 1) / 2;
        *h = LINES;
    }
}

#ifdef HAVE_SIGACTION
static void handle_winch(int sig)
{
    int w, h, x, y;

    endwin();

    if (LINES < 24 || COLS < 80) {
        printf("Terminal size too small!\n");
        exit(-1);
    }

    refresh();
    clear();

    wresize(stdscr, LINES, COLS);

    get_layout(TESTS_OUT, &w, &h, &x, &y);

    wresize(tests_out_border, h, w);
    mvwin(tests_out_border, y, x);
    box(tests_out_border, 0, 0);
    mvwprintw(tests_out_border, 0, 4, "Test Output");

    wresize(tests_out_win, h-2, w-2);
    mvwin(tests_out_win, y+1, x+1);

    get_layout(TESTS_LOG, &w, &h, &x, &y);

    wresize(tests_log_border, h, w);
    mvwin(tests_log_border, y, x);
    box(tests_log_border, 0, 0);
    mvwprintw(tests_log_border, 0, 4, "Test Log");

    wresize(tests_log_win, h-2, w-2);
    mvwin(tests_log_win, y+1,  x+1);

    clear();
    refresh();

    wrefresh(tests_out_border);
    wrefresh(tests_out_win);

    wrefresh(tests_log_border);
    wrefresh(tests_log_win);
}
#endif

static void init_screen(void)
{
    int w, h, x, y;

    initscr();

    if (LINES < 24 || COLS < 80) {
        printf("Terminal size too small!\n");
        endwin();
        exit(-1);
    }

    noecho();
    nodelay(stdscr, TRUE);
    refresh();

    get_layout(TESTS_OUT, &w, &h, &x, &y);

    tests_out_border = newwin(h, w, y, x);
    box(tests_out_border, 0, 0);
    mvwprintw(tests_out_border, 0, 4, "Test Result");
    wrefresh(tests_out_border);

    tests_out_win = newwin(h-2, w-2, y+1, x+1);
    scrollok(tests_out_win, TRUE);
    wrefresh(tests_out_win);

    get_layout(TESTS_LOG, &w, &h, &x, &y);

    tests_log_border = newwin(h, w, y, x);
    box(tests_log_border, 0, 0);
    mvwprintw(tests_log_border, 0, 4, "Test Log");
    wrefresh(tests_log_border);

    tests_log_win = newwin(h-2, w-2, y+1,  x+1);
    scrollok(tests_log_win, TRUE);
    wrefresh(tests_log_win);

#ifdef HAVE_SIGACTION
    struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = handle_winch;
    sigaction(SIGWINCH, &sa, NULL);
#endif
}

static void cleanup_screen(void)
{
    endwin();

    delwin(tests_out_border);
    delwin(tests_out_win);

    delwin(tests_log_border);
    delwin(tests_log_win);
}

struct dump_args {
    FILE *fp;
    WINDOW *win;
};

#if defined(_WIN32) || defined(_WIN64)
#define EOP         255
#else
#define EOP         EOF
#endif

static void *dump_routine(void *ptr)
{
    int ch;
    struct dump_args *args = (struct dump_args *)ptr;

    while ((ch = fgetc(args->fp)) != EOP) {
        pthread_mutex_lock(&screen_lock);
        waddch(args->win, (chtype)ch);
        wrefresh(args->win);
        pthread_mutex_unlock(&screen_lock);
    }

    free(args);
    return NULL;
}

static int dump(pthread_t *th, FILE *fp, WINDOW *win)
{
    int rc;
    struct dump_args *args;

    args = (struct dump_args *)malloc(sizeof(struct dump_args));
    if (!args)
        return -1;

    args->fp = fp;
    args->win = win;

    rc = pthread_create(th, NULL, dump_routine, args);
    if (rc != 0) {
        free(args);
        return -1;
    }

    return 0;
}

static void launcher_cleanup(void)
{
    int i;

    for (i = 0; i < sizeof(dumper) / sizeof(dumper[0]); i++)
        pthread_join(dumper[i], NULL);

    if (tests)
        spclose(tests);

    wprintw(tests_out_win, "\n\nPress q to quit...");
    wrefresh(tests_out_win);
    while (getchar() != 'q');
    cleanup_screen();
}

static void signal_handler(int signum)
{
    printf("Got signal: %d, force exit.\n", signum);

    launcher_cleanup();

    exit(-1);
}

int launcher_main(int argc, char *argv[])
{
    char cmdbase[1024] = { 0 };
    char cmdline[1024];

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    init_screen();

    int i;
    for (i = 0; i < argc; i++) {
        strcat(cmdbase, argv[i]);
        strcat(cmdbase, " ");
    }

    sprintf(cmdline, "%s --cases", cmdbase);
    tests = spopen(cmdline, "r");
    if (tests == NULL) {
        launcher_cleanup();
        return -1;
    }

    dump(&dumper[0], spstdout(tests), tests_out_win);
    dump(&dumper[1], spstderr(tests), tests_log_win);

    launcher_cleanup();
    return 0;
}
