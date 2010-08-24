#ifndef SWIFT_FCT_VECTORIZER_H
#define SWIFT_FCT_VECTORIZER_H

namespace Packetizer {
    class Packetizer;
}

namespace swift {

class Class;
class Context;
class MemberFct;

class FctVectorizer
{
public:

    FctVectorizer(Context* ctxt);

private:

    void process(Class* c, MemberFct* m);

    Context* ctxt_;
    Packetizer::Packetizer* packetizer_;
};

} // namespace swift

#endif // SWIFT_FCT_VECTORIZER_H
