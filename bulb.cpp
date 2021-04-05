#include "bulb.hpp"

bulb::bulb(QString ip, QString id, QString name, int brightness)
  : ip_str(ip)
  , id_str(id)
  , internal_name_(name)
  , brightness_(brightness)
{
  bool r = false;
  id_ = id.toULongLong(&r, 16);
  if (!r) {
    throw new std::exception();
  }
}

QString bulb::get_ip_str() const
{
  return ip_str;
}

QString bulb::get_id_str() const
{
  return id_str;
}

int bulb::get_port() const
{
  return port;
}

void bulb::set_brightness(int brn_value)
{
  brightness_ = brn_value;
}

int bulb::get_brightness() const
{
  return brightness_;
}
