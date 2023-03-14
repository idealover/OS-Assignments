//#include "new.h"
#include "memlab.h"

myMem TheMemory;


void func1(int x, int y){
    // TheMemory.scope_init();
    srand(10);
	char name[] = "array1";
	TheMemory.createArr(INT, name, 50000);
	//int* ind = (int*) TheMemory.get_location(name);
	for(int i=0; i < 50000 ; i++){
		
		TheMemory.assignArr<int>(name, rand(), i);
	}
	TheMemory.freeElem(name);
	// TheMemory.scope_end();
    return;
}

void func2(char x, char y){
	
    TheMemory.scope_init();
    srand(10);
	char name[] = "array2";
	TheMemory.createArr(18, name, 50000);
	//char* ind = (char*) TheMemory.get_location(name);
	for(int i=0; i < 50000 ; i++){

		TheMemory.assignArr<char>(name, 'a'+ rand()%26, i);
	}
	TheMemory.freeElem(name);
	TheMemory.scope_end();
    return;
}
void func3(bool x, bool y){
	
    TheMemory.scope_init();
    srand(10);
	char name[] = "array3";
	TheMemory.createArr(20, name, 50000);
	//bool* ind = (bool*) TheMemory.get_location(name);
	for(int i=0; i < 50000 ; i++){

		TheMemory.assignArr<bool>(name, (bool)rand()%2, i);
	}
	TheMemory.freeElem(name);
	TheMemory.scope_end();
    return;
}
void func4(m_int x, m_int y){
	
    TheMemory.scope_init();
    srand(10);
	char name[] = "array4";
	TheMemory.createArr(19, name, 50000);
	//m_int* ind = (m_int*) TheMemory.get_location(name);
	for(int i=0; i < 50000 ; i++){

		TheMemory.m_int_assignArr(name, (m_int)rand(), i);
	}
	TheMemory.freeElem(name);
	TheMemory.scope_end();
    return;
}

void func5(int x, int y){
	
    TheMemory.scope_init();
    srand(20);
	char name[] = "array5";
	TheMemory.createArr(17, name, 50000);
	//int* ind = (int*) TheMemory.get_location(name);
	for(int i=0; i < 50000 ; i++){

		TheMemory.assignArr<int>(name, rand(), i);
	}
	TheMemory.freeElem(name);
	TheMemory.scope_end();
    return;
}

void func6(char x, char y){
	
    TheMemory.scope_init();
    srand(20);
	char name[] = "array6";
	TheMemory.createArr(18, name, 50000);
	//char* ind = (char*) TheMemory.get_location(name);
	for(int i=0; i < 50000 ; i++){

		TheMemory.assignArr<char>(name, 'a'+rand()%26, i);
	}
	TheMemory.freeElem(name);
	TheMemory.scope_end();
    return;
}
void func7(bool x, bool y){
	
    TheMemory.scope_init();
    srand(20);
	char name[] = "array7";
	TheMemory.createArr(20, name, 50000);
	//bool* ind = (bool*) TheMemory.get_location(name);
	for(int i=0; i < 50000 ; i++){

		TheMemory.assignArr<bool>(name, (bool)rand()%2, i);
	}
	TheMemory.freeElem(name);
	TheMemory.scope_end();
    return;
}
void func8(m_int x, m_int y){
	
    TheMemory.scope_init();
    srand(20);
	char name[] = "array8";
	TheMemory.createArr(19, name, 50000);
	//m_int* ind = (m_int*) TheMemory.get_location(name);
	for(int i=0; i < 50000 ; i++){
		TheMemory.m_int_assignArr(name, (m_int)rand(), i);
	}
	TheMemory.freeElem(name);
	TheMemory.scope_end();
    return;
}
void func9(int x, int y){
	
    TheMemory.scope_init();
    srand(30);
	char name[] = "array9";
	TheMemory.createArr(17, name, 50000);
	//int* ind = (int*) TheMemory.get_location(name);
	for(int i=0; i < 50000 ; i++){

		TheMemory.assignArr<int>(name, rand(), i);
	}
	TheMemory.freeElem(name);
	TheMemory.scope_end();
    return;
}

void func10(char x, char y){
	
    TheMemory.scope_init();
    srand(30);
	char name[] = "array10";
	TheMemory.createArr(18, name, 50000);
	//char* ind = (char*) TheMemory.get_location(name");
	for(int i=0; i < 50000 ; i++){

		TheMemory.assignArr<char>(name, 'a'+rand()%26, i);
	}	
	TheMemory.freeElem(name);
	TheMemory.scope_end();
    return;
}


int main(){

	
	TheMemory.createMem(250000000 * sizeof(char));

	// if(TheMemory.isempty()) cout << "Too much allocated" << endl;
	// exit(0);

	int xi=5,yi=10;
	char xc = 'a',yc='b';
	bool xb = 1, yb=0;
	m_int xm = m_int(10), ym = m_int(5);

	func1(xi,yi);
	func2(xc,yc);
	func3(xb,yb);
	func4(xm,ym);
	func5(xi,yi);
	func6(xc,yc);
	func7(xb,yb);
	func8(xm,ym);
	func9(xi,yi);
	func10(xc,yc);

	TheMemory.print_stats();
	// cout << "Size of int is " << 

}
