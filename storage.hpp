#pragma once
#include <QObject>
#include <QtSql/QSqlDatabase>

class storage : public QObject
{
  Q_OBJECT
private:
  static constexpr const char* db_name = "yeelight.db";
  static constexpr const char* db_schema = "create table bulb(id integer primary key, name text, ip text);"
                                   "create table scene(id integer primary key, name text);";
  QSqlDatabase db;
public:
  storage();
  void open();
  void initialize_db();
};
