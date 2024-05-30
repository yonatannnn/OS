#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MEMORY_SIZE 65536
#define MAX_COMMAND_LENGTH 100

typedef struct FreeBlock {
    int start;
    int size;
    struct FreeBlock* next;
} FreeBlock;

typedef struct Process {
    int process_id;
    struct AllocatedBlock* allocated_blocks;
    struct Process* next;
} Process;

typedef struct AllocatedBlock {
    int start;
    int size;
    struct AllocatedBlock* next;
} AllocatedBlock;

typedef struct QueueNode {
    Process* process;
    struct QueueNode* next;
} QueueNode;

typedef struct Queue {
    QueueNode* front;
    QueueNode* rear;
} Queue;

FreeBlock* free_list = NULL;
Process* process_list = NULL;
Queue* waiting_queue = NULL;

void process_next_command();

FreeBlock* initialize_free_list() {
    FreeBlock* head = (FreeBlock*)malloc(sizeof(FreeBlock));
    head->start = 0;
    head->size = MEMORY_SIZE;
    head->next = NULL;
    return head;
}

void enqueue(Queue* queue, Process* process) {
    QueueNode* new_node = (QueueNode*)malloc(sizeof(QueueNode));
    new_node->process = process;
    new_node->next = NULL;
    if (queue->rear == NULL) {
        queue->front = queue->rear = new_node;
    } else {
        queue->rear->next = new_node;
        queue->rear = new_node;
    }
}

Process* dequeue(Queue* queue) {
    if (queue->front == NULL) return NULL;
    QueueNode* temp = queue->front;
    Process* process = temp->process;
    queue->front = queue->front->next;
    if (queue->front == NULL) queue->rear = NULL;
    free(temp);
    return process;
}

int allocate_memory(int process_id, int size) {
    FreeBlock* current = free_list;
    FreeBlock* prev = NULL;

    while (current != NULL) {
        if (current->size >= size) {
            int start_address = current->start;
            current->start += size;
            current->size -= size;

            if (current->size == 0) {
                if (prev == NULL) {
                    free_list = current->next;
                } else {
                    prev->next = current->next;
                }
                free(current);
            }

            Process* process = process_list;
            while (process && process->process_id != process_id) {
                process = process->next;
            }
            if (!process) {
                process = (Process*)malloc(sizeof(Process));
                process->process_id = process_id;
                process->allocated_blocks = NULL;
                process->next = process_list;
                process_list = process;
            }

            AllocatedBlock* new_block = (AllocatedBlock*)malloc(sizeof(AllocatedBlock));
            new_block->start = start_address;
            new_block->size = size;
            new_block->next = process->allocated_blocks;
            process->allocated_blocks = new_block;
            printf("Allocated %d bytes to process %d at address %d.\n", size, process_id, new_block->start);
            process_next_command();
            return start_address;
        }

        prev = current;
        current = current->next;
    }

    Process* process = process_list;
    while (process && process->process_id != process_id) {
        process = process->next;
    }
    if (process) {
        enqueue(waiting_queue, process);
        printf("Process %d is waiting for memory allocation.\n", process_id);
    }
    process_next_command();
    return -1; // Allocation failed
}

void show_process_queue() {
    printf("Process Queue:\n");
    printf("---------------\n");
    QueueNode* current = waiting_queue->front;
    while (current != NULL) {
        printf("Process %d\n", current->process->process_id);
        current = current->next;
    }
    printf("---------------\n");
}

void free_memory(int process_id, int address) {
    Process* process = process_list;
    while (process && process->process_id != process_id) {
        process = process->next;
    }

    if (!process) {
        printf("Process %d not found.\n", process_id);
        return;
    }

    AllocatedBlock* block = process->allocated_blocks;
    AllocatedBlock* prev_block = NULL;

    while (block && block->start != address) {
        prev_block = block;
        block = block->next;
    }

    if (!block) {
        printf("Address %d not allocated to process %d.\n", address, process_id);
        return;
    }

    if (prev_block) {
        prev_block->next = block->next;
    } else {
        process->allocated_blocks = block->next;
    }

    FreeBlock* new_free_block = (FreeBlock*)malloc(sizeof(FreeBlock));
    new_free_block->start = address;
    new_free_block->size = block->size;
    new_free_block->next = free_list;
    free_list = new_free_block;

    free(block);

    // Merge adjacent free blocks
    FreeBlock* current = free_list;
    FreeBlock* next = NULL;

    while (current != NULL) {
        next = current->next;
        while (next != NULL && current->start + current->size == next->start) {
            current->size += next->size;
            current->next = next->next;
            free(next);
            next = current->next;
        }
        current = current->next;
    }

    if (!process->allocated_blocks) {
        Process* temp = process_list;
        Process* prev = NULL;
        while (temp && temp->process_id != process_id) {
            prev = temp;
            temp = temp->next;
        }
        if (prev)
            prev->next = temp->next;
        else
            process_list = temp->next;
        free(temp);
    }

    // Check waiting queue for processes
    Process* waiting_process = dequeue(waiting_queue);
    if (waiting_process) {
        printf("Process %d is no longer waiting and is being allocated memory.\n", waiting_process->process_id);
        allocate_memory(waiting_process->process_id, waiting_process->allocated_blocks->size);
    }
}

void show_memory() {
    printf("Memory Status:\n");
    printf("---------------------------------------------------\n");
    printf("| Start Address | End Address   | Status          |\n");
    printf("---------------------------------------------------\n");

    int current_address = 0;

    FreeBlock* free_current = free_list;

    while (current_address < MEMORY_SIZE) {
        int found = 0;

        if (free_current && free_current->start == current_address) {
            printf("| %12d | %12d | Free             |\n", free_current->start, free_current->start + free_current->size - 1);
            current_address += free_current->size;
            free_current = free_current->next;
            found = 1;
        } else {
            Process* process = process_list;
            while (process) {
                AllocatedBlock* block = process->allocated_blocks;
                while (block) {
                    if (block->start == current_address) {
                        printf("| %12d | %12d | Process %5d    |\n", block->start, block->start + block->size - 1, process->process_id);
                        current_address += block->size;
                        block = NULL;
                        found = 1;
                    } else {
                        block = block->next;
                    }
                }
                process = process->next;
            }
        }

        if (!found) {
            current_address++;
        }
    }

    printf("---------------------------------------------------\n");
}

void create_process(int process_id) {
    Process* new_process = (Process*)malloc(sizeof(Process));
    new_process->process_id = process_id;
    new_process->allocated_blocks = NULL;
    new_process->next = process_list;
    process_list = new_process;
    printf("Process %d created.\n", process_id);
}

void terminate_process(int process_id) {
    Process* process = process_list;
    while (process && process->process_id != process_id) {
        process = process->next;
    }

    if (!process) {
        printf("Process %d not found.\n", process_id);
        return;
    }

    AllocatedBlock* block = process->allocated_blocks;
    while (block) {
        free_memory(process_id, block->start);
        block = block->next;
    }

    printf("Process %d terminated.\n", process_id);
}

void parse_command(char* command) {
    char* token = strtok(command, " ");
    if (strcmp(token, "create") == 0) {
        int process_id = atoi(strtok(NULL, " "));
        create_process(process_id);
    } else if (strcmp(token, "terminate") == 0) {
        int process_id = atoi(strtok(NULL, " "));
        terminate_process(process_id);
    } else if (strcmp(token, "allocate") == 0) {
        int process_id = atoi(strtok(NULL, " "));
        int size = atoi(strtok(NULL, " "));
        int address = allocate_memory(process_id, size);
        if (address != -1) {
            printf("Allocated.\n");
        } else {
            printf("Allocation failed for process %d.\n", process_id);
        }
    } else if (strcmp(token, "free") == 0) {
        int process_id = atoi(strtok(NULL, " "));
        int address = atoi(strtok(NULL, " "));
        free_memory(process_id, address);
        printf("Freed memory at address %d for process %d.\n", address, process_id);
    } else if (strcmp(token, "show") == 0) {
        token = strtok(NULL, " ");
        if (strcmp(token, "memory") == 0) {
            show_memory();
        } else if (strcmp(token, "process") == 0) {
            show_process_queue();
        }
    }
}

void process_next_command() {
    char command[MAX_COMMAND_LENGTH];
    printf("\nEnter command: ");
    if (fgets(command, sizeof(command), stdin) == NULL) return;
    command[strcspn(command, "\n")] =    '\0';  // Remove newline character
    if (strcmp(command, "exit") == 0) return;
    parse_command(command);
}

int main() {
    free_list = initialize_free_list();
    waiting_queue = (Queue*)malloc(sizeof(Queue));
    waiting_queue->front = waiting_queue->rear = NULL;
    char command[MAX_COMMAND_LENGTH];

    printf("Welcome to Memory Management Simulator!\n");
    printf("Available commands:\n");
    printf("create <process_id>: Create a new process\n");
    printf("terminate <process_id>: Terminate an existing process\n");
    printf("allocate <process_id> <size>: Allocate memory for a process\n");
    printf("free <process_id> <address>: Free memory allocated to a process\n");
    printf("show memory: Display memory status\n");
    printf("exit: Exit the simulator\n");

    while (1) {
        process_next_command();
    }
    FreeBlock* current = free_list;
    while (current != NULL) {
        FreeBlock* temp = current;
        current = current->next;
        free(temp);
    }

    Process* process = process_list;
    while (process != NULL) {
        Process* temp = process;
        process = process->next;
        AllocatedBlock* block = temp->allocated_blocks;
        while (block != NULL) {
            AllocatedBlock* temp_block = block;
            block = block->next;
            free(temp_block);
        }
        free(temp);
    }

    QueueNode* node = waiting_queue->front;
    while (node != NULL) {
        QueueNode* temp = node;
        node = node->next;
        free(temp);
    }

    free(waiting_queue);

    return 0;
}


