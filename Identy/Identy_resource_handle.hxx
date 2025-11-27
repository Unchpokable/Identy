#pragma once

#ifndef UNC_INDENTY_RESOURCE_HANDLE_H
#define UNC_INDENTY_RESOURCE_HANDLE_H

#include <cstdlib>

namespace identy
{

template<typename T>
concept ValueType = !std::is_pointer_v<T>;

template<typename Alloc, typename Value>
concept Allocator = std::is_invocable_r_v<Value*, Alloc, std::size_t> && std::is_trivially_constructible_v<Alloc>
    && std::is_trivially_destructible_v<Alloc>;

template<typename Free, typename Value>
concept Deallocator = std::is_invocable_v<Free, Value*> && std::is_trivially_constructible_v<Free>
    && std::is_trivially_destructible_v<Free>;

template<ValueType T>
struct CAlloc
{
    T* operator()(std::size_t size)
    {
        auto memory_ptr = malloc(size);
        if(memory_ptr == nullptr) {
            return memory_ptr;
        }

        return reinterpret_cast<T*>(memory_ptr);
    }
};

template<ValueType T>
struct CFree
{
    void operator()(T* ptr)
    {
        free(reinterpret_cast<void*>(ptr));
    }
};

template<ValueType T, Allocator<T> Alloc, Deallocator<T> Free>
class CResourceHandle
{
public:
    CResourceHandle() : m_pointer(nullptr) {};
    CResourceHandle(T* ptr, std::size_t size);

    CResourceHandle(const CResourceHandle&) = delete;
    CResourceHandle& operator=(const CResourceHandle&) = delete;

    CResourceHandle(CResourceHandle&& other);
    CResourceHandle& operator=(CResourceHandle&& other);

    ~CResourceHandle();

    void allocate(std::size_t size);

    const T* data() const noexcept;
    T* data() noexcept;

    std::size_t bytes_length() const noexcept;
    std::size_t items_length() const noexcept;

    void swap(T* new_data, std::size_t data_size);

    void set_size(std::size_t new_size);

    T* operator->() const noexcept;
    T* operator->() noexcept;

    operator T*() const;
    operator T*();

private:
    T* m_pointer;
    std::size_t m_size;

    bool m_was_moved { false };

    Alloc m_alloc;
    Free m_free;
};

template<ValueType T>
using CStdHandle = CResourceHandle<T, CAlloc<T>, CFree<T>>;

template<ValueType T, Allocator<T> Alloc, Deallocator<T> Free>
CResourceHandle<T, Alloc, Free>::CResourceHandle(T* ptr, std::size_t size) : m_pointer(ptr), m_size(size)
{
}

template<ValueType T, Allocator<T> Alloc, Deallocator<T> Free>
CResourceHandle<T, Alloc, Free>::CResourceHandle(CResourceHandle&& other)
{
    m_pointer = other.m_pointer;
    m_size = other.m_size;

    other.m_was_moved = true;
}

template<ValueType T, Allocator<T> Alloc, Deallocator<T> Free>
CResourceHandle<T, Alloc, Free>& CResourceHandle<T, Alloc, Free>::operator=(CResourceHandle&& other)
{
    if(this != &other) {
        if(m_pointer) {
            m_free(m_pointer);
        }

        m_pointer = other.m_pointer;
        m_size = other.m_size;

        other.m_was_moved = true;
    }
}

template<ValueType T, Allocator<T> Alloc, Deallocator<T> Free>
CResourceHandle<T, Alloc, Free>::~CResourceHandle()
{
    if(!m_was_moved && m_pointer) {
        m_free(m_pointer);
    }
}

template<ValueType T, Allocator<T> Alloc, Deallocator<T> Free>
void CResourceHandle<T, Alloc, Free>::allocate(std::size_t size)
{
    m_pointer = m_alloc(size);
    m_size = size;
}

template<ValueType T, Allocator<T> Alloc, Deallocator<T> Free>
const T* CResourceHandle<T, Alloc, Free>::data() const noexcept
{
    return m_pointer;
}

template<ValueType T, Allocator<T> Alloc, Deallocator<T> Free>
T* CResourceHandle<T, Alloc, Free>::data() noexcept
{
    return m_pointer;
}

template<ValueType T, Allocator<T> Alloc, Deallocator<T> Free>
std::size_t CResourceHandle<T, Alloc, Free>::bytes_length() const noexcept
{
    return m_size;
}

template<ValueType T, Allocator<T> Alloc, Deallocator<T> Free>
std::size_t CResourceHandle<T, Alloc, Free>::items_length() const noexcept
{
    return m_size / sizeof(T);
}

template<ValueType T, Allocator<T> Alloc, Deallocator<T> Free>
void CResourceHandle<T, Alloc, Free>::swap(T* new_data, std::size_t data_size)
{
    if(m_pointer) {
        m_free(m_pointer);
    }

    m_pointer = new_data;
    m_size = data_size;
}

template<ValueType T, Allocator<T> Alloc, Deallocator<T> Free>
void CResourceHandle<T, Alloc, Free>::set_size(std::size_t new_size)
{
    m_size = new_size;
}

template<ValueType T, Allocator<T> Alloc, Deallocator<T> Free>
inline T* CResourceHandle<T, Alloc, Free>::operator->() const noexcept
{
    return m_pointer;
}

template<ValueType T, Allocator<T> Alloc, Deallocator<T> Free>
inline T* CResourceHandle<T, Alloc, Free>::operator->() noexcept
{
    return m_pointer;
}

template<ValueType T, Allocator<T> Alloc, Deallocator<T> Free>
CResourceHandle<T, Alloc, Free>::operator T*() const
{
    return m_pointer;
}

template<ValueType T, Allocator<T> Alloc, Deallocator<T> Free>
CResourceHandle<T, Alloc, Free>::operator T*()
{
    return m_pointer;
}

} // namespace identy
#endif