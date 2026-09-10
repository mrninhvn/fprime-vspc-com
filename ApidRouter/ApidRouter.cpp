// ======================================================================
// \title  ApidRouter.cpp
// \brief  cpp file for ApidRouter component implementation class
// ======================================================================

#include "ApidRouter/ApidRouter.hpp"

namespace Vspc {

// ----------------------------------------------------------------------
// Construction / destruction
// ----------------------------------------------------------------------

ApidRouter ::ApidRouter(const char* const compName) : ApidRouterComponentBase(compName) {}

ApidRouter ::~ApidRouter() {}

// ----------------------------------------------------------------------
// Configuration
// ----------------------------------------------------------------------

void ApidRouter ::setDefaultDestination(Destination dest) {
    this->m_defaultDest = dest;
}

void ApidRouter ::setApidRange(U16 loApid, U16 hiApid, Destination dest) {
    loApid = static_cast<U16>(loApid & APID_MASK);
    hiApid = static_cast<U16>(hiApid & APID_MASK);
    if (loApid > hiApid) {
        const U16 tmp = loApid;
        loApid = hiApid;
        hiApid = tmp;
    }
    if (this->m_numRoutes < MAX_ROUTES) {
        this->m_routes[this->m_numRoutes].apidLo = loApid;
        this->m_routes[this->m_numRoutes].apidHi = hiApid;
        this->m_routes[this->m_numRoutes].dest = dest;
        this->m_numRoutes++;
    }
}

void ApidRouter ::setApidRoute(U16 apid, Destination dest) {
    this->setApidRange(apid, apid, dest);
}

ApidRouter::Destination ApidRouter ::routeFor(U16 apid) const {
    // First matching range wins (insertion order).
    for (FwSizeType i = 0; i < this->m_numRoutes; i++) {
        if ((apid >= this->m_routes[i].apidLo) && (apid <= this->m_routes[i].apidHi)) {
            return this->m_routes[i].dest;
        }
    }
    return this->m_defaultDest;
}

// ----------------------------------------------------------------------
// Handler implementations
// ----------------------------------------------------------------------

void ApidRouter ::dataIn_handler(FwIndexType portNum, Fw::Buffer& data, const ComCfg::FrameContext& context) {
    // Need the 5-octet TC primary header plus the first 2 octets of the Space
    // Packet primary header (packet identification) to read the APID.
    const FwSizeType minSize = TC_PRIMARY_HEADER_SIZE + 2;
    if (!data.isValid() || (data.getSize() < minSize)) {
        this->m_dropCount++;
        this->tlmWrite_DropCount(this->m_dropCount);
        this->log_WARNING_LO_ShortFrame(static_cast<U32>(data.getSize()));
        this->dataReturnOut_out(0, data, context);
        return;
    }

    const U8* const bytes = data.getData();
    const U16 packetId = static_cast<U16>((static_cast<U16>(bytes[TC_PRIMARY_HEADER_SIZE]) << 8) |
                                          bytes[TC_PRIMARY_HEADER_SIZE + 1]);
    const U16 apid = static_cast<U16>(packetId & APID_MASK);

    const Destination dest = this->routeFor(apid);
    this->log_DIAGNOSTIC_Routed(apid, dest == Destination::RADIO);

    if (dest == Destination::RADIO) {
        this->m_radioCount++;
        this->tlmWrite_RadioCount(this->m_radioCount);
        this->radioOut_out(0, data, context);
    } else {
        this->m_localCount++;
        this->tlmWrite_LocalCount(this->m_localCount);
        this->localOut_out(0, data, context);
    }
}

}  // namespace Vspc
