import os
import re

def final_sync_refs():
    file_path = 'docs/academic/ВКР_черновик_claude.md'
    with open(file_path, 'r', encoding='utf-8') as f:
        text = f.read()

    # Define the final bibliography based on previous script results
    # Sequential, skipping 22, removed DPDK items.
    bib_final = """## СПИСОК ИСПОЛЬЗОВАННОЙ ЛИТЕРАТУРЫ

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

19) PostgreSQL Global Development Group. PostgreSQL Documentation. COPY / PostgreSQL Global Development Group. — Текст : электронный. — 2026. — URL: https://www.postgresql.org/docs/current/SQL-copy.html (дата обращения: 08.04.2026).

20) Preshing, J. An Introduction to Lock-Free Programming / J. Preshing. — Текст : электронный. — 2012. — URL: https://preshing.com/20120612/an-introduction-to-lock-free-programming/ (дата обращения: 08.04.2026).

21) Qt Group. Signals & Slots | Qt Core | Qt 6 / Qt Group. — Текст : электронный. — URL: https://doc.qt.io/qt-6/signalsandslots.html (дата обращения: 08.04.2026).

23) Sharafaldin, I. Toward Generating a New Intrusion Detection Dataset and Intrusion Traffic Characterization / I. Sharafaldin, A. H. Lashkari, A. A. Ghorbani. — Текст : электронный // Proceedings of the 4th International Conference on Information Systems Security and Privacy (ICISSP). — 2018. — P. 108–116. — URL: https://www.scitepress.org/Papers/2018/66398/66398.pdf (дата обращения: 08.04.2026).

24) University of New Brunswick. DDoS evaluation dataset (CIC-DDoS2019) / UNB. — Текст : электронный. — URL: https://www.unb.ca/cic/datasets/ddos-2019.html (дата обращения: 08.04.2026).

25) Tellez, A. Analyzing BotNets with Suricata & Machine Learning / A. Tellez. — Текст : электронный // Splunk Blog. — 2017. — URL: https://www.splunk.com/en_us/blog/security/analyzing-botnets-with-suricata-machine-learning.html (дата обращения: 08.04.2026).

26) University of Portsmouth. A Comparative Analysis of Snort 3 and Suricata / University of Portsmouth. — Текст : электронный. — URL: https://pure.port.ac.uk/ (дата обращения: 08.04.2026).

27) Лукацкий, А. В. Обнаружение атак / А. В. Лукацкий. — Санкт-Петербург : БХВ-Петербург, 2003. — 624 с. — Текст : непосредственный.

28) ГОСТ Р 50922-2006. Защита информации. Основные термины и определения. — Москва : Стандартинформ, 2008. — 11 с. — Текст : непосредственный.

29) ГОСТ Р ИСО/МЭК 27001-2021. Информационные технологии. Методы и средства обеспечения безопасности. Системы управления информационной безопасностью. Требования. — Москва : Стандартинформ, 2022. — Текст : непосредственный.

30) ГОСТ 19.301-79. Единая система программной документации. Программа и методика испытаний. Требования к содержанию и оформлению. — Москва : Стандартинформ, 2010. — Текст : непосредственный.

31) ГОСТ 19.401-78. Единая система программной документации. Текст программы. Требования к содержанию и оформлению. — Москва : Стандартинформ, 2010. — Текст : непосредственный.

32) IEEE 830-1998. Recommended Practice for Software Requirements Specifications. — IEEE, 1998. — Текст : непосредственный.

33) Олифер, В. Г. Компьютерные сети. Принципы, технологии, протоколы / В. Г. Олифер, Н. А. Олифер. — 5-е изд. — Санкт-Петербург : Питер, 2016. — 992 с. — Текст : непосредственный.

34) Воронцов, К. В. Машинное обучение: курс лекций / К. В. Воронцов. — Москва : МГУ, 2020. — Текст : электронный. — URL: http://www.machinelearning.ru (дата обращения: 08.04.2026).

35) IEEE 610.12-1990. IEEE Standard Glossary of Software Engineering Terminology. — IEEE, 1990. — Текст : непосредственный.

36) Дейт, К. Дж. Введение в системы баз данных / К. Дж. Дейт. — 8-е изд. — Москва : Вильямс, 2005. — 1328 с. — Текст : непосредственный.

37) libpqxx: The official C++ client API for PostgreSQL / libpqxx. — Текст : электронный. — URL: https://pqxx.org/development/libpqxx/ (дата обращения: 08.04.2026).
"""

    start_idx = text.find('## СПИСОК ИСПОЛЬЗОВАННОЙ ЛИТЕРАТУРЫ')
    if start_idx != -1:
        pre_text = text[:start_idx]
        with open(file_path, 'w', encoding='utf-8') as f:
            f.write(pre_text + bib_final)

final_sync_refs()
