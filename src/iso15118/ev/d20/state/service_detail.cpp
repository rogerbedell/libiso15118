// SPDX-License-Identifier: Apache-2.0
// Copyright 2025 Pionix GmbH and Contributors to EVerest
#include <iso15118/ev/d20/state/service_detail.hpp>

namespace iso15118::ev::d20::state {

void ServiceDetail::enter() {
}

Result ServiceDetail::feed([[maybe_unused]] Event ev) {
    return {};
}

} // namespace iso15118::ev::d20::state