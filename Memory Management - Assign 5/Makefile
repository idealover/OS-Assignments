demo: demo.cpp memlab.h libmymem.a demo2
	g++ demo.cpp -L. -lmymem -lpthread -o demo

demo2: demo2.cpp memlab.h libmymem.a demo1
	g++ demo2.cpp -L. -lmymem -lpthread -o demo2

demo1: demo1.cpp memlab.h libmymem.a 
	g++ demo1.cpp -L. -lmymem -lpthread -o demo1

libmymem.a: memlab.o
	ar -rcs libmymem.a memlab.o

memlab.o: memlab.cpp memlab.h
	g++ -pthread -c memlab.cpp -o memlab.o

clean:
	rm -r memlab.o demo1 demo2 libmymem.a