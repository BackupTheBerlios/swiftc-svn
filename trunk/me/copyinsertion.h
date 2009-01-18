#ifndef ME_COPY_INSERTION_H
#define ME_COPY_INSERTION_H

#include "me/codepass.h"
#include "me/functab.h"

namespace me {

/** 
 * @brief Insert copies of regs if necessary.
 *
 * This pass ensures that a live-through arg which is constrained to the same
 * color as a result is copied.
 */
class CopyInsertion : public CodePass
{
private:

    RDUMap phis_;

public:

    /*
     * constructor and destructor
     */

    CopyInsertion(Function* function);

    /*
     * further methods
     */

    virtual void process();

private:

    void insertIfNecessary(InstrNode* instrNode);

    void insertCopy(size_t regIdx, InstrNode* instrNode);
};

} // namespace me

#endif // ME_COPY_INSERTION_H


