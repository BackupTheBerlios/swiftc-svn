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

namespace swift {

/*
 * forward declaraions
 */

class Local;
class Method;
class Param;
class Scope;
class Statement;
class Type;

typedef std::vector<const Type*> TypeList;

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

    void appendInParam(Param* param);
    void appendOutParam(Param* param);

    size_t getNumIn() const;
    size_t getNumOut() const;

    Param* getIn(size_t i);
    Param* getOut(size_t i);

    /// Analyses this Sig for correct syntax.
    bool analyze() const;

    /**
     * Check whether the ingoing part of the given Sig matches.
     *
     * @param inSig The Sig which should be checked. It is assumed that this
     *      this Sig only has an ingoing part.
     *
     * @return true -> it fits, flase -> otherwise.
     */
    bool checkIngoing(const Signature* sig) const;

    /// Check whether two given signatures fit.
    bool check(const Signature* sig);

    /**
     * Check whether the ingoing part of the given Sig matches.
     *
     * @param in The TypeList which should be checked. 
     *
     * @return true -> it fits, flase -> otherwise.
     */
    bool check(const TypeList& in) const;

    /// Check whether two given signatures fit.
    bool check(const TypeList& in, const TypeList& out) const;

    /**
     * Find a Param by name.
     *
     * @return The Param or 0 if it was not found.
     */
    Param* findParam(const std::string* id);

    const Param* findParam(const std::string* id) const;

    std::string toString() const;

private:

    /*
     * data
     */

    typedef std::vector<Param*> Params;

    /**
     * @brief This list stores the \a Param objects. 
     *
     * The parameters are sorted from left to right as given in the Sig 
     * of the procedure. 
     */
    Params in_;

    /**
     * @brief This list stores the \a Param objects. 
     *
     * The return parameters are sorted from left to right as given in the Sig 
     * of the procedure. 
     */
    Params out_;
};

} // namespace swift

#endif //SWIFT_PROC
