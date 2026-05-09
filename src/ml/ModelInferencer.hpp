#pragma once

#include <string>
#include <vector>
#include <memory>
#include <shared_mutex>

#ifdef ONNX_RUNTIME_AVAILABLE
#include <onnxruntime_cxx_api.h>
#endif

class ModelInferencer {
public:
    ModelInferencer();
    ~ModelInferencer();

    bool loadModel(const std::string& modelPath, const std::string& ep = "cpu");
    void unloadModel();
    bool isLoaded() const;

    // Predict: returns {label, confidence}
    std::pair<int, float> predict(const std::vector<float>& features);

    std::string modelName() const;
    std::string modelPath() const;

    // Hot-swap: thread-safe model replacement
    bool hotSwapModel(const std::string& newModelPath, const std::string& ep = "cpu");

private:
    std::string modelPath_;
    std::string modelName_;
    mutable std::shared_mutex mutex_;

#ifdef ONNX_RUNTIME_AVAILABLE
    std::unique_ptr<Ort::Env> env_;
    std::unique_ptr<Ort::Session> session_;
    std::unique_ptr<Ort::SessionOptions> sessionOptions_;

    std::vector<std::string> inputNames_;
    std::vector<std::string> outputNames_;
    std::vector<const char*> inputNamePtrs_;
    std::vector<const char*> outputNamePtrs_;
#endif
};
