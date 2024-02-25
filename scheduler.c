#include "headers.h"

bool flag = true;

int main(int argc, char *argv[])
{
    FILE *file = fopen("scheduler.log", "w");
    wtaNode *wtaHead = NULL; // creating a queue to store WTA of each process
    int processCount = 0;    // number of processes
    double avgWaiting = 0, avgWTA = 0, std = 0, wta = 0;
    int busyTime = 0;
    if (file == NULL)
    {
        printf("Unable to open the file.\n");
        return 1; // Return an error code
    }
    FILE* memoryFile=fopen("memory.log","w");
    if (memoryFile == NULL)
    {
        printf("Unable to open the memory file.\n");
        return 1; // Return an error code
    }
    fprintf(file, "#At\ttime\tx\tprocess\ty\tstate\tarr\tw\ttotal\tz\tremain\ty\twait\tk\n"); // write the header of the log file
    fprintf(memoryFile, "#At\ttime\tx\tallocated\ty\tbytes\tfor\tprocess\tz\tfrom\ti\tto\tj\n"); // write the header of the log file

    initClk();
    int pid;
    bool isProcessed = true; // flag to check if the process is processed or not
    message msg;
    int msgqid = msgget(toProcessKey, 0666 | IPC_CREAT); // message queue id for receiving processes from process generator
    ProcessMessage pMsg;
    int processToScheduler = msgget(msgKey, 0666 | IPC_CREAT); // message queue id for receiving processes from scheduler
    int recVal, recValue;                                      // return value of msgrcv
    int type = 1;
    int q;
    char ch[100];
    TreeNode *root = (TreeNode *)malloc(sizeof(TreeNode));
    root->parent = NULL;
    root->size = 1024;
    root->isEmpty = true;
    root->left = NULL;
    root->right = NULL;
    root->start = 0;
    root->end = 1023;
    TreeNode *minNode = NULL;
    PriorityNode *waitingHead = NULL;
    if (atoi(argv[1]) == 1) // HPF
    {
        Process runningProcess;    // the process that is currently running
        PriorityNode *head = NULL; // priority queue to store processes
        while (1)
        {
            recVal = 1;                           // initialize the return value of msgrcv
            type = 1;                             // initialize the type of the message
            while (recVal != -1 /*&& type == 1*/) // while there is a message in the queue
            {
                recVal = msgrcv(processToScheduler, &pMsg, sizeof(pMsg.process), 0, IPC_NOWAIT);
                if (recVal != -1)
                {
                    type = pMsg.mtype;
                    if (type == 1)
                    {
                        printf("scheduler: process %d arrived at %d\n", pMsg.process.id, getClk());
                        if (isEmpty(&head))
                        {
                            head = newPriorityNode(pMsg.process, pMsg.process.priority); // create a new node
                        }
                        else
                        {
                            push(&head, pMsg.process, pMsg.process.priority); // push the process to the queue
                        }
                        processCount++; // increment the number of processes
                        printf("scheduler: process %d arrived at %d\n", pMsg.process.id, getClk());
                        printf("scheduler: head process is %d at %d\n", peek(&head).id, getClk());
                    }
                }
            }
            if (!isEmpty(&head)) // if the queue is not empty
            {
                Process p = peek(&head); // get the process with the highest priority
                if (isProcessed == true) // if the process is processed
                {
                    int pmemsize = p.memsize;
                    minNode = NULL;
                    int memstart = 0;
                    int memend = 0;
                    getMinNode(root, &minNode, p.memsize);

                    allocateMemory(minNode, pmemsize, &memstart, &memend);
                    fprintf(memoryFile, "#At\ttime\t%d\tallocated\t%d\tbytes\tfor\tprocess\t%d\tfrom\t%d\tto\t%d\n",getClk(),p.memsize,p.id,memstart,memend);
                    // hena hancheck lw fy makan fa elmemory
                    p.memstart = memstart;
                    p.memend = memend;
                    isProcessed = false; // set the flag to false
                    pop(&head);          // remove the process from the queue
                    pid = fork();        // create a child process
                    if (pid == 0)        // if child
                    {
                        sprintf(ch, "%d", p.remainingTime);          // convert the remaining time to string
                        execl("./process.out", "process", ch, NULL); // execute the process
                    }
                    p.startTime = getClk(); // set the start time of the process
                    runningProcess = p;
                    fprintf(file, "At\ttime\t%d\tprocess\t%d\tstarted\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\n", getClk(), p.id, p.arrivalTime, p.runningTime, p.remainingTime, getClk() - p.arrivalTime - p.runningTime + p.remainingTime); // write to the log file
                    printf("scheduler: process %d started at %d\n", p.id, getClk());
                    busyTime += p.runningTime; // increment the busy time
                }
            }
            recValue = msgrcv(msgqid, &msg, sizeof(msg.mtext), 1, IPC_NOWAIT); // receive a message from the process

            if (recValue != -1) // if there is a message
            {
                printf("scheduler: process %d finished at %d\n", runningProcess.id, getClk());
                fprintf(file, "At\ttime\t%d\tprocess\t%d\tfinished\tarr\t%d\ttotal\t%d\tremain\t0\twait\t%d\tTA\t%d\tWTA\t%.2f\n", getClk(), runningProcess.id, runningProcess.arrivalTime, runningProcess.runningTime, runningProcess.startTime - runningProcess.arrivalTime - runningProcess.runningTime + runningProcess.remainingTime, getClk() - runningProcess.arrivalTime, (getClk() - runningProcess.arrivalTime) / (float)runningProcess.runningTime); // write to the log file
                avgWaiting += (getClk() - runningProcess.arrivalTime - runningProcess.runningTime);                                                                                                                                                                                                                                                                                                                                                             // increment the total waiting time
                avgWTA += (getClk() - runningProcess.arrivalTime) / (float)runningProcess.runningTime;                                                                                                                                                                                                                                                                                                                                                          // increment the total WTA
                isProcessed = true;                                                                                                                                                                                                                                                                                                                                                                                                                             // set the flag to true
                DeallocateMemory(root, runningProcess.memstart, runningProcess.memend);
                fprintf(memoryFile, "#At\ttime\t%d\tfreed\t%d\tbytes\tfor\tprocess\t%d\tfrom\t%d\tto\t%d\n",getClk(),runningProcess.memsize,runningProcess.id,runningProcess.memstart,runningProcess.memend);
                wta_enqueue((getClk() - runningProcess.arrivalTime) / (float)runningProcess.runningTime); // enqsueue the WTA of the process
                printf("is generator done? %d\n is queue empty? %d\n", pMsg.process.isLast, isEmpty(&head));
                if (pMsg.process.isLast && isEmpty(&head)) // if the process is the last process and the queue is empty
                {

                    fclose(file);                        // close the log file
                    file = fopen("scheduler.perf", "w"); // open the performance file
                    if (file == NULL)
                    {
                        printf("Unable to open the file.\n");
                        return 1; // Return an error code
                    }
                    avgWTA /= (double)processCount; // calculate the average WTA
                    printf("avgWTA = %f\n", avgWTA);
                    while (!wta_isEmpty()) // while the queue is not empty
                    {
                        wta = wta_peek();                       // get the WTA of the process
                        std += (wta - avgWTA) * (wta - avgWTA); // calculate the standard deviation
                        printf("wta = %f\n", wta);
                        printf("avgWTA = %f\n", avgWTA);
                        printf("std = %f\n", std);
                        wta_dequeue();
                    }
                    printf("avgWTA = %f\n", avgWTA);
                    printf("std = %f\n", std);
                    std = sqrt(std / (double)processCount);
                    printf("avgWTA = %f\n", avgWTA);
                    printf("std = %f\n", std);
                    fprintf(file, "CPU utilization = %.2f%%\nAvg WTA = %.2f\nAvg Waiting = %.2f\nStd WTA = %.2f", ((double)(busyTime) / getClk()) * 100, avgWTA, avgWaiting / (double)processCount, std); // write to the performance file
                    fclose(file);
                    fclose(memoryFile);
                    exit(0); // exit the scheduler
                }
            }
        }
    }
    else if (atoi(argv[1]) == 2) // SRTN
    {
        PriorityNode *head = NULL;
        Process runningProcess;
        Process p;
        runningProcess.started = false;
        while (1)
        {
            recVal = 1;
            type = 1;
            while (recVal != -1 /*&& type == 1*/)
            {
                recVal = msgrcv(processToScheduler, &pMsg, sizeof(pMsg.process), 0, IPC_NOWAIT);
                if (recVal != -1)
                {
                    type = pMsg.mtype;
                    if (type == 1)
                    {
                        if (isEmpty(&head)) // if the queue is empty
                        {
                            head = newPriorityNode(pMsg.process, pMsg.process.remainingTime); // create a new node
                            printf("scheduler: process %d arrived at %d\n", pMsg.process.id, getClk());
                            printf("scheduler: head process is %d at %d\n", peek(&head).id, getClk());
                        }
                        else
                        {
                            push(&head, pMsg.process, pMsg.process.remainingTime); // push the process to the queue
                            printf("scheduler: process %d arrived at %d\n", pMsg.process.id, getClk());
                            printf("scheduler: head process is %d at %d\n", peek(&head).id, getClk());
                        }
                        processCount++;
                        p = peek(&head);                                       // get the process with the least running time
                        if (runningProcess.started && !runningProcess.stopped) // if the process is running
                        {
                            int processedTime = getClk() - runningProcess.continueTime;                  // get the time the process has been running
                            runningProcess.remainingTime = runningProcess.remainingTime - processedTime; // calculate the remaining time
                            if (p.remainingTime < runningProcess.remainingTime)                          // if the new process has less remaining time
                            {
                                kill(runningProcess.realID, SIGSTOP);                      // stop the process
                                push(&head, runningProcess, runningProcess.remainingTime); // push the process to the queue
                                isProcessed = true;
                                runningProcess.stopped = true;
                                printf("process %d stopped at %d with remainning time %d\n", runningProcess.id, getClk(), runningProcess.remainingTime);
                                fprintf(file, "At\ttime\t%d\tprocess\t%d\tstopped\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\n", getClk(), runningProcess.id, runningProcess.arrivalTime, runningProcess.runningTime, runningProcess.remainingTime, getClk() - runningProcess.arrivalTime - runningProcess.runningTime + runningProcess.remainingTime);
                            }
                        }
                    }
                }
            }
            if (!isEmpty(&head)) // If queue is not empty
            {
                if (isProcessed == true)
                {

                    // runningProcess = SRTNpop(&head);
                    runningProcess = peek(&head); // get the process with the least remaining time
                    pop(&head);
                    if (runningProcess.started) // if the process is running
                    {
                        kill(runningProcess.realID, SIGCONT); // continue the process
                        runningProcess.continueTime = getClk();
                        isProcessed = false;
                        printf("process %d continued at %d\n", runningProcess.id, getClk());
                        fprintf(file, "At\ttime\t%d\tprocess\t%d\tresumed\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\n", getClk(), runningProcess.id, runningProcess.arrivalTime, runningProcess.runningTime, runningProcess.remainingTime, getClk() - runningProcess.arrivalTime - runningProcess.runningTime + runningProcess.remainingTime);
                    }
                    else
                    {
                        int pmemsize = runningProcess.memsize;
                        minNode = NULL;
                        int memstart = 0;
                        int memend = 0;
                        printf("before minnode\n");
                        getMinNode(root, &minNode, pmemsize);
                        if (!runningProcess.isAllocated && minNode == NULL)
                        {
                            printf("No space is available, process %d entered waiting queue", runningProcess.id);
                            waiting_enqueue(runningProcess); // create a new node
                        }
                        else
                        {
                            isProcessed = false;
                            if (!runningProcess.isAllocated)
                            {
                                allocateMemory(minNode, pmemsize, &memstart, &memend);
                                // hena hancheck lw fy makan fa elmemory
                                runningProcess.memstart = memstart;
                                runningProcess.memend = memend;
                                fprintf(memoryFile, "#At\ttime\t%d\tallocated\t%d\tbytes\tfor\tprocess\t%d\tfrom\t%d\tto\t%d\n",getClk(),runningProcess.memsize,runningProcess.id,runningProcess.memstart,runningProcess.memend);
                            }
                            runningProcess.started = true;
                            runningProcess.startTime = getClk(); // set the start time of the process
                            runningProcess.continueTime = getClk();
                            pid = fork(); // create a child process
                            if (pid == -1)
                            {
                                perror("error in fork");
                            }
                            if (pid == 0)
                            {
                                sprintf(ch, "%d", runningProcess.remainingTime); // convert the remaining time to string
                                execl("./process.out", "process", ch, NULL);     // execute the process
                            }
                            printf("scheduler: process %d started at %d\n", runningProcess.id, getClk());
                            fprintf(file, "At\ttime\t%d\tprocess\t%d\tstarted\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\n", getClk(), runningProcess.id, runningProcess.arrivalTime, runningProcess.runningTime, runningProcess.remainingTime, getClk() - runningProcess.arrivalTime - runningProcess.runningTime + runningProcess.remainingTime);
                            busyTime += runningProcess.runningTime;
                            runningProcess.realID = pid; // get the real id of the process
                        }
                    }
                }
            }
            recValue = msgrcv(msgqid, &msg, sizeof(msg.mtext), 0, IPC_NOWAIT);
            if (recValue != -1)
            {
                printf("scheduler: process %d finished at %d\n", runningProcess.id, getClk());
                isProcessed = true;
                DeallocateMemory(root, runningProcess.memstart, runningProcess.memend);
                fprintf(memoryFile, "#At\ttime\t%d\tfreed\t%d\tbytes\tfor\tprocess\t%d\tfrom\t%d\tto\t%d\n",getClk(),runningProcess.memsize,runningProcess.id,runningProcess.memstart,runningProcess.memend);

                if (runningProcess.id == 1)
                {
                    int x = 0;
                }
                printf("%d\n", getSize());
                for (int j = 0; j < getSize(); j++)
                {
                    Process inWaiting = waiting_peek();
                    minNode = NULL;
                    waiting_dequeue();
                    getMinNode(root, &minNode, inWaiting.memsize);
                    if (minNode == NULL)
                    {
                        waiting_enqueue(inWaiting);
                    }
                    else
                    {
                        int memstart = 0;
                        int memend = 0;
                        allocateMemory(minNode, inWaiting.memsize, &memstart, &memend);

                        inWaiting.memstart = memstart;
                        inWaiting.memend = memend;
                        inWaiting.isAllocated = true;
                        fprintf(memoryFile, "#At\ttime\t%d\tallocated\t%d\tbytes\tfor\tprocess\t%d\tfrom\t%d\tto\t%d\n",getClk(),inWaiting.memsize,inWaiting.id,inWaiting.memstart,inWaiting.memend);
                        if (isEmpty(&head))
                        {
                            head = newPriorityNode(inWaiting, inWaiting.remainingTime);
                        }
                        else
                        {
                            push(&head, inWaiting, inWaiting.remainingTime);
                        }
                    }
                }
                runningProcess.started = false;
                printf("%s\n", msg.mtext);
                fprintf(file, "At\ttime\t%d\tprocess\t%d\tfinished\tarr\t%d\ttotal\t%d\tremain\t0\twait\t%d\tTA\t%d\tWTA\t%.2f\n", getClk(), runningProcess.id, runningProcess.arrivalTime, runningProcess.runningTime, getClk() - runningProcess.arrivalTime - runningProcess.runningTime, getClk() - runningProcess.arrivalTime, (getClk() - runningProcess.arrivalTime) / (float)runningProcess.runningTime);
                avgWaiting += (getClk() - runningProcess.arrivalTime - runningProcess.runningTime);
                avgWTA += (getClk() - runningProcess.arrivalTime) / (float)runningProcess.runningTime;
                wta_enqueue((getClk() - runningProcess.arrivalTime) / (float)runningProcess.runningTime);
                if (pMsg.process.isLast && isEmpty(&head) && waiting_isEmpty())
                {

                    fclose(file);
                    file = fopen("scheduler.perf", "w");
                    if (file == NULL)
                    {
                        printf("Unable to open the file.\n");
                        return 1; // Return an error code
                    }
                    avgWTA /= (double)processCount;
                    printf("avgWTA = %f\n", avgWTA);
                    while (!wta_isEmpty())
                    {
                        wta = wta_peek();
                        std += (wta - avgWTA) * (wta - avgWTA);
                        printf("wta = %f\n", wta);
                        printf("avgWTA = %f\n", avgWTA);
                        printf("std = %f\n", std);
                        wta_dequeue();
                    }
                    printf("avgWTA = %f\n", avgWTA);
                    printf("std = %f\n", std);
                    std = sqrt(std / (double)processCount);
                    printf("avgWTA = %f\n", avgWTA);
                    printf("std = %f\n", std);
                    fprintf(file, "CPU utilization = %.2f%%\nAvg WTA = %.2f\nAvg Waiting = %.2f\nStd WTA = %.2f", ((double)(busyTime) / getClk()) * 100, avgWTA, avgWaiting / (double)processCount, std);
                    fclose(file);
                    fclose(memoryFile);
                    exit(0); // exit the scheduler
                }
            }
        }
    }
    else if (atoi(argv[1]) == 3) // Round Robin
    {
        q = atoi(argv[2]); // get the quanta
        printf("The quanta:%d\n", q);
        // Node *head = NULL; // circular queue to store processes
        Process runningProcess;
        Process p;
        runningProcess.started = false;
        int processedTime;
        while (1)
        {
            recVal = 1;
            type = 1;
            while (recVal != -1 /*&& type == 1*/)
            {
                recVal = msgrcv(processToScheduler, &pMsg, sizeof(pMsg.process), 0, IPC_NOWAIT);
                if (recVal != -1)
                {
                    type = pMsg.mtype;
                    if (type == 1)
                    {
                        enqueue(pMsg.process); // enqueue the process
                        p = c_peek();          // get the process at the head of the queue
                        printf("scheduler: process %d arrived at %d\n", pMsg.process.id, getClk());
                        printf("scheduler: head process is %d at %d\n", p.id, getClk());
                        processCount++;
                    }
                }
            }
            if (runningProcess.started)
            {
                processedTime = getClk() - runningProcess.continueTime; // get the time the process has been running
                if (processedTime == q)                                 // if the process has finished its quanta
                {
                    runningProcess.remainingTime -= q; // calculate the remaining time
                    runningProcess.continueTime = getClk();
                }
            }
            if (!c_isEmpty()) // if the queue is not empty
            {
                if (runningProcess.started)
                {
                    // get the time the process has been running
                    if (processedTime == q) // if the process has finished its quanta
                    {

                        if (runningProcess.remainingTime > 0) // if the process has remaining time
                        {
                            kill(runningProcess.realID, SIGSTOP); // stop the process
                            isProcessed = true;
                            runningProcess.stopTime = getClk(); // set the stop time of the process
                            printf("process %d stopped at %d with remainning time %d and x = %d\n", runningProcess.id, getClk(), runningProcess.remainingTime, runningProcess.stopTime);
                            enqueue(runningProcess); // enqueue the process
                            fprintf(file, "At\ttime\t%d\tprocess\t%d\tstopped\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\n", getClk(), runningProcess.id, runningProcess.arrivalTime, runningProcess.runningTime, runningProcess.remainingTime, getClk() - runningProcess.arrivalTime - runningProcess.runningTime + runningProcess.remainingTime);
                        }
                    }
                }
                if (isProcessed == true)
                {
                    runningProcess = c_peek();
                    dequeue();
                    if (runningProcess.started)
                    {
                        kill(runningProcess.realID, SIGCONT); // continue the process
                        isProcessed = false;
                        printf("process %d continued at %d\n", runningProcess.id, getClk());
                        runningProcess.continueTime = getClk(); // set the continue time of the process
                        fprintf(file, "At\ttime\t%d\tprocess\t%d\tresumed\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\n", getClk(), runningProcess.id, runningProcess.arrivalTime, runningProcess.runningTime, runningProcess.remainingTime, getClk() - runningProcess.arrivalTime - runningProcess.runningTime + runningProcess.remainingTime);
                    }
                    else
                    {
                        int pmemsize = runningProcess.memsize;
                        minNode = NULL;
                        int memstart = 0;
                        int memend = 0;
                        getMinNode(root, &minNode, pmemsize);
                        if (!runningProcess.isAllocated && minNode == NULL)
                        {
                            printf("No space is available, process %d entered waiting queue", runningProcess.id);
                            waiting_enqueue(runningProcess); // create a new node
                        }
                        else
                        {
                            isProcessed = false;
                            if (!runningProcess.isAllocated)
                            {
                                allocateMemory(minNode, pmemsize, &memstart, &memend);

                                // hena hancheck lw fy makan fa elmemory
                                runningProcess.memstart = memstart;
                                runningProcess.memend = memend;
                                fprintf(memoryFile, "#At\ttime\t%d\tallocated\t%d\tbytes\tfor\tprocess\t%d\tfrom\t%d\tto\t%d\n",getClk(),runningProcess.memsize,runningProcess.id,runningProcess.memstart,runningProcess.memend);
                            }
                            runningProcess.started = true;
                            runningProcess.startTime = getClk(); // set the start time of the process
                            runningProcess.continueTime = getClk();
                            pid = fork(); // create a child process
                            if (pid == -1)
                            {
                                perror("error in fork");
                            }
                            if (pid == 0)
                            {
                                sprintf(ch, "%d", runningProcess.remainingTime); // convert the remaining time to string
                                execl("./process.out", "process", ch, NULL);     // execute the process
                            }
                            printf("scheduler: process %d started at %d\n", runningProcess.id, getClk());
                            fprintf(file, "At\ttime\t%d\tprocess\t%d\tstarted\tarr\t%d\ttotal\t%d\tremain\t%d\twait\t%d\n", getClk(), runningProcess.id, runningProcess.arrivalTime, runningProcess.runningTime, runningProcess.remainingTime, getClk() - runningProcess.arrivalTime - runningProcess.runningTime + runningProcess.remainingTime);
                            busyTime += runningProcess.runningTime;
                            runningProcess.realID = pid; // get the real id of the process
                        }
                    }
                }
            }
            recVal = msgrcv(msgqid, &msg, sizeof(msg.mtext), 0, IPC_NOWAIT);
            if (recVal != -1)
            {
                printf("scheduler: process %d finished at %d\n", runningProcess.id, getClk());
                isProcessed = true;
                DeallocateMemory(root, runningProcess.memstart, runningProcess.memend);
                fprintf(memoryFile, "#At\ttime\t%d\tfreed\t%d\tbytes\tfor\tprocess\t%d\tfrom\t%d\tto\t%d\n",getClk(),runningProcess.memsize,runningProcess.id,runningProcess.memstart,runningProcess.memend);
                if (runningProcess.id == 1)
                {
                    int x = 0;
                }
                printf("%d\n", getSize());
                for (int j = 0; j < getSize(); j++)
                {
                    Process inWaiting = waiting_peek();
                    minNode = NULL;
                    waiting_dequeue();
                    getMinNode(root, &minNode, inWaiting.memsize);
                    if (minNode == NULL)
                    {
                        waiting_enqueue(inWaiting);
                    }
                    else
                    {
                        int memstart = 0;
                        int memend = 0;
                        allocateMemory(minNode, inWaiting.memsize, &memstart, &memend);
                        inWaiting.memstart = memstart;
                        inWaiting.memend = memend;
                        inWaiting.isAllocated = true;
                        fprintf(memoryFile, "#At\ttime\t%d\tallocated\t%d\tbytes\tfor\tprocess\t%d\tfrom\t%d\tto\t%d\n",getClk(),inWaiting.memsize,inWaiting.id,inWaiting.memstart,inWaiting.memend);

                        enqueue(inWaiting);
                    }
                }
                printf("%s\n", msg.mtext);
                fprintf(file, "At\ttime\t%d\tprocess\t%d\tfinished\tarr\t%d\ttotal\t%d\tremain\t0\twait\t%d\tTA\t%d\tWTA\t%.2f\n", getClk(), runningProcess.id, runningProcess.arrivalTime, runningProcess.runningTime, getClk() - runningProcess.arrivalTime - runningProcess.runningTime, getClk() - runningProcess.arrivalTime, (getClk() - runningProcess.arrivalTime) / (float)runningProcess.runningTime);
                avgWaiting += (getClk() - runningProcess.arrivalTime - runningProcess.runningTime);
                avgWTA += (getClk() - runningProcess.arrivalTime) / (float)runningProcess.runningTime;
                wta_enqueue((getClk() - runningProcess.arrivalTime) / (double)runningProcess.runningTime);
                if (pMsg.process.isLast && c_isEmpty())
                {

                    fclose(file);
                    file = fopen("scheduler.perf", "w");
                    if (file == NULL)
                    {
                        printf("Unable to open the file.\n");
                        return 1; // Return an error code
                    }
                    avgWTA /= (double)processCount;
                    printf("avgWTA = %f\n", avgWTA);
                    while (!wta_isEmpty())
                    {
                        wta = wta_peek();
                        std += (wta - avgWTA) * (wta - avgWTA);
                        printf("wta = %f\n", wta);
                        printf("avgWTA = %f\n", avgWTA);
                        printf("std = %f\n", std);
                        wta_dequeue();
                    }
                    printf("avgWTA = %f\n", avgWTA);
                    printf("std = %f\n", std);
                    std = sqrt(std / (double)processCount);
                    printf("avgWTA = %f\n", avgWTA);
                    printf("std = %f\n", std);
                    fprintf(file, "CPU utilization = %.2f%%\nAvg WTA = %.2f\nAvg Waiting = %.2f\nStd WTA = %.2f", ((double)(busyTime) / getClk()) * 100, avgWTA, avgWaiting / (double)processCount, std);
                    fclose(file);
                    fclose(memoryFile);
                    exit(0); // exit the scheduler
                }
            }
        }
    }
    else
    {
        printf("Invalid input\n"); // if the input is invalid
        exit(0);                   // exit the scheduler
    }
    // TODO implement the scheduler :)
    // upon termination release the clock resources.
}
