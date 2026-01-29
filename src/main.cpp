#include <QApplication>
#include <QMainWindow>
#include <QStatusBar>

int main(int argc, char* argv[]) {
  QApplication app(argc, argv);

  QMainWindow window;
  window.setWindowTitle(QStringLiteral("DoS Detector"));
  window.resize(1024, 768);
  window.statusBar()->showMessage(QStringLiteral("Ready"));
  window.show();

  return app.exec();
}
