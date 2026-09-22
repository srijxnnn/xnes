#include "debug_log.h"

#include <QFont>
#include <QPlainTextEdit>
#include <QVBoxLayout>

DebugLog::DebugLog(QWidget *parent) : QWidget(parent) {
  setWindowFlag(Qt::Window);
  setWindowTitle("Debug");
  setAttribute(Qt::WA_DeleteOnClose);

  log = new QPlainTextEdit(this);
  log->setReadOnly(true);
  log->setLineWrapMode(QPlainTextEdit::NoWrap);
  QFont font(QStringLiteral("Monospace"));
  font.setStyleHint(QFont::Monospace);
  log->setFont(font);

  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(log);
  resize(420, 480);
}

void DebugLog::set_text(const std::string &text) {
  log->setPlainText(QLatin1String(text.data(), static_cast<int>(text.size())));
}
