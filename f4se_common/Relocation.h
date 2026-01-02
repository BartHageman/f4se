#pragma once  // Include guard - prevents this header from being included twice

class RelocationManager
{
public:
    RelocationManager();
    static uintptr_t s_baseAddr;  // Static = shared across all instances
                                  // Stores where Game.exe actually loaded in memory due to ASLR
};

// Template class - T is a placeholder for any type (int, float, SomeGameClass, etc.)
// Use this when you have an OFFSET that points to a POINTER to data
template <typename T>
class RelocPtr
{
public:
    // Constructor: takes a raw offset (like 0x1234567) and adds the base address
    // So if the game loaded at 0x140000000 and offset is 0x1234567,
    // m_offset becomes 0x141234567 (the actual runtime address)
    RelocPtr(uintptr_t offset)
        : m_offset(offset + RelocationManager::s_baseAddr)  // Member initializer list
    {
        //
    }

    // Operator overloading - lets you use RelocPtr<T> as if it were T*
    // Example: RelocPtr<int> myPtr(0x1234); int x = *myPtr;
    operator T *() const
    {
        return GetPtr();
    }

    // Arrow operator - lets you do myPtr->someMember
    T * operator->() const
    {
        return GetPtr();
    }

    T * GetPtr() const
    {
        // reinterpret_cast: "trust me compiler, treat this number as a pointer"
        // This is the dangerous/powerful cast for low-level memory work
        return reinterpret_cast<T *>(m_offset);
    }
                    //     |   I pinky promise not to modify the object I'm called on. "I don't
                    //     v   modify 'this' during the function call."
    const T * GetConst() const
    // ^ "I will return a pointer which points to something of type const T.
    // The value stored there can't be modified, but the pointer I return can
    // be modified to point elsewhere."
    {
        return reinterpret_cast<T *>(m_offset);
    }

    uintptr_t GetUIntPtr() const  
    // uintptr_t's are unsigned integer type guaranteed to be large enough to
    // hold any pointer. On 64-bit systems, it's 64 bits. On 32-bit, 32 bits.
    // They're used because you sometimes want to treat pointers like regular
    // integers for performing simple math on them. Regular pointers don't
    // allow that because of pointer arithmetic.
    {
        return m_offset;
    }

private:
    uintptr_t m_offset;  // The name is a misnomer because we store the actual runtime address in here.

    // These are declared private with no implementation = deleted
    // Prevents: default construction, copying, assignment
    // Forces you to always provide an offset
    RelocPtr();
    RelocPtr(RelocPtr & rhs);
    RelocPtr & operator=(RelocPtr & rhs);
};

// Use this for function pointers or when you need the ADDRESS ITSELF as your value
// (not a pointer to something else)
template <typename T>
class RelocAddr
{
public:
    RelocAddr(uintptr_t offset)
        : m_offset(reinterpret_cast<BlockConversionType *>(offset + RelocationManager::s_baseAddr))
    {
        //
    }

    operator T()  // Note: returns T, not T* — this IS the value/function pointer
    {
        return reinterpret_cast<T>(m_offset);
    }

    uintptr_t GetUIntPtr() const
    {
        return reinterpret_cast<uintptr_t>(m_offset);
    }

private:
    // This struct exists purely as a workaround:
    // reinterpret_cast<uintptr_t>(some_uintptr_t) is illegal (same type)
    // By storing as a pointer to this dummy struct, we can cast freely
    struct BlockConversionType { };
    BlockConversionType * m_offset;

    RelocAddr();
    RelocAddr(RelocAddr & rhs);
    RelocAddr & operator=(RelocAddr & rhs);
};
