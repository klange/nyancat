/*
 * Copyright (c) 2011 Kevin Lange.  All rights reserved.
 *
 * Developed by: Kevin Lange
 *               http://github.com/klange/nyancat
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal with the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimers.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimers in the
 *      documentation and/or other materials provided with the distribution.
 *   3. Neither the names of the Association for Computing Machinery, Kevin
 *      Lange, nor the names of its contributors may be used to endorse
 *      or promote products derived from this Software without specific prior
 *      written permission.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * WITH THE SOFTWARE.
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "telnet.h"

#include "animation.h"
char * colors[256] = {NULL};
char * output = "  ";

#define MIN_ROW 20
#define MAX_ROW 44

#define MIN_COL 10
#define MAX_COL 50

void SIGINT_handler(int sig){
	printf("\033[?25h");
	exit(0);
}

unsigned char telnet_options[256] = { 0 };
unsigned char telnet_willack[256] = { 0 };

unsigned char telnet_do_set[256]  = { 0 };
unsigned char telnet_will_set[256]= { 0 };

void set_options() {
	telnet_options[ECHO] = WONT;
	telnet_options[SGA]  = WILL;
	telnet_options[NEW_ENVIRON] = WONT;

	telnet_willack[ECHO]  = DO;
	telnet_willack[SGA]   = DO;
	telnet_willack[NAWS]  = DONT;
	telnet_willack[TTYPE] = DO;
	telnet_willack[LINEMODE] = DONT;
	telnet_willack[NEW_ENVIRON] = DO;
}

void send_command(int cmd, int opt) {
	if (cmd == DO || cmd == DONT) {
		if (((cmd == DO) && (telnet_do_set[opt] != DO)) ||
			((cmd == DONT) && (telnet_do_set[opt] != DONT))) {
			telnet_do_set[opt] = cmd;
			printf("%c%c%c", IAC, cmd, opt);
		}
	} else if (cmd == WILL || cmd == WONT) {
		if (((cmd == WILL) && (telnet_will_set[opt] != WILL)) ||
			((cmd == WONT) && (telnet_will_set[opt] != WONT))) {
			telnet_will_set[opt] = cmd;
			printf("%c%c%c", IAC, cmd, opt);
		}
	} else {
		printf("%c%c", IAC, cmd);
	}
}


int main(int argc, char ** argv) {

	int k, ttype;
	uint32_t option = 0, done = 0, sb_mode = 0, do_echo = 0;
	char term[1024] = {'a','n','s','i'};
	char sb[1024] = {0};
	char sb_len   = 0;

	if (argc > 1) {
		if (!strcmp(argv[1], "-t")) {

			set_options();

			for (option = 0; option < 256; option++) {
				if (telnet_options[option]) {
					send_command(telnet_options[option], option);
					fflush(stdout);
				}
			}
			for (option = 0; option < 256; option++) {
				if (telnet_willack[option]) {
					send_command(telnet_willack[option], option);
					fflush(stdout);
				}
			}

			while (!feof(stdin) && !done) {
				unsigned char i = getchar();
				unsigned char opt = 0;
				if (i == 255) {
					i = getchar();
					switch (i) {
						case SE:
							sb_mode = 0;
							if (sb[0] == TTYPE) {
								strcpy(term, &sb[2]);
								goto ready;
							}
							break;
						case NOP:
							send_command(NOP, 0);
							fflush(stdout);
							break;
						case WILL:
						case WONT:
							opt = getchar();
							if (!telnet_willack[opt]) {
								telnet_willack[opt] = WONT;
							}
							send_command(telnet_willack[opt], opt);
							fflush(stdout);
							if ((i == WILL) && (opt == TTYPE)) {
								printf("%c%c%c%c%c%c", IAC, SB, TTYPE, SEND, IAC, SE);
								fflush(stdout);
							}
							break;
						case DO:
						case DONT:
							opt = getchar();
							if (!telnet_options[opt]) {
								telnet_options[opt] = DONT;
							}
							send_command(telnet_options[opt], opt);
							if (opt == ECHO) {
								do_echo = (i == DO);
							}
							fflush(stdout);
							break;
						case SB:
							sb_mode = 1;
							sb_len  = 0;
							memset(sb, 0, 1024);
							break;
						case IAC: 
							/* Connection Closed */
							done = 1;
							break;
						default:
							break;
					}
				} else if (sb_mode) {
					sb[sb_len] = i;
					sb_len++;
				}
			}
		}
	} else {
		char * nterm = getenv("TERM");
		strcpy(term, nterm);
	}

ready:

	for (k = 0; k < strlen(term); ++k) {
		term[k] = tolower(term[k]);
	}

	if (strstr(term, "xterm")) {
		ttype = 1;
	} else if (strstr(term, "linux")) {
		ttype = 3;
	} else if (strstr(term, "vtnt")) {
		ttype = 5;
	} else if (strstr(term, "cygwin")) {
		ttype = 5;
	} else if (strstr(term, "vt220")) {
		ttype = 6;
	} else if (strstr(term, "fallback")) {
		ttype = 4;
	} else if (strstr(term, "rxvt")) {
		ttype = 3;
	} else {
		ttype = 2;
	}

	int always_escape = 0;
	signal(SIGINT, SIGINT_handler);
	switch (ttype) {
		case 1:
			colors[',']  = "\033[48;5;17m";  /* Blue background */
			colors['.']  = "\033[48;5;15m";  /* White stars */
			colors['\''] = "\033[48;5;0m";   /* Black border */
			colors['@']  = "\033[48;5;230m"; /* Tan poptart */
			colors['$']  = "\033[48;5;175m"; /* Pink poptart */
			colors['-']  = "\033[48;5;162m"; /* Red poptart */
			colors['>']  = "\033[48;5;9m";   /* Red rainbow */
			colors['&']  = "\033[48;5;202m"; /* Orange rainbow */
			colors['+']  = "\033[48;5;11m";  /* Yellow Rainbow */
			colors['#']  = "\033[48;5;10m";  /* Green rainbow */
			colors['=']  = "\033[48;5;33m";  /* Light blue rainbow */
			colors[';']  = "\033[48;5;19m";  /* Dark blue rainbow */
			colors['*']  = "\033[48;5;8m";   /* Gray cat face */
			colors['%']  = "\033[48;5;175m"; /* Pink cheeks */
			break;
		case 2:
			colors[',']  = "\033[104m";      /* Blue background */
			colors['.']  = "\033[107m";      /* White stars */
			colors['\''] = "\033[40m";       /* Black border */
			colors['@']  = "\033[47m";       /* Tan poptart */
			colors['$']  = "\033[105m";      /* Pink poptart */
			colors['-']  = "\033[101m";      /* Red poptart */
			colors['>']  = "\033[101m";      /* Red rainbow */
			colors['&']  = "\033[43m";       /* Orange rainbow */
			colors['+']  = "\033[103m";      /* Yellow Rainbow */
			colors['#']  = "\033[102m";      /* Green rainbow */
			colors['=']  = "\033[104m";      /* Light blue rainbow */
			colors[';']  = "\033[44m";       /* Dark blue rainbow */
			colors['*']  = "\033[100m";      /* Gray cat face */
			colors['%']  = "\033[105m";      /* Pink cheeks */
			break;
		case 3:
			colors[',']  = "\033[25;44m";    /* Blue background */
			colors['.']  = "\033[5;47m";     /* White stars */
			colors['\''] = "\033[25;40m";    /* Black border */
			colors['@']  = "\033[5;47m";     /* Tan poptart */
			colors['$']  = "\033[5;45m";     /* Pink poptart */
			colors['-']  = "\033[5;41m";     /* Red poptart */
			colors['>']  = "\033[5;41m";     /* Red rainbow */
			colors['&']  = "\033[25;43m";    /* Orange rainbow */
			colors['+']  = "\033[5;43m";     /* Yellow Rainbow */
			colors['#']  = "\033[5;42m";     /* Green rainbow */
			colors['=']  = "\033[25;44m";    /* Light blue rainbow */
			colors[';']  = "\033[5;44m";     /* Dark blue rainbow */
			colors['*']  = "\033[5;40m";     /* Gray cat face */
			colors['%']  = "\033[5;45m";     /* Pink cheeks */
			break;
		case 4:
			colors[',']  = "\033[0;34;44m";       /* Blue background */
			colors['.']  = "\033[1;37;47m";       /* White stars */
			colors['\''] = "\033[0;30;40m";       /* Black border */
			colors['@']  = "\033[1;37;47m";       /* Tan poptart */
			colors['$']  = "\033[1;35;45m";       /* Pink poptart */
			colors['-']  = "\033[1;31;41m";       /* Red poptart */
			colors['>']  = "\033[1;31;41m";       /* Red rainbow */
			colors['&']  = "\033[0;33;43m";       /* Orange rainbow */
			colors['+']  = "\033[1;33;43m";       /* Yellow Rainbow */
			colors['#']  = "\033[1;32;42m";       /* Green rainbow */
			colors['=']  = "\033[1;34;44m";       /* Light blue rainbow */
			colors[';']  = "\033[0;34;44m";       /* Dark blue rainbow */
			colors['*']  = "\033[1;30;40m";       /* Gray cat face */
			colors['%']  = "\033[1;35;45m";       /* Pink cheeks */
			output = "██";
			break;
		case 5:
			colors[',']  = "\033[0;34;44m";       /* Blue background */
			colors['.']  = "\033[1;37;47m";       /* White stars */
			colors['\''] = "\033[0;30;40m";       /* Black border */
			colors['@']  = "\033[1;37;47m";       /* Tan poptart */
			colors['$']  = "\033[1;35;45m";       /* Pink poptart */
			colors['-']  = "\033[1;31;41m";       /* Red poptart */
			colors['>']  = "\033[1;31;41m";       /* Red rainbow */
			colors['&']  = "\033[0;33;43m";       /* Orange rainbow */
			colors['+']  = "\033[1;33;43m";       /* Yellow Rainbow */
			colors['#']  = "\033[1;32;42m";       /* Green rainbow */
			colors['=']  = "\033[1;34;44m";       /* Light blue rainbow */
			colors[';']  = "\033[0;34;44m";       /* Dark blue rainbow */
			colors['*']  = "\033[1;30;40m";       /* Gray cat face */
			colors['%']  = "\033[1;35;45m";       /* Pink cheeks */
			output = "\333\333";
			break;
		case 6:
			colors[',']  = "  ";       /* Blue background */
			colors['.']  = "**";       /* White stars */
			colors['\''] = "##";       /* Black border */
			colors['@']  = "##";       /* Tan poptart */
			colors['$']  = "??";       /* Pink poptart */
			colors['-']  = "<>";       /* Red poptart */
			colors['>']  = "##";       /* Red rainbow */
			colors['&']  = "==";       /* Orange rainbow */
			colors['+']  = "--";       /* Yellow Rainbow */
			colors['#']  = "++";       /* Green rainbow */
			colors['=']  = "~~";       /* Light blue rainbow */
			colors[';']  = "$$";       /* Dark blue rainbow */
			colors['*']  = "  ";       /* Gray cat face */
			colors['%']  = "()";       /* Pink cheeks */
			always_escape = 1;
			break;
		default:
			break;
	}

	printf("\033[H\033[2J\033[?25l");


	int countdown_clock = 5;
	for (k = 0; k < countdown_clock; ++k) {
		printf("\n\n\n");
		printf("   Nyancat Telnet Server\n");
		printf("\n");
		printf("   written and run by \033[1;32mKevin Lange\033[1;34m @kevinlange\033[0m\n");
		printf("\n");
		printf("   If things don't look right, try:\n");
		printf("      TERM=fallback telnet ...\n");
		printf("   Or on Windows:\n");
		printf("      telnet -t vtnt ...\n");
		printf("\n");
		printf("   Problems? I am also a webserver:\n");
		printf("      \033[1;34mhttp://miku.acm.uiuc.edu\033[0m\n");
		printf("\n");
		printf("   Starting in %d...      \n", countdown_clock-k);

		fflush(stdout);
		usleep(400000);
		printf("\033[H");
	}

	printf("\033[H\033[2J\033[?25l");

	int playing = 1;
	size_t i = 0;
	char last = 0;
	size_t y, x;
	while (playing) {
		for (y = MIN_ROW; y < MAX_ROW; ++y) {
			for (x = MIN_COL; x < MAX_COL; ++x) {
				if (always_escape) {
					printf("%s", colors[frames[i][y][x]]);
				} else {
					if (frames[i][y][x] != last && colors[frames[i][y][x]]) {
						last = frames[i][y][x];
						printf("%s%s", colors[frames[i][y][x]], output);
					} else {
						printf("%s", output);
					}
				}
			}
			if (y != MAX_ROW - 1)
				printf("\n");
		}
		++i;
		if (!frames[i]) {
			i = 0;
		}
		printf("\033[H");
		usleep(90000);
	}
	return 0;
}
