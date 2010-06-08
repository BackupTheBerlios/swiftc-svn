#include "fe/classcodegen.h"

#include <typeinfo>

#include <llvm/Module.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/TypeBuilder.h>

#include "fe/context.h"
#include "fe/scope.h"
#include "fe/stmntcodegen.h"
#include "fe/type.h"

namespace swift {

ClassCodeGen::ClassVisitor(Context* ctxt)
    : ClassVisitorBase(ctxt)
    , scg_( new StmntCodeGen(ctxt) )
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
    if ( m->isTrivial() )
        return; // do nothing

    /*
     * get some stuff for easy access
     */

    TypeList& out = m->sig_.outTypes_;
    llvm::Module* llvmModule = ctxt_->module_->getLLVMModule();
    llvm::LLVMContext& llvmCtxt = llvmModule->getContext();
    llvm::IRBuilder<>& builder = ctxt_->builder_;
    llvm::Function* fct = m->llvmFct_;

    // update context
    ctxt_->llvmFct_ = fct;

    /*
     * emit code for the function
     */

    // create root BB and connect to fct and to the builder    
    llvm::BasicBlock* bb = llvm::BasicBlock::Create(llvmCtxt, m->llvmName_, fct);
    builder.SetInsertPoint(bb);

    // create return basic block
    m->returnBB_ = llvm::BasicBlock::Create(llvmCtxt, "return");

    // initialize return values
    for (size_t i = 0; i < out.size(); ++i)
    {
        RetVal* retval = m->sig_.out_[i];
        retval->createEntryAlloca(ctxt_);
    }

    llvm::Function::arg_iterator iter = fct->arg_begin();

    // initialize 'self' alloca
    if ( Method* method = dynamic_cast<Method*>(m) )
    {
        // create alloca and store the initial
        method->selfValue_ = builder.CreateAlloca( m->params_[0], 0, "self" );
        builder.CreateStore(iter,  method->selfValue_);

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
        builder.CreateStore(iter, alloca);

        iter->setName( io->cid() );
    }

    // enter scope and gen code
    m->scope_->accept( scg_.get(), ctxt_ );

    /*
     * build epilogue
     */

    if ( m->realOut_.empty() )
        builder.CreateRetVoid();
    else 
    {
        // connect last bb with return bb
        builder.CreateBr(m->returnBB_);
        fct->getBasicBlockList().push_back(m->returnBB_);
        builder.SetInsertPoint(m->returnBB_);

        if (m->main_)
        {
            RetVal* retval = m->sig_.out_[0];
            llvm::Value* value = builder.CreateLoad( 
                    retval->getAddr(ctxt_), retval->cid() );
            builder.CreateRet(value);
        }
        else
        {
            llvm::Value* retStruct = llvm::UndefValue::get(m->retType_);
            for (size_t i = 0; i < m->realOut_.size(); ++i)
            {
                RetVal* retval = m->realOut_[i];
                retStruct = builder.CreateInsertValue(
                        retStruct, 
                        builder.CreateLoad( retval->getAddr(ctxt_), retval->cid() ), 
                        i,
                        "retval");
            }

            builder.CreateRet(retStruct);
        }
    }
}

} // namespace swift
