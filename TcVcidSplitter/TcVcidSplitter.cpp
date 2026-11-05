// ======================================================================
// \title  TcVcidSplitter.cpp
// \brief  Diverts the TC frames of one virtual channel out of the local
//         deframe chain (see TcVcidSplitter.fpp)
// ======================================================================

#include "TcVcidSplitter/TcVcidSplitter.hpp"

namespace Vspc {

TcVcidSplitter ::TcVcidSplitter(const char* const compName) : TcVcidSplitterComponentBase(compName) {}

TcVcidSplitter ::~TcVcidSplitter() {}

void TcVcidSplitter ::configure(U8 vcId) {
    this->m_vcId = vcId;
}

void TcVcidSplitter ::dataIn_handler(FwIndexType portNum, Fw::Buffer& data, const ComCfg::FrameContext& context) {
    // TC primary header: [ver/flags/SCID (2)] [VCID (6 bits) | frame length (10 bits)] ...
    if (data.isValid() && (data.getSize() > VCID_OCTET) && this->isConnected_vcOut_OutputPort(0) &&
        (static_cast<U8>(data.getData()[VCID_OCTET] >> VCID_SHIFT) == this->m_vcId)) {
        this->vcOut_out(0, data);
        return;
    }
    this->dataOut_out(0, data, context);
}

}  // namespace Vspc
