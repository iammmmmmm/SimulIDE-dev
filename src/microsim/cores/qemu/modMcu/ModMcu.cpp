#include "ModMcu.h"

#include <QDir>
#include <QDirIterator>
#include <QLibrary>

#include "simulator.h"
ModMcu::ModMcu(QString type, QString id, QString device) : QemuDevice(type, id) {
  QString const exePath = QCoreApplication::applicationDirPath();
  const QString path = QDir(exePath).absoluteFilePath("data/modMcu/" + type);

  if (QLibrary::isLibrary(path)) {
    qDebug() << Q_FUNC_INFO << ": loading module \"" << path << "\"";
    mylibrary.setFileName(path);
    mylibrary.load();

    m_runToTime = reinterpret_cast<RunToTimeFunc>(mylibrary.resolve("mcu_run_to_time"));
    m_stamp = mylibrary.resolve("mcu_stamp");
    m_initialize = mylibrary.resolve("mcu_initialize");
    m_onPinChanged = reinterpret_cast<PinChangedFunc>(mylibrary.resolve("mcu_on_pin_changed"));
    m_onScheduled = mylibrary.resolve("mcu_on_scheduled_event");
    m_mcuSetUpFunc = reinterpret_cast<McuSetUpHostFunc>(mylibrary.resolve("mcu_setup_host"));
    if (!m_runToTime || !m_stamp || !m_initialize ||
      !m_onPinChanged || !m_onScheduled || !m_mcuSetUpFunc) {
      qCritical() << "错误: 动态库中缺少必要的导出函数！";

      // 解析失败时，建议把指针全部清空，防止后续逻辑误用
      m_runToTime = nullptr;
      m_stamp = nullptr;
      m_initialize = nullptr;
      m_onPinChanged = nullptr;
      m_onScheduled = nullptr;
      m_mcuSetUpFunc = nullptr;

      return;
    }
    ModMcuHost host;
    host.context=this;
    host.addEvent=;

    m_mcuSetUpFunc(host);
  } else {
    qWarning() << "not find dll/so file :" + type;
    return;
  }
}
ModMcu::~ModMcu() {
  mylibrary.unload();
}
void ModMcu::stamp() {
  m_stamp();
}
void ModMcu::initialize() {
  m_initialize();
}
// LibraryItem * ModMcu::libraryItem() {
//   return ;
// }
Component *ModMcu::construct(QString type, QString id) {
  const QString device = getDevice(id);
  return new ModMcu(type, id, device);
}
void ModMcu::runToTime(const uint64_t time) {
  m_runToTime(time);
}
void ModMcu::runEvent() {
  mcu_on_scheduled_event();
}
