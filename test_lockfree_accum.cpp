#include "lockfree_accum.h"
#include <iostream>
#include <string>
#include <thread>
#include <unistd.h>

class test_buf
{
public:
    test_buf &operator=(const test_buf &other)
    {
        assert(!accumulating);
        assert(!other.accumulating);
        assert(!reporting);
        data = other.data;
        return *this;
    }

    void accum(const std::string &val)
    {
        assert(!accumulating);
        assert(!reporting);
        accumulating = true;
        data += val;
        usleep(10000);
        accumulating = false;
    }

    const std::string &report()
    {
        assert(!accumulating);
        assert(!reporting);
        reporting = true;
        usleep(75000);
        return data;
    }

    void reset()
    {
        data.clear();
        reporting = false;
    }

private:
    bool accumulating = false;
    bool reporting = false;
    std::string data;
};

int main(void)
{
    lockfree_accum<test_buf, std::string> lfa;

    std::thread worker([&]() {
        for (int i = 0; i < 100; i++) {
            lfa.accum(std::to_string(i) + ",");
        }
    });

    for (int i = 0; i < 16; i++) {
        const std::string *r = lfa.report();
        if (r) {
            std::cout << "Report #" << i << ": " << *r << "\n";
            lfa.reset();
        } else {
            std::cout << "Report #" << i << " is empty\n";
            usleep(75000);
        }
    }

    worker.join();
    return 0;
}
