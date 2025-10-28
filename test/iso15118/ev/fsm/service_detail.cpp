// SPDX-License-Identifier: Apache-2.0
// Copyright 2025 Pionix GmbH and Contributors to EVerest
#include <catch2/catch_test_macros.hpp>

#include "helper.hpp"

#include <iso15118/ev/d20/state/service_detail.hpp>
#include <iso15118/message/service_detail.hpp>
#include <iso15118/message/type.hpp>
#include <iso15118/message/common_types.hpp>

using namespace iso15118;

SCENARIO("ISO15118-20 EV ServiceDetail state transitions") {

    const ev::d20::session::feedback::Callbacks callbacks{};

    auto state_helper = FsmStateHelper(callbacks);

    auto ctx = state_helper.get_context();

    GIVEN("Good case - service detail response with OK and MCS service") {

        // setup the state and context to something reasonable
        const auto header = message_20::Header{
            .session_id = std::array<uint8_t, 8>{0x10, 0x34, 0xAB, 0x7A, 0x01, 0xF3, 0x95, 0x02},
            .timestamp = 1691411798,
        };

        ctx.get_session().set_id(header.session_id);

        fsm::v2::FSM<ev::d20::StateBase> fsm{ctx.create_state<ev::d20::state::ServiceDetail>()};
        message_20::datatypes::McsParameterList mcs_params{
            .connector = message_20::datatypes::McsConnector::Mcs,
            .control_mode = message_20::datatypes::ControlMode::Dynamic,
            .mobility_needs_mode = message_20::datatypes::MobilityNeedsMode::ProvidedByEvcc,
            .pricing = message_20::datatypes::Pricing::NoPricing,
        };
        const auto res =
            message_20::ServiceDetailResponse{header, message_20::datatypes::ResponseCode::OK,
                                              message_20::to_underlying_value(message_20::datatypes::ServiceCategory::MCS),
                                              message_20::datatypes::ServiceParameterList{
                                                    message_20::datatypes::ParameterSet{
                                                        0, mcs_params},
                                                }};
        state_helper.handle_response(res);

        const auto result = fsm.feed(ev::d20::Event::V2GTP_MESSAGE);

        THEN("Check if passes to Service Selection state and sends ServiceSelectionRequest with MCS") {
            REQUIRE(result.transitioned() == true);
            REQUIRE(fsm.get_current_state_id() == ev::d20::StateID::ServiceSelection);

            const auto request_message = ctx.get_request<message_20::ServiceSelectionRequest>();
            REQUIRE(request_message.has_value());

            const auto& request = request_message.value();
            REQUIRE(request.header.session_id == header.session_id);

            // Check that the requested service is MCS
            REQUIRE(request.selected_energy_transfer_service.service_id ==
                    message_20::datatypes::ServiceCategory::MCS);
            REQUIRE(request.selected_energy_transfer_service.parameter_set_id == 0);
        }
    }
}
