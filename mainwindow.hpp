#pragma once

#include <QMainWindow>
#include <QtNetwork/QHostInfo>
#include <QtNetwork>

#include <cstdint>
#include <vector>
#include <algorithm>
#include <unordered_map>

#include "bulb.hpp"
#include "bulb_model.hpp"
#include "storage.hpp"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = 0);
  ~MainWindow();

signals:
  void write_to_socket(QTcpSocket* socket, const QByteArray message);
  void done_consuming_unlimited_sockets();

private slots:
  void processPendingDatagrams();
  void on_qpb_connect_clicked();
  void on_qpb_discover_clicked();
  void on_qpb_toggle_clicked();
  void on_horizontalSlider_valueChanged(int value);
  void on_qpb_initialize_db_clicked();
  void on_qpb_color_dialog_clicked();
  void on_qpb_wreak_havoc_clicked();
  void wreak_havoc();
  void on_ready_read_tcp_socket();
  void on_new_tcp_server_connection();
  void on_set_name(::bulb& bulb, QVariant value);
  void on_write_to_socket(QTcpSocket* socket, const QByteArray message);
  void on_done_consuming_unlimited_sockets();
  void on_toggle_all_clicked(bool isSelected);

private:
  int message_id = 0;
  bulb_model* model;
  storage storage;
  Ui::MainWindow *ui;
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

  void connect_to_all_bulbs();
  void discover();
  QString next_message_id();
  QByteArray sub_string(const QByteArray& source, const char* start_str, const char* end_str);
  void set_color(QColor color);
  void set_brightness(int value);
};
