"""
Скрипт для обучения моделей обнаружения DDoS-атак в среде Kaggle.

Инструкция:
1. Создайте новый Notebook в Kaggle.
2. Скопируйте весь код из этого файла в ячейку Kaggle.
3. Убедитесь, что установлены необходимые библиотеки (обычно они уже есть в Kaggle):
   !pip install scikit-learn onnx onnxmltools skl2onnx
4. Запустите ячейку. Скрипт сгенерирует синтетический датасет, обучит модели (Random Forest и MLP), 
   и сохранит .onnx модели и scaler_params.json в текущую директорию (папка /kaggle/working/).
5. Скачайте полученные 4 файла и поместите их в папку `models/` вашего C++ проекта.

ВНИМАНИЕ: Скрипт генерирует синтетические данные. Для реального диплома вы можете 
заменить функцию `generate_dataset()` на код загрузки реального CSV-файла (например, CIC-DDoS2019),
но обязательно сохраните названия и порядок 16 фичей, которые собирает C++ коллектор!
"""

import os
import json
import numpy as np
import pandas as pd
from sklearn.ensemble import RandomForestClassifier
from sklearn.neural_network import MLPClassifier
from sklearn.preprocessing import StandardScaler
from sklearn.model_selection import train_test_split
from sklearn.metrics import classification_report
import onnx
from skl2onnx import convert_sklearn
from skl2onnx.common.data_types import FloatTensorType

# Список признаков, в точности совпадающий с порядком в FeatureExtractor.cpp
FEATURES = [
    "flow_duration",       # 0
    "total_fwd_packets",   # 1
    "total_bwd_packets",   # 2
    "total_len_fwd",       # 3
    "total_len_bwd",       # 4
    "fwd_mean",            # 5
    "bwd_mean",            # 6
    "flow_pps",            # 7
    "syn_packets",         # 8 (Новые расширенные признаки)
    "ack_packets",         # 9
    "fin_packets",         # 10
    "rst_packets",         # 11
    "psh_packets",         # 12
    "urg_packets",         # 13
    "entropy",             # 14 (Энтропия payload'а)
    "avg_window_size"      # 15
]

def generate_synthetic_dataset(n_samples=10000):
    """
    Генерирует синтетический датасет для демонстрации пайплайна.
    Label: 0 = Benign (Нормальный трафик), 1 = Attack (DDoS)
    """
    print(f"Генерация синтетического датасета ({n_samples} записей)...")
    np.random.seed(42)
    data = []
    labels = []

    for _ in range(n_samples):
        is_attack = np.random.rand() > 0.5
        
        if not is_attack:
            # Нормальный HTTP/HTTPS трафик
            duration = np.random.uniform(100000, 5000000) # микросекунды
            fwd_pkts = np.random.randint(5, 50)
            bwd_pkts = np.random.randint(5, 50)
            len_fwd = fwd_pkts * np.random.uniform(64, 200)
            len_bwd = bwd_pkts * np.random.uniform(500, 1500)
            fwd_mean = len_fwd / fwd_pkts
            bwd_mean = len_bwd / bwd_pkts
            pps = (fwd_pkts + bwd_pkts) / (duration / 1e6)
            
            syn = 1
            ack = fwd_pkts + bwd_pkts - 2
            fin = 1
            rst = 0
            psh = np.random.randint(0, 10)
            urg = 0
            entropy = np.random.uniform(6.0, 7.8) # Сжатые данные / HTML
            win_size = np.random.uniform(10000, 65535)
            
            label = 0
        else:
            attack_type = np.random.choice(["SYN_FLOOD", "UDP_FLOOD"])
            if attack_type == "SYN_FLOOD":
                duration = np.random.uniform(1000, 50000)
                fwd_pkts = np.random.randint(100, 1000)
                bwd_pkts = 0
                len_fwd = fwd_pkts * 64
                len_bwd = 0
                fwd_mean = 64
                bwd_mean = 0
                pps = fwd_pkts / (duration / 1e6)
                
                syn = fwd_pkts
                ack = 0
                fin = 0
                rst = 0
                psh = 0
                urg = 0
                entropy = 0.0 # Нет payload'а
                win_size = np.random.uniform(512, 1024)
            else: # UDP FLOOD
                duration = np.random.uniform(1000, 50000)
                fwd_pkts = np.random.randint(200, 2000)
                bwd_pkts = 0
                len_fwd = fwd_pkts * np.random.uniform(512, 1024)
                len_bwd = 0
                fwd_mean = len_fwd / fwd_pkts
                bwd_mean = 0
                pps = fwd_pkts / (duration / 1e6)
                
                syn = ack = fin = rst = psh = urg = 0
                entropy = np.random.uniform(1.0, 3.0) # Повторяющийся мусор
                win_size = 0
            
            label = 1

        row = [duration, fwd_pkts, bwd_pkts, len_fwd, len_bwd, fwd_mean, bwd_mean, pps,
               syn, ack, fin, rst, psh, urg, entropy, win_size]
        data.append(row)
        labels.append(label)

    df = pd.DataFrame(data, columns=FEATURES)
    df['Label'] = labels
    return df

def save_scaler_params(scaler, feature_names, filename, use_log1p=False):
    """
    Сохраняет параметры обученного скейлера в JSON файл,
    который ожидает C++ движок (FeatureExtractor).
    """
    params = {
        "features": feature_names,
        "mean": scaler.mean_.tolist(),
        "scale": scaler.scale_.tolist(),
        "use_log1p": use_log1p
    }
    with open(filename, 'w', encoding='utf-8') as f:
        json.dump(params, f, indent=4)
    print(f"Сохранен файл конфигурации скейлера: {filename}")

def export_onnx_model(model, num_features, filename):
    """
    Экспортирует модель scikit-learn в ONNX формат.
    ВАЖНО: zipmap=False отключает сложный Sequence<Map> вывод в ONNX,
    оставляя чистый FloatTensor для вероятностей, который легко читается в C++.
    """
    initial_type = [('float_input', FloatTensorType([None, num_features]))]
    
    # Настройки для отключения zipmap
    options = {id(model): {'zipmap': False}}
    
    onnx_model = convert_sklearn(model, initial_types=initial_type, options=options)
    
    with open(filename, "wb") as f:
        f.write(onnx_model.SerializeToString())
    print(f"Сохранена ONNX модель: {filename}")


def main():
    # 1. Загрузка данных
    df = generate_synthetic_dataset(15000)
    X = df[FEATURES].values
    y = df['Label'].values

    X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)

    # ==========================================
    # ПАЙПЛАЙН 1: RANDOM FOREST (БЕЗ ЛОГАРИФМИРОВАНИЯ)
    # ==========================================
    print("\n--- Обучение Random Forest ---")
    rf_scaler = StandardScaler()
    X_train_rf = rf_scaler.fit_transform(X_train)
    X_test_rf = rf_scaler.transform(X_test)

    rf_model = RandomForestClassifier(n_estimators=50, max_depth=10, random_state=42)
    rf_model.fit(X_train_rf, y_train)

    print("Random Forest Classification Report:")
    print(classification_report(y_test, rf_model.predict(X_test_rf)))

    save_scaler_params(rf_scaler, FEATURES, "rf_scaler_params.json", use_log1p=False)
    export_onnx_model(rf_model, len(FEATURES), "rf_model.onnx")


    # ==========================================
    # ПАЙПЛАЙН 2: MLP / NEURAL NETWORK (С ЛОГАРИФМИРОВАНИЕМ)
    # ==========================================
    print("\n--- Обучение MLP (Нейросеть) ---")
    # Для нейросетей (особенно при сетевом трафике) очень полезно применять log1p 
    # к сырым значениям (байты, пакеты могут быть огромными), чтобы сжать выбросы.
    X_train_log = np.log1p(X_train)
    X_test_log = np.log1p(X_test)

    mlp_scaler = StandardScaler()
    X_train_mlp = mlp_scaler.fit_transform(X_train_log)
    X_test_mlp = mlp_scaler.transform(X_test_log)

    mlp_model = MLPClassifier(hidden_layer_sizes=(64, 32), max_iter=500, random_state=42, early_stopping=True)
    mlp_model.fit(X_train_mlp, y_train)

    print("MLP Classification Report:")
    print(classification_report(y_test, mlp_model.predict(X_test_mlp)))

    # Указываем use_log1p=True, чтобы C++ код сам сделал log1p перед применением StandardScaler
    save_scaler_params(mlp_scaler, FEATURES, "mlp_scaler_params.json", use_log1p=True)
    export_onnx_model(mlp_model, len(FEATURES), "mlp_model.onnx")

    print("\nВсе модели успешно обучены и экспортированы!")
    print("В текущей папке появились файлы:")
    print("- rf_model.onnx")
    print("- rf_scaler_params.json")
    print("- mlp_model.onnx")
    print("- mlp_scaler_params.json")

if __name__ == "__main__":
    main()
