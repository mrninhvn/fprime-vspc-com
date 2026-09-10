module VspcCom {

    # ComPacket Queue enum for queue types
    enum Ports_ComPacketQueue : U8 {
        EVENTS,
        TELEMETRY 
    }

    enum Ports_ComBufferQueue : U8 {
        FILE
    }

    # ----------------------------------------------------------------------
    # Active Components
    # ----------------------------------------------------------------------
    instance comQueue: Svc.ComQueue base id VspcComConfig.BASE_ID + 0x00000 \
        queue size VspcComConfig.QueueSizes.comQueue \
        stack size VspcComConfig.StackSizes.comQueue \
        priority VspcComConfig.Priorities.comQueue \
    {
        phase Fpp.ToCpp.Phases.configComponents """
        using namespace VspcCom;
        Svc::ComQueue::QueueConfigurationTable configurationTable;

        // Events (highest-priority)
        configurationTable.entries[Ports_ComPacketQueue::EVENTS].depth = VspcComConfig::QueueDepths::events;
        configurationTable.entries[Ports_ComPacketQueue::EVENTS].priority = VspcComConfig::QueuePriorities::events;

        // Telemetry
        configurationTable.entries[Ports_ComPacketQueue::TELEMETRY].depth = VspcComConfig::QueueDepths::tlm;
        configurationTable.entries[Ports_ComPacketQueue::TELEMETRY].priority = VspcComConfig::QueuePriorities::tlm;

        // File Downlink Queue (buffer queue using NUM_CONSTANTS offset)
        configurationTable.entries[Ports_ComPacketQueue::NUM_CONSTANTS + Ports_ComBufferQueue::FILE].depth = VspcComConfig::QueueDepths::file;
        configurationTable.entries[Ports_ComPacketQueue::NUM_CONSTANTS + Ports_ComBufferQueue::FILE].priority = VspcComConfig::QueuePriorities::file;

        // Allocation identifier is 0 as the MallocAllocator discards it
        VspcCom::comQueue.configure(configurationTable, 0, VspcCom::Allocation::memAllocator);
        """
        phase Fpp.ToCpp.Phases.tearDownComponents """
        VspcCom::comQueue.cleanup();
        """
    }

    # ----------------------------------------------------------------------
    # Passive Components
    # ----------------------------------------------------------------------
    instance frameAccumulator: Svc.FrameAccumulator base id VspcComConfig.BASE_ID + 0x01000 \ 
    {

        phase Fpp.ToCpp.Phases.configObjects """
        Svc::FrameDetectors::CcsdsTcFrameDetector frameDetector;
        """
        phase Fpp.ToCpp.Phases.configComponents """
        VspcCom::frameAccumulator.configure(
            ConfigObjects::VspcCom_frameAccumulator::frameDetector,
            1,
            VspcCom::Allocation::memAllocator,
            VspcComConfig::BuffMgr::frameAccumulatorSize
        );
        """

        phase Fpp.ToCpp.Phases.tearDownComponents """
        VspcCom::frameAccumulator.cleanup();
        """
    }

    instance commsBufferManager: Svc.BufferManager base id VspcComConfig.BASE_ID + 0x02000 \
    {
        phase Fpp.ToCpp.Phases.configObjects """
        Svc::BufferManager::BufferBins bins;
        """

        phase Fpp.ToCpp.Phases.configComponents """
        memset(&ConfigObjects::VspcCom_commsBufferManager::bins, 0, sizeof(ConfigObjects::VspcCom_commsBufferManager::bins));
        ConfigObjects::VspcCom_commsBufferManager::bins.bins[0].bufferSize = VspcComConfig::BuffMgr::commsBuffSize;
        ConfigObjects::VspcCom_commsBufferManager::bins.bins[0].numBuffers = VspcComConfig::BuffMgr::commsBuffCount;
        ConfigObjects::VspcCom_commsBufferManager::bins.bins[1].bufferSize = VspcComConfig::BuffMgr::commsFileBuffSize;
        ConfigObjects::VspcCom_commsBufferManager::bins.bins[1].numBuffers = VspcComConfig::BuffMgr::commsFileBuffCount;
        VspcCom::commsBufferManager.setup(
            VspcComConfig::BuffMgr::commsBuffMgrId,
            0,
            VspcCom::Allocation::memAllocator,
            ConfigObjects::VspcCom_commsBufferManager::bins
        );
        """

        phase Fpp.ToCpp.Phases.tearDownComponents """
        VspcCom::commsBufferManager.cleanup();
        """
    }

    instance fprimeRouter: Svc.FprimeRouter base id VspcComConfig.BASE_ID + 0x03000

    instance tcDeframer: Svc.Ccsds.TcDeframer base id VspcComConfig.BASE_ID + 0x04000

    instance spacePacketDeframer: Svc.Ccsds.SpacePacketDeframer base id VspcComConfig.BASE_ID + 0x05000

    instance aggregator: Svc.ComAggregator base id VspcComConfig.BASE_ID + 0x06000 \
        queue size VspcComConfig.QueueSizes.aggregator \
        stack size VspcComConfig.StackSizes.aggregator

    # NOTE: name 'framer' is used for the framer that connects to the Com Adapter Interface for better subtopology interoperability
    instance framer: Svc.Ccsds.TmFramer base id VspcComConfig.BASE_ID + 0x07000

    instance spacePacketFramer: Svc.Ccsds.SpacePacketFramer base id VspcComConfig.BASE_ID + 0x08000

    instance apidManager: Svc.Ccsds.ApidManager base id VspcComConfig.BASE_ID + 0x09000

    instance comStub: Svc.ComStub base id VspcComConfig.BASE_ID + 0x0A000

    # APID-based uplink router: peeks the Space Packet APID of each complete TC
    # frame and routes the whole frame to the local deframe stack or out the
    # radio (radioOut, user-wired). Configure its routes in the deployment cpp
    # (apidRouter.setDefaultDestination / setApidRoute).
    instance apidRouter: Vspc.ApidRouter base id VspcComConfig.BASE_ID + 0x0B000

    topology FramingSubtopology {
        # Usage Note:
        #
        # When importing this subtopology, users shall establish 5 port connections with a component implementing
        # the Svc.Com (Svc/Interfaces/Com.fpp) interface. They are as follows:
        #
        # 1) Outputs:
        #     - VspcCom.framer.dataOut                 -> [Svc.Com].dataIn
        #     - VspcCom.frameAccumulator.dataReturnOut -> [Svc.Com].dataReturnIn
        # 2) Inputs:
        #     - [Svc.Com].dataReturnOut -> VspcCom.framer.dataReturnIn
        #     - [Svc.Com].comStatusOut  -> VspcCom.framer.comStatusIn
        #     - [Svc.Com].dataOut       -> VspcCom.frameAccumulator.dataIn


        # Active Components
        instance comQueue

        # Passive Components
        instance commsBufferManager
        instance frameAccumulator
        instance fprimeRouter
        instance tcDeframer
        instance spacePacketDeframer
        instance framer
        instance spacePacketFramer
        instance apidManager
        instance aggregator
        instance apidRouter

        connections Downlink {
            # ComQueue <-> SpacePacketFramer
            comQueue.dataOut                -> spacePacketFramer.dataIn
            spacePacketFramer.dataReturnOut -> comQueue.dataReturnIn
            # SpacePacketFramer buffer and APID management
            spacePacketFramer.bufferAllocate   -> commsBufferManager.bufferGetCallee
            spacePacketFramer.bufferDeallocate -> commsBufferManager.bufferSendIn
            spacePacketFramer.getApidSeqCount  -> apidManager.getApidSeqCountIn
            # SpacePacketFramer <-> TmFramer
            spacePacketFramer.dataOut -> aggregator.dataIn
            aggregator.dataOut        -> framer.dataIn

            framer.dataReturnOut      -> aggregator.dataReturnIn
            aggregator.dataReturnOut    -> spacePacketFramer.dataReturnIn

            # ComStatus
            framer.comStatusOut            -> aggregator.comStatusIn
            aggregator.comStatusOut        -> spacePacketFramer.comStatusIn
            spacePacketFramer.comStatusOut -> comQueue.comStatusIn
            # (Outgoing) Framer <-> ComInterface connections shall be established by the user
        }

        connections Uplink {
            # (Incoming) ComInterface <-> FrameAccumulator connections shall be established by the user
            # FrameAccumulator buffer allocations
            frameAccumulator.bufferDeallocate -> commsBufferManager.bufferSendIn
            frameAccumulator.bufferAllocate   -> commsBufferManager.bufferGetCallee
            # FrameAccumulator -> ApidRouter: complete TC frames are routed by
            # their Space Packet APID to the local deframe stack (localOut) or
            # out the radio (radioOut, user-wired in the deployment). The radio
            # path relays the frame verbatim and frees the buffer via the radio
            # component's own deallocate (same buffer manager), so only the
            # local and drop paths return to the accumulator here.
            frameAccumulator.dataOut -> apidRouter.dataIn
            apidRouter.localOut      -> tcDeframer.dataIn
            apidRouter.dataReturnOut -> frameAccumulator.dataReturnIn
            tcDeframer.dataReturnOut -> frameAccumulator.dataReturnIn
            # apidRouter.radioOut -> [radio TX] shall be established by the user
            # TcDeframer <-> SpacePacketDeframer
            tcDeframer.dataOut                -> spacePacketDeframer.dataIn
            spacePacketDeframer.dataReturnOut -> tcDeframer.dataReturnIn
            # SpacePacketDeframer APID validation
            spacePacketDeframer.validateApidSeqCount -> apidManager.validateApidSeqCountIn
            # SpacePacketDeframer <-> Router
            spacePacketDeframer.dataOut -> fprimeRouter.dataIn
            fprimeRouter.dataReturnOut  -> spacePacketDeframer.dataReturnIn
            # Router buffer allocations
            fprimeRouter.bufferAllocate   -> commsBufferManager.bufferGetCallee
            fprimeRouter.bufferDeallocate -> commsBufferManager.bufferSendIn
        }
    } # end FramingSubtopology

    # This subtopology uses FramingSubtopology with a ComStub component for Com Interface
    topology Subtopology {
        import FramingSubtopology

        instance comStub

        connections ComStub {
            # Framer <-> ComStub (Downlink)
            VspcCom.framer.dataOut -> comStub.dataIn
            comStub.dataReturnOut   -> VspcCom.framer.dataReturnIn
            comStub.comStatusOut    -> VspcCom.framer.comStatusIn

            # ComStub <-> FrameAccumulator (Uplink)
            comStub.dataOut -> VspcCom.frameAccumulator.dataIn
            VspcCom.frameAccumulator.dataReturnOut -> comStub.dataReturnIn
        }
    } # end Subtopology

} # end VspcCom
