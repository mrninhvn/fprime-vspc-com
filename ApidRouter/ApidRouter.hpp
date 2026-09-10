// ======================================================================
// \title  ApidRouter.hpp
// \brief  hpp file for ApidRouter component implementation class
//
// Routes complete CCSDS TC frames by their Space Packet APID to either the
// local deframe stack or the radio (verbatim relay). See ApidRouter.fpp.
// ======================================================================

#ifndef Vspc_ApidRouter_HPP
#define Vspc_ApidRouter_HPP

#include "ApidRouter/ApidRouterComponentAc.hpp"

namespace Vspc {

class ApidRouter final : public ApidRouterComponentBase {
  public:
    //! Where a frame is sent based on its APID.
    enum class Destination { LOCAL, RADIO };

    //! Construct ApidRouter object
    ApidRouter(const char* const compName);

    //! Destroy ApidRouter object
    ~ApidRouter();

    // ----------------------------------------------------------------------
    // Configuration (called from the topology, before frames flow)
    // ----------------------------------------------------------------------

    //! Destination used for any APID not covered by a configured range.
    void setDefaultDestination(Destination dest);

    //! Route an inclusive range of APIDs [loApid, hiApid] to \p dest. Ranges
    //! are tested in the order they are added and the first match wins, so add
    //! a narrower/special-case range before a broader one that overlaps it.
    //! Ignored once the route table is full. APIDs are masked to 11 bits.
    void setApidRange(U16 loApid, U16 hiApid, Destination dest);

    //! Route a single APID to \p dest (a one-wide range; see setApidRange).
    void setApidRoute(U16 apid, Destination dest);

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for typed input ports
    // ----------------------------------------------------------------------

    //! Handler for dataIn: peek the APID and forward the frame.
    void dataIn_handler(FwIndexType portNum,
                        Fw::Buffer& data,
                        const ComCfg::FrameContext& context) override;

    //! Look up the destination for an APID (explicit route, else default).
    Destination routeFor(U16 apid) const;

    //! Size of the TC transfer frame primary header, in octets (F Prime
    //! Type-BD, no segment header). The Space Packet begins right after it.
    static constexpr FwSizeType TC_PRIMARY_HEADER_SIZE = 5;

    //! 11-bit APID mask in the Space Packet packet-identification field.
    static constexpr U16 APID_MASK = 0x07FF;

    //! Maximum number of configured APID ranges.
    static constexpr FwSizeType MAX_ROUTES = 16;

    struct Route {
        U16 apidLo;  //!< Inclusive low APID of the range
        U16 apidHi;  //!< Inclusive high APID of the range
        Destination dest;
    };

    Route m_routes[MAX_ROUTES];
    FwSizeType m_numRoutes = 0;
    Destination m_defaultDest = Destination::LOCAL;

    U32 m_localCount = 0;
    U32 m_radioCount = 0;
    U32 m_dropCount = 0;
};

}  // namespace Vspc

#endif
