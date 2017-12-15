#pragma once

#include <cstdint>
#include <vector>
#include <algorithm>
#include <unordered_map>

#include <QAbstractTableModel>
#include <QtNetwork/QHostInfo>
#include <QtNetwork>

#include "bulb.hpp"

class bulb_model : public QAbstractTableModel
{
  Q_OBJECT

public:
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

signals:
  void set_name(::bulb& bulb, QVariant value);
  void write_to_socket(QTcpSocket* socket, const QByteArray message);
  void done_consuming_unlimited_sockets();

private slots:
  void processPendingDatagrams();
  void wreak_havoc();
  void on_ready_read_tcp_socket();
  void on_new_tcp_server_connection();
  void on_set_name(::bulb& bulb, QVariant value);
  void on_write_to_socket(QTcpSocket* socket, const QByteArray message);
  void on_done_consuming_unlimited_sockets();

public slots:
  void change_selection_state_for_all_bulbs(bool is_selected);

private:
  int message_id = 0;
  QUdpSocket* udp_receive_socket;
  QUdpSocket* udp_send_socket;
  uint16_t udp_port;
  using socket_map = std::unordered_map<uint64_t, QTcpSocket*>;
  socket_map tcp_sockets;
  socket_map unlimited_tcp_sockets;
  uint64_t pending_bulb_id = -1;
  QHostAddress mcast_addr;
  QTcpServer tcp_server;
  QString local_ip;

  QString next_message_id();
  QByteArray sub_string(const QByteArray& source, const char* start_str, const char* end_str);

public:
  std::vector<::bulb> bulbs;
  void set_color(QColor color);
  void set_brightness(int value);
  void toggle_bulbs();
  void discover_bulbs();
  void connect_to_all_bulbs();
  void begin_havok();
  bool have_bulb(const ::bulb& bulb) const;
  void add_bulb(const ::bulb& bulb);
  int size() const;
  selected_bulbs_proxy selected_bulbs() const
  {
    return bulb_model::selected_bulbs_proxy(bulbs);
  }
  bulb_model();
  virtual ~bulb_model();
  virtual int rowCount(const QModelIndex & parent = QModelIndex()) const override;
  virtual int columnCount(const QModelIndex & parent = QModelIndex()) const override;
  virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
  virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
  virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
  virtual Qt::ItemFlags flags(const QModelIndex &index) const override;
};