// SPDX-License-Identifier: Apache-2.0
// Copyright 2025 Pionix GmbH and Contributors to EVerest
#include <iso15118/detail/helper.hpp>
#include <iso15118/ev/d20/state/dc_charge_parameter_discovery.hpp>
#include <iso15118/ev/d20/state/schedule_exchange.hpp>
#include <iso15118/ev/detail/d20/context_helper.hpp>
#include <iso15118/message/dc_charge_parameter_discovery.hpp>
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

void DC_ChargeParameterDiscovery::enter() {
    // TODO(SL): Adding logging
}

Result DC_ChargeParameterDiscovery::feed(Event ev) {
    if (ev != Event::V2GTP_MESSAGE) {
        return {};
    }

    const auto variant = m_ctx.pull_response();

    if (const auto res = variant->get_if<message_20::DC_ChargeParameterDiscoveryResponse>()) {

        if (not check_response_code(res->response_code)) {
            m_ctx.stop_session(true); // Tell stack to close the tcp/tls connection
            return {};
        }

        // The EVSE reports what it can provide in terms of charge parameters.
        // The EV can use this information to adjust its charging strategy accordingly.
        // For simplicity, we just log the received parameters here.
        // TODO(RB): Inform the EV application layer of the received charge parameters so it can adjust its charging
        // strategy
        if (std::holds_alternative<message_20::datatypes::DC_CPDResEnergyTransferMode>(res->transfer_mode)) {
            const auto& transfer_mode =
                std::get<message_20::datatypes::DC_CPDResEnergyTransferMode>(res->transfer_mode);
            logf_info("Received DC Charge Parameters from EVSE:");
            logf_info("  Max Charge Power: %.2f kW",
                      message_20::datatypes::from_RationalNumber(transfer_mode.max_charge_power));
            logf_info("  Min Charge Power: %.2f kW",
                      message_20::datatypes::from_RationalNumber(transfer_mode.min_charge_power));
            logf_info("  Max Charge Current: %.2f A",
                      message_20::datatypes::from_RationalNumber(transfer_mode.max_charge_current));
            logf_info("  Min Charge Current: %.2f A",
                      message_20::datatypes::from_RationalNumber(transfer_mode.min_charge_current));
            logf_info("  Max Voltage: %.2f V", message_20::datatypes::from_RationalNumber(transfer_mode.max_voltage));
            logf_info("  Min Voltage: %.2f V", message_20::datatypes::from_RationalNumber(transfer_mode.min_voltage));
            if (transfer_mode.power_ramp_limit.has_value()) {
                logf_info("  Power Ramp Limit: %.2f kW/s",
                          message_20::datatypes::from_RationalNumber(transfer_mode.power_ramp_limit.value()));
            }
        } else if (std::holds_alternative<message_20::datatypes::BPT_DC_CPDResEnergyTransferMode>(res->transfer_mode)) {
            const auto& transfer_mode =
                std::get<message_20::datatypes::BPT_DC_CPDResEnergyTransferMode>(res->transfer_mode);
            logf_info("Received BPT DC Charge Parameters from EVSE:");
            logf_info("  Max Charge Power: %.2f kW",
                      message_20::datatypes::from_RationalNumber(transfer_mode.max_charge_power));
            logf_info("  Min Charge Power: %.2f kW",
                      message_20::datatypes::from_RationalNumber(transfer_mode.min_charge_power));
            logf_info("  Max Charge Current: %.2f A",
                      message_20::datatypes::from_RationalNumber(transfer_mode.max_charge_current));
            logf_info("  Min Charge Current: %.2f A",
                      message_20::datatypes::from_RationalNumber(transfer_mode.min_charge_current));
            logf_info("  Max Voltage: %.2f V", message_20::datatypes::from_RationalNumber(transfer_mode.max_voltage));
            logf_info("  Min Voltage: %.2f V", message_20::datatypes::from_RationalNumber(transfer_mode.min_voltage));
            logf_info("  Max Discharge Power: %.2f kW",
                      message_20::datatypes::from_RationalNumber(transfer_mode.max_discharge_power));
            logf_info("  Min Discharge Power: %.2f kW",
                      message_20::datatypes::from_RationalNumber(transfer_mode.min_discharge_power));
            logf_info("  Max Discharge Current: %.2f A",
                      message_20::datatypes::from_RationalNumber(transfer_mode.max_discharge_current));
            logf_info("  Min Discharge Current: %.2f A",
                      message_20::datatypes::from_RationalNumber(transfer_mode.min_discharge_current));
        } else {
            logf_error("Unknown transfer mode variant received in DC_ChargeParameterDiscoveryResponse");
        }

        // Next is ScheduleExchange. First, we need the selected service parameters ControlMode to know to use Scheduled
        // or Dynamic
        /*const auto& control_mode = m_ctx.get_saved_selected_service_parameters().control_mode;
        if (control_mode == message_20::datatypes::ControlMode::Scheduled) {
            logf_info("Control Mode: Scheduled");
        } else {
            logf_info("Control Mode: Dynamic");
        }*/
        // Assuming Scheduled for now
        message_20::datatypes::ControlMode control_mode = message_20::datatypes::ControlMode::Scheduled;
        logf_info("Assuming Control Mode: Scheduled for ScheduleExchangeRequest");

        message_20::ScheduleExchangeRequest req;
        setup_header(req.header, m_ctx.get_session());
        req.max_supporting_points = message_20::datatypes::MaxSupportingPointsScheduleTuple{10};
        if (control_mode == message_20::datatypes::ControlMode::Scheduled) {
            message_20::datatypes::Scheduled_SEReqControlMode scheduled_mode{};
            scheduled_mode.departure_time = std::nullopt; // No specific departure time
            scheduled_mode.target_energy = std::nullopt;  // No specific target energy
            scheduled_mode.max_energy = std::nullopt;     // No specific max energy
            scheduled_mode.min_energy = std::nullopt;     // No specific min energy
            scheduled_mode.energy_offer = std::nullopt;   // No specific energy offer
            req.control_mode = scheduled_mode;
        } else {
            message_20::datatypes::Dynamic_SEReqControlMode dynamic_mode{};
            dynamic_mode.departure_time = 0;            // Immediate departure
            dynamic_mode.minimum_soc = std::nullopt;    // No specific minimum SOC
            dynamic_mode.target_soc = std::nullopt;     // No specific target SOC
            dynamic_mode.target_energy = {0, 0};        // No specific target energy
            dynamic_mode.max_energy = {0, 0};           // No specific max energy
            dynamic_mode.min_energy = {0, 0};           // No specific min energy
            dynamic_mode.max_v2x_energy = std::nullopt; // No specific max V2X energy
            dynamic_mode.min_v2x_energy = std::nullopt; // No specific min V2X energy
            req.control_mode = dynamic_mode;
        }
        m_ctx.respond(req);

        return m_ctx.create_state<ScheduleExchange>();

    } else {
        logf_error("expected DC_ChargeParameterDiscoveryResponse! But code type id: %d", variant->get_type());
        m_ctx.stop_session(true); // Tell stack to close the tcp/tls connection
        return {};
    }
}

} // namespace iso15118::ev::d20::state