#ifndef BE_SPILLER_H
#define BE_SPILLER_H

// forward declarations
namespace me {
    struct Function;
    struct CFG;
}

namespace be {

struct CodeGenerator_;

//------------------------------------------------------------------------------

struct Spiller
{

    me::Function* function_;
    me::CFG*      cfg_;

/*
    constructor and destructor
*/
    Spiller();
    virtual ~Spiller() {}

/*
    further methods
*/

    void setFunction(me::Function* function);
    virtual void spill() = 0;
};

} // namespace be

#endif // BE_SPILLER_H
