#include "SettingDialog.h"

#include <QLineEdit>
#include <QSpinBox>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QSettings>
#include <QCoreApplication>
#include <QDir>
#include <QIcon>

SettingDialog::SettingDialog(QWidget *parent)
    :QDialog(parent)
{
    setWindowTitle("设置");
    setWindowIcon(QIcon(":/icon/icons/setting.png"));

    m_productIdEdit=new QLineEdit(this);
    m_deviceNameEdit=new QLineEdit(this);
    m_authorizationEdit=new QLineEdit(this);

    m_rawPageSizeSpin=new QSpinBox(this);
    m_rawPageSizeSpin->setRange(10,100);
    m_rawPageSizeSpin->setSingleStep(10);

    //表单布局
    QFormLayout *formLayout=new QFormLayout();
    formLayout->addRow("Product ID",m_productIdEdit);
    formLayout->addRow("Device Name",m_deviceNameEdit);
    formLayout->addRow("token",m_authorizationEdit);
    formLayout->addRow("Raw Data 每页条数",m_rawPageSizeSpin);

    auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,this);
    QVBoxLayout *mainLayout=new QVBoxLayout(this);
    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(buttons);

    loadSettings();

    connect(buttons,&QDialogButtonBox::accepted,this,[=](){
        saveSettings();
        accept();
    });

    connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);

}

QSettings SettingDialog::appSettings()
{
    return QSettings(settingsFilePath(),QSettings::IniFormat);
}

void SettingDialog::loadSettings()
{
    QSettings settings=appSettings();

    m_productIdEdit->setText(settings.value("onenet/productId", "pmJ97H965j").toString());
    m_deviceNameEdit->setText(settings.value("onenet/deviceName", "dev1").toString());
    m_authorizationEdit->setText(settings.value("onenet/authorization","version=2018-10-31&res=products%2FpmJ97H965j%2Fdevices%2Fdev1&et=1806050344&method=md5&sign=MBQgX0y0Megx1%2Bt8Zb3FkQ%3D%3D").toString());
    m_rawPageSizeSpin->setValue(settings.value("rawData/pageSize", 20).toInt());
    
}

void SettingDialog::saveSettings()
{
    QSettings settings=appSettings();

    settings.setValue("onenet/productId", m_productIdEdit->text().trimmed());
    settings.setValue("onenet/deviceName", m_deviceNameEdit->text().trimmed());
    settings.setValue("onenet/authorization",m_authorizationEdit->text().trimmed());
    settings.setValue("rawData/pageSize", m_rawPageSizeSpin->value());

    settings.sync();
}

QString SettingDialog::settingsFilePath()
{

    QString configDir=QCoreApplication::applicationDirPath()+"/config";
    QDir().mkpath(configDir);

    return configDir+"/settings.ini";
}
