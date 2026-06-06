#include "AppDatabase.h"

#include <QSqlQuery>
#include <QSqlError>



AppDatabase::AppDatabase(QObject *parent)
    :QObject(parent)
{
}

bool AppDatabase::open(const QString &databasePath)
{
    m_connectionName="main_connection";

    QSqlDatabase db=QSqlDatabase::addDatabase("QSQLITE",m_connectionName);

    db.setDatabaseName(databasePath);

    if(!db.open()){
        m_lastError=db.lastError().text();
        return false;
    };

    if(!createTables()){
        return false;
    }
    
    return true;
}

void AppDatabase::close()
{
    if(QSqlDatabase::contains(m_connectionName)){
        QSqlDatabase db=QSqlDatabase::database(m_connectionName);
        db.close();
    }
}

bool AppDatabase::isOpen()
{
    QSqlDatabase db=QSqlDatabase::database(m_connectionName);
    return db.isOpen();
}

QString AppDatabase::lastError()
{
    return m_lastError;
}

QSqlDatabase AppDatabase::database()
{
    return QSqlDatabase::database(m_connectionName);
}

bool AppDatabase::createTables()
{

    QSqlQuery query(database());
    bool ret;
    
    //启用外键约束检查
    ret = query.exec("PRAGMA foreign_keys = ON;");
    if(!ret) {
        m_lastError=query.lastError().text();
        return false;
    }


    ret=query.exec(R"(
        CREATE TABLE IF NOT EXISTS devices (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        product_id TEXT NOT NULL,
        device_name TEXT NOT NULL,
        display_name TEXT,
        remark TEXT,
        created_at TEXT NOT NULL,
        last_sync_time TEXT,
        UNIQUE(product_id, device_name));
    )");
    if(!ret) {
        m_lastError=query.lastError().text();
        return false;
    }

    ret=query.exec(R"(
        CREATE TABLE IF NOT EXISTS rides (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        device_id INTEGER NOT NULL,
        title TEXT,
        start_time TEXT NOT NULL,
        end_time TEXT NOT NULL,
        duration_seconds INTEGER DEFAULT 0,
        start_latitude REAL,
        start_longitude REAL,
        end_latitude REAL,
        end_longitude REAL,
        trip_mileage REAL DEFAULT 0,
        avg_heart_rate REAL DEFAULT 0,
        max_heart_rate INTEGER DEFAULT 0,
        avg_speed REAL DEFAULT 0,
        max_speed REAL DEFAULT 0,
        avg_spo2 REAL DEFAULT 0,
        min_spo2 INTEGER DEFAULT 0,
        avg_temperature REAL DEFAULT 0,
        avg_humidity REAL DEFAULT 0,
        event_count INTEGER DEFAULT 0,
        created_at TEXT NOT NULL,
        FOREIGN KEY(device_id) REFERENCES devices(id)
        );
    )");
    if(!ret) {
        m_lastError=query.lastError().text();
        return false;
    }


    ret=query.exec(R"(
        CREATE TABLE IF NOT EXISTS sensor_data (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        ride_id INTEGER NOT NULL,
        device_id INTEGER NOT NULL,
        timestamp_ms INTEGER NOT NULL,
        time_text TEXT NOT NULL,
        heart_rate INTEGER,
        spo2 INTEGER,
        speed REAL,
        temperature REAL,
        humidity REAL,
        latitude REAL,
        longitude REAL,
        trip_mileage REAL,
        total_mileage REAL,
        created_at TEXT NOT NULL,
        FOREIGN KEY(ride_id) REFERENCES rides(id),
        FOREIGN KEY(device_id) REFERENCES devices(id),
        UNIQUE(ride_id, timestamp_ms)
        );
    )");
    if(!ret) {
        m_lastError=query.lastError().text();
        return false;
    }


    ret=query.exec(R"(
        CREATE TABLE IF NOT EXISTS events (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        ride_id INTEGER,
        device_id INTEGER NOT NULL,
        timestamp_ms INTEGER NOT NULL,
        time_text TEXT NOT NULL,
        identifier TEXT NOT NULL,
        name TEXT,
        event_type INTEGER,
        level TEXT,
        value_json TEXT,
        latitude REAL,
        longitude REAL,
        heart_rate INTEGER,
        trip_mileage REAL,
        message TEXT,
        created_at TEXT NOT NULL,
        FOREIGN KEY(ride_id) REFERENCES rides(id),
        FOREIGN KEY(device_id) REFERENCES devices(id)
        );
    )");
    if(!ret) {
        m_lastError=query.lastError().text();
        return false;
    }


    return true;
}
