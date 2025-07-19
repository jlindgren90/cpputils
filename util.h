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
auto find(L &list, V &&val)
{
    return std::find(list.begin(), list.end(), std::forward<V>(val));
}

/* Shorthand for std::find_if */
template<typename L, typename F>
auto find_if(L &list, F &&func)
{
    return std::find_if(list.begin(), list.end(), std::forward<F>(func));
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
