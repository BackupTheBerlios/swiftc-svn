/*
 * Swift compiler framework
 * Copyright (C) 2007-2009 Roland Leißa <r_leis01@math.uni-muenster.de>
 *
 * This framework is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 3 as published by the Free Software Foundation.
 *
 * This framework is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this framework; see the file LICENSE. If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef SWIFT_MEMMGR_H
#define SWIFT_MEMMGR_H

#if defined(SWIFT_DEBUG) && defined(__GNUC__)

#include <iostream>
#include <map>
#include <new>
#include <string>
#include <sstream>

#include "utils/types.h"
/*
    Declaration of the dynamic memory operators to be overloaded
*/
void* operator new  (size_t size);
void* operator new[](size_t size);
void  operator delete   (void* p);
void  operator delete[] (void* p);

/**
 * This class is a memory tracer. All new, new[], delete and delete[] operatores
 * will be traced after initialization which can be done with the Singleton
 * class.
*/
class MemMgr {
private:

    /**
     * so many Breakpoints can be used at most
     */
    enum {
        MAX_BREAKPOINTS = 10
    };

    /**
     * Kind of allocation
     */
    enum AllocInfo {
        NEW,
        ARRAY_NEW
    };

private:

    // Friend declarations of the dynamic memory operators
    friend void* ::operator new  (size_t size);
    friend void* ::operator new[](size_t size);
    friend void  ::operator delete  (void* p);
    friend void  ::operator delete[](void* p);

    /**
     * Intern class, which capsulates the values of the inner map.
     * Contains allocinfos, file name and line number
    */
    class MemMgrValue {
    public:

        AllocInfo   allocInfo_;
        void*       caller_;
        uint        callNumber_;

        /// default constructor -- does nothing
        MemMgrValue() {}

        /**
         * Init MemMgrValue.
         * @param info Allocinfo
         * @param filename file name
         * @param line Zeilennummer
        */
        MemMgrValue(AllocInfo info, void* caller, uint callNumber)
            :  allocInfo_(info)
            , caller_(caller)
            , callNumber_(callNumber)
        {}

        /**
         * Returns a string of this class in readable form.
         * @param free
         * False: An output is created as this instance contains a new or new[] trace.
         * True:  An output is created as this instance contains a delete or delete[] trace.
         * @return The output string
        */
        std::string toString(bool free = false) const;
    };

    /// This data structure keeps account
    typedef std::map<void*, MemMgrValue> PtrMap;

    static PtrMap           map_;
    static bool             isReady_;
    static uint             breakpoints_[MAX_BREAKPOINTS];
    static size_t           breakpointCounter_;
    static uint             callCounter_;
    volatile static long    lock_;

public:

    /**
     * Inits this class. Please hold only one instance simultanously in memory.
    */
    static void init();

    /**
     * Deinits this class and gives an output.
    */
    static void deinit();

    /**
     * Returns the current content of the map in readable form.
     * @return The output string
    */
    static std::string toString();

    /**
     * Turns the manager off
    */
    static void turnOff() {
        isReady_ = false;
    }

    /**
     * Turns the manager on
    */
    static void turnOn() {
        isReady_ = true;
    }

    /**
     * Sets a breakpoint for a new or new[]-call
     * @param call the call number
    */
    static void setBreakpoint(uint call) {
        if (breakpointCounter_ >= MAX_BREAKPOINTS) {
            std::cout << "Sorry, can't take any further breakpoints" << std::endl;
            return;
        }
        breakpoints_[breakpointCounter_] = call;
        std::cout << "Breakpoint #" << breakpointCounter_ << " prepared for call #"
            << breakpoints_[breakpointCounter_] << std::endl;
        ++breakpointCounter_;
    }

    /**
     * This class keeps the memory manager from tracing itself
    */
    class MemMgrLock {
    public:
        MemMgrLock() {
            ++MemMgr::lock_;
        }
        ~MemMgrLock() {
            --MemMgr::lock_;
        }
    };

private:

    /**
     * adds an new pointer to the inner map
     * @param p the new pointer
     * @param info alloc info
     * @param filename file name
     * @param line line number
    */
    static void add(void* p, AllocInfo info, void* caller);

    /**
     * Deletes pointer of the inner map and returns an error message if applicable
     * @param p the pointer which should be removed
     * @param info allocinfo
    */
    static bool remove(void* p, AllocInfo info, void* caller);
};

#else // defined(SWIFT_DEBUG) && defined(__GNUC__)

// Here you can implement your own memory manger for the release version if you like

#endif // defined(SWIFT_DEBUG) && defined(__GNUC__)

// macros for easy usage for the debug and release version
#if defined(SWIFT_DEBUG) && defined(__GNUC__)
    /**
     * Turn on the MemMgr
    */
    #define SWIFT_TURN_ON_MEMMGR   SWIFT_::MemMgr::turnOn();

    /**
     * Turn off the MemMgr
    */
    #define SWIFT_TURN_OFF_MEMMGR  SWIFT_::MemMgr::turnOff();

    /**
     * Print the current map of the MemMgr
    */
    #define SWIFT_PRINT_MEM_MAP    SWIFT_::MemMgr::toString();
#else // defined(SWIFT_DEBUG) && defined(__GNUC__)
    //Empty macros in the release version
    #define SWIFT_TURN_ON_MEMMGR
    #define SWIFT_TURN_OFF_MEMMGR
    #define SWIFT_PRINT_MEM_MAP
#endif // defined(SWIFT_DEBUG) && defined(__GNUC__)

#endif // SWIFT_MEMMGR_H
