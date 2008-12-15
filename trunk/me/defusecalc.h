#ifndef ME_DEF_USE_CALC_H
#define ME_DEF_USE_CALC_H

#include <fstream>

#include "me/codepass.h"
#include "me/functab.h"

namespace me {


class DefUseCalc : public CodePass
{
public:

    /*
     * constructor
     */

    DefUseCalc(Function* function);

    /*
     * methods
     */

    virtual void process();

private:

    /**
     * @brief Compiles for all vars their defining instruction. 
     *
     * The left hand side of \a PhiInstr instances counts as definition, too.
     */
    void calcDef();

    /** 
     * @brief Compiles for all v in vars the instructinos which make use of v.
     *
     * The right hand side of \a PhiInstr instances count as a use, too. 
     */
    void calcUse();

    /** 
     * @brief Used internally by \a CalcUse.
     * 
     * @param var Current var. 
     * @param bb Current basic block.
     */
    void calcUse(Reg* var, BBNode* bb);
};

} // namespace me

#endif // ME_DEF_USE_CALC_H
