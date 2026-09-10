module VspcComConfig {
    # Base ID for the VspcCom Subtopology; all components are offsets from this
    # base ID. Distinct from the framework ComCcsds subtopology (0x02000000) so
    # the two can coexist in the FPP model.
    constant BASE_ID = 0x0A000000

    module QueueSizes {
        constant comQueue    = 10
        constant aggregator  = 5
    }

    module StackSizes {
        constant comQueue   = 4 * 1024 # Must match prj.conf thread stack size
        constant aggregator = 4 * 1024 # Must match prj.conf thread stack size
    }

    module Priorities {
        constant comQueue   = 5
        constant aggregator = 4
    }

    # Queue configuration constants
    module QueueDepths {
        constant events      = 20
        constant tlm         = 20
        constant file        = 1
    }

    module QueuePriorities {
        constant events      = 0
        constant tlm         = 2
        constant file        = 1
    }

    # Buffer management constants
    module BuffMgr {
        constant frameAccumulatorSize  = 2 * ComCfg.TmFrameFixedSize
        constant commsBuffSize         = 5 + ComCfg.TmFrameFixedSize
        # Large bin: must hold the biggest uplink allocation. Radio RX forwards
        # a packet-sized buffer (LR2021 FLRC/FSK max payload = 511); sized to
        # 512 so any radio packet fits.
        constant commsFileBuffSize     = 512
        constant commsBuffCount        = 64
        constant commsFileBuffCount    = 64
        constant commsBuffMgrId        = 65
    }
}
