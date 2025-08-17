/*
 * util.h
 * Copyright 2025 John Lindgren
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions, and the following disclaimer in the documentation
 *    provided with the distribution.
 *
 * This software is provided "as is" and without any warranty, express or
 * implied. In no event shall the authors be liable for any damages arising from
 * the use of this software.
 */
#ifndef UTIL_H
#define UTIL_H

#include <algorithm>

namespace util
{

/* Shorthand for std::find */
template<typename L, typename V>
auto find(L &list, const V &val)
{
    return std::find(list.begin(), list.end(), val);
}

/* Shorthand for std::find_if */
template<typename L, typename F>
auto find_if(L &list, const F &func)
{
    return std::find_if(list.begin(), list.end(), func);
}

/* Shorthand for std::remove + std::erase */
template<typename L, typename V>
void remove(L &list, V &&val)
{
    list.erase(std::remove(list.begin(), list.end(), std::forward<V>(val)),
               list.end());
}

/* Shorthand for std::remove_if + std::erase */
template<typename L, typename F>
void remove_if(L &list, F &&func)
{
    list.erase(std::remove_if(list.begin(), list.end(), std::forward<F>(func)),
               list.end());
}

/* Find-by-address for reflist (not fully generic) */
template<typename It, typename P>
auto find_ptr(It start, const P &ptr)
{
    for (; start; ++start) {
        if (start.get() == ptr) {
            break;
        }
    }
    return start;
}

template<typename It, typename P>
auto next_after(It start, const P &ptr, bool wrap)
{
    auto cur = find_ptr(start, ptr);
    if (!cur) {
        return start;
    }
    auto next = cur;
    if (!(++next) && wrap && cur != start) {
        next = start;
    }
    return next;
}

template<typename It, typename P, typename F>
auto next_after_if(It start, It stop, const P &ptr, bool wrap, const F &func)
{
    auto cur = find_ptr(start, ptr);
    if (cur == stop) {
        return std::find_if(start, stop, func);
    }
    auto next = cur;
    next = std::find_if(++next, stop, func);
    if (next == stop && wrap) {
        next = std::find_if(start, cur, func);
    }
    return next;
}

/* Generic implementation of operator=() using constructor */
template<typename T, typename V>
T &reconstruct(T &self, V &&val)
{
    if (&self != &val) {
        self.~T();
        new (&self) T(std::forward<V>(val));
    }
    return self;
}

} // namespace util

#endif // UTIL_H
