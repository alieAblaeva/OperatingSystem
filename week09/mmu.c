#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <signal.h>
#include <sys/types.h>

#define MAX_LINE 100

int num_pages;

struct PTE {
    int valid;
    int frame;
    int dirty;
    int referenced;
    int counter_nfu;
    unsigned char reference_bits;
};

void ageing_setting(struct PTE* page_table, int number_of_pages) {
    for (int i = 0; i < number_of_pages; i++) {
        page_table[i].reference_bits >>= 1; 
        if (page_table[i].referenced) {
            page_table[i].reference_bits |= 0x80; 
        } 
    }
}

int main(int argc, char *argv[]) {
    int number_of_requests = argc-3;
    int number_of_hits = 0;
    
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <num_pages> <reference_string> <pager_pid>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    num_pages = atoi(argv[1]);
    //char *reference_string = argv[2];
    int pager_pid = atoi(argv[argc - 1]);

    int fd = open("/tmp/ex2/pagetable", O_RDWR, 0666);
    if (fd == -1) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    struct PTE *page_table = mmap(NULL, num_pages * sizeof(struct PTE), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (page_table == MAP_FAILED) {
        perror("Error mapping the file");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < argc - 3; i++) {
        char mode = argv[i+2][0];
        int page = atoi(argv[i+2] + 1);
        char m[10];
        if(mode == 'R'){
        	strcpy(m, "Read");
        } else
        	strcpy(m, "Write");
	printf("%s Request for page %d\n", m, page);
	page_table[page].referenced = 1;
	ageing_setting(page_table, num_pages);
	page_table[page].counter_nfu++;
        if (page_table[page].valid == 0) {
            printf("It is not a valid page --> page fault\n");
            printf("Ask pager to load it from disk (SIGUSR1 signal) and wait\n");
            printf("-------------------------\n");
            
            page_table[page].referenced = getpid();
            kill(pager_pid, SIGUSR1);
            kill(getpid(), SIGSTOP);
            printf("MMU resumed by SIGCONT signal from pager\n");
        } else{
        	printf("It is a valid page\n");
        	number_of_hits++;
	}
        if (mode == 'W') {
            page_table[page].dirty = 1;
        }
	
        for(int i = 0; i<num_pages; i++){
        	page_table[i].referenced = 0;
        }
        
        printf("Page table\n");
        for (int j = 0; j < num_pages; j++) {
            printf("Page %d ---> valid=%d, frame=%d, dirty=%d, referenced=%d\n", j, page_table[j].valid,
                   page_table[j].frame, page_table[j].dirty, page_table[j].referenced);
                  
        }
        printf("-------------------------\n");
        printf("\n");
    }

    close(fd);
    printf("MMU sends SIGUSR1 to the pager.\n");
    kill(pager_pid, SIGUSR1);
    sleep(2);
    printf("Hits: %d\nMisses: %d\nHit Ratio: %f\n", number_of_hits, number_of_requests-number_of_hits, (float)number_of_hits/(float)number_of_requests);
    printf("MMU terminates.\n");

    return 0;
}

