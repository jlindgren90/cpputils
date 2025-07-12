/* See reflist.h for copyright & license */
#include "reflist.h"
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

std::string to_str(reflist<test> &list)
{
    std::string str;
    for (auto rp : list) {
        str += rp->val;
    }
    return str;
}

std::string to_str_rev(reflist<test> &list)
{
    std::string str;
    for (auto rp : list.reversed()) {
        str += rp->val;
    }
    return str;
}

int main(void)
{
    reflist<test> list, list2;
    refptr<test> a{new test("a")};

    list.append(a);
    list.append(new test("b"));
    list.append(new test("c"));

    TEST(to_str(list) == "abc");
    TEST(to_str_rev(list) == "cba");

    list2.prepend(new test("3"));
    list2.prepend(new test("2"));
    list2.prepend(new test("1"));

    TEST(to_str(list2) == "123");
    TEST(to_str_rev(list2) == "321");

    list2.append_all(list.begin(), list.end());

    TEST(to_str(list2) == "123abc");
    TEST(to_str_rev(list2) == "cba321");

    list = std::move(list2);

    TEST(to_str(list) == "123abc");
    TEST(to_str(list2) == "");

    int count = 0;
    // calling end() repeatedly is inefficient but intentional here
    for (auto it = list.begin(); it != list.end(); ++it) {
        if ((*it)->val[0] >= '0' && (*it)->val[0] <= '9') {
            list.append(std::move(*it));

            count++;
            if (count == 1) {
                TEST(to_str(list) == "23abc1");
            } else if (count == 2) {
                TEST(to_str(list) == "3abc12");
            } else {
                TEST(to_str(list) == "abc123");
            }
        }
    }

    TEST(list.remove(a));
    TEST(to_str(list) == "bc123");

    return 0;
}
