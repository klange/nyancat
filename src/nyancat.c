/*
 * Copyright (c) 2011 Kevin Lange.  All rights reserved.
 *
 * Developed by:            Kevin Lange
 *                          http://github.com/klange/nyancat
 *                          http://miku.acm.uiuc.edu
 * 
 * 40-column support by:    Peter Hazenberg
 *                          http://github.com/Peetz0r/nyancat
 *                          http://peter.haas-en-berg.nl
 *
 * This is a simple telnet server / standalone application which renders the
 * classic Nyan Cat (or "poptart cat") to your terminal.
 *
 * It makes use of various ANSI escape sequences to render color, or in the case
 * of a VT220, simply dumps text to the screen.
 *
 * For more information, please see:
 *
 *     http://miku.acm.uiuc.edu
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
#include <time.h>
#include <setjmp.h>
#include <sys/ioctl.h>

/*
 * telnet.h contains some #defines for the various
 * commands, escape characters, and modes for telnet.
 * (it surprises some people that telnet is, really,
 *  a protocol, and not just raw text transmission)
 */
#include "telnet.h"

/*
 * The animation frames are stored separately in
 * this header so they don't clutter the core source
 */
#include "animation.h"

/*
 * Color palette to use for final output
 * Specifically, this should be either control sequences
 * or raw characters (ie, for vt220 mode)
 */
char * colors[256] = {NULL};

/*
 * For most modes, we ouput spaces, but for some
 * we will use block characters (or even nothing)
 */
char * output = "  ";

/*
 * Are we currently in telnet mode?
 */
int telnet = 0;

/*
 * Environment to use for setjmp/longjmp
 * when breaking out of options handler
 */
jmp_buf environment;


/*
 * I refuse to include libm to keep this low
 * on external dependencies.
 *
 * Count the number of digits in a number for
 * use with string output.
 */
int digits(int val) {
	int d = 1, c;
	if (val >= 0) for (c = 10; c <= val; c *= 10) d++;
	else for (c = -10 ; c >= val; c *= 10) d++;
	return (c < 0) ? ++d : d;
}

/*
 * These values crop the animation, as we have a full 64x64 stored,
 * but we only want to display 40x24 (double width).
 */
#define MIN_ROW 20
#define MAX_ROW 43
#define MIN_COL 10
#define MAX_COL 50

/*
 * In the standalone mode, we want to handle an interrupt signal
 * (^C) so that we can restore the cursor and clear the terminal.
 */
void SIGINT_handler(int sig){
	printf("\033[?25h\033[0m\033[H\033[2J");
	exit(0);
}

/*
 * Handle the alarm which breaks us off of options
 * handling if we didn't receive a terminal
 */
void SIGALRM_handler(int sig) {
	alarm(0);
	longjmp(environment, 1);
	/* Unreachable */
}

/*
 * Telnet requires us to send a specific sequence
 * for a line break (\r\000\n), so let's make it happy.
 */
void newline(int n) {
	int i = 0;
	for (i = 0; i < n; ++i) {
		/* We will send `n` linefeeds to the client */
		if (telnet) {
			/* Send the telnet newline sequence */
			putc('\r', stdout);
			putc(0, stdout);
			putc('\n', stdout);
		} else {
			/* Send a regular line feed */
			putc('\n', stdout);
		}
	}
}

/*
 * These are the options we want to use as
 * a telnet server. These are set in set_options()
 */
unsigned char telnet_options[256] = { 0 };
unsigned char telnet_willack[256] = { 0 };

/*
 * These are the values we have set or
 * agreed to during our handshake.
 * These are set in send_command(...)
 */
unsigned char telnet_do_set[256]  = { 0 };
unsigned char telnet_will_set[256]= { 0 };

/*
 * Set the default options for the telnet server.
 */
void set_options() {
	/* We will not echo input */
	telnet_options[ECHO] = WONT;
	/* We will set graphics modes */
	telnet_options[SGA]  = WILL;
	/* We will not set new environments */
	telnet_options[NEW_ENVIRON] = WONT;

	/* The client should echo its own input */
	telnet_willack[ECHO]  = DO;
	/* The client can set a graphics mode */
	telnet_willack[SGA]   = DO;
	/* The client should not change, but it should tell us its window size */
	telnet_willack[NAWS]  = DO;
	/* The client should tell us its terminal type (very important) */
	telnet_willack[TTYPE] = DO;
	/* No linemode */
	telnet_willack[LINEMODE] = DONT;
	/* And the client can set a new environment */
	telnet_willack[NEW_ENVIRON] = DO;
}

/*
 * Send a command (cmd) to the telnet client
 * Also does special handling for DO/DONT/WILL/WONT
 */
void send_command(int cmd, int opt) {
	/* Send a command to the telnet client */
	if (cmd == DO || cmd == DONT) {
		/* DO commands say what the client should do. */
		if (((cmd == DO) && (telnet_do_set[opt] != DO)) ||
			((cmd == DONT) && (telnet_do_set[opt] != DONT))) {
			/* And we only send them if there is a disagreement */
			telnet_do_set[opt] = cmd;
			printf("%c%c%c", IAC, cmd, opt);
		}
	} else if (cmd == WILL || cmd == WONT) {
		/* Similarly, WILL commands say what the server will do. */
		if (((cmd == WILL) && (telnet_will_set[opt] != WILL)) ||
			((cmd == WONT) && (telnet_will_set[opt] != WONT))) {
			/* And we only send them during disagreements */
			telnet_will_set[opt] = cmd;
			printf("%c%c%c", IAC, cmd, opt);
		}
	} else {
		/* Other commands are sent raw */
		printf("%c%c", IAC, cmd);
	}
}


int main(int argc, char ** argv) {

	/* I have a bad habit of being very C99, so this may not be everything */
	/* The default terminal is ANSI */
	char term[1024] = {'a','n','s','i', 0};
	int terminal_width = 80;
	int k, ttype;
	uint32_t option = 0, done = 0, sb_mode = 0, do_echo = 0;
	/* Various pieces for the telnet communication */
	char sb[1024] = {0};
	char sb_len   = 0;

	if (argc > 1) {
		if (!strcmp(argv[1], "-t")) {
			/* Yes, I know, lame way to get arguments, whatever, we only want one of them. */
			telnet = 1;

			/* Set the default options */
			set_options();

			/* Let the client know what we're using */
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

			/* Set the alarm handler to execute the longjmp */
			signal(SIGALRM, SIGALRM_handler);

			/* Negotiate options */
			if (!setjmp(environment)) {
				/* We will stop handling options after one second */
				alarm(1);

				/* Let's do this */
				while (!feof(stdin) && done < 2) {
					/* Get either IAC (start command) or a regular character (break, unless in SB mode) */
					unsigned char i = getchar();
					unsigned char opt = 0;
					if (i == IAC) {
						/* If IAC, get the command */
						i = getchar();
						switch (i) {
							case SE:
								/* End of extended option mode */
								sb_mode = 0;
								if (sb[0] == TTYPE) {
									/* This was a response to the TTYPE command, meaning
									 * that this should be a terminal type */
									alarm(0);
									strcpy(term, &sb[2]);
									done++;
								}
								else if (sb[0] == NAWS) {
									/* This was a response to the NAWS command, meaning
									 * that this should be a window size */
									alarm(0);
									terminal_width = sb[2];
									done++;
								}
								break;
							case NOP:
								/* No Op */
								send_command(NOP, 0);
								fflush(stdout);
								break;
							case WILL:
							case WONT:
								/* Will / Won't Negotiation */
								opt = getchar();
								if (!telnet_willack[opt]) {
									/* We default to WONT */
									telnet_willack[opt] = WONT;
								}
								send_command(telnet_willack[opt], opt);
								fflush(stdout);
								if ((i == WILL) && (opt == TTYPE)) {
									/* WILL TTYPE? Great, let's do that now! */
									printf("%c%c%c%c%c%c", IAC, SB, TTYPE, SEND, IAC, SE);
									fflush(stdout);
								}
								break;
							case DO:
							case DONT:
								/* Do / Don't Negotation */
								opt = getchar();
								if (!telnet_options[opt]) {
									/* We default to DONT */
									telnet_options[opt] = DONT;
								}
								send_command(telnet_options[opt], opt);
								if (opt == ECHO) {
									/* We don't really need this, as we don't accept input, but,
									 * in case we do in the future, set our echo mode */
									do_echo = (i == DO);
								}
								fflush(stdout);
								break;
							case SB:
								/* Begin Extended Option Mode */
								sb_mode = 1;
								sb_len  = 0;
								memset(sb, 0, 1024);
								break;
							case IAC: 
								/* IAC IAC? That's probably not right. */
								done = 2;
								break;
							default:
								break;
						}
					} else if (sb_mode) {
						/* Extended Option Mode -> Accept character */
						if (sb_len < 1023) {
							/* Append this character to the SB string,
							 * but only if it doesn't put us over
							 * our limit; honestly, we shouldn't hit
							 * the limit, as we're only collecting characters
							 * for a terminal type or window size, but better safe than
							 * sorry (and vulernable).
							 */
							sb[sb_len] = i;
							sb_len++;
						}
					}
				}
			}
		}
	} else {
		/* We are running standalone, retrieve the
		 * terminal type from the environment. */
		char * nterm = getenv("TERM");
		if (nterm)
			strcpy(term, nterm);
		
		/* Also get the number of columns */
		struct winsize w;
		ioctl(0, TIOCGWINSZ, &w);
		terminal_width = w.ws_col;
	}

	/* Convert the entire terminal string to lower case */
	for (k = 0; k < strlen(term); ++k) {
		term[k] = tolower(term[k]);
	}
	
	/* We don't want terminals wider than 80 columns */
	if(terminal_width > 80) terminal_width = 80;

	/* Do our terminal detection */
	if (strstr(term, "xterm")) {
		ttype = 1; /* 256-color, spaces */
	} else if (strstr(term, "linux")) {
		ttype = 3; /* Spaces and blink attribute */
	} else if (strstr(term, "vtnt")) {
		ttype = 5; /* Extended ASCII fallback == Windows */
	} else if (strstr(term, "cygwin")) {
		ttype = 5; /* Extended ASCII fallback == Windows */
	} else if (strstr(term, "vt220")) {
		ttype = 6; /* No color support */
	} else if (strstr(term, "fallback")) {
		ttype = 4; /* Unicode fallback */
	} else if (strstr(term, "rxvt")) {
		ttype = 3; /* Accepts LINUX mode */
	} else if (strstr(term, "vt100") && terminal_width == 40) {
		ttype = 7; /* No color support, only 40 columns */
	} else {
		ttype = 2; /* Everything else */
	}

	int always_escape = 0; /* Used for text mode */
	/* Accept ^C -> restore cursor */
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
			colors[',']  = "\033[0;34;44m";  /* Blue background */
			colors['.']  = "\033[1;37;47m";  /* White stars */
			colors['\''] = "\033[0;30;40m";  /* Black border */
			colors['@']  = "\033[1;37;47m";  /* Tan poptart */
			colors['$']  = "\033[1;35;45m";  /* Pink poptart */
			colors['-']  = "\033[1;31;41m";  /* Red poptart */
			colors['>']  = "\033[1;31;41m";  /* Red rainbow */
			colors['&']  = "\033[0;33;43m";  /* Orange rainbow */
			colors['+']  = "\033[1;33;43m";  /* Yellow Rainbow */
			colors['#']  = "\033[1;32;42m";  /* Green rainbow */
			colors['=']  = "\033[1;34;44m";  /* Light blue rainbow */
			colors[';']  = "\033[0;34;44m";  /* Dark blue rainbow */
			colors['*']  = "\033[1;30;40m";  /* Gray cat face */
			colors['%']  = "\033[1;35;45m";  /* Pink cheeks */
			output = "██";
			break;
		case 5:
			colors[',']  = "\033[0;34;44m";  /* Blue background */
			colors['.']  = "\033[1;37;47m";  /* White stars */
			colors['\''] = "\033[0;30;40m";  /* Black border */
			colors['@']  = "\033[1;37;47m";  /* Tan poptart */
			colors['$']  = "\033[1;35;45m";  /* Pink poptart */
			colors['-']  = "\033[1;31;41m";  /* Red poptart */
			colors['>']  = "\033[1;31;41m";  /* Red rainbow */
			colors['&']  = "\033[0;33;43m";  /* Orange rainbow */
			colors['+']  = "\033[1;33;43m";  /* Yellow Rainbow */
			colors['#']  = "\033[1;32;42m";  /* Green rainbow */
			colors['=']  = "\033[1;34;44m";  /* Light blue rainbow */
			colors[';']  = "\033[0;34;44m";  /* Dark blue rainbow */
			colors['*']  = "\033[1;30;40m";  /* Gray cat face */
			colors['%']  = "\033[1;35;45m";  /* Pink cheeks */
			output = "\333\333";
			break;
		case 6:
			colors[',']  = "::";             /* Blue background */
			colors['.']  = "@@";             /* White stars */
			colors['\''] = "  ";             /* Black border */
			colors['@']  = "##";             /* Tan poptart */
			colors['$']  = "??";             /* Pink poptart */
			colors['-']  = "<>";             /* Red poptart */
			colors['>']  = "##";             /* Red rainbow */
			colors['&']  = "==";             /* Orange rainbow */
			colors['+']  = "--";             /* Yellow Rainbow */
			colors['#']  = "++";             /* Green rainbow */
			colors['=']  = "~~";             /* Light blue rainbow */
			colors[';']  = "$$";             /* Dark blue rainbow */
			colors['*']  = ";;";             /* Gray cat face */
			colors['%']  = "()";             /* Pink cheeks */
			always_escape = 1;
			break;
		case 7:
			colors[',']  = ".";             /* Blue background */
			colors['.']  = "@";             /* White stars */
			colors['\''] = " ";             /* Black border */
			colors['@']  = "#";             /* Tan poptart */
			colors['$']  = "?";             /* Pink poptart */
			colors['-']  = "O";             /* Red poptart */
			colors['>']  = "#";             /* Red rainbow */
			colors['&']  = "=";             /* Orange rainbow */
			colors['+']  = "-";             /* Yellow Rainbow */
			colors['#']  = "+";             /* Green rainbow */
			colors['=']  = "~";             /* Light blue rainbow */
			colors[';']  = "$";             /* Dark blue rainbow */
			colors['*']  = ";";             /* Gray cat face */
			colors['%']  = "o";             /* Pink cheeks */
			always_escape = 1;
			terminal_width = 40;
			break;
		default:
			break;
	}

	/* Attempt to set terminal title */
	printf("\033kNyanyanyanyanyanyanya...\033\134");
	printf("\033]1;Nyanyanyanyanyanyanya...\007");
	printf("\033]2;Nyanyanyanyanyanyanya...\007");

	/* Clear the screen */
	printf("\033[H\033[2J\033[?25l");

	/* Display the MOTD */
	int countdown_clock = 5;
	for (k = 0; k < countdown_clock; ++k) {
		newline(3);
		printf("                             \033[1mNyancat Telnet Server\033[0m");
		newline(2);
		printf("                   written and run by \033[1;32mKevin Lange\033[1;34m @kevinlange\033[0m");
		newline(2);
		printf("        If things don't look right, try:");
		newline(1);
		printf("                TERM=fallback telnet ...");
		newline(2);
		printf("        Or on Windows:");
		newline(1);
		printf("                telnet -t vtnt ...");
		newline(2);
		printf("        Problems? I am also a webserver:");
		newline(1);
		printf("                \033[1;34mhttp://miku.acm.uiuc.edu\033[0m");
		newline(2);
		printf("        This is a telnet server, remember your escape keys!");
		newline(1);
		printf("                \033[1;31m^]quit\033[0m to exit");
		newline(2);
		printf("        Starting in %d...                \n", countdown_clock-k);

		fflush(stdout);
		usleep(400000);
		printf("\033[H"); /* Reset cursor */
	}

	/* Clear the screen again */
	printf("\033[H\033[2J\033[?25l");

	/* Store the start time */
	time_t start, current;
	time(&start);

	int playing = 1; /* Animation should continue [left here for modifications] */
	size_t i = 0;    /* Current frame # */
	char last = 0;   /* Last color index rendered */
	size_t y, x;     /* x/y coordinates of what we're drawing */
	while (playing) {
		/* Render the frame */
		for (y = MIN_ROW; y < MAX_ROW; ++y) {
			for (x = MIN_COL; x < MAX_COL; ++x) {
				if (always_escape) {
					/* Text mode (or "Always Send Color Escapse") */
					printf("%s", colors[frames[i][y][x]]);
				} else {
					if (frames[i][y][x] != last && colors[frames[i][y][x]]) {
						/* Normal Mode, send escape (because the color changed) */
						last = frames[i][y][x];
						printf("%s%s", colors[frames[i][y][x]], output);
					} else {
						/* Same color, just send the output characters */
						printf("%s", output);
					}
				}
			}
			/* End of row, send newline */
			newline(1);
		}
		/* Get the current time for the "You have nyaned..." string */
		time(&current);
		double diff = difftime(current, start);
		/* Now count the length of the time difference so we can center */
		int nLen = digits((int)diff);
		int width = (terminal_width - 29 - nLen) / 2;
		/* Spit out some spaces so that we're actually centered */
		while (width > 0) {
			printf(" ");
			width--;
		}
		/* You have nyaned for [n] seconds!
		 * The \033[J ensures that the rest of the line has the dark blue
		 * background, and the \033[1;37m ensures that our text is bright white.
		 * The \033[0m prevents the Apple ][ from flipping everything, but
		 * makes the whole nyancat less bright on the vt220
		 */
		printf("\033[1;37mYou have nyaned for %0.0f seconds!\033[J\033[0m", diff);
		
		/* Reset the last color so that the escape sequences rewrite */
		last = 0;
		/* Update frame crount */
		++i;
		if (!frames[i]) {
			/* Loop animation */
			i = 0;
		}
		/* Reset cursor */
		printf("\033[H");
		/* Wait */
		usleep(90000);
	}
	return 0;
}
