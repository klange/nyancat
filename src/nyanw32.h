#include <windows.h>
#include <mmsystem.h>
#define SIGALRM 14

#ifndef FOREGROUND_MASK
# define FOREGROUND_MASK (FOREGROUND_RED|FOREGROUND_BLUE|FOREGROUND_GREEN|FOREGROUND_INTENSITY)
#endif
#ifndef BACKGROUND_MASK
# define BACKGROUND_MASK (BACKGROUND_RED|BACKGROUND_BLUE|BACKGROUND_GREEN|BACKGROUND_INTENSITY)
#endif

typedef void (*sighandler_t)(int);
sighandler_t f_sigalarm = NULL;

void _signal(int sig, sighandler_t t) {
	if (sig == SIGALRM) {
		f_sigalarm = t;
	} else {
		signal(sig, t);
	}
}

void alarm(int sec) {
	timeSetEvent(sec * 1000, 100, (LPTIMECALLBACK) f_sigalarm, 0, TIME_ONESHOT);
}

int _printf(const char* format, ...) {
	va_list args;
	int r;
	char buf[BUFSIZ], *ptr = buf, *stop;
	static WORD attr_old;
	static HANDLE stdo = INVALID_HANDLE_VALUE;
	WORD attr;
	DWORD written, csize;
	CONSOLE_CURSOR_INFO cci;
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	COORD coord;

	if (stdo == INVALID_HANDLE_VALUE) {
		stdo = GetStdHandle(STD_OUTPUT_HANDLE);
		GetConsoleScreenBufferInfo(stdo, &csbi);
		attr = attr_old = csbi.wAttributes;
	}

	va_start(args, format);
	r = vsnprintf(buf, sizeof(buf), format, args);
	va_end(args);

	while (*ptr) {
		if (*ptr == '\033' && !telnet) {
			unsigned char c;
			int i, n = 0, m, v[6], w, h;
			for (i = 0; i < 6; i++) v[i] = -1;
			ptr++;
retry:
			if ((c = *ptr++) == 0) break;
			if (isdigit(c)) {
				if (v[n] == -1) v[n] = c - '0';
				else v[n] = v[n] * 10 + c - '0';
				goto retry;
			}
			if (c == '[') {
				goto retry;
			}
			if (c == ';') {
				if (++n == 6) break;
				goto retry;
			}
			if (c == '>' || c == '?') {
				m = c;
				goto retry;
			}

			switch (c) {
			case 'h':
				if (m == '?') {
					for (i = 0; i <= n; i++) {
						switch (v[i]) {
						case 3:
							GetConsoleScreenBufferInfo(stdo, &csbi);
							w = csbi.dwSize.X;
							h = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
							csize = w * (h + 1);
							coord.X = 0;
							coord.Y = csbi.srWindow.Top;
							FillConsoleOutputCharacter(stdo, ' ', csize, coord, &written);
							FillConsoleOutputAttribute(stdo, csbi.wAttributes, csize, coord, &written);
							SetConsoleCursorPosition(stdo, csbi.dwCursorPosition);
							csbi.dwSize.X = 132;
							SetConsoleScreenBufferSize(stdo, csbi.dwSize);
							csbi.srWindow.Right = csbi.srWindow.Left + 131;
							SetConsoleWindowInfo(stdo, TRUE, &csbi.srWindow);
							break;
						case 5:
							attr =
								((attr & FOREGROUND_MASK) << 4) |
								((attr & BACKGROUND_MASK) >> 4);
							SetConsoleTextAttribute(stdo, attr);
							break;
						case 9:
							break;
						case 25:
							GetConsoleCursorInfo(stdo, &cci);
							cci.bVisible = TRUE;
							SetConsoleCursorInfo(stdo, &cci);
							break;
						case 47:
							coord.X = 0;
							coord.Y = 0;
							SetConsoleCursorPosition(stdo, coord);
							break;
						default:
							break;
						}
					}
				} else if (m == '>' && v[0] == 5) {
					GetConsoleCursorInfo(stdo, &cci);
					cci.bVisible = FALSE;
					SetConsoleCursorInfo(stdo, &cci);
				}
				break;
			case 'l':
				if (m == '?') {
					for (i = 0; i <= n; i++) {
						switch (v[i]) {
						case 3:
							GetConsoleScreenBufferInfo(stdo, &csbi);
							w = csbi.dwSize.X;
							h = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
							csize = w * (h + 1);
							coord.X = 0;
							coord.Y = csbi.srWindow.Top;
							FillConsoleOutputCharacter(stdo, ' ', csize, coord, &written);
							FillConsoleOutputAttribute(stdo, csbi.wAttributes, csize, coord, &written);
							SetConsoleCursorPosition(stdo, csbi.dwCursorPosition);
							csbi.srWindow.Right = csbi.srWindow.Left + 79;
							SetConsoleWindowInfo(stdo, TRUE, &csbi.srWindow);
							csbi.dwSize.X = 80;
							SetConsoleScreenBufferSize(stdo, csbi.dwSize);
							break;
						case 5:
							attr =
								((attr & FOREGROUND_MASK) << 4) |
								((attr & BACKGROUND_MASK) >> 4);
							SetConsoleTextAttribute(stdo, attr);
							break;
						case 25:
							GetConsoleCursorInfo(stdo, &cci);
							cci.bVisible = FALSE;
							SetConsoleCursorInfo(stdo, &cci);
							break;
						default:
							break;
						}
					}
				}
				else if (m == '>' && v[0] == 5) {
					GetConsoleCursorInfo(stdo, &cci);
					cci.bVisible = TRUE;
					SetConsoleCursorInfo(stdo, &cci);
				}
				break;
			case 'm':
				attr = attr_old;
				for (i = 0; i <= n; i++) {
					if (v[i] == -1 || v[i] == 0)
						attr = attr_old;
					else if (v[i] == 1)
						attr |= FOREGROUND_INTENSITY;
					else if (v[i] == 4)
						attr |= FOREGROUND_INTENSITY;
					else if (v[i] == 5)
						attr |= FOREGROUND_INTENSITY;
					else if (v[i] == 7)
						attr =
							((attr & FOREGROUND_MASK) << 4) |
							((attr & BACKGROUND_MASK) >> 4);
					else if (v[i] == 10)
						; // symbol on
					else if (v[i] == 11)
						; // symbol off
					else if (v[i] == 22)
						attr &= ~FOREGROUND_INTENSITY;
					else if (v[i] == 24)
						attr &= ~FOREGROUND_INTENSITY;
					else if (v[i] == 25)
						attr &= ~FOREGROUND_INTENSITY;
					else if (v[i] == 27)
						attr =
							((attr & FOREGROUND_MASK) << 4) |
							((attr & BACKGROUND_MASK) >> 4);
					else if (v[i] >= 30 && v[i] <= 37) {
						attr = (attr & BACKGROUND_MASK)
							| FOREGROUND_INTENSITY;
						if ((v[i] - 30) & 1)
							attr |= FOREGROUND_RED;
						if ((v[i] - 30) & 2)
							attr |= FOREGROUND_GREEN;
						if ((v[i] - 30) & 4)
							attr |= FOREGROUND_BLUE;
					}
					//else if (v[i] == 39)
					//attr = (~attr & BACKGROUND_MASK);
					else if (v[i] >= 40 && v[i] <= 47) {
						attr = (attr & FOREGROUND_MASK)
							| BACKGROUND_INTENSITY;
						if ((v[i] - 40) & 1)
							attr |= BACKGROUND_RED;
						if ((v[i] - 40) & 2)
							attr |= BACKGROUND_GREEN;
						if ((v[i] - 40) & 4)
							attr |= BACKGROUND_BLUE;
					}
					//else if (v[i] == 49)
					//attr = (~attr & FOREGROUND_MASK);
					else if (v[i] == 100)
						attr = attr_old;
				}
				SetConsoleTextAttribute(stdo, attr);
				break;
			case 'K':
				GetConsoleScreenBufferInfo(stdo, &csbi);
				coord = csbi.dwCursorPosition;
				switch (v[0]) {
				default:
				case 0:
					csize = csbi.dwSize.X - coord.X;
					break;
				case 1:
					csize = coord.X;
					coord.X = 0;
					break;
				case 2:
					csize = csbi.dwSize.X;
					coord.X = 0;
					break;
				}
				FillConsoleOutputCharacter(stdo, ' ', csize, coord, &written);
				FillConsoleOutputAttribute(stdo, csbi.wAttributes, csize, coord, &written);
				SetConsoleCursorPosition(stdo, csbi.dwCursorPosition);
				break;
			case 'J':
				GetConsoleScreenBufferInfo(stdo, &csbi);
				w = csbi.dwSize.X;
				h = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
				coord = csbi.dwCursorPosition;
				switch (v[0]) {
				default:
				case 0:
					csize = w * (h - coord.Y) - coord.X;
					coord.X = 0;
					break;
				case 1:
					csize = w * coord.Y + coord.X;
					coord.X = 0;
					coord.Y = csbi.srWindow.Top;
					break;
				case 2:
					csize = w * (h + 1);
					coord.X = 0;
					coord.Y = csbi.srWindow.Top;
					break;
				}
				FillConsoleOutputCharacter(stdo, ' ', csize, coord, &written);
				FillConsoleOutputAttribute(stdo, csbi.wAttributes, csize, coord, &written);
				SetConsoleCursorPosition(stdo, csbi.dwCursorPosition);
				break;
			case 'H':
				GetConsoleScreenBufferInfo(stdo, &csbi);
				coord = csbi.dwCursorPosition;
				if (v[0] != -1) {
					if (v[1] != -1) {
						coord.Y = csbi.srWindow.Top + v[0] - 1;
						coord.X = v[1] - 1;
					} else
						coord.X = v[0] - 1;
				} else {
					coord.X = 0;
					coord.Y = csbi.srWindow.Top;
				}
				if (coord.X < csbi.srWindow.Left)
					coord.X = csbi.srWindow.Left;
				else if (coord.X > csbi.srWindow.Right)
					coord.X = csbi.srWindow.Right;
				if (coord.Y < csbi.srWindow.Top)
					coord.Y = csbi.srWindow.Top;
				else if (coord.Y > csbi.srWindow.Bottom)
					coord.Y = csbi.srWindow.Bottom;
				SetConsoleCursorPosition(stdo, coord);
				break;
			default:
				break;
			}
		} else {
			putchar(*ptr);
			ptr++;
		}
	}
}

#define signal(x, y) _signal(x, y)
#define printf(...) _printf(__VA_ARGS__)

/* windows console wrap line if write in columns 79. */
#undef MAX_COL
#define MAX_COL 49
