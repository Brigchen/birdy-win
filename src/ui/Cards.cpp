#include "ui/Cards.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVariant>
#include <QVBoxLayout>

namespace birdy {
namespace {

constexpr char kExpandedProp[] = "birdyExpanded";

void styleCard(QWidget* outer) {
    outer->setObjectName(QStringLiteral("birdyCard"));
    outer->setStyleSheet(QStringLiteral(
        "#birdyCard { background-color: #FFFFFF; border-radius: 8px; }"));
}

QLabel* titleLabel(const QString& title) {
    auto* label = new QLabel(title);
    label->setStyleSheet(QStringLiteral(
        "background-color: #FFFFFF; color: #333333; font-weight: bold; font-size: 11pt;"
        "padding: 6px 12px; border-bottom: 1px solid #F0F0F0;"));
    return label;
}

}  // namespace

Section makeCard(const QString& title) {
    Section section;
    section.outer = new QWidget;
    styleCard(section.outer);
    auto* layout = new QVBoxLayout(section.outer);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(titleLabel(title));
    section.body = new QWidget(section.outer);
    section.body->setStyleSheet(QStringLiteral("background-color: #FFFFFF;"));
    layout->addWidget(section.body);
    return section;
}

Section makeCollapsible(const QString& title, bool expanded, const QList<QWidget*>& headerExtras) {
    Section section;
    section.outer = new QWidget;
    styleCard(section.outer);
    section.outer->setProperty(kExpandedProp, expanded);
    auto* layout = new QVBoxLayout(section.outer);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto* header = new QWidget(section.outer);
    header->setStyleSheet(QStringLiteral(
        "background-color: #FFFFFF; border-bottom: 1px solid #F0F0F0;"));
    auto* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(4, 6, 10, 6);
    headerLayout->setSpacing(6);

    auto* toggle = new QPushButton(expanded ? QStringLiteral("▼") : QStringLiteral("▶"), header);
    toggle->setCheckable(true);
    toggle->setChecked(expanded);
    toggle->setFixedWidth(32);
    toggle->setFlat(true);
    toggle->setCursor(Qt::PointingHandCursor);
    toggle->setStyleSheet(QStringLiteral(
        "QPushButton { border: none; font-size: 11pt; font-weight: bold;"
        "background: transparent; color: #333; padding: 0; min-height: 0; }"
        "QPushButton:hover { background-color: #F0F0F0; border-radius: 4px; }"));

    auto* titleWidget = new QLabel(title, header);
    titleWidget->setStyleSheet(QStringLiteral(
        "background: transparent; color: #333333; font-weight: bold; font-size: 11pt;"));

    headerLayout->addWidget(toggle);
    headerLayout->addWidget(titleWidget);
    headerLayout->addStretch(1);
    for (QWidget* extra : headerExtras) {
        headerLayout->addWidget(extra);
    }
    layout->addWidget(header);

    section.body = new QWidget(section.outer);
    section.body->setStyleSheet(QStringLiteral("background-color: #FFFFFF;"));
    section.body->setVisible(expanded);
    layout->addWidget(section.body);

    QObject::connect(toggle, &QPushButton::toggled, section.outer, [section, toggle](bool on) {
        section.body->setVisible(on);
        section.outer->setProperty(kExpandedProp, on);
        toggle->setText(on ? QStringLiteral("▼") : QStringLiteral("▶"));
    });
    return section;
}

bool sectionExpanded(const QWidget* outer) {
    return outer != nullptr && outer->property(kExpandedProp).toBool();
}

}  // namespace birdy
