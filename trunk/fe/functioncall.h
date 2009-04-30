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

    FunctionCall(std::string* id, ExprList* exprList, int line);
    virtual ~FunctionCall();

    /*
     * virtual methods
     */

    virtual void genSSA();

    /*
     * further methods
     */

    void setTupel(Tupel* tupel);
    bool analyzeArgs();

protected:

    bool analyzeExprList() const;

    std::string callToString() const;

    /*
     * data
     */

    std::string* id_;
    ExprList* exprList_;
    Tupel* tupel_;

    std::vector<me::Op*> in_;
    std::vector<me::Var*> out_;
};

//------------------------------------------------------------------------------

class CCall : public FunctionCall
{
public:

    /*
     * constructor and destructor
     */

    CCall(Type* returnType, 
          int kind, 
          std::string* id, 
          ExprList* exprList, 
          int line);

    virtual ~CCall();

    /*
     * virtual methods
     */

    virtual bool analyze();
    virtual std::string toString() const;

private:

    /*
     * data
     */

    Type* returnType_;
    int kind_;
};

//------------------------------------------------------------------------------

class MemberFunctionCall : public FunctionCall
{
public:

    /*
     * constructor 
     */

    MemberFunctionCall(std::string* id, ExprList* exprList, int line);

protected:

    /*
     * further methods
     */

    bool analyze(Class* _class, const TypeList& argTypeList);

    /*
     * data
     */

    std::string* classId_;
    Class* class_;
    MemberFunction* memberFunction_;
};

//------------------------------------------------------------------------------

class RoutineCall : public MemberFunctionCall
{
public:

    /*
     * constructor and destructor
     */

    RoutineCall(std::string* classId, 
                std::string* id, 
                ExprList* exprList, 
                int line);

    virtual ~RoutineCall();

    /*
     * virtual methods
     */

    virtual bool analyze();
    virtual std::string toString() const;

private:

    /*
     * data
     */

    std::string* classId_;
};


//------------------------------------------------------------------------------

class MethodCall : public MemberFunctionCall
{
public:

    /*
     * constructor and destructor
     */

    MethodCall(Expr* expr, 
               std::string* id, 
               ExprList* exprList, 
               int line);

    virtual ~MethodCall();

    /*
     * virtual methods
     */

    virtual bool analyze();
    virtual std::string toString() const;
    //virtual bool specialAnalyze() = 0;
    virtual std::string concatentationStr() const = 0;

private:

    /*
     * data
     */

    Expr* expr_;
};

//------------------------------------------------------------------------------

class ReaderCall : public MethodCall
{
public:

    /*
     * constructor 
     */

    ReaderCall(Expr* expr, 
               std::string* id, 
               ExprList* exprList, 
               int line);

    /*
     * virtual methods
     */

    //virtual bool specialAnalyze();
    virtual std::string concatentationStr() const;

protected:

    /*
     * data
     */

    Expr* expr_;
};

//------------------------------------------------------------------------------

class WriterCall : public MethodCall
{
public:

    /*
     * constructor
     */

    WriterCall(Expr* expr, 
               std::string* id, 
               ExprList* exprList, 
               int line);

    /*
     * virtual methods
     */

    //virtual bool specialAnalyze();
    virtual std::string concatentationStr() const;
};

} // namespace swift
