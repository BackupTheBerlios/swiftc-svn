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

    // get some stuff for easy access
    TypeList&  in = m->sig_. inTypes_;
    TypeList& out = m->sig_.outTypes_;
    Module* module = ctxt_->module_;
    llvm::Module* llvmModule = ctxt_->module_->getLLVMModule();
    llvm::LLVMContext& llvmCtxt = llvmModule->getContext();
    llvm::IRBuilder<>& builder = ctxt_->builder_;

    /*
     * build return type
     */

    if ( out.empty() )
        m->retType_ = llvm::TypeBuilder<void, true>::get(llvmCtxt);
    else
    {
        std::vector<const llvm::Type*> types;
        for (size_t i = 0; i < out.size(); ++i)
            types.push_back( out[i]->getLLVMType(module) );

        m->retType_ = llvm::StructType::get(llvmCtxt, types);
    }

    /*
     * build parameter
     */

    std::vector<const llvm::Type*> params;

    // push hidden 'self' param first if necessary
    if ( dynamic_cast<Method*>(m) )
        params.push_back( ctxt_->class_->llvmType() );

    // now push the rest
    for (size_t i = 0; i < in.size(); ++i)
        params.push_back( in[i]->getLLVMType(module) );

    /*
     * build llvm name
     */

    static int counter = 0;
    std::ostringstream oss;
    oss << ctxt_->class_->cid() << '.' << *m->id_ << counter++;
    m->llvmName_ = oss.str();

    /*
     * create function
     */

    const llvm::FunctionType* fctType = llvm::FunctionType::get(
            m->retType_, params, false);

    llvm::Function* fct = llvm::cast<llvm::Function>(
        llvmModule->getOrInsertFunction(llvm::StringRef(m->llvmName_), fctType) );

    ctxt_->llvmFct_ = fct;
    m->llvmFct_     = fct;

    /*
     * create root BB and connect to fct and to the builder
     */

    llvm::BasicBlock* bb = llvm::BasicBlock::Create(
            ctxt_->module_->getLLVMModule()->getContext(), m->llvmName_, fct);

    builder.SetInsertPoint(bb);

    /*
     * initialize return value
     */

    if ( !out.empty() )
    {
        m->retAlloca_ = builder.CreateAlloca(m->retType_, 0, "retval" );
        for (size_t i = 0; i < out.size(); ++i)
        {
            RetVal* retval = m->sig_.out_[i];
            retval->setAlloca(m->retAlloca_, i);
        }
    }

    /*
     * initialize params
     */

    size_t i = 0; 
    typedef llvm::Function::arg_iterator ArgIter;
    for (ArgIter iter = fct->arg_begin(); i < m->sig_.in_.size(); ++iter, ++i)
    {
        Param* param = m->sig_.in_[i];

        param->createEntryAlloca(ctxt_);
        iter->setName( param->cid() );

        // Store the initial value into the alloca.
        builder.CreateStore( iter, param->getAddr(ctxt_) );
    }

    // enter scope and gen code
    m->scope_->accept( scg_.get(), ctxt_ );

    if ( out.empty() )
        builder.CreateRetVoid();
    else
        builder.CreateRet( builder.CreateLoad(m->retAlloca_) );
}

} // namespace swift
