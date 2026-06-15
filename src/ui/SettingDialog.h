#pragma once

#include <QDialog>

class QSettings;
class QLineEdit;
class QSpinBox;


class SettingDialog : public QDialog
{
    Q_OBJECT

public:
    SettingDialog(QWidget *parent=nullptr);

    static QSettings appSettings();
private:
    QLineEdit *m_productIdEdit=nullptr;
    QLineEdit *m_deviceNameEdit=nullptr;
    QLineEdit *m_authorizationEdit=nullptr;
    QSpinBox *m_rawPageSizeSpin=nullptr;

    void loadSettings();
    void saveSettings();

    static QString settingsFilePath();


};
