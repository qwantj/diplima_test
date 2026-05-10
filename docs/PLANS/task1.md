# Tasks

- [ ] Stage 2: Packet Capture and Analysis Core <!-- id: 0 -->
    - [x] Configure `CMakeLists.txt` for PcapPlusPlus <!-- id: 1 -->
    - [/] Create `TrafficMonitor` class <!-- id: 2 -->
        - [/] Implement `openFile` for PCAP files <!-- id: 3 -->
        - [/] Implement `startCapture` for live traffic <!-- id: 4 -->
    - [ ] Create `FeatureExtractor` class <!-- id: 5 -->
        - [ ] Implement flow aggregation/time windows <!-- id: 6 -->
        - [ ] Implement feature calculation (packets, bytes, flags) <!-- id: 7 -->
        - [ ] Implement Scaler (using params from Stage 1) <!-- id: 8 -->
