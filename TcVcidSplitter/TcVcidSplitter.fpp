module Vspc {

    @ Splits complete CCSDS TC frames by virtual channel. Frames of the configured
    @ VCID leave on vcOut (ownership passes to the receiver); every other frame,
    @ and every frame while vcOut is unconnected or no VCID is configured, goes on
    @ to the local deframer on dataOut.
    passive component TcVcidSplitter {

        @ Complete TC frames in (FrameAccumulator.dataOut)
        sync input port dataIn: Svc.ComDataWithContext

        @ Frames for this node (TcDeframer.dataIn)
        output port dataOut: Svc.ComDataWithContext

        @ Frames of the configured VCID; the receiver frees the buffer
        output port vcOut: Fw.BufferSend

    }
}
