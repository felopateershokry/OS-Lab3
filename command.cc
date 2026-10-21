
/*
 * 
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include <glob.h>
#include <wordexp.h>
#include <vector>
#include <string>
#include <cctype>
#include <iostream>
#include <sstream>
#include "command.h"
#include "tokenizer.h"
#include <cerrno>
#include <fstream>
#include <ctime>
#include <glob.h>
#include <readline/readline.h>
#include <readline/history.h>

volatile sig_atomic_t childFinished =0;
void handleSigchld(int){
    childFinished = 1;
}

void logChildTermination(pid_t pid, int status){
    std::ofstream log("process.log", std::ios::app);
    if(!log){
        std::cerr << "cannot open process.log" << std::endl;
        return;
    }
    time_t now = time(NULL);
    log << "Time: " << ctime(&now) << "PID: " << pid;
    
    if(WIFEXITED(status)){
        log << " | Exit code: " << WEXITSTATUS(status);
    }else if(WIFSIGNALED(status)){
        log << " | Signal: " << WTERMSIG(status);
    }
    log << "\n";
}

std::vector<pid_t> backgroundChildren;
void checkBackgroundChildren(){
    for(size_t i = 0; i < backgroundChildren.size();){
        int status = 0;
        pid_t pid = backgroundChildren[i];
        pid_t result = waitpid(pid, &status, WNOHANG);
        if(result == pid){
            logChildTermination(pid, status);
            backgroundChildren.erase(backgroundChildren.begin() + i);
        }else if(result < 0 && errno == ECHILD){
            backgroundChildren.erase(backgroundChildren.begin() + i);
        }
        else{
            ++i;
        }
    }
    childFinished = 0;
}

void insertExpandedArgument(SimpleCommand *simple, const std::string &argument){
    if(argument.find_first_of("*?") == std::string::npos){
        simple->insertArgument(strdup(argument.c_str()));
        return;
    }
    glob_t results = {};
    int status = glob(argument.c_str(), GLOB_NOCHECK, NULL, &results);
    if(status == 0){
        for(size_t i = 0; i < results.gl_pathc; i++){
            simple->insertArgument(strdup(results.gl_pathv[i]));
        }
    }
    else{
        simple->insertArgument(strdup(argument.c_str()));
    }

    globfree(&results);
}

void parse(std::vector<Token> &tokens)
{
    Command currentCommand;
    SimpleCommand *currentSimpleCommand = new SimpleCommand();
    auto cleanup = [&](){
        delete currentSimpleCommand;
        currentCommand.clear();
    };
    for(size_t i = 0; i < tokens.size(); i++){
        Token token = tokens[i];
        if(token.type == TOKEN_EOF){
            break;
        }
        if(token.type == TOKEN_REDIRECT || token.type == TOKEN_APPEND || token.type == TOKEN_INPUT || token.type == TOKEN_ERROR || token.type == TOKEN_REDIRECT_AND_ERROR){
            if(currentSimpleCommand->_numberOfArguments == 0){
                std::cerr << "Syntax error"<< std::endl;
                cleanup();
                return;
            }
        }
        if(token.type == TOKEN_COMMAND || token.type == TOKEN_ARGUMENT){
            insertExpandedArgument(currentSimpleCommand, token.value);
        }
        else if(token.type == TOKEN_PIPE){
            if(currentSimpleCommand->_numberOfArguments == 0){
                std::cerr << "error: unexpected pipe"<< std::endl;
                cleanup();
                return;
            }

            currentCommand.insertSimpleCommand(currentSimpleCommand);
            currentSimpleCommand = new SimpleCommand();
        }
        else if(token.type == TOKEN_REDIRECT){
            if(currentCommand._outFile != NULL){
                std::cerr << "error: duplicate output redirection"<< std::endl;
                cleanup();
                return;
            }
            if(i + 1 >= tokens.size() || (tokens[i + 1].type != TOKEN_ARGUMENT && tokens[i + 1].type != TOKEN_COMMAND)){
                std::cerr << "error: missing output filename"<< std::endl;
                cleanup();
                return;
            }
            i++;
            currentCommand._outFile = strdup(tokens[i].value.c_str());
            currentCommand._append = 0;
        }
        else if(token.type == TOKEN_APPEND){
            if(currentCommand._outFile != NULL){
                std::cerr << "error: duplicate output redirection"<< std::endl;
                cleanup();
                return;
            }
            if(i + 1 >= tokens.size() || (tokens[i + 1].type != TOKEN_ARGUMENT && tokens[i + 1].type != TOKEN_COMMAND)){
                std::cerr << "error: missing append filename"<< std::endl;
                cleanup();
                return;
            }
            i++;
            currentCommand._outFile = strdup(tokens[i].value.c_str());
            currentCommand._append = 1;
        }
        else if(token.type == TOKEN_INPUT){
            if(currentCommand._inputFile != NULL){
                std::cerr << "error: duplicate input redirection"<< std::endl;
                cleanup();
                return;
            }
            if(i + 1 >= tokens.size() || (tokens[i + 1].type != TOKEN_ARGUMENT && tokens[i + 1].type != TOKEN_COMMAND)){
                std::cerr << "error: missing input filename"<< std::endl;
                cleanup();
                return;
            }
            i++;
            currentCommand._inputFile = strdup(tokens[i].value.c_str());
        }
        else if(token.type == TOKEN_ERROR){
            if(currentCommand._errFile != NULL){
                std::cerr << "error: duplicate error redirection"<< std::endl;
                cleanup();
                return;
            }
            if(i + 1 >= tokens.size() || (tokens[i + 1].type != TOKEN_ARGUMENT && tokens[i + 1].type != TOKEN_COMMAND)){
                std::cerr << "error: missing error filename"<< std::endl;
                cleanup();
                return;
            }
            i++;
            currentCommand._errFile = strdup(tokens[i].value.c_str());
        }
        else if(token.type == TOKEN_REDIRECT_AND_ERROR){
            if(currentCommand._outFile != NULL || currentCommand._errFile != NULL){
                std::cerr << "error: duplicate output/error redirection"<< std::endl;
                cleanup();
                return;
            }
            if(i + 1 >= tokens.size() || (tokens[i + 1].type != TOKEN_ARGUMENT && tokens[i + 1].type != TOKEN_COMMAND)){
                std::cerr << "error: missing filename"<< std::endl;
                cleanup();
                return;
            }
            i++;
            currentCommand._outFile = strdup(tokens[i].value.c_str());
            currentCommand._errFile = strdup(tokens[i].value.c_str());
            currentCommand._out_error = 1;
            currentCommand._append = 1;
        }
        else if(token.type == TOKEN_BACKGROUND){
            if(currentSimpleCommand->_numberOfArguments == 0){
                std::cerr << "missing command before &" << std::endl;
                cleanup();
                return;
            }
            if(i + 1 >= tokens.size() || tokens[i + 1].type != TOKEN_EOF){
               std::cerr << "& must be at the end" << std::endl;
                cleanup();
                return; 
            }
            currentCommand._background = 1;
        }
    }
    if(currentSimpleCommand->_numberOfArguments == 0){
        if(currentCommand._numberOfSimpleCommands > 0){
            std::cerr << "missing command after pipe"<< std::endl;
        }
        cleanup();
        return;
    }
    currentCommand.insertSimpleCommand(currentSimpleCommand);
    currentCommand.execute();
}

SimpleCommand::SimpleCommand()
{
    _numberOfAvailableArguments = 5;
    _numberOfArguments = 0;
    _arguments = (char **)malloc(_numberOfAvailableArguments * sizeof(char *));
}

SimpleCommand::~SimpleCommand()
{
    for(int i=0; i<_numberOfArguments; i++){
        free(_arguments[i]);
    }
    free(_arguments);
}

void SimpleCommand::insertArgument(char *argument)
{
    if (_numberOfAvailableArguments == _numberOfArguments + 1)
    {
        _numberOfAvailableArguments *= 2;
        _arguments = (char **)realloc(_arguments,
                                      _numberOfAvailableArguments * sizeof(char *));
    }

    _arguments[_numberOfArguments] = argument;
    _arguments[_numberOfArguments + 1] = NULL;

    _numberOfArguments++;
}

Command::Command()
{
    _numberOfAvailableSimpleCommands = 1;
    _numberOfSimpleCommands = 0;
    _simpleCommands = (SimpleCommand **)
        malloc(_numberOfAvailableSimpleCommands * sizeof(SimpleCommand *));

    
    _outFile = NULL;
    _inputFile = NULL;
    _errFile = NULL;
    _out_error = 0;
    _background = 0;
    _append = 0;
}
Command::~Command(){
    clear();
    free(_simpleCommands);
}
void Command::insertSimpleCommand(SimpleCommand *simpleCommand)
{
    if (_numberOfAvailableSimpleCommands == _numberOfSimpleCommands)
    {
        _numberOfAvailableSimpleCommands *= 2;
        _simpleCommands = (SimpleCommand **)realloc(_simpleCommands,
                                                    _numberOfAvailableSimpleCommands * sizeof(SimpleCommand *));
    }

    _simpleCommands[_numberOfSimpleCommands] = simpleCommand;
    _numberOfSimpleCommands++;
}

void Command::clear()
{
    for (int i = 0; i < _numberOfSimpleCommands; i++)
    {
        delete _simpleCommands[i];
    }

    if (_outFile)
    {
        free(_outFile);
    }

    if (_inputFile)
    {
        free(_inputFile);
    }

    if (_errFile)
    {
        free(_errFile);
    }

    _numberOfSimpleCommands = 0;
    _outFile = 0;
    _inputFile = 0;
    _errFile = 0;
    _background = 0;
    _append = 0;
    _out_error = 0;
}

void Command::print()
{
    printf("\n\n");
    printf("              COMMAND TABLE                \n");
    printf("\n");
    printf("  #   Simple Commands\n");
    printf("  --- ----------------------------------------------------------\n");

    for (int i = 0; i < _numberOfSimpleCommands; i++)
    {
        printf("  %-3d ", i);
        for (int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++)
        {
            printf("\"%s\" \t", _simpleCommands[i]->_arguments[j]);
        }
        printf("\n");
    }

    printf("\n\n");
    printf("  Output       Input        Error        Err&Out       Background\n");
    printf("  ------------ ------------ ------------ ------------ ------------\n");
    printf("  %-12s %-12s %-12s %-12s %-12s\n", _outFile ? _outFile : "default",
           _inputFile ? _inputFile : "default", _errFile ? _errFile : "default", _out_error == 1 ? _errFile : "default"
           ,_background ? "YES" : "NO");
    printf("\n\n");
}

void Command::execute()
{
    if(_numberOfSimpleCommands == 0){
        clear();
        return;
    }
    if(_numberOfSimpleCommands == 1){
        SimpleCommand *simple = _simpleCommands[0];
        if(strcmp(simple->_arguments[0],"exit") == 0){
            printf("Good bye!!\n");
            clear();
            exit(0);
        }
        if(strcmp(simple->_arguments[0],"cd") == 0){

            if(simple->_numberOfArguments > 2){
                std::cerr << "cd: too many arguments" << std::endl;
                clear();
                return;
            }

            const char *path;
            if(simple->_numberOfArguments > 1){
                path = simple->_arguments[1];
            }else{
                path = getenv("HOME");
            }
            if(path == NULL){
                std::cerr << "cd : HOME is not set" << std::endl;
            }else if(chdir(path) < 0){
                perror("cd");
            }
            clear();
            return;
        }
    }
    int prevRead = -1;
    std::vector<pid_t> children;
    bool failed = false;

    for(int i = 0; i < _numberOfSimpleCommands; i++){
        int fd[2] = {-1, -1};
        bool lastCommand = (i == _numberOfSimpleCommands - 1);

        if(!lastCommand && pipe(fd) < 0){
            perror("pipe");
            failed = true;
            break;
        }

        pid_t pid = fork();

        if(pid < 0){
            perror("fork");

            if(fd[0] != -1)
                close(fd[0]);

            if(fd[1] != -1)
                close(fd[1]);

            failed = true;
            break;
        }

        if(pid == 0){
            signal(SIGINT,SIG_DFL);
            if(prevRead != -1){
                if(dup2(prevRead, STDIN_FILENO) < 0){
                    perror("dup2 pipe input");
                    _exit(1);
                }
            }

            if(!lastCommand){
                if(dup2(fd[1], STDOUT_FILENO) < 0){
                    perror("dup2 pipe output");
                    _exit(1);
                }
            }

            if(prevRead != -1)
                close(prevRead);

            if(fd[0] != -1)
                close(fd[0]);

            if(fd[1] != -1)
                close(fd[1]);

            if(i == 0 && _inputFile != NULL){
                int inputFd = open(_inputFile, O_RDONLY);

                if(inputFd < 0){
                    perror("open input");
                    _exit(1);
                }

                if(dup2(inputFd, STDIN_FILENO) < 0){
                    perror("dup2 input");
                    close(inputFd);
                    _exit(1);
                }

                close(inputFd);
            }

            if(lastCommand && _out_error){
                int outputFd = open(_outFile,O_WRONLY | O_CREAT | O_APPEND,0644);

                if(outputFd < 0){
                    perror("open combined output");
                    _exit(1);
                }

                if(dup2(outputFd, STDOUT_FILENO) < 0 || dup2(outputFd, STDERR_FILENO) < 0){
                    perror("dup2 combined output");
                    close(outputFd);
                    _exit(1);
                }

                close(outputFd);
            }
            else{
                if(lastCommand && _outFile != NULL){
                    int flags = O_WRONLY | O_CREAT;

                    if(_append)
                        flags |= O_APPEND;
                    else
                        flags |= O_TRUNC;

                    int outputFd = open(_outFile, flags, 0644);

                    if(outputFd < 0){
                        perror("open output");
                        _exit(1);
                    }

                    if(dup2(outputFd, STDOUT_FILENO) < 0){
                        perror("dup2 output");
                        close(outputFd);
                        _exit(1);
                    }

                    close(outputFd);
                }

                if(_errFile != NULL){
                    int errorFd = open(_errFile,O_WRONLY | O_CREAT | O_TRUNC,0644);

                    if(errorFd < 0){
                        perror("open error");
                        _exit(1);
                    }

                    if(dup2(errorFd, STDERR_FILENO) < 0){
                        perror("dup2 error");
                        close(errorFd);
                        _exit(1);
                    }

                    close(errorFd);
                }
            }

            SimpleCommand *simple = _simpleCommands[i];

            execvp(simple->_arguments[0], simple->_arguments);

            perror(simple->_arguments[0]);
            _exit(127);
        }

        children.push_back(pid);

        if(prevRead != -1)
            close(prevRead);

        if(fd[1] != -1)
            close(fd[1]);

        prevRead = fd[0];
    }

    if(prevRead != -1)
        close(prevRead);

    if(failed){
        for(pid_t childPid : children)
            kill(childPid, SIGTERM);
    }

    if(_background && !failed){
        for(pid_t childPid : children){
            backgroundChildren.push_back(childPid);
        }
    }

    if(!_background || failed){
        for(pid_t childPid : children){
            int status;

            pid_t result;
            do{
                result = waitpid(childPid, &status, 0);
            }while(result < 0 && errno == EINTR);
            if(result == childPid){
                logChildTermination(childPid,status);
            }else if(result < 0 && errno != ECHILD){
                perror("waitpid");
            }
        }
    }

    clear();
}

void Command::prompt()
{
    std::string input;
    while(true){
        if(childFinished || !backgroundChildren.empty()){
            checkBackgroundChildren();
        }
        char *line = readline("myshell> ");
        if(line == NULL){
            break;
        }
        std::string input(line);

        if(!input.empty()){
            add_history(line);
        }
        free(line);

        std::vector<Token> tokens = tokenize(input);
        parse(tokens);
    }
}

Command Command::_currentCommand;
SimpleCommand *Command::_currentSimpleCommand;

int main()
{
    signal(SIGINT,SIG_IGN);

    struct sigaction sa = {};
    sa.sa_handler = handleSigchld;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_NOCLDSTOP | SA_RESTART;

    if(sigaction(SIGCHLD, &sa, NULL) < 0){
        perror("sigaction");
        return 1; 
    }

    Command::_currentCommand.prompt();
    return 0;
}
