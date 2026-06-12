#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "../include/command.h"

#define MAX_TOKENS 128

static int expand_var(char *buf, size_t bufsz, const char **pp)
{
    (*pp)++;
    if (**pp == '\0' || **pp == ' ' || **pp == '\t')
    {
        strncat(buf, "$", bufsz - strlen(buf) - 1);
        return 0;
    }

    char varname[256];
    int i = 0;
    if (**pp == '{')
    {
        (*pp)++;
        while (**pp && **pp != '}' && i < 254)
            varname[i++] = *(*pp)++;
        if (**pp == '}') (*pp)++;
    }
    else
    {
        while (**pp && (isalnum((unsigned char)**pp) || **pp == '_') && i < 254)
            varname[i++] = *(*pp)++;
    }
    varname[i] = '\0';

    char *val = getenv(varname);
    if (val)
        strncat(buf, val, bufsz - strlen(buf) - 1);
    return 1;
}

static char *expand_tilde(const char *word)
{
    if (word[0] != '~')
        return strdup(word);

    char *home = getenv("HOME");
    if (!home)
        return strdup(word);

    size_t homelen = strlen(home);
    size_t wordlen = strlen(word);
    char *result = malloc(homelen + wordlen + 1);
    if (!result) return NULL;

    strcpy(result, home);
    strcat(result, word + 1);
    return result;
}

static char *expand_word(const char *start, int len, int do_vars)
{
    char buf[4096] = {0};
    const char *p = start;
    const char *end = start + len;

    while (p < end)
    {
        if (do_vars && *p == '$')
        {
            expand_var(buf, sizeof(buf), &p);
        }
        else
        {
            strncat(buf, p, 1);
            p++;
        }
    }

    char *result = expand_tilde(buf);
    return result;
}

static char *dupstr(const char *start, int len)
{
    char *s = malloc(len + 1);
    if (!s) return NULL;
    strncpy(s, start, len);
    s[len] = '\0';
    return s;
}

static int split_segments(char *input, char sep, char ***out)
{
    int cap = 8;
    int n = 0;
    *out = malloc(cap * sizeof(char *));
    if (!*out) return -1;

    char *p = input;
    while (*p)
    {
        while (*p == ' ' || *p == '\t')
            p++;
        if (*p == '\0')
            break;

        int start = p - input;
        int dq = 0, sq = 0;

        while (*p)
        {
            if (*p == '"' && !sq) dq = !dq;
            if (*p == '\'' && !dq) sq = !sq;
            if (*p == sep && !dq && !sq)
                break;
            p++;
        }

        if (n >= cap)
        {
            cap *= 2;
            *out = realloc(*out, cap * sizeof(char *));
        }

        int seglen = (p - input) - start;
        if (seglen > 0)
        {
            (*out)[n] = dupstr(input + start, seglen);
            n++;
        }

        if (*p == sep) p++;
    }

    (*out)[n] = NULL;
    return n;
}

static command_t parse_command_str(char *str)
{
    command_t cmd = {NULL, NULL, NULL, 0, 0};
    char *args[MAX_TOKENS];
    int argc = 0;
    char *p = str;

    while (*p)
    {
        while (*p == ' ' || *p == '\t')
            p++;

        if (*p == '\0')
            break;

        if (*p == '>')
        {
            p++;
            if (*p == '>')
            {
                cmd.append = 1;
                p++;
            }
            while (*p == ' ' || *p == '\t') p++;
            if (*p == '\0') break;

            if (*p == '"')
            {
                p++;
                int s = p - str;
                while (*p && *p != '"') p++;
                free(cmd.outfile);
                cmd.outfile = dupstr(str + s, p - str - s);
                if (*p) p++;
            }
            else
            {
                int s = p - str;
                while (*p && *p != ' ' && *p != '\t' && *p != '>' && *p != '<' && *p != '|' && *p != '&')
                    p++;
                free(cmd.outfile);
                cmd.outfile = dupstr(str + s, p - str - s);
            }
            continue;
        }

        if (*p == '<')
        {
            p++;
            while (*p == ' ' || *p == '\t') p++;
            if (*p == '\0') break;

            if (*p == '"')
            {
                p++;
                int s = p - str;
                while (*p && *p != '"') p++;
                free(cmd.infile);
                cmd.infile = dupstr(str + s, p - str - s);
                if (*p) p++;
            }
            else
            {
                int s = p - str;
                while (*p && *p != ' ' && *p != '\t' && *p != '>' && *p != '<' && *p != '|' && *p != '&')
                    p++;
                free(cmd.infile);
                cmd.infile = dupstr(str + s, p - str - s);
            }
            continue;
        }

        if (*p == '&')
        {
            cmd.background = 1;
            p++;
            continue;
        }

        if (argc >= MAX_TOKENS - 1)
            break;

        if (*p == '"')
        {
            p++;
            int s = p - str;
            while (*p && *p != '"') p++;
            args[argc] = expand_word(str + s, p - str - s, 1);
            if (*p) p++;
            argc++;
        }
        else if (*p == '\'')
        {
            p++;
            int s = p - str;
            while (*p && *p != '\'') p++;
            args[argc] = dupstr(str + s, p - str - s);
            if (*p) p++;
            argc++;
        }
        else
        {
            int s = p - str;
            while (*p && *p != ' ' && *p != '\t' && *p != '>' && *p != '<' && *p != '|' && *p != '&')
                p++;
            args[argc] = expand_word(str + s, p - str - s, 1);
            argc++;
        }
    }

    args[argc] = NULL;
    cmd.args = malloc((argc + 1) * sizeof(char *));
    for (int i = 0; i <= argc; i++)
        cmd.args[i] = args[i];

    return cmd;
}

line_t parse_line(char *input)
{
    line_t line = {NULL, 0};

    char **pipeline_strs = NULL;
    int np = split_segments(input, ';', &pipeline_strs);

    if (np <= 0)
    {
        free(pipeline_strs);
        return line;
    }

    line.pipelines = calloc(np, sizeof(pipeline_t));
    int npipes = 0;

    for (int pi = 0; pi < np; pi++)
    {
        char **cmd_strs = NULL;
        int nc = split_segments(pipeline_strs[pi], '|', &cmd_strs);

        if (nc <= 0)
        {
            free(cmd_strs);
            continue;
        }

        pipeline_t pl = {NULL, nc};
        pl.commands = calloc(nc, sizeof(command_t));

        for (int ci = 0; ci < nc; ci++)
        {
            if (ci == nc - 1)
            {
                command_t cmd = parse_command_str(cmd_strs[ci]);
                pl.commands[ci] = cmd;
            }
            else
            {
                command_t cmd = parse_command_str(cmd_strs[ci]);
                cmd.background = 0;
                pl.commands[ci] = cmd;
            }
        }

        int bg = 0;
        for (int ci = nc - 1; ci >= 0 && !bg; ci--)
            bg = pl.commands[ci].background;

        for (int ci = 0; ci < nc; ci++)
            pl.commands[ci].background = bg;

        line.pipelines[npipes++] = pl;

        for (int i = 0; i < nc; i++)
            free(cmd_strs[i]);
        free(cmd_strs);
    }

    for (int i = 0; i < np; i++)
        free(pipeline_strs[i]);
    free(pipeline_strs);

    line.npipelines = npipes;
    return line;
}

void free_command(command_t *cmd)
{
    if (cmd->args)
    {
        for (int i = 0; cmd->args[i]; i++)
            free(cmd->args[i]);
        free(cmd->args);
    }
    free(cmd->infile);
    free(cmd->outfile);
}

void free_pipeline(pipeline_t *pl)
{
    for (int i = 0; i < pl->ncommands; i++)
        free_command(&pl->commands[i]);
    free(pl->commands);
}

void free_line(line_t *line)
{
    for (int i = 0; i < line->npipelines; i++)
        free_pipeline(&line->pipelines[i]);
    free(line->pipelines);
}
