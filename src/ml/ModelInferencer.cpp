#include "ml/ModelInferencer.hpp"
#include "common/AppLogger.hpp"
#include <filesystem>

ModelInferencer::ModelInferencer() {
#ifdef ONNX_RUNTIME_AVAILABLE
    env_ = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "DDoSDetector");
#endif
}

ModelInferencer::~ModelInferencer() { unloadModel(); }

std::string ModelInferencer::modelName() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return modelName_;
}

std::string ModelInferencer::modelPath() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return modelPath_;
}

bool ModelInferencer::loadModel(const std::string& modelPath, const std::string& ep) {
#ifdef ONNX_RUNTIME_AVAILABLE
    std::unique_lock<std::shared_mutex> lock(mutex_);
    try {
        sessionOptions_ = std::make_unique<Ort::SessionOptions>();
        sessionOptions_->SetIntraOpNumThreads(1);
        sessionOptions_->SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

        if (ep == "dml") {
            // DirectML provider if available
            // OrtSessionOptionsAppendExecutionProvider_DML(*sessionOptions_, 0);
        }
        // Default: CPU

        std::wstring wpath(modelPath.begin(), modelPath.end());
        session_ = std::make_unique<Ort::Session>(*env_, wpath.c_str(), *sessionOptions_);

        // Get input/output names
        Ort::AllocatorWithDefaultOptions allocator;
        inputNames_.clear();
        outputNames_.clear();
        inputNamePtrs_.clear();
        outputNamePtrs_.clear();

        size_t numInputs = session_->GetInputCount();
        for (size_t i = 0; i < numInputs; i++) {
            auto name = session_->GetInputNameAllocated(i, allocator);
            inputNames_.push_back(name.get());
        }
        size_t numOutputs = session_->GetOutputCount();
        for (size_t i = 0; i < numOutputs; i++) {
            auto name = session_->GetOutputNameAllocated(i, allocator);
            outputNames_.push_back(name.get());
        }

        for (auto& n : inputNames_) inputNamePtrs_.push_back(n.c_str());
        for (auto& n : outputNames_) outputNamePtrs_.push_back(n.c_str());

        modelPath_ = modelPath;
        modelName_ = std::filesystem::path(modelPath).filename().string();

        AppLogger::get()->info("ModelInferencer: loaded '{}' ({} inputs, {} outputs, EP={})",
            modelName_, numInputs, numOutputs, ep);
        return true;
    } catch (const Ort::Exception& e) {
        AppLogger::get()->error("ModelInferencer: ONNX load error: {}", e.what());
        session_.reset();
        return false;
    }
#else
    AppLogger::get()->error("ModelInferencer: ONNX Runtime not available");
    return false;
#endif
}

void ModelInferencer::unloadModel() {
#ifdef ONNX_RUNTIME_AVAILABLE
    std::unique_lock<std::shared_mutex> lock(mutex_);
    session_.reset();
    sessionOptions_.reset();
    inputNames_.clear();
    outputNames_.clear();
    inputNamePtrs_.clear();
    outputNamePtrs_.clear();
#endif
}

bool ModelInferencer::isLoaded() const {
#ifdef ONNX_RUNTIME_AVAILABLE
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return session_ != nullptr;
#else
    return false;
#endif
}

std::pair<int, float> ModelInferencer::predict(const std::vector<float>& features) {
#ifdef ONNX_RUNTIME_AVAILABLE
    std::shared_lock<std::shared_mutex> lock(mutex_);
    if (!session_) return {0, 0.0f};

    try {
        auto memInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        std::vector<int64_t> inputShape = {1, (int64_t)features.size()};
        auto inputTensor = Ort::Value::CreateTensor<float>(
            memInfo, const_cast<float*>(features.data()),
            features.size(), inputShape.data(), inputShape.size());

        // Only request the first output (label) to avoid issues with
        // sklearn map outputs for probabilities
        const char* labelOutputName = outputNamePtrs_[0];
        auto outputs = session_->Run(
            Ort::RunOptions{nullptr},
            inputNamePtrs_.data(), &inputTensor, 1,
            &labelOutputName, 1);

        int label = 0;
        float confidence = 0.0f;

        if (!outputs.empty()) {
            auto& labelTensor = outputs[0];
            // Check if it is a tensor before accessing
            if (labelTensor.IsTensor()) {
                auto typeInfo = labelTensor.GetTensorTypeAndShapeInfo();
                auto elemType = typeInfo.GetElementType();
                if (elemType == ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64) {
                    label = (int)*labelTensor.GetTensorData<int64_t>();
                } else if (elemType == ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT) {
                    float val = *labelTensor.GetTensorData<float>();
                    label = (int)(val > 0.5f);
                    confidence = val;
                }
            }
        }

        // Now try to get probability from second output if available
        if (outputNamePtrs_.size() > 1) {
            try {
                const char* probOutputName = outputNamePtrs_[1];
                auto probOutputs = session_->Run(
                    Ort::RunOptions{nullptr},
                    inputNamePtrs_.data(), &inputTensor, 1,
                    &probOutputName, 1);

                if (!probOutputs.empty() && probOutputs[0].IsTensor()) {
                    auto& pt = probOutputs[0];
                    auto typeInfo = pt.GetTensorTypeAndShapeInfo();
                    auto elemType = typeInfo.GetElementType();
                    if (elemType == ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT) {
                        auto shape = typeInfo.GetShape();
                        size_t count = 1;
                        for (auto s : shape) if (s > 0) count *= s;
                        const float* probs = pt.GetTensorData<float>();
                        confidence = (count >= 2) ? probs[1] : probs[0];
                    }
                } else {
                    // Output is a map/sequence (sklearn RF/DT) — no tensor probs
                    confidence = (label == 1) ? 0.99f : 0.01f;
                }
            } catch (...) {
                // Fallback on any exception
                confidence = (label == 1) ? 0.99f : 0.01f;
            }
        }

        return {label, confidence};
    } catch (const Ort::Exception& e) {
        AppLogger::get()->error("ModelInferencer: inference error: {}", e.what());
        return {0, 0.0f};
    }
#else
    return {0, 0.0f};
#endif
}

bool ModelInferencer::hotSwapModel(const std::string& newModelPath, const std::string& ep) {
    AppLogger::get()->info("ModelInferencer: hot-swapping to '{}'", newModelPath);
    unloadModel();
    return loadModel(newModelPath, ep);
}
