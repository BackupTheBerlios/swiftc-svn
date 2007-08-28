#ifndef SWIFT_OPTIMIZER_H
#define SWIFT_OPTIMIZER_H

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

#endif // SWIFT_OPTIMIZER_H
