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
  int brightness;
  uint64_t id_;

public:
  bool selected = true;
  bulb(void);
  bulb(QString ip, QString id);
  QString get_ip_str() const;
  QString get_id_str() const;
  uint64_t id() const { return id_; }
  int get_port() const;
  bool operator ==(const bulb &x) const
  {
    return (this->ip_str == x.ip_str) && (this->port == x.port) && (this->id_str == x.id_str);
  }
  void set_brightness(int brn_value);
  int get_brightness() const;
};

