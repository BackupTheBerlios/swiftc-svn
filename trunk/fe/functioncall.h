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

#include "fe/expr.h" 
#include "fe/typelist.h" 

namespace swift {

/*
 * forward declarations
 */

class ExprList;
class Method;

class FunctionCall : public Expr
{
public:

    /*
     * constructor and destructor
     */

    FunctionCall(std::string* id, 
                 ExprList* exprList, 
                 int kind, 
                 int line = NO_LINE);

    virtual ~FunctionCall();

    /*
     * virtual methods
     */

    virtual void genSSA();

    /*
     * further methods
     */

    Method* getMethod();

protected:

    void analyze(bool& result, TypeList& argTypeList, PlaceList& argPlaceList) const;
    bool analyze(Class* _class, const TypeList& argTypeList);

    /*
     * data
     */

    std::string* id_;
    ExprList* exprList_;
    int kind_;
    Method* method_;
};

//------------------------------------------------------------------------------

class CCall : public FunctionCall
{
public:

    /*
     * constructor and destructor
     */

    CCall(Type* returnType, 
          std::string* id, 
          ExprList* exprList, 
          int kind, 
          int line = NO_LINE);

    virtual ~CCall();

    /*
     * virtual methods
     */

    virtual bool analyze();

private:

    /*
     * data
     */

    Type* returnType_;
};

//------------------------------------------------------------------------------

class RoutineCall : public FunctionCall
{
public:

    /*
     * constructor and destructor
     */

    RoutineCall(std::string* classId, 
                std::string* id, 
                ExprList* exprList, 
                int kind, 
                int line = NO_LINE);

    virtual ~RoutineCall();

    /*
     * virtual methods
     */

    virtual bool analyze();

private:

    /*
     * data
     */

    std::string* classId_;
};

//------------------------------------------------------------------------------

class MethodCall : public FunctionCall
{
public:

    /*
     * constructor and destructor
     */

    MethodCall(Expr* expr, 
               std::string* id, 
               ExprList* exprList, 
               int kind, 
               int line = NO_LINE);

    virtual ~MethodCall();

    /*
     * virtual methods
     */

    virtual bool analyze();

private:

    /*
     * data
     */

    Expr* expr_;
};

//------------------------------------------------------------------------------

} // namespace swift
