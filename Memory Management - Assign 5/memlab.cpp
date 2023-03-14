#include "memlab.h"

int sampint;
char sampchar;
bool sampbool;
m_int sampmint;
int shiftbits;
int freedentries = 0;
char scope_element[] = "***";


int get_sizeof_type(char type) {
    //get size of type 
    if(type == 'i') return sizeof(sampint);
    if(type == 'c') return sizeof(sampchar);
    if(type == 'b') return sizeof(sampbool);
    if(type == 'm') return sizeof(sampmint);
    else return 0;
}

char get_type_char(int type) {
    //get type in char from int
    if(type == INT) return 'i';
    if(type == CHAR) return 'c';
    if(type == BOOL) return 'b';
    if(type == M_INT) return 'm';
    return 'e';
}

m_int::m_int() {}

m_int::m_int(int value){

    val[2] = value & 0xFF ;
    val[1] = (value >> 8) & 0xFF ;
    val[0] = (value >> 16) & 0xFF ;   
}

m_int::m_int(const m_int &a){
    
    val[2] = a.val[2];
    val[1] = a.val[1];
    val[0] = a.val[0];  
}

void myMem::gc_run() {
    //lock before running garbage collection
    pthread_mutex_lock(&mutex);
    
    cout << "Garbage collection algorithm has started" << "\n";

    s_entry* start_ptr  = first;

    for(int i=0; i < var_stack_index; i++) {
        if(var_stack[i]==NULL) {
            continue;
        }
        var_stack[i]->mark();
    }

    cout << "Marking of all stack entries has been done" << "\n";

    while(1) {

        if(start_ptr == NULL) break;

        s_entry* n = start_ptr->get_next_entry();

        if(start_ptr->get_mark_bit() == false) {
            s_entry *old = start_ptr;
            start_ptr = start_ptr->get_next_entry();
            freeElem_gc(old); 
        }

        if(n== NULL) break;

        start_ptr = n;
    }

    cout << "Freeing elements is done" << "\n";
    
    start_ptr = first;

    while(start_ptr != last){

        start_ptr->unmark();
        start_ptr = start_ptr->get_next_entry();
    }
    if(start_ptr!=NULL) start_ptr->unmark();

    cout << "Unmarking of the remaining elements has been done" << "\n";

    // checking to see if we can run compact or not
    if(next_mem !=0 && ((double)(freed_mem)/next_mem)*100.0 >= MAX_HOLE_PERC) {
        // cout << "---------------------------------------- Hole percentage: " << ((double)(freed_mem)/next_mem)*100.0 << "-----------------------" << "\n";
        cout << "As the hole percentage is " << ((double)(freed_mem)/next_mem)*100.0 << " compact() is being run" << "\n";
        compact();
        cout << "Compaction has finished" << "\n";
    }

    pthread_mutex_unlock(&mutex);
}

void* myMem::wrapper(void* self) {
    return static_cast<myMem*>(self)->gc_runner();
}

void* myMem::gc_runner() {
    while(1){
        sleep(GC_SLEEP_TIME);
        cout << "GC is being run from the thread" << "\n";
        gc_run();
    }
}

s_entry::s_entry() {}

s_entry::s_entry(char type_, char* name_, int addr, bool isarray_, int size_, bool scope_elem_) {
    type = type_;   
    // isanarray = isarray_;
    specs[3] = isarray_;
    strcpy(name,name_);   size = size_;
    logicaladdr = addr; 
    // markbit = false; 
    specs[2] = false;
    // scope_elem = scope_elem_; valid = true;
    specs[0] = scope_elem_; specs[1] = true;
    stack_entry = -1; //nascent entry, not in stack yet
}

void s_entry::set_values(char type_, char* name_, int addr, bool isarray_, int size_, bool scope_elem_) {
    type = type_;   
    // isanarray = isarray_;
    specs[3] = isarray_;
    strcpy(name,name_);   size = size_;
    logicaladdr = addr; 
    // markbit = false; 
    specs[2] = false;
    // scope_elem = scope_elem_; valid = true;
    specs[0] = scope_elem_; specs[1] = true;
    stack_entry = -1; //nascent entry, not in stack yet
}

//various methods for accessing or setting values in s_entry
char s_entry::get_type() {
    return type;
}

int s_entry::get_stack_entry() {
    return stack_entry;
}

void s_entry::set_stack_entry(int x) {
    stack_entry = x;
}

void s_entry::add_offset(int x) {
    logicaladdr += x;
}

char* s_entry::get_name() {
    return name;
}

int s_entry::get_no_elements() {
    return (size / get_sizeof_type(type));
}

void s_entry::mark() {
    // markbit = true;
    specs[2] = true;
}

int s_entry::get_size() {
    return size;
}

void s_entry::unmark() {
    // markbit = false;
    specs[2] = false;
}

int s_entry::get_addr() {
    return logicaladdr;
}

bool s_entry::get_mark_bit() {
    // return markbit;
    return specs[2];
}

bool s_entry::isvalid() {
    // return valid;
    return specs[1];
}

bool s_entry::isarray() {
    // return isanarray;
    return specs[3];
}

bool s_entry::is_scope_elem() {
    // return scope_elem;
    return specs[0];
}

s_entry* s_entry::get_prev_entry() {
    return prev;
}

s_entry* s_entry::get_next_entry() {
    return next;
}

void s_entry::set_invalid() {
    // valid = false;
    specs[1] = false;
}

void s_entry::change_prev_entry(s_entry* tothis) {
    prev = tothis;
}

void s_entry::change_next_entry(s_entry* tothis) {
    next = tothis;
}

//printing the s_entry
void s_entry::print_entry() {
    cout << "Printing symbol table entry with var name: " << name << endl;
    cout << "Type: " << type << ", Size: " << size << endl;
    cout << "Markbit: " << specs[2] << ", Is Array: " << specs[3] << endl;
    cout << "Logical Address: " << logicaladdr << endl;
    if(specs[0]) cout << "It is a scope element" << endl;
}

//constructor for myMem class
myMem::myMem() {
    next_mem = 0; shiftbits = 0;
    while(pow(2,shiftbits)!=PAGE_SIZE) shiftbits++;
    first = NULL; last = NULL;
}

//constructor with size
myMem::myMem(size_t memsize) {
    size = memsize; next_mem = 0;
    shiftbits = 0;
    while(pow(2,shiftbits)!=PAGE_SIZE) shiftbits++;
    createMem(memsize);
    first = NULL; last = NULL;
}

//createMem function
int myMem::createMem(size_t memsize) {
    size = memsize; //set size

    mem = malloc(memsize + (PAGE_SIZE - memsize%PAGE_SIZE)%PAGE_SIZE); //allocate a multiple of page size
    if(mem==NULL && memsize!=(size_t)0) return -1; //unsuccessful allocation
    
    //totalsize of the required book_keeping structs
    size_t totalsize = sizeof(int)*(memsize / PAGE_SIZE + 1) + sizeof(s_entry)*((memsize+3)/4) + ((size + 3)/4)* sizeof(s_entry*);
    
    //allocate another single space for book_keeping structs
    char *orig = (char*)malloc(totalsize);

    //split into page table and symbol table
    page_table = (int*)orig;
    point_to_st = orig + sizeof(int)*(memsize / PAGE_SIZE + 1);

    //store the start to the symbol table location
    start_st_pointer = point_to_st;

    //check if memory has been allocated properly
    if(orig == NULL) return -1;
    if(page_table == NULL || point_to_st == NULL) return -1;

    //set default values for page_table
    for(int i=0;i<(memsize / PAGE_SIZE + 1);i++) page_table[i] = 0;

    gc_initialize(); //initialize the garbage collector

    //initialize the mutex lock
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&mutex,&attr);

    //currently freed memory is 0
    freed_mem = 0;

    cout << "Finished creating memory with size " << size << endl;
    cout << "Space allocated for bookkeeping structures is " << totalsize << endl;
    // cout << "Size of s_entry is " << sizeof(s_entry) << endl;

    return 0; //successful allocation
}

void* myMem::getphysicaladdress(int logical_addr) {
    //return the physical address of a logical address
    return (void*)(page_table[logical_addr >> shiftbits]+(char*)mem);
}

bool myMem::isallocated(int logical_addr) {
    //check if the memory is allocated or not
    if(page_table[logical_addr >> shiftbits]<=0) return false;
    return true;
}

void* myMem::get_memory(int logical_addr) {
    //return allocated memory if it is, otherwise allocate a new page
    if(isallocated(logical_addr)) return getphysicaladdress(logical_addr);
    if(next_mem >= size) return NULL;

    cout << "Allocating new memory for logical address " << logical_addr << "\n";
    // cout << "Setting value of " << (logical_addr >> shiftbits) << " to " << next_mem << endl;
    page_table[logical_addr >> shiftbits] = next_mem;
    // next_mem += 4;
    return (void*)(next_mem + (char*)mem);
}

bool myMem::isempty() {
    //check if allocated space is empty(error with malloc)
    if(mem==NULL) return true;
    return false;
}

bool myMem::does_exist(char* name) {
    s_entry* curr = last;
    while((curr != NULL && strcmp(name,curr->get_name())!=0 && !curr->is_scope_elem())) 
        curr = curr->get_prev_entry();

    if(curr==NULL) return false;
    if(curr!=NULL && curr->is_scope_elem()) return false;
    return true;
}

void* myMem::createVar(char type, char* name) {
    //lock before creating
    pthread_mutex_lock(&mutex);
    if(does_exist(name)) {
        cout << "A variable with the name already exists in the current scope" << endl;
        pthread_mutex_unlock(&mutex);
        return NULL;
    }

    if(size - next_mem < 4 && size - (next_mem-freed_mem) >= 4) {
        cout << "There is not enough space to allocate without compaction, so compacting first" << endl;
        compact();
    }
    
    void* curr = get_memory(next_mem); //allocate memory
    if(curr==NULL) {
        //get memory returns null in case of insufficient memory
        cout << "Insufficient memory to create the variable " << name << endl;
        pthread_mutex_unlock(&mutex);
        return NULL;
    }

    cout << "Creating a variable of type " << "\'" << type << "\'" << endl;
    //size of all variables is atmost 4 bytes
    next_mem += 4; 

    //to enter the variable into the symbol table
    s_entry* tempv;

    //check if there are any holes in symbol table
    if(freedentries>0) {
        char* tempptr = start_st_pointer;
        tempv = (s_entry*)tempptr;
        while(tempv->isvalid()) {
            tempptr += sizeof(s_entry);
            tempv = (s_entry*)tempptr;
        }
        tempv->change_next_entry(NULL);
        tempv->change_prev_entry(NULL);
        freedentries--;
    }

    else {
        tempv = (s_entry*)point_to_st;

        point_to_st = point_to_st + sizeof(s_entry);
    }

    //set values
    tempv->set_values(type,name,next_mem - 4, false,get_sizeof_type(type));
    
    //add to the list at the end
    tempv->change_prev_entry(last);
    if(last!=NULL) last->change_next_entry(tempv);
    else first = tempv;
    last = tempv;

    cout << "Finished pushing the variable " << name << " into the symbol table" << endl;

    //push it into the stack, as it is referenced now
    if(tempv->get_stack_entry()==-1) {
        var_stack[var_stack_index++] = tempv;
        tempv->set_stack_entry(var_stack_index-1);
    }

    pthread_mutex_unlock(&mutex);

    return curr; //return 1 if succesfully created the variable
}

void* myMem::createArr(char type, char* name, int size_) {
    //lock before creating the array
    pthread_mutex_lock(&mutex);
    if(does_exist(name)) {
        cout << "A variable with the name already exists in the current scope" << endl;
        pthread_mutex_unlock(&mutex);
        return NULL;
    }

    int actualsize = size_*get_sizeof_type(type);

    if(size - next_mem < actualsize && size - (next_mem-freed_mem) >= actualsize) {
        cout << "There is not enough space to allocate without compaction, so compacting first" << endl;
        compact();
    }

    void* curr; int i;
    void* firstptr; int firstlogicaladdr;

    cout << "Creating an array of type " << type << " with " << size_ << " number of elements" << endl;

    //get memory for the whole array
    for(i=0;i<size_;i++) {
        curr = get_memory(next_mem);
        if(i==0) {
            //set address and pointer
            firstptr = curr;
            firstlogicaladdr = next_mem;
        }
        if(curr == NULL) {
            cout << "Insufficient memory, allocated only until index " << i << endl;
            cout << "Please free this variable and others to make enough space" << endl;
            pthread_mutex_unlock(&mutex);
            return first;
        }
        //increase based on size of the tye
        next_mem += get_sizeof_type(type);
    }

    cout << "Currently performing word alignment, from " << next_mem;
    next_mem += (4-next_mem%4)%4; //word alignment
    cout << " to " << next_mem << endl;

    s_entry* tempv;

    if(freedentries>0) {
        char* tempptr = start_st_pointer;
        tempv = (s_entry*)tempptr;
        while(tempv->isvalid()) {
            tempptr += sizeof(s_entry);
            tempv = (s_entry*)tempptr;
        }
        tempv->change_next_entry(NULL);
        tempv->change_prev_entry(NULL);
        freedentries--;
    }

    else {
        tempv = (s_entry*)point_to_st;

        point_to_st = point_to_st + sizeof(s_entry);
    }

    tempv->set_values(type,name,firstlogicaladdr,true,(size_*get_sizeof_type(type)));

    tempv->change_prev_entry(last);
    if(last!=NULL) last->change_next_entry(tempv);
    else first = tempv;
    last = tempv;

    cout << "Finished adding " << name << " to symbol table " << endl;

    if(tempv->get_stack_entry()==-1) {
        var_stack[var_stack_index++] = tempv;
        tempv->set_stack_entry(var_stack_index-1);
    }

    pthread_mutex_unlock(&mutex);
    return firstptr;
}

void* myMem::createArr(int type,char* name,int size_) {
    //different arguments, use the previous function
    return createArr(get_type_char(type),name,size_);
}

void* myMem::createVar(int type, char* name) {
    //use the previous function
    return createVar(get_type_char(type),name);
}

void* myMem::get_location(char* name) {
    //get location inside the memory, provided for the sake of the user
    //extra functionality not mentioned in the assignment
    s_entry* curr = last;
    while(curr != NULL && strcmp(name,curr->get_name())!=0) curr = curr->get_prev_entry();
    if(curr==NULL) return NULL;
    return getphysicaladdress(curr->get_addr());
}

s_entry* myMem::get_s_entry(char* name) {
    //get the symbol table entry corresponding to a certain name
    s_entry* curr = last; //loop from the back 
    while(curr != NULL && strcmp(name,curr->get_name())!=0) curr = curr->get_prev_entry();
    return curr; //can return NULL
}

//template function for assigning variable values
template <typename T>
int myMem::assignVar(char* name, T value) {
    //lock before changing stuff
    pthread_mutex_lock(&mutex);
    s_entry* elem = get_s_entry(name);

    if(elem == NULL) {
        //check if it exists or not
        cout << "Variable requested for doesn't exist" << endl;
        pthread_mutex_unlock(&mutex);
        return -1;
    }

    cout << "Assigning value " << value << " to variable " << name << endl;

    //get the physical address as we are assigning value
    void* curr = getphysicaladdress(elem->get_addr());

    cout << "Got the address of the variable, time to update" << endl;

    //for type checking
    const type_info& t1 = typeid(T);

    //for int type
    if(elem->get_type()=='i') {
        const type_info &t2 = typeid(sampint);
        if(t2!=t1) {
            //mismatched types
            cout << "Invalid types for name " << name << endl;
            pthread_mutex_unlock(&mutex);
            return -1;
        }

        //typecast it as int pointer to assign value
        int* currr = (int*)curr;
        *currr = value;

        cout << "Finished updating the value" << endl;

        //add to stack as it is referenced now
        if(elem->get_stack_entry()==-1) {
            var_stack[var_stack_index++] = elem;
            elem->set_stack_entry(var_stack_index-1);
        }

        pthread_mutex_unlock(&mutex);
        return 1; //return 1 if successful
    }

    else if(elem->get_type()=='c') {
        const type_info &t2 = typeid(sampchar);
        if(t2!=t1) {
            cout << "Invalid types" << endl;
            pthread_mutex_unlock(&mutex);
            return -1;
        }
        char* currr = (char*)curr;
        *currr = value;

        cout << "Finished updating the value" << endl;

        if(elem->get_stack_entry()==-1) {
            var_stack[var_stack_index++] = elem;
            elem->set_stack_entry(var_stack_index-1);
        }

        pthread_mutex_unlock(&mutex);
        return 1;
    }

    else if(elem->get_type()=='b') {
        const type_info &t2 = typeid(sampbool);
        if(t2!=t1) {
            cout << "Invalid types" << endl;
            pthread_mutex_unlock(&mutex);
            return -1;
        }
        bool* currr = (bool*)curr;
        *currr = value;

        cout << "Finished updating the value" << endl;

        if(elem->get_stack_entry()==-1) {
            var_stack[var_stack_index++] = elem;
            elem->set_stack_entry(var_stack_index-1);
        }

        pthread_mutex_unlock(&mutex);
        return 1;
    }

    else if(elem->get_type()=='m') {
        const type_info &t2 = typeid(sampmint);
        if(t2!=t1) {
            cout << "Invalid types" << endl;
            pthread_mutex_unlock(&mutex);
            return -1;
        }
        m_int* currr = (m_int*)curr;
        *currr = value;

        cout << "Finished updating the value" << endl;

        if(elem->get_stack_entry()==-1) {
            var_stack[var_stack_index++] = elem;
            elem->set_stack_entry(var_stack_index-1);
        }

        pthread_mutex_unlock(&mutex);
        return 1;
    }
    else {
        pthread_mutex_unlock(&mutex);
        return -1;
    }
}

//declare template for 3 types, we use another function for medium int
template int myMem::assignVar<int>(char* name, int value);
template int myMem::assignVar<char>(char* name, char value);
template int myMem::assignVar<bool>(char* name, bool value);

//medium int assign value
int myMem::m_int_assignVar(char* name, m_int value) {
    //lock
    pthread_mutex_lock(&mutex);
    s_entry* elem = get_s_entry(name);
    if(elem == NULL) {
        cout << "Variable requested for doesn't exit" << endl;
        pthread_mutex_unlock(&mutex);
        return -1;
    }

    cout << "Assigning value " << value << " to variable " << name << endl;

    void* curr = getphysicaladdress(elem->get_addr());

    cout << "Finished retreiving the address of the variable " << name << endl;

    const type_info& t1 = typeid(sampmint);

    if(elem->get_type()=='m') {
        const type_info &t2 = typeid(sampmint);
        if(t2!=t1) {
            cout << "Invalid types" << endl;
            pthread_mutex_unlock(&mutex);
            return -1;
        }
        m_int* currr = (m_int*)curr;
        *currr = value;

        cout << "Finished updating the value" << endl;

        if(elem->get_stack_entry()==-1) {
            var_stack[var_stack_index++] = elem;
            elem->set_stack_entry(var_stack_index-1);
        }

        pthread_mutex_unlock(&mutex);
        return 1;
    }

    else {
        pthread_mutex_unlock(&mutex);
        return -1;
    }
}

//assignArr. if index is -1, assign to all indices otherwise only to that index
template <typename T>
int myMem::assignArr(char* name, T value, int index /*= -1*/) {
    pthread_mutex_lock(&mutex);
    s_entry* elem = get_s_entry(name);

    if(elem == NULL) {
        cout << "Variable requested for doesn't exist" << endl;
        pthread_mutex_unlock(&mutex);
        return -1;
    }

    if(!elem->isarray()) {
        //check if array
        cout << "Asked variable is not an array" << endl;
        pthread_mutex_unlock(&mutex);
        return -1;
    }

    cout << "Assigning value to variable " << name << endl;

    const type_info& t1 = typeid(T);
    if(elem->get_type()=='i') {
        //int type
        const type_info &t2 = typeid(sampint);
        if(t2!=t1) {
            cout << "Invalid types" << endl;
            pthread_mutex_unlock(&mutex);
            return -1;
        }
        int* curr = (int*)getphysicaladdress(elem->get_addr());

        cout << "Got the address of the first element of the variable" << endl;

        if(index == -1) {
            cout << "Index is -1 " << endl;
            for(int i=0;i<elem->get_no_elements();i++) {
                // cout << "Assigning value" << endl;
                *curr = value;
                curr += 1;
            }
        }

        else {
            if(index >= elem->get_no_elements()) {
                //check if index out of bounds
                cout << "Index out of bounds" << endl;
                pthread_mutex_unlock(&mutex);
                return -1;
            }
            curr[index] = value;
        }

        cout << "Finished assigning value to required elements" << endl;

        //add to stack
        if(elem->get_stack_entry()==-1) {
            var_stack[var_stack_index++] = elem;
            elem->set_stack_entry(var_stack_index-1);
        }

        pthread_mutex_unlock(&mutex);
        return 1;
    }
    else if(elem->get_type()=='c') {
        const type_info &t2 = typeid(sampchar);
        if(t2!=t1) {
            cout << "Invalid types" << endl;
            pthread_mutex_unlock(&mutex);
            return -1;
        }
        char* curr = (char*)getphysicaladdress(elem->get_addr());
        
        cout << "Got the address of the first element of the variable" << endl;
 
        if(index == -1) {
            for(int i=0;i<elem->get_no_elements();i++) {
                *curr = value;
                curr += 1;
            }
        }

        else {
            if(index >= elem->get_no_elements()) {
                cout << "Index out of bounds" << endl;
                pthread_mutex_unlock(&mutex);
                return -1;
            }

            curr[index] = value;
        }

        cout << "Finished assigning value to required elements" << endl;

        if(elem->get_stack_entry()==-1) {
            var_stack[var_stack_index++] = elem;
            elem->set_stack_entry(var_stack_index-1);
        }

        pthread_mutex_unlock(&mutex);
        return 1;
    }
    else if(elem->get_type()=='b') {
        const type_info &t2 = typeid(sampbool);
        if(t2!=t1) {
            cout << "Invalid types" << endl;
            pthread_mutex_unlock(&mutex);
            return -1;
        }
        bool* curr = (bool*)getphysicaladdress(elem->get_addr());
        
        cout << "Got the address of the first element of the variable" << endl;
 
        if(index == -1) {
            for(int i=0;i<elem->get_no_elements();i++) {
                *curr = value;
                curr += 1;
            }
        }

        else {
            if(index >= elem->get_no_elements()) {
                cout << "Index out of bounds" << endl;
                pthread_mutex_unlock(&mutex);
                return -1;
            }

            curr[index] = value;
        }

        cout << "Finished assigning value to required elements" << endl;

        if(elem->get_stack_entry()==-1) {
            var_stack[var_stack_index++] = elem;
            elem->set_stack_entry(var_stack_index-1);
        }

        pthread_mutex_unlock(&mutex);
        return 1;
    }
    else if(elem->get_type()=='m') {
        const type_info &t2 = typeid(sampmint);
        if(t2!=t1) {
            cout << "Invalid types" << endl;
            pthread_mutex_unlock(&mutex);
            return -1;
        }
        m_int* curr = (m_int*)getphysicaladdress(elem->get_addr());
        
        cout << "Got the address of the first element of the variable" << endl;
 
        if(index == -1) {
            for(int i=0;i<elem->get_no_elements();i++) {
                *curr = value;
                curr += 1;
            }
        }

        else {
            if(index >= elem->get_no_elements()) {
                cout << "Index out of bounds" << endl;
                pthread_mutex_unlock(&mutex);
                return -1;
            }

            curr[index] = value;
        }

        cout << "Finished assigning value to required elements" << endl;

        if(elem->get_stack_entry()==-1) {
            var_stack[var_stack_index++] = elem;
            elem->set_stack_entry(var_stack_index-1);
        }

        pthread_mutex_unlock(&mutex);
        return 1;
    }


    pthread_mutex_unlock(&mutex);
    return -1;
}

//declare for three types
template int myMem::assignArr<int>(char* name, int value, int index);
template int myMem::assignArr<char>(char* name, char value, int index);
template int myMem::assignArr<bool>(char* name, bool value, int index);

//different assignArr function for medium int
int myMem::m_int_assignArr(char* name, m_int value, int index /*= -1*/) {
    pthread_mutex_lock(&mutex);
    s_entry* elem = get_s_entry(name);
    if(elem == NULL) {
        cout << "Variable requested for doesn't exit" << endl;
        pthread_mutex_unlock(&mutex);
        return -1;
    }

    if(!elem->isarray()) {
        cout << "Asked variable is not an array" << endl;
        pthread_mutex_unlock(&mutex);
        return -1;
    }

    cout << "Assigning value to variable " << name << endl;

    const type_info& t1 = typeid(sampmint);
    
    if(elem->get_type()=='m') {
        const type_info &t2 = typeid(sampmint);
        if(t2!=t1) {
            cout << "Invalid types" << endl;
            pthread_mutex_unlock(&mutex);
            return -1;
        }
        m_int* curr = (m_int*)getphysicaladdress(elem->get_addr());

        cout << "Got the address of the first element of the array " << name << endl;

        if(index == -1) {
            for(int i=0;i<elem->get_no_elements();i++) {
                *curr = value;
                curr += 1;
            }
        }
        
        else {
            if(index >= elem->get_no_elements()) {
                cout << "Index out of bounds" << endl;
                pthread_mutex_unlock(&mutex);
                return -1;
            }

            curr[index] = value;
        }

        cout << "Finished assigning value to required elements" << endl;

        if(elem->get_stack_entry()==-1) {
            var_stack[var_stack_index++] = elem;
            elem->set_stack_entry(var_stack_index-1);
        }

        pthread_mutex_unlock(&mutex);
        return 1;
    }
    pthread_mutex_unlock(&mutex);
    return -1;
}

int myMem::freeElem_gc(s_entry *elem) {
    //free an element, called from garbage collector
    // s_entry* elem = get_s_entry(name);
    // if(elem == NULL) {
    //     cout << "Variable requested to free does not exist." << endl;
    //     return -1;
    // }

    cout << "Freeing element " << elem->get_name() << " from GC " << endl;
   
    //increase freed entries
    freedentries++;

    //no need to do this if it is a special element
    if(!elem->is_scope_elem()) var_stack[elem->get_stack_entry()] = NULL;

    if(elem->get_prev_entry()!=NULL) {
        elem->get_prev_entry()->change_next_entry(elem->get_next_entry());
    }

    if(elem->get_next_entry()!=NULL) {
        elem->get_next_entry()->change_prev_entry(elem->get_prev_entry());
    }

    if(elem == first) first = elem->get_next_entry();
    if(elem == last) last = elem->get_prev_entry();

    //set to invalid, so that it can be replaced later on 
    elem->set_invalid();

    cout << "Removed the entry from the symbol table " << endl;

    //no need for below checks if it is a scope element
    if(elem->is_scope_elem()) {
        return 1;
    }

    if(elem->isarray()) {
        //need to remove all elements, if its an array
        int i = elem->get_addr();
        
        //delete all pages allocated to the array
        cout << "Setting the pages associated to the variable to invalid" << endl;
        for(int j=0;j<elem->get_size();j+=4) 
            page_table[(i+j) >> shiftbits] -= (INT16_MAX);

        freed_mem += (elem->get_size() + (4 - elem->get_size()%4)%4);
        return 1;
    }

    cout << "Setting the page allocated to the variable to invalid" << endl;
    page_table[elem->get_addr() >> shiftbits] -= INT16_MAX;

    freed_mem += (elem->get_size() + (4 - elem->get_size()%4)%4);

    return 1;
}

int myMem::freeElem(char* name) {
    //normal freeElem, same as above
    pthread_mutex_lock(&mutex);
    s_entry* elem = get_s_entry(name);
    cout << "Got the symbol table entry corresponding to the name " << endl;
    if(elem == NULL) {
        cout << "Variable requested to free does not exist." << endl;
        pthread_mutex_unlock(&mutex);
        return -1;
    }

    freedentries++;

    //no need to do this if it is a special element
    if(!elem->is_scope_elem()) var_stack[elem->get_stack_entry()] = NULL;

    if(elem->get_prev_entry()!=NULL) {
        elem->get_prev_entry()->change_next_entry(elem->get_next_entry());
    }

    if(elem->get_next_entry()!=NULL) {
        elem->get_next_entry()->change_prev_entry(elem->get_prev_entry());
    }

    if(elem == first) first = elem->get_next_entry();
    if(elem == last) last = elem->get_prev_entry();

    elem->set_invalid();
    cout << "Removed the entry from the symbol table " << endl;

    //no need for below checks if it is a scope element
    if(elem->is_scope_elem()) {
        pthread_mutex_unlock(&mutex);
        return 1;
    }

    if(elem->isarray()) {
        int i = elem->get_addr();
        
        //delete all pages allocated to the array
        cout << "Setting all the pages associated with the variable to invalid" << endl;
        for(int j=0;j<elem->get_size();j+=4) 
            page_table[(i+j) >> shiftbits] -= (INT16_MAX);

        freed_mem += (elem->get_size() + (4 - elem->get_size()%4)%4);
        pthread_mutex_unlock(&mutex);
        return 1;
    }

    cout << "Setting the page associated with the variable to invalid " << endl;
    page_table[elem->get_addr() >> shiftbits] -= INT16_MAX;

    freed_mem += (elem->get_size() + (4 - elem->get_size()%4)%4);
    pthread_mutex_unlock(&mutex);
    return 1;
}

void myMem::print_st() {
    //extra functionality to print the symbol table
    pthread_mutex_lock(&mutex);
    s_entry* curr = first;
    while(curr != NULL) {
        curr->print_entry();
        curr = curr->get_next_entry();
    }
    pthread_mutex_unlock(&mutex);
}

void myMem::display_empty_frames() {
    //extra functionality to check which logical addresses are emptied
    cout << "The following logical addresses are emptied: " << endl;
    for(int i=0;i<next_mem;i+=4) {
        if(page_table[i >> shiftbits]<0) cout << page_table[i >> shiftbits] + INT32_MAX << endl;
    }
}

void myMem::print_freed_entries() {
    //another extra functionality
    cout << "Freed Entries: " << freedentries << endl;
}

void myMem::scope_init() {
    //needs to be called at the beginning of every scope initialization
    s_entry* tempv;

    //push a scope element into the stack
    if(freedentries>0) {
        char* tempptr = start_st_pointer;
        tempv = (s_entry*)tempptr;
        while(tempv->isvalid()) {
            tempptr += sizeof(s_entry);
            tempv = (s_entry*)tempptr;
        }
        tempv->change_next_entry(NULL);
        tempv->change_prev_entry(NULL);
        freedentries--;
    }

    else {
        tempv = (s_entry*)point_to_st;

        point_to_st = point_to_st + sizeof(s_entry);
    }

    tempv->set_values('a',scope_element,0,false,0,true);

    tempv->change_prev_entry(last);
    if(last!=NULL) last->change_next_entry(tempv);
    else first = tempv;
    last = tempv;

    //add to stack
    if(tempv->get_stack_entry()==-1) {
        var_stack[var_stack_index++] = tempv;
        tempv->set_stack_entry(var_stack_index-1);
    }
}


void myMem::gc_initialize() {
    //initialize the stack
    cout << "Initializing the stack " << endl;
    var_stack = (s_entry**) (point_to_st +sizeof(s_entry)*((size+3)/4));
    var_stack_index = 0;

    //create the garbage thread
    cout << "Creating the garbage collection thread" << endl;
    pthread_t garbage_thread;
    pthread_create(&garbage_thread,NULL, &myMem::wrapper, this);
}

ostream& operator<<(ostream& os, const m_int& mt){
    //print the medium int
    int mask = 255;
    int mask2 = mask << 24;
    int temp;
    int val_a;


    temp = (((int)mt.val[0] & mask) << 16) | (((int)mt.val[1] & mask) << 8) | (((int)mt.val[2] & mask));

    if((mt.val[0] >> 7)&(char)1 == 1){
        val_a = temp | mask2;
    }
    else{
        val_a = temp;
    }

    os << val_a ;

    return os;
}


m_int operator+(const m_int &a, const m_int &b){
    //addition operator
    int mask = 255;
    int mask2 = mask << 24;
    int temp;
    int val_a, val_b;
    int sum;

    temp = (((int)a.val[0] & mask) << 16) | (((int)a.val[1] & mask) << 8) | (((int)a.val[2] & mask));

    if((a.val[0] >> 7)&(char)1 == 1){
        val_a = temp | mask2;
    }
    else{
        val_a = temp;
    }

    temp = (((int)b.val[0] & mask) << 16) | (((int)b.val[1] & mask) << 8) | (((int)b.val[2] & mask));

    if((b.val[0] >> 7)&(char)1 == 1){
        val_b = temp | mask2;
    }
    else{
        val_b = temp;
    }

    sum = val_a + val_b;
    m_int ans(sum);
    return ans;    
}

m_int operator-(const m_int &a, const m_int &b) {
    //subtraction operator
    int mask = 255;
    int mask2 = mask << 24;
    int temp;
    int val_a, val_b;
    int sum;

    temp = (((int)a.val[0] & mask) << 16) | (((int)a.val[1] & mask) << 8) | (((int)a.val[2] & mask));

    if((a.val[0] >> 7)&(char)1 == 1){
        val_a = temp | mask2;
    }
    else{
        val_a = temp;
    }

    temp = (((int)b.val[0] & mask) << 16) | (((int)b.val[1] & mask) << 8) | (((int)b.val[2] & mask));

    if((b.val[0] >> 7)&(char)1 == 1){
        val_b = temp | mask2;
    }
    else{
        val_b = temp;
    }

    sum = val_a - val_b;
    m_int ans(sum);
    return ans;
}

//pop the last variables from the stack 
void myMem::pop_last_vars() {
    cout << "Removing the irrelevant entries from the stack(called from scope_end())" << endl;
    while(var_stack_index>=1) {
        if(var_stack[var_stack_index-1]==NULL) {
            var_stack_index--; continue;
        }
        if(var_stack[var_stack_index-1]->is_scope_elem()) {
            var_stack_index--;
            break;
        }
        var_stack_index--; //pop the elements from the stack
    }
}

//needs to be called before ending the scope
void myMem::scope_end() {
    pop_last_vars();
    gc_run(); //call gc_run before exiting from a function too
}

//update logical addresses, used in compaction
void myMem::updateLogicalAddresses(s_entry* first, int offset) {
    s_entry* curr = first;
    while(curr != NULL) {
        curr->add_offset(offset);
        curr = curr->get_next_entry();
    }
}

//move pages down by a specific number of bytes, used in compaction
void myMem::move_pages(char* from, char* to, int no_of_bytes /*= PAGE_SIZE*/) {
    char* a = from; char* b = to;
    for(int i=0;i<no_of_bytes;i++) (*(b+i)) = (*(a+i));
}

//compact function
void myMem::compact() {
    cout << "Starting compaction" << endl;
    s_entry* curr = first;
    char* start  = (char*)mem; int curr_ind = 0;
    int total_allocated = next_mem - freed_mem;

    //debug statements

    cout << "Total allocated is " << total_allocated << endl;
    cout << "Next Mem is " << next_mem << endl;
    cout << "Freed Mem is " << freed_mem << endl;
    
    if(total_allocated == 0) {
        next_mem = 0; freed_mem = 0;
    }
    
    while(total_allocated != 0 && curr_ind < total_allocated) {

        if(page_table[curr_ind >> shiftbits]>0 && curr_ind == curr->get_addr()) {
            curr_ind += curr->get_size();
            curr_ind += (4-curr_ind%4)%4; curr = curr->get_next_entry();
            continue;
        }

        if(page_table[curr_ind >> shiftbits] <= 0) {
            int diff = curr->get_addr() - curr_ind; freed_mem -= diff;
            move_pages((start+curr->get_addr()),(start+curr_ind),total_allocated - curr_ind);
            updateLogicalAddresses(curr, -diff);
            curr_ind += curr->get_size();
            curr_ind += (4-curr_ind%4)%4; curr = curr->get_next_entry();
            continue;
        }

        //this is the special scope element case
        if(page_table[curr_ind >> shiftbits]>0 && curr_ind != curr->get_addr()) {
            curr = curr->get_next_entry(); continue;
        }
    }

    cout << "Finished compacting the memory and the page table" << endl;

    //compact stack too
    cout << "Compacting the stack now(O(n) time algorithm)" << endl;
    int i = 0, j = 0; 
    int prev_ = var_stack_index;
    for(int i=0;i<prev_;i++) {
        for(;j<i;j++) {
            if(var_stack[j]==NULL) break;
        }
        if(var_stack[i]==NULL) continue;
        if(j>=i) continue;
        var_stack[j] = var_stack[i];
        var_stack[i] = NULL;
        j++;
    }
    var_stack_index = j;
    cout << "Finished compacting the stack" << endl;
    
    next_mem = curr_ind;
}

void myMem::call_compact() {
    //extra functionality to user to call compact
    pthread_mutex_lock(&mutex);
    compact();
    pthread_mutex_unlock(&mutex);
}

void myMem::print_final_freed_mem() {
    //extra functionality for user to get number of holes currently
    cout << freed_mem << endl;
}

void myMem::print_stats() {
    int total_allocated = next_mem - freed_mem;
    //debug statements

    // if(curr == NULL) cout << "Yes" << endl;

    cout << "Total allocated is " << total_allocated << endl;
    cout << "Next Mem is " << next_mem << endl;
    cout << "Freed Mem is " << freed_mem << endl;

    if(first==NULL) return;
    cout << "First entry in symbol table: " << "\n";
    first->print_entry();

    cout << "Last entry in the symbol table: " << "\n";
    last->print_entry();
}
