// ======================================================================
// \title  TcVcidSplitter.hpp
// \brief  Diverts the TC frames of one virtual channel out of the local
//         deframe chain (see TcVcidSplitter.fpp)
// ======================================================================

#ifndef Vspc_TcVcidSplitter_HPP
#define Vspc_TcVcidSplitter_HPP

#include "TcVcidSplitter/TcVcidSplitterComponentAc.hpp"

namespace Vspc {

class TcVcidSplitter final : public TcVcidSplitterComponentBase {
  public:
    TcVcidSplitter(const char* const compName);
    ~TcVcidSplitter();

    //! Frames of \p vcId go on vcOut, every other frame on dataOut
    void configure(U8 vcId);

  private:
    void dataIn_handler(FwIndexType portNum, Fw::Buffer& data, const ComCfg::FrameContext& context) override;

    //! Outside the 6-bit VCID range: nothing is diverted until configure()
    static constexpr U8 VCID_NONE = 0xFF;

    //! TC primary header octet holding the VCID in its upper 6 bits
    static constexpr FwSizeType VCID_OCTET = 2;
    static constexpr U8 VCID_SHIFT = 2;

    U8 m_vcId = VCID_NONE;
};

}  // namespace Vspc

#endif
