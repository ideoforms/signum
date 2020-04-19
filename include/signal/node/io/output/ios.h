#pragma once

#include "signal/core/platform.h"

#if __APPLE__

#if TARGET_OS_IPHONE

#define AudioOut AudioOut_iOS

#include "abstract.h"

#include "signal/core/graph.h"
#include "signal/node/node.h"

namespace libsignal
{

class AudioOut_iOS : public AudioOut_Abstract
{
public:
    AudioOut_iOS(AudioGraph *graph);

    virtual int init() override;
    virtual int start() override;
    virtual int close() override;
};

} // namespace libsignal

#endif

#endif
