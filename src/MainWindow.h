#pragma once

#include "infer/BirdEngine.h"
#include "ui/SettingsForm.h"
#include "ui/StatusPanel.h"

#include <QMainWindow>

namespace birdy {

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void loadConfig();
    void saveConfig() const;
    void loadModels(const QString& dir);
    void startProcessing();
    QString findAsset(const QStringList& names) const;

    SettingsForm* settings_ = nullptr;
    StatusPanel* status_ = nullptr;
    BirdEngine engine_;
    bool saving_ = false;
};

}  // namespace birdy
