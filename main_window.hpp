#pragma once

#include <QMainWindow>

#include "bulb_model.hpp"
#include "storage.hpp"

namespace Ui {
class main_window;
}

class main_window : public QMainWindow
{
  Q_OBJECT

public:
  explicit main_window(QWidget *parent = 0);
  virtual ~main_window();

private slots:
  void on_qpb_connect_clicked();
  void on_qpb_discover_clicked();
  void on_qpb_toggle_clicked();
  void on_horizontalSlider_valueChanged(int value);
  void on_qpb_initialize_db_clicked();
  void on_qpb_color_dialog_clicked();
  void on_qpb_wreak_havoc_clicked();
  void on_q_slider_color_temperature_valueChanged(int value);

private:
  bulb_model* model;
  storage storage;
  Ui::main_window *ui;
  int screenIndex = 0;
};
