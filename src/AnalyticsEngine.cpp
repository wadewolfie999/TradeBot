#include "AnalyticsEngine.hpp"
#include "PortfolioAllocator.hpp"
#include <fstream>
#include <filesystem>
#include <iomanip>
#include <cmath>

void AnalyticsEngine::setPortfolioAllocator(PortfolioAllocator* allocator) noexcept
{
    m_allocator = allocator;
}

void AnalyticsEngine::recordTrade(uint64_t timestamp, char side,
                                  double price, double quantity,
                                  double fee, double realizedPnL,
                                  double grossPnL,
                                  const std::string& strategyId)
{
    TradeLogEntry entry{timestamp, side, price, quantity,
                        fee, realizedPnL, grossPnL, strategyId};
    m_trades.push_back(std::move(entry));

    // Phase 11 telemetry: notify PortfolioAllocator on sell-leg closure
    if (m_allocator && side == 'S' && !strategyId.empty()) {
        m_allocator->onTradeClosed(strategyId, realizedPnL);
    }
}

void AnalyticsEngine::recordEquity(uint64_t timestamp, double equity,
                                   double drawdownPct, int openPositions,
                                   double var95)
{
    m_equity.push_back({timestamp, equity, drawdownPct, openPositions, var95});
}

void AnalyticsEngine::recordSlippage(uint64_t timestamp, const std::string& symbol,
                                     double expectedPrice, double actualPrice,
                                     double quantity)
{
    if (expectedPrice <= 0.0 || quantity <= 0.0) {
        return;
    }
    const double bps = ((actualPrice - expectedPrice) / expectedPrice) * 10000.0;
    m_slippage.push_back({timestamp, symbol, expectedPrice, actualPrice, quantity, bps});
    if (m_slippage.size() == 1) {
        m_slippageEwmaBps = bps;
    } else {
        m_slippageEwmaBps = (m_slippageAlpha * bps)
                          + ((1.0 - m_slippageAlpha) * m_slippageEwmaBps);
    }
}

void AnalyticsEngine::recordRegimeMatrix(uint64_t timestamp, const std::string& symbol,
                                         const std::string& regime,
                                         double adx, double variance)
{
    m_regimeMatrix.push_back({timestamp, symbol, regime, adx, variance});
}

bool AnalyticsEngine::finalize(const std::string& outputDir) const
{
    std::filesystem::create_directories(outputDir);

    {
        const std::string path = outputDir + "/trades.csv";
        std::ofstream ofs(path);
        if (!ofs.is_open()) { return false; }
        ofs << "timestamp,side,price,quantity,fee,realizedPnL,grossPnL,strategy_id\n";
        ofs << std::fixed << std::setprecision(6);
        for (const auto& t : m_trades) {
            ofs << t.timestamp    << ','
                << t.side         << ','
                << t.price        << ','
                << t.quantity     << ','
                << t.fee          << ','
                << t.realizedPnL  << ','
                << t.grossPnL     << ','
                << t.strategy_id  << '\n';
        }
    }

    {
        const std::string path = outputDir + "/equity.csv";
        std::ofstream ofs(path);
        if (!ofs.is_open()) { return false; }
        ofs << "timestamp,equity,drawdownPct,openPositions,var95\n";
        ofs << std::fixed << std::setprecision(6);
        for (const auto& e : m_equity) {
            ofs << e.timestamp      << ','
                << e.equity         << ','
                << e.drawdownPct    << ','
                << e.openPositions  << ','
                << e.var95          << '\n';
        }
    }

    {
        const std::string path = outputDir + "/slippage.csv";
        std::ofstream ofs(path);
        if (!ofs.is_open()) { return false; }
        ofs << "timestamp,symbol,expectedPrice,actualPrice,quantity,slippageBps,slippageEwmaBps\n";
        ofs << std::fixed << std::setprecision(6);
        double ewma = 0.0;
        for (std::size_t i = 0; i < m_slippage.size(); ++i) {
            const auto& s = m_slippage[i];
            ewma = (i == 0) ? s.slippageBps
                            : (m_slippageAlpha * s.slippageBps) + ((1.0 - m_slippageAlpha) * ewma);
            ofs << s.timestamp      << ','
                << s.symbol         << ','
                << s.expectedPrice  << ','
                << s.actualPrice    << ','
                << s.quantity       << ','
                << s.slippageBps    << ','
                << ewma             << '\n';
        }
    }

    {
        const std::string path = outputDir + "/regime_matrix.csv";
        std::ofstream ofs(path);
        if (!ofs.is_open()) { return false; }
        ofs << "timestamp,symbol,regime,adx,variance\n";
        ofs << std::fixed << std::setprecision(6);
        for (const auto& r : m_regimeMatrix) {
            ofs << r.timestamp << ','
                << r.symbol    << ','
                << r.regime    << ','
                << r.adx       << ','
                << r.variance  << '\n';
        }
    }

    return true;
}

const std::vector<TradeLogEntry>& AnalyticsEngine::getTrades() const noexcept
{
    return m_trades;
}

const std::vector<EquityPoint>& AnalyticsEngine::getEquity() const noexcept
{
    return m_equity;
}

const std::vector<SlippagePoint>& AnalyticsEngine::getSlippage() const noexcept
{
    return m_slippage;
}

const std::vector<RegimeMatrixPoint>& AnalyticsEngine::getRegimeMatrix() const noexcept
{
    return m_regimeMatrix;
}

double AnalyticsEngine::slippageEwmaBps() const noexcept
{
    return m_slippageEwmaBps;
}
