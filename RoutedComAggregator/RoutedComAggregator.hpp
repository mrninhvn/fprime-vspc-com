// ======================================================================
// \title  RoutedComAggregator.hpp
// \author ninhdh4
// \brief  Svc::ComAggregator that never mixes ComQueue queues in one frame
//         (multi-link downlink: frames are routed per queue). Derived from
//         Svc/ComAggregator (lestarch); only the isFull guard differs.
// ======================================================================

#ifndef Vspc_RoutedComAggregator_HPP
#define Vspc_RoutedComAggregator_HPP

#include <atomic>
#include "RoutedComAggregator/RoutedComAggregatorComponentAc.hpp"

namespace Vspc {

class RoutedComAggregator final : public RoutedComAggregatorComponentBase {
  public:
    //! Construct RoutedComAggregator object
    RoutedComAggregator(const char* const compName  //!< The component name
    );

    //! Destroy RoutedComAggregator object
    ~RoutedComAggregator();

    void preamble() override;

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for typed input ports
    // ----------------------------------------------------------------------

    //! Port receiving the general status from the downstream component
    void comStatusIn_handler(FwIndexType portNum, Fw::Success& condition) override;

    //! Port to receive data to frame, in a Fw::Buffer with optional context
    void dataIn_handler(FwIndexType portNum, Fw::Buffer& data, const ComCfg::FrameContext& context) override;

    //! Buffer coming from a deallocate call in a ComDriver component
    void dataReturnIn_handler(FwIndexType portNum, Fw::Buffer& data, const ComCfg::FrameContext& context) override;

    //! Rate-group timeout: send a partially filled frame
    void timeout_handler(FwIndexType portNum, U32 context) override;

  private:
    // ----------------------------------------------------------------------
    // Implementations for internal state machine actions
    // ----------------------------------------------------------------------

    void Svc_AggregationMachine_action_doClear(SmId smId, Svc_AggregationMachine::Signal signal) override;

    void Svc_AggregationMachine_action_doFill(SmId smId,
                                              Svc_AggregationMachine::Signal signal,
                                              const Svc::ComDataContextPair& value) override;

    void Svc_AggregationMachine_action_doSend(SmId smId, Svc_AggregationMachine::Signal signal) override;

    void Svc_AggregationMachine_action_doHold(SmId smId,
                                              Svc_AggregationMachine::Signal signal,
                                              const Svc::ComDataContextPair& value) override;

    void Svc_AggregationMachine_action_assertNoStatus(SmId smId, Svc_AggregationMachine::Signal signal) override;

  private:
    // ----------------------------------------------------------------------
    // Implementations for internal state machine guards
    // ----------------------------------------------------------------------

    //! The incoming buffer cannot join the current frame: it does not fit,
    //! or it comes from a different queue than the frame being filled (the
    //! frame would otherwise be routed by the last packet's queue only).
    bool Svc_AggregationMachine_guard_isFull(SmId smId,
                                             Svc_AggregationMachine::Signal signal,
                                             const Svc::ComDataContextPair& value) const override;

    //! The incoming buffer will exactly fill the aggregation buffer
    bool Svc_AggregationMachine_guard_willFill(SmId smId,
                                               Svc_AggregationMachine::Signal signal,
                                               const Svc::ComDataContextPair& value) const override;

    //! The aggregation buffer is not empty
    bool Svc_AggregationMachine_guard_isNotEmpty(SmId smId, Svc_AggregationMachine::Signal signal) const override;

    //! The last status is good
    bool Svc_AggregationMachine_guard_isGood(SmId smId,
                                             Svc_AggregationMachine::Signal signal,
                                             const Fw::Success& value) const override;

  private:
    U8 m_frameBufferStore[ComCfg::AggregationSize];  //!< Buffer to hold the frame data
    Fw::Buffer::OwnershipState m_bufferState =
        Fw::Buffer::OwnershipState::OWNED;  //!< whether m_frameBuffer is owned by TmFramer
    Fw::Buffer m_frameBuffer;
    Fw::ExternalSerializeBufferWithMemberCopy m_frameSerializer;  //!< Serializer for m_frameBuffer
    ComCfg::FrameContext m_lastContext;                           //!< Context for the current frame

    Svc::ComDataContextPair m_held;     //!< Held data while waiting for send
    std::atomic<bool> m_allow_timeout;  //!< Whether status has been received
};

}  // namespace Vspc

#endif
