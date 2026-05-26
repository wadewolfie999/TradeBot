#include "AsyncNetworkClient.hpp"
#include "BrokerGateway.hpp"
#include "LiveDataAdapter.hpp"
#include "LocalMetricsExporter.hpp"
#include "PortfolioManager.hpp"
#include "SystemConfig.hpp"

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

namespace {

void require(bool cond, const char* msg)
{
    if (!cond) {
        throw std::runtime_error(msg);
    }
}

bool runCommand(const std::string& cmd)
{
    return std::system(cmd.c_str()) == 0;
}

void test_local_metrics_exporter_payload()
{
    LocalMetricsExporter exporter;
    exporter.setGauge("aiio_live_queue_depth", 7.0);
    exporter.incrementCounter("aiio_connection_lifecycle_total", 2.0);
    exporter.ingestSystemSnapshot(
        0.321, 4, 100, 98, 3, 5, 2, 1);

    const std::string payload = exporter.renderPrometheusText();
    require(payload.find("# TYPE aiio_live_queue_depth gauge") != std::string::npos,
            "metrics payload missing live queue metric");
    require(payload.find("# TYPE aiio_connection_lifecycle_total counter") != std::string::npos,
            "metrics payload missing counter type");
    require(payload.find("aiio_network_handoff_p99_ms 0.321000") != std::string::npos,
            "metrics payload missing p99 handoff gauge value");

    const bool started = exporter.start(9470);
    if (started) {
        require(exporter.isRunning(), "metrics exporter expected running state");
        exporter.stop();
        require(!exporter.isRunning(), "metrics exporter failed to stop");
    } else {
        require(!exporter.lastError().empty(), "metrics exporter start failure missing error detail");
    }
}

void test_offline_tls_local_ca_validation()
{
    const std::filesystem::path base =
        std::filesystem::path("/tmp") / "aiio_phase17_tls_test";
    std::filesystem::remove_all(base);
    std::filesystem::create_directories(base);

    const std::string caKey = (base / "ca.key").string();
    const std::string caCert = (base / "ca.crt").string();
    const std::string serverKey = (base / "server.key").string();
    const std::string serverCsr = (base / "server.csr").string();
    const std::string serverCert = (base / "server.crt").string();
    const std::string sanCfg = (base / "san.cnf").string();
    const std::string rogueCaKey = (base / "rogue_ca.key").string();
    const std::string rogueCaCert = (base / "rogue_ca.crt").string();

    require(runCommand("openssl req -x509 -newkey rsa:2048 -sha256 -days 1 -nodes "
                       "-subj '/CN=AIIO Local Test CA' -keyout " + caKey + " -out " + caCert
                       + " > /dev/null 2>&1"),
            "failed to generate local CA");
    require(runCommand("openssl req -newkey rsa:2048 -sha256 -nodes "
                       "-subj '/CN=localhost' -keyout " + serverKey + " -out " + serverCsr
                       + " > /dev/null 2>&1"),
            "failed to generate server CSR");

    {
        std::ofstream cfgOut(sanCfg);
        cfgOut << "subjectAltName=DNS:localhost,IP:127.0.0.1\n";
        cfgOut << "extendedKeyUsage=serverAuth\n";
    }

    require(runCommand("openssl x509 -req -in " + serverCsr + " -CA " + caCert + " -CAkey "
                       + caKey + " -CAcreateserial -out " + serverCert
                       + " -days 1 -sha256 -extfile " + sanCfg + " > /dev/null 2>&1"),
            "failed to sign server certificate");

    require(runCommand("openssl req -x509 -newkey rsa:2048 -sha256 -days 1 -nodes "
                       "-subj '/CN=AIIO Rogue CA' -keyout " + rogueCaKey + " -out " + rogueCaCert
                       + " > /dev/null 2>&1"),
            "failed to generate rogue CA");

    std::string verifyErr;
    require(AsyncNetworkClient::validateCertificateChainForTest(caCert, serverCert, &verifyErr),
            "expected local CA verification success");
    require(!AsyncNetworkClient::validateCertificateChainForTest(rogueCaCert, serverCert, &verifyErr),
            "expected rogue CA verification failure");

    AsyncNetworkClient net;
    net.setStrictTlsValidation(true, caCert, "localhost", true);
    require(!net.connectWebSocket("wss://example.com/ws"),
            "strict INTRANET TLS should reject non-loopback host");
    require(net.lastError().find("non-loopback") != std::string::npos,
            "strict INTRANET TLS rejection message missing");
    net.stop();
}

void test_broker_fault_injector_determinism()
{
    SystemConfig cfg;
    cfg.mode = SystemMode::PAPER;

    PortfolioManager portfolio;
    BrokerGateway broker(cfg, portfolio);
    broker.connect();

    BrokerGateway::FaultInjectorConfig faultCfg;
    faultCfg.enabled = true;
    faultCfg.dropEveryN = 3;
    faultCfg.latencySpikeEveryN = 2;
    faultCfg.latencySpikeMs = 2;
    faultCfg.disconnectEveryN = 4;
    broker.setFaultInjectorConfig(faultCfg);

    uint32_t failures = 0;
    for (uint64_t i = 0; i < 12; ++i) {
        const BrokerFill fill = broker.submitOrder("BTCUSDT",
                                                   (i % 2) == 0,
                                                   0.5,
                                                   100.0 + static_cast<double>(i) * 0.1,
                                                   1'000 + i,
                                                   "FAULT_TEST");
        if (!fill.success) {
            ++failures;
        }
    }

    require(failures >= 4, "fault injector drop cadence did not trigger expected failures");
    require(broker.faultDropsTriggered() >= 4, "fault injector drop counter mismatch");
    require(broker.faultLatencySpikesTriggered() >= 6, "fault injector latency counter mismatch");
    require(broker.faultDisconnectsTriggered() >= 3, "fault injector disconnect counter mismatch");
    require(broker.reconnectLifecycleCount() >= 1, "broker reconnect lifecycle not observed");
    require(broker.isConnected(), "broker should be connected after auto-reconnect");
}

void test_live_resilience_with_fault_pressure_and_observability()
{
    SystemConfig cfg;
    cfg.mode = SystemMode::LIVE;
    cfg.wssEndpoint = "mock://phase17-live-feed";
    cfg.restEndpoint = "http://127.0.0.1:9";
    cfg.reconnectInitialMs = 10;
    cfg.reconnectMaxMs = 200;

    LiveDataAdapter adapter(cfg);
    adapter.connect();
    require(adapter.isConnected(), "live adapter failed initial mock connection");

    adapter.simulateDisconnect();
    const auto start = std::chrono::steady_clock::now();
    while (!adapter.isConnected()) {
        if (std::chrono::steady_clock::now() - start > std::chrono::seconds(2)) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    require(adapter.isConnected(), "live adapter failed reconnect under simulated network degradation");
    require(adapter.gapFillCount() > 0, "gap-fill not triggered after reconnect");

    LocalMetricsExporter exporter;
    constexpr int N = 200;
    for (int i = 0; i < N; ++i) {
        const uint64_t tsMs = 1710200000000ULL + static_cast<uint64_t>(i);
        const std::string payload = "{\"s\":\"BTCUSDT\",\"c\":\"63000.321\",\"v\":\"2.0\",\"E\":"
                                  + std::to_string(tsMs) + "}";
        adapter.injectSecureExternalPayloadForTest(payload);
    }

    int consumed = 0;
    while (consumed < N) {
        const auto c = adapter.getNextCandle(50);
        if (!c.has_value()) { continue; }
        ++consumed;
        exporter.ingestSystemSnapshot(
            adapter.networkHandoffP99Ms(),
            adapter.queueSize(),
            static_cast<uint64_t>(consumed),
            static_cast<uint64_t>(consumed),
            0,
            1, 0, adapter.reconnectAttempts());
    }

    const double p99Ms = adapter.networkHandoffP99Ms();
    std::cerr << "[phase17_tests] networkHandoffP99Ms=" << p99Ms << "\n";
    require(p99Ms < 5.0, "phase17 secure + observability overhead broke p99<5ms");
    const std::string metrics = exporter.renderPrometheusText();
    require(metrics.find("aiio_network_handoff_p99_ms") != std::string::npos,
            "exporter missing handoff metric after resilience run");
    adapter.disconnect();
}

} // namespace

int main()
{
    std::cerr << "[phase17_tests] local metrics exporter...\n";
    test_local_metrics_exporter_payload();
    std::cerr << "[phase17_tests] offline tls local ca validation...\n";
    test_offline_tls_local_ca_validation();
    std::cerr << "[phase17_tests] broker fault injector determinism...\n";
    test_broker_fault_injector_determinism();
    std::cerr << "[phase17_tests] live resilience and observability...\n";
    test_live_resilience_with_fault_pressure_and_observability();
    std::cout << "[phase17_tests] All tests passed.\n";
    return 0;
}
