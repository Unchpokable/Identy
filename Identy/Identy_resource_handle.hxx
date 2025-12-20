/**
 * @file Identy_resource_handle.hxx
 * @brief RAII-compliant resource management template for dynamic memory allocation
 *
 * Provides a generic smart pointer template (CResourceHandle) for managing dynamically
 * allocated memory with custom allocator and deallocator functors. This header implements
 * the core memory management abstraction used internally by Identy for handling raw
 * SMBIOS data and other variable-length structures.
 *
 * ## Key Features
 *
 * - **RAII Compliance** - Automatic resource cleanup on scope exit
 * - **Move Semantics** - Efficient transfer of ownership without copying
 * - **Custom Allocators** - Pluggable allocation/deallocation strategies
 * - **Type Safety** - C++20 concepts enforce correct allocator/deallocator signatures
 * - **Zero Overhead** - Trivial allocator types compiled away at optimization
 *
 * @note This is an internal utility header. Application code typically does not need
 *       to use CResourceHandle directly unless implementing custom memory management.
 */

#pragma once

#ifndef UNC_IDENTY_RESOURCE_HANDLE_H
#define UNC_IDENTY_RESOURCE_HANDLE_H

#include <cstdlib>

namespace identy
{

/**
 * @brief C++20 concept constraining template parameter to non-pointer value types
 *
 * Ensures that the managed type T is a value type (not a pointer), preventing
 * accidental double-indirection scenarios in memory management.
 *
 * @tparam T Type to validate
 */
template<typename T>
concept ValueType = !std::is_pointer_v<T>;

/**
 * @brief C++20 concept defining valid allocator functors
 *
 * An allocator must be invocable with a size_t parameter and return a pointer
 * to the managed value type. Additionally, it must be trivially constructible
 * and destructible for zero-overhead abstraction.
 *
 * @tparam Alloc Allocator functor type
 * @tparam Value Managed value type
 */
template<typename Alloc, typename Value>
concept Allocator = std::is_invocable_r_v<Value*, Alloc, std::size_t> && std::is_trivially_constructible_v<Alloc>
    && std::is_trivially_destructible_v<Alloc>;

/**
 * @brief C++20 concept defining valid deallocator functors
 *
 * A deallocator must be invocable with a pointer to the managed value type.
 * Additionally, it must be trivially constructible and destructible for
 * zero-overhead abstraction.
 *
 * @tparam Free Deallocator functor type
 * @tparam Value Managed value type
 */
template<typename Free, typename Value>
concept Deallocator = std::is_invocable_v<Free, Value*> && std::is_trivially_constructible_v<Free>
    && std::is_trivially_destructible_v<Free>;

/**
 * @brief Standard C malloc-based allocator functor
 *
 * Allocates raw memory using std::malloc and casts the result to the
 * appropriate pointer type. This is the default allocator used by CStdHandle.
 *
 * @tparam T Managed value type
 *
 * @exception std::bad_alloc
 */
template<ValueType T>
struct CAlloc
{
    /**
     * @brief Allocates memory of specified size in bytes
     *
     * @param size Number of bytes to allocate
     * @return Pointer to allocated memory, or nullptr on failure
     */
    T* operator()(std::size_t size)
    {
        auto memory_ptr = std::malloc(size);
        if(memory_ptr == nullptr) {
            throw std::bad_alloc();
        }

        return reinterpret_cast<T*>(memory_ptr);
    }
};

/**
 * @brief Standard C free-based deallocator functor
 *
 * Releases memory using std::free. This is the default deallocator
 * used by CStdHandle to pair with CAlloc.
 *
 * @tparam T Managed value type
 */
template<ValueType T>
struct CFree
{
    /**
     * @brief Frees memory pointed to by ptr
     *
     * @param ptr Pointer to memory allocated by CAlloc
     */
    void operator()(T* ptr)
    {
        free(reinterpret_cast<void*>(ptr));
    }
};

/**
 * @brief RAII smart pointer template with custom allocator/deallocator support
 *
 * Generic resource handle implementing move-only semantics for managing
 * dynamically allocated memory with configurable allocation strategies.
 * Similar to std::unique_ptr but with explicit size tracking and custom
 * allocator functors instead of deleters.
 *
 * @tparam T Managed value type (must satisfy ValueType concept)
 * @tparam Alloc Allocator functor type (must satisfy Allocator<T> concept)
 * @tparam Free Deallocator functor type (must satisfy Deallocator<T> concept)
 *
 * ## Usage Example
 *
 * ```cpp
 * // Using the standard handle with malloc/free
 * CStdHandle<SMBIOS_Raw> smbios_data;
 * smbios_data.allocate(1024); // Allocates 1KB
 *
 * // Access raw pointer
 * SMBIOS_Raw* ptr = smbios_data.data();
 *
 * // Automatically freed when smbios_data goes out of scope
 * ```
 *
 * @note This class is move-only (copy constructor/assignment deleted)
 * @see CStdHandle
 * @see CAlloc
 * @see CFree
 */
template<ValueType T, Allocator<T> Alloc, Deallocator<T> Free>
class CResourceHandle
{
public:
    /** @brief Default constructor initializing null pointer */
    CResourceHandle() : m_pointer(nullptr), m_size(0)
    {
    }

    /**
     * @brief Constructs handle from existing allocated pointer
     *
     * @param ptr Pointer to already-allocated memory
     * @param size Size of the allocated memory in bytes
     */
    CResourceHandle(T* ptr, std::size_t size);

    /** @brief Copy constructor (deleted - move-only semantics) */
    CResourceHandle(const CResourceHandle&) = delete;

    /** @brief Copy assignment (deleted - move-only semantics) */
    CResourceHandle& operator=(const CResourceHandle&) = delete;

    /**
     * @brief Move constructor transferring ownership
     *
     * @param other Source handle to move from
     */
    CResourceHandle(CResourceHandle&& other) noexcept;

    /**
     * @brief Move assignment operator transferring ownership
     *
     * @param other Source handle to move from
     * @return Reference to this handle
     */
    CResourceHandle& operator=(CResourceHandle&& other) noexcept;

    /**
     * @brief Destructor automatically freeing managed memory
     *
     * Calls the deallocator functor on the managed pointer unless
     * the handle was moved from.
     */
    ~CResourceHandle();

    /**
     * @brief Allocates new memory of specified size
     *
     * @param size Number of bytes to allocate
     *
     */
    void allocate(std::size_t size);

    /**
     * @brief Returns const pointer to managed memory
     *
     * @return Const pointer to data, or nullptr if not allocated
     */
    const T* data() const noexcept;

    /**
     * @brief Returns mutable pointer to managed memory
     *
     * @return Pointer to data, or nullptr if not allocated
     */
    T* data() noexcept;

    /**
     * @brief Returns size of allocated memory in bytes
     *
     * @return Number of bytes allocated
     */
    std::size_t bytes_length() const noexcept;

    /**
     * @brief Returns number of T items that fit in allocated memory
     *
     * @return Number of complete T objects that fit (bytes_length / sizeof(T))
     */
    std::size_t items_length() const noexcept;

    /**
     * @brief Swaps managed pointer with new externally-allocated memory
     *
     * Frees the old managed memory (if any) and takes ownership of new_data.
     *
     * @param new_data Pointer to new memory to manage
     * @param data_size Size of new memory in bytes
     */
    void swap(T* new_data, std::size_t data_size);

    /**
     * @brief Updates the stored size without reallocation
     *
     * @param new_size New size value in bytes
     *
     * @warning Does not actually resize memory, only updates the size field.
     *          Use with caution to reflect external size changes.
     */
    void set_size(std::size_t new_size);

    /**
     * @brief Arrow operator for pointer-like access (const)
     *
     * @return Const pointer to managed memory
     */
    T* operator->() const noexcept;

    /**
     * @brief Arrow operator for pointer-like access (mutable)
     *
     * @return Pointer to managed memory
     */
    T* operator->() noexcept;

    /**
     * @brief Implicit conversion to const T* pointer
     *
     * @return Const pointer to managed memory
     */
    operator T*() const;

    /**
     * @brief Implicit conversion to T* pointer
     *
     * @return Pointer to managed memory
     */
    operator T*();

private:
    T* m_pointer;               ///< Pointer to managed memory
    std::size_t m_size;         ///< Size of allocated memory in bytes
    bool m_was_moved { false }; ///< Flag indicating this handle was moved from

    Alloc m_alloc; ///< Allocator functor instance
    Free m_free;   ///< Deallocator functor instance
};

/**
 * @brief Type alias for CResourceHandle using standard malloc/free allocators
 *
 * This is the recommended handle type for most use cases, providing RAII
 * memory management with standard C allocation functions.
 *
 * @tparam T Managed value type
 *
 * Example:
 * ```cpp
 * CStdHandle<byte> buffer;
 * buffer.allocate(1024);
 * std::memcpy(buffer.data(), source, 1024);
 * // Automatically freed on scope exit
 * ```
 */
template<ValueType T>
using CStdHandle = CResourceHandle<T, CAlloc<T>, CFree<T>>;

template<ValueType T, Allocator<T> Alloc, Deallocator<T> Free>
CResourceHandle<T, Alloc, Free>::CResourceHandle(T* ptr, std::size_t size) : m_pointer(ptr), m_size(size)
{
}

template<ValueType T, Allocator<T> Alloc, Deallocator<T> Free>
CResourceHandle<T, Alloc, Free>::CResourceHandle(CResourceHandle&& other) noexcept
{
    m_pointer = other.m_pointer;
    m_size = other.m_size;

    other.m_was_moved = true;
    other.m_pointer = nullptr;
}

template<ValueType T, Allocator<T> Alloc, Deallocator<T> Free>
CResourceHandle<T, Alloc, Free>& CResourceHandle<T, Alloc, Free>::operator=(CResourceHandle&& other) noexcept
{
    if(this != &other) {
        if(m_pointer) {
            m_free(m_pointer);
        }

        m_pointer = other.m_pointer;
        m_size = other.m_size;

        other.m_was_moved = true;
        other.m_pointer = nullptr;
    }

    return *this;
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
    if(m_pointer) {
        m_free(m_pointer);
    }

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
T* CResourceHandle<T, Alloc, Free>::operator->() const noexcept
{
    return m_pointer;
}

template<ValueType T, Allocator<T> Alloc, Deallocator<T> Free>
T* CResourceHandle<T, Alloc, Free>::operator->() noexcept
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
