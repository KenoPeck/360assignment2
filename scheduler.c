#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

// Headers as needed

typedef enum {false, true} bool;        // Allows boolean types in C

/* Defines a job struct */
typedef struct Process {
    uint32_t A;                         // A: Arrival time of the process
    uint32_t B;                         // B: Upper Bound of CPU burst times of the given random integer list
    uint32_t C;                         // C: Total CPU time required
    uint32_t M;                         // M: Multiplier of CPU burst time
    uint32_t processID;                 // The process ID given upon input read

    uint8_t status;                     // 0 is unstarted, 1 is ready, 2 is running, 3 is blocked, 4 is terminated

    int32_t finishingTime;              // The cycle when the the process finishes (initially -1)
    uint32_t currentCPUTimeRun;         // The amount of time the process has already run (time in running state)
    uint32_t currentIOBlockedTime;      // The amount of time the process has been IO blocked (time in blocked state)
    uint32_t currentWaitingTime;        // The amount of time spent waiting to be run (time in ready state)

    uint32_t IOBurst;                   // The amount of time until the process finishes being blocked
    uint32_t CPUBurst;                  // The CPU availability of the process (has to be > 1 to move to running)

    int32_t quantum;                    // Used for schedulers that utilise pre-emption

    bool isFirstTimeRunning;            // Used to check when to calculate the CPU burst when it hits running mode

    struct Process* nextInBlockedList;  // A pointer to the next process available in the blocked list
    struct Process* nextInReadyQueue;   // A pointer to the next process available in the ready queue
    struct Process* nextInReadySuspendedQueue; // A pointer to the next process available in the ready suspended queue
} _process;


uint32_t CURRENT_CYCLE = 0;             // The current cycle that each process is on
uint32_t TOTAL_CREATED_PROCESSES = 0;   // The total number of processes constructed
uint32_t TOTAL_STARTED_PROCESSES = 0;   // The total number of processes that have started being simulated
uint32_t TOTAL_FINISHED_PROCESSES = 0;  // The total number of processes that have finished running
uint32_t TOTAL_NUMBER_OF_CYCLES_SPENT_BLOCKED = 0; // The total cycles in the blocked state

const char* RANDOM_NUMBER_FILE_NAME= "random-numbers";
const uint32_t SEED_VALUE = 200;  // Seed value for reading from file

// Additional variables as needed


/**
 * Reads a random non-negative integer X from a file with a given line named random-numbers (in the current directory)
 */
uint32_t getRandNumFromFile(uint32_t line, FILE* random_num_file_ptr){
    uint32_t end, loop;
    char str[512];

    rewind(random_num_file_ptr); // reset to be beginning
    for(end = loop = 0;loop<line;++loop){
        if(0==fgets(str, sizeof(str), random_num_file_ptr)){ //include '\n'
            end = 1;  //can't input (EOF)
            break;
        }
    }
    if(!end) {
        return (uint32_t) atoi(str);
    }

    // fail-safe return
    return (uint32_t) 1804289383;
}



/**
 * Reads a random non-negative integer X from a file named random-numbers.
 * Returns the CPU Burst: : 1 + (random-number-from-file % upper_bound)
 */
uint32_t randomOS(uint32_t upper_bound, uint32_t process_indx, FILE* random_num_file_ptr)
{
    char str[20];
    
    uint32_t unsigned_rand_int = (uint32_t) getRandNumFromFile(SEED_VALUE+process_indx, random_num_file_ptr);
    uint32_t returnValue = 1 + (unsigned_rand_int % upper_bound);

    return returnValue;
} 


/********************* SOME PRINTING HELPERS *********************/


/**
 * Prints to standard output the original input
 * process_list is the original processes inputted (in array form)
 */
void printStart(_process process_list[])
{
    printf("The original input was: %i", TOTAL_CREATED_PROCESSES);

    uint32_t i = 0;
    for (; i < TOTAL_CREATED_PROCESSES; ++i)
    {
        printf(" ( %i %i %i %i)", process_list[i].A, process_list[i].B,
               process_list[i].C, process_list[i].M);
    }
    printf("\n");
} 

/**
 * Prints to standard output the final output
 * finished_process_list is the terminated processes (in array form) in the order they each finished in.
 */
void printFinal(_process finished_process_list[])
{
    printf("The (sorted) input is: %i", TOTAL_CREATED_PROCESSES);

    uint32_t i = 0;
    for (; i < TOTAL_FINISHED_PROCESSES; ++i)
    {
        printf(" ( %i %i %i %i)", finished_process_list[i].A, finished_process_list[i].B,
               finished_process_list[i].C, finished_process_list[i].M);
    }
    printf("\n");
} // End of the print final function

/**
 * Prints out specifics for each process.
 * @param process_list The original processes inputted, in array form
 */
void printProcessSpecifics(_process process_list[])
{
    uint32_t i = 0;
    printf("\n");
    for (; i < TOTAL_CREATED_PROCESSES; ++i)
    {
        printf("Process %i:\n", process_list[i].processID);
        printf("\t(A,B,C,M) = (%i,%i,%i,%i)\n", process_list[i].A, process_list[i].B,
               process_list[i].C, process_list[i].M);
        printf("\tFinishing time: %i\n", process_list[i].finishingTime);
        printf("\tTurnaround time: %i\n", process_list[i].finishingTime - process_list[i].A);
        printf("\tI/O time: %i\n", process_list[i].currentIOBlockedTime);
        printf("\tWaiting time: %i\n", process_list[i].currentWaitingTime);
        printf("\n");
    }
} // End of the print process specifics function

/**
 * Prints out the summary data
 * process_list The original processes inputted, in array form
 */
void printSummaryData(_process process_list[])
{
    uint32_t i = 0;
    double total_amount_of_time_utilizing_cpu = 0.0;
    double total_amount_of_time_io_blocked = 0.0;
    double total_amount_of_time_spent_waiting = 0.0;
    double total_turnaround_time = 0.0;
    uint32_t final_finishing_time = CURRENT_CYCLE - 1;
    for (; i < TOTAL_CREATED_PROCESSES; ++i)
    {
        total_amount_of_time_utilizing_cpu += process_list[i].currentCPUTimeRun;
        total_amount_of_time_io_blocked += process_list[i].currentIOBlockedTime;
        total_amount_of_time_spent_waiting += process_list[i].currentWaitingTime;
        total_turnaround_time += (process_list[i].finishingTime - process_list[i].A);
    }

    // Calculates the CPU utilisation
    double cpu_util = total_amount_of_time_utilizing_cpu / final_finishing_time;

    // Calculates the IO utilisation
    double io_util = (double) TOTAL_NUMBER_OF_CYCLES_SPENT_BLOCKED / final_finishing_time;

    // Calculates the throughput (Number of processes over the final finishing time times 100)
    double throughput =  100 * ((double) TOTAL_CREATED_PROCESSES/ final_finishing_time);

    // Calculates the average turnaround time
    double avg_turnaround_time = total_turnaround_time / TOTAL_CREATED_PROCESSES;

    // Calculates the average waiting time
    double avg_waiting_time = total_amount_of_time_spent_waiting / TOTAL_CREATED_PROCESSES;

    printf("Summary Data:\n");
    printf("\tFinishing time: %i\n", CURRENT_CYCLE - 1);
    printf("\tCPU Utilisation: %6f\n", cpu_util);
    printf("\tI/O Utilisation: %6f\n", io_util);
    printf("\tThroughput: %6f processes per hundred cycles\n", throughput);
    printf("\tAverage turnaround time: %6f\n", avg_turnaround_time);
    printf("\tAverage waiting time: %6f\n", avg_waiting_time);
} // End of the print summary data function

#define COMMANDLINE_INPUT_LENGTH 128
uint32_t QUANTUM = 2;
int debug = 1;

/**
 * The magic starts from here
 */
int main(int argc, char *argv[])
{
    uint32_t total_num_of_process; // Read from the file -- number of process to create
    // Other variables
    int A, B, C, M;

    // Write code for your shiny scheduler
    if(debug){
        printf("argc = %i, argv[0] = %s\n", argc, argv[0]);
        printf("argc = %i, argv[1] = %s\n", argc, argv[1]);
    }
    
    if(argc > 1)
    {
        FILE* fp = fopen(argv[1], "r");
        if (fp == NULL){
            printf("%s not found\n", argv[1]);
            return -1;
        }
        FILE* randomFile = fopen(RANDOM_NUMBER_FILE_NAME, "r");
        if (randomFile == NULL){
            printf("%s not found\n", RANDOM_NUMBER_FILE_NAME);
            return -1;
        }
        char fileInput[COMMANDLINE_INPUT_LENGTH];
        fscanf(fp, "%u", &total_num_of_process);
        _process process_list[total_num_of_process]; // Creates a container for all processes
        _process finished_processes[total_num_of_process];
        for (int i = 0; i < total_num_of_process; i++){
            fscanf(fp, " (%i %i %i %i) ", &A, &B, &C, &M);
            if(debug){
                printf("A: %i, B: %i, C: %i, M: %i\n",A,B,C,M);
            }
            process_list[i].A = A;
            process_list[i].B = B;
            process_list[i].C = C;
            process_list[i].M = M;
            process_list[i].processID = i;
            TOTAL_CREATED_PROCESSES += 1;
        }
        fclose(fp);

        // FCFS---------------------------------------------------------------------------------

        TOTAL_STARTED_PROCESSES = TOTAL_FINISHED_PROCESSES = TOTAL_NUMBER_OF_CYCLES_SPENT_BLOCKED = CURRENT_CYCLE =  0;
        initialize_processes(process_list);
        printf("\n######################### START OF FIRST COME FIRST SERVE #########################\n");
        
        FCFS(process_list, finished_processes, randomFile);
        printStart(process_list);
        printFinal(finished_processes);
        printf("\nThe scheduling algorithm used was First Come First Serve\n");
        printProcessSpecifics(process_list);
        printSummaryData(process_list);
        printf("######################### END OF FIRST COME FIRST SERVE #########################");

        // RR ---------------------------------------------------------------------------------

        TOTAL_STARTED_PROCESSES = TOTAL_FINISHED_PROCESSES = TOTAL_NUMBER_OF_CYCLES_SPENT_BLOCKED = CURRENT_CYCLE =  0;
        initialize_processes(process_list);
        printf("\n######################### START OF ROUND ROBIN #########################\n");
        
        RR(process_list, finished_processes, randomFile);
        printStart(process_list);
        printFinal(finished_processes);
        printf("\nThe scheduling algorithm used was Round Robin\n");
        printProcessSpecifics(process_list);
        printSummaryData(process_list);
        printf("######################### END OF ROUND ROBIN #########################");

        //SJF ---------------------------------------------------------------------------------

        TOTAL_STARTED_PROCESSES = TOTAL_FINISHED_PROCESSES = TOTAL_NUMBER_OF_CYCLES_SPENT_BLOCKED = CURRENT_CYCLE =  0;
        initialize_processes(process_list);
        printf("\n######################### START OF SHORTEST JOB FIRST #########################\n");
        
        SJF(process_list, finished_processes, randomFile);
        printStart(process_list);
        printFinal(finished_processes);
        printf("\nThe scheduling algorithm used was Shortest Job First\n");
        printProcessSpecifics(process_list);
        printSummaryData(process_list);
        printf("######################### END OF SHORTEST JOB FIRST #########################\n");

        fclose(randomFile);
    }
    return 0;
}

void initialize_processes(_process process_list[]){
    for (int i = 0; i < TOTAL_CREATED_PROCESSES; i++){
        process_list[i].status = 0;
        process_list[i].finishingTime = -1;
        process_list[i].currentCPUTimeRun = 0;
        process_list[i].currentIOBlockedTime = 0;
        process_list[i].currentWaitingTime = 0;
        process_list[i].IOBurst = 0;
        process_list[i].CPUBurst = 0;
        process_list[i].quantum = 2;
        process_list[i].isFirstTimeRunning = true;
        process_list[i].nextInBlockedList = NULL;
        process_list[i].nextInReadyQueue = NULL;
        process_list[i].nextInReadySuspendedQueue = NULL;
    }
}

void start_process(_process *process, FILE *randomFile){
    if ((*process).isFirstTimeRunning){
        (*process).isFirstTimeRunning = false;
        TOTAL_STARTED_PROCESSES++;
    }
    (*process).status = 2;
    (*process).nextInReadyQueue = (*process).nextInBlockedList = NULL;
    uint32_t time, burst_time;
    if (!(*process).CPUBurst){
        burst_time = randomOS((*process).B, (*process).processID, randomFile);
        time = (*process).C - (*process).currentCPUTimeRun;
        if (time < burst_time){
            (*process).CPUBurst = time;
        }
        else{
            (*process).CPUBurst = burst_time;
            (*process).IOBurst = burst_time * (*process).M;
        }
    }
}

void process_finished(_process* process, _process finished_processes[]){
    _process* finished = &finished_processes[TOTAL_FINISHED_PROCESSES];
    (*finished).A = (*process).A;
    (*finished).B = (*process).B;
    (*finished).C = (*process).C;
    (*finished).M = (*process).M;
    (*finished).processID = (*process).processID;
    (*finished).finishingTime = (*process).finishingTime;
    (*finished).currentCPUTimeRun = (*process).currentCPUTimeRun;
    (*finished).currentIOBlockedTime = (*process).currentIOBlockedTime;
    (*finished).currentWaitingTime = (*process).currentWaitingTime;
    TOTAL_FINISHED_PROCESSES += 1;
}

void block(_process* newBlocked, _process**  oldBlocked){
    (*newBlocked).status = 3;
    if (!(*oldBlocked)){
        *oldBlocked = newBlocked;
        return;
    }
    else if ((**oldBlocked).IOBurst > (*newBlocked).IOBurst){
        (*newBlocked).nextInBlockedList = *oldBlocked;
        *oldBlocked = newBlocked;
        return;
    }
    _process* current = *oldBlocked;
    while ((*current).nextInBlockedList){
        if ((*current).nextInBlockedList->IOBurst > (*newBlocked).IOBurst){
            (*newBlocked).nextInBlockedList = (*current).nextInBlockedList;
            (*current).nextInBlockedList = newBlocked;
            return;
        }
        else{
            current = (*current).nextInBlockedList;
        }
    }
    (*current).nextInBlockedList = newBlocked;
}

void FCFS(_process processes[], _process finished_processes[], FILE* randomFile){
    _process* active = NULL, *current = NULL, *ready = NULL, *blocked = NULL;
    while (TOTAL_FINISHED_PROCESSES < TOTAL_CREATED_PROCESSES){
        if (blocked){
            current = blocked;
            TOTAL_NUMBER_OF_CYCLES_SPENT_BLOCKED += 1;
            while (current){
                (*current).currentIOBlockedTime += 1;
                (*current).IOBurst -= 1;
                if ((*current).IOBurst > 0){
                    current = (*current).nextInBlockedList;
                }
                else{
                    blocked = (*blocked).nextInBlockedList;
                    FCFS_ready(current, &ready);
                    current = blocked;
                }
            }
        }
        for (int i = 0; i < TOTAL_CREATED_PROCESSES; i++){
            if (processes[i].A == CURRENT_CYCLE){
                FCFS_ready(&processes[i], &ready);
            }
        }
        if(active){
            (*active).CPUBurst -= 1;
            (*active).currentCPUTimeRun += 1;
            if ((*active).currentCPUTimeRun == (*active).C){
                (*active).finishingTime = CURRENT_CYCLE;
                (*active).status = 4;
                process_finished(active, finished_processes);
                active = NULL;
            }
            else if ((*active).CPUBurst == 0){
                block(active, &blocked);
                active = NULL;
            }
        }
        if (ready && !active){
            active = ready;
            ready = (*ready).nextInReadyQueue;
            start_process(active, randomFile);
        }
        current = ready;
        while (current){
            (*current).currentWaitingTime += 1;
            current = (*current).nextInReadyQueue;
        }
        CURRENT_CYCLE += 1;
    }
}

void FCFS_ready(_process* newReady, _process** oldReady){
    (*newReady).status = 1;
    if (!(*oldReady)){
        *oldReady = newReady;
        return;
    }
    else{
        _process* current = *oldReady;
        while ((*current).nextInReadyQueue){
            current = (*current).nextInReadyQueue;
        }
        (*current).nextInReadyQueue = newReady;
    }
}

void RR(_process processes[], _process finished_processes[], FILE* randomFile){
    _process* active = NULL, *current = NULL, *ready = NULL, *blocked = NULL, *temp = NULL;
    while (TOTAL_FINISHED_PROCESSES < TOTAL_CREATED_PROCESSES){
        if (blocked){
            TOTAL_NUMBER_OF_CYCLES_SPENT_BLOCKED += 1;
            current = blocked;
            while (current){
                (*current).currentIOBlockedTime += 1;
                (*current).IOBurst -= 1;
                if ((*current).IOBurst > 0){
                    current = (*current).nextInBlockedList;
                }
                else{
                    if (ready){
                        temp = ready;
                        while((*temp).nextInReadyQueue){
                            temp = (*temp).nextInReadyQueue;
                        }
                        (*temp).nextInReadyQueue = current;
                    }
                    else{
                        ready = current;
                    }
                    blocked = (*blocked).nextInBlockedList;
                    current = blocked;
                }
            }
        }
        if (TOTAL_STARTED_PROCESSES < TOTAL_CREATED_PROCESSES){
            for (int i = 0; i < TOTAL_CREATED_PROCESSES; i++){
                if (processes[i].A == CURRENT_CYCLE){
                    RR_ready(&processes[i], &ready);
                }
            }
        }
        if(active){
            (*active).CPUBurst -= 1;
            (*active).quantum -= 1;
            (*active).currentCPUTimeRun += 1;
            if ((*active).currentCPUTimeRun == (*active).C){
                (*active).finishingTime = CURRENT_CYCLE;
                (*active).status = 4;
                process_finished(active, finished_processes);
                active = NULL;
            }
            else if ((*active).CPUBurst == 0){
                (*active).quantum = 2;
                block(active, &blocked);
                active = NULL;
            }
            else if ((*active).quantum == 0){
                if(ready){
                    current = ready;
                    while((*current).nextInReadyQueue){
                        current = (*current).nextInReadyQueue;
                    }
                    (*current).nextInReadyQueue = active;
                    (*active).quantum = 2;
                    (*active).status = 1;
                    active = NULL;
                }
            }
        }
        if (ready && !active){
            active = ready;
            ready = (*ready).nextInReadyQueue;
            start_process(active, randomFile);
        }
        current = ready;
        while (current){
            (*current).currentWaitingTime += 1;
            current = (*current).nextInReadyQueue;
        }
        CURRENT_CYCLE += 1;
    }
}

void RR_ready(_process *newReady, _process **oldReady){
    (*newReady).status = 1;
    if (!(*oldReady)) {
        *oldReady = newReady;
        return;
    }
    else if ((**oldReady).A > (*newReady).A) {
        (*newReady).nextInReadyQueue = *oldReady;
        *oldReady = newReady;
        return;
    }
    else if ((((**oldReady).processID) > (*newReady).processID) && ((*newReady).A == (**oldReady).A)){
        (*newReady).nextInReadyQueue = *oldReady;
        *oldReady = newReady;
        return;
    }
    else{
        _process *current = *oldReady;
        while ((*current).nextInReadyQueue) {
            if ((*current).nextInReadyQueue->A > (*newReady).A) {
                (*newReady).nextInReadyQueue = (*current).nextInReadyQueue;
                (*current).nextInReadyQueue = newReady;
                return;
            }
            else if ((((*current).nextInReadyQueue->processID > (*newReady).processID) && ((*newReady).A == (*current).nextInReadyQueue->A))){
                (*newReady).nextInReadyQueue = (*current).nextInReadyQueue;
                (*current).nextInReadyQueue = newReady;
                return;
            }
            current = (*current).nextInReadyQueue;
        }
        (*current).nextInReadyQueue = newReady;
    }
}

void SJF(_process processes[], _process finished_processes[], FILE* randomFile){
    _process* active = NULL, *current = NULL, *ready = NULL, *blocked = NULL;
    while (TOTAL_FINISHED_PROCESSES < TOTAL_CREATED_PROCESSES){
        if (blocked){
            current = blocked;
            TOTAL_NUMBER_OF_CYCLES_SPENT_BLOCKED += 1;
            while (current){
                (*current).currentIOBlockedTime += 1;
                (*current).IOBurst -= 1;
                if ((*current).IOBurst > 0){
                    current = (*current).nextInBlockedList;
                }
                else{
                    blocked = (*blocked).nextInBlockedList;
                    SJF_ready(current, &ready);
                    current = blocked;
                }
            }
        }
        for (int i = 0; i < TOTAL_CREATED_PROCESSES; i++){
            if (processes[i].A == CURRENT_CYCLE){
                SJF_ready(&processes[i], &ready);
            }
        }
        if(active){
            (*active).CPUBurst -= 1;
            (*active).currentCPUTimeRun += 1;
            if ((*active).currentCPUTimeRun == (*active).C){
                (*active).finishingTime = CURRENT_CYCLE;
                (*active).status = 4;
                process_finished(active, finished_processes);
                active = NULL;
            }
            else if ((*active).CPUBurst == 0){
                block(active, &blocked);
                active = NULL;
            }
        }
        if (ready && !active){
            active = ready;
            ready = (*ready).nextInReadyQueue;
            start_process(active, randomFile);
        }
        current = ready;
        while (current){
            (*current).currentWaitingTime += 1;
            current = (*current).nextInReadyQueue;
        }
        CURRENT_CYCLE += 1;
    }
}

void SJF_ready(_process *newReady, _process **oldReady){
    (*newReady).status = 1;
    if (!(*oldReady)) {
        *oldReady = newReady;
        return;
    }
    else if(((**oldReady).C - (**oldReady).currentCPUTimeRun) > ((*newReady).C - (*newReady).currentCPUTimeRun)){
        (*newReady).nextInReadyQueue = *oldReady;
        *oldReady = newReady;
        return;
    }
    else if (((**oldReady).A > (*newReady).A) && (((**oldReady).C - (**oldReady).currentCPUTimeRun) == ((*newReady).C - (*newReady).currentCPUTimeRun))) {
        (*newReady).nextInReadyQueue = *oldReady;
        *oldReady = newReady;
        return;
    }
    else if ((((**oldReady).processID) > (*newReady).processID) && ((*newReady).A == (**oldReady).A) && (((**oldReady).C - (**oldReady).currentCPUTimeRun) == ((*newReady).C - (*newReady).currentCPUTimeRun))){
        (*newReady).nextInReadyQueue = *oldReady;
        *oldReady = newReady;
        return;
    }
    else{
        _process *current = *oldReady;
        while ((*current).nextInReadyQueue) {
            if (((*current).C - (*current).currentCPUTimeRun) > ((*newReady).C - (*newReady).currentCPUTimeRun)) {
                (*newReady).nextInReadyQueue = (*current).nextInReadyQueue;
                (*current).nextInReadyQueue = newReady;
                return;
            }
            else if (((*current).A > (*newReady).A) && (((*current).C - (*current).currentCPUTimeRun) == ((*newReady).C - (*newReady).currentCPUTimeRun))){
                (*newReady).nextInReadyQueue = (*current).nextInReadyQueue;
                (*current).nextInReadyQueue = newReady;
                return;
            }
            else if ((((*current).processID) > (*newReady).processID) && ((*newReady).A == (*current).A) && (((*current).C - (*current).currentCPUTimeRun) == ((*newReady).C - (*newReady).currentCPUTimeRun))){
                (*newReady).nextInReadyQueue = (*current).nextInReadyQueue;
                (*current).nextInReadyQueue = newReady;
                return;
            }
            current = (*current).nextInReadyQueue;
        }
        (*current).nextInReadyQueue = newReady;
    }
}