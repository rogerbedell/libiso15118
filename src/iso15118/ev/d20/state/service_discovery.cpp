// SPDX-License-Identifier: Apache-2.0
// Copyright 2025 Pionix GmbH and Contributors to EVerest
#include <iso15118/detail/helper.hpp>
#include <iso15118/ev/d20/state/service_detail.hpp>
#include <iso15118/ev/d20/state/service_discovery.hpp>
#include <iso15118/ev/detail/d20/context_helper.hpp>
#include <iso15118/message/service_detail.hpp>
#include <iso15118/message/service_discovery.hpp>

namespace iso15118::ev::d20::state {

namespace {
using ResponseCode = message_20::datatypes::ResponseCode;
bool check_response_code(ResponseCode response_code) {
    switch (response_code) {
    case ResponseCode::OK:
        return true;
        [[fallthrough]];
    default:
        return false;
    }
}
} // namespace

void ServiceDiscovery::enter() {
    // TODO(SL): Adding logging
}

Result ServiceDiscovery::feed(Event ev) {
    if (ev != Event::V2GTP_MESSAGE) {
        return {};
    }

    const auto variant = m_ctx.pull_response();

    if (const auto res = variant->get_if<message_20::ServiceDiscoveryResponse>()) {

        if (not check_response_code(res->response_code)) {
            m_ctx.stop_session(true); // Tell stack to close the tcp/tls connection
            return {};
        }
        // Send request and transition to next state
        // Note that it is possible to send more than one ServiceDetailRequest, so we may need to stay in this state to
        // send more requests However, for simplicity, we just send one request for now and transition to
        // ServiceSelection state Specifically, look for the first service in the energy transfer service list ToDo(RB):
        // Handle multiple requests if needed
        message_20::ServiceDetailRequest req;
        setup_header(req.header, m_ctx.get_session());
        req.service = message_20::to_underlying_value(res->energy_transfer_service_list.front().service_id);

        m_ctx.respond(req);
        return m_ctx.create_state<ServiceDetail>();
    } else {
        logf_error("expected ServiceDiscoveryResponse! But code type id: %d", variant->get_type());
        m_ctx.stop_session(true); // Tell stack to close the tcp/tls connection
        return {};
    }
}

} // namespace iso15118::ev::d20::state