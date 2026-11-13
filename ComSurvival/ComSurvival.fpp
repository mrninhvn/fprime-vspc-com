module Vspc {

    @ Survival mode 2 beacon rounds. While the mission mode is Survival2, every
    @ period it goes round the COMs of the deployment, one phase each: the enter
    @ commands (power on / tune / route the beacon to the COM), the beacon
    @ commands repeated a number of times, then the exit commands (route back /
    @ retune / power off); after each round, the restore commands put every COM
    @ back in its boot setup (what a failed exit command left). Every command is
    @ an existing one sent through CdhCore.cmdDisp; the deployment gives the tables (configure), so the same
    @ component serves the VSPC OBC and the AT91. The project must define
    @ MissionControllerComponent.MissionControllerOutPort with MissionModeSurvival2.
    @ Passive: every handler but schedIn only records what it got; schedIn does
    @ the work.
    passive component ComSurvival {

        @ Tick, any rate (times are measured, not counted)
        guarded input port schedIn: Svc.Sched

        @ Mission mode broadcast (MissionController.missionOut)
        guarded input port missionIn: MissionControllerComponent.MissionControllerOutPort

        @ Command packets to CdhCore.cmdDisp.seqCmdBuff
        output port cmdOut: Fw.Com

        @ Their responses (CdhCore.cmdDisp.seqCmdStatus)
        guarded input port cmdResponseIn: Fw.CmdResponse

        @ Period / beacon count (config) and last round / running phase (state)
        output port prmGetOut: Fw.PrmGet
        output port prmSetOut: Fw.PrmSet

        @ Bench: run one round now (START steps first), then stop. Refused while a round
        @ or Survival2 is running.
        guarded command SURV_ROUND opcode 0x00

        @ Survival2 rounds started or stopped
        event SurvRunning(running: bool) \
            severity activity high id 0 format "Survival beacon rounds running: {}"

        @ A phase of the round started
        event SurvPhase(com_phase: U8) \
            severity activity high id 1 format "Survival round: phase {}"

        @ A phase was skipped: the OBC was reset during it last time
        event SurvPhaseSkipped(com_phase: U8) \
            severity warning high id 2 format "Survival round: phase {} skipped, reset during it last time"

        @ The round ended; the next one starts after next_s seconds (0 = none, bench round)
        event SurvRoundDone(next_s: U32) \
            severity activity high id 3 format "Survival round done, next in {} s"

        @ A step command was not answered OK (an exit / restore one is tried again, then the round goes on)
        event SurvCmdFailed(op_code: FwOpcodeType, response: Fw.CmdResponse) \
            severity warning high id 4 format "Survival step opcode 0x{x}: {}"

        @ A step command got no response in time (an exit / restore one is tried again, then the round goes on)
        event SurvCmdTimeout(op_code: FwOpcodeType) \
            severity warning high id 5 format "Survival step opcode 0x{x}: no response"

        @ Port for requesting the current time
        time get port timeCaller

        import Fw.Command
        import Fw.Event
    }
}
