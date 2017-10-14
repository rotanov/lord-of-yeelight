#include "bulb_model.hpp"

bool bulb_model::have_bulb(const::bulb & bulb) const
{
  return std::find(bulbs.begin(), bulbs.end(), bulb) != bulbs.end();
}

void bulb_model::add_bulb(const::bulb & bulb)
{
  int n = bulbs.size();
  beginInsertRows(QModelIndex(), n, n);
  bulbs.push_back(bulb);
  endInsertRows();
}

int bulb_model::size() const
{
  return bulbs.size();
}

bulb_model::bulb_model()
{

}

int bulb_model::rowCount(const QModelIndex & parent) const
{
  return bulbs.size();
}

int bulb_model::columnCount(const QModelIndex & parent) const
{
  return 3;
}

QVariant bulb_model::data(const QModelIndex & index, int role) const
{
  int row = index.row();
  int column = index.column();
  auto& b = bulbs[row];
  switch (role) {
  case Qt::DisplayRole: {
    switch (column) {
    case 0: return b.get_id_str();
    case 1: return b.get_ip_str();
    case 2: return b.get_port();
    }
  }
  case Qt::CheckStateRole: {
    if (column == 0) {
      return b.selected ? Qt::Checked : Qt::Unchecked;
    } else {
      return QVariant();
    }
  }
  default: return QVariant();
  }
}

bool bulb_model::setData(const QModelIndex & index, const QVariant & value, int role)
{
  int row = index.row();
  int column = index.column();
  auto& b = bulbs[row];
  if (role == Qt::CheckStateRole) {
    b.selected = value.toBool();
    return true;
  }
  return false;
}

QVariant bulb_model::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (role != Qt::DisplayRole) {
    return QVariant();
  }
  if (orientation != Qt::Horizontal) {
    return QVariant();
  }
  switch (section) {
  case 0: return "id";
  case 1: return "ip";
  case 2: return "port";
  }
}

Qt::ItemFlags bulb_model::flags(const QModelIndex & index) const
{
  int column = index.column();
  Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
  if (column == 0) {
    flags |= Qt::ItemIsUserCheckable;
  }
  return flags;
}
