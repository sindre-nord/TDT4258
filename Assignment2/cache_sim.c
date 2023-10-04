/* 
Cache-Sim for TDT4255
Author: Sindre Nordtveit 
Date: 2023-09-15

This sim will take some user params that desctibe different cache organizations, mapping
and size. 

Usefull for this development is the following:
 - Wikipedia article on Cache Placement Plicies (https://en.wikipedia.org/wiki/Cache_placement_policies)
 - DLL, Doubly Linked List (https://www.geeksforgeeks.org/introduction-and-insertion-in-a-doubly-linked-list)
*/



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <math.h>

typedef enum {dm, fa} cache_map_t;
typedef enum {uc, sc} cache_org_t;
typedef enum {instruction, data} access_t;

typedef struct {
    uint32_t address;
    access_t accesstype;
} mem_access_t;

typedef struct {
    uint64_t accesses;
    uint64_t hits;
} cache_stat_t;

typedef struct {
    uint32_t tag;
    uint32_t valid;
} cache_line_t;


cache_stat_t cache_stat;

// DECLARE CACHES AND COUNTERS FOR THE STATS HERE
#define MAX_TRACE_FILE_LINE_LENGTH 256
#define ACCESS_TYPE_LENGTH 2
#define ADDRESS_LENGTH 8

#define TRACE_FILE_NAME "mem_trace2.txt"
// #define TRACE_FILE_NAME "testcases/m100hit.txt"
// Declaring constants here
const int block_size = 64;

// Declare global variables here
int cache_size;
cache_map_t cache_map;
cache_org_t cache_org;

int num_of_blocks;
int num_of_bits_for_block_offset = 6; // log_2(block_size) = 6 fixed at 64
int num_of_bits_for_index;
int num_of_bits_for_tag;

// I found this online on how to make a dynamic array in C
// https://www.geeksforgeeks.org/dynamic-array-in-c/
// It seems to what I want.


/* This function returns [bool] true if a value is within an array */
bool contains(int arr[], int size, int value){
    for(int i=0; i<size; i++){
        if(arr[i] == value){
            return true;
        }
    }
    return false;
}

/* Parses all the arguments and returns 1 if an argument is not valid.
It checks both validity sets the values to the appropriate config params
for the program*/
int parse_arguments(int argc, char const *argv[]){
    int correct_amount_of_arguments = 4;
    if(argc != correct_amount_of_arguments){
        printf("Invalid number of arguments. Usage: %s <cache_size> <cache_mapping> <cache_org>\n", argv[0]);
        return 1;
    }
    // Parse the cache size
    cache_size = atoi(argv[1]); //Converts string to int
    int valid_cache_size[6] = {128, 256, 512, 1024, 2048, 4096};
    bool is_valid_cache_size = contains(valid_cache_size, 6, cache_size);
    if(!is_valid_cache_size){
        printf("Invlid cache size. Valid values are 128, 256, 512, 1024, 2048, 4096\n");
        return 1;
    }

    // Parse the cache mapping
    if(strcmp(argv[2], "dm") == 0){
        cache_map = dm;
    } else if(strcmp(argv[2], "fa") == 0){
        cache_map = fa;
    } else {
        printf("Invalid cache mapping. Valid values are 'dm' or 'fa'\n");
        return 1;
    }

    // Parse the cache organization
    if(strcmp(argv[3], "uc") == 0){
        cache_org = uc;
    } else if(strcmp(argv[3], "sc") == 0){
        cache_org = sc;
    } else {
        printf("Invalid cache organization. Valid values are 'uc' or 'sc'\n");
        return 1;
    }

    return 0;
}

/* Calculates all the needed params for the cache sim */
void calculate_config(){
    if(cache_org == sc){
        // Split cache
        cache_size = cache_size/2;
    }
    num_of_blocks = cache_size/block_size;
    num_of_bits_for_block_offset = 6; // log_2(block_size) = 6 fixed at 64
    if(cache_map == dm){
        num_of_bits_for_index = log2(num_of_blocks);
    } else {
        num_of_bits_for_index = 0;
    }
    num_of_bits_for_tag = 32 - num_of_bits_for_index - num_of_bits_for_block_offset; // 32-bit address space
}

/* Print the whole cache config */
void print_cache_config(){
    printf("######## Cache config ########\n");
    if(cache_org == uc){
        printf("Cache organization:\t Unified\n");
        printf("Cache mapping:\t\t %s\n", cache_map == dm ? "Direct Mapped" : "Fully Associative");
        printf("Block size:\t\t %d\n", block_size);
        printf("Cache size:\t\t %d\n", cache_size);
        printf("-----------------------------\n");
        printf("Number of Blocks:\t\t %d (%d/%d) \n", num_of_blocks, cache_size, block_size);
        printf("Number of bits for block offset: %d\n", num_of_bits_for_block_offset);
        printf("Number of bits for the index:\t %d\n", num_of_bits_for_index);
        printf("Number of bits for the tag:\t %d\n", num_of_bits_for_tag);
        printf("-----------------------------\n");
    } else {
        printf("Cache organization: Split\n");
        printf("Cache mapping:\t\t %s\n", cache_map == dm ? "Direct Mapped" : "Fully Associative");
        // The split cache will always split equally between instruction and data
        printf("Block size:\t\t %d / %d\n", block_size, block_size);
        printf("Cache size:\t\t %d / %d\n", cache_size, cache_size);
        printf("-----------------------------\n");
        printf("Number of Blocks:\t\t %d / %d (%d/%d) \n", num_of_blocks, num_of_blocks, cache_size, block_size);
        printf("Number of bits for block offset: %d / %d\n", num_of_bits_for_block_offset, num_of_bits_for_block_offset);
        printf("Number of bits for the index:\t %d / %d\n", num_of_bits_for_index, num_of_bits_for_index);
        printf("Number of bits for the tag:\t %d / %d\n", num_of_bits_for_tag, num_of_bits_for_tag);
        printf("-----------------------------\n");
    }

}



// ########## Start of linked list shenanigans ##########
typedef struct cache_line_node_t { // This could have been made more general, but I like how this is more tailored
    cache_line_t cache_line;
    struct cache_line_node_t* next;
    struct cache_line_node_t* prev;
} cache_line_node_t;

typedef struct cache_line_queue_t {
    cache_line_node_t* head; //Gives you the first item
    cache_line_node_t* tail; // Conveinience feature.
    uint8_t size; //Not currently in use. Might just remove
} cache_line_queue_t;

cache_line_node_t* search_for_block_in(cache_line_queue_t* queue, uint32_t tag){
    // Search for the block in the queue
    cache_line_node_t* current_node = queue->head;
    while(current_node != NULL){
        if(current_node->cache_line.tag == tag){ // We have a hit
            return current_node;
        }
        current_node = current_node->next;
    }
    return NULL; // No hits implicates a miss
}

void pop_back_of_queue(cache_line_queue_t* queue){
    cache_line_node_t* last_node = queue->tail; // This is just to be able to free it later
    queue->tail = last_node->prev;
    queue->tail->next = NULL;
    free(last_node);
    return;
}
// Expects you to malloc a new node and put it into the queu
void push_front_of_queue(cache_line_queue_t* queue, cache_line_node_t* node){
    node->next = queue->head;
    node->prev = NULL;
    queue->head->prev = node;
    queue->head = node;
    return;
}
// Might not be ideal to have this as its own function, it assumes the block is in the queue
void move_to_front(cache_line_queue_t* queue, cache_line_node_t* node){
    // Stitch the nabouring nodes together
    if(node == queue->head){ // In case the node was the head
        return;
    }
    node->prev->next = node->next; // If we havent returned there has to be a prev.
    if (node->next) {
        node->next->prev = node->prev;
    } else { // If there is no next, we were the tail.
        queue->tail = node->prev;
    }

    // Place the node in front
    push_front_of_queue(queue, node);
    return;
}

cache_line_queue_t* init_queue(){
    cache_line_queue_t* new_queue = (cache_line_queue_t*)malloc(sizeof(cache_line_queue_t));
    new_queue->head = NULL;
    new_queue->tail = NULL;
    new_queue->size = 0;

    // Might as well just generate the nodes here.
    for(int i=0; i < num_of_blocks; i++){
        cache_line_node_t* new_line = (cache_line_node_t*)malloc(sizeof(cache_line_node_t));
        new_line->cache_line.tag = 0;
        new_line->cache_line.valid = 0;
        new_line->next = NULL;
        new_line->prev = NULL;
        if(i == 0){
            new_queue->head = new_line;
            new_queue->tail = new_line;
        } else {
            push_front_of_queue(new_queue, new_line);
        }
    }

    return new_queue;
}

void deallocate_queue(cache_line_queue_t* queue){
    cache_line_node_t* current_node = queue->head;
    while(current_node != NULL){
        cache_line_node_t* next_node = current_node->next;
        free(current_node);
        current_node = next_node;
    }
    free(queue);
    return;
}
// ########## End of inked list shenanigans ##########

/* Simulates an associative cache */
void associative_sim(void){
    // The thing that differs from the direct mapped cache is that we
    // dont use the index anymore to find a specific cache line but
    // instead we seach the whole cache-FIFO-queue for a hit. The bottom of the
    // FIFO is ejected on a miss and the requested block is placed on
    // top (Although this is just one policy on how to handle this)

    // After a lot of reading it seems that a doubly linked list might be
    // what we want in this context. As you can stich the list together at any point quite
    // easely. Also it seems like fun things to try.

    if(cache_org == sc){ // Split cache
        cache_line_queue_t* instruction_cache_fifo_queue = init_queue();
        cache_line_queue_t* data_cache_fifo_queue        = init_queue();

        FILE *trace_file;
        trace_file = fopen(TRACE_FILE_NAME, "r");
        if(trace_file == NULL){
            printf("Error opening trace file\n");
            exit(1);
        }

        // Read trace file line by line, \n is the delimiter
        char line[MAX_TRACE_FILE_LINE_LENGTH];
        while(fgets(line, sizeof(line), trace_file)){
            char accesstype[ACCESS_TYPE_LENGTH];
            char    address[ADDRESS_LENGTH];
            sscanf(line, "%s %s", accesstype, address);
            uint32_t address_int = strtoul(address, NULL, 16); // 16 is the base

            mem_access_t mem_access;
            mem_access.address = address_int;
            if(strcmp(accesstype, "I") == 0){
                mem_access.accesstype = instruction;
            } else {
                mem_access.accesstype = data;
            }

            // Calculate the tag only because this is an associative cache
            uint32_t tag = (address_int >> (num_of_bits_for_block_offset)) & ((1 << num_of_bits_for_tag) - 1);
        
            cache_stat.accesses++; // Updates the stats
            // Check if it is a hit or miss
            cache_line_node_t* hit_node;
            if(mem_access.accesstype == instruction){
                hit_node = search_for_block_in(instruction_cache_fifo_queue, tag);
                if(hit_node != NULL){ // The request is a hit
                    cache_stat.hits++; // Updates the stats
                    move_to_front(instruction_cache_fifo_queue, hit_node);
                } else { // The request is a miss
                    pop_back_of_queue(instruction_cache_fifo_queue);
                    cache_line_node_t* new_node = (cache_line_node_t*)malloc(sizeof(cache_line_node_t));
                    new_node->cache_line.tag = tag;
                    new_node->cache_line.valid = 1;
                    push_front_of_queue(instruction_cache_fifo_queue, new_node);
                }
            } else {
                hit_node = search_for_block_in(data_cache_fifo_queue, tag);
                if(hit_node != NULL){ // The request is a hit
                    cache_stat.hits++; // Updates the stats
                    move_to_front(data_cache_fifo_queue, hit_node);
                } else { // The request is a miss
                    pop_back_of_queue(data_cache_fifo_queue);
                    cache_line_node_t* new_node = (cache_line_node_t*)malloc(sizeof(cache_line_node_t));
                    new_node->cache_line.tag = tag;
                    new_node->cache_line.valid = 1;
                    push_front_of_queue(data_cache_fifo_queue, new_node);
                }
            }
        }
    } else { //Unified cache
        cache_line_queue_t* cache_fifo_queue = init_queue();
        // Open the trace file
        FILE *trace_file;
        trace_file = fopen(TRACE_FILE_NAME, "r");
        if(trace_file == NULL){
            printf("Error opening trace file\n");
            exit(1);
        }

        // Read trace file line by line, \n is the delimiter
        char line[MAX_TRACE_FILE_LINE_LENGTH];
        while(fgets(line, sizeof(line), trace_file)){
            char accesstype[ACCESS_TYPE_LENGTH];
            char    address[ADDRESS_LENGTH];
            sscanf(line, "%s %s", accesstype, address);
            uint32_t address_int = strtoul(address, NULL, 16); // 16 is the base

            mem_access_t mem_access;
            mem_access.address = address_int;
            if(strcmp(accesstype, "I") == 0){
                mem_access.accesstype = instruction;
            } else {
                mem_access.accesstype = data;
            }

            // Calculate the tag only because this is an associative cache
            uint32_t tag = (address_int >> (num_of_bits_for_block_offset)) & ((1 << num_of_bits_for_tag) - 1);
        
            cache_stat.accesses++; // Updates the stats
            // Check if it is a hit or miss
            cache_line_node_t* hit_node = search_for_block_in(cache_fifo_queue, tag);
            if(hit_node != NULL){ // The request is a hit
                cache_stat.hits++; // Updates the stats
                move_to_front(cache_fifo_queue, hit_node);
            } else { // The request is a miss
                pop_back_of_queue(cache_fifo_queue);
                cache_line_node_t* new_node = (cache_line_node_t*)malloc(sizeof(cache_line_node_t));
                new_node->cache_line.tag = tag;
                new_node->cache_line.valid = 1;
                push_front_of_queue(cache_fifo_queue, new_node);
            }
        }
        fclose(trace_file);     // Close the trace file
        deallocate_queue(cache_fifo_queue); // Free the memory
    }
}

/* Simulates a direct mapped cache */
void direct_mapped_sim(void){
    if(cache_org == sc){ // Split cache
        //Dynamically allocated array for the instructions
        cache_line_t* instruction_cache = (cache_line_t*)malloc(num_of_blocks * sizeof(cache_line_t));
        // Dynamically allocated array for the data
        cache_line_t* data_cache        = (cache_line_t*)malloc(num_of_blocks * sizeof(cache_line_t));

        if (instruction_cache == NULL || data_cache == NULL) { // Check if the memory was allocated
            printf("Error allocating memory for cache\n");
            exit(1);
        }
        // Initialize the caches
        for(int index=0; index<num_of_blocks; index++){
            instruction_cache[index].tag = 0;
            instruction_cache[index].valid = 0;
            data_cache[index].tag = 0;
            data_cache[index].valid = 0;
        }

        // Feach each line in trace file,
        FILE *trace_file;
        trace_file = fopen(TRACE_FILE_NAME, "r");
        if(trace_file == NULL){
            printf("Error opening trace file\n");
            exit(1);
        }
        
        char line[MAX_TRACE_FILE_LINE_LENGTH];
        while(fgets(line, sizeof(line), trace_file)){
            char accesstype[ACCESS_TYPE_LENGTH];
            char    address[ADDRESS_LENGTH];
            sscanf(line, "%s %s", accesstype, address);
            uint32_t address_int = strtoul(address, NULL, 16); // 16 is the base

            mem_access_t mem_access;
            mem_access.address = address_int;
            if(strcmp(accesstype, "I") == 0){
                mem_access.accesstype = instruction;
            } else {
                mem_access.accesstype = data;
            }
            
            // Calculate the index and tag
            uint32_t index = (address_int >> num_of_bits_for_block_offset) & ((1 << num_of_bits_for_index) - 1);
            uint32_t tag = (address_int >> (num_of_bits_for_block_offset + num_of_bits_for_index)) & ((1 << num_of_bits_for_tag) - 1);

            cache_stat.accesses++; // Updates the stats
            // Check if it is a hit or miss
            if(mem_access.accesstype == instruction){
                if(instruction_cache[index].valid == 1 && instruction_cache[index].tag == tag){ // The request is a hit
                    cache_stat.hits++; // Updates the stats
                } else { // The request is a miss
                    instruction_cache[index].valid = 1;
                    instruction_cache[index].tag = tag;
                }
            } else {
                if(data_cache[index].valid == 1 && data_cache[index].tag == tag){ // The request is a hit
                    cache_stat.hits++; // Updates the stats
                } else { // The request is a miss
                    data_cache[index].valid = 1;
                    data_cache[index].tag = tag;
                }
            }
            /* Perhaps one solution to combine the unified
            and split cache methods would be to let the instruction
            be used to choose an index that somehow relates to a
            pointer. For the unified cache we just use the same pointer
            for the two. This would make the code shorter, but 
            is a bit misleading on what actually happens...*/

        }

        fclose(trace_file);     // Close the trace file
        free(instruction_cache);    // Free the memory
        free(data_cache);           // Free the memory


    } else { // Unified cache
        //Dynamically allocated array
        cache_line_t* unified_cache = (cache_line_t*)malloc(num_of_blocks * sizeof(cache_line_t));
        if (unified_cache == NULL) { // Check if the memory was allocated
            printf("Error allocating memory for cache\n");
            exit(1);
        }
        // Initialize the cache
        for(int index=0; index<num_of_blocks; index++){
            unified_cache[index].tag = 0; // Stricly speaking this is neither needed or correct
            unified_cache[index].valid = 0;
        }

        // Open the trace file
        FILE *trace_file;
        trace_file = fopen(TRACE_FILE_NAME, "r");
        if(trace_file == NULL){
            printf("Error opening trace file\n");
            exit(1);
        }

        // Read trace file line by line, \n is the delimiter
        char line[MAX_TRACE_FILE_LINE_LENGTH];
        while(fgets(line, sizeof(line), trace_file)){
            char accesstype[ACCESS_TYPE_LENGTH];
            char    address[ADDRESS_LENGTH];
            sscanf(line, "%s %s", accesstype, address);
            uint32_t address_int = strtoul(address, NULL, 16); // 16 is the base


            // Create a mem_access_t struct
            // This is not needed for a unified cache, but it will be
            // relevant for the split cache. Might combine the two!
            mem_access_t mem_access;
            mem_access.address = address_int;
            if(strcmp(accesstype, "I") == 0){
                mem_access.accesstype = instruction;
            } else {
                mem_access.accesstype = data;
            }

            // Calculate the index and tag
            uint32_t index = (address_int >> num_of_bits_for_block_offset) & ((1 << num_of_bits_for_index) - 1);
            uint32_t tag = (address_int >> (num_of_bits_for_block_offset + num_of_bits_for_index)) & ((1 << num_of_bits_for_tag) - 1);

            cache_stat.accesses++; // Updates the stats
            // Check if it is a hit or miss
            if(unified_cache[index].valid == 1 && unified_cache[index].tag == tag){ // The request is a hit
                cache_stat.hits++; // Updates the stats
            } else { // The request is a miss
                unified_cache[index].valid = 1;
                unified_cache[index].tag = tag;
            }
        }
        fclose(trace_file);     // Close the trace file
        free(unified_cache);    // Free the memory
    }
    
}

/* Prints the cache stats */
void print_cache_stats(void){
    // printf("######## Cache stats ########\n");
    // printf("Accesses:\t %llu\n", cache_stat.accesses);
    // printf("Hits:\t\t %llu\n", cache_stat.hits);
    // printf("Misses:\t\t %llu\n", cache_stat.accesses - cache_stat.hits);
    // printf("Hit rate:\t %f\n", (double)cache_stat.hits/cache_stat.accesses);
    // printf("Miss rate:\t %f\n", (double)(cache_stat.accesses - cache_stat.hits)/cache_stat.accesses);
    // printf("#############################\n");

    //Previously un-provided-now-provided-print-syntax...
    printf("\nCache Statistics\n");
    printf("-----------------\n\n");
    printf("Accesses: %llu\n", cache_stat.accesses);
    printf("Hits:     %llu\n", cache_stat.hits);
    printf("Hit Rate: %.4f\n",
            (double)cache_stat.hits / cache_stat.accesses);
    }

int main(int argc, char const *argv[])
{   
    // Parse all the arguments.
    if(parse_arguments(argc, argv)){
        printf("Exiting...\n");
        return 1; // Returns with an error
    }

    // Clear the terminal
    // system("clear");
    // printf("######## Cache sim ########\n");

    // Calculate the configuration
    calculate_config();
    // Print the configuration
    // print_cache_config();

    cache_stat.accesses = 0;
    cache_stat.hits = 0;

    if(cache_map == dm){
        direct_mapped_sim();
    } else {
        associative_sim();
    }

    print_cache_stats();
    

    return 0;
}

