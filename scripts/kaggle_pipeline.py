"""
Скрипт для обучения моделей обнаружения DDoS-атак в среде Kaggle на реальном датасете.

Обучает 3 архитектуры: Random Forest, MLP (нейросеть) и XGBoost.
Выводит подробные метрики: Precision, Recall, F1, ROC AUC, Матрицу ошибок.
Генерирует график сравнения ROC-кривых (Рис. 12).

Инструкция:
1. Создайте новый Notebook в Kaggle.
2. Добавьте датасет: kristianfrossos/cicddos2019
3. Убедитесь, что Internet в Settings -> On.
4. В первую ячейку:
   !pip install -q scikit-learn onnx onnxmltools skl2onnx pandas numpy xgboost matplotlib seaborn
5. Во вторую ячейку скопируйте этот код.
"""

import os
import json
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
from sklearn.ensemble import RandomForestClassifier
from sklearn.neural_network import MLPClassifier
from xgboost import XGBClassifier
from sklearn.preprocessing import StandardScaler
from sklearn.model_selection import train_test_split
from sklearn.metrics import classification_report, roc_auc_score, confusion_matrix, roc_curve
import onnx
from skl2onnx import convert_sklearn
from skl2onnx.common.data_types import FloatTensorType
import onnxmltools

# Список признаков в порядке, ожидаемом C++ коллектором
FEATURES = [
    "flow_duration",       # 0
    "total_fwd_packets",   # 1
    "total_bwd_packets",   # 2
    "total_len_fwd",       # 3
    "total_len_bwd",       # 4
    "fwd_mean",            # 5
    "bwd_mean",            # 6
    "flow_pps",            # 7
    "syn_packets",         # 8 
    "ack_packets",         # 9
    "fin_packets",         # 10
    "rst_packets",         # 11
    "psh_packets",         # 12
    "urg_packets",         # 13
    "entropy",             # 14
    "avg_window_size"      # 15
]

def load_real_dataset(base_path, sample_per_file=25000):
    print(f"Поиск CSV файлов в {base_path}...")
    all_files = []
    for root, dirs, files in os.walk(base_path):
        for file in files:
            if file.endswith(".csv"):
                all_files.append(os.path.join(root, file))
    
    print(f"Найдено {len(all_files)} файлов. Загрузка данных...")
    dfs = []
    for f in all_files:
        try:
            print(f"Чтение {os.path.basename(f)}...")
            temp_df = pd.read_csv(f, nrows=sample_per_file, low_memory=False)
            temp_df.columns = temp_df.columns.str.strip()
            
            mapping = {
                'Flow Duration': 'flow_duration',
                'Total Fwd Packets': 'total_fwd_packets',
                'Total Backward Packets': 'total_bwd_packets',
                'Total Length of Fwd Packets': 'total_len_fwd',
                'Total Length of Bwd Packets': 'total_len_bwd',
                'Fwd Packet Length Mean': 'fwd_mean',
                'Bwd Packet Length Mean': 'bwd_mean',
                'Flow Packets/s': 'flow_pps',
                'SYN Flag Count': 'syn_packets',
                'ACK Flag Count': 'ack_packets',
                'FIN Flag Count': 'fin_packets',
                'RST Flag Count': 'rst_packets',
                'PSH Flag Count': 'psh_packets',
                'URG Flag Count': 'urg_packets',
                'Init_Win_bytes_forward': 'avg_window_size',
                'Label': 'Label'
            }
            
            available_cols = [c for c in mapping.keys() if c in temp_df.columns]
            subset = temp_df[available_cols].copy()
            subset = subset.rename(columns=mapping)
            if 'entropy' not in subset.columns:
                subset['entropy'] = 0.0
            dfs.append(subset)
        except Exception as e:
            print(f"Ошибка загрузки {f}: {e}")
    
    full_df = pd.concat(dfs, ignore_index=True)
    full_df.replace([np.inf, -np.inf], np.nan, inplace=True)
    full_df.dropna(inplace=True)
    full_df['Label'] = full_df['Label'].apply(lambda x: 0 if 'BENIGN' in str(x).upper() else 1)
    print(f"Итого загружено: {len(full_df)} строк.")
    return full_df

def print_detailed_metrics(model_name, y_true, y_pred, y_prob):
    print(f"\n{'='*40}\n ОТЧЕТ ПО МОДЕЛИ: {model_name}\n{'='*40}")
    print("\n1. Классификационные метрики:")
    print(classification_report(y_true, y_pred, digits=4))
    auc = roc_auc_score(y_true, y_prob)
    print(f"2. ROC AUC Score: {auc:.4f}")
    print("\n3. Матрица ошибок:")
    cm = confusion_matrix(y_true, y_pred)
    print(cm)
    tn, fp, fn, tp = cm.ravel()
    print(f"\nДетализация: TP={tp}, FP={fp}, FN={fn}, TN={tn}")

def save_scaler_params(scaler, feature_names, filename, use_log1p=False):
    params = {"features": feature_names, "mean": scaler.mean_.tolist(), 
              "scale": scaler.scale_.tolist(), "use_log1p": use_log1p}
    with open(filename, 'w', encoding='utf-8') as f:
        json.dump(params, f, indent=4)
    print(f"Сохранен скейлер: {filename}")

def export_model_to_onnx(model, num_features, filename, is_xgboost=False):
    if is_xgboost:
        from onnxmltools.convert.common.data_types import FloatTensorType
        initial_type = [('float_input', FloatTensorType([None, num_features]))]
        # Для XGBoost аргумент options не поддерживается
        onnx_model = onnxmltools.convert_xgboost(model, initial_types=initial_type)
    else:
        from skl2onnx.common.data_types import FloatTensorType
        initial_type = [('float_input', FloatTensorType([None, num_features]))]
        # Для sklearn отключаем zipmap
        options = {id(model): {'zipmap': False}}
        onnx_model = convert_sklearn(model, initial_types=initial_type, options=options)
    
    with open(filename, "wb") as f:
        f.write(onnx_model.SerializeToString())
    print(f"Сохранена модель: {filename}")

def main():
    # 1. Загрузка данных
    DATASET_PATH = "/kaggle/input/datasets/kristianfrossos/cicddos2019"
    if not os.path.exists(DATASET_PATH):
        DATASET_PATH = "/kaggle/input/cicddos2019"
    df = load_real_dataset(DATASET_PATH)
    X = df[FEATURES].values
    y = df['Label'].values
    X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42, stratify=y)

    # ==========================================
    # 1. RANDOM FOREST
    # ==========================================
    print("\n--- [1/4] Обучение Random Forest ---")
    rf_scaler = StandardScaler()
    X_train_rf = rf_scaler.fit_transform(X_train)
    X_test_rf = rf_scaler.transform(X_test)
    rf_model = RandomForestClassifier(n_estimators=100, max_depth=15, random_state=42, n_jobs=-1)
    rf_model.fit(X_train_rf, y_train)
    rf_pred = rf_model.predict(X_test_rf)
    rf_prob = rf_model.predict_proba(X_test_rf)[:, 1]
    print_detailed_metrics("Random Forest", y_test, rf_pred, rf_prob)
    save_scaler_params(rf_scaler, FEATURES, "rf_scaler_params.json")
    export_model_to_onnx(rf_model, len(FEATURES), "rf_model.onnx")

    # ==========================================
    # 2. XGBOOST
    # ==========================================
    print("\n--- [2/4] Обучение XGBoost ---")
    xgb_model = XGBClassifier(n_estimators=200, learning_rate=0.1, max_depth=6, random_state=42, n_jobs=-1)
    xgb_model.fit(X_train_rf, y_train)
    xgb_pred = xgb_model.predict(X_test_rf)
    xgb_prob = xgb_model.predict_proba(X_test_rf)[:, 1]
    print_detailed_metrics("XGBoost", y_test, xgb_pred, xgb_prob)
    save_scaler_params(rf_scaler, FEATURES, "xgb_scaler_params.json")
    export_model_to_onnx(xgb_model, len(FEATURES), "xgb_model.onnx", is_xgboost=True)

    # ==========================================
    # 3. MLP (Нейросеть)
    # ==========================================
    print("\n--- [3/4] Обучение MLP (Нейросеть) ---")
    X_train_log = np.log1p(np.abs(X_train))
    X_test_log = np.log1p(np.abs(X_test))
    mlp_scaler = StandardScaler()
    X_train_mlp = mlp_scaler.fit_transform(X_train_log)
    X_test_mlp = mlp_scaler.transform(X_test_log)
    mlp_model = MLPClassifier(hidden_layer_sizes=(64, 32), max_iter=200, random_state=42, early_stopping=True)
    mlp_model.fit(X_train_mlp, y_train)
    mlp_pred = mlp_model.predict(X_test_mlp)
    mlp_prob = mlp_model.predict_proba(X_test_mlp)[:, 1]
    print_detailed_metrics("MLP Neural Network", y_test, mlp_pred, mlp_prob)
    save_scaler_params(mlp_scaler, FEATURES, "mlp_scaler_params.json", use_log1p=True)
    export_model_to_onnx(mlp_model, len(FEATURES), "mlp_model.onnx")

    # ==========================================
    # 4. ПОСТРОЕНИЕ ROC-КРИВЫХ (Рис. 12)
    # ==========================================
    print("\n--- [4/4] Генерация графика ROC-кривых ---")
    plt.figure(figsize=(10, 8))
    
    for prob, name in [(rf_prob, 'Random Forest'), (xgb_prob, 'XGBoost'), (mlp_prob, 'MLP Neural Network')]:
        fpr, tpr, _ = roc_curve(y_test, prob)
        auc = roc_auc_score(y_test, prob)
        plt.plot(fpr, tpr, label=f'{name} (AUC = {auc:.4f})', linewidth=2)
    
    plt.plot([0, 1], [0, 1], 'k--', linewidth=1, label='Random Guessing')
    plt.xlim([0.0, 1.0]); plt.ylim([0.0, 1.05])
    plt.xlabel('False Positive Rate', fontsize=12); plt.ylabel('True Positive Rate', fontsize=12)
    plt.title('Рис. 12. ROC-кривые классификаторов трафика', fontsize=14, pad=15)
    plt.legend(loc="lower right"); plt.grid(True, linestyle='--', alpha=0.6)
    
    plt.savefig("roc_curves.png", dpi=300, bbox_inches='tight')
    plt.show()

    print("\nВсе модели обучены! Скачайте файлы и roc_curves.png из вкладки Output.")

if __name__ == "__main__":
    main()
