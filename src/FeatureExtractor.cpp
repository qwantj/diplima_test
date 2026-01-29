#include "FeatureExtractor.h"

FeatureExtractor::FeatureVector FeatureExtractor::extract(std::uint64_t totalPackets,
                                                         std::uint64_t totalBytes,
                                                         std::uint64_t tcpPackets,
                                                         std::uint64_t udpPackets,
                                                         std::uint64_t icmpPackets,
                                                         std::uint64_t synPackets,
                                                         std::uint64_t finPackets,
                                                         std::uint64_t rstPackets) const {
  return {static_cast<double>(totalPackets),
          static_cast<double>(totalBytes),
          static_cast<double>(tcpPackets),
          static_cast<double>(udpPackets),
          static_cast<double>(icmpPackets),
          static_cast<double>(synPackets),
          static_cast<double>(finPackets),
          static_cast<double>(rstPackets)};
}
