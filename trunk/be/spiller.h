#ifndef BE_SPILLER_H
#define BE_SPILLER_H

namespace be {

struct Spiller
{
    Spiller() {}
    virtual ~Spiller() {}

    virtual void spill() = 0;
};

} // namespace be

#endif // BE_SPILLER_H
