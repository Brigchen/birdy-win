#pragma once

#include <QList>
#include <QString>

class QWidget;

namespace birdy {

struct Section {
    QWidget* outer = nullptr;
    QWidget* body = nullptr;
};

Section makeCard(const QString& title);
Section makeCollapsible(const QString& title, bool expanded, const QList<QWidget*>& headerExtras = {});
bool sectionExpanded(const QWidget* outer);

}  // namespace birdy
