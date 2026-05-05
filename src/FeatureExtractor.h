#pragma once

#include <array>
#include <cstdint>

class FeatureExtractor {
public:
  using FeatureVector = std::array<double, 8>;

  FeatureVector extract(std::uint64_t totalPackets,
                        std::uint64_t totalBytes,
                        std::uint64_t tcpPackets,
                        std::uint64_t udpPackets,
                        std::uint64_t icmpPackets,
                        std::uint64_t synPackets,
                        std::uint64_t finPackets,
                        std::uint64_t rstPackets) const;
};
