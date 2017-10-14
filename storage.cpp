#include <QDebug>
#include <QFile>
#include <QProcess>

#include "storage.hpp"

storage::storage()
{
  auto db = QSqlDatabase::addDatabase("QSQLITE");
  db.setDatabaseName(db_name);
}

void storage::open()
{
  if (!db.open()) {
    qDebug() << "Error: can't open db.";
  } else {
    qDebug() << "Connected to database";
  }
}

void storage::initialize_db()
{
  // TODO: remove button in production or sync db connections
  QFile db_file(db_name);
  db_file.remove();
  auto sqlite = new QProcess(/*TODO:parent*/);
  sqlite->start("sqlite3.exe", { db_name, db_schema });
}
