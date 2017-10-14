#pragma once

#include <QMainWindow>
#include <QtNetwork/QHostInfo>
#include <QtNetwork>
#include <stdint.h>
#include <vector>
#include <algorithm>

#include "bulb.hpp"
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
  QByteArray sub_string(const char* start_str, const char* end_str);

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

private:
  storage storage;
  Ui::MainWindow *ui;
  QUdpSocket* udp_receive_socket;
  QUdpSocket* udp_send_socket;
  uint16_t udp_port;
  QByteArray udp_datagram_recv;
  QList<QTcpSocket*> tcp_sockets;
  QList<QTcpSocket*> unlimited_tcp_sockets;
  QList<::bulb>::iterator ib;
  QHostAddress mcast_addr;
  QTcpServer tcp_server;
  QString local_ip;
  QString bulb_ip;
  QString bulb_id_str;

  void connect_to_all_bulbs();
  void discover();
};
