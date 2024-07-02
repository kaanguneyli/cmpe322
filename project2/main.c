#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include "functions.c"

#define p_count 10
#define MAX_P_LENGTH 15
#define MAX_FILENANME_LENGTH 13
#define MAX_DEF_LINE_LENGTH 50
#define MAX_PROCESS_NUMBER 10

#define filename_def "definition.txt"


int main() {

    // read the file instructions.txt and write the content in an array
    int   instructions[21];
    FILE *instructions_file = fopen("instructions.txt", "r");
    char  line_inst[25];
    if (instructions_file == NULL) {
        printf("File not found.");
    } else {
        int i=0;
        while (fgets(line_inst, sizeof(line_inst), instructions_file) != NULL) {
            strtok(line_inst, " ");
            char *numptr = strtok(NULL, " ");
            char num[5];
            strcpy(num, numptr);
            instructions[i] = atoi(num);
            i++;
        }
    }
    fclose(instructions_file);        


    // read the files P1-P10.txt and write all the instruction number to array
    // merge these arrays in a 2-d array
    int  p_instrs[p_count][MAX_P_LENGTH];
    char line_p[10];   // hardcoded number
    for (int i=0; i<p_count; i++) {
        char filename[MAX_FILENANME_LENGTH];
        sprintf(filename, "P%d.txt", i+1);
        FILE *p_file = fopen(filename, "r");
        if (p_file == NULL) {
            printf("File not found.");
        } else {
            int j=0;
            while (fgets(line_p, sizeof(line_p), p_file) != NULL) {
                if (strcmp(line_p, "exit") == 0) {
                    p_instrs[i][j] = 21;
                } else {
                    char item[2];   // harcoded number
                    strcpy(item, line_p + 5);   // harcoded number
                    p_instrs[i][j] = atoi(item);
                }
                j++;
            }
        }
       fclose(p_file);
    }

    // create an array that will hold the total execution times for each process
    int platinum_times[p_count];
    for (int i=0; i<p_count; i++) {
        int item1 = 0;
        for (int j=0; p_instrs[i][j] != 21; j++)
            item1 += instructions[p_instrs[i][j]-1];
        item1 += 10;   // hardcoded number
        platinum_times[i] = item1;
    }

    FILE *definition_file = fopen(filename_def, "r");
    char  line_def[MAX_DEF_LINE_LENGTH];
    int   definition[MAX_PROCESS_NUMBER][4];   // harcoded number
    int   process_count = 0;


    // read the definition file and write the lines in a 2-d array in an integer format
    if (definition_file == NULL) {
        printf("File not found.");
    } else {
        while (fgets(line_def, sizeof(line_def), definition_file) != NULL) {
            int j=0;
            char *item2 = strtok(line_def, " ");
            item2++;
            char item_new[2];
            while (j < 4) {
                strcpy(item_new, item2);
                if (j == 3) {
                    int num = -1;
                    if (strcmp(item_new, "SILVER\n") == 0 || strcmp(item_new, "SILVER") == 0) 
                        num = 2;
                    else if (strcmp(item_new, "GOLD\n") == 0 || strcmp(item_new, "GOLD") == 0)
                        num = 1;
                    else if (strcmp(item_new, "PLATINUM\n") == 0 || strcmp(item_new, "PLATINUM") == 0)
                        num = 0;
                    definition[process_count][j] = num;
                } else {
                    definition[process_count][j] = atoi(item_new);
                    item2 = strtok(NULL, " ");
                }
                j++;
            }
            process_count++;
        }
    }
    fclose(definition_file);


    
    // we sort the definition array in the order that they will be admitted to the ready queue
    // comparison order: 1-arrival time 2-priority 3-name
    // platinum processes will be handled later
    int min;
    for (int i=0; i<process_count; i++) {
        for (int j=i; j<process_count; j++) {
            if (j==i)
                min = j;    // SON ANDA KIM MIN JAEEEEEEEE
            else if (definition[j][arrival] < definition[min][arrival])
                min = j;
            else if (definition[j][arrival] == definition[min][arrival]) {
                if (definition[j][priority] > definition[min][priority])
                    min = j;
                else if (definition[j][priority] == definition[min][priority]) {
                    int n=2;
                    int m=2;
                    if (definition[j][process] == 10) {
                        n = 3;
                    }
                    if (definition[min][process] == 10) {
                        m = 3;
                    }
                    char jj[n];
                    char mm[m];
                    sprintf(jj, "%d", definition[j][process]);
                    sprintf(mm, "%d", definition[min][process]);
                    if (strcmp(jj, mm) < 0)
                        min = j;
 //                   if (definition[j][process] < definition[min][process]) 
 //                       min = j;
                }
            }  
        }
        swap(4, definition, min, i);   // hardcoded number
    }

    // we create the ready queue which is the most important queue in the project
    // this queue will hold the admitted processes and make us able to keep them in the correct order to execute properly
    // the reason for creating the array 12 times bigger than the process number is that one process can be sent to the queue 9 times at max, i added 3 more just to be safe
    // all list set to 0 for making debugging easier and being safe from the danger of interpreting garbage values as real values 
    int coefficient = 12;   
    int readyqueue[coefficient*process_count][readyqueue_size];   
    for (int i=0; i<coefficient*process_count; i++) {
        for (int j=0; j<readyqueue_size; j++)
            readyqueue[i][j] = 0; 
    }


    int  next_arrival = 0;   // hold the index for the next process to admit to ready queue for the definition array 
    int  head = 0;   // holds the index of the process to be executed on the ready queue, the processes above this index are either exited or sent to the end of the ready queue 
    int  tail = 0;   // holds the index of the ready queue which will take the new process to be admitted, values at this index and after are garbage values (set to 0)
    int  time = 0;
    int  current_instruction;
    int  current_process_count = 0;
    int  completed_process = 0;
    int  arrival_for_result = 0;
    int  total_turnaround_time = 0;
    int  total_waiting_time = 0;
    bool exit = false;              // true when the last process terminates
    bool context_switch = false;    // true when we have to make an context switch
    bool roundrobin = false;        // true if we are in a round robin condition
    bool start = true;              // true in the first iteration


    // loop until all the processes end
    while (completed_process != process_count) {
        // admit the processes which have past their arrival times to the ready queue 
        // add the necessary extra fields to processes
        // burst completed is already 0 bacuse of the way we created the array
        roundrobin = false;
        for (int i=next_arrival; i<process_count; i++) {
            if (definition[i][arrival] <= time) {
                for (int a=0; a<4; a++)
                    readyqueue[tail][a] = definition[i][a];
                readyqueue[tail][instruction_to_execute] = 0;
                if (definition[i][level] == 2) {
                    readyqueue[tail][remaining_quantum] = 80;
                    readyqueue[tail][quantum_to_upgrade] = 3;
                } else if (definition[i][level] == 1) {
                    readyqueue[tail][remaining_quantum] = 120;
                    readyqueue[tail][quantum_to_upgrade] = 5;
                } else {
                    readyqueue[tail][remaining_quantum] = INT_MAX;
                    readyqueue[tail][quantum_to_upgrade] = -1;
                }  
                tail++;
                next_arrival++;
                current_process_count++;
            } else {
                break;
            }
        }
        if (current_process_count == 0) {   // if we are in idle state, to a time that you can get new a process
            time = definition[next_arrival][arrival];
            continue;
        }
        if (exit)   // if the last process exited, we need to make a context switch, so increase the time
            time += 10;
        if (start) {   // increase time by 10 for the context switch of the first iteration
            time +=10;
            start = false;
        }



        // organize the processes in the ready queue according to the execution order between them (except the process on the index head)
        // achieved by swapping the process under when necessary
        for (int i=tail-1; i > head + 1; i--) {
            if (readyqueue[i][level] == 0 && readyqueue[i-1][level] != 0)
                swap(readyqueue_size, readyqueue, i, i-1);
            else if (readyqueue[i][priority] > readyqueue[i-1][priority] && readyqueue[i-1][level] != 0)
                swap(readyqueue_size, readyqueue, i, i-1);
            else if (readyqueue[i][priority] == readyqueue[i-1][priority] && readyqueue[i-1][arrival] > readyqueue[i][arrival] && readyqueue[i-1][level] != 0)
                swap(readyqueue_size, readyqueue, i, i-1);
        }


        // compare the head and the second process when there is more than one process on the ready queue
        if (head+1 != tail) {
            if (readyqueue[head + 1][level] == 0 && readyqueue[head][level] != 0)
                context_switch = true;
            else if (readyqueue[head + 1][priority] > readyqueue[head][priority] && readyqueue[head][level] != 0)
                context_switch = true;
            else if (readyqueue[head + 1][priority] == readyqueue[head][priority]) {
                roundrobin = true;
                if (readyqueue[head][remaining_quantum] <= 0)
                    context_switch = true;
                else if (readyqueue[head][arrival] > readyqueue[head + 1][arrival]) // we are looking at arrival because we update the arrival times to keep track of the round robin order
                    context_switch = true;
            }
        }

        // control the upgrades when there is no context switch
        if ((! context_switch) && readyqueue[head][remaining_quantum] <= 0) {
            readyqueue[head][quantum_to_upgrade]--;
            if (readyqueue[head][quantum_to_upgrade] == 0) // this if unnecesary but prevents the redundant running of the else part, so worth the cost 
                check_and_do_upgrade(readyqueue, head);
            else {
                if (readyqueue[head][level] == 2)
                    readyqueue[head][remaining_quantum] = 80;
                else if (readyqueue[head][level] == 1)
                    readyqueue[head][remaining_quantum] = 120;
                else
                    readyqueue[head][remaining_quantum] = INT_MAX;
            }
        }

        // this handles a small edge case
        // case: current executing process shouldn't yield when finds itself in a round robin situation with a process that has JUST arrived 
        if (roundrobin && context_switch && readyqueue[head + 1][arrival] == time) {
            roundrobin = false;
            context_switch = false;
        } 

        // here we handle the gold processes which are on their last quantum
        // these processes become platinum when they get preempted 
        // we had to handle the cases which occur because of this separately
        if (context_switch && readyqueue[head][level] == 1 && readyqueue[head][quantum_to_upgrade] == 1) {
            if ((readyqueue[head + 1][level] == 0 && (!(readyqueue[head][priority] > readyqueue[head + 1][priority])))
                ||
                (readyqueue[head + 1][level] != 0)) {
                    context_switch = false;
                    readyqueue[head][quantum_to_upgrade]--;
                    check_and_do_upgrade(readyqueue, head);
            }
            if (readyqueue[head + 1][level] == 0) {
                if (! readyqueue[head][priority] > readyqueue[head + 1][priority]) {
                    context_switch = false;
                    readyqueue[head][quantum_to_upgrade]--;
                    check_and_do_upgrade(readyqueue, head);                
                }
            } else {
                context_switch = false;
                readyqueue[head][quantum_to_upgrade]--;
                check_and_do_upgrade(readyqueue, head);
            }
        }


        // we handle the context switches here
        if (context_switch) {
            if (!exit) {   
                // if the last process has exited, we aplly the context switch policy before
                // here we just have to choose the new process to be executed
                // so we should not do these operations
                readyqueue[head][quantum_to_upgrade]--;
                readyqueue[head][arrival] = time;
                time += 10;
            }
            if (readyqueue[head][quantum_to_upgrade] == 0) // this is in the function too, but prevents us from unnecessary use of the else part
                check_and_do_upgrade(readyqueue, head);
            else {
                if (readyqueue[head][level] == 2)
                    readyqueue[head][remaining_quantum] = 80;
                else if (readyqueue[head][level] == 1)
                    readyqueue[head][remaining_quantum] = 120;
                else
                    readyqueue[head][remaining_quantum] = INT_MAX;
            }
            // increase the head by one and send the preempted process to the tail
            for (int i=0; i<readyqueue_size; i++)
                readyqueue[tail][i] = readyqueue[head][i];
            tail++;
            head++;
            context_switch = false;
        }
        if (exit)
            exit = false;

        // prints the ready queue (for debugging)
        //printf("time: %d  head: %d  tail: %d\n", time, head, tail);
        //for (int i = head; i < tail; i++) {
        //    for (int j = 0; j < readyqueue_size; j++) {
        //        printf("%d ", readyqueue[i][j]);
        //    }
        //    printf("\n");
        //}

        // now we execute the process on the headth index of readyqueue
        int time_past;
        if (readyqueue[head][level] == 0) { // if the process is platinum finish all
            time_past = platinum_times[readyqueue[head][process]-1] - readyqueue[head][burst_completed]; // complete the process directly
            current_instruction = 21;
        } else { // if not platinum just execute one instruction
            current_instruction = p_instrs[readyqueue[head][process]-1][readyqueue[head][instruction_to_execute]];
            time_past = instructions[current_instruction-1];
            readyqueue[head][instruction_to_execute]++;
            readyqueue[head][remaining_quantum] -= time_past;
        }
        readyqueue[head][burst_completed] += time_past;
        time += time_past;   // increase the time by the amoun of the cpu burst
        if (current_instruction == 21) {   // exit 
            current_process_count--;
            completed_process++;
            for (int i=0; i<process_count; i++) {   // calculate the turnaround time
                if (readyqueue[head][process] == definition[i][process])
                    arrival_for_result = definition[i][arrival];
            }
            int turnaround = time - arrival_for_result;   
            total_turnaround_time += turnaround;
            total_waiting_time += turnaround - platinum_times[readyqueue[head][process]-1];   // decrese the burst time to find the waiting time
            head++;
            exit = true;
        }
    }
    // print the averages
    float avg_wait = (float) total_waiting_time / (float) process_count;
    float avg_turn = (float) total_turnaround_time / (float) process_count;
    if (avg_wait == (int) avg_wait)
        printf("%d\n", (int) avg_wait);
    else 
        printf("%.1f\n", avg_wait);
    if (avg_turn == (int) avg_turn)
        printf("%d\n", (int) avg_turn);
    else 
        printf("%.1f\n", avg_turn);
}


