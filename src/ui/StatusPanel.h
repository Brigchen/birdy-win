#pragma once

#include <QWidget>

class QLabel;
class QProgressBar;
class QTableWidget;
class QTextEdit;

namespace birdy {

class StatusPanel : public QWidget {
    Q_OBJECT
public:
    explicit StatusPanel(const QString& copyrightLogoPath, QWidget* parent = nullptr);

    void appendLog(const QString& line);
    void clearLog();
    void setProgress(int value);
    void setElapsed(const QString& text);
    void setEta(const QString& text);

private:
    QProgressBar* progress_ = nullptr;
    QLabel* elapsed_ = nullptr;
    QLabel* eta_ = nullptr;
    QTextEdit* log_ = nullptr;
    QTableWidget* stats_ = nullptr;
};

}  // namespace birdy
