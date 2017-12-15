#include "bulb_model.hpp"

#include <QRgb>
#include <QApplication>
#include <QColor>
#include <QScreen>
#include <QPixmap>

#include "delegate_thread.hpp"

static std::vector<QString> column_names = {
  "", "name", "id", "ip", "brightness",
};

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
  connect(this, &bulb_model::set_name,
    this, &bulb_model::on_set_name);

  mcast_addr = "239.255.255.250";
  udp_port = 1982;

  {
    QString localHostName = QHostInfo::localHostName();
    QHostInfo info = QHostInfo::fromName(localHostName);
    foreach(QHostAddress address, info.addresses()) {
      if (address.protocol() == QAbstractSocket::IPv4Protocol) {
        local_ip = address.toString();
        qDebug() << "IP:" << address.toString();
      }
    }
  }

  // for writing to TCP sockets from threads
  connect(this, &bulb_model::write_to_socket,
    this, &bulb_model::on_write_to_socket);

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
      this, &bulb_model::processPendingDatagrams);
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
      this, &bulb_model::processPendingDatagrams);
  }
}

bulb_model::~bulb_model()
{
  for (auto kv : tcp_sockets) {
    kv.second->close();
  }
  udp_send_socket->close();
  udp_receive_socket->close();
}

int bulb_model::rowCount(const QModelIndex & parent) const
{
  return bulbs.size();
}

int bulb_model::columnCount(const QModelIndex & parent) const
{
  return column_names.size();
}

QVariant bulb_model::data(const QModelIndex & index, int role) const
{
  int row = index.row();
  int column = index.column();
  auto& b = bulbs[row];
  switch (role) {
  case Qt::DisplayRole: {
    switch (column) {
    case 1: return b.internal_name();
    case 2: return b.get_id_str();
    case 3: return b.get_ip_str();
    case 4: return b.get_brightness();
    default: return QVariant();
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
  } else if (role == Qt::EditRole) {
    switch (column) {
    case 1: {
      emit set_name(b, value);
      //emit dataChanged(QModelIndex(), QModelIndex());
      return true;
    }
    default:
      return false;
    }
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
  return column_names[section];
}

Qt::ItemFlags bulb_model::flags(const QModelIndex & index) const
{
  int column = index.column();
  Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
  if (column == 0) {
    flags |= Qt::ItemIsUserCheckable | Qt::ItemIsEditable;
  }
  return flags;
}

void bulb_model::on_done_consuming_unlimited_sockets()
{
  auto timer = new QTimer(this);
  timer->setInterval(200);
  timer->start();
  connect(timer, &QTimer::timeout, this, &bulb_model::wreak_havoc);
}

static int cycle = 0;
static int prev_hue = -1;
static int prev_bright = -1;
static int prev_sat = -1;

void bulb_model::wreak_havoc()
{
  cycle++;
  auto hue = rand() % 360;
  if (hue == -1) {
    hue = 0;
  }
  auto sat = (cycle % 2) * 100;

  auto s0 = QApplication::screens()[0];
  auto r = s0->geometry();
  auto pixmap = s0->grabWindow(0, r.left(), r.top(), r.width(), r.height());
  auto img = pixmap.toImage();
  double totalBrightness = 0.0;
  double totalRed = 0.0;
  double totalBlue = 0.0;
  double totalGreen = 0.0;
  for (int y = 0; y < img.height(); y++) {
    QRgb *line = (QRgb *)img.scanLine(y);
    for (int x = 0; x < pixmap.width(); x++) {
      // line[x] has an individual pixel
      auto c = line[x];
      totalBrightness += (qRed(c) + qGreen(c) + qBlue(c)) / 3.0f;
      totalRed += qRed(c);
      totalBlue += qBlue(c);
      totalGreen += qGreen(c);
    }
  }
  int n = img.width() * img.height();
  float brightness = totalBrightness / n;
  brightness = brightness / 255.0f * 100.0f;

  auto color = QColor(totalRed / n, totalGreen / n, totalBlue / n);
  hue = color.hueF() * 359.0f;
  sat = color.saturationF() * 100.0f;
  if (hue == prev_hue && sat == prev_sat && (int)brightness == prev_bright) {
    return;
  }
  prev_hue = hue;
  prev_bright = brightness;
  prev_sat = sat;

  for (auto& b : selected_bulbs()) {
    if (unlimited_tcp_sockets.find(b.id()) == unlimited_tcp_sockets.end()) {
      return;
    }
    auto& s = *unlimited_tcp_sockets[b.id()];
    QByteArray cmd_str;
    auto msg_id = next_message_id();
    cmd_str.clear();
    cmd_str.append(QString("{\"id\":%1,\"method\":\"set_hsv\",\"params\":[%2,%3,\"smooth\",500]}\r\n")
      .arg(msg_id, QString::number(hue), QString::number(sat)));
    s.write(cmd_str.data());

    cmd_str.clear();
    msg_id = next_message_id();
    cmd_str.append(QString("{\"id\":%1,\"method\":\"set_bright\",\"params\":[%2,\"smooth\",500]}\r\n")
      .arg(msg_id, QString::number((int)brightness)));// QString::number((cycle % 2) == 0 ? 1 : 100)));
    s.write(cmd_str.data());
  }
}

void bulb_model::on_ready_read_tcp_socket()
{
  QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
  QByteArray buffer;
  while (socket->bytesAvailable() > 0)
  {
    buffer.append(socket->readAll());
    qDebug() << buffer;
  }
}

void bulb_model::on_new_tcp_server_connection()
{
  auto& s = *tcp_server.nextPendingConnection();
  connect(&s, &QTcpSocket::readyRead,
    this, &bulb_model::on_ready_read_tcp_socket);
  unlimited_tcp_sockets[pending_bulb_id] = &s;
  pending_bulb_id = -1;
}

void bulb_model::on_set_name(::bulb & bulb, QVariant value)
{
  auto& s = *tcp_sockets[bulb.id()];
  QByteArray message;
  message.append(QString("{\"id\":%1,\"method\":\"set_name\",\"params\":[\"%2\"]}\r\n")
    .arg(next_message_id(), value.toString()));
  s.write(message);
}

QString bulb_model::next_message_id()
{
  return QString::number(message_id++);
}

void bulb_model::begin_havok()
{
  connect(this, &bulb_model::done_consuming_unlimited_sockets,
    this, &bulb_model::on_done_consuming_unlimited_sockets);
  auto thread = new delegate_thread([=]() {
    for (auto& b : this->selected_bulbs()) {
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

void bulb_model::set_color(QColor color)
{
  auto hue = color.hsvHue();
  if (hue == -1) {
    hue = 0;
  }
  auto sat = static_cast<int>(color.hsvSaturationF() * 100);
  for (auto& b : this->selected_bulbs()) {
    auto& s = *tcp_sockets[b.id()];
    QByteArray cmd_str;
    cmd_str.clear();
    cmd_str.append(QString("{\"id\":%1,\"method\":\"set_hsv\",\"params\":[%2,%3,\"sudden\",500]}\r\n")
      .arg(next_message_id(), QString::number(hue), QString::number(sat)));
    s.write(cmd_str.data());
  }
}

void bulb_model::on_write_to_socket(QTcpSocket* socket, const QByteArray message)
{
  socket->write(message);
}

void bulb_model::discover_bulbs()
{
  QByteArray datagram = "M-SEARCH * HTTP/1.1\r\n"
    "HOST: 239.255.255.250:1982\r\n"
    "MAN: \"ssdp:discover\"\r\n"
    "ST: wifi_bulb";
  int ret = udp_send_socket->writeDatagram(datagram.data(), datagram.size(), mcast_addr, udp_port);
  qDebug() << "udp write " << ret << " bytes";
}

void bulb_model::connect_to_all_bulbs()
{
  if (!tcp_server.isListening()) {
    tcp_server.listen(QHostAddress(local_ip), 0);
    connect(&tcp_server, &QTcpServer::newConnection,
      this, &bulb_model::on_new_tcp_server_connection);
  }
  for (auto& b : this->selected_bulbs()) {
    if (tcp_sockets.find(b.id()) == tcp_sockets.end()) {
      tcp_sockets[b.id()] = new QTcpSocket(this);
    }
    auto& s = *tcp_sockets[b.id()];
    if (!s.isOpen()) {
      s.connectToHost(QHostAddress(b.get_ip_str()), b.get_port());
      connect(&s, &QTcpSocket::readyRead,
        this, &bulb_model::on_ready_read_tcp_socket);
    }
  }
}

void bulb_model::toggle_bulbs()
{
  for (auto& b : this->selected_bulbs()) {
    auto& s = *tcp_sockets[b.id()];
    QByteArray cmd_str;
    cmd_str.append("{\"id\":");
    cmd_str.append(next_message_id());
    cmd_str.append(",\"method\":\"toggle\",\"params\":[]}\r\n");
    s.write(cmd_str.data());
  }
}

void bulb_model::processPendingDatagrams()
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

    if (ip.size() == 0) {
      return;
    }

    ::bulb bulb_tmp(ip, id_str, name, brightness);
    if (!this->have_bulb(bulb_tmp)) {
      this->add_bulb(bulb_tmp);
      connect_to_all_bulbs();
    }
  }
}

QByteArray bulb_model::sub_string(const QByteArray& source, const char* start_str, const char* end_str)
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

void bulb_model::set_brightness(int value)
{
  for (auto& b : this->selected_bulbs()) {
    auto& s = *tcp_sockets[b.id()];
    QByteArray cmd_str;
    cmd_str.clear();
    cmd_str.append("{\"id\":");
    cmd_str.append(next_message_id());
    cmd_str.append(",\"method\":\"set_bright\",\"params\":[");
    cmd_str.append(QString("%1").arg(value));
    cmd_str.append(", \"smooth\", 500]}\r\n");
    s.write(cmd_str.data());
  }
}

void bulb_model::change_selection_state_for_all_bulbs(bool is_selected)
{
  for (auto&& bulb : bulbs) {
    bulb.selected = is_selected;
  }
  for (int i = 0; i < rowCount(); i++) {
    QModelIndex index = this->index(i, 0);
    dataChanged(index, index);
  }
}