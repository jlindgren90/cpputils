all: test_lockfree_accum test_reflist test_refptr

test_lockfree_accum: lockfree_accum.h test_lockfree_accum.cpp
	g++ -Wall -O2 -g -std=c++17 -o test_lockfree_accum test_lockfree_accum.cpp

test_reflist: refptr.h reflist.h test_reflist.cpp
	g++ -Wall -O2 -g -std=c++17 -o test_reflist test_reflist.cpp

test_refptr: refptr.h test_refptr.cpp
	g++ -Wall -O2 -g -std=c++17 -o test_refptr test_refptr.cpp

clean:
	rm -f test_lockfree_accum test_reflist test_refptr
