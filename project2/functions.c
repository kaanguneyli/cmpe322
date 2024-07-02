#include <limits.h>

#define process 0
#define priority 1
#define arrival 2
#define level 3
#define instruction_to_execute 4
#define remaining_quantum 5
#define quantum_to_upgrade 6
#define burst_completed 7
#define completed_quantum 8
#define readyqueue_size 9


// swap the items of an array on the given indexes
void swap(int size, int arr[][size], int i, int j) {    
    int temp[size];
    for (int a = 0; a < size; a++) {
        temp[a] = arr[i][a];
        arr[i][a] = arr[j][a];
        arr[j][a] = temp[a];
    }
}


void check_and_do_upgrade(int arr[][readyqueue_size], int index) {
    if (arr[index][quantum_to_upgrade] == 0) {   // upgrade condition
        arr[index][level]--;
        if (arr[index][level] == 2) {
            arr[index][quantum_to_upgrade] = 3;
            arr[index][remaining_quantum] = 80;

        } else if (arr[index][level] == 1) {
            arr[index][quantum_to_upgrade] = 5;
            arr[index][remaining_quantum] = 120;

        } else {
            arr[index][quantum_to_upgrade] = -1;
            arr[index][remaining_quantum] = INT_MAX;
        }
    }
}

