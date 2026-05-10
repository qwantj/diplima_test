# Task Checklist
- [x] Inspect `DetectionEngine::handlePacket` again. Why is UI freezing even with rate limits?
- [x] Check if `m_detectionCallback` is being called for *every* packet during the attack instead of just once per window.
- [x] Identify if `tcpServer.broadcastStats` is flooding the network buffer because it's called 150,000 times a second.
- [x] Apply a rate limit or windowing logic to `m_detectionCallback` so it only fires ONCE per analysis window (every 2 seconds).
- [x] Recompile and verify.
