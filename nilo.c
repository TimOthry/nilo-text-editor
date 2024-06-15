/*** includes ***/

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

/*** data ***/

struct termios orig_termios;

/*** terminal ***/

/*
* Error handler - prints te message
*/
void die(const char *s) {
    perror(s);
    exit(1);
}

/*
* When the program exits, raw mode is disabled and
* terminal is reset back to normal.
*/
void disableRawMode() {
     if (tcsetattr(STDERR_FILENO, TCSAFLUSH, &orig_termios) == -1)
        die("tcsetattr");
}

/* 
* This function makes it so that user input isnt 
* displayed in the terminal. Enables raw mode.
*/
void enableRawMode() {
    // Get original terminal attribute
     if (tcgetattr(STDIN_FILENO, &orig_termios) == - 1) die("tcgetattr");
    atexit(disableRawMode);

    // Runs a mask to set local/input flags on or off
    struct termios raw = orig_termios;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;
    
    // Sets the terminal to masked attribute
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}

int main() {
    enableRawMode();

    while (1) {
        char c = '\0';
        // Reads 1 byte (user input), stores to variable c
        if (read(STDIN_FILENO, &c, 1) == -1 && errno != EAGAIN) die("read");
        if (iscntrl(c)) { // If control char
            printf("%d\r\n", c);
        } else {
            printf("%d ('%c')\r\n", c, c);
        }
        if (c == 'q') break;
    }

    return 0;
}
