all: test_reflist test_refptr

test_reflist: refptr.h reflist.h test_reflist.cpp
	g++ -Wall -O2 -std=c++17 -o test_reflist test_reflist.cpp

test_refptr: refptr.h test_refptr.cpp
	g++ -Wall -O2 -std=c++17 -o test_refptr test_refptr.cpp

clean:
	rm -f test_reflist test_refptr
