#if defined(SWIFT_DEBUG) && defined(__GNUC__)

#include "memorymanager.h"

#include <sstream>
#include <fstream>

#include "assert.h"

std::string MemoryManager::MemoryManagerValue::toString(bool free) const {
    std::ostringstream oss;

    if (!free) {
        switch (allocInfo_)
        {
        case (NEW):
            oss << "new";
            break;
        case (ARRAY_NEW):
            oss << "new[]";
            break;
        default:
            swiftAssert(false, "Illegal enum-value");
            break;
        }
    }
    else {
        switch (allocInfo_) {
        case (NEW):
            oss << "delete";
            break;
        case (ARRAY_NEW):
            oss << "delete[]";
            break;
        default:
            swiftAssert(false, "Illegal enum-value");
            break;
        }
    }

    oss << " in function at address " << caller_ << ". Call #" << callNumber_;

    return oss.str();
}

/*
    init static stuff
*/

bool    MemoryManager::isReady_                 = false;
uint    MemoryManager::breakpoints_[10]         = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
uint    MemoryManager::callCounter_             = 0;
size_t  MemoryManager::breakpointCounter_       = 0;
MemoryManager::PtrMap   MemoryManager::map_     = PtrMap();
volatile long           MemoryManager::lock_    = 0;

void MemoryManager::init() {
    isReady_ = true;
}

void MemoryManager::deinit() {
    isReady_ = false;

    std::ofstream mem_leaks("mem_leaks");
    std::ofstream calls("calls");

    if (map_.empty()) {
        std::cout << std::endl << std::endl << "No memory leaks detected :)" << std::endl;
        return;
    }

    std::cout << std::endl << "WARNING: Memory leaks detected" << std::endl;
    std::cout << "Please run" << std::endl;
    std::cout << "\t$ leakfinder <EXECUTABLE>" << std::endl;

    for (PtrMap::iterator iter = map_.begin(); iter != map_.end(); ++iter) {
        // skip infos in unknown locations
        //Skip infos in paths starting '/' or '\' (these things are bugs, or strange or magic things in libs)
        mem_leaks << iter->second.caller_ << std::endl;
        calls  << iter->second.callNumber_ << std::endl;
/*        if (iter->second.line_ <= 0 || iter->second.filename_[0] == '/' || iter->second.filename_[0] == '\\') {
            mem_leaks_full  << "   " << iter->second.toString() << " at address: " << (uintptr_t) iter->first << std::endl;
        }
        else {
            mem_leaks           << "   " << iter->second.toString() << " at address: " << (uintptr_t) iter->first << std::endl;
            mem_leaks_full      << "   " << iter->second.toString() << " at address: " << (uintptr_t) iter->first << std::endl;
        }*/
    }

    calls.close();
    mem_leaks.close();
}

std::string MemoryManager::toString() {
    std::ostringstream oss;
    PtrMap::iterator iter = map_.begin();
    while(iter != map_.end()) {
        oss << "   " << iter->second.toString() << std::endl;
        ++iter;
    }
    return oss.str();
}

void MemoryManager::add(void* p, AllocInfo info, void* caller)
{
    if (!isReady_ || lock_ > 0)
        return;

    volatile MemoryManager::MemoryManagerLock lock;

    MemoryManagerValue val(info, caller, callCounter_);
    map_[p] = val;
}

bool MemoryManager::remove(void* p, AllocInfo info, void* caller)
{
    if (!isReady_ || lock_ > 0)
        return true;

    volatile MemoryManagerLock lock;

    PtrMap::iterator iter = map_.find(p);

    if (iter == map_.end()) {
        switch (info) {
        case (NEW):
            std::cout << "ERROR: 'delete' used with free memory!!!" << std::endl;
            std::cout << "   delete called from " << caller << " at address: " << p << std::endl;
            break;
        case (ARRAY_NEW):
            std::cout << "ERROR: 'delete[]' used with free memory!!!" << std::endl;
            std::cout << "   delete[] called from " << caller << " at address: " << p << std::endl;
            break;
        default:
            swiftAssert(false, "Illegal enum-value");
            break;
        }
        return false;
    }

    if (iter->second.allocInfo_ == info) {
        map_.erase(iter);
        return true;
    }

    if ((iter->second.allocInfo_ == NEW)       && (info == ARRAY_NEW))
        std::cout << "ERROR: You called 'new' but deleted with 'delete[]'" << std::endl;
    if ((iter->second.allocInfo_ == ARRAY_NEW) && (info == NEW))
        std::cout << "ERROR: You called 'new[]' but deleted with 'delete'" << std::endl;

    std::cout << "   " << iter->second.toString() << std::endl;
    std::cout << "   " << MemoryManagerValue(info, caller, callCounter_).toString(true) << std::endl;

    map_.erase(iter);

    return false;
};

// New implementation of global new, delete and new[] and delete[]

void* operator new(size_t size)
{
    ++MemoryManager::callCounter_;

    void* p = malloc(size);
    // Is here a breakpoint?
    for (size_t i = 0; i < MemoryManager::breakpointCounter_; ++i) {
        if (MemoryManager::breakpoints_[i] == MemoryManager::callCounter_) {
            std::cout << "Breakpoint at adress: " << uintptr_t(p) << std::endl;
            SWIFT_THROW_BREAKPOINT;
            break;
        }
    }

    if (MemoryManager::isReady_)
        MemoryManager::add(p, MemoryManager::NEW, __builtin_return_address(0));

    return p;
};

void* operator new[](size_t size)
{
  ++MemoryManager::callCounter_;

    void* p = malloc(size);
    //Has been set a breakpoint here?
    for (size_t i = 0; i < MemoryManager::breakpointCounter_; ++i) {
        if (MemoryManager::breakpoints_[i] == MemoryManager::callCounter_) {
            std::cout << "Breakpoint at adress: " << uintptr_t(p) << std::endl;
            SWIFT_THROW_BREAKPOINT;
            break;
        }
    }

    if (MemoryManager::isReady_)
        MemoryManager::add(p, MemoryManager::ARRAY_NEW, __builtin_return_address(0));

    return p;
}

void operator delete(void* p)
{
    if (MemoryManager::isReady_) {
        if (p == NULL) {
            std::cout << "ERROR. You try to delete a NULL-pointer." << std::endl;
            std::cout << " in function at address " <<  __builtin_return_address(0) << ". Call #" << MemoryManager::callCounter_ << std::endl;
            return;
        }
        MemoryManager::remove(p, MemoryManager::NEW,  __builtin_return_address(0));
    }

    free(p);
}

void operator delete[](void* p)
{
    if (MemoryManager::isReady_) {
        if (p == NULL) {
            std::cout << "ERROR. You try to delete[] a NULL-pointer." << std::endl;
            std::cout << " in function at address " <<  __builtin_return_address(0) << ". Call #" << MemoryManager::callCounter_ << std::endl;
            return;
        }
        MemoryManager::remove(p, MemoryManager::ARRAY_NEW,  __builtin_return_address(0));
    }

    free(p);
}

#endif // SWIFT_DEBUG...
