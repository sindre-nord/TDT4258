#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>

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


// DECLARE CACHES AND COUNTERS FOR THE STATS HERE

// Declaring constants here
int block_size = 64;

// Declare global variables here
int cache_size;
cache_map_t cache_map;
cache_org_t cache_org;

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

int main(int argc, char const *argv[])
{
    // Parse all the arguments.
    if(parse_arguments(argc, argv) == 1){
        return 1; // Returns with an error
    }

    



    return 0;
}

