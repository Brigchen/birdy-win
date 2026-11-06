#pragma once

#include <QJsonObject>
#include <QWidget>

class QButtonGroup;
class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QRadioButton;
class QSlider;
class QSpinBox;
class QWidget;

namespace birdy {

class SettingsForm : public QWidget {
    Q_OBJECT
public:
    explicit SettingsForm(QWidget* parent = nullptr);

    void applyConfig(const QJsonObject& config);
    QJsonObject collectConfig() const;
    void setBusy(bool busy);
    void setModelStatus(const QString& text);

signals:
    void logMessage(const QString& text);
    void configEdited();
    void modelDirChanged(const QString& dir);
    void startRequested();
    void stopRequested();

private:
    void buildUi();
    void refreshDerived();
    void refreshTrackPath();
    void syncGpsMode();
    void syncBurstEnabled();
    void syncBirdDetect();
    void syncWatermarkOptions();
    void browseDirectory(QLineEdit* target, const QString& title);
    void browseFile(QLineEdit* target, const QString& title, const QString& filter);
    void notePending(const QString& name);
    void touch();
    void setCombo(QComboBox* box, const QString& data) const;
    static void fillCombo(QComboBox* box, const QList<QPair<QString, QString>>& items);

    bool loading_ = false;

    QLineEdit* imageFolder_ = nullptr;
    QComboBox* dualFormat_ = nullptr;
    QLineEdit* outputRoot_ = nullptr;
    QLabel* derivedPaths_ = nullptr;
    QWidget* legacyPaths_ = nullptr;
    QLineEdit* outputFolder_ = nullptr;
    QLineEdit* cropFolder_ = nullptr;

    QCheckBox* gpsWrite_ = nullptr;
    QRadioButton* gpsFixed_ = nullptr;
    QRadioButton* gpsGpx_ = nullptr;
    QComboBox* location_ = nullptr;
    QLineEdit* latitude_ = nullptr;
    QLineEdit* longitude_ = nullptr;
    QLineEdit* provinceCity_ = nullptr;
    QListWidget* gpxList_ = nullptr;
    QComboBox* exifTz_ = nullptr;
    QComboBox* gpxTz_ = nullptr;
    QCheckBox* gpxToScreened_ = nullptr;

    QCheckBox* burstOn_ = nullptr;
    QDoubleSpinBox* timeThreshold_ = nullptr;
    QDoubleSpinBox* keepRatio_ = nullptr;
    QSpinBox* keepMin_ = nullptr;
    QDoubleSpinBox* focusWeight_ = nullptr;
    QDoubleSpinBox* areaWeight_ = nullptr;
    QCheckBox* birdDetect_ = nullptr;
    QComboBox* birdMode_ = nullptr;
    QCheckBox* eyeDetect_ = nullptr;
    QCheckBox* fastMode_ = nullptr;
    QDoubleSpinBox* fastRatio_ = nullptr;

    QCheckBox* speciesOn_ = nullptr;
    QLineEdit* speciesFolder_ = nullptr;
    QRadioButton* localModel_ = nullptr;
    QRadioButton* doubaoModel_ = nullptr;
    QComboBox* speciesModel_ = nullptr;
    QLineEdit* modelDir_ = nullptr;
    QLabel* modelStatus_ = nullptr;
    QCheckBox* dxo_ = nullptr;
    QComboBox* geoMode_ = nullptr;
    QCheckBox* confEnable_ = nullptr;
    QDoubleSpinBox* confValue_ = nullptr;
    QCheckBox* cleanBefore_ = nullptr;
    QCheckBox* cleanBlur_ = nullptr;
    QSlider* clarity_ = nullptr;
    QLabel* clarityValue_ = nullptr;
    QCheckBox* cleanDedupe_ = nullptr;
    QSlider* dupSimilarity_ = nullptr;
    QLabel* dupValue_ = nullptr;
    QLineEdit* cleanFolder_ = nullptr;

    QCheckBox* watermarkOn_ = nullptr;
    QLineEdit* wmInput_ = nullptr;
    QLineEdit* wmOutput_ = nullptr;
    QLineEdit* wmLogo_ = nullptr;
    QDoubleSpinBox* wmLogoRatio_ = nullptr;
    QComboBox* wmStyle_ = nullptr;
    QCheckBox* wmLocation_ = nullptr;
    QCheckBox* wmGpsCity_ = nullptr;
    QLineEdit* wmLocationText_ = nullptr;
    QCheckBox* wmDate_ = nullptr;
    QCheckBox* wmSpecies_ = nullptr;
    QCheckBox* wmCamera_ = nullptr;
    QCheckBox* wmSample_ = nullptr;
    QSpinBox* wmSampleCount_ = nullptr;
    QComboBox* wmCollage_ = nullptr;
    QCheckBox* wmOneAll_ = nullptr;
    QCheckBox* wmMulti_ = nullptr;
    QCheckBox* wmExposure_ = nullptr;
    QSlider* wmExposureStrength_ = nullptr;
    QLabel* wmExposureValue_ = nullptr;
    QCheckBox* wmDenoise_ = nullptr;
    QComboBox* wmDenoiseModel_ = nullptr;
    QSlider* wmDenoiseStrength_ = nullptr;
    QLabel* wmDenoiseValue_ = nullptr;
    QCheckBox* wmSharpen_ = nullptr;
    QSlider* wmSharpenStrength_ = nullptr;
    QLabel* wmSharpenValue_ = nullptr;

    QCheckBox* trackOn_ = nullptr;
    QCheckBox* trackGpx_ = nullptr;
    QCheckBox* trackExif_ = nullptr;
    QComboBox* trackSource_ = nullptr;
    QLineEdit* trackFolder_ = nullptr;
    QLabel* trackPath_ = nullptr;
    QDoubleSpinBox* trackRadius_ = nullptr;
    QComboBox* trackBasemap_ = nullptr;
    QCheckBox* trackElevation_ = nullptr;
    QCheckBox* trackRotate_ = nullptr;
    QCheckBox* trackPlaces_ = nullptr;
    QLineEdit* trackPlaceText_ = nullptr;

    QCheckBox* exportOn_ = nullptr;
    QLineEdit* exportClass_ = nullptr;
    QLineEdit* exportOut_ = nullptr;
    QCheckBox* exportEbird_ = nullptr;
    QCheckBox* exportBirdreport_ = nullptr;
    QLineEdit* exportCountry_ = nullptr;
    QLineEdit* exportState_ = nullptr;
    QCheckBox* exportCount_ = nullptr;

    QWidget* geoBody_ = nullptr;
    QWidget* burstBody_ = nullptr;
    QWidget* speciesBody_ = nullptr;
    QWidget* watermarkBody_ = nullptr;
    QWidget* trackBody_ = nullptr;
    QWidget* exportBody_ = nullptr;

    QPushButton* startButton_ = nullptr;
    QPushButton* stopButton_ = nullptr;
};

}  // namespace birdy
