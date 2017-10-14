#include "bulb.hpp"

bulb::bulb(void)
{
  ip_str.clear();
  id_str.clear();
  brightness = 100;
}

bulb::bulb(QString ip, QString id)
{
  ip_str = ip;
  id_str = id;
  bool r = false;
  id_ = id.toULongLong(&r, 16);
  brightness = 100;
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
  brightness = brn_value;
}

int bulb::get_brightness() const
{
  return brightness;
}
