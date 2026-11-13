// ======================================================================
// \title  ComSurvival.hpp
// \brief  Survival mode 2 beacon rounds (see ComSurvival.fpp)
//
//   OFF --(Survival2 / SURV_ROUND)--> START steps --> RESTORE steps
//       --> ROUND: for each phase: enter steps, beacon steps x count, exit steps
//       --> RESTORE steps --> WAIT until the period (from the round start) is over --> ROUND ...
//
// Steps go one at a time: the next one is sent once the previous one is
// answered (or timed out, COM_SURVIVAL_CMD_TIMEOUT_S) and its delayAfterS is
// over. A failed or unanswered step is reported and the round goes on; an exit
// or restore step is tried again first (COM_SURVIVAL_RETRIES). The restore
// steps put every COM back in its boot setup and can run any number of times,
// so whatever an exit step failed to undo is undone at the end of the round.
// Leaving Survival2 runs the exit steps of the phase in progress and the
// restore steps, then stops.
//
// Kept over a reset (state parameters, ids from configure): the start time of
// the last round (a reset does not start a round before the period is over)
// and the phase in progress (set before its enter steps, cleared after its
// exit steps). A phase still marked at boot was cut by a reset: it is skipped
// in the next round, so a COM that resets the OBC cannot loop it.
// ======================================================================

#ifndef Vspc_ComSurvival_HPP
#define Vspc_ComSurvival_HPP

#include <Fw/Types/Assert.hpp>
#include <Fw/Types/Serializable.hpp>
#include <Os/RawTime.hpp>

#include "ComSurvival/ComSurvivalCfg.hpp"
#include "ComSurvival/ComSurvivalComponentAc.hpp"

namespace Vspc {

class ComSurvival final : public ComSurvivalComponentBase {
  public:
    //! One command of a step list
    struct Step {
        FwOpcodeType opcode;            //!< Global opcode; 0 = skipped (this deployment has no such command)
        U8 args[COM_SURVIVAL_ARG_MAX];  //!< Serialised arguments, as the command dispatcher reads them
        U8 argSize;                     //!< Bytes used in args
        U16 delayAfterS;                //!< Wait after the response (or the timeout) before the next step
    };

    //! One COM of the round
    struct Phase {
        const Step* enter;  //!< Power on / tune / route the beacon to the COM
        U8 nEnter;
        const Step* beacon;  //!< Repeated "count" times
        U8 nBeacon;
        const Step* exit;  //!< Route back / retune / power off
        U8 nExit;
    };

    struct Config {
        const Phase* phases;  //!< At most COM_SURVIVAL_MAX_PHASES; the tables must outlive the component
        U8 nPhases;
        const Step* start;  //!< Once each time the rounds start (Survival2 entered, boot in Survival2)
        U8 nStart;
        const Step* restore;  //!< Every COM to its boot setup: after START, after each round, when stopping
        U8 nRestore;
        FwPrmIdType periodPrm;  //!< [U32 s] round period, from one round start to the next
        U32 periodDefaultS;     //!< When periodPrm is not stored (or 0)
        FwPrmIdType countPrm;   //!< [U8] beacon steps per phase
        U8 countDefault;        //!< When countPrm is not stored
        FwPrmIdType lastRoundPrm;  //!< [U32 s, system time] state: start of the last round
        FwPrmIdType phasePrm;      //!< [U8 index + 1, 0 = none] state: phase in progress
    };

    //! A step whose arguments are \p args serialised in order (use the autocoded types: enums, Fw.On, ...)
    template <typename... A>
    static Step makeStep(FwOpcodeType opcode, U16 delayAfterS, const A&... args) {
        Step step = {};
        step.opcode = opcode;
        step.delayAfterS = delayAfterS;
        Fw::ExternalSerializeBuffer buffer(step.args, sizeof(step.args));
        Fw::SerializeStatus status = Fw::FW_SERIALIZE_OK;
        const int expand[] = {
            0, (status = (status == Fw::FW_SERIALIZE_OK) ? buffer.serializeFrom(args) : status, 0)...};
        static_cast<void>(expand);
        FW_ASSERT(status == Fw::FW_SERIALIZE_OK, static_cast<FwAssertArgType>(status));
        step.argSize = static_cast<U8>(buffer.getSize());
        return step;
    }

    //! A phase from three step tables, their sizes counted from the arrays
    template <FwSizeType NE, FwSizeType NB, FwSizeType NX>
    static Phase makePhase(const Step (&enter)[NE], const Step (&beacon)[NB], const Step (&exit)[NX]) {
        static_assert((NE <= 255U) && (NB <= 255U) && (NX <= 255U), "at most 255 steps per list");
        return {enter, static_cast<U8>(NE), beacon, static_cast<U8>(NB), exit, static_cast<U8>(NX)};
    }

    ComSurvival(const char* const compName);
    ~ComSurvival();

    void configure(const Config& config);

  private:
    void schedIn_handler(FwIndexType portNum, U32 context) override;
    void missionIn_handler(FwIndexType portNum, const MissionControllerComponent::MissionControllerMode& mode) override;
    void cmdResponseIn_handler(FwIndexType portNum,
                               FwOpcodeType opCode,
                               U32 cmdSeq,
                               const Fw::CmdResponse& response) override;
    void SURV_ROUND_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) override;

    enum Stage : U8 { OFF, START, RESTORE, ENTER, BEACON, EXIT, WAIT };
    enum StepState : U8 { READY, WAIT_RESPONSE, DELAY };
    static constexpr U8 PHASE_NONE = 0xFF;

    void bootCheck();
    void begin();
    void abort();
    void finishStop();
    void afterRestore();
    void startRound();
    void enterPhase(U8 index);
    void beaconOrExit();
    void roundDone();
    void listDone();
    void runList(const Step* list, U8 count, Stage stage);
    bool stepList();
    void sendStep(const Step& step);
    static U32 elapsedS(const Os::RawTime& since);

    template <typename T>
    bool prmGet(FwPrmIdType id, T& value);
    template <typename T>
    void prmSet(FwPrmIdType id, const T& value);

    Config m_config = {};

    // Inputs, recorded by the handlers
    bool m_survival = false;  //!< Last mode broadcast was Survival2
    bool m_bench = false;     //!< SURV_ROUND requested / running
    bool m_responseReady = false;
    Fw::CmdResponse m_response = Fw::CmdResponse::OK;

    // Round
    Stage m_stage = OFF;
    bool m_stopping = false;  //!< Exit steps of the phase cut by leaving Survival2, restore, then OFF
    bool m_roundEnded = false;  //!< The restore steps in progress follow a round (else they follow START)
    bool m_bootChecked = false;
    U8 m_phase = 0;
    U8 m_skipPhase = PHASE_NONE;
    U8 m_count = 0;
    U8 m_rep = 0;
    U32 m_periodS = 0;
    U32 m_waitS = 0;
    Os::RawTime m_roundStart;

    // Step list in progress
    const Step* m_list = nullptr;
    U8 m_listN = 0;
    U8 m_idx = 0;
    StepState m_stepState = READY;
    Os::RawTime m_stepStart;
    FwOpcodeType m_expectOpcode = 0;
    U8 m_retries = 0;  //!< Tries of the step in flight beyond the first
    U32 m_context = 0;
};

}  // namespace Vspc

#endif
