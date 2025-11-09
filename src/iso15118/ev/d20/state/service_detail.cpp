// SPDX-License-Identifier: Apache-2.0
// Copyright 2025 Pionix GmbH, Roger Bedell, and Contributors to EVerest
#include <iso15118/detail/helper.hpp>
#include <iso15118/ev/d20/state/service_detail.hpp>
#include <iso15118/ev/d20/state/service_selection.hpp>
#include <iso15118/ev/detail/d20/context_helper.hpp>
#include <iso15118/message/service_detail.hpp>

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

void ServiceDetail::enter() {
}

Result ServiceDetail::feed([[maybe_unused]] Event ev) {
    if (ev != Event::V2GTP_MESSAGE) {
        return {};
    }

    const auto variant = m_ctx.pull_response();

    if (const auto res = variant->get_if<message_20::ServiceDetailResponse>()) {

        if (not check_response_code(res->response_code)) {
            m_ctx.stop_session(true); // Tell stack to close the tcp/tls connection
            return {};
        }

        // Todo(RB): Inform the EV application layer of the service details so it can choose an energy transfer
        // parameter set and optionally a VAS parameter set

        // Send request and transition to next state
        // Note that it is possible to send more than one ServiceDetailRequest, so we may need to stay in this state to
        // send more requests However, for simplicity, we just send one request for now and transition to
        // ServiceSelection state Specifically, look for the first service in the energy transfer service list ToDo(RB):
        // Handle multiple requests if needed, and multiple parameter sets
        message_20::ServiceSelectionRequest req;
        setup_header(req.header, m_ctx.get_session());
        req.selected_energy_transfer_service.parameter_set_id = res->service_parameter_list.front().id;
        req.selected_energy_transfer_service.service_id =
            static_cast<message_20::datatypes::ServiceCategory>(res->service);

        // TODO(RB) Save the selected parameters in the context in case we need them later, e.g., for scheduling
        //        m_ctx.save_selected_service_parameters(res->service_parameter_list,
        //                                              req.selected_energy_transfer_service.service_id);

        // Todo(RB): Handle VAS selection if needed
        m_ctx.respond(req);
        return m_ctx.create_state<ServiceSelection>();

    } else {
        logf_error("expected ServiceDetailResponse! But code type id: %d", variant->get_type());
        m_ctx.stop_session(true); // Tell stack to close the tcp/tls connection
        return {};
    }
}

} // namespace iso15118::ev::d20::state