#include "monitor_ui/ReportGenerator.hpp"
#include <QPdfWriter>
#include <QTextDocument>
#include <QPageSize>
#include <QDateTime>

bool ReportGenerator::generatePdf(const QString& filePath, const IncidentReportData& data) {
    QPdfWriter pdfWriter(filePath);
    pdfWriter.setPageSize(QPageSize(QPageSize::A4));
    pdfWriter.setResolution(300);

    QTextDocument doc;
    QString html = R"(
        <html>
        <head>
            <style>
                body { font-family: Arial, sans-serif; color: #333; }
                h1 { color: #e84a5f; text-align: center; border-bottom: 2px solid #e84a5f; padding-bottom: 10px; }
                table { width: 100%; border-collapse: collapse; margin-top: 20px; }
                th, td { border: 1px solid #ddd; padding: 12px; text-align: left; }
                th { background-color: #f0f0f0; color: #333; font-weight: bold; width: 40%; }
                .footer { margin-top: 40px; font-size: 10px; color: #777; text-align: center; }
            </style>
        </head>
        <body>
            <h1>Security Incident Report</h1>
            <p><strong>Generated on:</strong> %7</p>
            <table>
                <tr><th>Incident Start Time</th><td>%1</td></tr>
                <tr><th>Duration (sec)</th><td>%2</td></tr>
                <tr><th>Attacker IP</th><td>%3</td></tr>
                <tr><th>Max PPS</th><td>%4</td></tr>
                <tr><th>Attack Type</th><td>%5</td></tr>
                <tr><th>Model Confidence</th><td>%6</td></tr>
            </table>
            <div class="footer">
                DDoS Detection System - Automated Security Report
            </div>
        </body>
        </html>
    )";
    
    html = html.arg(data.startTime)
               .arg(data.duration)
               .arg(data.attackerIp)
               .arg(data.maxPps)
               .arg(data.type)
               .arg(data.confidence)
               .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));

    doc.setHtml(html);
    doc.print(&pdfWriter);
    return true;
}
