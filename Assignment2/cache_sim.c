#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>

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


int parse_arguments(int argc, char const *argv[]){
    if(argc != 4){
        printf("Invalid number of arguments. Usage: %s <cache_size> <cache_mapping> <cache_org>\n", argv[0]);
    }
    // Parse the cache size
    cache_size = atoi(argv[1]); //Converts string to int

    return 0;
}

int main(int argc, char const *argv[])
{
    if(parse_arguments(argc, argv) != 0){
        return 1; // Returns with an error
    }


    return 0;
}

