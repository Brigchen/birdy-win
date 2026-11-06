#include "ui/SettingsForm.h"

#include "ui/AppStyle.h"
#include "ui/Cards.h"
#include "ui/Paths.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDesktopServices>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QJsonArray>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QSlider>
#include <QSpinBox>
#include <QUrl>
#include <QVBoxLayout>

namespace birdy {
namespace {

QScrollArea* verticalScroll(QWidget* parent) {
    auto* scroll = new QScrollArea(parent);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    return scroll;
}

QLabel* hintLabel(const QString& text) {
    auto* label = new QLabel(text);
    label->setWordWrap(true);
    label->setStyleSheet(QStringLiteral("color: #555555; font-size: 9pt;"));
    return label;
}

QFormLayout* formOn(QWidget* body) {
    auto* form = new QFormLayout(body);
    form->setSpacing(8);
    form->setContentsMargins(12, 10, 12, 12);
    return form;
}

QWidget* rowOf(const QList<QWidget*>& widgets, int stretchIndex = 0) {
    auto* wrap = new QWidget;
    auto* layout = new QHBoxLayout(wrap);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);
    if (stretchIndex < 0) {
        stretchIndex = widgets.size() - 1;
    }
    for (int i = 0; i < widgets.size(); ++i) {
        layout->addWidget(widgets.at(i), i == stretchIndex ? 1 : 0);
    }
    return wrap;
}

QString skillSrcDir() {
    return QDir(QStringLiteral("C:/Users/brigc/WorkBuddy/python/birdy/birdy-skill/src")).absolutePath();
}

QString comboData(const QComboBox* box) {
    const QVariant data = box->currentData();
    return data.isValid() ? data.toString() : box->currentText();
}

}  // namespace

SettingsForm::SettingsForm(QWidget* parent) : QWidget(parent) {
    buildUi();
}

void SettingsForm::fillCombo(QComboBox* box, const QList<QPair<QString, QString>>& items) {
    for (const auto& item : items) {
        box->addItem(item.first, item.second);
    }
    box->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    box->setMinimumContentsLength(8);
}

void SettingsForm::setCombo(QComboBox* box, const QString& data) const {
    const int index = box->findData(data);
    box->setCurrentIndex(index >= 0 ? index : 0);
}

void SettingsForm::touch() {
    if (!loading_) {
        emit configEdited();
    }
}

void SettingsForm::notePending(const QString& name) {
    emit logMessage(QStringLiteral("「%1」的处理逻辑还在从 Birdy 迁入，界面和设置已经就位。").arg(name));
}

void SettingsForm::browseDirectory(QLineEdit* target, const QString& title) {
    const QString start = target->text().isEmpty() ? QDir::homePath() : target->text();
    const QString path = QFileDialog::getExistingDirectory(this, title, start);
    if (path.isEmpty()) {
        return;
    }
    target->setText(QDir::toNativeSeparators(path));
    refreshDerived();
    refreshTrackPath();
    touch();
}

void SettingsForm::browseFile(QLineEdit* target, const QString& title, const QString& filter) {
    const QString path = QFileDialog::getOpenFileName(this, title, target->text(), filter);
    if (path.isEmpty()) {
        return;
    }
    target->setText(QDir::toNativeSeparators(path));
    touch();
}

void SettingsForm::buildUi() {
    auto* scroll = verticalScroll(this);
    auto* panel = new QWidget;
    auto* layout = new QVBoxLayout(panel);
    layout->setSpacing(12);
    layout->setContentsMargins(0, 0, 8, 0);

    const Section folder = makeCard(QStringLiteral("📁 文件夹设置"));
    auto* folderForm = formOn(folder.body);
    imageFolder_ = new QLineEdit;
    imageFolder_->setReadOnly(true);
    auto* imageBrowse = new QPushButton(QStringLiteral("浏览..."));
    connect(imageBrowse, &QPushButton::clicked, this, [this] {
        browseDirectory(imageFolder_, QStringLiteral("选择图片文件夹"));
    });
    folderForm->addRow(QStringLiteral("图片文件夹:"), rowOf({imageFolder_, imageBrowse}));

    dualFormat_ = new QComboBox;
    fillCombo(dualFormat_, {
        {QStringLiteral("关"), QStringLiteral("off")},
        {QStringLiteral("仅 JPG"), QStringLiteral("jpg_only")},
        {QStringLiteral("JPG + 复制 RAW"), QStringLiteral("jpg_copy_raw")},
    });
    dualFormat_->setToolTip(QStringLiteral(
        "RAW+JPG 同目录时使用：主流程只处理 JPG；「复制 RAW」在筛选后将配对 RAW 写入 Screened_raw_images。"));
    folderForm->addRow(QStringLiteral("RAW+JPG:"), dualFormat_);

    outputRoot_ = new QLineEdit;
    outputRoot_->setPlaceholderText(QStringLiteral("例如 D:/birdy/output（留空则使用下方手动路径）"));
    auto* rootBrowse = new QPushButton(QStringLiteral("浏览..."));
    connect(rootBrowse, &QPushButton::clicked, this, [this] {
        browseDirectory(outputRoot_, QStringLiteral("选择输出根目录"));
    });
    folderForm->addRow(QStringLiteral("输出根目录:"), rowOf({outputRoot_, rootBrowse}));

    derivedPaths_ = hintLabel(QString());
    derivedPaths_->setVisible(false);
    folderForm->addRow(QStringLiteral("自动生成:"), derivedPaths_);

    legacyPaths_ = new QWidget;
    auto* legacyForm = new QFormLayout(legacyPaths_);
    legacyForm->setContentsMargins(0, 0, 0, 0);
    legacyForm->setSpacing(8);
    outputFolder_ = new QLineEdit(QStringLiteral("./outputs"));
    cropFolder_ = new QLineEdit(QStringLiteral("./crops"));
    auto* outputBrowse = new QPushButton(QStringLiteral("浏览..."));
    auto* cropBrowse = new QPushButton(QStringLiteral("浏览..."));
    connect(outputBrowse, &QPushButton::clicked, this, [this] {
        browseDirectory(outputFolder_, QStringLiteral("选择输出文件夹"));
    });
    connect(cropBrowse, &QPushButton::clicked, this, [this] {
        browseDirectory(cropFolder_, QStringLiteral("选择分类归档文件夹"));
    });
    legacyForm->addRow(QStringLiteral("输出文件夹:"), rowOf({outputFolder_, outputBrowse}));
    legacyForm->addRow(QStringLiteral("分类归档文件夹:"), rowOf({cropFolder_, cropBrowse}));
    folderForm->addRow(legacyPaths_);
    layout->addWidget(folder.outer);

    gpsWrite_ = new QCheckBox(QStringLiteral("加入主流程"));
    gpsWrite_->setStyleSheet(flowCheckStyle());
    gpsWrite_->setToolTip(QStringLiteral(
        "勾选后，连拍筛选完成时按下方写入方式自动写入 Screened_images。"));
    const Section geo = makeCollapsible(QStringLiteral("🌍 地理位置"), true, {gpsWrite_});
    geoBody_ = geo.outer;
    auto* geoForm = formOn(geo.body);
    geoForm->addRow(hintLabel(QStringLiteral(
        "GPS 写入二选一（勾选「加入主流程」后自动执行）：指定地点统一经纬度，或 GPX 按拍摄时间匹配。"
        "下方「批量写入照片 GPS」可随时单独对任意文件夹执行。")));

    gpsFixed_ = new QRadioButton(QStringLiteral("指定地点统一写入"));
    gpsGpx_ = new QRadioButton(QStringLiteral("GPX 按拍摄时间匹配"));
    gpsFixed_->setChecked(true);
    auto* gpsModes = new QButtonGroup(this);
    gpsModes->addButton(gpsFixed_);
    gpsModes->addButton(gpsGpx_);
    geoForm->addRow(QStringLiteral("写入方式:"), rowOf({gpsFixed_, gpsGpx_, new QWidget}));

    auto* amapButton = new QPushButton(QStringLiteral("打开配置文件"));
    amapButton->setToolTip(QStringLiteral("编辑 amap_api_config.json，填写 api_key"));
    connect(amapButton, &QPushButton::clicked, this, [this] {
        const QString path = QDir(skillSrcDir()).filePath(QStringLiteral("amap_api_config.json"));
        if (!QFileInfo::exists(path)) {
            notePending(QStringLiteral("高德配置"));
            return;
        }
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    });
    geoForm->addRow(QStringLiteral("高德API:"), rowOf({amapButton, new QWidget}, -1));

    location_ = new QComboBox;
    location_->setEditable(true);
    location_->lineEdit()->setPlaceholderText(QStringLiteral("输入地址，或下拉选择历史位置"));
    auto* query = new QPushButton(QStringLiteral("查询"));
    connect(query, &QPushButton::clicked, this, [this] { notePending(QStringLiteral("地址查询")); });
    geoForm->addRow(QStringLiteral("地址:"), rowOf({location_, query}));
    latitude_ = new QLineEdit(QStringLiteral("31.2304"));
    latitude_->setPlaceholderText(QStringLiteral("例: 31.230416"));
    longitude_ = new QLineEdit(QStringLiteral("121.4737"));
    longitude_->setPlaceholderText(QStringLiteral("例: 121.473682"));
    provinceCity_ = new QLineEdit;
    provinceCity_->setReadOnly(true);
    provinceCity_->setPlaceholderText(QStringLiteral("查询后自动显示省市"));
    geoForm->addRow(QStringLiteral("纬度:"), latitude_);
    geoForm->addRow(QStringLiteral("经度:"), longitude_);
    geoForm->addRow(QStringLiteral("省市:"), provinceCity_);

    auto* gpxBox = new QGroupBox(QStringLiteral("GPX 轨迹与时间（主流程 GPX 模式 / 下方独立按钮）"));
    auto* gpxForm = new QFormLayout(gpxBox);
    gpxList_ = new QListWidget;
    gpxList_->setMaximumHeight(84);
    gpxForm->addRow(QStringLiteral("GPX 轨迹:"), gpxList_);
    auto* addGpx = new QPushButton(QStringLiteral("添加 GPX..."));
    auto* removeGpx = new QPushButton(QStringLiteral("移除"));
    auto* mergeGpx = new QPushButton(QStringLiteral("合并为文件..."));
    connect(addGpx, &QPushButton::clicked, this, [this] {
        const QStringList files = QFileDialog::getOpenFileNames(
            this, QStringLiteral("添加 GPX"), QString(), QStringLiteral("GPX (*.gpx);;所有文件 (*.*)"));
        for (const QString& file : files) {
            gpxList_->addItem(QDir::toNativeSeparators(file));
        }
        if (!files.isEmpty()) {
            touch();
        }
    });
    connect(removeGpx, &QPushButton::clicked, this, [this] {
        qDeleteAll(gpxList_->selectedItems());
        touch();
    });
    connect(mergeGpx, &QPushButton::clicked, this, [this] { notePending(QStringLiteral("合并 GPX")); });
    gpxForm->addRow(QString(), rowOf({addGpx, removeGpx, mergeGpx, new QWidget}));
    exifTz_ = new QComboBox;
    gpxTz_ = new QComboBox;
    for (QComboBox* box : {exifTz_, gpxTz_}) {
        box->setEditable(true);
        box->addItems({
            QStringLiteral("Asia/Shanghai"), QStringLiteral("UTC"), QStringLiteral("Asia/Hong_Kong"),
            QStringLiteral("Asia/Taipei"), QStringLiteral("Asia/Singapore"), QStringLiteral("Asia/Tokyo"),
            QStringLiteral("Europe/London"), QStringLiteral("America/New_York"),
            QStringLiteral("America/Los_Angeles"),
        });
    }
    exifTz_->setCurrentText(QStringLiteral("Asia/Shanghai"));
    gpxTz_->setCurrentText(QStringLiteral("UTC"));
    gpxForm->addRow(QStringLiteral("EXIF 时区:"), exifTz_);
    gpxForm->addRow(QStringLiteral("GPX 时区:"), gpxTz_);
    gpxToScreened_ = new QCheckBox(QStringLiteral("写入到输出目录 Screened_images（否则写入图片文件夹）"));
    gpxToScreened_->setChecked(true);
    gpxForm->addRow(QString(), gpxToScreened_);
    auto* writeGps = new QPushButton(QStringLiteral("批量写入照片 GPS"));
    connect(writeGps, &QPushButton::clicked, this, [this] { notePending(QStringLiteral("批量写入照片 GPS")); });
    gpxForm->addRow(QString(), writeGps);
    geoForm->addRow(gpxBox);
    layout->addWidget(geo.outer);

    burstOn_ = new QCheckBox(QStringLiteral("加入主流程"));
    burstOn_->setChecked(true);
    burstOn_->setStyleSheet(flowCheckStyle());
    burstOn_->setToolTip(QStringLiteral("勾选：对图片文件夹做连拍分组与筛选，并写入 Screened_images。"));
    const Section burst = makeCollapsible(QStringLiteral("📷 连拍处理"), true, {burstOn_});
    burstBody_ = burst.outer;
    auto* burstForm = formOn(burst.body);
    timeThreshold_ = new QDoubleSpinBox;
    timeThreshold_->setRange(0.1, 10.0);
    timeThreshold_->setSingleStep(0.1);
    timeThreshold_->setValue(1.0);
    timeThreshold_->setSuffix(QStringLiteral(" 秒"));
    keepRatio_ = new QDoubleSpinBox;
    keepRatio_->setRange(0.05, 1.0);
    keepRatio_->setSingleStep(0.05);
    keepRatio_->setDecimals(2);
    keepRatio_->setValue(0.2);
    keepMin_ = new QSpinBox;
    keepMin_->setRange(1, 50);
    keepMin_->setValue(2);
    focusWeight_ = new QDoubleSpinBox;
    focusWeight_->setRange(0.0, 10.0);
    focusWeight_->setSingleStep(0.5);
    focusWeight_->setDecimals(1);
    focusWeight_->setValue(9.0);
    areaWeight_ = new QDoubleSpinBox;
    areaWeight_->setRange(0.0, 10.0);
    areaWeight_->setSingleStep(0.5);
    areaWeight_->setDecimals(1);
    areaWeight_->setValue(1.0);
    burstForm->addRow(QStringLiteral("连拍时间阈值:"), timeThreshold_);
    burstForm->addRow(QStringLiteral("连拍保留比例:"), keepRatio_);
    burstForm->addRow(QStringLiteral("连拍最少保留:"), keepMin_);
    burstForm->addRow(QStringLiteral("对焦:面积 权重:"), rowOf({focusWeight_, new QLabel(QStringLiteral(":")), areaWeight_}));
    birdDetect_ = new QCheckBox(QStringLiteral("启用鸟体检测"));
    birdDetect_->setChecked(true);
    birdMode_ = new QComboBox;
    fillCombo(birdMode_, {{QStringLiteral("普通"), QStringLiteral("normal")}, {QStringLiteral("精细"), QStringLiteral("fine")}});
    eyeDetect_ = new QCheckBox(QStringLiteral("启用鸟眼检测（需先启用鸟体检测）"));
    fastMode_ = new QCheckBox(QStringLiteral("使用快速模式"));
    fastMode_->setChecked(true);
    fastRatio_ = new QDoubleSpinBox;
    fastRatio_->setRange(0.0, 1.0);
    fastRatio_->setSingleStep(0.05);
    fastRatio_->setDecimals(2);
    fastRatio_->setValue(0.5);
    burstForm->addRow(QString(), rowOf({birdDetect_, birdMode_, new QWidget}));
    burstForm->addRow(QString(), eyeDetect_);
    burstForm->addRow(QString(), rowOf({fastMode_, new QLabel(QStringLiteral("抽样比例:")), fastRatio_, new QWidget}));
    layout->addWidget(burst.outer);

    speciesOn_ = new QCheckBox(QStringLiteral("加入主流程"));
    speciesOn_->setChecked(true);
    speciesOn_->setStyleSheet(flowCheckStyle());
    const Section species = makeCollapsible(QStringLiteral("🦅 物种识别"), true, {speciesOn_});
    speciesBody_ = species.outer;
    auto* speciesForm = formOn(species.body);
    speciesFolder_ = new QLineEdit;
    speciesFolder_->setPlaceholderText(QStringLiteral("留空则用上方「图片文件夹」"));
    auto* speciesBrowse = new QPushButton(QStringLiteral("浏览..."));
    connect(speciesBrowse, &QPushButton::clicked, this, [this] {
        browseDirectory(speciesFolder_, QStringLiteral("选择相片文件夹"));
    });
    speciesForm->addRow(QStringLiteral("指定相片文件夹:"), rowOf({speciesFolder_, speciesBrowse}));
    auto* speciesRun = new QPushButton(QStringLiteral("单独裁切并识别"));
    connect(speciesRun, &QPushButton::clicked, this, [this] { notePending(QStringLiteral("单独裁切并识别")); });
    speciesForm->addRow(QString(), speciesRun);
    localModel_ = new QRadioButton(QStringLiteral("本地模型 (离线)"));
    doubaoModel_ = new QRadioButton(QStringLiteral("豆包 Seed 2.0 视觉（在线，约 1 张/秒）"));
    localModel_->setChecked(true);
    auto* modelModes = new QButtonGroup(this);
    modelModes->addButton(localModel_);
    modelModes->addButton(doubaoModel_);
    speciesForm->addRow(QStringLiteral("物种识别模式:"), rowOf({localModel_, doubaoModel_}));
    speciesModel_ = new QComboBox;
    fillCombo(speciesModel_, {
        {QStringLiteral("ResNet34（推荐）"), QStringLiteral("resnet34")},
        {QStringLiteral("EfficientNet-B0"), QStringLiteral("efficientnet_b0")},
    });
    speciesForm->addRow(QStringLiteral("本地物种模型:"), speciesModel_);
    modelDir_ = new QLineEdit;
    modelDir_->setPlaceholderText(QStringLiteral("bird-seg、bird-iden 与 bird_info.json 所在目录"));
    auto* modelBrowse = new QPushButton(QStringLiteral("浏览..."));
    connect(modelBrowse, &QPushButton::clicked, this, [this] {
        browseDirectory(modelDir_, QStringLiteral("选择模型目录"));
        emit modelDirChanged(modelDir_->text());
    });
    speciesForm->addRow(QStringLiteral("模型目录:"), rowOf({modelDir_, modelBrowse}));
    modelStatus_ = hintLabel(QStringLiteral("推理未链接"));
    speciesForm->addRow(QStringLiteral("状态:"), modelStatus_);
    dxo_ = new QCheckBox(QStringLiteral("识别前用 DxO PureRAW 降噪（Screened 中 RAW）"));
    speciesForm->addRow(QString(), dxo_);
    geoMode_ = new QComboBox;
    fillCombo(geoMode_, {
        {QStringLiteral("自动（有 GPS/省份用省名单，否则全国）"), QStringLiteral("auto")},
        {QStringLiteral("全国名单"), QStringLiteral("china")},
        {QStringLiteral("仅省份名单（无省信息时回退全国）"), QStringLiteral("province")},
        {QStringLiteral("不限制地理"), QStringLiteral("none")},
    });
    speciesForm->addRow(QStringLiteral("地理约束:"), geoMode_);
    auto* doubaoConfig = new QPushButton(QStringLiteral("打开配置文件"));
    connect(doubaoConfig, &QPushButton::clicked, this, [this] {
        const QString path = QDir(skillSrcDir()).filePath(QStringLiteral("doubao_api_config.json"));
        if (!QFileInfo::exists(path)) {
            notePending(QStringLiteral("豆包配置"));
            return;
        }
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    });
    speciesForm->addRow(QStringLiteral("豆包API配置:"), rowOf({doubaoConfig, new QWidget}, -1));
    confEnable_ = new QCheckBox(QStringLiteral("启用"));
    confValue_ = new QDoubleSpinBox;
    confValue_->setRange(0.0, 1.0);
    confValue_->setSingleStep(0.05);
    confValue_->setDecimals(2);
    confValue_->setValue(0.5);
    confValue_->setEnabled(false);
    speciesForm->addRow(QStringLiteral("未知种类阈值(可选):"), rowOf({confEnable_, confValue_, new QWidget}));
    cleanBefore_ = new QCheckBox(QStringLiteral("切割后、识别前清洗切割图（主流程与单独识别）"));
    cleanBlur_ = new QCheckBox(QStringLiteral("去除鸟体模糊的图"));
    cleanBlur_->setChecked(true);
    clarity_ = new QSlider(Qt::Horizontal);
    clarity_->setRange(0, 100);
    clarity_->setValue(35);
    clarityValue_ = new QLabel(QStringLiteral("35"));
    clarityValue_->setMinimumWidth(28);
    cleanDedupe_ = new QCheckBox(QStringLiteral("去除高度重复的鸟图"));
    cleanDedupe_->setChecked(true);
    dupSimilarity_ = new QSlider(Qt::Horizontal);
    dupSimilarity_->setRange(50, 100);
    dupSimilarity_->setValue(92);
    dupValue_ = new QLabel(QStringLiteral("92%"));
    dupValue_->setMinimumWidth(36);
    cleanFolder_ = new QLineEdit;
    cleanFolder_->setPlaceholderText(QStringLiteral("留空则使用分类归档目录"));
    auto* cleanBrowse = new QPushButton(QStringLiteral("浏览..."));
    connect(cleanBrowse, &QPushButton::clicked, this, [this] {
        browseDirectory(cleanFolder_, QStringLiteral("选择清洗目录"));
    });
    auto* cleanRun = new QPushButton(QStringLiteral("清洗选定目录"));
    connect(cleanRun, &QPushButton::clicked, this, [this] { notePending(QStringLiteral("清洗选定目录")); });
    speciesForm->addRow(QString(), cleanBefore_);
    speciesForm->addRow(QString(), cleanBlur_);
    speciesForm->addRow(QStringLiteral("模糊阈值:"), rowOf({clarity_, clarityValue_}));
    speciesForm->addRow(QString(), cleanDedupe_);
    speciesForm->addRow(QStringLiteral("去重阈值:"), rowOf({dupSimilarity_, dupValue_}));
    speciesForm->addRow(QStringLiteral("清洗目录:"), rowOf({cleanFolder_, cleanBrowse}));
    speciesForm->addRow(QString(), cleanRun);
    layout->addWidget(species.outer);

    watermarkOn_ = new QCheckBox(QStringLiteral("加入主流程"));
    watermarkOn_->setStyleSheet(flowCheckStyle());
    const Section watermark = makeCollapsible(QStringLiteral("🖼 水印生成"), false, {watermarkOn_});
    watermarkBody_ = watermark.outer;
    auto* wmForm = formOn(watermark.body);
    wmInput_ = new QLineEdit;
    wmInput_->setPlaceholderText(QStringLiteral("可选：指定输入目录（支持多级子目录）"));
    wmOutput_ = new QLineEdit(QStringLiteral("./watermarked"));
    wmLogo_ = new QLineEdit;
    wmLogo_->setPlaceholderText(QStringLiteral("可选：签名 logo 图片路径（png/jpg）"));
    auto* wmInBrowse = new QPushButton(QStringLiteral("浏览..."));
    auto* wmOutBrowse = new QPushButton(QStringLiteral("浏览..."));
    auto* wmLogoBrowse = new QPushButton(QStringLiteral("选择文件..."));
    connect(wmInBrowse, &QPushButton::clicked, this, [this] {
        browseDirectory(wmInput_, QStringLiteral("选择水印输入目录"));
    });
    connect(wmOutBrowse, &QPushButton::clicked, this, [this] {
        browseDirectory(wmOutput_, QStringLiteral("选择水印输出目录"));
    });
    connect(wmLogoBrowse, &QPushButton::clicked, this, [this] {
        browseFile(wmLogo_, QStringLiteral("选择签名 Logo"),
                   QStringLiteral("图片 (*.png *.jpg *.jpeg *.webp *.bmp);;所有文件 (*.*)"));
    });
    wmForm->addRow(QStringLiteral("指定相片文件夹:"), rowOf({wmInput_, wmInBrowse}));
    wmForm->addRow(QStringLiteral("水印输出文件夹:"), rowOf({wmOutput_, wmOutBrowse}));
    wmForm->addRow(QStringLiteral("签名 Logo:"), rowOf({wmLogo_, wmLogoBrowse}));
    wmLogoRatio_ = new QDoubleSpinBox;
    wmLogoRatio_->setRange(0.05, 0.80);
    wmLogoRatio_->setSingleStep(0.01);
    wmLogoRatio_->setDecimals(2);
    wmLogoRatio_->setValue(0.30);
    wmLogoRatio_->setSuffix(QStringLiteral(" × 长边"));
    wmStyle_ = new QComboBox;
    fillCombo(wmStyle_, {
        {QStringLiteral("外框 + 底栏文字 + 图内签名"), QStringLiteral("frame")},
        {QStringLiteral("无外框 · 图内签名 + 竖线标签"), QStringLiteral("inline")},
    });
    wmLocation_ = new QCheckBox(QStringLiteral("显示地理位置"));
    wmLocation_->setChecked(true);
    wmGpsCity_ = new QCheckBox(QStringLiteral("优先使用图片 GPS 自动定位城市"));
    wmGpsCity_->setChecked(true);
    wmLocationText_ = new QLineEdit;
    wmLocationText_->setPlaceholderText(QStringLiteral("人工输入地点（填入后优先使用）"));
    wmDate_ = new QCheckBox(QStringLiteral("显示拍照日期"));
    wmDate_->setChecked(true);
    wmSpecies_ = new QCheckBox(QStringLiteral("显示物种名"));
    wmSpecies_->setChecked(true);
    wmCamera_ = new QCheckBox(QStringLiteral("显示相机参数"));
    wmCamera_->setChecked(true);
    wmSample_ = new QCheckBox(QStringLiteral("每物种目录抽若干张（不勾选则全部生成）"));
    wmSampleCount_ = new QSpinBox;
    wmSampleCount_->setRange(1, 999);
    wmSampleCount_->setValue(3);
    wmSampleCount_->setSuffix(QStringLiteral(" 张/物种目录"));
    wmSampleCount_->setEnabled(false);
    wmCollage_ = new QComboBox;
    fillCombo(wmCollage_, {
        {QStringLiteral("不拼图"), QStringLiteral("off")},
        {QStringLiteral("2拼 · 左右"), QStringLiteral("2h")},
        {QStringLiteral("2拼 · 上下"), QStringLiteral("2v")},
        {QStringLiteral("3拼 · 左右"), QStringLiteral("3h")},
        {QStringLiteral("3拼 · 上下"), QStringLiteral("3v")},
        {QStringLiteral("3拼 · 上二下一"), QStringLiteral("3t")},
        {QStringLiteral("4拼"), QStringLiteral("4")},
        {QStringLiteral("5拼 · 上二中一下二"), QStringLiteral("5")},
        {QStringLiteral("6拼 · 上中下各二"), QStringLiteral("6")},
        {QStringLiteral("9拼 · 上中下各三"), QStringLiteral("9")},
        {QStringLiteral("24拼 · 每行四张"), QStringLiteral("24")},
    });
    wmOneAll_ = new QCheckBox(QStringLiteral("其中一张用群体图（all）"));
    wmMulti_ = new QCheckBox(QStringLiteral("多物种合拼（每物种一张）"));
    wmExposure_ = new QCheckBox(QStringLiteral("水印前自动曝光"));
    wmExposureStrength_ = new QSlider(Qt::Horizontal);
    wmExposureStrength_->setRange(0, 100);
    wmExposureStrength_->setValue(100);
    wmExposureValue_ = new QLabel(QStringLiteral("1.00"));
    wmDenoise_ = new QCheckBox(QStringLiteral("水印前 AI 降噪"));
    wmDenoiseModel_ = new QComboBox;
    fillCombo(wmDenoiseModel_, {
        {QStringLiteral("Real-ESRGAN"), QStringLiteral("realesrgan")},
        {QStringLiteral("NAFNet"), QStringLiteral("nafnet")},
    });
    wmDenoiseStrength_ = new QSlider(Qt::Horizontal);
    wmDenoiseStrength_->setRange(0, 100);
    wmDenoiseStrength_->setValue(50);
    wmDenoiseValue_ = new QLabel(QStringLiteral("0.50"));
    wmSharpen_ = new QCheckBox(QStringLiteral("水印前 AI 锐化"));
    wmSharpenStrength_ = new QSlider(Qt::Horizontal);
    wmSharpenStrength_->setRange(0, 100);
    wmSharpenStrength_->setValue(50);
    wmSharpenValue_ = new QLabel(QStringLiteral("0.50"));
    wmForm->addRow(QStringLiteral("Logo 宽度占比:"), wmLogoRatio_);
    wmForm->addRow(QStringLiteral("水印布局:"), wmStyle_);
    wmForm->addRow(QString(), wmLocation_);
    wmForm->addRow(QString(), wmGpsCity_);
    wmForm->addRow(QStringLiteral("人工地点:"), wmLocationText_);
    wmForm->addRow(QString(), wmDate_);
    wmForm->addRow(QString(), wmSpecies_);
    wmForm->addRow(QString(), wmCamera_);
    wmForm->addRow(QString(), wmSample_);
    wmForm->addRow(QStringLiteral("抽样张数:"), wmSampleCount_);
    wmForm->addRow(QStringLiteral("拼图:"), wmCollage_);
    wmForm->addRow(QString(), wmOneAll_);
    wmForm->addRow(QString(), wmMulti_);
    wmForm->addRow(QString(), wmExposure_);
    wmForm->addRow(QStringLiteral("曝光强度:"), rowOf({wmExposureStrength_, wmExposureValue_}));
    wmForm->addRow(QString(), wmDenoise_);
    wmForm->addRow(QStringLiteral("降噪模型:"), wmDenoiseModel_);
    wmForm->addRow(QStringLiteral("降噪强度:"), rowOf({wmDenoiseStrength_, wmDenoiseValue_}));
    wmForm->addRow(QString(), wmSharpen_);
    wmForm->addRow(QStringLiteral("锐化强度:"), rowOf({wmSharpenStrength_, wmSharpenValue_}));
    auto* wmPreview = new QPushButton(QStringLiteral("预览一张效果"));
    auto* wmRun = new QPushButton(QStringLiteral("单独批量水印生成"));
    auto* wmBurst = new QPushButton(QStringLiteral("动图生成"));
    auto* wmVideo = new QPushButton(QStringLiteral("视频裁剪"));
    auto* wmFull = new QPushButton(QStringLiteral("全拼图"));
    connect(wmPreview, &QPushButton::clicked, this, [this] { notePending(QStringLiteral("水印预览")); });
    connect(wmRun, &QPushButton::clicked, this, [this] { notePending(QStringLiteral("单独批量水印生成")); });
    connect(wmBurst, &QPushButton::clicked, this, [this] { notePending(QStringLiteral("动图生成")); });
    connect(wmVideo, &QPushButton::clicked, this, [this] { notePending(QStringLiteral("视频裁剪")); });
    connect(wmFull, &QPushButton::clicked, this, [this] { notePending(QStringLiteral("全拼图")); });
    wmForm->addRow(QString(), rowOf({wmPreview, wmRun, wmBurst, wmVideo, wmFull, new QWidget}, -1));
    layout->addWidget(watermark.outer);

    trackOn_ = new QCheckBox(QStringLiteral("加入主流程"));
    trackOn_->setStyleSheet(flowCheckStyle());
    const Section track = makeCollapsible(QStringLiteral("🗺 轨迹图生成"), false, {trackOn_});
    trackBody_ = track.outer;
    auto* trackForm = formOn(track.body);
    trackForm->addRow(hintLabel(QStringLiteral(
        "从分类归档生成行迹与物种分布 PNG（4K 竖屏 2160×3840，需高德 Key）。"
        "图内标题地点优先使用「水印生成 → 人工地点」。")));
    trackGpx_ = new QCheckBox(QStringLiteral("使用 GPX 轨迹（未选则仅用照片 EXIF GPS）"));
    trackGpx_->setChecked(true);
    trackExif_ = new QCheckBox(QStringLiteral("使用照片 EXIF 中的 GPS"));
    trackExif_->setChecked(true);
    trackSource_ = new QComboBox;
    fillCombo(trackSource_, {
        {QStringLiteral("分类归档（物种目录）"), QStringLiteral("classification")},
        {QStringLiteral("Screened_images 筛选图"), QStringLiteral("screened")},
    });
    trackFolder_ = new QLineEdit;
    trackFolder_->setPlaceholderText(QStringLiteral("留空则使用上方来源的自动路径"));
    auto* trackBrowse = new QPushButton(QStringLiteral("浏览…"));
    connect(trackBrowse, &QPushButton::clicked, this, [this] {
        browseDirectory(trackFolder_, QStringLiteral("选择鸟图目录"));
    });
    trackPath_ = hintLabel(QString());
    trackRadius_ = new QDoubleSpinBox;
    trackRadius_->setRange(0.01, 100.0);
    trackRadius_->setDecimals(2);
    trackRadius_->setSingleStep(0.1);
    trackRadius_->setSuffix(QStringLiteral(" km"));
    trackRadius_->setValue(1.0);
    trackBasemap_ = new QComboBox;
    fillCombo(trackBasemap_, {
        {QStringLiteral("高德·标准（默认）"), QStringLiteral("normal")},
        {QStringLiteral("高德·无路网卫星"), QStringLiteral("satellite")},
        {QStringLiteral("高德·有路网卫星"), QStringLiteral("satellite_roads")},
    });
    trackElevation_ = new QCheckBox(QStringLiteral("同时生成海拔-距离剖面图"));
    trackElevation_->setChecked(true);
    trackRotate_ = new QCheckBox(QStringLiteral("底图自适应旋转"));
    trackPlaces_ = new QCheckBox(QStringLiteral("显示关键地点"));
    trackPlaceText_ = new QLineEdit;
    trackPlaceText_->setPlaceholderText(QStringLiteral("多个地点用逗号、顿号或分号分隔"));
    trackPlaceText_->setEnabled(false);
    auto* trackPreview = new QPushButton(QStringLiteral("预览轨迹图"));
    auto* trackSave = new QPushButton(QStringLiteral("单独生成并保存 PNG"));
    connect(trackPreview, &QPushButton::clicked, this, [this] { notePending(QStringLiteral("预览轨迹图")); });
    connect(trackSave, &QPushButton::clicked, this, [this] { notePending(QStringLiteral("轨迹图")); });
    trackForm->addRow(QString(), trackGpx_);
    trackForm->addRow(QString(), trackExif_);
    trackForm->addRow(QStringLiteral("鸟图来源:"), trackSource_);
    trackForm->addRow(QStringLiteral("鸟图目录:"), rowOf({trackFolder_, trackBrowse}));
    trackForm->addRow(QStringLiteral("当前使用:"), trackPath_);
    trackForm->addRow(QStringLiteral("物种去重半径:"), trackRadius_);
    trackForm->addRow(QStringLiteral("底图风格:"), trackBasemap_);
    trackForm->addRow(QString(), trackElevation_);
    trackForm->addRow(QString(), trackRotate_);
    trackForm->addRow(QString(), trackPlaces_);
    trackForm->addRow(QStringLiteral("关键地点:"), trackPlaceText_);
    trackForm->addRow(QString(), rowOf({trackPreview, trackSave, new QWidget}));
    layout->addWidget(track.outer);

    exportOn_ = new QCheckBox(QStringLiteral("加入主流程"));
    exportOn_->setStyleSheet(flowCheckStyle());
    const Section record = makeCollapsible(QStringLiteral("📤 观鸟记录导出"), true, {exportOn_});
    exportBody_ = record.outer;
    auto* exportForm = formOn(record.body);
    exportForm->addRow(hintLabel(QStringLiteral(
        "从分类归档导出 eBird Checklist .csv 与中国观鸟记录中心鸟种导入 .xls。")));
    exportClass_ = new QLineEdit;
    exportClass_->setPlaceholderText(QStringLiteral("默认使用分类归档路径"));
    exportOut_ = new QLineEdit;
    exportOut_->setPlaceholderText(QStringLiteral("留空则导出到 <输出根>/reports/record_export"));
    auto* classBrowse = new QPushButton(QStringLiteral("浏览..."));
    auto* outBrowse = new QPushButton(QStringLiteral("浏览..."));
    connect(classBrowse, &QPushButton::clicked, this, [this] {
        browseDirectory(exportClass_, QStringLiteral("选择分类归档目录"));
    });
    connect(outBrowse, &QPushButton::clicked, this, [this] {
        browseDirectory(exportOut_, QStringLiteral("选择导出目录"));
    });
    exportEbird_ = new QCheckBox(QStringLiteral("eBird Checklist Format（.csv）"));
    exportEbird_->setChecked(true);
    exportBirdreport_ = new QCheckBox(QStringLiteral("中国观鸟记录中心（鸟种导入.xls，官方两列模版）"));
    exportBirdreport_->setChecked(true);
    exportCountry_ = new QLineEdit(QStringLiteral("CN"));
    exportCountry_->setMaxLength(8);
    exportState_ = new QLineEdit(QStringLiteral("FJ"));
    exportState_->setMaxLength(3);
    exportCount_ = new QCheckBox(QStringLiteral("累计只数（计数）"));
    exportCount_->setChecked(true);
    auto* exportRun = new QPushButton(QStringLiteral("导出观鸟记录"));
    connect(exportRun, &QPushButton::clicked, this, [this] { notePending(QStringLiteral("导出观鸟记录")); });
    auto* ebirdLink = new QPushButton(QStringLiteral("ebird上传网页"));
    auto* chinaLink = new QPushButton(QStringLiteral("中国观鸟记录中心"));
    ebirdLink->setFlat(true);
    chinaLink->setFlat(true);
    connect(ebirdLink, &QPushButton::clicked, this, [] {
        QDesktopServices::openUrl(QUrl(QStringLiteral("https://ebird.org/import/upload")));
    });
    connect(chinaLink, &QPushButton::clicked, this, [] {
        QDesktopServices::openUrl(QUrl(QStringLiteral("https://www.birdreport.cn/")));
    });
    exportForm->addRow(QStringLiteral("分类归档目录:"), rowOf({exportClass_, classBrowse}));
    exportForm->addRow(QStringLiteral("导出目录:"), rowOf({exportOut_, outBrowse}));
    exportForm->addRow(QString(), exportEbird_);
    exportForm->addRow(QString(), exportBirdreport_);
    exportForm->addRow(QStringLiteral("eBird 国家代码:"), exportCountry_);
    exportForm->addRow(QStringLiteral("eBird 省/州:"), exportState_);
    exportForm->addRow(QStringLiteral("计数:"), exportCount_);
    exportForm->addRow(QString(), rowOf({ebirdLink, chinaLink, new QWidget}));
    exportForm->addRow(QString(), exportRun);
    layout->addWidget(record.outer);

    const Section actions = makeCard(QStringLiteral("操作"));
    auto* actionRow = new QHBoxLayout(actions.body);
    actionRow->setSpacing(12);
    actionRow->setContentsMargins(12, 10, 12, 12);
    startButton_ = new QPushButton(QStringLiteral("▶ 开始处理"));
    stopButton_ = new QPushButton(QStringLiteral("⏹ 停止"));
    startButton_->setStyleSheet(startButtonStyle());
    stopButton_->setStyleSheet(stopButtonStyle());
    stopButton_->setEnabled(false);
    connect(startButton_, &QPushButton::clicked, this, &SettingsForm::startRequested);
    connect(stopButton_, &QPushButton::clicked, this, &SettingsForm::stopRequested);
    actionRow->addWidget(startButton_, 1);
    actionRow->addWidget(stopButton_, 1);
    layout->addWidget(actions.outer);

    auto* tip = new QLabel(QStringLiteral(
        "💡 提示：设置「输出根目录」后每次只需换「图片文件夹」；或留空根目录并手动指定输出与分类路径。"));
    tip->setWordWrap(true);
    tip->setStyleSheet(QStringLiteral("color: #666666; font-size: 10pt;"));
    auto* brand = new QLabel(QStringLiteral("Birdy · 鸟图智慧仓储"));
    brand->setAlignment(Qt::AlignRight);
    brand->setStyleSheet(QStringLiteral("color: #E0E0E0; font-size: 9pt;"));
    layout->addWidget(tip);
    layout->addWidget(brand);

    scroll->setWidget(panel);
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->addWidget(scroll);

    connect(outputRoot_, &QLineEdit::textChanged, this, [this] {
        refreshDerived();
        refreshTrackPath();
        touch();
    });
    connect(imageFolder_, &QLineEdit::textChanged, this, [this] {
        refreshDerived();
        refreshTrackPath();
    });
    connect(dualFormat_, qOverload<int>(&QComboBox::currentIndexChanged), this, [this] { refreshDerived(); touch(); });
    connect(trackSource_, qOverload<int>(&QComboBox::currentIndexChanged), this, [this] { refreshTrackPath(); touch(); });
    connect(trackFolder_, &QLineEdit::textChanged, this, [this] { refreshTrackPath(); touch(); });
    connect(gpsFixed_, &QRadioButton::toggled, this, [this] { syncGpsMode(); touch(); });
    connect(burstOn_, &QCheckBox::toggled, this, [this] { syncBurstEnabled(); touch(); });
    connect(birdDetect_, &QCheckBox::toggled, this, [this] { syncBirdDetect(); touch(); });
    connect(fastMode_, &QCheckBox::toggled, this, [this] {
        fastRatio_->setEnabled(fastMode_->isChecked() && burstOn_->isChecked());
        touch();
    });
    connect(confEnable_, &QCheckBox::toggled, confValue_, &QDoubleSpinBox::setEnabled);
    connect(cleanBlur_, &QCheckBox::toggled, clarity_, &QSlider::setEnabled);
    connect(cleanDedupe_, &QCheckBox::toggled, dupSimilarity_, &QSlider::setEnabled);
    connect(clarity_, &QSlider::valueChanged, this, [this](int value) {
        clarityValue_->setText(QString::number(value));
        touch();
    });
    connect(dupSimilarity_, &QSlider::valueChanged, this, [this](int value) {
        dupValue_->setText(QStringLiteral("%1%").arg(value));
        touch();
    });
    connect(wmSample_, &QCheckBox::toggled, this, [this] { syncWatermarkOptions(); touch(); });
    connect(wmCollage_, qOverload<int>(&QComboBox::currentIndexChanged), this, [this] { syncWatermarkOptions(); touch(); });
    connect(wmMulti_, &QCheckBox::toggled, this, [this] { syncWatermarkOptions(); touch(); });
    auto bindStrength = [this](QSlider* slider, QLabel* label, QCheckBox* gate) {
        slider->setEnabled(gate->isChecked());
        connect(gate, &QCheckBox::toggled, slider, &QSlider::setEnabled);
        connect(slider, &QSlider::valueChanged, this, [this, label](int value) {
            label->setText(QString::number(value / 100.0, 'f', 2));
            touch();
        });
    };
    bindStrength(wmExposureStrength_, wmExposureValue_, wmExposure_);
    bindStrength(wmDenoiseStrength_, wmDenoiseValue_, wmDenoise_);
    bindStrength(wmSharpenStrength_, wmSharpenValue_, wmSharpen_);
    connect(wmDenoise_, &QCheckBox::toggled, wmDenoiseModel_, &QComboBox::setEnabled);
    wmDenoiseModel_->setEnabled(false);
    wmExposureStrength_->setEnabled(false);
    wmSharpenStrength_->setEnabled(false);
    connect(trackPlaces_, &QCheckBox::toggled, trackPlaceText_, &QLineEdit::setEnabled);

    const auto edits = findChildren<QLineEdit*>();
    for (QLineEdit* edit : edits) {
        connect(edit, &QLineEdit::editingFinished, this, [this] { touch(); });
    }
    const auto checks = findChildren<QCheckBox*>();
    for (QCheckBox* box : checks) {
        connect(box, &QCheckBox::toggled, this, [this] { touch(); });
    }

    syncGpsMode();
    syncBurstEnabled();
    syncWatermarkOptions();
    refreshDerived();
    refreshTrackPath();
}

void SettingsForm::refreshDerived() {
    const QString root = outputRoot_->text().trimmed();
    const QString image = imageFolder_->text().trimmed();
    if (root.isEmpty()) {
        legacyPaths_->setVisible(true);
        derivedPaths_->setVisible(false);
        return;
    }
    legacyPaths_->setVisible(false);
    derivedPaths_->setVisible(true);
    if (image.isEmpty()) {
        derivedPaths_->setText(QStringLiteral(
            "已设置输出根目录。请选择「图片文件夹」后，将按该文件夹名自动生成：\n"
            "· <根>/screened_<文件夹名>/\n"
            "· <根>/classification_<文件夹名>/\n"
            "· <根>/reports/"));
        return;
    }
    const SessionPaths paths = sessionPaths(root, image, outputFolder_->text(), cropFolder_->text());
    QStringList lines = {
        QStringLiteral("会话标签「%1」：").arg(sessionSlug(image)),
        QStringLiteral("· %1").arg(paths.outputFolder),
        QStringLiteral("· %1").arg(paths.cropFolder),
        QStringLiteral("· %1").arg(paths.reportsFolder),
    };
    if (comboData(dualFormat_) == QLatin1String("jpg_copy_raw")) {
        lines.insert(3, QStringLiteral("· %1").arg(QDir(paths.outputFolder).filePath(QStringLiteral("Screened_raw_images"))));
    }
    derivedPaths_->setText(lines.join(QLatin1Char('\n')));
}

void SettingsForm::refreshTrackPath() {
    const QString overridePath = trackFolder_->text().trimmed();
    QString effective = overridePath;
    if (effective.isEmpty()) {
        const SessionPaths paths = sessionPaths(
            outputRoot_->text(), imageFolder_->text(), outputFolder_->text(), cropFolder_->text());
        effective = comboData(trackSource_) == QLatin1String("screened") ? paths.screenedImages : paths.cropFolder;
    }
    if (effective.isEmpty()) {
        trackPath_->setText(QStringLiteral("未解析到路径：请设置输出根目录与图片文件夹，或手动浏览指定鸟图目录。"));
        return;
    }
    const QString status = QDir(effective).exists()
        ? QStringLiteral("（目录存在）")
        : QStringLiteral("（目录不存在或为空路径）");
    trackPath_->setText(effective + QLatin1Char('\n') + status);
}

void SettingsForm::syncGpsMode() {
    const bool fixed = gpsFixed_->isChecked();
    location_->setEnabled(fixed);
    latitude_->setEnabled(fixed);
    longitude_->setEnabled(fixed);
    provinceCity_->setEnabled(fixed);
}

void SettingsForm::syncBurstEnabled() {
    const bool on = burstOn_->isChecked();
    timeThreshold_->setEnabled(on);
    keepRatio_->setEnabled(on);
    keepMin_->setEnabled(on);
    focusWeight_->setEnabled(on);
    areaWeight_->setEnabled(on);
    birdDetect_->setEnabled(on);
    fastMode_->setEnabled(on);
    syncBirdDetect();
    fastRatio_->setEnabled(on && fastMode_->isChecked());
}

void SettingsForm::syncBirdDetect() {
    const bool on = burstOn_->isChecked() && birdDetect_->isChecked();
    birdMode_->setEnabled(on);
    eyeDetect_->setEnabled(on);
    if (!on) {
        eyeDetect_->setChecked(false);
    }
}

void SettingsForm::syncWatermarkOptions() {
    const bool collage = comboData(wmCollage_) != QLatin1String("off");
    const bool multi = collage && wmMulti_->isChecked();
    wmMulti_->setEnabled(collage);
    wmOneAll_->setEnabled(collage && !multi);
    wmDate_->setEnabled(!collage);
    wmCamera_->setEnabled(!collage);
    wmStyle_->setEnabled(!collage);
    wmSpecies_->setEnabled(!multi);
    wmSample_->setEnabled(!multi);
    wmSampleCount_->setEnabled(wmSample_->isChecked() && !multi);
}

void SettingsForm::setBusy(bool busy) {
    startButton_->setEnabled(!busy);
    stopButton_->setEnabled(busy);
}

void SettingsForm::setModelStatus(const QString& text) {
    modelStatus_->setText(text);
}

void SettingsForm::applyConfig(const QJsonObject& config) {
    loading_ = true;
    auto text = [&](QLineEdit* edit, const char* key, const QString& fallback = QString()) {
        edit->setText(config.value(QLatin1String(key)).toString(edit->text().isEmpty() ? fallback : edit->text()));
    };
    auto flag = [&](QCheckBox* box, const char* key, bool fallback) {
        box->setChecked(config.contains(QLatin1String(key)) ? config.value(QLatin1String(key)).toBool() : fallback);
    };
    text(imageFolder_, "image_folder");
    setCombo(dualFormat_, config.value(QLatin1String("dual_format_mode")).toString(QStringLiteral("off")));
    text(outputRoot_, "output_root_folder");
    text(outputFolder_, "output_folder", QStringLiteral("./outputs"));
    text(cropFolder_, "crop_output_folder", QStringLiteral("./crops"));
    flag(gpsWrite_, "enable_gps_write", true);
    const QString gpsMode = config.value(QLatin1String("gps_write_mode")).toString(QStringLiteral("fixed"));
    (gpsMode == QLatin1String("gpx") ? gpsGpx_ : gpsFixed_)->setChecked(true);
    if (config.contains(QLatin1String("location_name"))) {
        location_->setCurrentText(config.value(QLatin1String("location_name")).toString());
    }
    if (config.contains(QLatin1String("gps_latitude"))) {
        latitude_->setText(QString::number(config.value(QLatin1String("gps_latitude")).toDouble()));
    }
    if (config.contains(QLatin1String("gps_longitude"))) {
        longitude_->setText(QString::number(config.value(QLatin1String("gps_longitude")).toDouble()));
    }
    provinceCity_->setText(config.value(QLatin1String("province")).toString()
                           + QLatin1Char(' ')
                           + config.value(QLatin1String("city")).toString());
    gpxList_->clear();
    const QJsonArray gpx = config.value(QLatin1String("gpx_file_paths")).toArray();
    for (const QJsonValue& item : gpx) {
        if (!item.toString().isEmpty()) {
            gpxList_->addItem(item.toString());
        }
    }
    if (gpx.isEmpty() && !config.value(QLatin1String("gpx_file_path")).toString().isEmpty()) {
        gpxList_->addItem(config.value(QLatin1String("gpx_file_path")).toString());
    }
    exifTz_->setCurrentText(config.value(QLatin1String("gpx_match_exif_tz")).toString(QStringLiteral("Asia/Shanghai")));
    gpxTz_->setCurrentText(config.value(QLatin1String("gpx_match_gpx_tz")).toString(QStringLiteral("UTC")));
    flag(gpxToScreened_, "gpx_apply_to_screened", true);
    flag(burstOn_, "enable_burst_detection", true);
    if (config.contains(QLatin1String("time_threshold"))) {
        timeThreshold_->setValue(config.value(QLatin1String("time_threshold")).toDouble());
    }
    if (config.contains(QLatin1String("burst_keep_ratio"))) {
        keepRatio_->setValue(config.value(QLatin1String("burst_keep_ratio")).toDouble());
    }
    if (config.contains(QLatin1String("burst_keep_min"))) {
        keepMin_->setValue(config.value(QLatin1String("burst_keep_min")).toInt());
    }
    if (config.contains(QLatin1String("focus_score_weight"))) {
        focusWeight_->setValue(config.value(QLatin1String("focus_score_weight")).toDouble());
    }
    if (config.contains(QLatin1String("area_score_weight"))) {
        areaWeight_->setValue(config.value(QLatin1String("area_score_weight")).toDouble());
    }
    flag(birdDetect_, "use_bird_detection", true);
    setCombo(birdMode_, config.value(QLatin1String("bird_detect_mode")).toString(QStringLiteral("normal")));
    flag(eyeDetect_, "use_eye_detection", false);
    flag(fastMode_, "use_fast_mode", true);
    if (config.contains(QLatin1String("fast_mode_sample_ratio"))) {
        fastRatio_->setValue(config.value(QLatin1String("fast_mode_sample_ratio")).toDouble());
    }
    flag(speciesOn_, "enable_species_detection", true);
    text(speciesFolder_, "species_input_folder");
    const bool local = config.contains(QLatin1String("use_local_model"))
        ? config.value(QLatin1String("use_local_model")).toBool()
        : true;
    (local ? localModel_ : doubaoModel_)->setChecked(true);
    setCombo(speciesModel_, config.value(QLatin1String("local_species_model")).toString(QStringLiteral("resnet34")));
    text(modelDir_, "model_dir");
    flag(dxo_, "enable_dxo_pureraw", false);
    setCombo(geoMode_, config.value(QLatin1String("species_geo_mode")).toString(QStringLiteral("auto")));
    flag(confEnable_, "species_conf_threshold_enabled", false);
    if (config.contains(QLatin1String("min_species_accept_confidence"))) {
        confValue_->setValue(config.value(QLatin1String("min_species_accept_confidence")).toDouble());
    }
    flag(cleanBefore_, "enable_image_clean_before_species", false);
    flag(cleanBlur_, "image_clean_remove_blurry", true);
    if (config.contains(QLatin1String("image_clean_min_clarity"))) {
        clarity_->setValue(config.value(QLatin1String("image_clean_min_clarity")).toInt());
    }
    flag(cleanDedupe_, "image_clean_dedupe", true);
    if (config.contains(QLatin1String("image_clean_dup_similarity"))) {
        dupSimilarity_->setValue(config.value(QLatin1String("image_clean_dup_similarity")).toInt());
    }
    text(cleanFolder_, "image_clean_folder");
    flag(watermarkOn_, "enable_watermark_generation", false);
    text(wmInput_, "watermark_input_folder");
    text(wmOutput_, "watermark_output_folder", QStringLiteral("./watermarked"));
    text(wmLogo_, "wm_logo_path");
    if (config.contains(QLatin1String("wm_logo_width_ratio"))) {
        wmLogoRatio_->setValue(config.value(QLatin1String("wm_logo_width_ratio")).toDouble());
    }
    setCombo(wmStyle_, config.value(QLatin1String("wm_watermark_style")).toString(QStringLiteral("frame")));
    flag(wmLocation_, "wm_enable_location", true);
    flag(wmGpsCity_, "wm_use_gps_city", true);
    text(wmLocationText_, "wm_location_text");
    flag(wmDate_, "wm_enable_date", true);
    flag(wmSpecies_, "wm_enable_species", true);
    flag(wmCamera_, "wm_enable_camera", true);
    flag(wmSample_, "wm_random_per_species", false);
    if (config.contains(QLatin1String("wm_random_per_species_count"))) {
        wmSampleCount_->setValue(config.value(QLatin1String("wm_random_per_species_count")).toInt());
    }
    setCombo(wmCollage_, config.value(QLatin1String("wm_collage_mode")).toString(QStringLiteral("off")));
    flag(wmOneAll_, "wm_collage_allow_one_all", false);
    flag(wmMulti_, "wm_collage_multi_species", false);
    flag(wmExposure_, "wm_enable_ai_exposure", false);
    flag(wmDenoise_, "wm_enable_ai_denoise", false);
    flag(wmSharpen_, "wm_enable_ai_sharpen", false);
    if (config.contains(QLatin1String("wm_ai_exposure_strength"))) {
        wmExposureStrength_->setValue(int(config.value(QLatin1String("wm_ai_exposure_strength")).toDouble() * 100));
    }
    if (config.contains(QLatin1String("wm_ai_denoise_strength"))) {
        wmDenoiseStrength_->setValue(int(config.value(QLatin1String("wm_ai_denoise_strength")).toDouble() * 100));
    }
    if (config.contains(QLatin1String("wm_ai_sharpen_strength"))) {
        wmSharpenStrength_->setValue(int(config.value(QLatin1String("wm_ai_sharpen_strength")).toDouble() * 100));
    }
    setCombo(wmDenoiseModel_, config.value(QLatin1String("wm_ai_denoise_model")).toString(QStringLiteral("realesrgan")));
    flag(trackOn_, "enable_track_map_auto", false);
    flag(trackGpx_, "track_map_use_gpx", true);
    flag(trackExif_, "track_map_use_exif", true);
    setCombo(trackSource_, config.value(QLatin1String("track_map_photo_source")).toString(QStringLiteral("classification")));
    text(trackFolder_, "track_map_photo_folder_override");
    if (config.contains(QLatin1String("track_map_radius_km"))) {
        trackRadius_->setValue(config.value(QLatin1String("track_map_radius_km")).toDouble());
    }
    setCombo(trackBasemap_, config.value(QLatin1String("track_map_basemap_style")).toString(QStringLiteral("normal")));
    flag(trackElevation_, "track_map_include_elevation", true);
    flag(trackRotate_, "track_map_adaptive_rotate", false);
    flag(trackPlaces_, "track_map_show_key_places", false);
    text(trackPlaceText_, "track_map_key_places");
    flag(exportOn_, "enable_record_export_auto", false);
    text(exportClass_, "record_export_classification_folder");
    text(exportOut_, "record_export_output_folder");
    flag(exportEbird_, "record_export_ebird", true);
    flag(exportBirdreport_, "record_export_birdreport", true);
    text(exportCountry_, "record_export_ebird_country", QStringLiteral("CN"));
    text(exportState_, "record_export_ebird_state", QStringLiteral("FJ"));
    flag(exportCount_, "record_export_count_individuals", true);
    auto restoreSection = [&](QWidget* outer, const char* key, bool fallback) {
        const bool on = config.contains(QLatin1String(key)) ? config.value(QLatin1String(key)).toBool() : fallback;
        if (QPushButton* toggle = outer->findChild<QPushButton*>()) {
            if (toggle->isCheckable()) {
                toggle->setChecked(on);
            }
        }
    };
    restoreSection(geoBody_, "ui_section_expanded_geo", true);
    restoreSection(burstBody_, "ui_section_expanded_burst", true);
    restoreSection(speciesBody_, "ui_section_expanded_species", true);
    restoreSection(watermarkBody_, "ui_section_expanded_watermark", false);
    restoreSection(trackBody_, "ui_section_expanded_track_map", false);
    restoreSection(exportBody_, "ui_section_expanded_export", true);
    syncGpsMode();
    syncBurstEnabled();
    syncWatermarkOptions();
    refreshDerived();
    refreshTrackPath();
    loading_ = false;
    clarityValue_->setText(QString::number(clarity_->value()));
    dupValue_->setText(QStringLiteral("%1%").arg(dupSimilarity_->value()));
    confValue_->setEnabled(confEnable_->isChecked());
    if (!modelDir_->text().isEmpty()) {
        emit modelDirChanged(modelDir_->text());
    }
}

QJsonObject SettingsForm::collectConfig() const {
    QJsonObject config;
    auto putText = [&](const char* key, const QLineEdit* edit) { config.insert(QLatin1String(key), edit->text()); };
    auto putFlag = [&](const char* key, const QCheckBox* box) { config.insert(QLatin1String(key), box->isChecked()); };
    putText("image_folder", imageFolder_);
    config.insert(QLatin1String("dual_format_mode"), comboData(dualFormat_));
    putText("output_root_folder", outputRoot_);
    putText("output_folder", outputFolder_);
    putText("crop_output_folder", cropFolder_);
    putFlag("enable_gps_write", gpsWrite_);
    config.insert(QLatin1String("gps_write_mode"), gpsGpx_->isChecked() ? QStringLiteral("gpx") : QStringLiteral("fixed"));
    config.insert(QLatin1String("location_name"), location_->currentText());
    config.insert(QLatin1String("gps_latitude"), latitude_->text().toDouble());
    config.insert(QLatin1String("gps_longitude"), longitude_->text().toDouble());
    QJsonArray gpx;
    for (int i = 0; i < gpxList_->count(); ++i) {
        gpx.append(gpxList_->item(i)->text());
    }
    config.insert(QLatin1String("gpx_file_paths"), gpx);
    config.insert(QLatin1String("gpx_file_path"), gpx.isEmpty() ? QString() : gpx.first().toString());
    config.insert(QLatin1String("gpx_match_exif_tz"), exifTz_->currentText());
    config.insert(QLatin1String("gpx_match_gpx_tz"), gpxTz_->currentText());
    putFlag("gpx_apply_to_screened", gpxToScreened_);
    putFlag("enable_burst_detection", burstOn_);
    config.insert(QLatin1String("time_threshold"), timeThreshold_->value());
    config.insert(QLatin1String("burst_keep_ratio"), keepRatio_->value());
    config.insert(QLatin1String("burst_keep_min"), keepMin_->value());
    config.insert(QLatin1String("focus_score_weight"), focusWeight_->value());
    config.insert(QLatin1String("area_score_weight"), areaWeight_->value());
    putFlag("use_bird_detection", birdDetect_);
    config.insert(QLatin1String("bird_detect_mode"), comboData(birdMode_));
    putFlag("use_eye_detection", eyeDetect_);
    putFlag("use_fast_mode", fastMode_);
    config.insert(QLatin1String("fast_mode_sample_ratio"), fastRatio_->value());
    putFlag("enable_species_detection", speciesOn_);
    putText("species_input_folder", speciesFolder_);
    config.insert(QLatin1String("use_local_model"), localModel_->isChecked());
    config.insert(QLatin1String("local_species_model"), comboData(speciesModel_));
    putText("model_dir", modelDir_);
    putFlag("enable_dxo_pureraw", dxo_);
    config.insert(QLatin1String("species_geo_mode"), comboData(geoMode_));
    putFlag("species_conf_threshold_enabled", confEnable_);
    config.insert(QLatin1String("min_species_accept_confidence"), confValue_->value());
    putFlag("enable_image_clean_before_species", cleanBefore_);
    putFlag("image_clean_remove_blurry", cleanBlur_);
    config.insert(QLatin1String("image_clean_min_clarity"), clarity_->value());
    putFlag("image_clean_dedupe", cleanDedupe_);
    config.insert(QLatin1String("image_clean_dup_similarity"), dupSimilarity_->value());
    putText("image_clean_folder", cleanFolder_);
    putFlag("enable_watermark_generation", watermarkOn_);
    putText("watermark_input_folder", wmInput_);
    putText("watermark_output_folder", wmOutput_);
    putText("wm_logo_path", wmLogo_);
    config.insert(QLatin1String("wm_logo_width_ratio"), wmLogoRatio_->value());
    config.insert(QLatin1String("wm_watermark_style"), comboData(wmStyle_));
    putFlag("wm_enable_location", wmLocation_);
    putFlag("wm_use_gps_city", wmGpsCity_);
    putText("wm_location_text", wmLocationText_);
    putFlag("wm_enable_date", wmDate_);
    putFlag("wm_enable_species", wmSpecies_);
    putFlag("wm_enable_camera", wmCamera_);
    putFlag("wm_random_per_species", wmSample_);
    config.insert(QLatin1String("wm_random_per_species_count"), wmSampleCount_->value());
    config.insert(QLatin1String("wm_collage_mode"), comboData(wmCollage_));
    putFlag("wm_collage_allow_one_all", wmOneAll_);
    putFlag("wm_collage_multi_species", wmMulti_);
    putFlag("wm_enable_ai_exposure", wmExposure_);
    putFlag("wm_enable_ai_denoise", wmDenoise_);
    putFlag("wm_enable_ai_sharpen", wmSharpen_);
    config.insert(QLatin1String("wm_ai_exposure_strength"), wmExposureStrength_->value() / 100.0);
    config.insert(QLatin1String("wm_ai_denoise_strength"), wmDenoiseStrength_->value() / 100.0);
    config.insert(QLatin1String("wm_ai_sharpen_strength"), wmSharpenStrength_->value() / 100.0);
    config.insert(QLatin1String("wm_ai_denoise_model"), comboData(wmDenoiseModel_));
    putFlag("enable_track_map_auto", trackOn_);
    putFlag("track_map_use_gpx", trackGpx_);
    putFlag("track_map_use_exif", trackExif_);
    config.insert(QLatin1String("track_map_photo_source"), comboData(trackSource_));
    putText("track_map_photo_folder_override", trackFolder_);
    config.insert(QLatin1String("track_map_radius_km"), trackRadius_->value());
    config.insert(QLatin1String("track_map_basemap_style"), comboData(trackBasemap_));
    putFlag("track_map_include_elevation", trackElevation_);
    putFlag("track_map_adaptive_rotate", trackRotate_);
    putFlag("track_map_show_key_places", trackPlaces_);
    putText("track_map_key_places", trackPlaceText_);
    putFlag("enable_record_export_auto", exportOn_);
    putText("record_export_classification_folder", exportClass_);
    putText("record_export_output_folder", exportOut_);
    putFlag("record_export_ebird", exportEbird_);
    putFlag("record_export_birdreport", exportBirdreport_);
    putText("record_export_ebird_country", exportCountry_);
    putText("record_export_ebird_state", exportState_);
    putFlag("record_export_count_individuals", exportCount_);
    config.insert(QLatin1String("ui_section_expanded_geo"), sectionExpanded(geoBody_));
    config.insert(QLatin1String("ui_section_expanded_burst"), sectionExpanded(burstBody_));
    config.insert(QLatin1String("ui_section_expanded_species"), sectionExpanded(speciesBody_));
    config.insert(QLatin1String("ui_section_expanded_watermark"), sectionExpanded(watermarkBody_));
    config.insert(QLatin1String("ui_section_expanded_track_map"), sectionExpanded(trackBody_));
    config.insert(QLatin1String("ui_section_expanded_export"), sectionExpanded(exportBody_));
    return config;
}

}  // namespace birdy
