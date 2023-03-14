#include "memlab.h"

int main() {
    // myMem tomanage(20*sizeof(int));
    myMem tomanage;

    tomanage.createMem(60*sizeof(int));

    cout << tomanage.isempty() << endl;

    // tomanage.createVar("nonsense".c_str(),INT);

    // tomanage.assignVar("nonsense".c_str(),20);
    char name[] = "nonsense";

    int* storing = (int*)tomanage.createVar(INT,name);

    cout << "Hi" << endl;
    int val = 20;

    tomanage.assignVar<int>(name,val);

    char arrname[] = "hithere";
    char* testingarr = (char*)tomanage.createArr(CHAR,arrname,10);

    // int* testingwithint = (int*)tomanage.createArr(INT,arrname,10);

    tomanage.assignArr<char>(arrname,'x');

    char boolname[] = "thisboo";
    bool* testingbool = (bool*)tomanage.createVar(BOOL,boolname);

    tomanage.assignVar<bool>(boolname,false);

    char intarr[] = "thisint";
    int* testintar = (int*)tomanage.createArr(INT,intarr,10);

    tomanage.assignArr<int>(intarr,3);

    // // int* checking = (int*)tomanage.get_address("nonsense");

    cout << "Checking value: " << *storing << endl;
    cout << "Checking arr value: " << testingarr[2] << endl;
    cout << "Checking bool value: " << *testingbool << endl;
    // cout << "Checking int arr value: " << testintar[2] << endl;

    cout << "Checking values of the whole integer array: " << endl;
    for(int i=0;i<10;i++) cout << testintar[i] << " ";
    cout << endl;

    cout << "Freeing first int" << endl;
    tomanage.freeElem(name);

    cout << "Freeing the second char array " << endl;
    tomanage.freeElem(arrname);

    // tomanage.print_st();
    // tomanage.display_empty_frames();

    tomanage.print_freed_entries();

    char name1[] = "name1";
    tomanage.createArr(BOOL,name1,10);

    tomanage.assignArr(name1,false);

    tomanage.print_freed_entries();

    tomanage.scope_init();

    tomanage.print_st();
    tomanage.display_empty_frames();

    cout << endl << endl;
    cout << "Printing the stack situation after 3 secs: " << endl;

    // sleep(3);
    cout << "Just before compact, symbol table situation" << endl;

    tomanage.print_st();

    tomanage.scope_end();
    // tomanage.scope_end();

    cout << "Final symbol table: " << endl;

    tomanage.print_st();

    cout << endl << "Table after compacting " << endl << endl;

    // tomanage.call_compact();

    tomanage.print_st();
    cout << endl; tomanage.print_final_freed_mem();

    cout << "Final testing with medium int" << endl;

    m_int x = 3;
    // cout << x << endl;
    char name3[] = "mintname";

    m_int *y = (m_int*)tomanage.createVar(M_INT,name3);
    tomanage.m_int_assignVar(name3,x);

    cout << "Value of mint is " << *y << endl;

    char name4[] = "mintar";
    m_int *z = (m_int*)tomanage.createArr(M_INT,name4,10);

    tomanage.print_st();

    tomanage.m_int_assignArr(name4,x,2);
    cout << "Value in arr is " << z[2] << endl;

    tomanage.print_stats();

    //checking does exist
    tomanage.scope_init();

    char name5[] = "name5";
    tomanage.createVar(INT,name5);
    tomanage.createArr(CHAR,name5,4);
    // cout << tomanage.does_exist(name5) << endl;
    tomanage.scope_end();

    cout << endl << endl; tomanage.print_st(); cout << endl << endl;

    tomanage.createArr(CHAR,name5,4);

    cout << endl << endl; tomanage.print_st(); cout << endl << endl;

    // cout << tomanage.does_exist(name5) << endl;

    // cout << tomanage.does_exist(name4) << endl;
    // cout << tomanage.does_exist(name4) << endl;
    cout << get_sizeof_type('m') << endl;
}