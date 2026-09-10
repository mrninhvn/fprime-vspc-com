#ifndef VSPCCOMSUBTOPOLOGY_DEFS_HPP
#define VSPCCOMSUBTOPOLOGY_DEFS_HPP

#include <Fw/Types/MallocAllocator.hpp>
#include <Svc/BufferManager/BufferManager.hpp>
#include <Svc/FrameAccumulator/FrameDetector/CcsdsTcFrameDetector.hpp>
#include "VspcComConfig/VspcComSubtopologyConfig.hpp"
#include "VspcCom/VspcComConfig/FppConstantsAc.hpp"

namespace VspcCom {
struct SubtopologyState {
    // Empty - no external state needed for VspcCom subtopology
};

struct TopologyState {
    SubtopologyState comCcsds;
};
}  // namespace VspcCom

#endif
