#include "mainwindow.hpp"
#include "ui_mainwindow.h"
#include <QtNetwork/QHostInfo>
#include <QtNetwork>
#include <QByteArray>
#include <stdint.h>
#include <QDebug>
#include <QProcess>
#include <QTableView>
#include <QTableWidget>
#include <QColorDialog>
#include <QVBoxLayout>
#include "header_view_with_checkbox.hpp"

#include <functional>

MainWindow::MainWindow(QWidget *parent)
  : QMainWindow(parent)
  , ui(new Ui::MainWindow)
{
  ui->setupUi(this);
  model = new bulb_model();
  connect(model, &bulb_model::set_name,
    this, &MainWindow::on_set_name);
  connect(ui->qpb_initialize_db, &QPushButton::clicked,
    this, &MainWindow::on_qpb_initialize_db_clicked);

  MainWindow::setWindowTitle("Lord of Yeelight");

  {
    auto qtwBulbs = new QTableView(this->centralWidget());
    qtwBulbs->setSelectionBehavior(QAbstractItemView::SelectRows);
    qtwBulbs->verticalHeader()->hide();
    //qtwBulbs->horizontalHeader()->setStretchLastSection(true);
    HeaderViewWithCheckbox *headerWithCheckbox = new HeaderViewWithCheckbox(Qt::Horizontal);
    connect(headerWithCheckbox, &HeaderViewWithCheckbox::on_checkbox_click, this, &MainWindow::on_toggle_all_clicked);
    qtwBulbs->setHorizontalHeader(headerWithCheckbox);
    qtwBulbs->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->centralWidget->layout()->addWidget(qtwBulbs);
    qtwBulbs->setModel(model);
    qtwBulbs->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    qtwBulbs->horizontalHeader()->resizeSection(0, 20);
  }

  mcast_addr = "239.255.255.250";
  udp_port = 1982;

  {
    QString localHostName = QHostInfo::localHostName();
    QHostInfo info = QHostInfo::fromName(localHostName);
    foreach (QHostAddress address, info.addresses()) {
      if(address.protocol() == QAbstractSocket::IPv4Protocol) {
        local_ip = address.toString();
        qDebug()<<"IP:"<<address.toString();
      }
    }
  }

  // for writing to TCP sockets from threads
  connect(this, &MainWindow::write_to_socket,
    this, &MainWindow::on_write_to_socket);

  {
    udp_send_socket = new QUdpSocket(this);
    udp_receive_socket = new QUdpSocket(this);

    udp_send_socket->close();
    if (!udp_send_socket->bind(QHostAddress(local_ip), 0, QUdpSocket::ReuseAddressHint)) {
      qDebug() << "udp_send_socket udp bind failed";
    } else {
      qDebug() << "udp_send_socket udp bind success";
    }
    if (!udp_send_socket->joinMulticastGroup(mcast_addr)) {
      qDebug() << "udp_send_socket failed joining multicast group.";
    } else {
      qDebug() << "udp_send_socket joined multicast group";
    }
    connect(udp_send_socket, &QUdpSocket::readyRead,
            this, &MainWindow::processPendingDatagrams);
    udp_receive_socket->close();
    if (!udp_receive_socket->bind(QHostAddress::AnyIPv4, udp_port, QUdpSocket::ReuseAddressHint)) {
      qDebug() << "udp_receive_socket udp bind failed";
    } else {
      qDebug() << "udp_receive_socket udp bind success";
    }
    if (!udp_receive_socket->joinMulticastGroup(mcast_addr)) {
      qDebug() << "udp_receive_socket failed joining multicast group.";
    } else {
      qDebug() << "udp_receive_socket joined multicast group";
    }
    connect(udp_receive_socket, &QUdpSocket::readyRead,
            this, &MainWindow::processPendingDatagrams);
  }

  discover();
}

MainWindow::~MainWindow()
{
  for (auto kv : tcp_sockets) {
    kv.second->close();
  }
  udp_send_socket->close();
  udp_receive_socket->close();
  delete ui;
}

void MainWindow::processPendingDatagrams()
{
  auto udp_socket = static_cast<QUdpSocket*>(QObject::sender());
  QByteArray buffer;
  while (udp_socket->hasPendingDatagrams()) {
    qDebug() << "udp receive data";
    buffer.resize(udp_socket->pendingDatagramSize());
    udp_socket->readDatagram(buffer.data(), buffer.size());
    qDebug() << buffer.data();

    auto ip = sub_string(buffer, "Location: yeelight://", ":");
    auto id_str = sub_string(buffer, "id: ", "\r\n");
    auto name = sub_string(buffer, "name: ", "\r\n");
    int brightness = sub_string(buffer, "bright: ", "\r\n").toInt();

    ::bulb bulb_tmp(ip, id_str, name, brightness);
    if (!model->have_bulb(bulb_tmp)) {
      model->add_bulb(bulb_tmp);
      connect_to_all_bulbs();
    }
  }
}

QByteArray MainWindow::sub_string(const QByteArray& source, const char* start_str, const char* end_str)
{
  QByteArray result;
  int pos1 = -1;
  int pos2 = -1;
  result.clear();
  auto prefix = QByteArray(start_str);
  auto suffix = QByteArray(end_str);
  pos1 = source.indexOf(prefix, 0);
  if (pos1 != -1) {
    result = source.mid(pos1);
    pos1 = prefix.length();
    result = result.mid(pos1);
    pos2 = result.indexOf(suffix);
    result = result.mid(0, pos2);
  }
  return result;
}

void MainWindow::on_qpb_discover_clicked()
{
  discover();
}

void MainWindow::discover()
{
  QByteArray datagram = "M-SEARCH * HTTP/1.1\r\n"
    "HOST: 239.255.255.250:1982\r\n"
    "MAN: \"ssdp:discover\"\r\n"
    "ST: wifi_bulb";
  int ret = udp_send_socket->writeDatagram(datagram.data(), datagram.size(), mcast_addr, udp_port);
  qDebug() << "udp write " << ret << " bytes";
}

void MainWindow::on_qpb_connect_clicked()
{
  connect_to_all_bulbs();
}

void MainWindow::connect_to_all_bulbs()
{
  if (!tcp_server.isListening()) {
    tcp_server.listen(QHostAddress(local_ip), 0);
    connect(&tcp_server, &QTcpServer::newConnection,
      this, &MainWindow::on_new_tcp_server_connection);
  }
  for (auto& b : model->selected_bulbs()) {
    if (tcp_sockets.find(b.id()) == tcp_sockets.end()) {
      tcp_sockets[b.id()] = new QTcpSocket(this);
    }
    auto& s = *tcp_sockets[b.id()];
    if (!s.isOpen()) {
      s.connectToHost(QHostAddress(b.get_ip_str()), b.get_port());
      connect(&s, &QTcpSocket::readyRead,
        this, &MainWindow::on_ready_read_tcp_socket);
    }
  }
}

void MainWindow::on_qpb_toggle_clicked()
{
  for (auto& b : model->selected_bulbs()) {
    auto& s = *tcp_sockets[b.id()];
    QByteArray cmd_str;
    cmd_str.append("{\"id\":");
    cmd_str.append(next_message_id());
    cmd_str.append(",\"method\":\"toggle\",\"params\":[]}\r\n");
    s.write(cmd_str.data());
  }
}

void MainWindow::on_horizontalSlider_valueChanged(int value)
{
  Q_UNUSED(value);
  int pos = ui->horizontalSlider->value();
  QString slider_value = QString("%1").arg(pos) + "%";
  QByteArray *cmd_str =new QByteArray;
  cmd_str->clear();
  cmd_str->append("{\"id\":");

  for (auto& b : model->selected_bulbs()) {
    auto& s = *tcp_sockets[b.id()];
    cmd_str->append(next_message_id());
    cmd_str->append(",\"method\":\"set_bright\",\"params\":[");
    cmd_str->append(QString("%1").arg(pos));
    cmd_str->append(", \"smooth\", 500]}\r\n");
    s.write(cmd_str->data());
  }
}

void MainWindow::on_qpb_initialize_db_clicked()
{
  storage.initialize_db();
}

void MainWindow::on_qpb_color_dialog_clicked()
{
  QColor color = QColorDialog::getColor(Qt::yellow, this);
  auto hue = color.hsvHue();
  if (hue == -1) {
    hue = 0;
  }
  auto sat = static_cast<int>(color.hsvSaturationF() * 100);
  for (auto& b : model->selected_bulbs()) {
    auto& s = *tcp_sockets[b.id()];
    QByteArray* cmd_str = new QByteArray();
    cmd_str->clear();
    cmd_str->append(QString("{\"id\":%1,\"method\":\"set_hsv\",\"params\":[%2,%3,\"sudden\",500]}\r\n")
      .arg(next_message_id(), QString::number(hue), QString::number(sat)));
    s.write(cmd_str->data());
    cmd_str->clear();
    cmd_str->append(QString("{\"id\":%1,\"method\":\"set_bright\",\"params\":[%2,\"sudden\",500]}\r\n")
      .arg(next_message_id(), QString::number(ui->horizontalSlider->value())));
    s.write(cmd_str->data());
  }
}

class delegate_thread : public QThread
{
private:
  std::function<void()> action;
public:
  delegate_thread(std::function<void()> action)
    : action(action) {}
  virtual void run() override
  {
    action();
  }
};

void MainWindow::on_write_to_socket(QTcpSocket* socket, const QByteArray message)
{
  socket->write(message);
}

void MainWindow::on_qpb_wreak_havoc_clicked()
{
  connect(this, &MainWindow::done_consuming_unlimited_sockets,
    this, &MainWindow::on_done_consuming_unlimited_sockets);
  auto thread = new delegate_thread([=]() {
    for (auto& b : model->selected_bulbs()) {
      auto& s = *tcp_sockets[b.id()];
      QByteArray cmd_str;
      auto message_id = next_message_id();
      cmd_str.append(
        QString("{\"id\":%1,\"method\":\"set_music\",\"params\":[1,\"%2\",%3]}\r\n")
        .arg(message_id, local_ip, QString::number(tcp_server.serverPort()))
        );
      pending_bulb_id = b.id();
      emit write_to_socket(&s, cmd_str);
      while (pending_bulb_id != -1) {
        QThread::msleep(1);
      }
    }
    emit done_consuming_unlimited_sockets();
  });
  connect(thread, &QThread::finished,
    thread, &QObject::deleteLater);
  thread->start();
}

void MainWindow::on_toggle_all_clicked(bool isSelected)
{
  int row = 0;
  for (auto& selected_bulb : model->bulbs) {
    QModelIndex index = model->index(row, 0);
    model->setData(index, isSelected, Qt::CheckStateRole);
    model->dataChanged(index, index);
    row++;
  }
}

void MainWindow::on_done_consuming_unlimited_sockets()
{
  auto timer = new QTimer(this);
  timer->setInterval(200);
  timer->start();
  connect(timer, &QTimer::timeout, this, &MainWindow::wreak_havoc);
}

static int cycle = 0;

void MainWindow::wreak_havoc()
{
  cycle++;
  auto hue = rand() % 360;
  if (hue == -1) {
    hue = 0;
  }
  auto sat = (cycle % 2) * 100;// rand() % 100;
  for (auto& b : model->selected_bulbs()) {
    if (unlimited_tcp_sockets.find(b.id()) == unlimited_tcp_sockets.end()) {
      return;
    }
    auto& s = *unlimited_tcp_sockets[b.id()];
    QByteArray cmd_str;
    cmd_str.clear();
    cmd_str.append(QString("{\"id\":%1,\"method\":\"set_hsv\",\"params\":[%2,%3,\"sudden\",500]}\r\n")
      .arg(next_message_id(), QString::number(hue), QString::number(sat)));
    s.write(cmd_str.data());
    cmd_str.clear();
    cmd_str.append(QString("{\"id\":%1,\"method\":\"set_bright\",\"params\":[%2,\"sudden\",0]}\r\n")
      .arg(next_message_id(), QString::number((cycle % 2) == 0 ? 1 : 100)));
    s.write(cmd_str.data());
  }
}

void MainWindow::on_ready_read_tcp_socket()
{
  QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
  QByteArray buffer;
  while (socket->bytesAvailable() > 0)
  {
    buffer.append(socket->readAll());
    qDebug() << buffer;
  }
}

void MainWindow::on_new_tcp_server_connection()
{
  auto& s = *tcp_server.nextPendingConnection();
  connect(&s, &QTcpSocket::readyRead,
    this, &MainWindow::on_ready_read_tcp_socket);
  unlimited_tcp_sockets[pending_bulb_id] = &s;
  pending_bulb_id = -1;
}

void MainWindow::on_set_name(::bulb & bulb, QVariant value)
{
  auto& s = *tcp_sockets[bulb.id()];
  QByteArray message;
  message.append(QString("{\"id\":%1,\"method\":\"set_name\",\"params\":[\"%2\"]}\r\n")
    .arg(next_message_id(), value.toString()));
  s.write(message);
}

QString MainWindow::next_message_id()
{
  return QString::number(message_id++);
}