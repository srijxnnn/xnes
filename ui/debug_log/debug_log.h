#ifndef DEBUG_LOG_H
#define DEBUG_LOG_H

#include <QWidget>

#include <string>

class QPlainTextEdit;

class DebugLog : public QWidget {
public:
  explicit DebugLog(QWidget *parent = nullptr);

  void set_text(const std::string &text);

private:
  QPlainTextEdit *log;
};

#endif
