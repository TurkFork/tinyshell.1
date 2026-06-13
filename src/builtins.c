#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>

#include "../include/color.h"
#include "../include/version.h"
#include "../include/input.h"

extern char **environ;

static char *prev_dir = NULL;

int builtin_cd(char **args)
{
    char *target;

    if (args[1] == NULL || strcmp(args[1], "~") == 0)
    {
        target = getenv("HOME");
        if (!target)
        {
            fprintf(stderr, "cd: HOME not set\n");
            return 1;
        }
    }
    else if (strcmp(args[1], "-") == 0)
    {
        if (!prev_dir)
        {
            fprintf(stderr, "cd: no previous directory\n");
            return 1;
        }
        target = prev_dir;
        printf("%s\n", target);
    }
    else
    {
        target = args[1];
    }

    char old_cwd[PATH_MAX];
    int have_old = getcwd(old_cwd, sizeof(old_cwd)) != NULL;

    if (chdir(target) != 0)
    {
        perror("cd");
        return 1;
    }

    if (have_old)
    {
        free(prev_dir);
        prev_dir = strdup(old_cwd);
    }

    return 0;
}

int builtin_pwd(char **args)
{
    (void)args;
    char cwd[1024];

    if (getcwd(cwd, sizeof(cwd)) != NULL)
    {
        printf("%s\n", cwd);
    }
    else
    {
        perror("pwd");
        return 1;
    }

    return 0;
}

int builtin_help(char **args)
{
    (void)args;
    const char *h = use_color() ? C_BOLD : "";
    const char *r = use_color() ? C_RESET : "";

    printf("%sTinyShell%s — built-in commands:\n", h, r);
    printf("  %scd%s [dir]      Change directory (~, -, no arg = home)\n",
           use_color() ? C_GREEN : "", r);
    printf("  %spwd%s           Print working directory\n",
           use_color() ? C_GREEN : "", r);
    printf("  %secho%s <text>   Print text\n",
           use_color() ? C_GREEN : "", r);
    printf("  %sclear%s         Clear screen\n",
           use_color() ? C_GREEN : "", r);
    printf("  %sexit%s [n]      Exit shell (default: 0)\n",
           use_color() ? C_GREEN : "", r);
    printf("  %sexport%s [VAR=val..]  Set/show environment variables\n",
           use_color() ? C_GREEN : "", r);
    printf("  %shelp%s          Show this help\n",
           use_color() ? C_GREEN : "", r);
    printf("\n");
    printf("%sFeatures:%s\n", h, r);
    printf("  cmd1 | cmd2       Pipe\n");
    printf("  cmd > file        Redirect stdout (overwrite)\n");
    printf("  cmd >> file       Redirect stdout (append)\n");
    printf("  cmd < file        Redirect stdin\n");
    printf("  cmd &             Background\n");
    printf("  cmd1 ; cmd2       Sequential\n");
    printf("  $VAR, $?          Variable expansion\n");
    printf("  \"...\" '...'       Quoting\n");
    printf("  ~                 Home directory\n");
    printf("  ~/.tinyshellrc    Config file (sourced on startup)\n");
    printf("  -c <cmd>          Run command non-interactively\n");
    printf("  Ctrl+C            Interrupt (doesn't kill shell)\n");

    return 0;
}

int builtin_clear(char **args)
{
    (void)args;
    printf("\033[H\033[J");
    return 0;
}

int builtin_echo(char **args)
{
    for (int i = 1; args[i] != NULL; i++)
    {
        printf("%s", args[i]);
        if (args[i + 1] != NULL)
            printf(" ");
    }
    printf("\n");
    return 0;
}

int builtin_export(char **args)
{
    if (args[1] == NULL)
    {
        for (char **e = environ; *e; e++)
            printf("%s\n", *e);
        return 0;
    }

    for (int i = 1; args[i]; i++)
    {
        char *eq = strchr(args[i], '=');
        if (eq)
        {
            *eq = '\0';
            setenv(args[i], eq + 1, 1);
            *eq = '=';
        }
        else
        {
            char *val = getenv(args[i]);
            if (val)
                setenv(args[i], val, 1);
        }
    }
    return 0;
}

int builtin_exit(char **args)
{
    int code = 0;
    if (args[1] != NULL)
        code = atoi(args[1]);
    exit(code);
    return 0;
}

static int version_compare(const char *a, const char *b)
{
    if (*a == 'v') a++;
    if (*b == 'v') b++;

    int ma, mi, pa, mb, pb, pc;
    if (sscanf(a, "%d.%d.%d", &ma, &mi, &pa) < 2) return 0;
    if (sscanf(b, "%d.%d.%d", &mb, &pb, &pc) < 2) return 0;

    if (ma != mb) return ma > mb ? 1 : -1;
    if (mi != pb) return mi > pb ? 1 : -1;
    if (pa != pc) return pa > pc ? 1 : -1;
    return 0;
}

static int check_update(void)
{
    char url[256];
    snprintf(url, sizeof(url), "https://api.github.com/repos/%s/releases/latest",
             TINYSHELL_REPO);

    char cmd[512];
    snprintf(cmd, sizeof(cmd), "curl -sL --connect-timeout 5 \"%s\"", url);

    FILE *f = popen(cmd, "r");
    if (!f)
    {
        printf("Could not check for updates (curl failed).\n");
        return 1;
    }

    char line[1024];
    char tag[64] = {0};
    int is_error = 0;

    while (fgets(line, sizeof(line), f))
    {
        if (strstr(line, "\"message\"")) is_error = 1;

        char *p = strstr(line, "\"tag_name\"");
        if (!p) continue;

        p = strchr(p, ':');
        if (!p) continue;
        p++;

        while (*p && *p != '"') p++;
        if (!*p) continue;
        p++;

        char *end = strchr(p, '"');
        if (!end) continue;
        *end = '\0';

        strncpy(tag, p, sizeof(tag) - 1);
        tag[sizeof(tag) - 1] = '\0';
        break;
    }

    int status = pclose(f);

    if (tag[0] == '\0')
    {
        if (status != 0)
            printf("Could not check for updates (network error).\n");
        else if (is_error)
            printf("No releases found on GitHub.\n");
        else
            printf("Could not parse latest release info.\n");
        return 1;
    }

    int cmp = version_compare(tag, TINYSHELL_VERSION_TAG);

    if (cmp > 0)
    {
        printf("Update available: %s (current: %s)\n", tag, TINYSHELL_VERSION_TAG);
        printf("  https://github.com/%s/releases/tag/%s\n", TINYSHELL_REPO, tag);
    }
    else if (cmp < 0)
    {
        printf("Current version (%s) is newer than latest release (%s).\n",
               TINYSHELL_VERSION_TAG, tag);
    }
    else
    {
        printf("TinyShell is up to date (%s).\n", TINYSHELL_VERSION_TAG);
    }

    return 0;
}

int builtin_tsh(char **args)
{
    if (args[1] == NULL || strcmp(args[1], "-v") == 0 || strcmp(args[1], "--version") == 0)
    {
        printf("TinyShell %s\n", TINYSHELL_VERSION_TAG);
        return 0;
    }

    if (strcmp(args[1], "update") == 0 || strcmp(args[1], "check") == 0)
    {
        return check_update();
    }

    if (strcmp(args[1], "history") == 0)
    {
        print_history();
        return 0;
    }

    if (strcmp(args[1], "help") == 0 || strcmp(args[1], "-h") == 0 || strcmp(args[1], "--help") == 0)
    {
        printf("Usage: tsh <command>\n");
        printf("\n");
        printf("Commands:\n");
        printf("  -v, --version   Show version\n");
        printf("  update, check   Check for updates on GitHub\n");
        printf("  history         Show command history\n");
        printf("  help, -h        Show this help\n");
        return 0;
    }

    fprintf(stderr, "tsh: unknown command '%s' (see: tsh help)\n", args[1]);
    return 1;
}