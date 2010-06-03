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
    TypeList&  in = m->sig_. inTypes_;
    TypeList& out = m->sig_.outTypes_;
    Module* module = ctxt_->module_;
    llvm::Module* llvmModule = ctxt_->module_->getLLVMModule();
    llvm::LLVMContext& llvmCtxt = llvmModule->getContext();
    llvm::IRBuilder<>& builder = ctxt_->builder_;

    /*
     * is this the entry point?
     */
    bool main = false;
    if ( *m->id_ == "main" 
            && m->sig_.in_.empty() 
            && !m->sig_.out_.empty() 
            && m->sig_.out_[0]->getType()->isInt()
            && dynamic_cast<Routine*>(m) )
    {
        main = true;
    }


    /*
     * create llvm function type
     */

    std::vector<const llvm::Type*> params;
    std::vector<InOut*> realIn;
    std::vector<RetVal*> realOut;

    // push hidden 'self' param first if necessary
    if ( dynamic_cast<Method*>(m) )
    {
        params.push_back( llvm::PointerType::getUnqual(
                    ctxt_->class_->llvmType()) );
    }

    // build return type
    if ( out.empty() )
        m->retType_ = llvm::TypeBuilder<void, true>::get(llvmCtxt);
    else if (main)
    {
        m->retType_ = llvm::IntegerType::getInt32Ty(llvmCtxt);
        realOut.push_back( m->sig_.out_[0] );
    }
    else
    {
        std::vector<const llvm::Type*> retTypes;
        for (size_t i = 0; i < out.size(); ++i)
        {
            RetVal* retval = m->sig_.out_[i];
            const llvm::Type* llvmType = retval->getType()->getLLVMType(module);

            if ( retval->getType()->isRef() )
            {
                params.push_back(llvmType);
                realIn.push_back(retval);
            }
            else
            {
                retTypes.push_back(llvmType);
                realOut.push_back(retval);
            }
        }

        m->retType_ = llvm::StructType::get(llvmCtxt, retTypes);
        //ctxt_->module_->getLLVMModule()->addTypeName("ret-type", m->retType_);
    }

    // now push the rest
    for (size_t i = 0; i < in.size(); ++i)
    {
        InOut* io = m->sig_.in_[i];
        params.push_back(io->getType()->getLLVMType(module));
        realIn.push_back(io);
    }

    const llvm::FunctionType* fctType = llvm::FunctionType::get(
            m->retType_, params, false);

    /*
     * create function
     */

    // create llvm name
    if (main)
        m->llvmName_ = "main";
    else
    {
        static int counter = 0;
        std::ostringstream oss;
        oss << ctxt_->class_->cid() << '.' << *m->id_ << counter++;
        m->llvmName_ = oss.str();
    }

    llvm::Function* fct = llvm::cast<llvm::Function>(
        llvmModule->getOrInsertFunction(m->llvmName_.c_str(), fctType) );

    ctxt_->llvmFct_ = fct;
    m->llvmFct_     = fct;

    // set calling convention
    if (!main)
        fct->setCallingConv(llvm::CallingConv::Fast);
    else
        fct->addFnAttr(llvm::Attribute::NoUnwind);

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
        method->selfValue_ = builder.CreateAlloca( params[0], 0, "self" );
        builder.CreateStore(iter,  method->selfValue_);

        iter->setName("self");
        //iter->addAttr(llvm::Attribute::InReg);
        ++iter;
    }

    // initialize params
    for (size_t i = 0; i < realIn.size(); ++iter, ++i)
    {
        InOut* io = realIn[i];

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

    if ( realOut.empty() )
        builder.CreateRetVoid();
    else 
    {
        // connect last bb with return bb
        builder.CreateBr(m->returnBB_);
        fct->getBasicBlockList().push_back(m->returnBB_);
        builder.SetInsertPoint(m->returnBB_);

        if (main)
        {
            RetVal* retval = m->sig_.out_[0];
            llvm::Value* value = builder.CreateLoad( 
                    retval->getAddr(ctxt_), retval->cid() );
            builder.CreateRet(value);
        }
        else
        {
            llvm::Value* retStruct = llvm::UndefValue::get(m->retType_);
            for (size_t i = 0; i < realOut.size(); ++i)
            {
                RetVal* retval = realOut[i];
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
