/* -*- c-file-style: "bsd"; -*- */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

const size_t LINE_SIZE = 11;

enum codes {
	CTRL_C = 3
};

static int refresh_line(const char *prompt, const char *line)
{
	char fresh[128] = { 0xD }; /* carriage return */
	size_t offset = 1;
	char erase[3] = { 0x1B, '[', 'K' }; /* Erase in Line */

	memcpy(fresh + offset, prompt, strlen(prompt));

	offset += strlen(prompt);

	memcpy(fresh + offset, erase, sizeof(erase));

	offset += sizeof(erase);

	memcpy(fresh + offset, line, strlen(line));

	offset += strlen(line);

	return write(STDOUT_FILENO, &fresh, offset);
}

int main()
{
	char line[LINE_SIZE];
	struct termios cooked, raw;
	const char *prompt = "chunky-monkey> ";
	const char *farewell = "Goodbye!";
	size_t index = 0, len = 0;
	char cursor_next_line[] = { 0x1B, '[', '8', '0', '0', 'E' };

	/* Are we being run from a terminal? */
	if (!isatty(STDIN_FILENO))
		exit(EXIT_FAILURE);

	/* Current terminal settings will provide the baseline for raw mode's */
	if (tcgetattr(STDIN_FILENO, &cooked) == -1)
		exit(EXIT_FAILURE);

	/* Yeah, structure assignment is possible in C! */
	raw = cooked;

	/*
	 * Advanced Programming in the UNIX Environment, 3rd Edition is a
	 * must-read to understand the code below.
	 */

	/* input modes: no break, no CR to NL, no parity check, no strip char,
	 * no start/stop output control. */
	raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
	/* output modes - disable post processing */
	raw.c_oflag &= ~(OPOST);
	/* control modes - set 8 bit chars */
	raw.c_cflag |= (CS8);
	/* local modes - echoing off, canonical off, no extended functions,
	 * no signal chars (^Z,^C) */
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
	/* control chars - set return condition: min number of bytes and timer.
	 * We want read to return every single byte, without timeout. */
	raw.c_cc[VMIN] = 1; raw.c_cc[VTIME] = 0; /* 1 byte, no timer */

	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
		goto restore_cooked_onfailure;
	
	/* Print out prompt */
	if (write(STDOUT_FILENO, prompt, strlen(prompt)) == -1)
		goto restore_cooked_onfailure;

	while (1) {
		int n;
		char ascii_char, sequence[3];

		n = read(STDIN_FILENO, &ascii_char, 1);

		if (n <= 0)
			goto restore_cooked_onfailure;

		switch (ascii_char) {

		case CTRL_C:
			goto restore_cooked_onsuccess;

		case '?':
			
			write(STDOUT_FILENO, cursor_next_line, sizeof(cursor_next_line));
			write(STDOUT_FILENO, farewell, strlen(farewell));
			goto restore_cooked_onsuccess;

		case 8: /* backspace */
		case 127:
			line[--index] = '\0';
			--len;
			refresh_line(prompt, line);
			break;

		default:
			if (index < LINE_SIZE - 1) {
				
				line[index++] = ascii_char;
				++len;
				refresh_line(prompt, line);
			}
			else {
				line[index] = '\0';
				/* write(STDOUT_FILENO, line, LINE_SIZE - 1); */
				goto restore_cooked_onsuccess;
			} 
			break; 
		}
	}

restore_cooked_onsuccess:
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &cooked);
	/* printf("%s\n", line); */
	printf("\n");
	exit(EXIT_SUCCESS);

restore_cooked_onfailure:
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &cooked) == -1) {
		printf("\n");
		exit(EXIT_FAILURE);
	}

	return 0;
}
