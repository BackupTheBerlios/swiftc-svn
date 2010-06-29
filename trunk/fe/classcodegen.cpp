#include "fe/classcodegen.h"

#include <typeinfo>

#include <llvm/Module.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/TypeBuilder.h>

#include "utils/cast.h"

#include "fe/context.h"
#include "fe/scope.h"
#include "fe/stmntcodegen.h"
#include "fe/type.h"

using llvm::Value;

namespace swift {

ClassCodeGen::ClassVisitor(Context* ctxt)
    : ClassVisitorBase(ctxt)
    , scg_( new StmntCodeGen(ctxt) )
    , builder_(ctxt->builder_)
    , lctxt_( ctxt->lctxt() )
{}

ClassCodeGen::~ClassVisitor()
{}

void ClassCodeGen::visit(Create* c)
{
    codeGen(c);
}

void ClassCodeGen::visit(Reader* r)
{
    codeGen(r);
}

void ClassCodeGen::visit(Writer* w)
{
    codeGen(w);
}

void ClassCodeGen::visit(Assign* a)
{
    codeGen(a);
}

void ClassCodeGen::visit(Operator* o)
{
    codeGen(o);
}

void ClassCodeGen::visit(Routine* r)
{
    codeGen(r);
}

void ClassCodeGen::visit(MemberVar* m)
{
}

void ClassCodeGen::codeGen(MemberFct* m)
{
    if ( m->isAutoGenerated() )
        return;

    // get some stuff for easy access
    TypeList& out = m->sig_.outTypes_;
    llvm::Function* fct = m->llvmFct_;

    // update context
    ctxt_->llvmFct_ = fct;

    /*
     * emit code for the function
     */

    // create root BB and connect to fct and to the builder    
    llvm::BasicBlock* bb = llvm::BasicBlock::Create(lctxt_, m->llvmName_, fct);
    builder_.SetInsertPoint(bb);

    // create return basic block
    m->returnBB_ = llvm::BasicBlock::Create(lctxt_, "return");

    // initialize return values
    for (size_t i = 0; i < out.size(); ++i)
    {
        RetVal* retval = m->sig_.out_[i];
        retval->createEntryAlloca(ctxt_);
    }

    llvm::Function::arg_iterator iter = fct->arg_begin();

    // initialize 'self' alloca
    if ( Method* method = dynamic<Method>(m) )
    {
        // create alloca and store the initial
        method->selfValue_ = builder_.CreateAlloca( m->params_[0], 0, "self" );
        builder_.CreateStore(iter,  method->selfValue_);

        iter->setName("self");
        //iter->addAttr(llvm::Attribute::InReg);
        ++iter;
    }

    // initialize params
    for (size_t i = 0; i < m->realIn_.size(); ++iter, ++i)
    {
        InOut* io = m->realIn_[i];

        // create alloca and store the initial
        llvm::AllocaInst* alloca = io->createEntryAlloca(ctxt_);
        builder_.CreateStore(iter, alloca);

        iter->setName( io->cid() );
    }

    // enter scope and gen code
    StmntCodeGen scg(ctxt_);
    m->scope_->accept(&scg);

    /*
     * build epilogue
     */

    // connect last bb with return bb
    builder_.CreateBr(m->returnBB_);
    fct->getBasicBlockList().push_back(m->returnBB_);
    builder_.SetInsertPoint(m->returnBB_);

    if ( m->realOut_.empty() )
        builder_.CreateRetVoid();
    else 
    {
        if (m->main_)
        {
            RetVal* retval = m->sig_.out_[0];
            Value* value = builder_.CreateLoad( 
                    retval->getAddr(builder_), retval->cid() );
            builder_.CreateRet(value);
        }
        else
        {
            Value* retStruct = llvm::UndefValue::get(m->retType_);
            for (size_t i = 0; i < m->realOut_.size(); ++i)
            {
                RetVal* retval = m->realOut_[i];
                retStruct = builder_.CreateInsertValue(
                        retStruct, 
                        builder_.CreateLoad( retval->getAddr(builder_), retval->cid() ), 
                        i,
                        "retval");
            }

            builder_.CreateRet(retStruct);
        }
    }
}

} // namespace swift
