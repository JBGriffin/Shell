/* ysh.c written by Garrett Griffin
 * Purpose: To create a shell like program. This program will be able
 *   		to handle the following commands:
 * > prog1 arg1 arg2 | prog2 arg3 < infile > outfile &
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

// Generate struct
struct cmd
{
	int redirect_in;
	int redirect_out;
	int redirect_append;
	int background;
	int piping;
	char *infile;
	char *outfile;
	char *argv1[10];
	char *argv2[10];
};

// Prototype for call to Professor's argument scanner
int cmdscan(char *cmdbuf, struct cmd *com);

int main(void){
    char buf[1024];
    struct cmd command;
	int fdin, fdout, fd[2], fileFlags;
    printf(">");
	// Run program while user has not typed in exit
    while( ( gets(buf)!=NULL && strcmp(buf,"exit"))!=0 )
	{
		// Grab next command
    	if (cmdscan(buf,&command))
		{
            printf("Illegal Format: \n");
            continue;
        }
        switch(fork())
		{
            case 1:
                perror("Fork Error\n");
                exit(1);
            case 0:
				// First fork for one program called
                switch(fork())
				{
                    case 1:
                        perror("Fork Error\n");
                        exit(1);
                    case 0:
						// Check for inputfiles
                        if(command.redirect_in)
						{
                            if((fdin = open(command.infile, O_RDONLY)) < 0)
							{
								perror("Open Error: Could not open input file!");
                                exit(1);
                            }
                        }
						// Set flags for normal output. If appending is needed, set flags
                        fileFlags = O_WRONLY | O_CREAT | O_TRUNC;
                        if(command.redirect_append)
                            fileFlags = O_WRONLY | O_CREAT | O_APPEND;
                        if(command.redirect_out)
						{
                           if((fdout = open(command.outfile, fileFlags, 0644)) < 0)
							{
                            	perror("Open Error: Could not open output file!");
                                exit(1);
                           }
                        }
						// If a pipe is called, fork and check stats for second program
                        if(command.piping)
						{
                           if(pipe(fd))
							{
						 		perror("Pipe Error: Could not create pipe!");
                              	exit(1);
                           }
                           switch(fork())
						   {
                           		case 1:
                                    perror("Fork Error\n");
                                    exit(1);
                                case 0:
                                	if(command.redirect_in)
									{
                                       	dup2(fdin, STDIN_FILENO);
                                       	close(fdin);
                                    }
                                    if(dup2(fd[1], STDOUT_FILENO) < 0)
									{
										perror("Dupe Error: Could not dupe fd[1]!");
										close(fd[0]);
                                      	exit(1);
                                    }
                                    close(fd[1]);
                                    close(fd[0]);
                                    execvp(command.argv1[0], command.argv1);
									perror("Exec Error: Could not exec ");
									printf("%s is not valid!\n", command.argv1[0]);
                                    exit(1);
                                default:
                                    if(command.redirect_out)
									{
                                    	dup2(fdout, STDOUT_FILENO);
                                    	close(fdout);
                                    }
                                    if(dup2(fd[0], STDIN_FILENO) < 0)
									{
										perror("Could not dupe fd[0]");
										close(fd[1]);
                                       	exit(1);
                                    }
                                    close(fd[1]);
                                    close(fd[0]);
                                    execvp(command.argv2[0], command.argv2);
									perror("Exec Error: Could not exec ");
									printf("%s is not valid!", command.argv2[0]);
                                    exit(1);
                           		}
                        }
                        if(command.redirect_in){
                           dup2(fdin, STDIN_FILENO);
                           close(fdin);
                        }
                        if(command.redirect_out) {
                           dup2(fdout, STDOUT_FILENO);
                           close(fdout);
                        }
                        execvp(command.argv1[0], command.argv1);
						perror("Exec Error: Could not exec ");
						printf("%s is not valid!\n", command.argv1[0]);
                        exit(1);
                    default:
						// For backgrounding, do not wait for the process to finish
                        if(command.background)
                            exit(0);
                        wait(NULL);
                        exit(0);
                }
                break;
            default:
                printf(">");
                wait(NULL);
 		}
	}
	// If by some miracle nothing crashed, fork bombed, or otherwise failed, exit with no error
	exit(0);
}
