
import re

def format_python_gost(code):
  code = code.replace('\t', '  ')
  lines = code.splitlines()
  new_lines = []
  for line in lines:
    if not line.strip():
      new_lines.append('')
      continue
    leading_spaces = len(line) - len(line.lstrip(' '))
    if leading_spaces > 0 and leading_spaces % 4 == 0:
      line = ' ' * (leading_spaces // 2) + line.lstrip(' ')
    new_lines.append(line)
  return '\n'.join(new_lines)

appendix_d_code = """import os
import json
import numpy as np
import pandas as pd
from sklearn.ensemble import RandomForestClassifier
from sklearn.neural_network import MLPClassifier
from xgboost import XGBClassifier
from sklearn.preprocessing import StandardScaler
from sklearn.model_selection import train_test_split
from sklearn.metrics import classification_report, roc_auc_score, confusion_matrix
import onnx
from skl2onnx import convert_sklearn
from skl2onnx.common.data_types import FloatTensorType
import onnxmltools

# ==============================================================================
# Файл: train_model.py
# Описание: Скрипт обучения и экспорта моделей обнаружения DoS-атак.
# Реализует пайплайн предобработки данных CIC-DDoS2019 и конвертацию в ONNX.
# ==============================================================================

# Признаки трафика (согласно FeatureExtractor.cpp)
FEATURES = [
  "flow_duration", "total_fwd_packets", "total_bwd_packets",
  "total_len_fwd", "total_len_bwd", "fwd_mean", "bwd_mean",
  "flow_pps", "syn_packets", "ack_packets", "fin_packets",
  "rst_packets", "psh_packets", "urg_packets", "entropy", "avg_window_size"
]

def load_data(path):
  \"\"\"Загрузка CSV-файлов датасета CIC-DDoS2019\"\"\"
  df = pd.read_csv(path, low_memory=False)
  df.columns = df.columns.str.strip()
  # Очистка бесконечных и пропущенных значений
  df.replace([np.inf, -np.inf], np.nan, inplace=True)
  df.dropna(inplace=True)
  # Бинаризация меток: BENIGN -> 0, Остальное -> 1
  df['Label'] = df['Label'].apply(lambda x: 0 if 'BENIGN' in str(x).upper() else 1)
  return df

def save_scaler(scaler, filename, use_log1p=False):
  \"\"\"Сохранение параметров нормализации в формате JSON для C++ ядра\"\"\"
  params = {
    "features": FEATURES,
    "mean": scaler.mean_.tolist(),
    "scale": scaler.scale_.tolist(),
    "use_log1p": use_log1p
  }
  with open(filename, 'w', encoding='utf-8') as f:
    json.dump(params, f, indent=4)

def export_onnx(model, num_features, filename, is_xgb=False):
  \"\"\"Конвертация обученной модели в универсальный формат ONNX\"\"\"
  initial_type = [('float_input', FloatTensorType([None, num_features]))]
  if is_xgb:
    onnx_model = onnxmltools.convert_xgboost(model, initial_types=initial_type)
  else:
    # Отключение ZipMap для получения прямого тензора вероятностей
    options = {id(model): {'zipmap': False}}
    onnx_model = convert_sklearn(model, initial_types=initial_type, options=options)
  
  with open(filename, "wb") as f:
    f.write(onnx_model.SerializeToString())

def main():
  # Загрузка и разделение выборки (80/20)
  df = load_data("cicddos2019_sample.csv")
  X = df[FEATURES].values
  y = df['Label'].values
  X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, stratify=y)

  # Обучение XGBoost (основная модель)
  scaler = StandardScaler()
  X_train_s = scaler.fit_transform(X_train)
  model = XGBClassifier(n_estimators=200, learning_rate=0.1, max_depth=6)
  model.fit(X_train_s, y_train)

  # Сохранение артефактов
  save_scaler(scaler, "xgb_scaler_params.json")
  export_onnx(model, len(FEATURES), "xgb_model.onnx", is_xgb=True)
  
  # Вывод итоговых метрик
  y_pred = model.predict(scaler.transform(X_test))
  print(classification_report(y_test, y_pred))

if __name__ == "__main__":
  main()
"""

formatted_code = format_python_gost(appendix_d_code)

with open('docs/academic/ВКР_черновик_claude.md', 'r', encoding='utf-8') as f:
  vkr = f.read()

# Replace the previous block if it exists
match_d = re.search(r'## ПРИЛОЖЕНИЕ Д.*?(?=### Приложение Е|## ПРИЛОЖЕНИЕ Е|---)', vkr, re.DOTALL | re.IGNORECASE)
app_d_content = f"## ПРИЛОЖЕНИЕ Д\n\nСкрипт обучения модели XGBoost/ONNX (Python)\n\n```python\n{formatted_code}\n```\n\n"

if match_d:
    vkr = vkr[:match_d.start()] + app_d_content + vkr[match_d.end():]
else:
    # Fallback to replace the previous placeholder if still there
    vkr = vkr.replace('### Приложение Д. Скрипт обучения модели XGBoost/ONNX (Python)\n\n*(Python-скрипт scripts/train_model.py с комментариями)*', app_d_content)

with open('docs/academic/ВКР_черновик_claude.md', 'w', encoding='utf-8') as f:
  f.write(vkr)

print("Appendix D has been polished and updated.")
