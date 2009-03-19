#include "fe/expr.h" 

namespace swift {

/*
 * forward declarations
 */

class ExprList;

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

protected:

    /*
     * further methods
     */

    void analyze(bool& result, TypeList& argTypeList, PlaceList& argPlaceList) const;

    /*
     * data
     */

    std::string* id_;
    ExprList* exprList_;
    int kind_;
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
