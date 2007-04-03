#if defined(SWIFT_DEBUG) && defined(__GNUC__)

#include "memmgr.h"

#include <sstream>
#include <fstream>

#include "assert.h"

std::string MemMgr::MemMgrValue::toString(bool free) const {
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

bool    MemMgr::isReady_                 = false;
uint    MemMgr::breakpoints_[10]         = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
uint    MemMgr::callCounter_             = 0;
size_t  MemMgr::breakpointCounter_       = 0;
MemMgr::PtrMap   MemMgr::map_     = PtrMap();
volatile long           MemMgr::lock_    = 0;

void MemMgr::init() {
    isReady_ = true;
}

void MemMgr::deinit() {
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

std::string MemMgr::toString() {
    std::ostringstream oss;
    PtrMap::iterator iter = map_.begin();
    while(iter != map_.end()) {
        oss << "   " << iter->second.toString() << std::endl;
        ++iter;
    }
    return oss.str();
}

void MemMgr::add(void* p, AllocInfo info, void* caller)
{
    if (!isReady_ || lock_ > 0)
        return;

    volatile MemMgr::MemMgrLock lock;

    MemMgrValue val(info, caller, callCounter_);
    map_[p] = val;
}

bool MemMgr::remove(void* p, AllocInfo info, void* caller)
{
    if (!isReady_ || lock_ > 0)
        return true;

    volatile MemMgrLock lock;

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
    std::cout << "   " << MemMgrValue(info, caller, callCounter_).toString(true) << std::endl;

    map_.erase(iter);

    return false;
};

// New implementation of global new, delete and new[] and delete[]

void* operator new(size_t size)
{
    ++MemMgr::callCounter_;

    void* p = malloc(size);
    // Is here a breakpoint?
    for (size_t i = 0; i < MemMgr::breakpointCounter_; ++i) {
        if (MemMgr::breakpoints_[i] == MemMgr::callCounter_) {
            std::cout << "Breakpoint at adress: " << uintptr_t(p) << std::endl;
            SWIFT_THROW_BREAKPOINT;
            break;
        }
    }

    if (MemMgr::isReady_)
        MemMgr::add(p, MemMgr::NEW, __builtin_return_address(0));

    return p;
};

void* operator new[](size_t size)
{
  ++MemMgr::callCounter_;

    void* p = malloc(size);
    //Has been set a breakpoint here?
    for (size_t i = 0; i < MemMgr::breakpointCounter_; ++i) {
        if (MemMgr::breakpoints_[i] == MemMgr::callCounter_) {
            std::cout << "Breakpoint at adress: " << uintptr_t(p) << std::endl;
            SWIFT_THROW_BREAKPOINT;
            break;
        }
    }

    if (MemMgr::isReady_)
        MemMgr::add(p, MemMgr::ARRAY_NEW, __builtin_return_address(0));

    return p;
}

void operator delete(void* p)
{
    if (MemMgr::isReady_) {
        if (p == NULL) {
            std::cout << "ERROR. You try to delete a NULL-pointer." << std::endl;
            std::cout << " in function at address " <<  __builtin_return_address(0) << ". Call #" << MemMgr::callCounter_ << std::endl;
            return;
        }
        MemMgr::remove(p, MemMgr::NEW,  __builtin_return_address(0));
    }

    free(p);
}

void operator delete[](void* p)
{
    if (MemMgr::isReady_) {
        if (p == NULL) {
            std::cout << "ERROR. You try to delete[] a NULL-pointer." << std::endl;
            std::cout << " in function at address " <<  __builtin_return_address(0) << ". Call #" << MemMgr::callCounter_ << std::endl;
            return;
        }
        MemMgr::remove(p, MemMgr::ARRAY_NEW,  __builtin_return_address(0));
    }

    free(p);
}

#endif // SWIFT_DEBUG...
