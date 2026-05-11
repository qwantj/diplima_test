#include "test_ModelInferencer.hpp"
#include "ml/ModelInferencer.hpp"
#include "common/AppLogger.hpp"

#include <QTemporaryFile>
#include <QTextStream>

void TestModelInferencer::initTestCase() {
    // Initialize AppLogger for the test environment to avoid null pointer dereference
    AppLogger::init();
}

void TestModelInferencer::cleanupTestCase() {
    // Optional cleanup
}

void TestModelInferencer::testLoadNonExistentModel() {
    ModelInferencer inferencer;

    // Path that definitely does not exist
    std::string badPath = "/tmp/non_existent_model_12345.onnx";

    bool result = inferencer.loadModel(badPath);
    QCOMPARE(result, false);
    QCOMPARE(inferencer.isLoaded(), false);

    // Predict should safely return {0, 0.0f} when not loaded
    auto prediction = inferencer.predict({1.0f, 2.0f, 3.0f});
    QCOMPARE(prediction.first, 0);
    QCOMPARE(prediction.second, 0.0f);
}

void TestModelInferencer::testLoadInvalidModel() {
    ModelInferencer inferencer;

    // Create an invalid ONNX model file (just text)
    QTemporaryFile invalidModelFile;
    QVERIFY(invalidModelFile.open());
    QTextStream out(&invalidModelFile);
    out << "This is definitely not a valid ONNX model data.";
    invalidModelFile.close();

    std::string invalidPath = invalidModelFile.fileName().toStdString();

    // Should trigger Ort::Exception internally and return false
    bool result = inferencer.loadModel(invalidPath);

#ifdef ONNX_RUNTIME_AVAILABLE
    // This specifically tests the catch block if ONNX is available
    QCOMPARE(result, false);
    QCOMPARE(inferencer.isLoaded(), false);

    auto prediction = inferencer.predict({1.0f, 2.0f, 3.0f});
    QCOMPARE(prediction.first, 0);
    QCOMPARE(prediction.second, 0.0f);
#else
    // Even without ONNX, it should gracefully fail and return false
    QCOMPARE(result, false);
    QCOMPARE(inferencer.isLoaded(), false);
#endif
}
