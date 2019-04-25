//
// Created by fatih on 1/8/19.
//

#pragma once

#include "common.hpp"
#include <nlohmann/json.hpp>

namespace caps
{
    template <class SignerT>
    nlohmann::json json_serialize(token<nlohmann::json, SignerT>);
}