#include "AuthManager.hpp"
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <vector>

namespace {

constexpr std::array<uint32_t, 64> K = {
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};

inline uint32_t rotr(uint32_t x, uint32_t n) noexcept
{
    return (x >> n) | (x << (32 - n));
}

std::array<uint8_t, 32> sha256(std::string_view data)
{
    std::array<uint32_t, 8> h = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };

    const uint64_t bitLen = static_cast<uint64_t>(data.size()) * 8ULL;
    std::size_t paddedLen = data.size() + 1 + 8;
    const std::size_t rem = paddedLen % 64;
    if (rem != 0) {
        paddedLen += (64 - rem);
    }

    std::vector<uint8_t> padded(paddedLen, 0);
    if (!data.empty()) {
        std::memcpy(padded.data(), data.data(), data.size());
    }
    padded[data.size()] = 0x80;
    for (int i = 0; i < 8; ++i) {
        padded[paddedLen - 1 - i] = static_cast<uint8_t>((bitLen >> (i * 8)) & 0xff);
    }

    std::array<uint32_t, 64> w {};
    for (std::size_t offset = 0; offset < paddedLen; offset += 64) {
        for (int i = 0; i < 16; ++i) {
            const std::size_t j = offset + static_cast<std::size_t>(i * 4);
            w[i] = (static_cast<uint32_t>(padded[j]) << 24)
                 | (static_cast<uint32_t>(padded[j + 1]) << 16)
                 | (static_cast<uint32_t>(padded[j + 2]) << 8)
                 | static_cast<uint32_t>(padded[j + 3]);
        }

        for (int i = 16; i < 64; ++i) {
            const uint32_t s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
            const uint32_t s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
            w[i] = w[i - 16] + s0 + w[i - 7] + s1;
        }

        uint32_t a = h[0];
        uint32_t b = h[1];
        uint32_t c = h[2];
        uint32_t d = h[3];
        uint32_t e = h[4];
        uint32_t f = h[5];
        uint32_t g = h[6];
        uint32_t hh = h[7];

        for (int i = 0; i < 64; ++i) {
            const uint32_t S1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
            const uint32_t ch = (e & f) ^ ((~e) & g);
            const uint32_t temp1 = hh + S1 + ch + K[i] + w[i];
            const uint32_t S0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
            const uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
            const uint32_t temp2 = S0 + maj;

            hh = g;
            g = f;
            f = e;
            e = d + temp1;
            d = c;
            c = b;
            b = a;
            a = temp1 + temp2;
        }

        h[0] += a;
        h[1] += b;
        h[2] += c;
        h[3] += d;
        h[4] += e;
        h[5] += f;
        h[6] += g;
        h[7] += hh;
    }

    std::array<uint8_t, 32> out {};
    for (int i = 0; i < 8; ++i) {
        out[i * 4 + 0] = static_cast<uint8_t>((h[i] >> 24) & 0xff);
        out[i * 4 + 1] = static_cast<uint8_t>((h[i] >> 16) & 0xff);
        out[i * 4 + 2] = static_cast<uint8_t>((h[i] >> 8) & 0xff);
        out[i * 4 + 3] = static_cast<uint8_t>(h[i] & 0xff);
    }
    return out;
}

std::string toHex(const uint8_t* data, std::size_t n)
{
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (std::size_t i = 0; i < n; ++i) {
        oss << std::setw(2) << static_cast<int>(data[i]);
    }
    return oss.str();
}

} // namespace

bool AuthManager::load(const SystemConfig& cfg) noexcept
{
    m_credentials = {};

    if (!cfg.apiKey.empty() && !cfg.apiSecret.empty()) {
        m_credentials.apiKey = cfg.apiKey;
        m_credentials.apiSecret = cfg.apiSecret;
        return true;
    }

    const char* key = std::getenv(cfg.apiKeyEnv.c_str());
    const char* sec = std::getenv(cfg.apiSecretEnv.c_str());
    if ((!key || !sec) || std::strlen(key) == 0 || std::strlen(sec) == 0) {
        key = std::getenv("BINANCE_API_KEY");
        sec = std::getenv("BINANCE_API_SECRET");
    }
    if (!key || !sec) {
        return false;
    }

    m_credentials.apiKey = key;
    m_credentials.apiSecret = sec;
    return m_credentials.valid();
}

bool AuthManager::hasCredentials() const noexcept
{
    return m_credentials.valid();
}

std::string AuthManager::apiKey() const
{
    return m_credentials.apiKey;
}

std::string AuthManager::redactedApiKey() const
{
    if (m_credentials.apiKey.size() <= 8) {
        return "****";
    }
    return m_credentials.apiKey.substr(0, 4) + "****"
         + m_credentials.apiKey.substr(m_credentials.apiKey.size() - 4);
}

std::string AuthManager::sign(std::string_view payload) const
{
    if (m_credentials.apiSecret.empty()) {
        return {};
    }
    return hmacSha256Hex(m_credentials.apiSecret, payload);
}

std::string AuthManager::hmacSha256Hex(std::string_view key,
                                       std::string_view payload)
{
    constexpr std::size_t blockSize = 64;

    std::array<uint8_t, blockSize> keyBlock {};
    if (key.size() > blockSize) {
        const auto keyHash = sha256(key);
        std::memcpy(keyBlock.data(), keyHash.data(), keyHash.size());
    } else if (!key.empty()) {
        std::memcpy(keyBlock.data(), key.data(), key.size());
    }

    std::array<uint8_t, blockSize> oKeyPad {};
    std::array<uint8_t, blockSize> iKeyPad {};
    for (std::size_t i = 0; i < blockSize; ++i) {
        oKeyPad[i] = keyBlock[i] ^ 0x5c;
        iKeyPad[i] = keyBlock[i] ^ 0x36;
    }

    std::vector<uint8_t> inner(blockSize + payload.size(), 0);
    std::memcpy(inner.data(), iKeyPad.data(), blockSize);
    if (!payload.empty()) {
        std::memcpy(inner.data() + blockSize, payload.data(), payload.size());
    }
    const auto innerHash = sha256(std::string_view(
        reinterpret_cast<const char*>(inner.data()), inner.size()));

    std::vector<uint8_t> outer(blockSize + innerHash.size(), 0);
    std::memcpy(outer.data(), oKeyPad.data(), blockSize);
    std::memcpy(outer.data() + blockSize, innerHash.data(), innerHash.size());
    const auto outerHash = sha256(std::string_view(
        reinterpret_cast<const char*>(outer.data()), outer.size()));

    return toHex(outerHash.data(), outerHash.size());
}

uint64_t AuthManager::nonceMs() noexcept
{
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
}
