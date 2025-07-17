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

struct test : public ref_owned<test>, public weak_target<test> {
    std::string val;

    test(const std::string &val) : val(val) {}
    ~test() { std::cout << "destroy: " << val << "\n"; }
};

int main(void)
{
    refptr test1{new test("test1")};
    auto test1b = test1;
    refptr test2{new test("test2")};
    auto test2b = test2;

    TEST(test1 == test1b);
    TEST(test2 == test2b);

    TEST(test1 && test1->refcount() == 2);
    TEST(test2 && test2->refcount() == 2);

    weakptr w1(test1.get());
    auto w1b = w1;
    weakptr w2(test2.get());
    auto w2b = w2;

    TEST(w1 && w1 == test1.get());
    TEST(w1b && w1b == w1);
    TEST(w2 && w2 == test2.get());
    TEST(w2b && w2b == w2);

    test2 = std::move(test1);

    TEST(!test1);
    TEST(test2 == test1b);

    TEST(test1b && test1b->refcount() == 2);
    TEST(test2b && test2b->refcount() == 1);

    w2 = w1;

    TEST(w1 && w1b && w2 && w2b);
    test1b.reset();
    TEST(w1 && w1b && w2 && w2b);
    test2.reset();
    TEST(!w1 && !w1b && !w2 && w2b);
    test2b.reset();
    TEST(!w1 && !w1b && !w2 && !w2b);

    return 0;
}
