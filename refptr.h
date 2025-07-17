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
    refptr() {}
    explicit refptr(T *ptr) { reset(ptr); }
    refptr(const refptr &rp) { reset(rp.m_ptr); }
    refptr(refptr &&rp) : m_ptr(rp.m_ptr) { rp.m_ptr = nullptr; }
    ~refptr() { reset(); }

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

    void reset(T *ptr = nullptr)
    {
        if (ptr) {
            ptr->m_refcount++;
        }
        if (m_ptr) {
            m_ptr->m_refcount--;
            if (!m_ptr->m_refcount) {
                m_ptr->last_unref();
            }
        }
        m_ptr = ptr;
    }

private:
    T *m_ptr = nullptr;
};

/* Mix-in for a reference-counted type */
template<typename T>
struct refcounted {
    friend refptr<T>;

    refcounted() {}
    ~refcounted() { assert(m_refcount == 0); }

    // The mix-in itself does not support copy/move, since the refcount
    // is not transferable. A derived class may implement copy/move if
    // desired, but remember that after copying/moving, any existing
    // refptrs will still point to the original object.
    refcounted(const refcounted &) = delete;
    refcounted &operator=(const refcounted &) = delete;

    unsigned refcount() const { return m_refcount; }

    // required to be defined in T:
    // void last_unref();

private:
    unsigned m_refcount = 0;
};

/*
 * Generic intrusive weak pointer.
 *
 * Automatically resets to null when the pointed-to object is deleted.
 * The pointed-to type needs to inherit the "weak_target" mix-in.
 */
template<typename T>
struct weakptr {
    weakptr() {}
    explicit weakptr(T *ptr) { reset(ptr); }
    weakptr(const weakptr &rp) { reset(rp.m_ptr); }
    // move constructor omitted (cannot be moved efficiently)
    ~weakptr() { reset(); }

    weakptr &operator=(const weakptr &rp) { return assign_from(*this, rp); }
    // move assignment omitted (cannot be moved efficiently)

    T *get() const { return m_ptr; }

    T &operator*() const { return *m_ptr; }
    T *operator->() const { return m_ptr; }

    explicit operator bool() const { return (bool)m_ptr; }

    bool operator==(T *ptr) const { return m_ptr == ptr; }
    bool operator==(const weakptr &rp) const { return m_ptr == rp.m_ptr; }
    bool operator!=(T *ptr) const { return m_ptr != ptr; }
    bool operator!=(const weakptr &rp) const { return m_ptr != rp.m_ptr; }

    void reset(T *ptr = nullptr)
    {
        if (m_ptr) {
            // remove from linked list
            if (m_ptr->head == this) {
                m_ptr->head = this->next;
            } else {
                auto prior = m_ptr->head;
                while (prior->next != this) {
                    prior = prior->next;
                }
                prior->next = this->next;
            }
        }
        m_ptr = ptr;
        if (ptr) {
            // add to linked list
            this->next = ptr->head;
            ptr->head = this;
        } else {
            this->next = nullptr;
        }
    }

private:
    T *m_ptr = nullptr;
    weakptr<T> *next = nullptr;
};

/* Mix-in for a weak pointer target type */
template<typename T>
struct weak_target {
    friend weakptr<T>;

    weak_target() {}

    ~weak_target()
    {
        while (head) {
            head->reset();
        }
    }

    // The mix-in itself does not support copy/move, since the weakptrs
    // would be left dangling. A derived class may implement copy/move
    // if desired, but remember that after copying/moving, any existing
    // weakptrs will still point to the original object.
    weak_target(const weak_target &) = delete;
    weak_target &operator=(const weak_target &) = delete;

private:
    weakptr<T> *head = nullptr; // linked list
};

#endif // REFPTR_H
