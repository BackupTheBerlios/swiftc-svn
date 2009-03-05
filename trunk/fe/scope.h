#ifndef SWIFT_SCOPE
#define SWIFT_SCOPE

#include <map>
#include <string>
#include <vector>

#include "utils/stringhelper.h"

namespace swift {

/*
 * forward declaraions
 */

class Local;
class Signature;

/**
 * @brief This represents the Scope of a Proc. 
 *
 * It can have child scopes and knows of its \a locals_ which are defined there.
 */
class Scope
{
public:

    /*
     * constructor and destructor
     */

    Scope(Scope* parent);
    ~Scope();

    /*
     * further methods
     */

    /// Returns the local by the id, of this scope or parent scopes. 0 if nothing was found.
    Local* lookupLocal(const std::string* id);

    bool insert(Local* local, const Signature* sig);

    /*
     * data
     */

    typedef std::vector<Scope*> Scopes;
    Scopes childScopes_; ///< List of child scopes.

private:

    Scope* parent_;      ///< 0 if root.

    typedef std::map<const std::string*, Local*, StringPtrCmp> LocalMap;
    LocalMap locals_;    ///< Map of locals in this scope sorted by identifier.
};

} // namespace swift

#endif // SWIFT_SCOPE
