#include "memlab.h"

myMem TheMemory;

int fibonacci(int n)
{
    if (n <= 1)
        return n;
    return fibonacci(n-1) + fibonacci(n-2);
}

int fibonacciProduct(int k){

    TheMemory.scope_init();
    char name[] = "FibArr";
    int* ptr  =(int*)TheMemory.createArr(INT, name, 100* sizeof(int));
    int product = 1;

    for(int i=1; i<= k ; i++){

        TheMemory.assignArr(name, fibonacci(i), i-1);
    }

    for(int i=0; i<k ; i++){

        product *= ptr[i];
    }

    TheMemory.scope_end();

    return product;
}


int main(int argc, char *argv[]){
    if(argc!=2) {
		printf("Improper arguments. Example is ./demo2 4\n");
		exit(-1);
	}

    int k = atoi(argv[1]);

    TheMemory.createMem(10000 * sizeof(int));

    int product = fibonacciProduct(k);

    printf("The product is %d\n", product);

}