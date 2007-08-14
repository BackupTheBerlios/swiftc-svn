#ifndef SWIFT_SPILLER_H
#define SWIFT_SPILLER_H

struct Spiller
{
    Spiller() {}
    virtual ~Spiller() {}

    virtual void spill() = 0;
};

#endif // SWIFT_SPILLER_H
