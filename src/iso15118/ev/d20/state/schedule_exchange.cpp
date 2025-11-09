// SPDX-License-Identifier: Apache-2.0
// Copyright 2025 Pionix GmbH and Contributors to EVerest
#include <iso15118/detail/helper.hpp>
#include <iso15118/ev/d20/state/schedule_exchange.hpp>
#include <iso15118/ev/detail/d20/context_helper.hpp>
#include <iso15118/message/schedule_exchange.hpp>

namespace iso15118::ev::d20::state {

namespace {
using ResponseCode = message_20::datatypes::ResponseCode;
bool check_response_code(ResponseCode response_code) {
    switch (response_code) {
    case ResponseCode::OK:
        return true;
    case ResponseCode::FAILED_WrongChargeParameter:
        return false;
        [[fallthrough]];
    default:
        return false;
    }
}
} // namespace

void ScheduleExchange::enter() {
    // TODO(SL): Adding logging
}

Result ScheduleExchange::feed(Event ev) {
    if (ev != Event::V2GTP_MESSAGE) {
        return {};
    }

    const auto variant = m_ctx.pull_response();

    if (const auto res = variant->get_if<message_20::ScheduleExchangeResponse>()) {

        if (not check_response_code(res->response_code)) {
            m_ctx.stop_session(true); // Tell stack to close the tcp/tls connection
            return {};
        }

        // m_ctx.respond(req);

        // The EVSE may send back a EVSEProcessing status = "ongoing" in which case the EV shall repeat the
        // ScheduleExchangeReq message after a certain time until a final status is received.
        return m_ctx.create_state<ScheduleExchange>();

    } else {
        logf_error("expected ScheduleExchangeResponse! But code type id: %d", variant->get_type());
        m_ctx.stop_session(true); // Tell stack to close the tcp/tls connection
        return {};
    }
}

} // namespace iso15118::ev::d20::state