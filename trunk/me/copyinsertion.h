#ifndef ME_COPY_INSERTION_H
#define ME_COPY_INSERTION_H

#include "me/codepass.h"
#include "me/functab.h"

namespace me {

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
};

} // namespace me

#endif // ME_COPY_INSERTION_H


