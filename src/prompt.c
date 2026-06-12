#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>

#include "../include/color.h"

void print_prompt(int last_status)
{
    char hostname[256];
    char cwd[PATH_MAX];
    char shortpath[PATH_MAX];

    gethostname(hostname, sizeof(hostname));
    getcwd(cwd, sizeof(cwd));

    char *user = getenv("USER");
    if (!user) user = "user";

    char *home = getenv("HOME");
    char *display = cwd;

    if (home)
    {
        size_t homelen = strlen(home);
        if (strncmp(cwd, home, homelen) == 0)
        {
            if (cwd[homelen] == '\0')
            {
                shortpath[0] = '~';
                shortpath[1] = '\0';
            }
            else
            {
                snprintf(shortpath, sizeof(shortpath), "~%s", cwd + homelen);
            }
            display = shortpath;
        }
    }

    if (use_color())
    {
        printf("%s%s@%s %s%s%s",
               C_BOLD C_GREEN, user, hostname,
               C_CYAN, display, C_RESET);
    }
    else
    {
        printf("%s@%s %s", user, hostname, display);
    }

    if (last_status != 0)
    {
        if (use_color())
            printf(" %s[%d]%s $ ", C_BOLD C_RED, last_status, C_RESET);
        else
            printf(" [%d] $ ", last_status);
    }
    else
    {
        printf(" $ ");
    }

    fflush(stdout);
}
