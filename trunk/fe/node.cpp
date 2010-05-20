#include "fe/node.h"

#include <llvm/LLVMContext.h>
#include <llvm/Module.h>

#include "fe/class.h"
#include "fe/classanalyzer.h"
#include "fe/classcodegen.h"
#include "fe/context.h"
#include "fe/error.h"
#include "fe/llvmtypebuilder.h"
#include "fe/typenode.h"

namespace swift {

//------------------------------------------------------------------------------

Node::Node(location loc)
    : loc_(loc)
{}

const location& Node::loc() const
{
    return loc_;
}

//------------------------------------------------------------------------------

Def::Def(location loc, std::string* id)
    : Node(loc)
    , id_(id)
{}

Def::~Def()
{
    delete id_;
}

//------------------------------------------------------------------------------

Module::Module(location loc, std::string* id)
    : Node(loc)
    , id_(id)
    , llvmCtxt_( new llvm::LLVMContext() )
    , llvmModule_( new llvm::Module( llvm::StringRef("default"), *llvmCtxt_) )
{}

Module::~Module()
{
    delete id_;

    for (ClassMap::iterator iter = classes_.begin(); iter != classes_.end(); ++iter)
        delete iter->second;

    delete llvmModule_;
    delete llvmCtxt_;
}

void Module::insert(Context* ctxt, Class* c)
{
    ClassMap::iterator iter = classes_.find( c->id() );


    if (iter != classes_.end())
    {
        errorf(c->loc(), "there is already a class '%s' defined in module '%s'", c->cid(), cid());
        SWIFT_PREV_ERROR(iter->second->loc());

        ctxt->result_ = false;
        return;
    }

    classes_[c->id()] = c;
    ctxt->class_ = c;

    return;
}

Class* Module::lookupClass(const std::string* id)
{
    ClassMap::iterator iter = classes_.find(id);

    if (iter == classes_.end())
        return 0;

    return iter->second;
}

const std::string* Module::id() const
{
    return id_;
}

const char* Module::cid() const
{
    return id_->c_str();
}

bool Module::analyze(Context* ctxt)
{
    ctxt->module_ = this;

    for (ClassMap::iterator iter = classes_.begin(); iter != classes_.end(); ++iter)
    {
        ClassAnalyzer classAnalyzer(ctxt);
        iter->second->accept(&classAnalyzer);
    }

    return ctxt->result_;
}

bool Module::buildLLVMTypes()
{
    LLVMTypebuilder llvmTypeBuilder(this);
    return llvmTypeBuilder.getResult();
}

void Module::codeGen(Context* ctxt)
{
    ctxt->module_ = this;

    for (ClassMap::iterator iter = classes_.begin(); iter != classes_.end(); ++iter)
    {
        ClassCodeGen classCodeGen(ctxt);
        iter->second->accept(&classCodeGen);
    }
}

llvm::Module* Module::getLLVMModule() const
{
    return llvmModule_;
}

const Module::ClassMap& Module::classes() const
{
    return classes_;
}

//------------------------------------------------------------------------------

} // namespace swift
