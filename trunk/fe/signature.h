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

#ifndef SWIFT_PROC
#define SWIFT_PROC

#include <map>
#include <string>
#include <vector>

#include "utils/stringhelper.h"

#include "fe/typelist.h"

namespace swift {

/*
 * forward declaraions
 */

class Local;
class Method;
class Param;
class InParam;
class OutParam;
class Scope;
class Statement;

//------------------------------------------------------------------------------

/**
 * This class abstracts a signature of a method, routine etc. It has a
 * List of Param objects and some useful methods.
*/
class Signature
{
public:

    /*
     * destructor
     */

    ~Signature();

    /* 
     * further methods
     */

    void appendInParam(InParam* param);
    void appendOutParam(OutParam* param);

    /// Analyses this Sig for correct syntax.
    bool analyze() const;

    /**
     * Check whether the ingoing part of the given Signature matches.
     *
     * @param in The TypeList which should be checked. 
     *
     * @return true -> it fits, flase -> otherwise.
     */
    bool checkIn(const TypeList& in) const;

    /**
     * Check whether the outgoing part of the given Signature matches.
     *
     * @param out The TypeList which should be checked. 
     *
     * @return true -> it fits, flase -> otherwise.
     */
    bool checkOut(const TypeList& out) const;

    /// Check whether two given signatures fit.
    bool check(const TypeList& in, const TypeList& out) const;
    
    bool check(const Signature* sig);
    
    /**
     * Find a Param by name.
     *
     * @return The Param or 0 if it was not found.
     */
    Param* findParam(const std::string* id);

    const Param* findParam(const std::string* id) const;

    std::string toString() const;

    const TypeList& getIn() const;
    const TypeList& getOut() const;

    Param* getInParam(size_t i);
    Param* getOutParam(size_t i);

    size_t getNumIn() const;
    size_t getNumOut() const;

private:

    /*
     * data
     */

    typedef std::vector<InParam*> InParams;
    typedef std::vector<OutParam*> OutParams;

    /**
     * @brief This list stores the \a Param objects. 
     *
     * The parameters are sorted from left to right as given in the signature
     * of the procedure. 
     */
    InParams inParams_;

    /**
     * @brief This list stores the \a Param objects. 
     *
     * The return parameters are sorted from left to right as given in the 
     * signature of the procedure. 
     */
    OutParams outParams_;

    TypeList inTypes_;
    TypeList outTypes_;
};

} // namespace swift

#endif //SWIFT_PROC
