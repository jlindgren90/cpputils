/* See refptr.h for copyright & license */
#include "refptr.h"
#include <iostream>
#include <string>

#define TEST(x) do {                    \
    if (x) {                            \
        std::cout << "PASS: " #x "\n";  \
    } else {                            \
        std::cout << "FAIL: " #x "\n";  \
    }                                   \
} while (0)

struct test : public refcounted<test> {
    std::string val;

    test(const std::string &val) : val(val) {}

    void last_unref()
    {
        std::cout << "destroy: " << val << "\n";
        delete this;
    }
};

int main(void)
{
    auto test1 = refptr(new test("test1"));
    auto test1b = test1;
    auto test2 = refptr(new test("test2"));
    auto test2b = test2;

    TEST(test1 == test1b);
    TEST(test2 == test2b);

    TEST(test1 && test1->refcount() == 2);
    TEST(test2 && test2->refcount() == 2);

    test2 = std::move(test1);

    TEST(!test1);
    TEST(test2 == test1b);

    TEST(test1b && test1b->refcount() == 2);
    TEST(test2b && test2b->refcount() == 1);

    return 0;
}
