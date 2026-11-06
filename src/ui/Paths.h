#pragma once

#include <QDir>
#include <QFileInfo>
#include <QString>

namespace birdy {

struct SessionPaths {
    QString outputFolder;
    QString cropFolder;
    QString reportsFolder;
    QString screenedImages;
};

inline QString sessionSlug(const QString& imageFolder) {
    const QString base = QFileInfo(QDir::cleanPath(imageFolder.trimmed())).fileName();
    if (base.isEmpty() || base == QLatin1String(".") || base == QLatin1String("..")) {
        return QStringLiteral("session");
    }
    QString out;
    out.reserve(base.size());
    const QString bad = QStringLiteral("<>:\"/\\|?*");
    for (const QChar ch : base) {
        if (ch.isSpace() || ch.unicode() < 32 || bad.contains(ch)) {
            out.append(QLatin1Char('_'));
        } else {
            out.append(ch);
        }
    }
    while (out.startsWith(QLatin1Char('.')) || out.startsWith(QLatin1Char('_'))) {
        out.remove(0, 1);
    }
    while (out.endsWith(QLatin1Char('.')) || out.endsWith(QLatin1Char('_'))) {
        out.chop(1);
    }
    return out.isEmpty() ? QStringLiteral("session") : out;
}

inline SessionPaths sessionPaths(const QString& outputRoot, const QString& imageFolder,
                                  const QString& manualOutput, const QString& manualCrop) {
    SessionPaths paths;
    const QString root = outputRoot.trimmed();
    if (!root.isEmpty()) {
        const QString slug = sessionSlug(imageFolder);
        paths.outputFolder = QDir(root).filePath(QStringLiteral("screened_") + slug);
        paths.cropFolder = QDir(root).filePath(QStringLiteral("classification_") + slug);
        paths.reportsFolder = QDir(root).filePath(QStringLiteral("reports"));
        paths.screenedImages = QDir(paths.outputFolder).filePath(QStringLiteral("Screened_images"));
        return paths;
    }
    paths.outputFolder = manualOutput.trimmed();
    paths.cropFolder = manualCrop.trimmed();
    if (!paths.outputFolder.isEmpty()) {
        paths.reportsFolder = QDir(paths.outputFolder).filePath(QStringLiteral("reports"));
        paths.screenedImages = QDir(paths.outputFolder).filePath(QStringLiteral("Screened_images"));
    }
    return paths;
}

}  // namespace birdy
