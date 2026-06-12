#ifndef COMMAND_H
#define COMMAND_H

typedef struct {
    char **args;
    char *infile;
    char *outfile;
    int append;
    int background;
} command_t;

typedef struct {
    command_t *commands;
    int ncommands;
} pipeline_t;

typedef struct {
    pipeline_t *pipelines;
    int npipelines;
} line_t;

void free_command(command_t *cmd);
void free_pipeline(pipeline_t *pl);
void free_line(line_t *line);

#endif
