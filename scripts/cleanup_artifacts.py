import os
import re

def final_dpdk_cleanup():
    files = [
        'docs/academic/ВКР_черновик_claude.md',
        'docs/academic/ВКР_Черновик.md',
        'docs/academic/ОТЧЕТ_ПО_ПРЕДДИПЛОМНОЙ_ПРАКТИКЕ.md',
        'docs/academic/Отчет_по_практике_Final.md'
    ]
    
    for file_path in files:
        if not os.path.exists(file_path):
            continue
            
        with open(file_path, 'r', encoding='utf-8') as f:
            text = f.read()

        # 1. Remove DPDK from future work/development sections
        # This regex matches the specific sentence about Kernel Bypass and DPDK
        text = re.sub(r'В качестве направлений для дальнейшего развития.*?Kernel Bypass \(DPDK\).*?прикладного уровня\.', 
                      'В качестве направлений для дальнейшего развития системы рассматривается исследование гибридных архитектур нейронных сетей для выявления скрытых атак прикладного уровня.', 
                      text)
        
        text = text.replace('механизмов Kernel bypass (DPDK), позволяющих приложению напрямую обращаться к памяти сетевой карты, минуя стандартный сетевой стек ОС [7, 19]',
                            'механизмов оптимизации захвата трафика для повышения пропускной способности системы')

        # 2. Specific fixes for specific files
        if 'claude.md' in file_path:
             # This was already renumbered, but I'll check if anything remained
             text = text.replace('DPDK — Data Plane Development Kit  \n', '')
             text = text.replace('| libpcap / DPDK |', '| libpcap / Npcap |')

        # 3. Save
        with open(file_path, 'w', encoding='utf-8') as f:
            f.write(text)

final_dpdk_cleanup()
print('DPDK references removed from all files.')
