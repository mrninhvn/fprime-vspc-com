module Vspc {

    @ Svc.ComAggregator for multi-link downlink.
    @
    @ The stock aggregator packs packets from every ComQueue queue into one
    @ frame and sends the frame with the context of the last packet added. When
    @ the com adapter routes frames per queue to different links (LR2021Manager
    @ setTxRoute / RadioTxRoute: events -> UART, telemetry -> UHF, ...), whole
    @ packets then leave on the wrong link: telemetry riding an event frame out
    @ the UART, so a ground station sees APID sequence jumps and reordering.
    @
    @ This component behaves exactly like Svc.ComAggregator (same state machine,
    @ same ports) except that a frame never mixes queues: a packet from a
    @ different queue (FrameContext comQueueIndex) than the frame being filled
    @ is handled like a packet that does not fit, i.e. the current frame is sent
    @ and the packet starts the next one. Every frame's context then matches
    @ all of its packets, so per-queue routing is exact.
    active component RoutedComAggregator {
        import Svc.Framer
        sync input port timeout: Svc.Sched

        @ State machine instance for aggregation state machine (Svc.ComAggregator's)
        state machine instance aggregationMachine: Svc.AggregationMachine
    }
}
