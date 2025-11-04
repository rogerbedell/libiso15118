// SPDX-License-Identifier: Apache-2.0
// Copyright 2025 Pionix GmbH and Contributors to EVerest
#include <iso15118/detail/helper.hpp>
#include <iso15118/ev/d20/state/dc_charge_parameter_discovery.hpp>
#include <iso15118/ev/d20/state/service_selection.hpp>
#include <iso15118/ev/detail/d20/context_helper.hpp>
#include <iso15118/message/dc_charge_parameter_discovery.hpp>
#include <iso15118/message/service_selection.hpp>

namespace iso15118::ev::d20::state {

namespace {
using ResponseCode = message_20::datatypes::ResponseCode;
bool check_response_code(ResponseCode response_code) {
    switch (response_code) {
    case ResponseCode::OK:
        return true;
    case ResponseCode::FAILED_ServiceIDInvalid:
        return false;
        [[fallthrough]];
    default:
        return false;
    }
}
} // namespace

void ServiceSelection::enter() {
    // TODO(SL): Adding logging
}

Result ServiceSelection::feed(Event ev) {
    if (ev != Event::V2GTP_MESSAGE) {
        return {};
    }

    const auto variant = m_ctx.pull_response();

    if (const auto res = variant->get_if<message_20::ServiceSelectionResponse>()) {

        if (not check_response_code(res->response_code)) {
            m_ctx.stop_session(true); // Tell stack to close the tcp/tls connection
            return {};
        }

        // Todo(RB): Do different things based on selected service and the requirements of the EV.
        // TODO(RB): For now, use canned values, normally get these from the EV
        // And, we just transition to DC_ChargeParameterDiscovery for DC services since we only handle MCS at the moment
        // Todo(RB): Handle AC and DC services as well.
        message_20::DC_ChargeParameterDiscoveryRequest req;
        setup_header(req.header, m_ctx.get_session());
        message_20::datatypes::DC_CPDReqEnergyTransferMode transfer_mode{};
        transfer_mode.max_charge_power = {1000, 0};   // 1000 kW
        transfer_mode.min_charge_power = {10, 0};     // 10 kW
        transfer_mode.max_charge_current = {1000, 0}; // 1000 A
        transfer_mode.min_charge_current = {20, 0};   // 20 A
        transfer_mode.max_voltage = {1250, 0};        // 1250 V
        transfer_mode.min_voltage = {400, 0};         // 400 V
        transfer_mode.target_soc = 90;                // 90 %
        req.transfer_mode = transfer_mode;

        m_ctx.respond(req);
        return m_ctx.create_state<DC_ChargeParameterDiscovery>();

    } else {
        logf_error("expected ServiceSelectionResponse! But code type id: %d", variant->get_type());
        m_ctx.stop_session(true); // Tell stack to close the tcp/tls connection
        return {};
    }
}

} // namespace iso15118::ev::d20::state