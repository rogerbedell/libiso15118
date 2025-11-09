// SPDX-License-Identifier: Apache-2.0
// Copyright 2025 Pionix GmbH and Contributors to EVerest
#include <catch2/catch_test_macros.hpp>

#include "helper.hpp"

#include <iso15118/ev/d20/state/dc_charge_parameter_discovery.hpp>
#include <iso15118/ev/d20/state/schedule_exchange.hpp>
#include <iso15118/message/common_types.hpp>
#include <iso15118/message/dc_charge_parameter_discovery.hpp>
#include <iso15118/message/schedule_exchange.hpp>
#include <iso15118/message/type.hpp>

using namespace iso15118;
namespace dt = message_20::datatypes;

SCENARIO("ISO15118-20 EV DC_ChargeParameterDiscovery state transitions") {

    const ev::d20::session::feedback::Callbacks callbacks{};

    auto state_helper = FsmStateHelper(callbacks);

    auto ctx = state_helper.get_context();

    GIVEN("Good case - DC_ChargeParameterDiscovery response with OK.") {

        // setup the state and context to something reasonable
        const auto header = message_20::Header{
            .session_id = std::array<uint8_t, 8>{0x10, 0x34, 0xAB, 0x7A, 0x01, 0xF3, 0x95, 0x02},
            .timestamp = 1691411798,
        };

        ctx.get_session().set_id(header.session_id);

        fsm::v2::FSM<ev::d20::StateBase> fsm{ctx.create_state<ev::d20::state::DC_ChargeParameterDiscovery>()};

        const auto res =
            message_20::DC_ChargeParameterDiscoveryResponse{header, message_20::datatypes::ResponseCode::OK,
                                                            dt::DC_CPDResEnergyTransferMode{
                                                                .max_charge_power = {100, 3},
                                                                .min_charge_power = {10, 0},
                                                                .max_charge_current = {1000, 0},
                                                                .min_charge_current = {20, 0},
                                                                .max_voltage = {1250, 0},
                                                                .min_voltage = {400, 0},
                                                                .power_ramp_limit = std::nullopt,
                                                            }};

        state_helper.handle_response(res);

        const auto result = fsm.feed(ev::d20::Event::V2GTP_MESSAGE);

        THEN("Check if passes to ScheduleExchange state and sends ScheduleExchangeRequest") {
            REQUIRE(result.transitioned() == true);
            REQUIRE(fsm.get_current_state_id() == ev::d20::StateID::ScheduleExchange);

            const auto request_message = ctx.get_request<message_20::ScheduleExchangeRequest>();
            REQUIRE(request_message.has_value());

            const auto& request = request_message.value();
            REQUIRE(request.header.session_id == header.session_id);

            // Check that the request parameters are as expected (even though most are optional and not currently set)
            REQUIRE(request.max_supporting_points == 10);
            REQUIRE(std::holds_alternative<dt::Scheduled_SEReqControlMode>(request.control_mode));
            const auto& control_mode = std::get<dt::Scheduled_SEReqControlMode>(request.control_mode);
            REQUIRE(!control_mode.departure_time.has_value());
            REQUIRE(!control_mode.target_energy.has_value());
            REQUIRE(!control_mode.max_energy.has_value());
            REQUIRE(!control_mode.min_energy.has_value());
        }
    }
}
