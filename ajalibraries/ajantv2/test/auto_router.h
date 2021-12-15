#pragma once

#include "ajabase/common/types.h"
#include "ajantv2/includes/ntv2devicefeatures.h"
#include "ajantv2/includes/ntv2enums.h"
#include "ajantv2/includes/ntv2publicinterface.h"
#include "ajantv2/includes/ntv2utils.h"

#include "ntv2qa/card_controller.h"

using CardCtrlPtr = std::unique_ptr<qa::CardController>;

class AutoRouter {
public:
    explicit AutoRouter(
        uint32_t card_index,
        uint32_t channel_index,
        uint32_t framestore_index,
        NTV2Mode mode,
        NTV2VideoFormat vf,
        NTV2PixelFormat pf,
        NTV2ReferenceSource ref_src,
        NTV2VANCMode vanc_mode,
        VPIDStandard vpid_std,
        ConnectionKind cnx_kind);
    AJAStatus Init();
    AJAStatus ApplyRouting(bool clear, bool force);
    CNTV2Card* GetCard();

private:
    bool check_wire_and_framestore_bounds(
        const NTV2DeviceID& device_id,
        uint32_t channel_index,
        uint32_t framestore_index,
        uint32_t want_wires,
        uint32_t want_framestores);

    NTV2ReferenceSource determine_ref_source(
                        NTV2Channel chan,
                        ConnectionKind kind);

    uint32_t m_card_index;
    uint32_t m_channel_index;
    uint32_t m_framestore_index;
    NTV2Mode m_mode;
    NTV2VideoFormat m_video_format;
    NTV2PixelFormat m_pixel_format;
    NTV2ReferenceSource m_ref_source;
    NTV2VANCMode m_vanc_mode;
    VPIDStandard m_vpid_standard;
    ConnectionKind m_connection_kind;
    bool m_insert_csc;
    bool m_clear_all;
    bool m_multi_format;
    bool m_force;
    CardCtrlPtr m_card_ctrl;
};