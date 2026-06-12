#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../include/parser.h"
#include "../include/executor.h"
#include "../include/prompt.h"
#include "../include/command.h"

#define MAX_INPUT 4096

int main(void)
{
    char input[MAX_INPUT];
    int last_status = 0;

    while (1)
    {
        print_prompt(last_status);

        if (fgets(input, sizeof(input), stdin) == NULL)
        {
            printf("\n");
            break;
        }

        input[strcspn(input, "\n")] = '\0';

        line_t line = parse_line(input);

        for (int i = 0; i < line.npipelines; i++)
        {
            last_status = execute_pipeline(&line.pipelines[i]);
        }

        free_line(&line);
    }

    return 0;
}
