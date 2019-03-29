#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <comp421/yalnix.h>
#include <comp421/hardware.h>

#define MAX_LINE 1024

main(int argc, char *argv[])
{
	TracePrintf(1, "CONSOLE\n");
    int console;
    char buf[TERMINAL_MAX_LINE];

    if (argc < 2) {
	TtyPrintf(TTY_CONSOLE, "usage: console termno\n");
	Exit(1);
    }
    console = atoi(argv[1]);
    if (console != TTY_CONSOLE) {
	TtyPrintf(TTY_CONSOLE, "console must run on terminal %d\n",
	    TTY_CONSOLE);
	Exit(1);
    }

    TtyPrintf(console, "YALNIX READY\n");
    TtyPrintf(console, "Type 'halt' to halt Yalnix.\n");

    while (1) {
		int n;
		char *word;
		char *end_word;

		TtyPrintf(console, ">>> ");
		TracePrintf(1, "Console reading\n");
		n = TtyRead(console, buf, TERMINAL_MAX_LINE);
		TracePrintf(1, "Console done reading\n");
		if (n == 0 || n >= TERMINAL_MAX_LINE) continue;
		buf[n] = '\0';

		for (word = buf; isspace(*word); word++);
		for (end_word = word; *end_word && !isspace(*end_word); end_word++);
		*end_word = '\0';

		if (!strcmp(word, "halt")) {
		    TtyPrintf(console, "Halting....\n");
		    Exit(0);
		}
		if (*word != '\0')
		    TtyPrintf(console, "'%s': Command not recognized.\n", word);
    }
}
