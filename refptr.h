/*
 * refptr.h
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
#ifndef REFPTR_H
#define REFPTR_H

#include <assert.h>
#include <utility>

/* Generic implementation of operator=() using constructor */
template<typename T, typename V>
T &assign_from(T &t, V &&v)
{
    if (&t != &v) {
        t.~T();
        new (&t) T(std::forward<V>(v));
    }
    return t;
}

/*
 * Generic intrusive reference-counting pointer.
 *
 * The pointed-to type needs to inherit the "refcounted" mix-in and
 * implement a last_unref() function, which is called to perform
 * type-specific behavior when the reference count drops to zero.
 *
 * Shared-ownership semantics can be obtained by making last_unref()
 * delete the object, but other behaviors are possible too.
 *
 * It is an error to destroy an object while refptrs to it exist.
 */
template<typename T>
struct refptr {
    refptr() : m_ptr(nullptr) {}

    explicit refptr(T *ptr) : m_ptr(ptr)
    {
        if (m_ptr) {
            m_ptr->m_refcount++;
        }
    }

    refptr(const refptr &rp) : refptr(rp.m_ptr) {}
    refptr(refptr &&rp) : m_ptr(rp.m_ptr) { rp.m_ptr = nullptr; }

    ~refptr()
    {
        if (m_ptr) {
            m_ptr->m_refcount--;
            if (!m_ptr->m_refcount) {
                m_ptr->last_unref();
            }
        }
    }

    refptr &operator=(const refptr &rp) { return assign_from(*this, rp); }
    refptr &operator=(refptr &&rp) { return assign_from(*this, std::move(rp)); }

    T *get() const { return m_ptr; }

    T &operator*() const { return *m_ptr; }
    T *operator->() const { return m_ptr; }

    explicit operator bool() const { return (bool)m_ptr; }

    bool operator==(T *ptr) const { return m_ptr == ptr; }
    bool operator==(const refptr &rp) const { return m_ptr == rp.m_ptr; }
    bool operator!=(T *ptr) const { return m_ptr != ptr; }
    bool operator!=(const refptr &rp) const { return m_ptr != rp.m_ptr; }

private:
    T *m_ptr;
};

/* Mix-in for a reference-counted type */
template<typename T>
struct refcounted {
    friend refptr<T>;

    refcounted() {}

    refcounted(const refcounted &) = delete;
    refcounted &operator=(const refcounted &) = delete;

    ~refcounted() { assert(m_refcount == 0); }

    unsigned refcount() const { return m_refcount; }

    // required to be defined in T:
    // void last_unref();

private:
    unsigned m_refcount = 0;
};

#endif // REFPTR_H
