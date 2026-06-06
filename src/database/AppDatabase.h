#pragma once 

#include <QObject>
#include <QSqlDatabase>



class AppDatabase : public QObject{
    Q_OBJECT

public:
    AppDatabase(QObject *parent=nullptr);

    
    //
    bool open(const QString &databasePath);
    void close();

    bool isOpen();
    QString lastError();

    QSqlDatabase database();

private:
    bool createTables();

private:
    QString m_connectionName;
    QString m_lastError;

};
