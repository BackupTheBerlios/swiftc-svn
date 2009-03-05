#include "fe/scope.h"

#include "fe/error.h"
#include "fe/signature.h"
#include "fe/var.h"

namespace swift {

/*
 * constructor and destructor
 */

Scope::Scope(Scope* parent)
    : parent_(parent)
{}

Scope::~Scope()
{
    // delete each child scope
    for (size_t i = 0; i < childScopes_.size(); ++i)
        delete childScopes_[i];
}

/*
 * further methods
 */

Local* Scope::lookupLocal(const std::string* id)
{
    LocalMap::const_iterator iter = locals_.find(id);
    if ( iter != locals_.end() )
        return iter->second;
    else
    {
        // try to find in parent scope
        if (parent_)
            return parent_->lookupLocal(id);
        else
            return 0;
    }
}

bool Scope::insert(Local* local, const Signature* sig)
{
    std::pair<LocalMap::iterator, bool> p 
        = locals_.insert( std::make_pair(local->id_, local) );

    if ( !p.second )
    {
        errorf(local->line_, "there is already a local '%s' defined in this scope in line %i",
            local->id_->c_str(), p.first->second->line_);

        return false;
    }

    const Param* found = sig->findParam(local->id_);
    if (found)
    {
        errorf(local->line_, "local '%s' shadows a parameter", local->id_->c_str());
        return false;
    }

    return true;
}

} // namespace swift
