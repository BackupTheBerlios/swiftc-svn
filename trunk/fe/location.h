#ifndef SWIFT_LOCATION_H
#define SWIFT_LOCATION_H

#include <string>

namespace swift {

struct Position
{
    std::string* filename_;
    int line_;

    Position() 
        : filename_(0)
        , line_(-1)
    {}

    Position(std::string* filename, int line)
        : filename_(filename)
        , line_(line)
    {}
};

struct Location
{
    Position begin_;
    Position end_;

    Location() {}

    Location(const Position& begin, const Position& end)
        : begin_(begin)
        , end_(end)
    {}

    Location(const Position& pos)
        : begin_(pos)
        , end_(pos)
    {}

    Location(std::string* filename, int line)
        : begin_( Position(filename, line) )
        , end_( Position(begin_) )
    {}
};

} // namespace swift

#endif // SWIFT_LOCATION_H
