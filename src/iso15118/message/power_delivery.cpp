// SPDX-License-Identifier: Apache-2.0
// Copyright 2023 Pionix GmbH and Contributors to EVerest
#include <iso15118/message/power_delivery.hpp>

#include <type_traits>

#include <iso15118/detail/variant_access.hpp>

#include <cbv2g/iso_20/iso20_CommonMessages_Encoder.h>

namespace iso15118::message_20 {

template <>
void convert(const struct iso20_Scheduled_EVPPTControlModeType& in, datatypes::Scheduled_EVPPTControlMode& out) {
    if (in.PowerToleranceAcceptance_isUsed) {
        out.power_tolerance_acceptance = static_cast<datatypes::PowerToleranceAcceptance>(in.PowerToleranceAcceptance);
    }
    out.selected_schedule = in.SelectedScheduleTupleID;
}

template <> void convert(const struct iso20_EVPowerProfileType& in, datatypes::PowerProfile& out) {
    out.time_anchor = in.TimeAnchor;

    if (in.Dynamic_EVPPTControlMode_isUsed) {
        out.control_mode.emplace<datatypes::Dynamic_EVPPTControlMode>();
        // NOTE (aw): nothing more to do here because Dynamic_EVPPTControlMode is empty
    } else if (in.Scheduled_EVPPTControlMode_isUsed) {
        auto& cm = out.control_mode.emplace<datatypes::Scheduled_EVPPTControlMode>();
        convert(in.Scheduled_EVPPTControlMode, cm);
    } else {
        throw std::runtime_error("PowerProfile control mode not defined");
    }

    auto& entries_in = in.EVPowerProfileEntries.EVPowerProfileEntry;
    out.entries.reserve(entries_in.arrayLen);
    for (auto i = 0; i < entries_in.arrayLen; ++i) {
        auto& entry_out = out.entries.emplace_back();
        const auto& entry_in = entries_in.array[i];
        convert(entry_in, entry_out);
    }
}

void convert(const iso20_channelSelectionType in, datatypes::ChannelSelection& out) {
    cb_convert_enum(in, out);
}

template <> void convert(const struct iso20_PowerDeliveryReqType& in, PowerDeliveryRequest& out) {
    convert(in.Header, out.header);

    cb_convert_enum(in.EVProcessing, out.processing);
    cb_convert_enum(in.ChargeProgress, out.charge_progress);

    CB2CPP_CONVERT_IF_USED(in.EVPowerProfile, out.power_profile);
    CB2CPP_CONVERT_IF_USED(in.BPT_ChannelSelection, out.channel_selection);
}

template <> void convert(const PowerDeliveryResponse& in, iso20_PowerDeliveryResType& out) {
    init_iso20_PowerDeliveryResType(&out);

    convert(in.header, out.Header);
    cb_convert_enum(in.response_code, out.ResponseCode);

    CPP2CB_CONVERT_IF_USED(in.status, out.EVSEStatus);
}

template <> void insert_type(VariantAccess& va, const struct iso20_PowerDeliveryReqType& in) {
    va.insert_type<PowerDeliveryRequest>(in);
};

template <> int serialize_to_exi(const PowerDeliveryResponse& in, exi_bitstream_t& out) {
    iso20_exiDocument doc;
    init_iso20_exiDocument(&doc);

    CB_SET_USED(doc.PowerDeliveryRes);

    convert(in, doc.PowerDeliveryRes);

    return encode_iso20_exiDocument(&out, &doc);
}

template <> size_t serialize(const PowerDeliveryResponse& in, const io::StreamOutputView& out) {
    return serialize_helper(in, out);
}

} // namespace iso15118::message_20
