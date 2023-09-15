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
    // You can declare additional statistics if
    // you like, however you are now allowed to
    // remove the accesses or hits
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

// Declaring constants here
int block_size = 64;

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
    num_of_bits_for_index = log2(num_of_blocks);
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

/* Simulates an associative cache */
void associative_sim(void){

}

/* Simulates a direct mapped cache */
void direct_mapped_sim(void){
    // Initialize the stats

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
        trace_file = fopen("mem_trace2.txt", "r");
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
        trace_file = fopen("mem_trace2.txt", "r");
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
    printf("######## Cache stats ########\n");
    printf("Accesses:\t %llu\n", cache_stat.accesses);
    printf("Hits:\t\t %llu\n", cache_stat.hits);
    printf("Misses:\t\t %llu\n", cache_stat.accesses - cache_stat.hits);
    printf("Hit rate:\t %f\n", (double)cache_stat.hits/cache_stat.accesses);
    printf("Miss rate:\t %f\n", (double)(cache_stat.accesses - cache_stat.hits)/cache_stat.accesses);
    printf("#############################\n");
}

int main(int argc, char const *argv[])
{   
    // Parse all the arguments.
    if(parse_arguments(argc, argv) == 1){
        return 1; // Returns with an error
    }

    // Clear the terminal
    system("clear");
    printf("######## Cache sim ########\n");

    // Calculate the configuration
    calculate_config();
    // Print the configuration
    print_cache_config();

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

