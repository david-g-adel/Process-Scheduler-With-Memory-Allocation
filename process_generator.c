#include "headers.h"
#include <string.h>

void clearResources(int);

int shmid, processToScheduler, msgqid;

int main(int argc, char *argv[])
{
    shmid = shmget(SHKEY, 4, IPC_CREAT | 0644); // clock shared memory id
    processToScheduler = msgget(msgKey, 0666 | IPC_CREAT); // message queue id for sending processes to scheduler
    msgqid = msgget(toProcessKey, 0666 | IPC_CREAT); // message queue id 
    signal(SIGINT, clearResources); // signal handler
    FILE *fptr;
    char filename[100], c[100]; 
    fptr = fopen("processes.txt", "r");
    int processCount = 0;
    while (!feof(fptr)) // count number of processes
    {
        fscanf(fptr, "%s", c);
        if (c[0] == '#') // ignore comments
        {
            fgets(c, 100, fptr); // ignore the rest of the line
            continue; // go to next line
        }
        processCount++;
    }
    fclose(fptr);
    processCount /= 5; 
    printf("number of processes is %d\n", processCount);
    Process p;
    Process processes[processCount];
    // Open file
    fptr = fopen("processes.txt", "r");
    int i = 1, j = 0; // i is the index of the word in the line, j is the index of the process
    // Read contents from file
    while (!feof(fptr)) // read file line by line
    {
        fscanf(fptr, "%s", c);
        if (c[0] == '#')
        {
            fgets(c, 100, fptr);
            continue;
        }
        // printf("%s ", c);
        if (i == 1) 
            p.id = atoi(c); 
        else if (i == 2)
            p.arrivalTime = atoi(c);
        else if (i == 3)
            p.runningTime = atoi(c);
        else if (i == 4)
            p.priority = atoi(c);
        else if (i == 5)
            p.memsize = atoi(c);
        i++;
        p.remainingTime = p.runningTime; // initialize remaining time
        if (i > 5) // if we reached the end of the line
        {
            i = 1;
            processes[j] = p;
            processes[j].started = false;
            processes[j].stopped = false;
            processes[j].isLast = false;
             processes[j].isAllocated=false;
            j++;
        }
    }
    fclose(fptr);

    printf("Choose the desired algorithm \n 1 for HPF \n 2 for SRTN \n 3 for RR \n");
    char algorithm[100];
    fgets(algorithm, sizeof(algorithm), stdin);
    char Quanta[50];
    if (atoi(algorithm) == 3)
    {
        printf("Enter your desired quanta\n");
        fgets(Quanta, sizeof(Quanta), stdin);
    }

    int pid = fork(); // create clock process
    if (pid == 0) // if child
    {
        execl("./clk.out", "clk", NULL); // execute clock process
    }
    pid = fork(); // create scheduler process
    if (pid == 0) // if child
    {
        execl("./scheduler.out", "scheduler", algorithm,Quanta, NULL); // execute scheduler process
    }
    initClk();
    // To get time use this
    i = 0;
    int sendVal;
    ProcessMessage pMsg;
    while (i < processCount) // send processes to scheduler
    {
        int x = getClk(); // get current time
        if (processes[i].arrivalTime == x) // if process arrival time is now
        {
            if(i==processCount-1) // if last process
                processes[i].isLast=true; // set isLast to true
            pMsg.mtype = 1;
            pMsg.process = processes[i];
            sendVal = msgsnd(processToScheduler, &pMsg, sizeof(pMsg.process), !IPC_NOWAIT); // send process to scheduler
            if (sendVal == -1)
            {
                perror("Error in send");
            }
            i++;
        }
        // else
        // {
        //     pMsg.mtype=2; // send empty message
        //     sendVal = msgsnd(processToScheduler, &pMsg, sizeof(pMsg.process), !IPC_NOWAIT); // send empty message to scheduler
        //     if (sendVal == -1)
        //     {
        //         perror("Error in send");
        //     }
        // }
    }

    // TODO Generation Main Loop
    // 5. Create a data structure for processes and provide it with its parameters.
    // 6. Send the information to the scheduler at the appropriate time.
    // 7. Clear clock resources
    // while (1)
    // {
    // }; // infinite loop to keep the process generator running
    wait(NULL); // wait for scheduler to terminate
    destroyClk(true); // destroy clock
}

void clearResources(int signum)
{
    // TODO Clears all resources in case of interruption
    shmctl(shmid, IPC_RMID, NULL); // remove clock shared memory
    msgctl(processToScheduler, IPC_RMID, NULL); // remove message queue
    msgctl(msgqid, IPC_RMID, NULL); // remove message queue
    printf("Process terminating!\n");
    exit(0);
}