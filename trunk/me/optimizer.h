#ifndef ME_OPTIMIZER_H
#define ME_OPTIMIZER_H

namespace me {

// forward declarations
struct Function;

struct Optimizer
{
    // optimizer flags
    static bool commonSubexprElimination_;
    static bool constantPropagation_;
    static bool deadCodeElimination_;

    Function* function_;

    Optimizer(Function* function)
        : function_(function)
    {}

    void optimize();
};

} // namespace me

#endif // ME_OPTIMIZER_H
