//
// Created by Anton Skorkin on 26.06.2023.
//
#include <unistd.h>
#include <stdio.h>
#include "parser.h"
#include "String.h"

int main() {
    struct String string;
    string.capacity_ = string.size = 0;
    string.storage_ = NULL;
    printf("> ");
    for (;;) {
        char c;
        scanf("%c", &c);
        if (c == '\n')
            break;
        push(&string, c);
    }
    push(&string, '\0');
//    char* c = del_spaces(string.storage_, string.size);
//    printf("%s", c);
    struct cmd *cmds;
    int cmd_num;
    parse(string.storage_, &cmds, &cmd_num);
    int pip[2];
    for (int i = 0; i < cmd_num; i++) {
        if (!strcmp(cmds[i].name, "cd")) {
            chdir(cmds[i].args);
            continue;
        }
        if (!strcmp(cmds[i].name, "exit") && cmds[i].pipe != 0) { // pipe = 0 - |
            return 0;
        }
//        if ()
//        pipe(pip);
//        int pid = ;
        if (fork()) {
            char* cmd = (char*) malloc(sizeof (char) * (strlen(cmds[i].args) + strlen(cmds[i].name) + 1));
            strcpy(cmd, cmds[i].name);
            cmd[strlen(cmds[i].name)] = ' ';
            cmd[strlen(cmds[i].args) + strlen(cmds[i].name)] = '\0';
            if (cmds[i].args != NULL)
                strcpy(cmd + strlen(cmds[i].name), cmds[i].args + 1);
//            execl(cmd, cmds[i].args);
//            execl(cmd, NULL);
            printf("--- %s ---", cmd);
            execl("/bin/bash", "bash","-c", cmd, NULL);
            free(cmd);
//            return 0;
        }
        wait(NULL);
//        waitpid(pid, NULL, 0);
        printf("cmd: %s %s\n", cmds[i].name, cmds[i].args);

    }

    return 0;
}