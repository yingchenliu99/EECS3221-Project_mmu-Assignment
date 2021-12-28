//Student name: YingChen Liu//
//Student number : //
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define PAGE_TABLE_COUNT 256
#define PAGE_TABLE_SIZE 256
#define TLB_COUNT 16
#define TLB_FRAME_SIZE 256
#define FRAME_COUNT 256


struct TLB {
    int tlb_page_number;
    int tlb_frame_number;
};

struct PageTable {
    int value;
    int valid;
    int waiting_time;
};

//Used to check if the current page_number(logical) data is available in the TLB//
int page_number_available_in_TLB(int page_number);
//Used to search for the corresponding frame number of the current page_number(logical) in TLB.//
int search_frame_number_in_TLB(int page_number);
//Used to check if the current page_number(logical) data is available in the Page Table//
int page_number_available_in_PageTable(int page_number);
//Used to search for the corresponding frame number of the current page_number(logical) in Page Table.//
int search_frame_number_in_PageTable(int page_number);
//Used to update the corresponding frame number of the current page_number(logical) in Page Table.//
void update_PageTable(int page_number, int new_frame_number);
//used to update the corresponding frame number of the current page_number(logical) in TLB.//
void update_TLB(int page_number);
//Used to find the longest unused page number(logical) in the Page Table
int longest_unused_in_pagetable();
//Used to increase the waiting time of data(vaild) in Page Table (used to change data)//
void increase_waiting_time();
//Because the physical memory size is limited, the longest unused page number will become invalid in the Page Table//
void invalue_old_data();
//use for handling page faults when physical address size is 256//
void physical_address_size_256();
//use for handling page faults when physical address size is 128, and proceed Page Replacement//
void physical_address_size_128();

struct TLB tlb[TLB_COUNT];
struct PageTable page_table[PAGE_TABLE_COUNT];
//type1 is used in the physical address space which is 256 page frames.//
char physical_memory_type1[256][256];
//type2 is used in the physical address space which is 128 page frames.//
char physical_memory_type2[256][256];
int total_count = 0;
int hit_count = 0;
int page_fault_count = 0;
int free_frame_index=0;
int tlb_current_index=0;
char *backing_store = "BACKING_STORE.bin";
char *input = "addresses.txt";
char *output;
int physical_address_space=0;

int main(int argc, char *argv[]) {
    //The data is obtained from the input and used to determine the physical address space.//
    int size = atoi(argv[1]);
    if(size == 256) {
        physical_address_space = 256;
        output = "output256.csv";
    }else if(size == 128) {
        physical_address_space = 128;
        output = "output128.csv";
    } 

    //At the beginning, the Page Table is empty, so all data is invalid, and the waiting time is also not available.//
    for(int i = 0; i < PAGE_TABLE_COUNT; i++) {
        page_table[i].valid = 0;
        page_table[i].waiting_time = -1;
    }
    //The length of the logical address is fixed at 8+8 bits//
    char data[16];

    int logical_addresses;
    int offset;
    int logical_page_number;
    int virtual_page_number;
    int frame_number;
 
    FILE *input_file = fopen(input, "r");
    FILE *output_file = fopen(output, "w");

    //Read data from input_file//
    while(fgets(data, 16, input_file) != NULL){
        //Obtain logical address//
        logical_addresses = atoi(data);
        //Positions betweeen 0 to 7 are offset//
		offset = logical_addresses & 0b11111111;
        //Positions between 8 and 15 are page number//
		logical_page_number = (logical_addresses >> 8) & 0b11111111;
        virtual_page_number = logical_addresses & 0b1111111100000000;

        total_count++;
        //The size of offset for physical memory is fixed to 256./
        char physical_memory_offset[FRAME_COUNT];

         //If the current page number appears in TLB, find the corresponding value, and assign it to frame_number//
        if(page_number_available_in_TLB(logical_page_number) == 1){
            frame_number = search_frame_number_in_TLB(logical_page_number);
            //When it appears in TLB, it means a hit, hit count+1//
            hit_count++;
        }
        //If the current page number does not appear in the TLB, look at the Page Table//
        else{
            //If the current page number appears in Page Table, find the corresponding value, and assign it to frame_number//
            if(page_number_available_in_PageTable(logical_page_number) == 1){
                frame_number = search_frame_number_in_PageTable(logical_page_number);
            }
             //The current page number does not appear in the Page  Table//
            else{
                //This means page fault, so page fault count+11//
                page_fault_count++;

                //Get the current page number data from the backing_store.//
                FILE *backing = fopen(backing_store, "r");
                fseek(backing, virtual_page_number, SEEK_SET);
                fread(physical_memory_offset, 1, 256, backing);  
                fclose(backing); 
                
                //If physical_address_space is 256, it means that Page Replacement is not considered.//
                if(physical_address_space == 256){
                    physical_address_size_256(physical_memory_offset, logical_page_number);
                }
               //If physical_address_space is 128, it means that Page Replacement is considered.//
                else if(physical_address_space == 128){
                    physical_address_size_128(physical_memory_offset, logical_page_number);
                }
                
                frame_number = search_frame_number_in_PageTable(logical_page_number);  
            }
        //If logical page number does not appear in the TLB, we need to update the TLB after we get the data from PageTable or BACKING_STORE// 
        update_TLB(logical_page_number);
        }
        //Because the data is used, the waiting time of the corresponding value in the page table is updated to 0.//
        page_table[logical_page_number].waiting_time = 0;
        //Each time the function runs, the waiting time for all vaild values in the page table will be +1//
        increase_waiting_time();

        int physical_addresses= (frame_number << 8) + offset;

        if(physical_address_space==256){
            int signed_byte_value = physical_memory_type1[frame_number][offset];
            fprintf(output_file, "%d,%d,%d\n", logical_addresses, physical_addresses, signed_byte_value);
        }
        else if(physical_address_space==128){
            int signed_byte_value = physical_memory_type2[frame_number][offset];
            fprintf(output_file, "%d,%d,%d\n", logical_addresses, physical_addresses, signed_byte_value);
        }
        
        
    }

    fprintf(output_file,"Page Faults Rate, %.2f%%,\n", (float)page_fault_count / (float)total_count * 100);
	fprintf(output_file,"TLB Hits Rate, %.2f%%,", (float)hit_count / (float)total_count * 100);

    fclose(input_file);
	fclose(output_file);

    return 0;

}

int page_number_available_in_TLB(int page_number){
    for (int i = 0; i < TLB_COUNT; i++) {
        if(tlb[i].tlb_page_number == page_number){
            return 1;
        }
    }

    return 0;
}

int search_frame_number_in_TLB(int page_number){
    int index;

    for (int i = 0; i < TLB_COUNT; i++) {
        if(tlb[i].tlb_page_number == page_number){
            index=i;
        }
    }

    return tlb[index].tlb_frame_number;
}

int page_number_available_in_PageTable(int page_number){
    for(int i = 0; i < PAGE_TABLE_COUNT; i++){
        if(i == page_number && page_table[i].valid==1) {
            return 1;
        }
    }

    return 0;  
}

int search_frame_number_in_PageTable(int page_number){
    int index;
    
    for(int i = 0; i < PAGE_TABLE_COUNT; i++){
        if(i == page_number && page_table[i].valid==1) {
            index=i;
        }
    }

    return page_table[index].value;
}

//Find the position corresponding to the current page_number, assign the data to it, and make it valid.//
void update_PageTable(int page_number, int new_frame_number){
    page_table[page_number].value = new_frame_number;
    page_table[page_number].valid = 1;
    free_frame_index++; 
}

//According to the FIFO algorithm, the page number and offset are placed in the corresponding position.//
void update_TLB(int page_number){
    tlb[tlb_current_index].tlb_page_number = page_number;
	tlb[tlb_current_index].tlb_frame_number = search_frame_number_in_PageTable(page_number);
    tlb_current_index = (tlb_current_index + 1) % TLB_COUNT;
}

//Find the longest unused data index in the pagetable//
int longest_unused_in_pagetable(){
    int longest_unused_data_index = 0;
    for (int i = 0; i < PAGE_TABLE_COUNT; i++) {
        if(page_table[i].waiting_time > page_table[longest_unused_data_index].waiting_time) {
            longest_unused_data_index = i;
        }
    }
    return longest_unused_data_index;
}

//Increase the waiting time for each valid data in the Page Table//
void increase_waiting_time(){
    for(int i = 0; i < PAGE_TABLE_COUNT; i++) {
        if(page_table[i].waiting_time >= 0) {
            page_table[i].waiting_time++;
        }
    }
}

//The longest unused data in the Page Table will make it invalid and remove its waiting time.//
void invalue_old_data(){
    page_table[longest_unused_in_pagetable()].valid = 0;
    page_table[longest_unused_in_pagetable()].waiting_time = -1;   
}

void physical_address_size_256(char memory_offset[], int logical_page_number){
    //When the location is not full, allocate the data from the backing_store directly to main memory. and update the PageTable.//
    for(int i = 0; i < 256; i++) {
        physical_memory_type1[free_frame_index][i] = memory_offset[i];
    }

    update_PageTable(logical_page_number, free_frame_index);
}

void physical_address_size_128(char memory_offset[], int logical_page_number){
    //When the location is not full, allocate the data from the backing_store directly to main memory. and update the PageTable.//
    if(free_frame_index < 128){
        for(int i = 0; i < 256; i++) {
            physical_memory_type2[free_frame_index][i] = memory_offset[i];
        }

        update_PageTable(logical_page_number, free_frame_index);
     }
    //When the position is full, need to consider page replacement//
    else{ 
        //Find the longest unused data according to the algorithm//
        int new_frame_number=page_table[longest_unused_in_pagetable()].value ;
        //Replace the corresponding data with new ones, and update the Page Table//
        for(int i = 0; i < 256; i++) {
            physical_memory_type2[free_frame_index][i] = memory_offset[i];
        }

        update_PageTable(logical_page_number, free_frame_index); 
        //Longest unused data in PageTable, it's valid state change to invaild//
        invalue_old_data();       
    }
}