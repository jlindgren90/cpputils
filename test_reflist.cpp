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

struct test : public ref_owned<test> {
    std::string val;

    test(const std::string &val) : val(val) {}
    ~test() { std::cout << "destroy: " << val << "\n"; }
};

std::string to_str(reflist<test> &list)
{
    std::string str;
    for (auto &t : list) {
        str += t.val;
    }
    return str;
}

std::string to_str_rev(reflist<test> &list)
{
    std::string str;
    for (auto &t : list.reversed()) {
        str += t.val;
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

    list2.append_all(list);

    TEST(to_str(list2) == "123abc");
    TEST(to_str_rev(list2) == "cba321");

    list = std::move(list2);

    TEST(to_str(list) == "123abc");
    TEST(to_str(list2) == "");

    int count = 0;
    for (auto it = list.begin(); it; ++it) {
        if (it->val[0] >= '0' && it->val[0] <= '9') {
            list.append(it.remove());

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
