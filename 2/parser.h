//
// Created by Anton Skorkin on 26.06.2023.
//

#ifndef SYSPROG_PARSER_H
#define SYSPROG_PARSER_H

#include <stdlib.h>
#include <string.h>

struct cmd {
    char *name;
    char *args;
    int pipe; // 0 - |, 1 - &, 2 - ||, 3 - &&
    char *output_file; // in case of > or >> elsewhere it = 0
};

char *del_spaces(const char *str, const int len) {
    int pref = 0;
    int suf = 0;
    int mid = 0;
    for (int i = 0; i < len; i++) {
        if (str[i] == ' ')
            pref++;
        else
            break;
    }
    for (int i = len - 1; i >= 0; i--) {
        if (str[i] == ' ')
            suf++;
        else
            break;
    }
    int is_in_q = 0;  // '
    int is_in_qq = 0; // "
    for (int i = pref; i < len - suf; i++) {
        if (str[i] == '\'' && (i == 0 || str[i - 1] != '\\'))
            is_in_q = 1 - is_in_q;
        if (str[i] == '\"' && (i == 0 || str[i - 1] != '\\'))
            is_in_qq = 1 - is_in_qq;
        if (!is_in_qq && !is_in_q && i != 0) {
            if (str[i] == ' ' && str[i - 1] == ' ')
                mid++;
            else if (str[i] == '|' && str[i - 1] != '|' && str[i - 1] != ' ')
                mid--;
            else if (str[i] == '&' && str[i - 1] != '&' && str[i - 1] != ' ')
                mid--;
            else if (str[i] == '>' && str[i - 1] != '>' && str[i - 1] != ' ')
                mid--;
            else if (str[i] != ' ' && (str[i - 1] == '|' || str[i - 1] == '>' || str[i - 1] == '&'))
                mid--;
        }
    }
    char *res = (char *) malloc(sizeof(char) * (len - pref - suf - mid + 1));
    for (int i = pref, j = 0; i < len - suf; i++) {
        if (str[i] == '\'' && (i == 0 || str[i - 1] != '\\'))
            is_in_q = 1 - is_in_q;
        if (str[i] == '\"' && (i == 0 || str[i - 1] != '\\'))
            is_in_qq = 1 - is_in_qq;
        if (!is_in_qq && !is_in_q && i != 0) {
            if (str[i] != ' ' || str[i - 1] != ' ') {
                res[j++] = str[i];
            }
            if (i != len - suf - 1) {
                if (str[i + 1] == '|' && str[i] != '|' && str[i] != ' ')
                    res[j++] = ' ';
                else if (str[i + 1] == '&' && str[i] != '&' && str[i] != ' ')
                    res[j++] = ' ';
                else if (str[i + 1] == '>' && str[i] != '>' && str[i] != ' ')
                    res[j++] = ' ';
                else if (str[i + 1] != ' ' && str[i + 1] != '>' && str[i + 1] != '|' && str[i + 1] != '&' &&
                         (str[i] == '|' || str[i] == '>' || str[i] == '&'))
                    res[j++] = ' ';
            }
        } else {
            res[j++] = str[i];
        }
    }
    res[len - pref - suf - mid] = '\0';
    return res;
}
//
//struct cmd *parse_one_cmd(const char *str) { // ' ', > >> | || & &&
//    size_t input_size = strlen(str);
//    char* input = del_spaces(str, input_size);
//    struct cmd *result = (struct cmd *) malloc(sizeof(struct cmd));
//    int argc = 0;
//    int is_in_q = 0;  // '
//    int is_in_qq = 0; // "
//    for (int i = 0; i < input_size; i++) {
//        if (input[i] == '\'' && (i == 0 || input[i - 1] != '\\'))
//            is_in_q = 1 - is_in_q;
//        if (input[i] == '\"' && (i == 0 || input[i - 1] != '\\'))
//            is_in_qq = 1 - is_in_qq;
//
//        if (!is_in_q && !is_in_qq && i != 0 && (input[i] == ' ' || input[i] == '>' || input[i] == '|') &&
//            (input[i - 1] != ' ' && input[i - 1] != '>' && input[i - 1] != '|')) {
//            argc++;
//        }
//    }
//    result->argc = argc;
//    if (argc == 0) {
//        result->name = (char *) malloc(sizeof(char) * (input_size + 1));
//        memcpy(result->name, input, input_size * sizeof(char));
//        result->name[input_size] = '\0';
//        free(input);
//        return result;
//    }
//    result->argv = (char **) malloc(sizeof(char *) * argc);
//    is_in_q = 0;  // '
//    is_in_qq = 0; // "
//    int arg_ind = 0;
//    int start_ind_of_par = 0;
//    for (int i = 0; i < input_size; i++) {
//        if (input[i] == '\'' && (i == 0 || input[i - 1] != '\\'))
//            is_in_q = 1 - is_in_q;
//        if (input[i] == '\"' && (i == 0 || input[i - 1] != '\\'))
//            is_in_qq = 1 - is_in_qq;
//
//        if (!is_in_qq && !is_in_q && input[i] == '|') {
//
//        }
//
//        if (!is_in_q && !is_in_qq && i != 0 && (input[i] == ' ' || input[i] == '>') &&
//            (input[i - 1] != ' ' && input[i - 1] != '>')) {
//            if (arg_ind++ == 0) {
//                result->name = (char *) malloc(sizeof(char) * (i + 1));
//                memcpy(result->name, input, i * sizeof(char));
//                result->name[i] = '\0';
//            } else {
//                result->argv[arg_ind - 2] = (char *) malloc(sizeof(char) * (i - start_ind_of_par + 1 + 1));
//                memcpy(result->argv[arg_ind - 2], &input[start_ind_of_par], sizeof(char) * (i - start_ind_of_par + 1));
//                result->argv[arg_ind - 2][i - start_ind_of_par + 1] = '\0';
//            }
//            if (input[i] == '>') {
//                result->argv[argc - 1] = (char *) malloc(sizeof(char) * (input_size - i + 1 + 1));
//                memcpy(result->argv[argc - 1], &input[i], sizeof(char) * (input_size - i + 1));
//                result->argv[argc - 1][input_size - i + 1] = '\0';
//                break;
//            }
//            start_ind_of_par = i + 1;
//        }
//        if (i == input_size - 1 && argc > 0) {
//            i++;
//            result->argv[arg_ind - 1] = (char *) malloc(sizeof(char) * (i - start_ind_of_par + 1 + 1));
//            memcpy(result->argv[arg_ind - 1], &input[start_ind_of_par], sizeof(char) * (i - start_ind_of_par + 1));
//            result->argv[arg_ind - 1][i - start_ind_of_par + 1] = '\0';
//        }
//    }
//    free(input);
//    return result;
//}

void parse(const char *str, struct cmd **result, int *result_size) { // | || & && > >> \ ' '
    char *input = del_spaces(str, strlen(str));
    const size_t input_size = strlen(input);
    struct cmd *out;
    int cmd_size = 1;
    int is_in_q = 0;  // '
    int is_in_qq = 0; // "
    for (int i = 0; i < input_size; i++) {
        if (input[i] == '\'' && (i == 0 || input[i - 1] != '\\'))
            is_in_q = 1 - is_in_q;
        if (input[i] == '\"' && (i == 0 || input[i - 1] != '\\'))
            is_in_qq = 1 - is_in_qq;
        if (!is_in_q && !is_in_qq && (i == 0 || input[i - 1] != '\\') && input[i] == '|') {
            if (input[i] == '|' || input[i] == '&') {
                cmd_size++;
                if (input[i + 1] == '|' || input[i + 1] == '&') {
                    i++;
                    continue;
                }
            }
        }
    }
    out = (struct cmd *) malloc(sizeof(struct cmd) * cmd_size);
    int start_of_cur_cmd = 0;
    int ind = 0;
    is_in_q = 0;  // '
    is_in_qq = 0; // "
    int cmd_flag = 0;
    for (int i = 0; i < input_size; i++) {
        if (input[i] == '\'' && (i == 0 || input[i - 1] != '\\'))
            is_in_q = 1 - is_in_q;
        if (input[i] == '\"' && (i == 0 || input[i - 1] != '\\'))
            is_in_qq = 1 - is_in_qq;
        if (!is_in_q && !is_in_qq && (i == 0 || input[i - 1] != '\\')) {
            if (input[i] == ' ' && !cmd_flag) {
                cmd_flag = 1;
                out[ind].name = (char *) malloc(sizeof(char) * (i - start_of_cur_cmd + 1));
                memcpy(out[ind].name, input + start_of_cur_cmd, sizeof(char) * (i - start_of_cur_cmd));
                out[ind].name[i - start_of_cur_cmd] = '\0';
                start_of_cur_cmd = i + 1;
            }
            if (input[i] == '|' || input[i] == '&') {
                cmd_flag = 0;
                start_of_cur_cmd = i + 2;
                out[ind].args = (char *) malloc(sizeof(char) * (i - start_of_cur_cmd));
                memcpy(out[ind].args, input + start_of_cur_cmd, sizeof(char) * (i - start_of_cur_cmd - 1));
                out[ind].args[i - start_of_cur_cmd - 1] = '\0';
                if (input[i] == '|')
                    out[ind].pipe = 0;
                else // &
                    out[ind].pipe = 1;
                if (input[i + 1] == '|' || input[i + 1] == '&') {
                    i++;
                    start_of_cur_cmd++;
                    out[ind++].pipe += 2;
                    continue;
                }
                ind++;
            }
        }
    }
    if (!cmd_flag) {
        out[ind].name = (char *) malloc(sizeof(char) * (input_size - start_of_cur_cmd + 1));
        memcpy(out[ind].name, input + start_of_cur_cmd, sizeof(char) * (input_size - start_of_cur_cmd));
        out[ind].name[input_size - start_of_cur_cmd] = '\0';
        out[ind].pipe = -1;
    } else {
        out[ind].args = (char *) malloc(sizeof(char) * (input_size - start_of_cur_cmd + 1));
        memcpy(out[ind].args, input + start_of_cur_cmd, sizeof(char) * (input_size - start_of_cur_cmd));
        out[ind].args[input_size - start_of_cur_cmd] = '\0';
        out[ind].pipe = -1;
    }
    free(input);
    *result = out;
    *result_size = cmd_size;
}

#endif //SYSPROG_PARSER_H

// TODO: && > >> &