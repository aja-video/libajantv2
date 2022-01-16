#include "auto_router.h"

#include "ajabase/system/make_unique_shim.h"
#include "ntv2qa/routing/presets.h"
#include "ntv2qa/helpers.h"

AutoRouter::AutoRouter(
        uint32_t card_index,
        uint32_t channel_index,
        uint32_t framestore_index,
        NTV2Mode mode,
        NTV2VideoFormat vf,
        NTV2PixelFormat pf,
        NTV2ReferenceSource ref_src,
        NTV2VANCMode vanc_mode,
        VPIDStandard vpid_std,
        ConnectionKind cnx_kind)
	: m_card_index{card_index},
	  m_channel_index{channel_index},
      m_framestore_index{framestore_index},
	  m_mode{mode},
	  m_video_format{vf},
	  m_pixel_format{pf},
	  m_ref_source{ref_src},
	  m_vanc_mode{vanc_mode},
      m_vpid_standard{vpid_std},
      m_connection_kind{cnx_kind},
	  m_insert_csc{false},
	  m_clear_all{false},
	  m_multi_format{false},
	  m_force{false},
	  m_card_ctrl{nullptr}
{
    m_card_ctrl = aja::make_unique<qa::CardController>(m_card_index);
}

CNTV2Card* AutoRouter::GetCard()
{
    return m_card_ctrl->GetCard();
}

NTV2ReferenceSource AutoRouter::determine_ref_source(
                    NTV2Channel chan,
                    ConnectionKind kind) {
    auto input_src = NTV2ChannelToInputSource(
        chan, ConnectionKindToInputSourceKinds(kind));
    return NTV2InputSourceToReferenceSource(input_src);
}

bool AutoRouter::check_wire_and_framestore_bounds(
    const NTV2DeviceID& device_id,
    uint32_t channel_index,
    uint32_t framestore_index,
    uint32_t want_wires,
    uint32_t want_framestores)
{
    const uint32_t num_framestores = ::NTV2DeviceGetNumFrameStores(device_id);

    if (m_mode == NTV2_MODE_CAPTURE) {
        const uint32_t num_inputs = static_cast<uint32_t>(::NTV2DeviceGetNumVideoInputs(device_id));
        if ((channel_index + want_wires) > num_inputs) {
            return false;
        }
    } else if (m_mode == NTV2_MODE_DISPLAY) {
        const uint32_t num_outputs = static_cast<uint32_t>(::NTV2DeviceGetNumVideoOutputs(device_id));
        if ((channel_index + want_wires) > num_outputs) {
            return false;
        }
    }

    if ((framestore_index + want_framestores) > num_framestores) {
        return false;
    }

    return true;
}

AJAStatus AutoRouter::Init()
{
    return m_card_ctrl->InitializeCard();
}

AJAStatus AutoRouter::ApplyRouting(bool clear, bool force)
{
    CNTV2Card* card = m_card_ctrl->GetCard();
    if (!card)
        return AJA_STATUS_NULL;
    const NTV2DeviceID& device_id = card->GetDeviceID();
    RoutingConfigurator rc;
    RoutingPreset rp;
    bool found_preset = rc.FindFirstPreset(
        m_connection_kind, device_id, m_mode,
        m_video_format, m_pixel_format,
        m_vpid_standard, rp);

    if (!found_preset) {
        // const QString vpid_byte_1_hex = QString("0x" % QString::number(static_cast<uint8_t>(m_vpid_standard), 16).toUpper());
        // qWarning() << "No RoutingPreset found! | kind = "
        //     << QString::fromStdString(ConnectionKindToString(m_connection_kind))
        //     << " | vf = " << QString::fromStdString(NTV2VideoFormatString(m_video_format))
        //     << " | pf = " << QString::fromStdString(NTV2FrameBufferFormatToString(m_pixel_format))
        //     << " | vpid = " << vpid_byte_1_hex;
        return AJA_STATUS_NOT_FOUND;
    }

    // ensure SDI and framestore indices are in bounds of preset
    if (!check_wire_and_framestore_bounds(
        device_id, m_channel_index, m_framestore_index,
        rp.num_channels, rp.num_framestores)) {
        // qCritical() << QString(
        //     "Framestore index " %
        //     QString::number(m_framestore_index) %
        //     " out of bounds! Framestores = " %
        //     QString::number(rp.num_framestores)
        // );
        return AJA_STATUS_FAIL;
    }

    if (m_card_ctrl->ApplyRoutingFromPreset(
        rp, m_channel_index, m_framestore_index, clear, force) == AJA_STATUS_SUCCESS) {

        // qInfo() << "Applied RoutingPreset: " << QString::fromStdString(rp.name);

        // zero-out the register settings for all channels
        for (uint32_t i = 0; i < NTV2_MAX_NUM_CHANNELS; i++) {
            NTV2Channel channel = (NTV2Channel)i;
            if (m_connection_kind == ConnectionKind::SDI) {
                card->SetSDIOut3GEnable(channel, false);
                card->SetSDIOut3GbEnable(channel, false);
                card->SetSDIOut6GEnable(channel, false);
                card->SetSDIOut12GEnable(channel, false);
                card->SetSDIInLevelBtoLevelAConversion(channel, false);
                card->SetSDIOutLevelAtoLevelBConversion(channel, false);
                card->SetSDIOutRGBLevelAConversion(channel, false);
                card->SetTsiFrameEnable(false, channel);
                card->Set4kSquaresEnable(false, channel);
                card->SetQuadQuadSquaresEnable(false, channel);
            }
        }

        // Per-channel register settings
        for (uint32_t i = m_channel_index; i < (m_channel_index + rp.num_channels); i++) {
            NTV2Channel channel = (NTV2Channel)i;

            if (m_connection_kind == ConnectionKind::SDI) {
                if (::NTV2DeviceHasBiDirectionalSDI(device_id))
                    card->SetSDITransmitEnable(channel, rp.mode == NTV2_MODE_DISPLAY);
                card->SetSDIOut3GEnable(channel, rp.flags & kEnable3GOut);
                card->SetSDIOut3GbEnable(channel, rp.flags & kEnable3GbOut);
                card->SetSDIOut6GEnable(channel, rp.flags & kEnable6GOut);
                card->SetSDIOut12GEnable(channel, rp.flags & kEnable12GOut);
                card->SetSDIInLevelBtoLevelAConversion(channel, rp.flags & kConvert3GIn);
                card->SetSDIOutLevelAtoLevelBConversion(channel, rp.flags & kConvert3GOut);
                card->SetSDIOutRGBLevelAConversion(channel, rp.flags & kConvert3GaRGBOut);
            }
        }

        // Per-framestore register settings
        bool is_4k = NTV2_IS_4K_VIDEO_FORMAT(m_video_format);
        bool is_8k = NTV2_IS_8K_VIDEO_FORMAT(m_video_format);
        for (uint32_t i = (uint32_t)m_framestore_index; i < (m_channel_index + rp.num_framestores); i++) {
            NTV2Channel channel = (NTV2Channel)i;
            card->EnableChannel(channel);
            card->SetMode(channel, rp.mode);
            // card->SetVANCMode(sdi_config.vanc_mode, channel);
            card->SetVideoFormat(m_video_format, false, false, channel);
            card->SetFrameBufferFormat(channel, m_pixel_format);
            if (is_4k || is_8k) {
                card->SetTsiFrameEnable(rp.flags & kEnable4KTSI, channel);
                if (is_4k && !is_8k) {
                    card->Set4kSquaresEnable(rp.flags & kEnable4KSquares, channel);
                } else if (!is_4k && is_8k) {
                    card->SetQuadQuadSquaresEnable(rp.flags & kEnable4KSquares, channel);
                }
            } else {
                card->SetTsiFrameEnable(false, channel);
                card->Set4kSquaresEnable(false, channel);
                card->SetQuadQuadSquaresEnable(false, channel);
            }
            // set_frame_number(fs.Channel(), fs.GetFrameBufferMode(), fs.GetFrameNumber());
        }

        if (m_mode == NTV2_MODE_DISPLAY) {
            NTV2Channel channel = GetNTV2ChannelForIndex(m_channel_index);
            NTV2AudioSystem audio_sys = NTV2ChannelToAudioSystem(channel);
            card->SetAudioLoopBack(NTV2_AUDIO_LOOPBACK_OFF, audio_sys);
            if (m_connection_kind == ConnectionKind::SDI) {
                card->SetSDIOutputAudioSystem(channel, audio_sys);
                card->SetSDIOutputDS2AudioSystem(channel, audio_sys);
            } else if (m_connection_kind == ConnectionKind::HDMI && NTV2DeviceGetNumHDMIVideoOutputs(device_id) > 0) {
                card->SetHDMIOutAudioChannels(NTV2_HDMIAudio8Channels);
                card->SetHDMIOutAudioSource2Channel(NTV2_AudioChannel1_2, audio_sys);
                card->SetHDMIOutAudioSource8Channel(NTV2_AudioChannel1_8, audio_sys);
                card->SetHDMIOutAudioFormat(NTV2_AUDIO_FORMAT_LPCM);
                card->SetHDMIOutAudioRate(NTV2_AUDIO_48K);
                card->SetHDMIOutVideoStandard(GetNTV2StandardFromVideoFormat(m_video_format));
                card->SetHDMIOutVideoFPS(GetNTV2FrameRateFromVideoFormat(m_video_format));
                if (NTV2DeviceCanDoAudioMixer(device_id)) {
                    card->SetAudioMixerInputAudioSystem(
                        NTV2_AudioMixerInputMain, audio_sys);
                    card->SetAudioMixerInputChannelSelect(
                        NTV2_AudioMixerInputMain, NTV2_AudioChannel1_2);
                    card->SetAudioMixerInputChannelsMute(
                        NTV2_AudioMixerInputAux1,
                        NTV2AudioChannelsMuteAll);
                    card->SetAudioMixerInputChannelsMute(
                        NTV2_AudioMixerInputAux2,
                        NTV2AudioChannelsMuteAll);
                }
            }
        }
    }

    return AJA_STATUS_SUCCESS;
}
