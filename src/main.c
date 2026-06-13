#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <limits.h>

#include "../include/parser.h"
#include "../include/executor.h"
#include "../include/input.h"
#include "../include/command.h"
#include "../include/plugin.h"

#define RC_FILE ".tinyshellrc"
#define MAX_LINE 4096

static volatile sig_atomic_t sigint_received = 0;

static void sigint_handler(int sig)
{
    (void)sig;
    sigint_received = 1;
}

static void load_config(void)
{
    char *home = getenv("HOME");
    if (!home) return;

    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s/%s", home, RC_FILE);

    FILE *f = fopen(path, "r");
    if (!f) return;

    char line[MAX_LINE];
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

static int run_line(char *input)
{
    line_t line = parse_line(input);
    int last_status = 0;
    for (int i = 0; i < line.npipelines; i++)
        last_status = execute_pipeline(&line.pipelines[i]);
    free_line(&line);
    return last_status;
}

int main(int argc, char **argv)
{
    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    load_config();
    init_plugins();
    init_history();

    if (argc > 1 && strcmp(argv[1], "-c") == 0)
    {
        if (argc < 3)
        {
            fprintf(stderr, "tinyshell: -c requires an argument\n");
            return 1;
        }
        return run_line(argv[2]);
    }

    int last_status = 0;

    while (1)
    {
        if (sigint_received)
        {
            sigint_received = 0;
            last_status = 130;
        }

        char *input = read_input(last_status);

        if (sigint_received)
        {
            sigint_received = 0;
            last_status = 130;
            free(input);
            continue;
        }

        if (input == NULL)
        {
            printf("\n");
            break;
        }

        if (input[0] == '\0')
        {
            free(input);
            continue;
        }

        last_status = run_line(input);

        char st[16];
        snprintf(st, sizeof(st), "%d", last_status);
        setenv("?", st, 1);

        free(input);
    }

    save_history();
    return 0;
}
