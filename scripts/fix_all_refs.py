import re

def fix_references_and_bibliography(file_path):
    with open(file_path, 'r', encoding='utf-8') as f:
        text = f.read()

    # 1. Replace the entire bibliography section
    bib_start_marker = "## СПИСОК ИСПОЛЬЗОВАННОЙ ЛИТЕРАТУРЫ"
    bib_end_marker = "---" # Assumes there's a --- after the bib and before Appendices start
    
    # Let's find the start and end of the bib section more robustly
    start_idx = text.find(bib_start_marker)
    if start_idx == -1:
        print("Could not find bibliography section.")
        return

    # Find where definitions/main text ends and bib starts
    # Based on the file structure, Appendices follow.
    end_idx = text.find("<p align=\"center\"><b>ПРИЛОЖЕНИЯ</b></p>", start_idx)
    if end_idx == -1:
         # Try another marker
         end_idx = text.find("---", start_idx + len(bib_start_marker))

    user_bib = """## СПИСОК ИСПОЛЬЗОВАННОЙ ЛИТЕРАТУРЫ

*(Оформление по ГОСТ Р 7.0.100–2018)*

1) Alharby, M. Evaluating machine learning approaches for multiple attack classification with improved computational efficiency in IoT networks / M. Alharby. — Текст : электронный // Scientific Reports. — 2025. — Vol. 15. — P. 39914. — URL: https://pmc.ncbi.nlm.nih.gov/articles/PMC12618690/ (дата обращения: 08.04.2026).

2) Antonakakis, M. Understanding the Mirai Botnet / M. Antonakakis, T. April, M. Bailey [и др.]. — Текст : электронный // 26th USENIX Security Symposium. — 2017. — P. 1093–1110. — URL: https://www.usenix.org/conference/usenixsecurity17/technical-sessions/presentation/antonakakis (дата обращения: 08.04.2026).

3) CISA. UDP-Based Amplification Attacks / Cybersecurity and Infrastructure Security Agency. — Текст : электронный. — URL: https://www.cisa.gov/news-events/cybersecurity-advisories/ta14-017a (дата обращения: 08.04.2026).

4) Chen, T. XGBoost: A Scalable Tree Boosting System / T. Chen, C. Guestrin. — Текст : электронный // Proceedings of the 22nd ACM SIGKDD International Conference on Knowledge Discovery and Data Mining. — ACM, 2016. — P. 785–794. — URL: https://dl.acm.org/doi/10.1145/2939672.2939785 (дата обращения: 08.04.2026).

5) Cloudflare. 2025 Q2 DDoS threat report: Hyper-volumetric DDoS attacks skyrocket / Cloudflare. — Текст : электронный // Cloudflare Blog. — 2025. — URL: https://blog.cloudflare.com/ddos-threat-report-for-2025-q2/ (дата обращения: 08.04.2026).

6) Cloudflare. 2025 Q3 DDoS threat report: including Aisuru, the apex of botnets / Cloudflare. — Текст : электронный // Cloudflare Blog. — 2025. — URL: https://blog.cloudflare.com/ddos-threat-report-2025-q3/ (дата обращения: 08.04.2026).

7) Cloudflare. 2025 Q4 DDoS threat report: A record-setting 31.4 Tbps attack caps a year of massive DDoS assaults / Cloudflare. — Текст : электронный // Cloudflare Blog. — 2025. — URL: https://blog.cloudflare.com/ddos-threat-report-2025-q4/ (дата обращения: 08.04.2026).

8) FastNetMon. FastNetMon Advanced documentation pack / FastNetMon. — Текст : электронный. — URL: https://fastnetmon.com/ (дата обращения: 08.04.2026).

9) Hansen, R. Slowloris HTTP DoS / R. Hansen. — Текст : электронный. — URL: https://web.archive.org/web/20120426002446/http://ha.ckers.org/slowloris/ (дата обращения: 08.04.2026).

10) Irofti, P. Detecting and Mitigating DDoS Attacks with AI: A Survey / P. Irofti [и др.]. — Текст : электронный // arXiv. — 2026. — URL: https://arxiv.org/abs/2503.17867 (дата обращения: 08.04.2026).

11) ISO/IEC 14882:2020 [11]. Programming languages — C++ (Standard C++20) / ISO/IEC. — Текст : непосредственный. — Geneva : ISO, 2020. — 1853 p.

12) MDPI. A Comparative Evaluation of Snort and Suricata for Detecting Data Exfiltration Tunnels in Cloud Environments / MDPI. — Текст : электронный // Applied Sciences. — 2024. — Vol. 14, No. 6. — P. 2489. — URL: https://www.mdpi.com/2076-3417/14/6/2489 (дата обращения: 08.04.2026).

13) Mhamdi, L. A Deep Learning Approach Combining Auto-encoder with One-class SVM for DDoS Attack Detection in SDNs / L. Mhamdi, D. McLernon, F. El-moussa [и др.]. — Текст : электронный // 2020 IEEE 8th International Conference on Communications and Networking (ComNet). — IEEE, 2021. — P. 1–8. — URL: https://ieeexplore.ieee.org/document/9366031/ (дата обращения: 08.04.2026).

14) Moodycamel. A Fast Lock-Free Queue for C++ / Moodycamel. — Текст : электронный. — 2013. — URL: https://moodycamel.com/blog/2013/a-fast-lock-free-queue-for-c++ (дата обращения: 08.04.2026).

15) ONNX Runtime. ONNX Runtime Architecture / ONNX Runtime. — Текст : электронный. — URL: https://onnxruntime.ai/docs/reference/high-level-design.html (дата обращения: 08.04.2026).

16) ONNX Runtime. Session Management / ONNX Runtime. — Текст : электронный. — URL: https://onnxruntime.ai/docs/ (дата обращения: 08.04.2026).

17) Paessler AG. PRTG Manual: System Requirements and Performance Considerations / Paessler AG. — Текст : электронный. — 2025. — URL: https://www.paessler.com/manuals/ (дата обращения: 08.04.2026).

18) Patil, V. T. Deep Learning-Driven IoT Defence: Comparative Analysis of CNN and LSTM for DDoS Detection and Mitigation / V. T. Patil, S. S. Deore. — Текст : электронный // Journal of Information Systems Engineering & Management. — 2025. — Vol. 10, No. 8. — P. 8–21. — URL: https://jisem.org/article/view/deep-learning-driven-iot-defence-comparative-analysis-of-cnn-and-lstm-for-ddos-detection-and-mitigation (дата обращения: 08.04.2026).

19) PcapPlusPlus. Feature Overview and DPDK Support / PcapPlusPlus. — Текст : электронный. — URL: https://pcapplusplus.github.io/ (дата обращения: 08.04.2026).

20) PostgreSQL Global Development Group. PostgreSQL Documentation. COPY / PostgreSQL Global Development Group. — Текст : электронный. — 2026. — URL: https://www.postgresql.org/docs/current/sql-copy.html (дата обращения: 08.04.2026).

21) Preshing, J. An Introduction to Lock-Free Programming / J. Preshing. — Текст : электронный. — 2012. — URL: https://preshing.com/20120612/an-introduction-to-lock-free-programming/ (дата обращения: 08.04.2026).

23) Qt Group. Signals & Slots | Qt Core | Qt 6 / Qt Group. — Текст : электронный. — URL: https://doc.qt.io/qt-6/signalsandslots.html (дата обращения: 08.04.2026).

24) Sharafaldin, I. Toward Generating a New Intrusion Detection Dataset and Intrusion Traffic Characterization / I. Sharafaldin, A. H. Lashkari, A. A. Ghorbani. — Текст : электронный // Proceedings of the 4th International Conference on Information Systems Security and Privacy (ICISSP). — 2018. — P. 108–116. — URL: https://www.scitepress.org/Papers/2018/66398/66398.pdf (дата обращения: 08.04.2026).

25) University of New Brunswick. DDoS evaluation dataset (CIC-DDoS2019) / UNB. — Текст : электронный. — URL: https://www.unb.ca/cic/datasets/ddos-2019.html (дата обращения: 08.04.2026).

26) Tellez, A. Analyzing BotNets with Suricata & Machine Learning / A. Tellez. — Текст : электронный // Splunk Blog. — 2017. — URL: https://www.splunk.com/en_us/blog/security/analyzing-botnets-with-suricata-machine-learning.html (дата обращения: 08.04.2026).

27) University of Portsmouth. A Comparative Analysis of Snort 3 and Suricata / University of Portsmouth. — Текст : электронный. — URL: https://pure.port.ac.uk/ (дата обращения: 08.04.2026).

28) Лукацкий, А. В. Обнаружение атак / А. В. Лукацкий. — Санкт-Петербург : БХВ-Петербург, 2003. — 624 с. — Текст : непосредственный.

29) ГОСТ Р 50922-2006. Защита информации. Основные термины и определения. — Москва : Стандартинформ, 2008. — 11 с. — Текст : непосредственный.

30) ГОСТ Р ИСО/МЭК 27001-2021. Информационные технологии. Методы и средства обеспечения безопасности. Системы управления информационной безопасностью. Требования. — Москва : Стандартинформ, 2022. — Текст : непосредственный.

31) ГОСТ 19.301-79. Единая система программной документации. Программа и методика испытаний. Требования к содержанию и оформлению. — Москва : Стандартинформ, 2010. — Текст : непосредственный.

32) ГОСТ 19.401-78. Единая система программной документации. Текст программы. Требования к содержанию и оформлению. — Москва : Стандартинформ, 2010. — Текст : непосредственный.

33) IEEE 830-1998. Recommended Practice for Software Requirements Specifications. — IEEE, 1998. — Текст : непосредственный.

34) DPDK: Data Plane Development Kit / DPDK Project. — Текст : электронный. — URL: https://www.dpdk.org/ (дата обращения: 08.04.2026).

35) Олифер, В. Г. Компьютерные сети. Принципы, технологии, протоколы / В. Г. Олифер, Н. А. Олифер. — 5-е изд. — Санкт-Петербург : Питер, 2016. — 992 с. — Текст : непосредственный.

36) Воронцов, К. В. Машинное обучение: курс лекций / К. В. Воронцов. — Москва : МГУ, 2020. — Текст : электронный. — URL: http://www.machinelearning.ru (дата обращения: 08.04.2026).

37) IEEE 610.12-1990. IEEE Standard Glossary of Software Engineering Terminology. — IEEE, 1990. — Текст : непосредственный.

38) Дейт, К. Дж. Введение в системы баз данных / К. Дж. Дейт. — 8-е изд. — Москва : Вильямс, 2005. — 1328 с. — Текст : непосредственный.

39) libpqxx: The official C++ client API for PostgreSQL / libpqxx. — Текст : электронный. — URL: https://pqxx.org/development/libpqxx/ (дата обращения: 08.04.2026).
"""

    if start_idx != -1 and end_idx != -1:
        text = text[:start_idx] + user_bib + "\n\n" + text[end_idx:]

    # 2. Update Definitions section with correct references
    # We'll use specific regex to catch the definitions and their current incorrect [21]
    
    mapping = {
        r'5\) \*\*Признак трафика \(feature\)\*\* — .*? \[21\]\.': '5) **Признак трафика (feature)** — числовая характеристика пакета или потока, используемая моделью машинного или глубокого обучения для классификации [36].',
        r'6\) \*\*Классификатор\*\* — .*? \[21\]\.': '6) **Классификатор** — алгоритм машинного и глубокого обучения, относящий объект к одному из заданных классов («атака» или «норма») [36].',
        r'9\) \*\*SRS \(Software Requirements Specification\)\*\* — .*? \[21\]\.': '9) **SRS (Software Requirements Specification)** — спецификация требований к программному обеспечению; документ, фиксирующий функциональные и нефункциональные требования к системе (стандарт IEEE 830) [33].',
        r'10\) \*\*Функциональное требование \(FR\)\*\* — .*? \[21\]\.': '10) **Функциональное требование (FR)** — требование, описывающее конкретное действие или поведение системы [33].',
        r'11\) \*\*Нефункциональное требование \(NFR\)\*\* — .*? \[21\]\.': '11) **Нефункциональное требование (NFR)** — требование, описывающее характеристику работы системы: производительность, надёжность, безопасность, совместимость [33].',
        r'12\) \*\*Критерий приёмки \(acceptance criterion\)\*\* — .*? \[21\]\.': '12) **Критерий приёмки (acceptance criterion)** — измеримое условие, при выполнении которого требование считается реализованным корректно [37].',
        r'14\) \*\*Окно агрегации \(inference window\)\*\* — .*? \[21\]\.': '14) **Окно агрегации (inference window)** — временной интервал (1 секунда в данном комплексе), за который собираются пакеты и вычисляются признаки перед вызовом классификатора [24].',
        r'15\) \*\*РСУБД\*\* \(RDBMS\) — .*? \[21\]\.': '15) **РСУБД** (RDBMS) — реляционная система управления базами данных; программное обеспечение для управления данными в виде связанных таблиц [38].',
        r'16\) \*\*libpqxx\*\* — .*? \[21\]\.': '16) **libpqxx** — официальный клиентский API для языка C++, предоставляющий доступ к возможностям СУБД PostgreSQL [39].',
        r'17\) \*\*Точность\*\* \(Precision\) — .*? \[21\]\.': '17) **Точность** (Precision) — доля истинно положительных срабатываний классификатора относительно всех объектов, отнесённых к положительному классу [36].',
        r'18\) \*\*Полнота\*\* \(Recall\) — .*? \[21\]\.': '18) **Полнота** (Recall) — доля истинно положительных срабатываний классификатора относительно всех объектов, фактически принадлежащих к положительному классу [36].',
        r'19\) \*\*F1-мера\*\* \(F1-score\) — .*? \[21\]\.': '19) **F1-мера** (F1-score) — гармоническое среднее точности и полноты, используемое для комплексной оценки качества классификации [36].',
        r'20\) \*\*ROC AUC\*\* \(Area Under the Receiver Operating Characteristic Curve\) — .*? \[21\]\.': '20) **ROC AUC** (Area Under the Receiver Operating Characteristic Curve) — площадь под кривой рабочих характеристик приёмника, количественный показатель качества классификатора [36].',
        r'26\) \*\*Z-score нормализация\*\* \(стандартизация\) — .*? \[21\]\.': '26) **Z-score нормализация** (стандартизация) — метод преобразования данных, при котором признаки центрируются относительно среднего значения и масштабируются по стандартному отклонению [36].',
        r'28\) \*\*Матрица ошибок\*\* \(confusion matrix\) — .*? \[21\]\.': '28) **Матрица ошибок** (confusion matrix) — инструмент оценки качества классификации, представляющий собой таблицу распределения предсказаний модели по классам [36].'
    }

    for pattern, replacement in mapping.items():
        text = re.sub(pattern, replacement, text)

    # 3. Final cleanup of common artifacts
    text = text.replace('рис. 1.1.1.', 'рис. 1.1')
    text = text.replace('Рис. 1.1.1.', 'Рис. 1.1')
    
    # 4. Check for any [21] that should be something else in main text
    # Usually in Section 1.4.2 it's [21] for features
    text = text.replace('набор признаков классификации [21]', 'набор признаков классификации [36]')
    text = text.replace('на датасете CIC-DDoS2019 [21]', 'на датасете CIC-DDoS2019 [24]')
    text = text.replace('фреймворка Qt6 [21]', 'фреймворка Qt6 [23]')

    with open(file_path, 'w', encoding='utf-8') as f:
        f.write(text)

fix_references_and_bibliography('docs/academic/ВКР_черновик_claude.md')
print('Corrected bibliography and all placeholders.')
