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
    // get some stuff for easy access
    TypeList&  in = m->sig_. inTypes_;
    TypeList& out = m->sig_.outTypes_;
    Module* module = ctxt_->module_;

    llvm::Module* llvmModule = ctxt_->module_->getLLVMModule();
    llvm::LLVMContext& llvmCtxt = llvmModule->getContext();

    swiftAssert(out.size() <= 1, "TODO");

    // set return type
    const llvm::Type* retType;
    if ( out.size() == 0 )
        retType = llvm::TypeBuilder<void, true>::get(llvmCtxt);
    else
        retType = out[0]->getLLVMType(ctxt_->module_);

    std::vector<const llvm::Type*> params;

    // push hidden 'self' param first if necessary
    if ( dynamic_cast<Method*>(m) )
        params.push_back( ctxt_->class_->llvmType() );

    // now push the rest
    for (size_t i = 0; i < in.size(); ++i)
        params.push_back( in[i]->getLLVMType(module) );

    // build llvm name
    static int counter = 0;
    std::ostringstream oss;
    oss << *m->id_ << counter++;
    m->llvmName_ = oss.str();

    // create llvm function
    const llvm::FunctionType* fctType = llvm::FunctionType::get(retType, params, false);
    llvm::Function* fct = llvm::cast<llvm::Function>(
        llvmModule->getOrInsertFunction(llvm::StringRef(m->llvmName_), fctType) );
    ctxt_->llvmFct_ = fct;
    m->llvmFct_ = fct;

    // create root BB
    llvm::BasicBlock* bb = llvm::BasicBlock::Create(
            ctxt_->module_->getLLVMModule()->getContext(),
            m->llvmName_,
            m->llvmFct_);
    ctxt_->builder_.SetInsertPoint(bb);

    // set names for all args
    size_t i = 0; 
    typedef llvm::Function::arg_iterator ArgIter;
    for (ArgIter iter = fct->arg_begin(); i < m->sig_.in_.size(); ++iter, ++i)
    {
        InOut* io = m->sig_.in_[i];

        io->createEntryAlloca(ctxt_);
        iter->setName( io->cid() );

        // Store the initial value into the alloca.
        ctxt_->builder_.CreateStore( iter, io->getAlloca() );
    }

    // enter scope and gen code
    m->scope_->accept( scg_.get(), ctxt_ );

    // TODO void
    ctxt_->builder_.CreateRetVoid();
}

} // namespace swift
