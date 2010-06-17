#include "fe/llvmfctdeclarer.h"

#include <llvm/Module.h>
#include <llvm/Support/TypeBuilder.h>

#include "utils/cast.h"
#include "utils/llvmhelper.h"

#include "fe/context.h"
#include "fe/class.h"
#include "fe/node.h"
#include "fe/type.h"

namespace swift {

LLVMFctDeclarer::LLVMFctDeclarer(Context* ctxt)
    : ctxt_(ctxt)
{
    typedef Module::ClassMap::const_iterator CIter;
    const Module::ClassMap& classes = ctxt_->module_->classes();

    // for each class
    for (CIter iter = classes.begin(); iter != classes.end(); ++iter)
    {
        Class* c = iter->second;

        // skip builtin types
        if ( ScalarType::isScalar(c->id()) )
            continue;

        // for each member fct
        for (size_t i = 0; i < c->memberFcts().size(); ++i)
        {
            MemberFct* m = c->memberFcts()[i];

            if ( !m->isAutoGenerated() )
                process(c, m);
        }
    }

    llvm::Module* llvmModule = ctxt_->lmodule();
    llvm::LLVMContext& lctxt = llvmModule->getContext();

    /*
     * declare malloc
     */

    {
        const llvm::Type* retType = llvm::PointerType::getInt8PtrTy(lctxt);

        LLVMTypes params(1);
        params[0] = llvm::IntegerType::getInt64Ty(lctxt);

        const llvm::FunctionType* fctType = 
            llvm::FunctionType::get(retType, params, false);

        ctxt_->malloc_ = cast<llvm::Function>(
            llvmModule->getOrInsertFunction("malloc", fctType) );
        ctxt_->malloc_->addAttribute(0, llvm::Attribute::NoAlias);
        ctxt_->malloc_->addAttribute(~0, llvm::Attribute::NoUnwind);
    }

    /*
     * declare memcpy
     */

    {
        const llvm::Type* retType = createVoid(lctxt);
        LLVMTypes params(3);
        params[0] = llvm::PointerType::getInt8PtrTy(lctxt);
        params[1] = llvm::PointerType::getInt8PtrTy(lctxt);
        params[2] = llvm::IntegerType::getInt64Ty(lctxt);

        const llvm::FunctionType* fctType = 
            llvm::FunctionType::get(retType, params, false);

        ctxt_->memcpy_ = cast<llvm::Function>(
            llvmModule->getOrInsertFunction("memcpy", fctType) );
        ctxt_->memcpy_->addAttribute(1, llvm::Attribute::NoCapture);
        ctxt_->memcpy_->addAttribute(2, llvm::Attribute::NoCapture);
        ctxt_->memcpy_->addAttribute(~0, llvm::Attribute::NoUnwind);
    }
}

void LLVMFctDeclarer::process(Class* c, MemberFct* m)
{
    /*
     * get some stuff for easy access
     */

    TypeList&  in = m->sig_. inTypes_;
    TypeList& out = m->sig_.outTypes_;
    Module* module = ctxt_->module_;
    llvm::Module* llvmModule = ctxt_->lmodule();
    llvm::LLVMContext& lctxt = llvmModule->getContext();

    /*
     * is this the entry point?
     */

    if ( *m->id() == "main" 
            && m->sig_.in_.empty() 
            && !m->sig_.out_.empty() 
            && m->sig_.out_[0]->getType()->isInt()
            && dynamic_cast<Routine*>(m) )
    {
        m->main_ = true;
    }


    /*
     * create llvm function type
     */

    // push hidden 'self' param first if necessary
    if ( dynamic_cast<Method*>(m) )
        m->params_.push_back( llvm::PointerType::getUnqual( c->getLLVMType()) );

    // build return type
    if (m->main_)
    {
        m->retType_ = llvm::IntegerType::getInt32Ty(lctxt);
        m->realOut_.push_back( m->sig_.out_[0] );
    }
    else if ( out.empty() )
        m->retType_ = createVoid(lctxt);
    else
    {
        LLVMTypes retTypes;
        for (size_t i = 0; i < out.size(); ++i)
        {
            RetVal* retval = m->sig_.out_[i];
            const llvm::Type* llvmType = retval->getType()->getLLVMType(module);

            if ( retval->getType()->perRef() )
            {
                m->params_.push_back(llvmType);
                m->realIn_.push_back(retval);
            }
            else
            {
                retTypes.push_back(llvmType);
                m->realOut_.push_back(retval);
            }
        }

        if ( m->realOut_.empty() )
            m->retType_ = createVoid(lctxt);
        else
            m->retType_ = llvm::StructType::get(lctxt, retTypes);
    }

    // now push the rest
    for (size_t i = 0; i < in.size(); ++i)
    {
        InOut* io = m->sig_.in_[i];
        m->params_.push_back(io->getType()->getLLVMType(module));
        m->realIn_.push_back(io);
    }

    const llvm::FunctionType* fctType = llvm::FunctionType::get(
            m->retType_, m->params_, false);

    /*
     * create function
     */

    // create llvm name
    if (m->main_)
        m->setLLVMName("main");
    else
    {
        static int counter = 0;
        std::ostringstream oss;
        oss << c->cid() << '.' << m->cid() << counter++;
        m->setLLVMName( oss.str() );
    }

    llvm::Function* fct = cast<llvm::Function>( 
            llvmModule->getOrInsertFunction(m->getLLVMName().c_str(), fctType) );

    ctxt_->llvmFct_ = fct;
    m->llvmFct_     = fct;

    // set calling convention
    if (!m->main_)
        fct->setCallingConv(llvm::CallingConv::Fast);
    else
        fct->addFnAttr(llvm::Attribute::NoUnwind);
}

} // namespace swift
