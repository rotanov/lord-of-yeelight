#pragma once

#include <iostream>
#include <string>

#include <QString>

using namespace std;
class bulb
{
private:
  static const int port = 55443;
  QString ip_str;
  QString id_str;
  int brightness_ = 100;
  uint64_t id_;
  QString user_name_;
  QString internal_name_;
public:
  bool selected = true;
  bulb(QString ip, QString id, QString name, int brightness);
  QString get_ip_str() const;
  QString get_id_str() const;
  uint64_t id() const { return id_; }
  int get_port() const;
  QString user_name() { return user_name_; }
  QString internal_name() const { return internal_name_; }
  bool operator ==(const bulb &x) const
  {
    return (this->ip_str == x.ip_str) && (this->port == x.port) && (this->id_str == x.id_str);
  }
  void set_brightness(int brn_value);
  int get_brightness() const;
};

