#pragma once

#include "common.hpp"
#include <array>

namespace dns {

const auto kDnsConfig = std::to_array({
    std::make_pair(
        Question{
            .domain = "example.com",
            .type = RecordType::A,
            .cls = QueryClass::IN,
        },
        Answer{
            .response = "127.0.0.1",
            .ttl = 60,
        }),
    std::make_pair(
        Question{
            .domain = "eine-biene.com",
            .type = RecordType::A,
            .cls = QueryClass::IN,
        },
        Answer{
            .response = "192.168.2.1",
            .ttl = 60,
        }),
});

} // namespace dns
