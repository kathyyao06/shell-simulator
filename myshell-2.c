#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

char error_message[30] = "An error has occurred\n";

bool exit_if_true = false;
char *redir_file;

char **extract_arguments(char *argstr, int token_counter)
{
    char **aargs = (char **)malloc((token_counter + 1) * sizeof(char *));

    for (int i = 0; i < token_counter + 1; i++)
    {
        aargs[i] = NULL;
    }

    char *argtok = strtok(argstr, " \t\n");
    for (int arg_arr_index = 0; argtok; argtok = strtok(NULL, " \t\n"), arg_arr_index++)
    {
        aargs[arg_arr_index] = argtok;
    }

    return aargs;
}

void myPrint(char *msg)
{
    write(STDOUT_FILENO, msg, strlen(msg));
}

void execute_redir(char *redir_target_file, bool called_from_advanced, bool do_advanced)
{
    int mainfile_fd;
    int sub_text;

    if (do_advanced && !called_from_advanced)
    {
        int try_open = access(redir_target_file, F_OK);

        switch (try_open)
        {
        case -1:
        {
            execute_redir(redir_target_file, true, true);
            return;
        }

        default:
        {
            mainfile_fd = open(redir_target_file, O_RDWR | O_CREAT | O_APPEND, 0664);

            int temp_reader;
            char reading_buffer[100];

            sub_text = open("sub_text", O_RDWR | O_CREAT | O_APPEND, 0664);

            while ((temp_reader = read(mainfile_fd, reading_buffer, 100)))
            {
                write(sub_text, reading_buffer, temp_reader);
            }

            mainfile_fd = open(redir_target_file, O_RDWR | O_CREAT | O_APPEND | O_TRUNC, 0664);
            lseek(mainfile_fd, 0, SEEK_SET);
            dup2(mainfile_fd, STDOUT_FILENO);

            close(sub_text);
            close(mainfile_fd);
            return;
        }
        }
    }
    else if (!do_advanced || called_from_advanced)
    {

        int try_open = access(redir_target_file, F_OK);

        switch (try_open)
        {
        case -1:
        {
            mainfile_fd = open(redir_target_file, O_RDWR | O_CREAT | O_APPEND | O_TRUNC, 0664);
            if (mainfile_fd != -1)
            {
                dup2(mainfile_fd, STDOUT_FILENO);
                close(mainfile_fd);
            }
            else
            {
                myPrint(error_message);
                exit(0);
            }
            return;
        }

        default:
        {
            myPrint(error_message);
            exit(0);
        }
        }
    }
    return;
}

void run_shell_general(char **aargs, bool redirecting, bool advanced_redir_if_true)
{
    int sub_text;
    char current_dir_retval[2000];
    int temp_reader;
    int stat;
    char reading_buffer[100];
    int mainfile_fd;

    if (!aargs[0])
    {
        return;
    }

    if (!redirecting)
    {
        if (!(strcmp("exit", aargs[0])))
        {
            exit_if_true = !aargs[1] ? true : exit_if_true;

            if (!exit_if_true)
            {
                myPrint(error_message);
                return;
            }
            return;
        }
        if (!(strcmp("cd", aargs[0])))
        {
            int success_stat;

            if (aargs[1])
            {
                success_stat = chdir(aargs[1]);
            }
            else
            {
                success_stat = chdir(getenv("HOME"));
            }

            if (success_stat == -1 || aargs[2])
            {
                myPrint(error_message);
                return;
            }
            else
            {
                return;
            }
        }
        if (!(strcmp("pwd", aargs[0])))
        {
            if (!aargs[1])
            {
                getcwd(current_dir_retval, sizeof(current_dir_retval));
                strcat(current_dir_retval, "\n");
                myPrint(current_dir_retval);
                return;
            }
            else
            {
                myPrint(error_message);
                return;
            }
        }
    }

    int forkret = fork();

    if (forkret > 0)
    {
        waitpid(forkret, &stat, 0);

        sub_text = open("sub_text", O_RDWR | O_CREAT | O_APPEND, 0664);

        if (0 <= sub_text)
        {
            lseek(sub_text, 0, SEEK_SET);

            mainfile_fd = open(redir_file, O_RDWR | O_CREAT | O_APPEND, 0664);
            lseek(mainfile_fd, 0, SEEK_END);

            while ((temp_reader = read(sub_text, reading_buffer, 100)))
            {
                write(mainfile_fd, reading_buffer, temp_reader);
            }

            unlink("sub_text");

            close(sub_text);
            close(mainfile_fd);
        }
    }
    else
    {
        if (forkret < 0)
        {
            myPrint(error_message);
        }
        else
        {
            if (redir_file)
            {
                execute_redir(redir_file, false, advanced_redir_if_true);
            }

            if (execvp(aargs[0], aargs) < 0)
            {
                myPrint(error_message);
            }

            exit(0);
        }
    }
    return;
}

void run_shell_first(int mode, char *batchfile_path)
{
    char *strtok_saveptr;
    char *another_strtok_saveptr;
    char input_str[1000];
    char *r_of_sign;
    char *l_of_sign;
    char *pinput;
    char *temp_token;
    char *copy_for_token_counting;
    FILE *batchfile_fd;

    if (mode == 1)
    {
        batchfile_fd = fopen(batchfile_path, "r");
        if (!batchfile_fd)
        {
            myPrint(error_message);
        }
    }

    while (!exit_if_true)
    {
        int num_redirections = 0;
        int token_counter = 0;
        bool there_was_an_error = false;

        switch (mode)
        {
        case 0:
        {
            myPrint("myshell> ");
            pinput = fgets(input_str, 100, stdin);
            if (pinput && strlen(pinput) >= 515)
            {
                myPrint(error_message);
                continue;
            }
            else if (!pinput)
            {
                exit(0);
            }
        }

        case 1:
        {
            pinput = fgets(input_str, 1000, batchfile_fd);

            bool skip_because_blank = true;

            if (pinput)
            {
                for (int blank_check_index = 0; blank_check_index < strlen(pinput); blank_check_index++)
                {
                    if (!((pinput[blank_check_index] == '\n') || (pinput[blank_check_index] == '\t')) &&
                        pinput[blank_check_index] != ' ')
                    {
                        skip_because_blank = false;
                        break;
                    }
                    else
                    {
                        continue;
                    }
                }

                if (skip_because_blank)
                {
                    continue;
                }

                if (515 <= strlen(pinput))
                {
                    myPrint(input_str);
                    myPrint(error_message);
                    continue;
                }
                else
                {
                    myPrint(input_str);
                }
            }
            else
            {
                exit(0);
            }
        }

        default:
            break;
        }

        char *arg_string = strtok_r(pinput, ";", &strtok_saveptr);

        for (int char_ind = 0; arg_string && arg_string[char_ind] != '\0'; char_ind += 1)
        {
            switch (arg_string[char_ind])
            {
            case '>':
            {
                num_redirections += 1;
                break;
            }
            default:
                continue;
            }
        }

        while (arg_string)
        {

            copy_for_token_counting = strdup(arg_string);

            if (num_redirections < 2)
            {
                switch (num_redirections)
                {
                case 0:
                {

                    for (temp_token = strtok(copy_for_token_counting, " \t\n"); temp_token; token_counter += 1)
                    {
                        temp_token = strtok(NULL, " \t\n");
                    }

                    char **aargs = extract_arguments(arg_string, token_counter);

                    run_shell_general(aargs, false, false);
                    break;
                }
                case 1:
                {

                    bool advanced_redir_if_true = strchr(arg_string, '+') ? true : false;

                    l_of_sign = !advanced_redir_if_true ? strtok_r(arg_string, ">", &another_strtok_saveptr) : strtok_r(arg_string, ">+", &another_strtok_saveptr);
                    r_of_sign = !advanced_redir_if_true ? strtok_r(NULL, ">", &another_strtok_saveptr) : strtok_r(NULL, ">+", &another_strtok_saveptr);

                    arg_string = strdup(l_of_sign);

                    redir_file = r_of_sign ? strtok(r_of_sign, " \t\n") : redir_file;

                    if (!there_was_an_error && (!redir_file))
                    {
                        myPrint(error_message);
                        there_was_an_error = true;
                    }

                    char *temp_token = strtok(l_of_sign, " \t\n");

                    while (temp_token)
                    {
                        temp_token = strtok(NULL, " \t\n");
                        token_counter += 1;
                    }

                    if (!there_was_an_error && token_counter == 1)
                    {
                        myPrint(error_message);
                        there_was_an_error = true;
                    }

                    char **aargs = extract_arguments(arg_string, token_counter);

                    if (!there_was_an_error && (!(aargs[0]) || !(strcmp("exit", aargs[0])) || !(strcmp("cd", aargs[0])) || !(strcmp("pwd", aargs[0]))))
                    {
                        myPrint(error_message);
                        there_was_an_error = true;
                    }

                    if (!there_was_an_error)
                    {
                        run_shell_general(aargs, true, advanced_redir_if_true);
                    }

                    break;
                }
                default:
                {
                    myPrint(error_message);
                    break;
                }
                }
                arg_string = strtok_r(NULL, ";", &strtok_saveptr);
            }
            else
            {
                myPrint(error_message);
                arg_string = strtok_r(NULL, ";", &strtok_saveptr);
            }
        }
    }
}

int main(int argc, char *argv[])
{

    switch (argc)
    {
    case 2:
        run_shell_first(1, argv[1]);
        break;
    case 1:
        run_shell_first(0, NULL);
        break;
    default:
        myPrint(error_message);
    }

    return 1;
}
