#pragma once

#include "signalflow/patch/patch-spec.h"
#include "signalflow/patch/patch.h"
#include <unordered_map>

namespace signalflow
{
class PatchRegistry
{
public:
    PatchRegistry();
    static PatchRegistry *global();

    PatchRef create(std::string name);
    void add(std::string name, PatchSpecRef patchspec);
    PatchSpecRef get(std::string name);

    std::unordered_map<std::string, PatchSpecRef> patchspecs;
};
}
