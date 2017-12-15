#pragma once

#include <functional>

#include <QThread>

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