#include "fe/node.h"

#include "fe/class.h"
#include "fe/classanalyzer.h"
#include "fe/error.h"
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
{}

Module::~Module()
{
    delete id_;

    for (ClassMap::iterator iter = classes_.begin(); iter != classes_.end(); ++iter)
        delete iter->second;
}

void Module::insert(Context& ctxt, Class* c)
{
    ClassMap::iterator iter = classes_.find( c->id() );


    if (iter != classes_.end())
    {
        errorf(c->loc(), "there is already a class '%s' defined in module '%s'", c->cid(), cid());
        SWIFT_PREV_ERROR(iter->second->loc());

        ctxt.result_ = false;
        return;
    }

    classes_[c->id()] = c;
    ctxt.class_ = c;

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

bool Module::analyze()
{
    Context ctxt;
    ctxt.module_ = this;

    for (ClassMap::iterator iter = classes_.begin(); iter != classes_.end(); ++iter)
    {
        ClassAnalyzer classAnalyzer(ctxt);
        iter->second->accept(&classAnalyzer);
    }

    return ctxt.result_;
}

//------------------------------------------------------------------------------

} // namespace swift
