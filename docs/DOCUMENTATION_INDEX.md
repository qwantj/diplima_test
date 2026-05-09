# Индекс документации

  

Дата актуализации: 08.04.2026

  

## 1. Основные документы

  

| Документ | Назначение |

|---|---|

| README.md | Общее описание комплекса, сборка, запуск |

| QUICK_START.md | Минимальный сценарий старта |

| TROUBLESHOOTING.md | Диагностика и исправление ошибок |

| docs/DOCUMENTATION.md | Полная техническая документация |

| docs/Codebase.md | Структура исходного кода |

| docs/FeatureExtractionAlgorithm.md | Формализация алгоритма признаков |

| docs/diagrams.md | Диаграммы для ПЗ/презентации |

| PZ_SUMMARY_RU.md | Сводный документ для пояснительной записки |

  

## 2. Дополнительные аналитические документы

  

| Документ | Назначение |

|---|---|

| RECOMMENDATIONS.md | Направления развития комплекса |

| implementation_plan.md | План работ и статус реализации |

| Refactoring DDoS Detector Project.md | План структурного рефакторинга |

| MLP Model DDoS Detection Issue.md | Разбор различий RF и MLP на реальном трафике |

| diagrams.md | Краткий вход в графические материалы |

  

## 3. Куда идти по задачам

  

- Нужно быстро запустить проект: QUICK_START.md

- Нужны CLI-аргументы collector: README.md

- Падает сборка CMake/Qt: TROUBLESHOOTING.md

- Нужно объяснить архитектуру на защите: docs/DOCUMENTATION.md

- Нужно показать структуру исходников: docs/Codebase.md

- Нужно объяснить фичи и ML-пайплайн: docs/FeatureExtractionAlgorithm.md

- Нужны Mermaid-диаграммы: docs/diagrams.md

- Нужен единый документ для ПЗ: PZ_SUMMARY_RU.md

  

## 4. Актуальные технические константы

  

- TCP-порт collector по умолчанию: 50050

- Размер окна инференса: 2 секунды

- MAX_QUEUE_SIZE: 500000

- Интервал flush в БД: 5000 мс

- БД по умолчанию: ddos_detection_db