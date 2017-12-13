#pragma once

#include <QAbstractTableModel>

#include "bulb.hpp"

class bulb_model : public QAbstractTableModel
{
  Q_OBJECT
signals:
  void set_name(::bulb& bulb, QVariant value);
private:
public:
  std::vector<::bulb> bulbs;
  class selected_bulbs_proxy
  {
  public:
    selected_bulbs_proxy(std::vector<::bulb> bulbs)
    {
      auto i = std::copy_if(bulbs.begin(), bulbs.end(), back_inserter(selected_bulbs_),
        [](::bulb i) {
          return i.selected;
        });
    }
    vector<::bulb>::iterator begin() { return selected_bulbs_.begin(); }
    vector<::bulb>::iterator end() { return selected_bulbs_.end(); }
  private:
    std::vector<::bulb> selected_bulbs_;
  };
  bool have_bulb(const ::bulb& bulb) const;
  void add_bulb(const ::bulb& bulb);
  int size() const;
  selected_bulbs_proxy selected_bulbs() const
  {
    return bulb_model::selected_bulbs_proxy(bulbs);
  }
  bulb_model();
  virtual int rowCount(const QModelIndex & parent = QModelIndex()) const override;
  virtual int columnCount(const QModelIndex & parent = QModelIndex()) const override;
  virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
  virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
  virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
  virtual Qt::ItemFlags flags(const QModelIndex &index) const override;
};