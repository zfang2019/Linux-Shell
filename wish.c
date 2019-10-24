//Add your code here
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/wait.h>

char error_message[30] = "An error has occurred\n";
char path[1024] = "/bin";

//Declaration of built-in commands functions
int shell_exit(char **args, int count);
int shell_cd(char **args, int count);
int shell_path(char **args, int count);

//built commands and corresponding function pointer
char *shell_builtin_cmd[] = {
    "exit",
    "cd",
    "path"
};

int (*shell_fp[]) (char **, int) = {
    &shell_exit,
    &shell_cd,
    &shell_path
};

void process_shell_script(char* scriptName,int redirect, char* out_file);
void batch_run(char* filename);
void trim(char *s);

//number of built-in functions
int num_shell_builtin_cmd() {
    return sizeof(shell_builtin_cmd)/sizeof(char*);
}

//builtin function implementation
//exit the shell, send 0 to signal exit
int shell_exit(char **args, int count) {
    if (args[1]==NULL)  return 0;
    else {
        //printf("Error from shell exit\n");
        write(STDERR_FILENO, error_message, strlen(error_message));
        return 1;
    }
}

//change directory, and send 1 so taht shell will continue running
int shell_cd(char **args, int count) { 
    if(args[1]==NULL) {
        //printf("Error from shell cd 1\n");
        write(STDERR_FILENO, error_message, strlen(error_message));
    }
    else if (args[2]==NULL) {
        chdir(args[1]);
        /*
        if (chdir(args[1]) != 0) {
            //printf("Error from shell cd 2\n");
            write(STDERR_FILENO, error_message, strlen(error_message));
            //exit(1);
        }
        else exit(0);
        */
    }
    else {//two many arguments
        //printf("Error from shell cd 3\n");
        write(STDERR_FILENO, error_message, strlen(error_message));
        //exit(0);
    }
    return 1;
}

void generate_abs_path(char *in_path, char *abs_path) {
    if (in_path[0] == '/') return;
    else {
       char temp_path[1024];
       getcwd(temp_path,sizeof(temp_path));
       strcpy(abs_path, temp_path);
       //printf("The abs_path is %s\n", abs_path);
       strcat(abs_path,"/");
       strcat(abs_path,in_path);
       //printf("The abs_path is %s\n", abs_path);
        return ;
    }
}
//change the path
int shell_path(char **args, int count) { 
    int i=1;
    //char temp[1024]="";
    char *temp = (char*)malloc(1024*sizeof(char));

    if (args[1] == NULL) strcpy(path,"");
    else {//>=2 arguments
        strcat(path,":");
        generate_abs_path(args[1],temp);
        strcat(path,temp);
        //free(temp);
        for (i = 2; i < count -1; i++) {
            strcat(path,":");
            generate_abs_path(args[i],temp);
            strcat(path,temp);
            //free(temp);
        }
    }
    //printf("New path is %s\n", path);
    return 1;
}

const char *get_filename_ext(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) return "";
    return dot + 1;
}


//launch a program and wait for it to finish
//args should terminate with NULL
int shell_launchProg(char **args, int redirect, char* out_file) {
     //printf("Current arg0 is %s\n", args[0]);
     //printf("Curent path is %s\n",path);
     //skip comment line
     if(args[0][0]=='#') return 1;

    //add path to the cmd args[0]
    int fd;
    int found_prog_success = 0;
    int is_sh_script = 0;
    char *cmd=(char*)malloc(1024*sizeof(char));
    char* temp;
    char *p_path = (char*)malloc(1024*sizeof(char));
    strcpy(p_path,path);

    //check whether it is a sh script
    //if it is sh file
    const char *sh_file = get_filename_ext(args[0]);
    if (strcmp(sh_file,"sh")==0) {
        is_sh_script = 1;
        //printf("It is a sh file\n");
    }
    
    while ((temp = strsep(&p_path,":")) != NULL) {
        //trim(cmd);
        //printf("Now search path: %s\n",temp);
        strcpy(cmd,temp);
        strcat(cmd,"/");
        strcat(cmd,args[0]);
        //printf("Now search command: %s\n",cmd);
        if (is_sh_script==1) {
            fd = access(cmd, F_OK);
        }
        else {
            fd = access(cmd, X_OK);
        }
        if(fd!=-1) {
            found_prog_success = 1;
            //exit
            //printf("Current cmd is %s\n", cmd);
            break;
        }
    }
    if (found_prog_success == 0) {
        //printf("Error from shell launch program 1\n");
        write(STDERR_FILENO, error_message, strlen(error_message));
        return 1;
    }
    /*
    strcpy(cmd,path);
    strcat(cmd,"/");
    strcat(cmd,args[0]);
    fd = access(cmd, X_OK);
    if (fd==-1)  {
            //printf("Cannot find the command %s\n",cmd);
            write(STDERR_FILENO, error_message, strlen(error_message));
            return 1;
    }
    */
   
    //if it is sh file
    //const char *sh_file = get_filename_ext(cmd);
    if (is_sh_script == 1) {
        //printf("Excuet this script %s\n",cmd);
        char *scriptName = (char*)malloc(1024*sizeof(char));
        strcpy(scriptName,cmd);
        process_shell_script(scriptName, redirect, out_file);
        return 1;
    }
    
    //implement redirection
    //int redirection = 0;
    /*
    while(args) {
        printf("The arguemnts are %s\n",*(args+i));
        //if(*(args+i)[0] == '>') printf("There is redirection\n");
        i++;
    }
    printf("There is no redirection\n");
    */
    
    //printf("Current real cmd is %s\n", cmd);
    pid_t pid;
    //int status;

    pid = fork();
    //execute child process
    if (pid == 0) {
        if (redirect) {
            close(STDOUT_FILENO);
            open(out_file, O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU);
            execv(cmd,args);
        }
        else execv(cmd,args);
        /*
        if (execv(cmd,args) == -1) {
            write(STDERR_FILENO, error_message, strlen(error_message));
        }
        exit(EXIT_FAILURE);
        */
    }
    else if (pid < 0 ) { //Forking error
        //printf("Error from shell launch program 2\n");
        write(STDERR_FILENO, error_message, strlen(error_message));
    }
    else { //parent process
        waitpid(pid, NULL, 0);
        /*
        do {
            waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
        */
    }

    return 1;
}

//execute a command
int shell_execute(char** args, int count, int redirect, char* out_file) {
    int i;

    if (args[0]==NULL) {//empty command
        return 1;
    }

    //run builtin command
    for ( i = 0; i < 3; i++)  {
        if (strcmp(args[0],shell_builtin_cmd[i]) == 0) {
            return (*shell_fp[i])(args,count);
        }
    }

    return shell_launchProg(args,redirect, out_file);
}

void trim(char *s) {
    if (strcmp(s,"")==0) return;

    char *p = s;
    int len = strlen(p);

    while(isspace(p[len-1])) p[--len] = 0;
    while(*p && isspace(*p))  {
        ++p;
        --len;
    }
    memmove(s,p, len+1);
}

char* read_line(void){
    size_t line_size = 1024;
    char *line = (char*)malloc(line_size*sizeof(char));
    if (line==NULL) {
        //printf("Error from read line program 1\n");
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1);
    }

    if  ( getline(&line, &line_size, stdin) >= 0 ) {
         trim(line);
         size_t length = strlen(line);
         if(line[length-1]=='\n' || line[length-1]==EOF) line[length-1] = '\0';
         //printf("The line is %s\n", line);
         return line;
    }
    else exit(0);
}

//parse the line into several commands
char** parse_commands(char *line, int *count) {
    //int j;
    size_t commands_num = 1024;
    size_t command_length = 256;
    char  **commands = (char**)malloc(commands_num*sizeof(char*));
    if (commands == NULL ) {
        //printf("Error from parse_commands 1\n");
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1); 
    }

    *count = 0;
    commands[*count] = (char*)malloc(command_length*sizeof(char));
    if (commands[*count] == NULL)  {
        //printf("Error from parse_commands 2\n");
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1); 
    }
    while ( (commands[*count] = strsep(&line,"&")) != NULL)  {
        trim(commands[*count]);
        if (strcmp(commands[*count],"") !=0) {
            (*count)++;
            commands[*count] = (char*)malloc(command_length*sizeof(char));
            if (commands[*count] == NULL)  {
                //printf("Error from parse_commands 3\n");
                write(STDERR_FILENO, error_message, strlen(error_message)); 
                exit(1);
            }
        }
    }
    /*
    printf("There are %d commands\n",*count);
    for (j = 0; j < *count; j++)  printf("The commands are %s\n",commands[j]);
    */
    return commands;
}


//parse each commands into arguments
char** parse_redirections(char *command, int *count, char* sep) {
    //int j;
    size_t argv_num = 256;
    size_t argv_length = 100;
    char  **arguments = (char**)malloc(argv_num*sizeof(char*));
    if (arguments == NULL ) {
        //printf("Error from parse_redirections 1\n");
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1); 
    }

    //process real_comand
    *count = 0;
    arguments[*count] = (char*)malloc(argv_length*sizeof(char));
    if (arguments[*count] == NULL)  {
        //printf("Error from parse_redirections 2\n");
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1); 
    }
    while ( (arguments[*count] = strsep(&command,sep)) != NULL)  {
        //trim(arguments[*count]);
        
        //if (!isspace(arguments[*count][0])  && (arguments[*count][0] != '\0') ) {
            (*count)++;
            arguments[*count] = (char*)malloc(argv_length*sizeof(char));
            if (arguments[*count] == NULL)  {
                //printf("Error from parse_redirections 3\n");
                write(STDERR_FILENO, error_message, strlen(error_message)); 
                exit(1);
            //}
        }
    }
    arguments[*count]=(char*)NULL;
    (*count)++;

    /*
    for (j = 0; j < *count; j++)
        printf("The arguments are %s\n", arguments[j]);
    */
    return arguments;
}

//parse each commands into arguments
char** parse_arguments(char *command, int *count, char* sep) {
    //int j;
    size_t argv_num = 256;
    size_t argv_length = 100;
    char  **arguments = (char**)malloc(argv_num*sizeof(char*));
    if (arguments == NULL ) {
        //printf("Error from parse_arguments 1\n");
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1); 
    }

    //process real_comand
    *count = 0;
    arguments[*count] = (char*)malloc(argv_length*sizeof(char));
    if (arguments[*count] == NULL)  {
        //printf("Error from parse_arguments 2\n");
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1); 
    }
    while ( (arguments[*count] = strsep(&command,sep)) != NULL)  {
        trim(arguments[*count]);
        
        if (!isspace(arguments[*count][0])  && (arguments[*count][0] != '\0') ) {
            (*count)++;
            arguments[*count] = (char*)malloc(argv_length*sizeof(char));
            if (arguments[*count] == NULL)  {
                //printf("Error from parse_arguments 3\n");
                write(STDERR_FILENO, error_message, strlen(error_message)); 
                exit(1);
            }
        }
    }
    arguments[*count]=(char*)NULL;
    (*count)++;

    return arguments;
}

//shell runs in interactive mode
void interactive_loop() {
    char *line;
    char **commands;
    char **redirections;
    char **arguments;
    char **outfiles;
    int commands_per_line;
    int i;
    int shell_continue = 1;
    
    do  {
        printf("Wish> ");
        line = read_line();
        //skip # line
        if (line[0]=='#') continue;
        commands = parse_commands(line, &commands_per_line);
        i = 0;
        while ( (i < commands_per_line) & shell_continue) {
            trim(commands[i]);
            int redirect_count;
            redirections = parse_redirections(commands[i],&redirect_count,">");
            //printf("There are %d directions\n",redirect_count);
            //printf("second argument is %s\n",redirections[redirect_count-1]);
            if (redirect_count > 3) {//too mant redirection symbol
                //printf("Error from interactive_loop 1\n");
                write(STDERR_FILENO, error_message, strlen(error_message)); 
                //exit(1);
            }
            else if (redirect_count == 2) {
                trim(redirections[0]);
                if (strcmp(redirections[0],"")==0) {
                    //printf("Error from interactive_loop 2\n");
                    write(STDERR_FILENO, error_message, strlen(error_message));
                }
                else {
                    int arg_count;
                    arguments = parse_arguments(redirections[0], &arg_count, " ");
                    /*
                    for (j=0; j <count; j++)
                    printf("The arguments are %s\n", arguments[j]);
                    */
                    shell_continue = shell_execute(arguments,arg_count,0,"");
                }
                //free(commands[i]);
                //free(arguments);
            }
            else {//just one redirect redirect_count == 3 due to NULL as last
                trim(redirections[0]);
                trim(redirections[1]);
                
                //printf("redirection 0 is %s \n", redirections[0]);
                //printf("redirection 1 is %s \n", redirections[1]);
                int out_file_no;
                outfiles = parse_arguments(redirections[1], &out_file_no, " ");
                  //printf("There are %d output files \n", out_file_no);
                if (strcmp(redirections[0],"")==0) {
                    //printf("Error from interactive_loop 3\n");
                    write(STDERR_FILENO, error_message, strlen(error_message));
                }
                else if (strcmp(redirections[1],"")==0 || redirections[1]==NULL  ) {
                    //printf("Error from interactive_loop 4\n");
                    write(STDERR_FILENO, error_message, strlen(error_message));
                }
                else if (out_file_no > 2) {//more than 1 outfile given
                    //printf("Error from interactive_loop 5\n");
                    write(STDERR_FILENO, error_message, strlen(error_message));
                }
                else {//just one output file
                    int arg_count;
                    arguments = parse_arguments(redirections[0], &arg_count, " ");
                    shell_continue = shell_execute(arguments,arg_count,1,outfiles[0]);
                }
            }
            i++;
        }
        //free(line);
        //for (i = 0; i < commands_per_line; i++) {
            //free(commands[i]);
        //}
        //free(commands);
    } while (shell_continue);
}

void process_shell_script(char* scriptName,int redirect, char* out_file) {
    size_t line_size = 1024;
    char *line = (char*)malloc(line_size*sizeof(char));
    if (line==NULL) {
        //printf("Error from batch run 1\n");
         write(STDERR_FILENO, error_message, strlen(error_message));
         exit(1);
    }

    char **arguments;

    FILE  *in_fp = fopen(scriptName, "r");
    if (in_fp == NULL) {//cannot open script file
        //printf("Cannot open the script file %s\n",filename);
        //printf("Error from batch run 2\n");
        write(STDERR_FILENO, error_message, strlen(error_message)); 
        exit(1);
    }
    
    while (getline(&line, &line_size, in_fp) >=0 )  {
        //printf("\nCurrent process this line: %s\n", line);
        trim(line);
        if (line[0]=='#') continue;
        size_t length = strlen(line);
        if(line[length-1]=='\n' || line[length-1]==EOF) line[length-1] = '\0';
        int arg_count;
        arguments = parse_arguments(line, &arg_count, " ");
        shell_execute(arguments,arg_count,redirect,out_file);
    }
    fclose(in_fp);
    free(line);
    //printf("The script has been processed\n");
    //free()
}


void batch_run(char* filename) {
    size_t line_size = 1024;
    char *line = (char*)malloc(line_size*sizeof(char));
    if (line==NULL) {
        //printf("Error from batch run 1\n");
         write(STDERR_FILENO, error_message, strlen(error_message));
         exit(1);
    }

    char **commands;
    char **redirections;
    char **arguments;
    char **outfiles;
    int commands_per_line;
    int i;

    FILE  *in_fp = fopen(filename, "r");
    if (in_fp == NULL) {//cannot open script file
        //printf("Cannot open the script file %s\n",filename);
        //printf("Error from batch run 2\n");
        write(STDERR_FILENO, error_message, strlen(error_message)); 
        exit(1);
    }
    
    while (getline(&line, &line_size, in_fp) >=0 )  {
        //printf("\nCurrent process this line: %s\n", line);
        trim(line);
        if (line[0]=='#') continue;
        size_t length = strlen(line);
        if(line[length-1]=='\n' || line[length-1]==EOF) line[length-1] = '\0';
        commands = parse_commands(line, &commands_per_line);
        i = 0;
        while ( i < commands_per_line ) {
           trim(commands[i]);
           int redirect_count;
            redirections = parse_redirections(commands[i],&redirect_count,">");
            if (redirect_count > 3) {//too mant redirection symbol
                //printf("Too many redirection symbol\n");
                //printf("Error from batch run 3\n");
                write(STDERR_FILENO, error_message, strlen(error_message)); 
                //exit(1);
            }
            else if (redirect_count == 2) {
                trim(redirections[0]);
                if (strcmp(redirections[0],"")==0) {
                    //printf("Empty command\n");
                    //printf("Error from batch run 4\n");
                    write(STDERR_FILENO, error_message, strlen(error_message));
                }
                else {
                    int arg_count;
                    arguments = parse_arguments(redirections[0], &arg_count, " ");
                    shell_execute(arguments,arg_count,0,"");
                }
                //free(commands[i]);
                //free(arguments);
            }
            else {//just one redirect
                trim(redirections[0]);
                trim(redirections[1]);
                int out_file_no;
                outfiles = parse_arguments(redirections[1], &out_file_no, " ");
                if (strcmp(redirections[0],"")==0) {
                    //printf("Error from batch run 5\n");
                    write(STDERR_FILENO, error_message, strlen(error_message));
                }
                else if (strcmp(redirections[1],"")==0 || redirections[1]==NULL  ) {
                    //printf("Error from batch run 6\n");
                    write(STDERR_FILENO, error_message, strlen(error_message));
                }
                else if (out_file_no > 2) {//more than 1 outfile given
                    //printf("Error from batch run 7\n");
                    write(STDERR_FILENO, error_message, strlen(error_message));
                }
                else {//just one output file
                    int arg_count;
                    arguments = parse_arguments(redirections[0], &arg_count, " ");
                    shell_execute(arguments,arg_count,1,outfiles[0]);
                }
            }
            i++;
        }
    }
    fclose(in_fp);
    free(line);
    //printf("The script has been processed\n");
    //free()
}

int main(int argc, char *argv[]) {
    if (argc ==1 ) {
        interactive_loop();
    }
    else if (argc ==2) {
        //run in batch mode
        batch_run(argv[1]);
    }
    else {
        //printf("Error from batch main\n");
        write(STDERR_FILENO, error_message, strlen(error_message));
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}