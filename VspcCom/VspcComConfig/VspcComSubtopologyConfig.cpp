#include "VspcComSubtopologyConfig.hpp"

namespace VspcCom {
namespace Allocation {
// This instance can be changed to use a different allocator in the VspcCom Subtopology
Fw::MallocAllocator mallocatorInstance;
Fw::MemAllocator& memAllocator = mallocatorInstance;
}  // namespace Allocation
}  // namespace VspcCom
