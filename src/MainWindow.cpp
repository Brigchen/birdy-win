#include "MainWindow.h"

#include "ui/AppStyle.h"
#include "ui/Paths.h"

#include <QCloseEvent>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QIcon>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QPixmap>
#include <QVBoxLayout>

namespace birdy {
namespace {

QString configPath() {
    return QDir(QStringLiteral("C:/Users/brigc/WorkBuddy/python/birdy/birdy-qt")).filePath(QStringLiteral("gui_config.json"));
}

QString legacyConfigPath() {
    return QDir(QStringLiteral("C:/Users/brigc/WorkBuddy/python/birdy/birdy-skill/src")).filePath(QStringLiteral("gui_config.json"));
}

QJsonObject readJson(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    return QJsonDocument::fromJson(file.readAll()).object();
}

QString versionText() {
    const QString info = QDir(QStringLiteral("C:/Users/brigc/WorkBuddy/python/birdy/birdy-skill")).filePath(QStringLiteral("version-info.json"));
    const QJsonObject object = readJson(info);
    const QString version = object.value(QLatin1String("version")).toString(QStringLiteral("2.1.85"));
    const QString date = object.value(QLatin1String("release_date")).toString();
    if (date.isEmpty()) {
        return version;
    }
    return version + QStringLiteral("（") + date + QStringLiteral("）");
}

}  // namespace

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle(QStringLiteral("Birdy"));
    resize(1200, 800);
    setStyleSheet(appStyleSheet());

    const QString iconPath = findAsset({
        QStringLiteral("birdy_logo.ico"),
        QStringLiteral("birdy_logo_128.png"),
        QStringLiteral("birdy_logo_640.png"),
    });
    if (!iconPath.isEmpty()) {
        setWindowIcon(QIcon(iconPath));
    }

    auto* central = new QWidget(this);
    auto* outer = new QVBoxLayout(central);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    auto* banner = new QWidget;
    banner->setObjectName(QStringLiteral("birdyTopBanner"));
    banner->setStyleSheet(QStringLiteral(
        "#birdyTopBanner { background-color: #FFFFFF; border-bottom: 1px solid #E0E0E0; }"));
    auto* bannerRow = new QHBoxLayout(banner);
    bannerRow->setContentsMargins(14, 8, 18, 8);
    bannerRow->setSpacing(10);
    auto* logo = new QLabel;
    logo->setFixedHeight(56);
    logo->setMinimumWidth(56);
    const QString bannerLogo = findAsset({QStringLiteral("birdy_logo_640.png"), QStringLiteral("birdy_logo_128.png")});
    if (!bannerLogo.isEmpty()) {
        const QPixmap pixmap(bannerLogo);
        if (!pixmap.isNull()) {
            const qreal dpr = logo->devicePixelRatioF();
            QPixmap scaled = pixmap.scaledToHeight(qMax(1, int(56 * dpr)), Qt::SmoothTransformation);
            scaled.setDevicePixelRatio(dpr);
            logo->setPixmap(scaled);
        }
    }
    auto* titles = new QVBoxLayout;
    titles->setSpacing(2);
    auto* cn = new QLabel(QStringLiteral("鸟图智慧仓储"));
    cn->setStyleSheet(QStringLiteral("color: #2E3A3F; font-size: 14pt; font-weight: bold;"));
    auto* en = new QLabel(QStringLiteral("Birdy"));
    en->setStyleSheet(QStringLiteral("color: #5A6B73; font-size: 10pt;"));
    auto* ver = new QLabel(QStringLiteral("版本 %1").arg(versionText()));
    ver->setStyleSheet(QStringLiteral("color: #7A8A92; font-size: 9pt;"));
    titles->addWidget(cn);
    titles->addWidget(en);
    titles->addWidget(ver);
    bannerRow->addWidget(logo, 0, Qt::AlignVCenter);
    bannerRow->addLayout(titles);
    bannerRow->addStretch(1);
    outer->addWidget(banner);

    settings_ = new SettingsForm;
    status_ = new StatusPanel(findAsset({QStringLiteral("logo.png"), QStringLiteral("logo.ico")}));
    settings_->setMinimumWidth(420);
    auto* body = new QWidget;
    auto* columns = new QHBoxLayout(body);
    columns->setSpacing(12);
    columns->setContentsMargins(12, 8, 12, 12);
    columns->addWidget(settings_, 1);
    columns->addWidget(status_, 1);
    outer->addWidget(body, 1);
    setCentralWidget(central);

    connect(settings_, &SettingsForm::logMessage, status_, &StatusPanel::appendLog);
    connect(settings_, &SettingsForm::configEdited, this, [this] {
        if (!saving_) {
            saveConfig();
        }
    });
    connect(settings_, &SettingsForm::modelDirChanged, this, &MainWindow::loadModels);
    connect(settings_, &SettingsForm::startRequested, this, &MainWindow::startProcessing);
    connect(settings_, &SettingsForm::stopRequested, this, [this] {
        status_->appendLog(QStringLiteral("当前没有正在运行的处理。"));
    });

    loadConfig();
    const QString now = QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss"));
    status_->appendLog(QStringLiteral("[%1] 窗口已打开。界面按 Birdy %2 排列。")
                           .arg(now, versionText()));
}

QString MainWindow::findAsset(const QStringList& names) const {
    const QStringList roots = {
        QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("resources")),
        QStringLiteral("C:/Users/brigc/WorkBuddy/python/birdy/birdy-qt/resources"),
        QStringLiteral("C:/Users/brigc/WorkBuddy/python/birdy/birdy-skill/resources"),
    };
    for (const QString& root : roots) {
        for (const QString& name : names) {
            const QString path = QDir(root).filePath(name);
            if (QFileInfo::exists(path)) {
                return path;
            }
        }
    }
    return {};
}

void MainWindow::loadConfig() {
    QString path = configPath();
    if (!QFileInfo::exists(path)) {
        path = legacyConfigPath();
    }
    const QJsonObject config = readJson(path);
    if (!config.isEmpty()) {
        saving_ = true;
        settings_->applyConfig(config);
        saving_ = false;
        status_->appendLog(QStringLiteral("已读取设置：") + path);
    }
}

void MainWindow::saveConfig() const {
    QFile file(configPath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return;
    }
    const QJsonDocument document(settings_->collectConfig());
    file.write(document.toJson(QJsonDocument::Indented));
}

void MainWindow::loadModels(const QString& dir) {
    if (dir.isEmpty()) {
        return;
    }
    std::string error;
    engine_.load(dir.toStdString(), &error);
    const QString status = QString::fromStdString(engine_.status());
    settings_->setModelStatus(status);
    status_->appendLog(error.empty() ? status : QString::fromStdString(error));
}

void MainWindow::startProcessing() {
    const QJsonObject config = settings_->collectConfig();
    const QString image = config.value(QLatin1String("image_folder")).toString().trimmed();
    if (image.isEmpty() || !QDir(image).exists()) {
        status_->appendLog(QStringLiteral("请先选择存在的图片文件夹。"));
        return;
    }
    const SessionPaths paths = sessionPaths(
        config.value(QLatin1String("output_root_folder")).toString(),
        image,
        config.value(QLatin1String("output_folder")).toString(),
        config.value(QLatin1String("crop_output_folder")).toString());
    status_->setProgress(0);
    status_->appendLog(QStringLiteral("主流程目录："));
    status_->appendLog(QStringLiteral("· 筛选 %1").arg(paths.outputFolder));
    status_->appendLog(QStringLiteral("· 归档 %1").arg(paths.cropFolder));
    status_->appendLog(QStringLiteral("· 报告 %1").arg(paths.reportsFolder));
    QStringList phases;
    if (config.value(QLatin1String("enable_burst_detection")).toBool()) {
        phases << QStringLiteral("连拍筛选");
    }
    if (config.value(QLatin1String("enable_gps_write")).toBool()) {
        phases << QStringLiteral("写入 GPS");
    }
    if (config.value(QLatin1String("enable_species_detection")).toBool()) {
        phases << QStringLiteral("物种识别");
    }
    if (config.value(QLatin1String("enable_watermark_generation")).toBool()) {
        phases << QStringLiteral("水印");
    }
    if (config.value(QLatin1String("enable_record_export_auto")).toBool()) {
        phases << QStringLiteral("观鸟记录");
    }
    if (config.value(QLatin1String("enable_track_map_auto")).toBool()) {
        phases << QStringLiteral("轨迹图");
    }
    status_->appendLog(phases.isEmpty()
                           ? QStringLiteral("没有勾选加入主流程的步骤。")
                           : QStringLiteral("将按顺序执行：%1。这些处理步骤还在从 Python 迁入。").arg(phases.join(QStringLiteral(" → "))));
}

void MainWindow::closeEvent(QCloseEvent* event) {
    saveConfig();
    QMainWindow::closeEvent(event);
}

}  // namespace birdy
