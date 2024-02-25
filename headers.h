// REFERENCES
// https://www.geeksforgeeks.org/priority-queue-using-linked-list/
// https://www.geeksforgeeks.org/circular-queue-set-1-introduction-array-implementation/
#include <stdio.h> //if you don't use scanf/printf change this include
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <math.h>

typedef short bool;
#define true 1
#define false 0

#define SHKEY 300
#define SEMKEY 250
#define toProcessKey 150
#define msgKey 100

////////////////////////////////////////// PROCESS /////////////////////////////////////////////
typedef struct process
{
    int id;
    int arrivalTime;
    int runningTime;
    int remainingTime;
    int priority;
    bool started;
    bool stopped;
    bool isLast;
    int realID;
    int startTime;
    int continueTime;
    int stopTime;
    int memsize;
    int memstart;
    int memend;
    bool isAllocated;
} Process;
////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////// Binary Tree //////////////////////////////////////////
typedef struct treeNode
{
    int size;
    bool isEmpty;
    int start;
    int end;
    int usedSize;
    struct treeNode *left;
    struct treeNode *right;
    struct treeNode *parent;
} TreeNode;

void getMinNode(TreeNode *root, TreeNode **minNode, int value)
{
    if (root->left == NULL && root->right == NULL)
    {
        if (root->isEmpty == true)
        {
            if ((*minNode) == NULL && root->size >= value)
            {
                *minNode = root;
            }
            else if ((*minNode) != NULL)
            {
                if (root->size < (*minNode)->size && root->size >= value)
                {
                    *minNode = root;
                }
            }
        }
    }
    else if (root->left != NULL && root->right != NULL)
    {
        getMinNode(root->left, minNode, value);
        getMinNode(root->right, minNode, value);
    }
    else if (root->left != NULL)
    {
        getMinNode(root->left, minNode, value);
    }
    else if (root->right != NULL)
    {
        getMinNode(root->right, minNode, value);
    }
}

void splitNode(TreeNode *node)
{
    TreeNode *left = (TreeNode *)malloc(sizeof(TreeNode));
    left->size = node->size / 2;
    left->isEmpty = true;
    left->start = node->start;
    left->end = node->start + left->size - 1;
    left->usedSize = 0;
    left->left = NULL;
    left->right = NULL;
    left->parent = node;
    node->left = left;

    TreeNode *right = (TreeNode *)malloc(sizeof(TreeNode));
    right->size = node->size / 2;
    right->isEmpty = true;
    right->start = node->start + left->size;
    right->end = node->end;
    right->usedSize = 0;
    right->left = NULL;
    right->right = NULL;
    right->parent = node;
    node->right = right;

    node->isEmpty = false;
    printf("Splitted Block from %d to %d \n", node->start, node->end);
}

void mergeNode(TreeNode *node)
{

    if (node->left->isEmpty == true && node->right->isEmpty == true)
    {
        node->isEmpty = true;
        node->start = node->left->start;
        node->end = node->right->end;
        node->usedSize = 0;
        free(node->left);
        free(node->right);
        node->left = NULL;
        node->right = NULL;
        printf("Merged block a from %d to %d \n", node->start, node->end);
        if (node->parent != NULL)
        {
            mergeNode(node->parent);
        }
    }
}

int roundToNextPowerOf2(int num)
{
    if (num <= 0)
        return 1; // Minimum power of 2 is 2^0 = 1

    int power = 1;

    // Find the smallest power of 2 greater than or equal to the input number
    while (power < num)
    {
        power *= 2; // Equivalent to power *= 2
    }

    return power;
}

void allocateMemory(TreeNode *node, int memsize, int *memstart, int *memend)
{
    int requiredSize = roundToNextPowerOf2(memsize);
    if (node->isEmpty && node->size == requiredSize)
    {
        node->isEmpty = false;
        node->usedSize = memsize;
        *memstart = node->start;
        *memend = node->end;
        printf("Allocated memory from %d to %d\n", *memstart, *memend);
    }
    else if (node->size > requiredSize)
    {
        splitNode(node);
        allocateMemory(node->left, memsize, memstart, memend);
    }
}

void DeallocateMemory(TreeNode *node, int memstart, int memend)
{

    if (memstart == node->start && memend == node->end)
    {
        node->isEmpty = true;
        node->usedSize = 0;
        printf("Delocated memory from %d to %d\n", memstart, memend);
        if (node->parent != NULL)
        {
            mergeNode(node->parent);
        }
    }
    else if (memstart >= (node->right->start) && memend <= (node->right->end))
    {
        DeallocateMemory(node->right, memstart, memend);
    }
    else if (memstart >= (node->left->start) && memend <= (node->left->end))
    {
        DeallocateMemory(node->left, memstart, memend);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////// PRIORITY QUEUE //////////////////////////////////////////
typedef struct priorityNode
{
    Process process;
    int priority; // Lower values indicate higher priority
    struct priorityNode *next;

} PriorityNode;

// Function to Create A New PriorityNode
PriorityNode *newPriorityNode(Process process, int p)
{
    PriorityNode *waiting = (PriorityNode *)malloc(sizeof(PriorityNode));
    waiting->process = process;
    waiting->priority = p;
    waiting->next = NULL;

    return waiting;
}

// Return the value at head
Process peek(PriorityNode **head)
{
    return (*head)->process;
}

// Removes the element with the
// highest priority from the list
void pop(PriorityNode **head)
{
    PriorityNode *waiting = *head;
    (*head) = (*head)->next;
    free(waiting);
}

// Process SRTNpop(PriorityNode **head)
// {
//     Process process = (*head)->process;
//     PriorityNode *waiting = *head;
//     (*head) = (*head)->next;
//     free(waiting);
//     return (process);
// }

// Function to push according to priority
void push(PriorityNode **head, Process process, int p)
{
    PriorityNode *start = (*head);

    // Create new PriorityNode
    PriorityNode *waiting = newPriorityNode(process, p);

    // Special Case: The head of list has lesser
    // priority than new PriorityNode. So insert new
    // PriorityNode before head PriorityNode and change head PriorityNode.
    if ((*head)->priority > p)
    {

        // Insert New PriorityNode before head
        waiting->next = *head;
        (*head) = waiting;
    }
    else
    {

        // Traverse the list and find a
        // position to insert new PriorityNode
        while (start->next != NULL &&
               start->next->priority <= p)
        {
            start = start->next;
        }

        // Either at the ends of the list
        // or at required position
        waiting->next = start->next;
        start->next = waiting;
    }
}

// Function to check is list is empty
int isEmpty(PriorityNode **head)
{
    return (*head) == NULL;
}
////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////// PROCESS QUEUE //////////////////////////////////////////////
typedef struct node
{
    Process process;
    struct node *next;
} Node;

Node *waitingF = NULL;
// Node *waitingR=NULL;
Node *f = NULL;
Node *r = NULL;

void enqueue(Process process) // Insert elements in Queue
{
    Node *n;
    n = (Node *)malloc(sizeof(Node));
    n->process = process;
    n->next = NULL;
    if ((r == NULL) && (f == NULL))
    {
        f = r = n;
        r->next = f;
    }
    else
    {
        r->next = n;
        r = n;
        n->next = f;
    }
}

void dequeue() // Delete an element from Queue
{
    struct node *t;
    t = f;
    if ((f == NULL) && (r == NULL))
        printf("\nQueue is Empty");
    else if (f == r)
    {
        f = r = NULL;
        free(t);
    }
    else
    {
        f = f->next;
        r->next = f;
        free(t);
    }
}

bool c_isEmpty()
{
    struct node *t;
    t = f;
    if (t != NULL)
    {
        return false;
    }
    return true;
}

Process c_peek()
{
    struct node *t;
    t = f;
    if (!c_isEmpty())
    {
        return (t->process);
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////// QUEUE /////////////////////////////////////////////////
typedef struct WTA_node
{
    double WTA;
    struct WTA_node *next;
} wtaNode;

wtaNode *wta_f = NULL;
wtaNode *wta_r = NULL;

void wta_enqueue(double WTA) // Insert elements in Queue
{
    wtaNode *n;
    n = (wtaNode *)malloc(sizeof(wtaNode));
    n->WTA = WTA;
    n->next = NULL;
    if ((wta_r == NULL) && (wta_f == NULL))
    {
        wta_f = wta_r = n;
        wta_r->next = wta_f;
    }
    else
    {
        wta_r->next = n;
        wta_r = n;
        n->next = wta_f;
    }
}

void wta_dequeue() // Delete an element from Queue
{
    struct WTA_node *t;
    t = wta_f;
    if ((wta_f == NULL) && (wta_r == NULL))
        printf("\nQueue is Empty");
    else if (wta_f == wta_r)
    {
        wta_f = wta_r = NULL;
        free(t);
    }
    else
    {
        wta_f = wta_f->next;
        wta_r->next = wta_f;
        free(t);
    }
}

bool wta_isEmpty()
{
    struct WTA_node *t;
    t = wta_f;
    if (t != NULL)
    {
        return false;
    }
    return true;
}

double wta_peek()
{
    struct WTA_node *t;
    t = wta_f;
    if (!wta_isEmpty())
    {
        return (t->WTA);
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////// WAITING QUEUE //////////////////////////////////////////

// Node* waitingF = NULL;
Node *waitingR = NULL;

void waiting_enqueue(Process process) // Insert elements in Queue
{
    Node *n;
    n = (Node *)malloc(sizeof(Node));
    n->process = process;
    n->next = NULL;
    if ((waitingR == NULL) && (waitingF == NULL))
    {
        waitingF = waitingR = n;
    }
    else
    {
        waitingR->next = n;
        waitingR = n;
        waitingR->next = NULL;
    }
}

void waiting_dequeue() // Delete an element from Queue
{
    Node *t;
    t = waitingF;
    if ((waitingF == NULL) && (waitingR == NULL))
        printf("\nQueue is Empty");
    else if (waitingF == waitingR)
    {
        waitingF = waitingR = NULL;
        free(t);
    }
    else
    {
        waitingF = waitingF->next;
        free(t);
    }
}

bool waiting_isEmpty()
{
    Node *t;
    t = waitingF;
    if (t != NULL)
    {
        return false;
    }
    return true;
}

Process waiting_peek()
{
    Node *t;
    t = waitingF;
    if (!waiting_isEmpty())
    {
        return (t->process);
    }
}

int getSize()
{
    int i=0;
    Node* t= waitingF;
    while(t)
    {
        i++;
        t=t->next;
    }
    return i;
}
///////////////////////////////////////////////////////////////////////////////////////////////

///==============================
// don't mess with this variable//
int *shmaddr; //
//===============================

/////////////////////////////////////////// SEMAPHORES /////////////////////////////////////////
union Semun
{
    int val;               /* Value for SETVAL */
    struct semid_ds *buf;  /* Buffer for IPC_STAT, IPC_SET */
    unsigned short *array; /*Array for GETALL, SETALL */
    struct seminfo *__buf; /* Buffer for IPC_INFO (Linux-specific) */
};
void down(int sem)
{
    struct sembuf op;

    op.sem_num = 0;
    op.sem_op = -1;
    op.sem_flg = !IPC_NOWAIT;

    if (semop(sem, &op, 1) == -1)
    {
        perror("Error in down()");
        exit(-1);
    }
}

void up(int sem)
{
    struct sembuf op;

    op.sem_num = 0;
    op.sem_op = 1;
    op.sem_flg = !IPC_NOWAIT;

    if (semop(sem, &op, 1) == -1)
    {
        perror("Error in up()");
        exit(-1);
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////// CLOCK ////////////////////////////////////////////////
int getClk()
{
    return *shmaddr;
}

/*
 * All process call this function at the beginning to establish communication between them and the clock module.
 * Again, remember that the clock is only emulation!
 */
void initClk()
{
    int shmid = shmget(SHKEY, 4, 0444);
    while ((int)shmid == -1)
    {
        // Make sure that the clock exists
        printf("Wait! The clock not initialized yet!\n");
        sleep(1);
        shmid = shmget(SHKEY, 4, 0444);
    }
    shmaddr = (int *)shmat(shmid, (void *)0, 0);
}

/*
 * All process call this function at the end to release the communication
 * resources between them and the clock module.
 * Again, Remember that the clock is only emulation!
 * Input: terminateAll: a flag to indicate whether that this is the end of simulation.
 *                      It terminates the whole system and releases resources.
 */

void destroyClk(bool terminateAll)
{
    shmdt(shmaddr);
    if (terminateAll)
    {
        int x = getpgrp();
        killpg(getpgrp(), SIGINT);
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////// MESSAGE /////////////////////////////////////////////
typedef struct msgbuff
{
    long mtype;
    char mtext[10];
} message;
////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////// PROCESS MESSAGE /////////////////////////////////////
typedef struct processMessage
{
    long mtype;
    Process process;
} ProcessMessage;
////////////////////////////////////////////////////////////////////////////////////////////////