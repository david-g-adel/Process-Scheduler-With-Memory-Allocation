#include "headers.h"

/* Modify this file as needed*/
int remainingtime;
int x;
void handler(int signum)
{
    x = getClk();
}

int main(int agrc, char *argv[])
{
    initClk();
    signal(SIGCONT, handler);
    message msg;
    int msgqid = msgget(toProcessKey, 0666 | IPC_CREAT); // message queue id
    char pid[20];
    // TODO it needs to get the remaining time from somewhere
    remainingtime = atoi(argv[1]); // initialize remaining time
    x = getClk();                  // get current time
    while (remainingtime > 0)      // while remaining time is not zero
    {
        if (getClk() - x == 1)
        {
            remainingtime--;
            x = getClk();
        }
        // sleep(1); // sleep for one second
    }
    msg.mtype = 1;
    strcpy(msg.mtext, "bye"); // send bye message to scheduler when done
    int sendVal = msgsnd(msgqid, &msg, sizeof(msg.mtext), !IPC_NOWAIT);
    if (sendVal == -1)
    {
        perror("Error in send");
    }

    destroyClk(false);

    return 0;
}
