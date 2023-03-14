#ifndef __MEMLAB_HXX
#define __MEMLAB_HXX

#include <iostream>
#include <vector>
#include <string>
#include <queue>
#include <unordered_map>
#include <typeinfo>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
using namespace std;

#define MAX_NAME_SIZE 10
#define PAGE_SIZE 4
#define MAX_HOLE_PERC 20 //maximum hole percentage allowed
#define GC_SLEEP_TIME 2 //how frequently should the thread run gc algorithm

//define types
#define INT 17
#define CHAR 18
#define M_INT 19
#define BOOL 20

int get_sizeof_type(char type);
char get_type_char(int type);
extern int shiftbits;
extern int freedentries;
extern char scope_element[];

class m_int {
    char val[3]; //to store the bytes of medium int
public:
    m_int();

    m_int(const m_int &a);
    m_int(int x);
    m_int(char c){}
    m_int(bool bb){}

    //required operations on m_int
    friend m_int operator+(const m_int &a, const m_int &b);
    friend m_int operator-(const m_int &a, const m_int &b);
    friend ostream& operator<<(ostream& os, const m_int& mt);
};

//sample types for type checking
extern int sampint;
extern char sampchar;
extern bool sampbool;
extern m_int sampmint;

class s_entry {
    //0 - scope_elem, 1 - valid, 2 - markbit, 3 - isanarray
    bool specs[4];
    // bool scope_elem; bool valid;
    char type; char name[MAX_NAME_SIZE+1];
    int size; int logicaladdr;
    // bool markbit, isanarray; 
    s_entry* prev; s_entry* next; //implemented as linked list
    int stack_entry;
public:
    s_entry();
    s_entry(char type_,char* name_,int addr,bool isarray_,int size_, bool scope_elem_ = false);

    //functions to get values

    void set_values(char type_,char* name_,int addr,bool isarray_,int size_, bool scope_elem_ = false);

    char get_type();
    char* get_name();
    int get_no_elements();
    int get_offset();
    int get_addr();
    int get_size();

    void add_offset(int x);

    void mark(); 
    void unmark();
    bool get_mark_bit();
    bool isarray();
    bool isvalid();
    bool doesexist();

    void set_stack_entry(int x);
    int get_stack_entry();

    void set_invalid();

    bool is_scope_elem();

    s_entry* get_prev_entry();
    s_entry* get_next_entry();

    void change_prev_entry(s_entry* tothis);
    void change_next_entry(s_entry* tothis);

    //debug function
    void print_entry();
};

class myMem {
    size_t size;  //size of the memory
    void *mem; //memory pointer 

    int freed_mem;

    int next_mem;
    int* page_table; //stores the offset for a specific logical address

    pthread_mutex_t mutex;

    s_entry* first; s_entry* last; //pointers to first and last elements of the symbol table
    
    char* start_st_pointer;
    char* point_to_st;

    //private function to access the actual location in the allocated memory from logical address
    void* getphysicaladdress(int logical_addr);

    //get a free page for allocation or return the same page if a page is already allocated
    void* get_memory(int logical_addr);

    //check if the logical address is currently allocated or not
    bool isallocated(int logical_addr);

    void* get_location(char* name); 
    s_entry* get_s_entry(char* name);

    void gc_initialize(); //add this in createmem
    void gc_run();

    s_entry** var_stack;
    int var_stack_index;

    static void* wrapper(void* self);
    void pop_last_vars();

    void move_pages(char* from, char* to, int no_of_bytes = PAGE_SIZE);
    void updateLogicalAddresses(s_entry* first, int offset);

    void compact();

    int freeElem_gc(s_entry* elem);

    //used to verify if name exists in the current scope
    bool does_exist(char* name);

public:
    myMem();
    myMem(size_t memsize);

    int createMem(size_t size);
    bool isempty();

    void* createVar(char type, char* name); //can be initialized with string as input
    void* createVar(int type, char* name); //can be initialized using the defined types as input

    template <typename T>
    int assignVar(char* name, T value); //to assign values to variables
    int m_int_assignVar(char* name, m_int value);

    void* createArr(char type, char* name, int size_);
    void* createArr(int type,char* name,int size_);

    template <typename T>
    int assignArr(char* name, T value, int index = -1); //index -1 means value is assigned to all elements of the array
    int m_int_assignArr(char* name, m_int value, int index = -1);

    //used for debug purposes
    // void* get_address(char* name);
    int freeElem(char* name);

    //runner function for garbage collection thread
    void* gc_runner();

    void scope_init();
    void scope_end();

    //debug functions
    void print_st();
    void display_empty_frames();
    void print_freed_entries();
    void call_compact();
    void print_final_freed_mem();

    void print_stats();    
};

#endif