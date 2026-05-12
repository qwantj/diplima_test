#pragma once

#include <QString>
#include <QDateTime>

struct IncidentReportData {
    QString startTime;
    QString duration;
    QString attackerIp;
    QString maxPps;
    QString type;
    QString confidence;
};

class ReportGenerator {
public:
    static bool generatePdf(const QString& filePath, const IncidentReportData& data);
};
