#pragma once

#include "signal/core/constants.h"
#include "signal/node/node.h"

namespace libsignal
{

class Subtract : public BinaryOpNode
{

public:
    Subtract(NodeRef a = 0, NodeRef b = 0);

    virtual void process(sample **out, int num_frames);
};

REGISTER(Subtract, "subtract");

}
