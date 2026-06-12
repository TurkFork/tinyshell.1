#ifndef COLOR_H
#define COLOR_H

#include <unistd.h>
#include <stdlib.h>

#define C_RESET   "\033[0m"
#define C_BOLD    "\033[1m"
#define C_RED     "\033[31m"
#define C_GREEN   "\033[32m"
#define C_YELLOW  "\033[33m"
#define C_BLUE    "\033[34m"
#define C_MAGENTA "\033[35m"
#define C_CYAN    "\033[36m"

static inline int use_color(void)
{
    static int checked = 0;
    static int enabled = 1;
    if (!checked)
    {
        checked = 1;
        if (!isatty(STDOUT_FILENO) || getenv("TINYSHELL_NO_COLOR"))
            enabled = 0;
    }
    return enabled;
}

#endif
