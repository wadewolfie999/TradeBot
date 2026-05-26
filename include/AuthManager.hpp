#pragma once
#include "SystemConfig.hpp"
#include <cstdint>
#include <string>
#include <string_view>

class AuthManager {
public:
    struct Credentials {
        std::string apiKey;
        std::string apiSecret;
        bool valid() const noexcept { return !apiKey.empty() && !apiSecret.empty(); }
    };

    AuthManager() = default;

    // Loads credentials from SystemConfig first, then env fallbacks.
    // Returns true when both key+secret are available.
    bool load(const SystemConfig& cfg) noexcept;

    bool hasCredentials() const noexcept;
    std::string apiKey() const;
    std::string redactedApiKey() const;

    // HMAC-SHA256 signature in lowercase hex.
    std::string sign(std::string_view payload) const;
    static std::string hmacSha256Hex(std::string_view key, std::string_view payload);

    static uint64_t nonceMs() noexcept;

private:
    Credentials m_credentials;
};
