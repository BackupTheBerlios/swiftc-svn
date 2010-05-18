#include "fe/typenodeanalyzer.h"

#include "fe/class.h"
#include "fe/context.h"
#include "fe/error.h"
#include "fe/scope.h"
#include "fe/type.h"

namespace swift {

TypeNodeAnalyzer::TypeNodeVisitor(Context* ctxt)
    : TypeNodeVisitorBase(ctxt)
{}

void TypeNodeAnalyzer::visit(Decl* d)
{
    // check whether this type exists
    ctxt_->result_ &= d->type_->validate(ctxt_->module_);
        
    // is this the root scope and do we shadow a paramenter?
    if (ctxt_->scopeDepth() == 1 && ctxt_->memberFct_->sig_.lookupInOut( d->id() ) )
    {
        errorf(d->loc(), "local '%s' shadows a parameter", d->cid());
        ctxt_->result_ = false;
    }
    // is here a name clash in this scope?
    else if ( Local* local = ctxt_->scope()->lookupLocalOneLevelOnly(d->id()) )
    {
        errorf(d->loc(), 
                "there is already a local '%s' defined in this scope", d->cid());
        SWIFT_PREV_ERROR( local->loc() );
    }
    else
    {
        // everything fine - so register the local
        d->local_ = new Local( d->loc(), d->type_->clone(), new std::string(*d->id()) );
        ctxt_->scope()->insert(d->local_);
    }
}

void TypeNodeAnalyzer::visit(Id* id)
{
    // is it a local?
    Var* var = ctxt_->scope()->lookupLocal( id->id() );

    if (!var) // no - is it a paramenter/return value?
        var = ctxt_->memberFct_->sig_.lookupInOut( id->id() );

    if (!var)
    {
        errorf( id->loc(), "there is neither a local, nor a parameter, "
                "nor a return value '%s' defined in this scope",
                id->cid() );

        ctxt_->result_ = false;
    }
    else // this expresion is valid
        id->type_ = var->getType()->clone();
}

void TypeNodeAnalyzer::visit(Literal* l)
{
    //if ( neededAsLValue_ )
    //{
        //errorf(loc_, "lvalue required as left operand of assignment");
        //return false;
    //}

    switch ( l->getToken() )
    {
        case Token::L_INDEX:  l->type_ = new BaseType(l->loc(), Token::CONST, new std::string("index") ); break;

        case Token::L_INT:    l->type_ = new BaseType(l->loc(), Token::CONST, new std::string("int")   ); break;
        case Token::L_INT8:   l->type_ = new BaseType(l->loc(), Token::CONST, new std::string("int8")  ); break;
        case Token::L_INT16:  l->type_ = new BaseType(l->loc(), Token::CONST, new std::string("int16") ); break;
        case Token::L_INT32:  l->type_ = new BaseType(l->loc(), Token::CONST, new std::string("int32") ); break;
        case Token::L_INT64:  l->type_ = new BaseType(l->loc(), Token::CONST, new std::string("int64") ); break;
        case Token::L_SAT8:   l->type_ = new BaseType(l->loc(), Token::CONST, new std::string("sat8")  ); break;
        case Token::L_SAT16:  l->type_ = new BaseType(l->loc(), Token::CONST, new std::string("sat16") ); break;

        case Token::L_UINT:   l->type_ = new BaseType(l->loc(), Token::CONST, new std::string("uint")  ); break;
        case Token::L_UINT8:  l->type_ = new BaseType(l->loc(), Token::CONST, new std::string("uint8") ); break;
        case Token::L_UINT16: l->type_ = new BaseType(l->loc(), Token::CONST, new std::string("uint16")); break;
        case Token::L_UINT32: l->type_ = new BaseType(l->loc(), Token::CONST, new std::string("uint32")); break;
        case Token::L_UINT64: l->type_ = new BaseType(l->loc(), Token::CONST, new std::string("uint64")); break;
        case Token::L_USAT8:  l->type_ = new BaseType(l->loc(), Token::CONST, new std::string("usat8") ); break;
        case Token::L_USAT16: l->type_ = new BaseType(l->loc(), Token::CONST, new std::string("usat16")); break;

        case Token::L_REAL:   l->type_ = new BaseType(l->loc(), Token::CONST, new std::string("real")  ); break;
        case Token::L_REAL32: l->type_ = new BaseType(l->loc(), Token::CONST, new std::string("real32")); break;
        case Token::L_REAL64: l->type_ = new BaseType(l->loc(), Token::CONST, new std::string("real64")); break;

        case Token::L_TRUE:
        case Token::L_FALSE:  l->type_ = new BaseType(l->loc(), Token::CONST, new std::string("bool")  ); break;

        default:
            swiftAssert(false, "illegal switch-case-value");
    }
}

void TypeNodeAnalyzer::visit(Nil* n)
{
    if ( !n->innerType_->validate(ctxt_->module_) )
        ctxt_->result_ = false;
    else
        n->type_ = new Ptr(n->loc(), Token::CONST, n->innerType_->clone() );
}

void TypeNodeAnalyzer::visit(Self* s)
{
    if ( Method* m = dynamic_cast<Method*>(ctxt_->memberFct_) )
    {
        s->type_ = new BaseType( 
                s->loc(), 
                m->getModifier(), 
                new std::string(*ctxt_->class_->id()) );

    }
    else
    {
        errorf( s->loc(), "the 'self' keyword may only be used within methods" );
        ctxt_->result_ = false;
    }
}

void TypeNodeAnalyzer::preVisit(IndexExpr* i) {}
void TypeNodeAnalyzer::postVisit(IndexExpr* i)
{
}

void TypeNodeAnalyzer::preVisit(MemberAccess* m) {}
void TypeNodeAnalyzer::postVisit(MemberAccess* m)
{
    if (m->postfixExpr_)
    {
        if (m->postfixExpr_->type_)
        {
            if ( const BaseType* bt = m->postfixExpr_->type_->isInner() )
                m->class_ = bt->lookupClass(ctxt_->module_);
        }
    }
    else
    {
        if ( dynamic_cast<Method*>(ctxt_->memberFct_) )
            m->class_ = ctxt_->class_;
        else
        {
            errorf(m->loc(), "a %s does not provide a hidden 'self' parameter",
                    ctxt_->memberFct_->qualifierStr() );
        }
    }

    if (m->class_)
    {
        MemberVar* memberVar = m->class_->lookupMemberVar( m->id() );

        if (!memberVar)
        {
            errorf( m->loc(), "there is no member variable '%s' defined in class '%s'",
                m->cid(), m->class_->cid() );
        }
        else
            m->type_ = memberVar->getType()->clone();
    }
}

void TypeNodeAnalyzer::preVisit(CCall* c)
{
    if (c->retType_)
    {
        if ( !c->retType_->validate(ctxt_->module_) )
            ctxt_->result_ = false;
        else
            c->type_ = c->retType_->clone();
    }
}

void TypeNodeAnalyzer::postVisit(CCall* c)
{
}

void TypeNodeAnalyzer::preVisit(ReaderCall* r) {} 
void TypeNodeAnalyzer::postVisit(ReaderCall* r)
{
    setClass(r);
    analyzeMemberFctCall(r);
}

void TypeNodeAnalyzer::preVisit(WriterCall* w) {}
void TypeNodeAnalyzer::postVisit(WriterCall* w)
{
    setClass(w);
    analyzeMemberFctCall(w);
}

void TypeNodeAnalyzer:: preVisit(RoutineCall* r) {}
void TypeNodeAnalyzer::postVisit(RoutineCall* r)
{
    setClass(r);
    analyzeMemberFctCall(r);
}

void TypeNodeAnalyzer::preVisit(BinExpr* b) {}
void TypeNodeAnalyzer::postVisit(BinExpr* b)
{
    setClass(b);
    analyzeMemberFctCall(b);
}

void TypeNodeAnalyzer::preVisit(UnExpr* u) {}
void TypeNodeAnalyzer::postVisit(UnExpr* u)
{
    setClass(u);
    analyzeMemberFctCall(u);
}

void TypeNodeAnalyzer::analyzeMemberFctCall(MemberFctCall* m)
{
    if ( !m->exprList_->isValid() )
        return;

    TypeList inTypes = m->exprList_->buildTypeList();

    if (m->class_)
    {
        m->memberFct_ = m->class_->lookupMemberFct(ctxt_->module_, m->id(), inTypes);

        if (!m->memberFct_)
        {
            errorf( m->loc(), "there is no %s '%s(%s)' defined in class '%s'",
                m->qualifierStr(), m->cid(), inTypes.toString().c_str(), m->class_->cid() );

            ctxt_->result_ = false;
        }
        else if ( !m->memberFct_->sig_.outTypes_.empty() )
                m->type_ = m->memberFct_->sig_.outTypes_[0]->clone();
    }
}

void TypeNodeAnalyzer::setClass(MethodCall* m)
{
    if (m->expr_)
    {
        if (m->expr_->type_)
        {
            if ( const BaseType* bt = m->expr_->type_->isInner() )
                m->class_ = bt->lookupClass(ctxt_->module_);
        }
    }
    else
        m->class_ = ctxt_->class_;
}

void TypeNodeAnalyzer::setClass(OperatorCall* o)
{
    if (o->op1_ && o->op1_->type_)
    {
        if ( const BaseType* bt = o->op1_->type_->isInner() )
            o->class_ = bt->lookupClass(ctxt_->module_);
    }
}

void TypeNodeAnalyzer::setClass(RoutineCall* r)
{
    if (r->classId_)
    {
        r->class_ = ctxt_->module_->lookupClass(r->classId_);

        if (!r->class_)
        {
            errorf( r->loc(), "class '%s' is not defined in module '%s'", 
                    r->classId_->c_str(), ctxt_->module_->cid() );
        }
    }
    else
        r->class_ = ctxt_->class_;
}

} // namespace swift
