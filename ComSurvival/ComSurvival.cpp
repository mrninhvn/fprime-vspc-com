// ======================================================================
// \title  ComSurvival.cpp
// \brief  Survival mode 2 beacon rounds (see ComSurvival.hpp)
// ======================================================================

#include "ComSurvival/ComSurvival.hpp"

#include <Fw/Com/ComPacket.hpp>

namespace Vspc {

ComSurvival::ComSurvival(const char* const compName) : ComSurvivalComponentBase(compName) {}

ComSurvival::~ComSurvival() {}

void ComSurvival::configure(const Config& config) {
    FW_ASSERT(config.nPhases <= COM_SURVIVAL_MAX_PHASES, static_cast<FwAssertArgType>(config.nPhases));
    FW_ASSERT((config.phases != nullptr) || (config.nPhases == 0U));
    FW_ASSERT((config.start != nullptr) || (config.nStart == 0U));
    FW_ASSERT((config.restore != nullptr) || (config.nRestore == 0U));
    this->m_config = config;
}

// ----------------------------------------------------------------------
// Handlers: all but schedIn only record what they got
// ----------------------------------------------------------------------

void ComSurvival::missionIn_handler(FwIndexType portNum, const MissionControllerComponent::MissionControllerMode& mode) {
    static_cast<void>(portNum);
    this->m_survival = (mode == MissionControllerComponent::MissionControllerMode::MissionModeSurvival2);
}

void ComSurvival::cmdResponseIn_handler(FwIndexType portNum,
                                        FwOpcodeType opCode,
                                        U32 cmdSeq,
                                        const Fw::CmdResponse& response) {
    static_cast<void>(portNum);
    // Only the step in flight; a late answer of a step left behind is dropped
    if ((this->m_stepState == WAIT_RESPONSE) && (opCode == this->m_expectOpcode) && (cmdSeq == this->m_context)) {
        this->m_response = response;
        this->m_responseReady = true;
    }
}

void ComSurvival::SURV_ROUND_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    if ((this->m_stage != OFF) || this->m_survival || this->m_bench) {
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::BUSY);
        return;
    }
    this->m_bench = true;
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void ComSurvival::schedIn_handler(FwIndexType portNum, U32 context) {
    static_cast<void>(portNum);
    static_cast<void>(context);

    if (!this->m_bootChecked) {
        this->bootCheck();
    }

    const bool wanted = this->m_survival || this->m_bench;
    if (wanted && (this->m_stage == OFF)) {
        this->begin();
    } else if (!wanted && (this->m_stage != OFF) && !this->m_stopping) {
        this->abort();
    }

    if (this->m_stage == OFF) {
        return;
    }
    if (this->m_stage == WAIT) {
        if (elapsedS(this->m_roundStart) >= this->m_waitS) {
            this->startRound();
        }
        return;
    }
    if (this->stepList()) {
        this->listDone();
    }
}

// ----------------------------------------------------------------------
// Round
// ----------------------------------------------------------------------

void ComSurvival::bootCheck() {
    this->m_bootChecked = true;
    // A phase still marked was cut by a reset: skip it next round, and clear the mark
    U8 marked = 0U;
    if (this->prmGet(this->m_config.phasePrm, marked) && (marked != 0U)) {
        this->m_skipPhase = static_cast<U8>(marked - 1U);
        this->prmSet(this->m_config.phasePrm, static_cast<U8>(0U));
    }
}

void ComSurvival::begin() {
    U32 period = this->m_config.periodDefaultS;
    if (!this->prmGet(this->m_config.periodPrm, period) || (period == 0U)) {
        period = this->m_config.periodDefaultS;
    }
    this->m_periodS = period;
    this->m_count = this->m_config.countDefault;
    static_cast<void>(this->prmGet(this->m_config.countPrm, this->m_count));
    this->m_stopping = false;
    this->m_roundEnded = false;
    this->log_ACTIVITY_HI_SurvRunning(true);
    this->runList(this->m_config.start, this->m_config.nStart, START);
}

void ComSurvival::abort() {
    switch (this->m_stage) {
        case ENTER:
        case BEACON:
            // Undo the phase in progress (the step in flight is left behind)
            this->m_stopping = true;
            this->runList(this->m_config.phases[this->m_phase].exit, this->m_config.phases[this->m_phase].nExit,
                          EXIT);
            break;
        case START:
        case RESTORE:
        case EXIT:
            // The list in progress runs to the end, then restore, then stop
            this->m_stopping = true;
            break;
        default:
            // WAIT: the round already ended with the restore steps
            this->finishStop();
            break;
    }
}

void ComSurvival::finishStop() {
    this->m_stage = OFF;
    this->m_stopping = false;
    this->m_bench = false;
    this->log_ACTIVITY_HI_SurvRunning(false);
}

void ComSurvival::startRound() {
    this->m_roundEnded = false;
    static_cast<void>(this->m_roundStart.now());
    this->prmSet(this->m_config.lastRoundPrm, static_cast<U32>(this->getTime().getSeconds()));
    this->enterPhase(0U);
}

void ComSurvival::enterPhase(U8 index) {
    while ((index < this->m_config.nPhases) && (index == this->m_skipPhase)) {
        this->log_WARNING_HI_SurvPhaseSkipped(index);
        this->m_skipPhase = PHASE_NONE;
        index++;
    }
    if (index >= this->m_config.nPhases) {
        this->roundDone();
        return;
    }
    this->m_phase = index;
    this->prmSet(this->m_config.phasePrm, static_cast<U8>(index + 1U));
    this->log_ACTIVITY_HI_SurvPhase(index);
    this->runList(this->m_config.phases[index].enter, this->m_config.phases[index].nEnter, ENTER);
}

void ComSurvival::beaconOrExit() {
    const Phase& phase = this->m_config.phases[this->m_phase];
    if ((this->m_rep < this->m_count) && (phase.nBeacon > 0U)) {
        this->runList(phase.beacon, phase.nBeacon, BEACON);
    } else {
        this->runList(phase.exit, phase.nExit, EXIT);
    }
}

void ComSurvival::roundDone() {
    this->m_roundEnded = true;
    this->runList(this->m_config.restore, this->m_config.nRestore, RESTORE);
}

void ComSurvival::afterRestore() {
    if (this->m_stopping) {
        this->finishStop();
        return;
    }
    if (!this->m_roundEnded) {
        // After START: the first round
        if (!this->m_survival) {
            this->startRound();  // bench: at once
            return;
        }
        // Not before the period since the last round (kept over a reset) is over
        U32 last = 0U;
        const U32 now = this->getTime().getSeconds();
        if (this->prmGet(this->m_config.lastRoundPrm, last) && (now >= last) && ((now - last) < this->m_periodS)) {
            static_cast<void>(this->m_roundStart.now());
            this->m_waitS = this->m_periodS - (now - last);
            this->m_stage = WAIT;
            this->log_ACTIVITY_HI_SurvRoundDone(this->m_waitS);
        } else {
            this->startRound();
        }
        return;
    }
    if (this->m_survival) {
        // Next round one period after the start of this one
        this->m_bench = false;
        this->m_waitS = this->m_periodS;
        this->m_stage = WAIT;
        const U32 elapsed = elapsedS(this->m_roundStart);
        this->log_ACTIVITY_HI_SurvRoundDone((elapsed < this->m_periodS) ? (this->m_periodS - elapsed) : 0U);
    } else {
        // Bench round
        this->log_ACTIVITY_HI_SurvRoundDone(0U);
        this->finishStop();
    }
}

void ComSurvival::listDone() {
    switch (this->m_stage) {
        case START:
            this->runList(this->m_config.restore, this->m_config.nRestore, RESTORE);
            break;
        case RESTORE:
            this->afterRestore();
            break;
        case ENTER:
            this->m_rep = 0U;
            this->beaconOrExit();
            break;
        case BEACON:
            this->m_rep++;
            this->beaconOrExit();
            break;
        case EXIT:
            this->prmSet(this->m_config.phasePrm, static_cast<U8>(0U));
            if (this->m_stopping) {
                this->runList(this->m_config.restore, this->m_config.nRestore, RESTORE);
            } else {
                this->enterPhase(static_cast<U8>(this->m_phase + 1U));
            }
            break;
        default:
            break;
    }
}

// ----------------------------------------------------------------------
// Step lists
// ----------------------------------------------------------------------

void ComSurvival::runList(const Step* list, U8 count, Stage stage) {
    this->m_list = list;
    this->m_listN = (list != nullptr) ? count : 0U;
    this->m_idx = 0U;
    this->m_stepState = READY;
    this->m_responseReady = false;
    this->m_stage = stage;
}

bool ComSurvival::stepList() {
    for (;;) {
        switch (this->m_stepState) {
            case READY:
                while ((this->m_idx < this->m_listN) && (this->m_list[this->m_idx].opcode == 0U)) {
                    this->m_idx++;
                }
                if (this->m_idx >= this->m_listN) {
                    return true;
                }
                this->m_retries = 0U;
                this->sendStep(this->m_list[this->m_idx]);
                return false;

            case WAIT_RESPONSE: {
                bool failed = false;
                if (this->m_responseReady) {
                    this->m_responseReady = false;
                    failed = (this->m_response != Fw::CmdResponse::OK);
                    if (failed) {
                        this->log_WARNING_HI_SurvCmdFailed(this->m_expectOpcode, this->m_response);
                    }
                } else if (elapsedS(this->m_stepStart) >= COM_SURVIVAL_CMD_TIMEOUT_S) {
                    failed = true;
                    this->log_WARNING_HI_SurvCmdTimeout(this->m_expectOpcode);
                } else {
                    return false;
                }
                // What an exit / restore step leaves undone stays wrong: try it again
                if (failed && ((this->m_stage == EXIT) || (this->m_stage == RESTORE)) &&
                    (this->m_retries < COM_SURVIVAL_RETRIES)) {
                    this->m_retries++;
                    this->sendStep(this->m_list[this->m_idx]);
                    return false;
                }
                this->m_stepState = DELAY;
                static_cast<void>(this->m_stepStart.now());
                break;
            }

            case DELAY:
            default:
                if (elapsedS(this->m_stepStart) < this->m_list[this->m_idx].delayAfterS) {
                    return false;
                }
                this->m_idx++;
                this->m_stepState = READY;
                break;
        }
    }
}

void ComSurvival::sendStep(const Step& step) {
    this->m_context++;
    this->m_expectOpcode = step.opcode;
    this->m_stepState = WAIT_RESPONSE;
    this->m_responseReady = false;
    static_cast<void>(this->m_stepStart.now());

    // Command packet: [descriptor][opcode][arguments], as the command dispatcher reads it
    Fw::ComBuffer cmd;
    Fw::SerializeStatus status =
        cmd.serializeFrom(static_cast<FwPacketDescriptorType>(Fw::ComPacketType::FW_PACKET_COMMAND));
    if (status == Fw::FW_SERIALIZE_OK) {
        status = cmd.serializeFrom(step.opcode);
    }
    if (status == Fw::FW_SERIALIZE_OK) {
        status = cmd.serializeFrom(step.args, static_cast<FwSizeType>(step.argSize), Fw::Serialization::OMIT_LENGTH);
    }
    FW_ASSERT(status == Fw::FW_SERIALIZE_OK, static_cast<FwAssertArgType>(status));

    if (!this->isConnected_cmdOut_OutputPort(0)) {
        this->m_response = Fw::CmdResponse::EXECUTION_ERROR;
        this->m_responseReady = true;
        return;
    }
    // The dispatcher hands the context back as the response's cmdSeq
    this->cmdOut_out(0, cmd, this->m_context);
}

U32 ComSurvival::elapsedS(const Os::RawTime& since) {
    Os::RawTime now;
    Fw::TimeInterval interval;
    if ((now.now() != Os::RawTime::OP_OK) || (now.getTimeInterval(since, interval) != Os::RawTime::OP_OK)) {
        return 0U;
    }
    return interval.getSeconds();
}

// ----------------------------------------------------------------------
// Parameters
// ----------------------------------------------------------------------

template <typename T>
bool ComSurvival::prmGet(FwPrmIdType id, T& value) {
    if (!this->isConnected_prmGetOut_OutputPort(0)) {
        return false;
    }
    Fw::ParamBuffer buffer;
    buffer.resetSer();
    if (this->prmGetOut_out(0, id, buffer) != Fw::ParamValid::VALID) {
        return false;
    }
    T read;
    buffer.resetDeser();
    if (buffer.deserializeTo(read) != Fw::FW_SERIALIZE_OK) {
        return false;
    }
    value = read;
    return true;
}

template <typename T>
void ComSurvival::prmSet(FwPrmIdType id, const T& value) {
    if (!this->isConnected_prmSetOut_OutputPort(0)) {
        return;
    }
    Fw::ParamBuffer buffer;
    buffer.resetSer();
    if (buffer.serializeFrom(value) == Fw::FW_SERIALIZE_OK) {
        this->prmSetOut_out(0, id, buffer);
    }
}

}  // namespace Vspc
