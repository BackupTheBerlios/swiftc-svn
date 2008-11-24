#include "me/codepass.h"

#include "me/functab.h"

namespace me {

CodePass::CodePass(Function* function)
    : function_(function)
    , cfg_( &function->cfg_ )
{}

} // namespace me
