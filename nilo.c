/*** includes ***/

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

/*** defines ****/

#define CTRL_KEY(k) ((k) & 0x1f) // Hex number 31 mask

/*** data ***/

struct editorConfig {
    int screenrows;
    int screencols;
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

int getCursorPosition(int *rows, int *cols) {
    char buf[32];
    unsigned int i = 0;

    // Escape sequence <esc>[6n, cursor position report

    // If sequence reponse is not 4 bytes, returns -1
    if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;

    // Reads response into buffer
    while (i < sizeof(buf) - 1) {
        if (read(STDOUT_FILENO, &buf[i], 1) != 1) break;
        if (buf[i] == 'R') break;
        i++;
    }
    // Ensure string ends in '0' byte and skip first character
    buf[i] = '\0';
    // Ensures that response in the the expected format
    if (buf[0] != '\x1b' || buf[1] != '[') return -1;
    if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;

    return 0;
}

int getWindowSize(int *rows, int *cols) {
    struct winsize ws;
    // Returns -1 if obtaining window size fails
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;
        editorReadKey();
        return getCursorPosition(rows, cols);
    } else {
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
}

/*** output ***/

void editorDrawRows(void) {
    int y;
    for (y = 0; y < E.screenrows; y++) {
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

void initEditor(void) {
    if (getWindowSize(&E.screenrows, &E.screencols) == -1) die ("getWindowSize");
}

int main(void) {
    enableRawMode();
    initEditor();

    while (1) {
        editorRefreshScreen();
        editorProcessKeypress();
    }

    return 0;
}
