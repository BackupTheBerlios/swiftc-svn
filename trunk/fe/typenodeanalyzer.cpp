#include "fe/typenodeanalyzer.h"

#include "utils/cast.h"

#include "fe/class.h"
#include "fe/context.h"
#include "fe/tnlist.h"
#include "fe/error.h"
#include "fe/scope.h"
#include "fe/type.h"

#define SWIFT_ERROR_ONLY_WITHIN_SIMD_LOOPS(loc) errorf((loc), "a simd index may only be used within simd loops");


namespace swift {

TypeNodeAnalyzer::TypeNodeVisitor(Context* ctxt)
    : TypeNodeVisitorBase(ctxt)
{}

void TypeNodeAnalyzer::visit(Decl* d)
{
    // check whether this type exists
    ctxt_->result_ &= d->get().type_->validate(ctxt_->module_);

    // is here a name clash in this scope?
    if ( Var* var = ctxt_->scope()->lookupVarOneLevelOnly(d->id()) )
    {
        if ( dynamic<InOut>(var) )
            errorf( d->loc(), "local '%s' shadows a parameter", d->cid());
        else
        {
            errorf( d->loc(), 
                    "there is already a local '%s' defined in this scope", 
                    d->cid() );
        }

        SWIFT_PREV_ERROR( var->loc() );

        goto error_out;
    }
    else if ( const Simd* simd = d->get().type_->cast<Simd>() )
    {
        const Type* inner = simd->getInnerType();

        if ( const BaseType* bt = inner->cast<BaseType>() )
        {
            if ( !bt->lookupClass(ctxt_->module_)->isSimd() )
            {
                errorf( d->loc(), 
                        "the inner type '%s' of the simd container '%s' "
                        "is not declared with 'simd'",
                        inner->toString().c_str(),
                        d->cid() );
                goto error_out;
            }
            else
                goto ok_out;
        }
        else
        {
            errorf( d->loc(), 
                    "the inner type of an simd container must be a base type "
                    "but type '%s' is given",
                    inner->toString().c_str() );
            goto error_out;
        }
    }
    else
        goto ok_out;

error_out:
    // substiture decl's type with an error type and set error
    delete d->get().type_;
    setError(d, true);
    return;

ok_out:

    // everything fine - so register the local
    d->local_ = new Local( 
            d->loc(), d->get().type_->clone(), new std::string(*d->id()) );
    ctxt_->scope()->insert(d->local_);

    d->results_.resize(1);
    d->set().inits_ = false;
    d->set().lvalue_ = true;
}

void TypeNodeAnalyzer::visit(ErrorExpr* e)
{
    setError(e, false);
}

void TypeNodeAnalyzer::visit(Id* id)
{
    Var* var = ctxt_->scope()->lookupVar( id->id() );

    if (!var)
    {
        errorf( id->loc(), "there is neither a local, nor a parameter, "
                "nor a return value '%s' defined in this scope",
                id->cid() );

        setError(id, true);
        return;
    }

    // this expresion is valid
    setResult(id, var->getType()->clone(), true);
}

void TypeNodeAnalyzer::visit(Literal* l)
{
    Type* type;

    switch ( l->getToken() )
    {
        case Token::L_INDEX:  type = new ScalarType(l->loc(), Token::CONST, new std::string("index") ); break;

        case Token::L_INT:    type = new ScalarType(l->loc(), Token::CONST, new std::string("int")   ); break;
        case Token::L_INT8:   type = new ScalarType(l->loc(), Token::CONST, new std::string("int8")  ); break;
        case Token::L_INT16:  type = new ScalarType(l->loc(), Token::CONST, new std::string("int16") ); break;
        case Token::L_INT32:  type = new ScalarType(l->loc(), Token::CONST, new std::string("int32") ); break;
        case Token::L_INT64:  type = new ScalarType(l->loc(), Token::CONST, new std::string("int64") ); break;
        case Token::L_SAT8:   type = new ScalarType(l->loc(), Token::CONST, new std::string("sat8")  ); break;
        case Token::L_SAT16:  type = new ScalarType(l->loc(), Token::CONST, new std::string("sat16") ); break;

        case Token::L_UINT:   type = new ScalarType(l->loc(), Token::CONST, new std::string("uint")  ); break;
        case Token::L_UINT8:  type = new ScalarType(l->loc(), Token::CONST, new std::string("uint8") ); break;
        case Token::L_UINT16: type = new ScalarType(l->loc(), Token::CONST, new std::string("uint16")); break;
        case Token::L_UINT32: type = new ScalarType(l->loc(), Token::CONST, new std::string("uint32")); break;
        case Token::L_UINT64: type = new ScalarType(l->loc(), Token::CONST, new std::string("uint64")); break;
        case Token::L_USAT8:  type = new ScalarType(l->loc(), Token::CONST, new std::string("usat8") ); break;
        case Token::L_USAT16: type = new ScalarType(l->loc(), Token::CONST, new std::string("usat16")); break;

        case Token::L_REAL:   type = new ScalarType(l->loc(), Token::CONST, new std::string("real")  ); break;
        case Token::L_REAL32: type = new ScalarType(l->loc(), Token::CONST, new std::string("real32")); break;
        case Token::L_REAL64: type = new ScalarType(l->loc(), Token::CONST, new std::string("real64")); break;

        case Token::L_BOOL:   type = new ScalarType(l->loc(), Token::CONST, new std::string("bool")  ); break;

        default:
            type = 0;
            swiftAssert(false, "illegal switch-case-value");
    }

    setResult(l, type, false);
}

void TypeNodeAnalyzer::visit(Nil* n)
{
    if ( !n->type_->validate(ctxt_->module_) )
        setError(n, false);
    else
        setResult(n, new Ptr(n->loc(), Token::CONST, n->type_->clone() ), false);
}

void TypeNodeAnalyzer::visit(Self* s)
{
    if ( Method* m = dynamic<Method>(ctxt_->memberFct_) )
    {
         Type* type = new UserType( 
                 s->loc(), 
                 m->getModifier(), 
                 new std::string(*ctxt_->class_->id()) );
         setResult(s, type, false);
    }
    else
    {
        errorf( s->loc(), 
                "the 'self' keyword may only be used within non-static methods" );
        setError(s, false);
    }
}

bool TypeNodeAnalyzer::examinePrefixExpr(Access* a)
{
    // examine prefix expr
    a->prefixExpr_->accept(this);

    if ( a->prefixExpr_->numResults() != 1 )
    {
        errorf( a->loc(), "the prefix expression of a member access "
                "must exactly return one element" );
        setError(a, true);
        return false;
    }

    return true;
}

void TypeNodeAnalyzer::visit(IndexExpr* i)
{
    i->indexExpr_->accept(this);

    if ( i->indexExpr_->numResults() != 1 )
    {
        errorf( i->loc(), "an indexing expression must exactly return one element" );
        setError(i, true);
        return;
    }

    const Type* indexType = i->indexExpr_->get().type_;
    if ( indexType->isIndex() != 1 )
    {
        errorf( i->loc(), 
                "the indexing type of an indexing expression "
                "must be of type 'index' but type '%s' is given", 
                indexType->toString().c_str() );
        setError(i, true);
        return;
    }

    if ( !examinePrefixExpr(i) )
        return;

    const Type* prefixType = i->prefixExpr_->get().type_->derefPtr();
    if ( const Container* container = prefixType->derefPtr()->cast<Container>() )
        setResult(i, container->getInnerType()->clone(), true);
    else
    {
        errorf( i->loc(), 
                "the indexed expression must be an array or simd-container "
                "but type '%s' is given", 
                prefixType->toString().c_str() );
        setError(i, true);
    }
}

void TypeNodeAnalyzer::visit(SimdIndexExpr* s)
{
    if (!ctxt_->simdIndex_)
    {
        SWIFT_ERROR_ONLY_WITHIN_SIMD_LOOPS( s->loc() );
        setError(s, false);
        return;
    }

    if ( !examinePrefixExpr(s) )
        return;

    if (!ctxt_->simdIndex_)
    {
        errorf(s->loc(), "the simd index '@' may only be used within simd loops");
        setError(s, true);
        return;
    }

    const Type* prefixType = s->prefixExpr_->get().type_->derefPtr();
    if ( const Container* container = prefixType->derefPtr()->cast<Container>() )
        setResult(s, container->getInnerType()->clone(), true);
    else
    {
        errorf( s->loc(), 
                "the indexed expression must be an array or simd-container "
                "but type '%s' is given", 
                prefixType->toString().c_str() );
        setError(s, true);
    }
}

void TypeNodeAnalyzer::visit(MemberAccess* m)
{
    if ( !examinePrefixExpr(m) )
        return;

    const Type* prefixType = m->prefixExpr_->get().type_;

    if ( const UserType* user = prefixType->derefPtr()->cast<UserType>() )
    {
        m->class_ = user->lookupClass(ctxt_->module_);

        if (!m->class_)
        {
            SWIFT_CONFIRM_ERROR;
            setError(m, true);
            return;
        }

        m->memberVar_ = m->class_->lookupMemberVar( m->id() );

        if (!m->memberVar_)
        {
            errorf( m->loc(), "there is no member variable '%s' defined in class '%s'",
                m->cid(), m->class_->cid() );
            setError(m, true);
            return;
        }
        else
        {
            const Type* memType = m->memberVar_->getType();
            Type* resType = memType->clone();
            setResult(m, resType, true);
            return;
        }
    }
    if ( prefixType->cast<ErrorType>() )
    {
        SWIFT_CONFIRM_ERROR;
        return;
    }

    swiftAssert(false, "TODO");
}

void TypeNodeAnalyzer::visit(CCall* c)
{
    TNList& args = *c->exprList_;
    args.accept(this);

    if (ctxt_->simdIndex_)
    {
        bool error = false;

        if (error)
        {
            setError(c, false);
            return;
        }
    }

    if ( c->retType_ )
    {
        if ( !c->retType_->validate(ctxt_->module_) )
            setError(c, false);
        else
            setResult( c, c->retType_->clone(), false );
    }
    else
        c->results_.clear();
}

void TypeNodeAnalyzer::visit(MethodCall* m)
{
    if ( setClass(m) )
        analyzeMemberFctCall(m);
}

void TypeNodeAnalyzer::visit(CreateCall* c)
{
    swiftAssert(false, "TODO");
    //if ( setClass(r) )
        //analyzeMemberFctCall(r);
    // TODO
}

void TypeNodeAnalyzer::visit(RoutineCall* r)
{
    if ( setClass(r) )
        analyzeMemberFctCall(r);
}

void TypeNodeAnalyzer::visit(UnExpr* u)
{
    if ( setClass(u) )
        analyzeMemberFctCall(u);

    if ( !u->op1_->get().type_->cast<ScalarType>() )
        u->builtin_ = false;
}

void TypeNodeAnalyzer::visit(BinExpr* b)
{
    if ( setClass(b) )
        analyzeMemberFctCall(b);

    if ( !b->op1_->get().type_->cast<ScalarType>() )
        b->builtin_ = false;
}

bool TypeNodeAnalyzer::setClass(MethodCall* m)
{
    bool error = false;

    m->expr_->accept(this);

    if (m->expr_->numResults() != 1)
    {
        errorf( m->loc(), "the prefix expression of a method call "
                "must exactly return one element" );
        error = true;
    }

    // check the arg list even on error
    m->exprList_->accept(this);

    if (!error)
    {
        const Type* type = m->expr_->get().type_;

        if ( const BaseType* bt = type->derefPtr()->cast<BaseType>() )
        {
            m->class_ = bt->lookupClass(ctxt_->module_);
            return m->class_;
        }

        errorf( m->loc(), "TODO" );
    }

    setError(m, false);
    return false;
}

bool TypeNodeAnalyzer::setClass(RoutineCall* r)
{
    r->exprList_->accept(this);
    r->class_ = ctxt_->module_->lookupClass(r->classId_);

    if (!r->class_)
    {
        errorf( r->loc(), "class '%s' is not defined in module '%s'", 
                r->classId_->c_str(), ctxt_->module_->cid() );

        setError(r, false);

        return false;
    }

    return true;
}

void TypeNodeAnalyzer::analyzeMemberFctCall(MemberFctCall* m)
{
    const TypeList& in = m->exprList_->typeList();
    m->memberFct_ = m->class_->lookupMemberFct(ctxt_->module_, m->id(), in);

    if (!m->memberFct_)
    {
        errorf( m->loc(), 
                "there is no %s '%s(%s)' defined in class '%s'",
                m->qualifierStr(), 
                m->cid(), 
                in.toString().c_str(), 
                m->class_->cid() );
        setError(m, false);
        return;
    }

    const TypeList& out = m->memberFct_->sig().outTypes_;

    m->results_.clear();

    for (size_t i = 0; i < out.size(); ++i)
    {
        const Type* outType = out[i];

        m->results_.push_back( TNResult() );

        m->set(i).lvalue_ = false;
        m->set(i).inits_  = outType->perRef();
        m->set(i).type_   = outType->clone();
    }
}

void TypeNodeAnalyzer::setResult(TypeNode* tn, Type* type, bool lvalue)
{
    tn->results_.resize(1);

    tn->set().type_   = type; 
    tn->set().inits_  = false;
    tn->set().lvalue_ = lvalue;
}

void TypeNodeAnalyzer::setError(TypeNode* tn, bool lvalue)
{
    tn->results_.resize(1);

    tn->set().type_   = new ErrorType();
    tn->set().inits_  = false;
    tn->set().lvalue_ = lvalue;

    ctxt_->result_ = false;
}

} // namespace swift
