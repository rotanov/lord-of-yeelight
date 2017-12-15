#pragma once

#include <QMainWindow>

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

private slots:
  void on_qpb_connect_clicked();
  void on_qpb_discover_clicked();
  void on_qpb_toggle_clicked();
  void on_horizontalSlider_valueChanged(int value);
  void on_qpb_initialize_db_clicked();
  void on_qpb_color_dialog_clicked();
  void on_qpb_wreak_havoc_clicked();

private:
  bulb_model* model;
  storage storage;
  Ui::MainWindow *ui;
};
