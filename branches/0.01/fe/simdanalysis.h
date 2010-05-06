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

#ifndef SWIFT_SIMD_H
#define SWIFT_SIMD_H

#include <vector>

/*
 * forward declarations
 */

namespace me {
    class Reg;
}

namespace swift {

//------------------------------------------------------------------------------

struct SimdInfo
{
    me::Reg* ptr_;
    int simdLength_;
    int size_;
};

//------------------------------------------------------------------------------

class SimdAnalysis : public std::vector<SimdInfo> 
{
public:

    /*
     * constructor 
     */

    SimdAnalysis();

    /*
     * further methods
     */

    /**
     * @brief Checks the different simd lengths given in this vector and emits
     * an error message if appropiate.
     *
     * @param line The line number - needed if an error message must be thrown.
     * @return -1, on error, 0 if no definite result could have been made, >0
     * otherwise.
     */
    int checkAndGetSimdLength(int line);

    /// Returns the precalculated simd length.
    int getSimdLength() const;

private:

    enum 
    {
        NOT_ANALYZED = -1,
    };

    int simdLength_;
};

} // namespace swift

#endif // SWIFT_SIMD_H
