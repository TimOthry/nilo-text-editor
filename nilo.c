/*** includes ***/

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

/*** defines ****/

#define CTRL_KEY(k) ((k) & 0x1f) // Hex number 31 mask

/*** data ***/

struct editorConfig {
    struct termios orig_termios;
};

struct editorConfig E;

/*** terminal ***/

void die(const char *s) {
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
    
    // Error handler - prints message
    perror(s);
    exit(1);
}

void disableRawMode(void) {
     if (tcsetattr(STDERR_FILENO, TCSAFLUSH, &E.orig_termios) == -1)
        die("tcsetattr");
}

void enableRawMode(void) {
    // Get original terminal attribute
    if (tcgetattr(STDIN_FILENO, &E.orig_termios) == - 1) die("tcgetattr");
    atexit(disableRawMode);

    // Runs a mask to set local/input flags on or off
    struct termios raw = E.orig_termios;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;
    
    // Sets the terminal to masked attribute
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}

char editorReadKey(void) {
    // Waits for one keypress to return it
    int nread;
    char c;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        // If failure, exception handled
        if (nread == -1 && errno != EAGAIN) die("read");
    }
    return c;
}

/*** output ***/

void editorDrawRows(void) {
    int y;
    for (y = 0; y < 24; y++) {
        write(STDIN_FILENO, "~\r\n", 3);
    }
}

void editorRefreshScreen(void) {
    // Escape sequence <esc>[2J, clears terminal
    write(STDOUT_FILENO, "\x1b[2J", 4);
    // Puts cursor to top of terminal
    write(STDOUT_FILENO, "\x1b[H", 3);

    editorDrawRows();
    write(STDOUT_FILENO, "\x1b[H", 3);
}

/*** input ***/

void editorProcessKeypress(void) {
    char c = editorReadKey();
    // Exits when ctrl-q is pressed
    switch (c) {
        case CTRL_KEY('q'):
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            exit(0);
            break;
    }
}

/*** init ***/

int main(void) {
    enableRawMode();

    while (1) {
        editorRefreshScreen();
        editorProcessKeypress();
    }

    return 0;
}
