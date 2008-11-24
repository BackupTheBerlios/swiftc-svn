#ifndef ME_CODE_PASS_H
#define ME_CODE_PASS_H

namespace me {

// forward declarations
class Function;
class CFG;

class CodePass
{
protected:

    Function* function_;
    CFG* cfg_;

public:

    /*
     * constructor
     */

    CodePass(Function* function);
    virtual ~CodePass() {}

    /*
     * methods
     */

    virtual void process() = 0;
};

} // namespace me

#endif // ME_CODE_PASS_H
