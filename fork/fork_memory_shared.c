#include <stdlib.h>
#include <unistd.h> 
#include <sys/types.h> 
#include <errno.h> 
#include <stdio.h> 
#include <sys/wait.h> 

int globalVar;

int main()
{

int localVar = 0; 
    int* p = (int*) malloc(2); 
    pid_t childPID = fork(); 
  
    // Putting value at allocated address 
    *p = 0; 
  
    if (childPID >= 0) // fork was successful 
    { 
        if (childPID == 0) // child process 
        { 
            printf("\n Child Process Initial Value :: localVar"
                   " = %d, globalVar = %d", localVar, 
                   globalVar); 
            localVar++; 
            globalVar++; 
  
            int c = 500; 
            printf("\n Child Process :: localVar = %d, "
                   "globalVar = %d", localVar, globalVar); 
            printf("\n Address of malloced mem child = %p "
                   "and value is %d", p, *p); 
            printf("\n lets change the value pointed my malloc"); 
  
            *p = 50; 
            printf("\n Address of malloced mem child = %p "
                   "and value is %d", p, *p); 
            printf("\n lets change the value pointed my "
                   "malloc in child"); 
  
            *p = 200; 
            printf("\n Address of malloced mem child = %p "
                   "and value is %d\n\n\n", p, *p); 
        } 
        else // Parent process 
        { 
            printf("\n Parent process Initial Value :: "
                   "localVar = %d, globalVar = %d", 
                   localVar, globalVar); 
  
            localVar = 10; 
            globalVar = 20; 
            printf("\n Parent process :: localVar = %d,"
                  " globalVar = %d", localVar, globalVar); 
            printf("\n Address of malloced mem parent= %p "
                   "and value is %d", p, *p); 
  
            *p = 100; 
            printf("\n Address of malloced mem parent= %p "
                   "and value is %d", p, *p); 
            printf("\n lets change the value pointed my"
                    " malloc in child"); 
            *p = 400; 
            printf("\n Address of malloced mem child = %p"
                   " and value is %d \n", p, *p); 
        } 
    } 
    else // fork failed 
    { 
        printf("\n Fork failed, quitting!!!!!!\n"); 
        return 1; 
    } 
  
    return 0; 
} 




