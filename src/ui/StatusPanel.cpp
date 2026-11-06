#include "ui/StatusPanel.h"

#include "ui/Cards.h"

#include <QHeaderView>
#include <QLabel>
#include <QPixmap>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QTableWidget>
#include <QTextEdit>
#include <QVBoxLayout>

namespace birdy {
namespace {

QScrollArea* verticalScroll(QWidget* parent) {
    auto* scroll = new QScrollArea(parent);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    return scroll;
}

}  // namespace

StatusPanel::StatusPanel(const QString& copyrightLogoPath, QWidget* parent) : QWidget(parent) {
    auto* scroll = verticalScroll(this);
    auto* panel = new QWidget;
    auto* layout = new QVBoxLayout(panel);
    layout->setSpacing(10);
    layout->setContentsMargins(0, 0, 8, 0);

    const Section card = makeCard(QStringLiteral("📊 处理进度"));
    auto* body = new QVBoxLayout(card.body);
    body->setSpacing(10);
    body->setContentsMargins(12, 10, 12, 12);

    progress_ = new QProgressBar;
    progress_->setRange(0, 100);
    progress_->setValue(0);
    body->addWidget(progress_);

    auto* timeRow = new QHBoxLayout;
    elapsed_ = new QLabel(QStringLiteral("已用时间：—"));
    eta_ = new QLabel(QStringLiteral("预计剩余：—"));
    for (QLabel* label : {elapsed_, eta_}) {
        label->setStyleSheet(QStringLiteral("font-size: 10pt; color: #444444;"));
        timeRow->addWidget(label);
    }
    timeRow->addStretch(1);
    body->addLayout(timeRow);

    auto* statusTitle = new QLabel(QStringLiteral("状态信息:"));
    statusTitle->setStyleSheet(QStringLiteral("font-size: 10pt; font-weight: bold; color: #333333;"));
    body->addWidget(statusTitle);
    log_ = new QTextEdit;
    log_->setReadOnly(true);
    log_->setMinimumHeight(160);
    body->addWidget(log_);

    auto* statsTitle = new QLabel(QStringLiteral("处理统计:"));
    statsTitle->setStyleSheet(statusTitle->styleSheet());
    body->addWidget(statsTitle);
    stats_ = new QTableWidget(0, 2);
    stats_->setHorizontalHeaderLabels({QStringLiteral("项目"), QStringLiteral("数值")});
    stats_->horizontalHeader()->setStretchLastSection(true);
    stats_->setMinimumHeight(120);
    stats_->verticalHeader()->setVisible(false);
    body->addWidget(stats_);

    auto* clear = new QPushButton(QStringLiteral("清空日志"));
    connect(clear, &QPushButton::clicked, this, &StatusPanel::clearLog);
    body->addWidget(clear);
    layout->addWidget(card.outer);

    auto* brand = new QVBoxLayout;
    brand->setSpacing(4);
    brand->setContentsMargins(0, 10, 0, 0);
    if (!copyrightLogoPath.isEmpty()) {
        const QPixmap pixmap(copyrightLogoPath);
        if (!pixmap.isNull()) {
            auto* logo = new QLabel;
            logo->setAlignment(Qt::AlignCenter);
            const qreal dpr = logo->devicePixelRatioF();
            QPixmap scaled = pixmap.scaledToHeight(qMax(1, int(44 * dpr)), Qt::SmoothTransformation);
            scaled.setDevicePixelRatio(dpr);
            logo->setPixmap(scaled);
            logo->setFixedHeight(44);
            brand->addWidget(logo);
        }
    }
    auto* copyright = new QLabel(QStringLiteral("© 2026 brigchen@gmail.com"));
    copyright->setAlignment(Qt::AlignCenter);
    copyright->setStyleSheet(QStringLiteral("color: #666666; font-size: 10pt;"));
    auto* license = new QLabel(QStringLiteral("基于开源协议，仅限爱好者、公益、科研等非盈利用途，请勿用于商业用途"));
    license->setAlignment(Qt::AlignCenter);
    license->setWordWrap(true);
    license->setStyleSheet(QStringLiteral("color: #999999; font-size: 9pt;"));
    brand->addWidget(copyright);
    brand->addWidget(license);
    layout->addLayout(brand);
    layout->addStretch(1);

    scroll->setWidget(panel);
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->addWidget(scroll);
}

void StatusPanel::appendLog(const QString& line) {
    log_->append(line);
}

void StatusPanel::clearLog() {
    log_->clear();
}

void StatusPanel::setProgress(int value) {
    progress_->setValue(value);
}

void StatusPanel::setElapsed(const QString& text) {
    elapsed_->setText(text);
}

void StatusPanel::setEta(const QString& text) {
    eta_->setText(text);
}

}  // namespace birdy
