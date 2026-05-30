
import re

with open('docs/academic/ВКР_черновик_claude.md', 'r', encoding='utf-8') as f:
    text = f.read()

# Рис. 4. Диаграмма класса TrafficMonitor
fig4_code = """```mermaid
classDiagram
  class TrafficMonitor {
    -pcpp::PcapLiveDevice* device_
    -BlockingConcurrentQueue~PacketBuffer*~ packetQueue_
    -ConcurrentQueue~PacketBuffer*~ bufferPool_
    -std::atomic~bool~ capturing_
    -std::atomic~uint64_t~ totalCaptured_
    -std::atomic~uint64_t~ droppedPackets_
    -std::string bpfFilter_
    +static listInterfaces() vector~pair~
    +startCapture(interfaceName) bool
    +stopCapture() void
    +replayPcap(filePath) bool
    +dequeuePacket(packet) bool
    +recycleBuffer(packet) void
    -static onPacketArrived(packet, dev, cookie)
    -getBuffer() PacketBuffer*
  }
  class PacketBuffer {
    +uint8_t* data
    +size_t size
    +timespec timestamp
  }
  class PcapPlusPlus {
    <<Library>>
  }
  TrafficMonitor ..> PcapPlusPlus : использует для захвата
  TrafficMonitor "1" *-- "many" PacketBuffer : управляет пулом
```
"""

# Рис. 5. Диаграмма класса FeatureExtractor
fig5_code = """```mermaid
classDiagram
  class FeatureExtractor {
    -ScalerParams scaler_
    -map~FlowKey, FlowStats~ activeFlows_
    -time_point windowStart_
    -uint64_t totalPackets_
    -map~string, uint64_t~ srcIpCounts_
    -vector~int~ sizeHistogram_
    +loadScalerParams(jsonPath) bool
    +processPacket(pktBuf) void
    +computeNormalizedFeatures() vector~double~
    +computeFeaturesBatch() vector~vector~
    +fillTelemetry(result) void
    +reset() void
    +setModelScaler(modelName) void
  }
  class FlowStats {
    +string initiatorIp
    +uint64_t fwdPackets
    +uint64_t bwdPackets
    +uint64_t fwdBytes
    +uint64_t bwdBytes
    +vector~uint64_t~ byteCounts
  }
  FeatureExtractor "1" *-- "many" FlowStats : агрегирует данные
```
"""

# Рис. 6. Диаграмма класса ModelInferencer
fig6_code = """```mermaid
classDiagram
  class ModelInferencer {
    -unique_ptr~Ort--Env~ env_
    -unique_ptr~Ort--Session~ session_
    -shared_mutex mutex_
    -string modelName_
    -string modelPath_
    +loadModel(modelPath, ep) bool
    +predict(features) pair~int, float~
    +hotSwapModel(newPath, ep) bool
    +unloadModel() void
    +isLoaded() bool
  }
  class ONNX_Runtime {
    <<Engine>>
  }
  ModelInferencer ..> ONNX_Runtime : инкапсулирует API
```
"""

# Insertion logic
text = text.replace('Рис. 4. Диаграмма класса TrafficMonitor', fig4_code + '\n<p align="center">Рис. 4. Диаграмма класса TrafficMonitor</p>')
text = text.replace('Рис. 5. Диаграмма класса FeatureExtractor', fig5_code + '\n<p align="center">Рис. 5. Диаграмма класса FeatureExtractor</p>')
text = text.replace('Рис. 6. Диаграмма класса ModelInferencer', fig6_code + '\n<p align="center">Рис. 6. Диаграмма класса ModelInferencer</p>')

with open('docs/academic/ВКР_черновик_claude.md', 'w', encoding='utf-8') as f:
    f.write(text)

print("UML diagrams (Figures 4, 5, 6) added to the document.")
