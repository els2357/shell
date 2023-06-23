// The MIT License (MIT)
// 
// Copyright (c) 2016 Trevor Bakker 
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>

#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
                                // so we need to define what delimits our tokens.
                                // In this case  white space
                                // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255    // The maximum command-line size

#define MAX_NUM_ARGUMENTS 11     // Mav shell only supports 10 arguments

int main()
{

  char * command_string = (char*) malloc( MAX_COMMAND_SIZE );

  //create history arrays that can hold up to 15 previous commands and their associated pids
  char history[15][MAX_COMMAND_SIZE];
  int pids[15];

  int count = 0;

  while( 1 )
  {
    // Print out the msh prompt
    printf ("msh> ");

    // Read the command from the commandline.  The
    // maximum command that will be read is MAX_COMMAND_SIZE
    // This while command will wait here until the user
    // inputs something since fgets returns NULL when there
    // is no input

    fflush(stdin);
    while( !fgets (command_string, MAX_COMMAND_SIZE, stdin) );
        
    /* Parse input */
    char *token[MAX_NUM_ARGUMENTS];

    for( int i = 0; i < MAX_NUM_ARGUMENTS; i++ )
    {
      token[i] = NULL;
    }

    int   token_count = 0;                                 
                                                           
    // Pointer to point to the token
    // parsed by strsep
    char *argument_ptr = NULL;                                         
                                                           
    char *working_string  = strdup( command_string );                

    if ((working_string[0] == '\n') || (working_string[0] == '\0') || (working_string == NULL))
    {
      continue;
    }

    // Add command to history array if less than 15 commands have been entered
    if (count < 15)
    {
      char *buffer = working_string;
      buffer[strcspn(buffer, "\n")] = 0;
      strcpy(history[count], buffer);
      pids[count] = getpid();
      count++;
    }
    
    // If more than 15 commands are entered, only last 15 are remembered
    else 
    {
      for (int i = 0; i<14 ; i++)
      {
        strcpy(history[i], history[i+1]);
        pids[i] = pids[i+1];
      }
      char *buffer = working_string;
      buffer[strcspn(buffer, "\n")] = 0;
      strcpy(history[14], buffer);
      pids[14] = getpid();
    }

    // Checks if entry begins with !, is followed by an int, and the int is 2 digits or less
    if ((command_string != NULL) && (command_string[0] == '!') && (isdigit(command_string[1])) && (strlen(command_string) < 4))
    {
      
      // Converts first digit of n to an integer
      int index = command_string[1] - '0';

      // If n is a double digit number update index
      if (isdigit(command_string[2]))
      {
        index = ((index*10)+(command_string[2] - '0'));
      }
      
      // Checks to see if index is in valid range
      if (index < 15)
      {
        
        // If n is higher than the number of commands in history, prints an error and continues the loop
        if (index >= count)
        {
          printf("Command not in history.\n");
          continue;
        }

        // TODO: Else, run nth command
        else
        {
          strcpy(working_string, history[index]);
        }
      }

      // If n is greater than 14, prints an error. 
      else
      {
        printf("Command not in history.\n");
        continue;
      }
    }

    // we are going to move the working_string pointer so
    // keep track of its original value so we can deallocate
    // the correct amount at the end
    char *head_ptr = working_string;

    // Tokenize the input strings with whitespace used as the delimiter
    while ( ( (argument_ptr = strsep(&working_string, WHITESPACE ) ) != NULL) && 
              (token_count<MAX_NUM_ARGUMENTS))
    {
      token[token_count] = strndup( argument_ptr, MAX_COMMAND_SIZE );
      if( strlen( token[token_count] ) == 0 )
      {
        token[token_count] = NULL;
      }
        token_count++;
    }

    // If first argument is quit or exit, return with 0 status
    if ( token[0] != NULL && !(strcmp(token[0], "quit")) )
    {
      _exit(0);
    }

    else if ( token[0] != NULL && !(strcmp(token[0], "exit")) )
    {
      _exit(0);
    }

    // If first argument is cd, stay in parent and execute cd
    if ( token[0] != NULL && !(strcmp(token[0], "cd")) )
    {
      if (token[1] != NULL)
      {
        chdir(token[1]);
      }
    }
    
    // If first argument is history, stay in parent and print history of 15 latest commands
    else if ( token[0] != NULL && !(strcmp(token[0], "history")) )
    {
      
      // TODO: If argument contains -p, print PID with history
      if (token[1] != NULL && !(strcmp(token[1], "-p")))
      {
        for( int i = 0; i < count; i++ )
        {
          if( history[i] != NULL )
          {
            printf("%d: %s %d \n", i, history[i], pids[i]);
          }
        }
      }
      
      // Else, print history
      else
      {
        for( int i = 0; i < count; i++ )
        {
          if( history[i] != NULL )
          {
            printf("%d: %s \n", i, history[i]);
          }
        }
      }
    }

    //Else, fork process normally
    else 
    {
      pid_t pid = fork();
      
      // Child process forks
      if (pid == 0)
      {
        int status_code = execvp(token[0], token);

        // If status = -1, then process didn't run properly
        if (status_code == -1)
        {
          printf("%s: Command not found.\n", token[0]);
          _exit(-1);
        }
      }

      // Parent waits for child to exit
      else
      {
        int status;
        wait(&status);
        pids[count-1] = pid;
      }
    }

    // Cleanup allocated memory
    for( int i = 0; i < MAX_NUM_ARGUMENTS; i++ )
    {
      if( token[i] != NULL )
      {
        free( token[i] );
      }
    }

    free( head_ptr );

  }

  free( command_string );

  return 0;
  // e2520ca2-76f3-90d6-0242ac120003
}

