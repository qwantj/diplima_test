# Task: Final Integration (Stage 4)

- [x] Fix Model Prediction
    - [x] Investigate -1.0 output issue <!-- id: 19 -->
    - [x] Fix ModelInferencer output parsing <!-- id: 20 -->
- [x] Real-Time Pipeline
    - [x] Create `DetectionEngine` class <!-- id: 21 -->
    - [x] Connect TrafficMonitor -> FeatureExtractor -> ModelInferencer <!-- id: 22 -->
    - [x] Add callback for detection results <!-- id: 23 -->
- [x] User Interface
    - [x] Create main detection window <!-- id: 24 -->
    - [x] Add interface selection dropdown <!-- id: 25 -->
    - [x] Add Start/Stop buttons <!-- id: 26 -->
    - [x] Add status display with detection results <!-- id: 27 -->
- [ ] Verification
    - [x] Build and test end-to-end flow <!-- id: 28 -->

- [x] Model Evaluation
    - [x] Analyze dataset and evaluation logic <!-- id: 29 -->
    - [x] Create/Run evaluation script (Accuracy, F1, Inference Speed) <!-- id: 30 -->
    - [x] Document results <!-- id: 31 -->

# Task: Database Integration (Stage 5)
- [ ] Database Setup
    - [ ] Create `DatabaseManager` class <!-- id: 32 -->
    - [ ] Configure connection (PostgreSQL/SQLite) <!-- id: 33 -->
    - [ ] Create `incidents` table schema <!-- id: 34 -->
- [ ] Logic Integration
    - [ ] Connect `DetectionEngine` to DB <!-- id: 35 -->
    - [ ] Save attack events asynchronously <!-- id: 36 -->
- [ ] UI Integration
    - [ ] Add "History" tab to MainWindow <!-- id: 37 -->
    - [ ] Display incidents from DB <!-- id: 38 -->
