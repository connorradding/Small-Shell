#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <unistd.h>     
#include <sys/wait.h>   
#include <sys/types.h>
#include <signal.h>
#include <fcntl.h> 
#include <errno.h>
#include <ctype.h>

int flag = 0;

struct user_input{
  char* command;
  char* args[512];
  int arg_int;
  char* input;
  char* output;
  int background;
};

struct children_info{
  int status;
  int signal;
  int stat_sig; //0 for a status exit ans 1 for a signal exit
  pid_t child_arr[100];
};

//signal handler for SIGTSTP
void handle_SIGTSTP(int signo){
  //if the flag is 0, changes it to 1 and informs the user that & is ignored
  if(flag == 0){
    char* message = "\nEntering foreground-only mode (& is now ignored)\n: ";
    write(1, message, strlen(message));
    flag = 1;
  }
  //if flag is 1, changes it to 0 and informs the user the & is accepted
  else{
    char* message = "\nExiting foreground-only mode\n: ";
    write(1, message, strlen(message));
    flag = 0;
  }
  
}

//This function finds a leading ampersand and removes it so the program doesn't have
//to worry about if an ampersand is at the end or the middle of an input
void remove_ampersand(struct user_input* input_struct, char* str){
  //trims any trailing white spaces, inspired by code found online
  int end = strlen(str) - 1;
  while (end >= 0 && isspace((unsigned char)str[end])) {
      end--;
  }
  str[end + 1] = '\0';

  //checks if the last char is an ampersand, and removes it if it is
  int len = strlen(str);
  if (len > 0 && str[len - 1] == '&') {
    input_struct->background = 1; //records that this will be a background process
    str[len - 1] = '\0'; 
  }
}

//inspired by stack overflow but my own code
//find the $$ pattern and replaces it with pid
void variable_expansion(char* input) {
  char* pattern = "$$";
  char pid[20]; //can hold any pid
  sprintf(pid, "%d", getpid()); //convert pid to string

  char* possition = input;
  while ((possition = strstr(possition, pattern)) != NULL) {
    //move the rest of the string to make space for the pid
    memmove(possition + strlen(pid), possition + 2, strlen(possition + 2) + 1);
    //copy the pid in place of $$
    memcpy(possition, pid, strlen(pid));
    possition += strlen(pid);
  }
}

int get_input(struct user_input* input_struct){

  ssize_t nread;
  size_t len = 0;
  char* input_line = NULL;
  char* token;
  fflush(stdout);
  //prompts the user for a command
  printf(": ");
  nread = getline(&input_line, &len, stdin);

  fflush(stdout);

  variable_expansion(input_line);

  //removes any trailing ampersands and updates the struct
  remove_ampersand(input_struct, input_line);

  //reads each string with a space as the delimiter
  token = strtok(input_line, " ");
  //if the user entered nothing or a #, it reprompts the user
  if(token == NULL || strstr(token, "#") != NULL){
    return 1;
  }
  else{
    input_struct->command = strdup(token);
    while(token != NULL){
      token = strtok(NULL, " ");
      //if token is NULL, return function since theres nothing left to read
      if(token == NULL){
        break;
      }

      //if input redirection symobol is found, it sets the field input to the next string
      else if(strcmp(token, "<") == 0){
        token = strtok(NULL, " ");
        if(token != NULL){          //makes sure that token isn't NULL before duping it
          input_struct->input = strdup(token);
        }
      }
      
      //if output redirection symobol is found, it sets the field input to the next string
      else if(strcmp(token, ">") == 0){
        token = strtok(NULL, " ");
        if(token != NULL){          //makes sure that token isn't NULL before duping it
          input_struct->output = strdup(token);
        }
      }

      //if nothing else applies and token is not null, it is an argument and put into the argument array
      else if(token != NULL && input_struct->arg_int < 513){
        input_struct->args[input_struct->arg_int] = strdup(token);
        input_struct->arg_int++;
      }
    }
  }
  return 0;
}

void initialize_user_input(struct user_input* input_struct){
  
  //sets all fields to NULL
  input_struct->command = NULL;
  for (int i = 0; i < 512; i++) {
    input_struct->args[i] = NULL;
  }
  input_struct->arg_int = 0;
  input_struct->input = NULL;
  input_struct->output = NULL;
  input_struct->background = 0;
}

void reset_user_input(struct user_input* input_struct){
  //frees memory of each field if they exist, and returns some fields to 0
  if(input_struct->command != NULL){
    free(input_struct->command);
    input_struct->command = NULL;
  }
  for (int i = 0; i < 512; i++) {
    if(input_struct->args[i] != NULL){
      free(input_struct->args[i]);
    }
    input_struct->args[i] = NULL;
  }
  input_struct->arg_int = 0;
  if(input_struct->input != NULL){
    free(input_struct->input);
    input_struct->input = NULL;
  }
  if(input_struct->output != NULL){
    free(input_struct->output);
    input_struct->output = NULL;
  }
  input_struct->background = 0;
}

void initialize_children_info(struct children_info* children_info){
  //sets all fields to 0
  children_info->status = 0;
  children_info->signal = 0;
  children_info->stat_sig = 0; //0 for a status exit and 1 for a signal exit
  for (int i = 0; i < 100; i++) {
    children_info->child_arr[i] = 0;
  }
}

void exec_function(struct user_input* input_struct, struct children_info* children_info){

  //creates the argument array for the exec() function, adding two spaces for the command and NULL
  char* argument[input_struct->arg_int + 2];

  argument[0] = input_struct->command;  //first in the array is the command
  argument[input_struct->arg_int + 1] = NULL; //last in the array is NULL terminator

  for(int i = 1; i <= input_struct->arg_int; i++){
    argument[i] = input_struct->args[i - 1];
  }

  int child_status;
  int status;

  pid_t spawn_pid = fork();

  if(spawn_pid == -1){
    perror("fork()\n");
  }
  else if(spawn_pid == 0){
    //child process
    
    //if this a foreground process
    if(input_struct->background == 0 || flag == 1){   //if the flag is 1, & are ignored
      //resets the SIGINT signal back to its original behavior
      struct sigaction SIGINT_action = {0};
      SIGINT_action.sa_handler = SIG_DFL; // Set to default handler
      sigaction(SIGINT, &SIGINT_action, NULL);

      //if there is an input file
      if (input_struct->input != NULL) {
        int inputFileDescriptor = open(input_struct->input, O_RDONLY);  //open input file with read only
        if (inputFileDescriptor < 0) {
            perror("Failed to open input file");
            exit(1); //exit child process if opening file fails
        }
        if (dup2(inputFileDescriptor, 0) < 0) { //duplicate file descriptor
            perror("Failed to redirect standard input");
            exit(1);
        }
        close(inputFileDescriptor); //close the original file descriptor
      }

      //if there is an output file
      if (input_struct->output != NULL) {
        int outputFileDescriptor = open(input_struct->output, O_WRONLY | O_CREAT | O_TRUNC, 0664);  //open output file descriptor
        if (outputFileDescriptor < 0) {
            perror("Failed to open output file");
            exit(1); //exit child process if opening file fails
        }
        if (dup2(outputFileDescriptor, STDOUT_FILENO) < 0) {  //duplicate file descriptor
            perror("Failed to redirect standard output");
            exit(1);
        }
        close(outputFileDescriptor); //close the original file descriptor
      }

      //execs to a new process, making sure to exit if there is an error
      execvp(argument[0], argument);
      perror("execvp");
      exit(EXIT_FAILURE);
    }
    //if this is a background process
    else{

      // Open /dev/null for reading and writing
      int devNullInput = open("/dev/null", O_RDONLY);
      int devNullOutput = open("/dev/null", O_WRONLY);

      if (devNullInput < 0 || devNullOutput < 0) {
          perror("Failed to open /dev/null");
          exit(1); //exit if files can't be opened for whatever reason
      }

      //redirect standard input to /dev/null if no input redirection is provided
      if (input_struct->input == NULL) {
          if (dup2(devNullInput, STDIN_FILENO) < 0) {
              perror("Failed to redirect standard input to /dev/null");
              exit(1);  //exit if files can't be opened for whatever reason
          }
      }

      //redirect standard output to /dev/null if no output redirection is provided
      if (input_struct->output == NULL) {
          if (dup2(devNullOutput, STDOUT_FILENO) < 0) {
              perror("Failed to redirect standard output to /dev/null");
              exit(1);
          }
      }

      close(devNullInput);  //close the original file descriptors
      close(devNullOutput);

      execvp(argument[0], argument); //exec into the new program
      perror("execvp");
      exit(EXIT_FAILURE);
    }
  }
  else{
    //parent process

    //forground process
    if(input_struct->background == 0 || flag == 1){
      spawn_pid = waitpid(spawn_pid, &child_status, 0);

      //if the child exited normal
      if(WIFEXITED(child_status)){
        children_info->status = WEXITSTATUS(child_status);  //records the status number it exited with
        children_info->stat_sig = 0;  //records that it was not killed by a signal
      }
      //if the child exited abnormally
      else if(WIFSIGNALED(child_status)){
        children_info->signal = WTERMSIG(child_status); //records the signal it was killed with
        children_info->stat_sig = 1;  //records that it was killed by a signal
      }
    }
    
    //background process
    else if(input_struct->background == 1){

      printf("background pid is %d\n", spawn_pid);   //prints pid of background process to user

      for(int i = 0; i < 100; i++){  //looping through background process array to find an empty spot
        if(children_info->child_arr[i] == 0){ 
          children_info->child_arr[i] = spawn_pid;  //once it finds an empty slot, it puts the child pid into the array
          break;
        }
      }
    }
  }
}

void check_backgroud_processes(struct children_info* children_info){

  pid_t child_pid;
  int child_status;
  //loop through children array and check if they have been terminated
  for(int i; i < 100; i++){
    if(children_info->child_arr[i] != 0){ //if there is a child pid in the array slot
      if(child_pid = waitpid(children_info->child_arr[i], &child_status, WNOHANG) != 0){ //if the child was terminated
        
        //if the child exited normally
        if(WIFEXITED(child_status)){
          printf("background pid %d is done: exit value %d\n", children_info->child_arr[i], WEXITSTATUS(child_status));  //prints the exit value and pid to the user
        }
        //if the child exited abnormally
        else if(WIFSIGNALED(child_status)){
          printf("background pid %d is done: terminated by signal %d\n", children_info->child_arr[i], WTERMSIG(child_status)); //prints the terminating signal and pid to the user
        }
        
        children_info->child_arr[i] = 0;  //resets array slot back to 0
      }
    }
  }
}

void print_status(struct children_info* children_info){

  if(children_info->stat_sig == 0){ //if the last forground process return a status number
    printf("exit value %d\n", children_info->status);
  }
  else if(children_info->stat_sig == 1){ //if the last forground process return a status number
    printf("terminated by signal %d\n", children_info->signal);
  }  
}

void exit_terminal(struct user_input* input_struct, struct children_info* children_info){

  //frees everything in input_struct
  free(input_struct->command);
  for (int i = 0; i < input_struct->arg_int; i++) {
      free(input_struct->args[i]);
  }
  free(input_struct->input);
  free(input_struct->output);
  //frees input_struct itself
  free(input_struct);

  //kills all background child processes
  for(int i; i < 100; i++){
    if(children_info->child_arr[i] != 0){ //if there is a child pid in the array slot
      kill(children_info->child_arr[i], SIGTERM);
    }
  }

  //frees children info struct
  free(children_info);

  //exits the program  
  exit(0);
}

int main(){
  //sets it so that SIGINT is ignored, this will also be applied to children
  struct sigaction SIGINT_action = {0};
  SIGINT_action.sa_handler = SIG_IGN;
  sigaction(SIGINT, &SIGINT_action, NULL);

  //sets it so that SIGTSTP will instead call the signal handler "handle_SIGTSTP()"
  struct sigaction SIGTSTP_action = {0};
  SIGTSTP_action.sa_handler = handle_SIGTSTP;
  sigfillset(&SIGTSTP_action.sa_mask);
  SIGTSTP_action.sa_flags = SA_RESTART;
  sigaction(SIGTSTP, &SIGTSTP_action, NULL);

  //initializing struct that will hold all the information about the users input
  struct user_input* input_struct = malloc(sizeof(struct user_input));
  initialize_user_input(input_struct);

  //initializing struct that will hold all relevant info on child processes
  struct children_info* children_info = malloc(sizeof(struct children_info));
  initialize_children_info(children_info);

  int input_result;
  while(1){
    //checks all the background processes to see if they are terminated
    check_backgroud_processes(children_info);
    //reseting struct to NULL values
    reset_user_input(input_struct);

    //getting the input from the user
    input_result = get_input(input_struct);
    //if the user didn't enter anything or a #, user gets reprompted
    if(input_result == 1){
      continue;
    }
    //if the command is cd
    if(strcmp(input_struct->command, "cd") == 0){
      if(input_struct->args[0] != NULL){ //checks if there is an argument after cd
        if(chdir(input_struct->args[0]) != 0){ //changes the directory to the argument
          perror("chdir\n");
          continue;
        }
      }
      else{
        char* home_dir = getenv("HOME"); //if there is no argument, it changes to the home directory
        if(chdir(home_dir) != 0){
          perror("chdir\n");
          continue;
        }
      }
    }
    //if the command is status
    else if(strcmp(input_struct->command, "status") == 0){
      print_status(children_info);
    }
    //if the command is exit
    else if(strcmp(input_struct->command, "exit") == 0){
      exit_terminal(input_struct, children_info);
    }
    else{
      exec_function(input_struct, children_info);
    }
  }
}