// SPDX-License-Identifier: Apache-2.0
// Copyright 2025 Pionix GmbH and Contributors to EVerest
#include <catch2/catch_test_macros.hpp>

#include "helper.hpp"

#include <iso15118/ev/d20/state/service_selection.hpp>
#include <iso15118/message/common_types.hpp>
#include <iso15118/message/service_selection.hpp>
#include <iso15118/message/type.hpp>

using namespace iso15118;
namespace dt = message_20::datatypes;

SCENARIO("ISO15118-20 EV ServiceSelection state transitions") {

    const ev::d20::session::feedback::Callbacks callbacks{};

    auto state_helper = FsmStateHelper(callbacks);

    auto ctx = state_helper.get_context();

    GIVEN("Good case - service selection response with OK.") {

        // setup the state and context to something reasonable
        const auto header = message_20::Header{
            .session_id = std::array<uint8_t, 8>{0x10, 0x34, 0xAB, 0x7A, 0x01, 0xF3, 0x95, 0x02},
            .timestamp = 1691411798,
        };

        ctx.get_session().set_id(header.session_id);

        fsm::v2::FSM<ev::d20::StateBase> fsm{ctx.create_state<ev::d20::state::ServiceSelection>()};

        const auto res = message_20::ServiceSelectionResponse{header, message_20::datatypes::ResponseCode::OK};

        state_helper.handle_response(res);

        const auto result = fsm.feed(ev::d20::Event::V2GTP_MESSAGE);

        THEN("Check if passes to DC_ChargeParameterDiscovery state and sends DC_ChargeParameterDiscoveryRequest") {
            REQUIRE(result.transitioned() == true);
            REQUIRE(fsm.get_current_state_id() == ev::d20::StateID::DC_ChargeParameterDiscovery);

            const auto request_message = ctx.get_request<message_20::DC_ChargeParameterDiscoveryRequest>();
            REQUIRE(request_message.has_value());

            const auto& request = request_message.value();
            REQUIRE(request.header.session_id == header.session_id);

            using DC_ModeReq = message_20::datatypes::DC_CPDReqEnergyTransferMode;

            REQUIRE(std::holds_alternative<DC_ModeReq>(request.transfer_mode));
            const auto& transfer_mode = std::get<DC_ModeReq>(request.transfer_mode);

            // Check that the requested parameters are as expected
            REQUIRE(dt::from_RationalNumber(transfer_mode.max_charge_power) == 1000.0f);
            REQUIRE(dt::from_RationalNumber(transfer_mode.min_charge_power) == 10.0f);
            REQUIRE(dt::from_RationalNumber(transfer_mode.max_charge_current) == 1000.0f);
            REQUIRE(dt::from_RationalNumber(transfer_mode.min_charge_current) == 20.0f);
            REQUIRE(dt::from_RationalNumber(transfer_mode.max_voltage) == 1250.0f);
            REQUIRE(dt::from_RationalNumber(transfer_mode.min_voltage) == 400.0f);
            REQUIRE(transfer_mode.target_soc == 90);
        }
    }
}
