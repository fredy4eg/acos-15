#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#define _BSD_SOURCE

extern char** environ;

int isRef(size_t parseCounter, char* c){
    if ((c[0] == '<' && parseCounter == 0) ||
        (c[0] == 'a' && parseCounter == 1) ||
        (c[0] == ' ' && parseCounter == 2) ||
        (c[0] == 'h' && parseCounter == 3) ||
        (c[0] == 'r' && parseCounter == 4) ||
        (c[0] == 'e' && parseCounter == 5) ||
        (c[0] == 'f' && parseCounter == 6) ||
        (c[0] == '=' && parseCounter == 7) ||
        (c[0] == '"' && parseCounter == 8) ||
        (c[0] == '"' && parseCounter == 9))
    {
        return 1;
    }
    return 0;
}

int main(int argc, char* argv[]) {
    
    ///то, что делает баш<
    int input_pipe;
    if((input_pipe = open(getenv("FIFONAME"), O_RDONLY)) < 0){
        printf("error opening fifo");
        exit(1);
    }
    //>
    
    int bashPipe[2];// общение баша с программой
    pipe(bashPipe);
    fcntl(bashPipe[0], F_SETFL, O_NONBLOCK);
    fcntl(bashPipe[1], F_SETFL, O_NONBLOCK);
    
    char currentURL[256]; // текущий считываемый url
    char* args[] = {"/usr/bin/curl", currentURL, "--silent", NULL};
    char* c = (char*)malloc(2*sizeof(char));
    char* parsing = (char*)malloc(sizeof(char)*2);
    strcpy(parsing, "");
    size_t parseSize = 0;
    size_t parseCounter = 0;

    while (read(input_pipe, currentURL, 256) > 0){
        pid_t pid = fork();
        if (pid == 0){
            argv[1] = currentURL;
            if (dup2(bashPipe[1], STDOUT_FILENO) < 0){ // перенаправление стандартного вывода в программу
                printf("error dupling stdout");
                exit(1);
            }
            execv(args[0], args);
            exit(1);
        }
        else{
            printf("\n\n-----SITE:%s\n", currentURL);
            while(wait(0) > 0);
            while (read(bashPipe[0], &(*c), 1) > 0){
                if (isRef(parseCounter, c) == 1){
                    ++parseCounter;
                    parseSize = 0;
                }
                else if (parseSize == 0)
                    parseCounter = 0;
                    //++parseCounter;
                if (parseCounter == 9){
                    parseSize++;
                    parsing = realloc(parsing, parseSize+2);

                    if (c[0] != '"'){
                        strcat(parsing, c);
                    }
                }
                if (parseCounter == 10){
                    parseCounter = 0;
                    parseSize = 0;
                    /* не совсем понял: нужно выводить только http(s) ссылки
                        или вообще всё, что стоит в теге href. Во втором случае надо
                        просто закомментить if ниже, оставив printf
                        */
                    if (strncmp(parsing, "http://", 7) == 0 || strncmp(parsing, "https://", 8) == 0)
                            printf("%s\n", parsing);
                    strcpy(parsing, "");
                }
            }
        }
    }
    free(c);
    free(parsing);
    return 0;
}
