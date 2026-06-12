#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <dirent.h>
#include <sys/stat.h>
#include <limits.h>
#include <errno.h>

#include "../include/input.h"
#include "../include/color.h"

#define MAX_HISTORY 1000
#define MAX_LINE 4096
#define HISTORY_FILE ".tinyshell_history"

static char *history[MAX_HISTORY];
static int history_count = 0;
static int history_pos = 0;

static struct termios orig_termios;
static int raw_mode_enabled = 0;

static void enable_raw_mode(void)
{
    if (raw_mode_enabled) return;
    if (!isatty(STDIN_FILENO)) return;

    tcgetattr(STDIN_FILENO, &orig_termios);
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON | ISIG);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    raw_mode_enabled = 1;
}

static void disable_raw_mode(void)
{
    if (!raw_mode_enabled) return;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
    raw_mode_enabled = 0;
}

void init_history(void)
{
    char *home = getenv("HOME");
    if (!home) return;

    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s/%s", home, HISTORY_FILE);

    FILE *f = fopen(path, "r");
    if (!f) return;

    char line[MAX_LINE];
    while (fgets(line, sizeof(line), f) && history_count < MAX_HISTORY)
    {
        line[strcspn(line, "\n")] = '\0';
        if (line[0] == '\0') continue;
        history[history_count++] = strdup(line);
    }
    fclose(f);
    history_pos = history_count;
}

static void append_history_file(const char *line)
{
    char *home = getenv("HOME");
    if (!home) return;

    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s/%s", home, HISTORY_FILE);

    FILE *f = fopen(path, "a");
    if (!f) return;
    fprintf(f, "%s\n", line);
    fclose(f);
}

void save_history(void)
{
    char *home = getenv("HOME");
    if (!home) return;

    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s/%s", home, HISTORY_FILE);

    FILE *f = fopen(path, "w");
    if (!f) return;

    int start = history_count > MAX_HISTORY ? history_count - MAX_HISTORY : 0;
    for (int i = start; i < history_count; i++)
        fprintf(f, "%s\n", history[i]);
    fclose(f);
}

void print_history(void)
{
    int start = history_count > 100 ? history_count - 100 : 0;
    for (int i = start; i < history_count; i++)
        printf("%5d  %s\n", i + 1, history[i]);
}

static void push_history(const char *line)
{
    if (line[0] == '\0') return;
    if (history_count > 0 && strcmp(history[history_count - 1], line) == 0)
        return;

    if (history_count >= MAX_HISTORY)
    {
        free(history[0]);
        memmove(history, history + 1, (MAX_HISTORY - 1) * sizeof(char *));
        history_count--;
    }

    history[history_count++] = strdup(line);
    history_pos = history_count;
    append_history_file(line);
}

static void build_prompt(char *buf, size_t size, int last_status)
{
    char hostname[256] = "?";
    char cwd[PATH_MAX] = "?";
    char shortpath[PATH_MAX];

    gethostname(hostname, sizeof(hostname));
    if (getcwd(cwd, sizeof(cwd)) == NULL)
        snprintf(cwd, sizeof(cwd), "?");

    char *user = getenv("USER");
    if (!user) user = "?";

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
        if (last_status != 0)
            snprintf(buf, size, "%s%s@%s %s%s%s %s[%d]%s $ ",
                     C_BOLD C_GREEN, user, hostname,
                     C_CYAN, display, C_RESET,
                     C_BOLD C_RED, last_status, C_RESET);
        else
            snprintf(buf, size, "%s%s@%s %s%s%s $ ",
                     C_BOLD C_GREEN, user, hostname,
                     C_CYAN, display, C_RESET);
    }
    else
    {
        if (last_status != 0)
            snprintf(buf, size, "%s@%s %s [%d] $ ", user, hostname, display, last_status);
        else
            snprintf(buf, size, "%s@%s %s $ ", user, hostname, display);
    }
}

static void refresh_line(const char *prompt, const char *buf, int pos, int len)
{
    printf("\r\033[K%s%s", prompt, buf);
    int move = len - pos;
    if (move > 0)
        printf("\033[%dD", move);
    fflush(stdout);
}

static int is_word_char(char c)
{
    return c > ' ' && c != '|' && c != '>' && c != '<' && c != '&' && c != ';';
}

static void find_current_word(const char *buf, int pos, int *start, int *end)
{
    *start = pos;
    while (*start > 0 && is_word_char(buf[*start - 1]))
        (*start)--;
    *end = pos;
    while (buf[*end] && is_word_char(buf[*end]))
        (*end)++;
}

static int complete_cmd(const char *word, char *out, size_t outsz)
{
    static const char *builtins[] = {
        "cd", "pwd", "echo", "clear", "exit", "export", "tsh", "help", NULL
    };

    // try builtins
    int match = -1;
    size_t wordlen = strlen(word);

    for (int i = 0; builtins[i]; i++)
    {
        if (strncmp(word, builtins[i], wordlen) == 0)
        {
            if (match >= 0) return 2; // multiple matches
            match = i;
        }
    }

    // try PATH
    char *path = getenv("PATH");
    if (path)
    {
        char *dup = strdup(path);
        char *dir = strtok(dup, ":");
        while (dir)
        {
            DIR *d = opendir(dir);
            if (d)
            {
                struct dirent *entry;
                while ((entry = readdir(d)) != NULL)
                {
                    if (strncmp(entry->d_name, word, wordlen) == 0 &&
                        strcmp(entry->d_name, ".") != 0 &&
                        strcmp(entry->d_name, "..") != 0)
                    {
                        if (match >= 0)
                        {
                            closedir(d);
                            free(dup);
                            return 2;
                        }
                        char full[PATH_MAX];
                        snprintf(full, sizeof(full), "%s/%s", dir, entry->d_name);
                        struct stat st;
                        if (stat(full, &st) == 0 && (st.st_mode & S_IXUSR))
                            match = -2; // mark as PATH match; we have the name
                    }
                }
                closedir(d);
            }
            dir = strtok(NULL, ":");
        }
        free(dup);
    }

    if (match >= 0)
    {
        snprintf(out, outsz, "%s", builtins[match] + wordlen);
        return 1;
    }

    // PATH match but we need the full name (we didn't store it)
    // Re-scan to get the full name
    if (path && wordlen > 0)
    {
        char *dup = strdup(path);
        char *dir = strtok(dup, ":");
        while (dir)
        {
            DIR *d = opendir(dir);
            if (d)
            {
                struct dirent *entry;
                while ((entry = readdir(d)) != NULL)
                {
                    if (strncmp(entry->d_name, word, wordlen) == 0 &&
                        strcmp(entry->d_name, ".") != 0 &&
                        strcmp(entry->d_name, "..") != 0)
                    {
                        char full[PATH_MAX];
                        snprintf(full, sizeof(full), "%s/%s", dir, entry->d_name);
                        struct stat st;
                        if (stat(full, &st) == 0 && (st.st_mode & S_IXUSR))
                        {
                            snprintf(out, outsz, "%s", entry->d_name + wordlen);
                            closedir(d);
                            free(dup);
                            return 1;
                        }
                    }
                }
                closedir(d);
            }
            dir = strtok(NULL, ":");
        }
        free(dup);
    }

    return 0;
}

static int complete_file(const char *word, char *out, size_t outsz)
{
    char dir[PATH_MAX] = ".";
    char prefix[PATH_MAX] = "";
    const char *slash = strrchr(word, '/');

    if (slash)
    {
        size_t dirlen = slash - word;
        strncpy(dir, word, dirlen);
        dir[dirlen] = '\0';
        if (dir[0] == '\0') strcpy(dir, "/");
        strcpy(prefix, slash + 1);
    }
    else
    {
        strcpy(prefix, word);
    }

    DIR *d = opendir(dir);
    if (!d) return 0;

    size_t prelen = strlen(prefix);
    int matches = 0;
    char first_match[PATH_MAX] = "";
    struct dirent *entry;

    while ((entry = readdir(d)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        if (strncmp(entry->d_name, prefix, prelen) == 0)
        {
            if (matches == 0)
            {
                if (slash)
                {
                    size_t dirlen2 = slash - word;
                    strncpy(first_match, word, dirlen2 + 1);
                    first_match[dirlen2 + 1] = '\0';
                    strcat(first_match, entry->d_name);
                }
                else
                {
                    strcpy(first_match, entry->d_name);
                }
            }
            matches++;
        }
    }
    closedir(d);

    if (matches == 1)
    {
        struct stat st;
        char full[PATH_MAX];
        if (slash)
            snprintf(full, sizeof(full), "%s/%s", dir, first_match + (slash - word + 1));
        else
            snprintf(full, sizeof(full), "%s/%s", dir, first_match);
        if (stat(full, &st) == 0 && S_ISDIR(st.st_mode))
            strcat(first_match, "/");
        snprintf(out, outsz, "%s", first_match + strlen(word));
        return 1;
    }

    return matches > 1 ? 2 : 0;
}

static int do_complete(char *buf, int *pos, int *len)
{
    int start, end;
    find_current_word(buf, *pos, &start, &end);

    char word[MAX_LINE] = "";
    strncpy(word, buf + start, end - start);
    word[end - start] = '\0';

    char completion[MAX_LINE] = "";
    int result = 0;

    // if first word (start == 0 or preceded by |/;), try command completion
    if (start == 0)
    {
        result = complete_cmd(word, completion, sizeof(completion));
        if (result == 0)
            result = complete_file(word, completion, sizeof(completion));
    }
    else
    {
        result = complete_file(word, completion, sizeof(completion));
        if (result == 0)
            result = complete_cmd(word, completion, sizeof(completion));
    }

    if (result == 1)
    {
        size_t complen = strlen(completion);
        if (*len + complen >= MAX_LINE - 1)
            return 0;

        memmove(buf + end + complen, buf + end, *len - end + 1);
        memcpy(buf + end, completion, complen);
        *len += complen;
        *pos = end + complen;
        return 1;
    }

    if (result == 2)
    {
        // multiple matches — print them
        printf("\n");
        // We won't re-list here for simplicity
        // Just show a bell/flash
        return 2;
    }

    return 0;
}

static void show_matches_cmd(const char *word)
{
    static const char *builtins[] = {
        "cd", "pwd", "echo", "clear", "exit", "export", "tsh", "help", NULL
    };

    size_t wordlen = strlen(word);
    int any = 0;

    for (int i = 0; builtins[i]; i++)
    {
        if (strncmp(word, builtins[i], wordlen) == 0)
        {
            printf("%s  ", builtins[i]);
            any = 1;
        }
    }

    char *path = getenv("PATH");
    if (path)
    {
        char *dup = strdup(path);
        char *dir = strtok(dup, ":");
        while (dir)
        {
            DIR *d = opendir(dir);
            if (d)
            {
                struct dirent *entry;
                while ((entry = readdir(d)) != NULL)
                {
                    if (strncmp(entry->d_name, word, wordlen) == 0 &&
                        strcmp(entry->d_name, ".") != 0 &&
                        strcmp(entry->d_name, "..") != 0)
                    {
                        char full[PATH_MAX];
                        snprintf(full, sizeof(full), "%s/%s", dir, entry->d_name);
                        struct stat st;
                        if (stat(full, &st) == 0 && (st.st_mode & S_IXUSR))
                        {
                            printf("%s  ", entry->d_name);
                            any = 1;
                        }
                    }
                }
                closedir(d);
            }
            dir = strtok(NULL, ":");
        }
        free(dup);
    }

    if (any) printf("\n");
}

static void show_matches_file(const char *word)
{
    char dir[PATH_MAX] = ".";
    char prefix[PATH_MAX] = "";
    const char *slash = strrchr(word, '/');

    if (slash)
    {
        size_t dirlen = slash - word;
        strncpy(dir, word, dirlen);
        dir[dirlen] = '\0';
        if (dir[0] == '\0') strcpy(dir, "/");
        strcpy(prefix, slash + 1);
    }
    else
    {
        strcpy(prefix, word);
    }

    DIR *d = opendir(dir);
    if (!d) return;

    size_t prelen = strlen(prefix);
    int any = 0;
    struct dirent *entry;

    while ((entry = readdir(d)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        if (strncmp(entry->d_name, prefix, prelen) == 0)
        {
            printf("%s  ", entry->d_name);
            any = 1;
        }
    }
    closedir(d);
    if (any) printf("\n");
}

char *read_input(int last_status)
{
    char prompt[512];
    build_prompt(prompt, sizeof(prompt), last_status);

    // Not a TTY — just do simple fgets, no editing
    if (!isatty(STDIN_FILENO))
    {
        printf("%s", prompt);
        fflush(stdout);
        char *buf = calloc(MAX_LINE, 1);
        if (!buf) return NULL;
        if (fgets(buf, MAX_LINE, stdin) == NULL)
        {
            free(buf);
            return NULL;
        }
        buf[strcspn(buf, "\n")] = '\0';
        push_history(buf);
        return buf;
    }

    char *buf = calloc(MAX_LINE, 1);
    if (!buf) return NULL;

    int pos = 0;
    int len = 0;

    printf("%s", prompt);
    fflush(stdout);

    enable_raw_mode();

    while (1)
    {
        char c;
        if (read(STDIN_FILENO, &c, 1) != 1)
        {
            if (errno == EINTR) continue;
            break;
        }

        if (c == 3)
        {
            // Ctrl+C — cancel current line, return empty
            disable_raw_mode();
            free(buf);
            return strdup("");
        }

        if (c == 4)
        {
            // Ctrl+D — EOF on empty line
            if (len == 0)
            {
                disable_raw_mode();
                free(buf);
                return NULL;
            }
            continue;
        }

        if (c == 13 || c == 10)
        {
            // Enter
            disable_raw_mode();
            printf("\n");
            push_history(buf);
            return buf;
        }

        if (c == 127 || c == 8)
        {
            // Backspace
            if (pos > 0)
            {
                memmove(buf + pos - 1, buf + pos, len - pos + 1);
                pos--;
                len--;
                refresh_line(prompt, buf, pos, len);
            }
            continue;
        }

        if (c == 9)
        {
            // Tab
            int result = do_complete(buf, &pos, &len);
            if (result == 1)
                refresh_line(prompt, buf, pos, len);
            else if (result == 2)
            {
                int start, end;
                find_current_word(buf, pos, &start, &end);
                char word[MAX_LINE] = "";
                strncpy(word, buf + start, end - start);
                word[end - start] = '\0';

                if (start == 0)
                {
                    show_matches_cmd(word);
                    if (word[0] != '\0' || end > 0)
                        show_matches_file(word);
                }
                else
                {
                    show_matches_file(word);
                }
                refresh_line(prompt, buf, pos, len);
            }
            continue;
        }

        if (c == '\033')
        {
            // Escape sequence
            char seq[3];
            if (read(STDIN_FILENO, &seq[0], 1) != 1) continue;
            if (read(STDIN_FILENO, &seq[1], 1) != 1) continue;

            if (seq[0] == '[')
            {
                switch (seq[1])
                {
                    case 'A': // Up
                        if (history_pos > 0)
                        {
                            history_pos--;
                            int hlen = strlen(history[history_pos]);
                            if (hlen > MAX_LINE - 1) hlen = MAX_LINE - 1;
                            memcpy(buf, history[history_pos], hlen);
                            buf[hlen] = '\0';
                            len = hlen;
                            pos = len;
                            refresh_line(prompt, buf, pos, len);
                        }
                        break;

                    case 'B': // Down
                        if (history_pos < history_count)
                        {
                            history_pos++;
                            if (history_pos == history_count)
                            {
                                buf[0] = '\0';
                                len = 0;
                                pos = 0;
                            }
                            else
                            {
                                int hlen = strlen(history[history_pos]);
                                if (hlen > MAX_LINE - 1) hlen = MAX_LINE - 1;
                                memcpy(buf, history[history_pos], hlen);
                                buf[hlen] = '\0';
                                len = hlen;
                                pos = len;
                            }
                            refresh_line(prompt, buf, pos, len);
                        }
                        break;

                    case 'C': // Right
                        if (pos < len) pos++;
                        refresh_line(prompt, buf, pos, len);
                        break;

                    case 'D': // Left
                        if (pos > 0) pos--;
                        refresh_line(prompt, buf, pos, len);
                        break;

                    case 'H': // Home
                        pos = 0;
                        refresh_line(prompt, buf, pos, len);
                        break;

                    case 'F': // End
                        pos = len;
                        refresh_line(prompt, buf, pos, len);
                        break;

                    case '1': // Possible Home (^[1~) or other
                    {
                        char extra;
                        if (read(STDIN_FILENO, &extra, 1) == 1 && extra == '~')
                        {
                            if (seq[1] == '1') { pos = 0; refresh_line(prompt, buf, pos, len); }
                            else if (seq[1] == '3') // Delete
                            {
                                if (pos < len)
                                {
                                    memmove(buf + pos, buf + pos + 1, len - pos);
                                    len--;
                                    refresh_line(prompt, buf, pos, len);
                                }
                            }
                            else if (seq[1] == '4') { pos = len; refresh_line(prompt, buf, pos, len); }
                        }
                        break;
                    }

                    case '3': // Delete (^[3~)
                    {
                        char extra;
                        if (read(STDIN_FILENO, &extra, 1) == 1 && extra == '~')
                        {
                            if (pos < len)
                            {
                                memmove(buf + pos, buf + pos + 1, len - pos);
                                len--;
                                refresh_line(prompt, buf, pos, len);
                            }
                        }
                        break;
                    }
                }
            }
            continue;
        }

        if (c >= 32 && c <= 126)
        {
            if (len >= MAX_LINE - 1) continue;
            memmove(buf + pos + 1, buf + pos, len - pos + 1);
            buf[pos] = c;
            pos++;
            len++;
            refresh_line(prompt, buf, pos, len);
        }
    }

    disable_raw_mode();
    free(buf);
    return NULL;
}
