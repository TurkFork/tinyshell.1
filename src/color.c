#include <unistd.h>
#include <stdlib.h>

#include "../include/color.h"

int use_color(void)
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
