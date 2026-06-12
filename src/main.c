#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <limits.h>

#include "../include/parser.h"
#include "../include/executor.h"
#include "../include/prompt.h"
#include "../include/command.h"

#define SHLINE_MAX 4096
#define RC_FILE ".tinyshellrc"

static volatile sig_atomic_t sigint_received = 0;

static void sigint_handler(int sig)
{
    (void)sig;
    sigint_received = 1;
    write(STDOUT_FILENO, "\n", 1);
}

static void load_config(void)
{
    char *home = getenv("HOME");
    if (!home) return;

    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s/%s", home, RC_FILE);

    FILE *f = fopen(path, "r");
    if (!f) return;

    char line[LINE_MAX];
    while (fgets(line, sizeof(line), f))
    {
        line[strcspn(line, "\n")] = '\0';

        char *p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '\0' || *p == '#')
            continue;

        line_t l = parse_line(line);
        for (int i = 0; i < l.npipelines; i++)
            execute_pipeline(&l.pipelines[i]);
        free_line(&l);
    }

    fclose(f);
}

int main(void)
{
    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    load_config();

    char input[LINE_MAX];
    int last_status = 0;

    while (1)
    {
        sigint_received = 0;
        print_prompt(last_status);

        if (fgets(input, sizeof(input), stdin) == NULL)
        {
            if (feof(stdin))
            {
                printf("\n");
                break;
            }
            clearerr(stdin);
            last_status = 130;
            continue;
        }

        if (sigint_received)
        {
            last_status = 130;
            continue;
        }

        input[strcspn(input, "\n")] = '\0';

        line_t line = parse_line(input);

        for (int i = 0; i < line.npipelines; i++)
        {
            last_status = execute_pipeline(&line.pipelines[i]);
        }

        char st[16];
        snprintf(st, sizeof(st), "%d", last_status);
        setenv("?", st, 1);

        free_line(&line);
    }

    return 0;
}
