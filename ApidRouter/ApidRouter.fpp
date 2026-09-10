module Vspc {

    @ APID-based frame router.
    @
    @ Takes complete CCSDS TC frames (from a Svc.FrameAccumulator configured
    @ with the CcsdsTcFrameDetector), peeks the Space Packet APID in the frame
    @ data field, and routes the whole frame to either the local deframe stack
    @ or the radio (relayed verbatim to another node) per a configurable APID
    @ table. The frame buffer is never modified, so the radio path forwards the
    @ original bytes untouched (no re-framing / sequence-count regeneration).
    @
    @ Frame layout assumed (CCSDS TC, F Prime Type-BD, no segment header):
    @   [0..4]  TC transfer frame primary header (5 octets)
    @   [5..6]  Space Packet primary header packet identification
    @           (APID = low 11 bits)
    passive component ApidRouter {

        # ----------------------------------------------------------------------
        # Routing ports
        # ----------------------------------------------------------------------

        @ Complete TC frames in (wire a FrameAccumulator.dataOut here)
        guarded input port dataIn: Svc.ComDataWithContext

        @ Frames whose APID routes to the local node's deframe stack
        output port localOut: Svc.ComDataWithContext

        @ Frames whose APID routes to the radio, relayed verbatim
        output port radioOut: Svc.ComDataWithContext

        @ Returns ownership of frames dropped here (too short / unparseable) to
        @ the upstream FrameAccumulator
        output port dataReturnOut: Svc.ComDataWithContext

        # ----------------------------------------------------------------------
        # Telemetry / events
        # ----------------------------------------------------------------------

        @ Count of frames routed to the local node
        telemetry LocalCount: U32 update on change

        @ Count of frames routed to the radio
        telemetry RadioCount: U32 update on change

        @ Count of frames dropped (undersized / no Space Packet header)
        telemetry DropCount: U32 update on change

        @ A frame too short to contain a Space Packet header was dropped
        event ShortFrame(numBytes: U32) severity warning low \
            format "APID router: frame too short ({} bytes), dropped" throttle 5

        @ Routing decision (diagnostic)
        event Routed(apid: U16, toRadio: bool) severity diagnostic \
            format "APID router: apid 0x{x} -> radio={}" throttle 20

        # ----------------------------------------------------------------------
        # Standard AC ports
        # ----------------------------------------------------------------------

        @ Port for requesting the current time
        time get port timeCaller

        @ Port for sending telemetry channels to downlink
        telemetry port tlmOut

        @ Port for sending events to downlink
        event port logOut

        @ Port for sending textual representation of events
        text event port logTextOut

    }
}
