#include "peripheral_factory.h"
#include <iostream>
#include <qlogging.h>

//TODO need more code to do more things
bool PeripheralDevice::handle_write(uc_engine *uc, uint64_t address, int size, int64_t value, void *user_data) {
  qWarning("Unprocessed operation to base class virtual function, address:%x",static_cast<unsigned>(address));
  return true;
}

bool PeripheralDevice::handle_read(uc_engine *uc, uint64_t address, int size, int64_t *read_value, void *user_data) {
  qWarning("Unprocessed operation to base class virtual function, address:%x",static_cast<unsigned>(address));
  if (read_value) {
    *read_value = 0;
  }
  return true;
}

uint64_t PeripheralDevice::getBaseAddress() {
  qWarning( "The base class PeripheralDevice::getBaseAddress() is called");
  return 0;
}

std::string PeripheralDevice::getName() {
  return "Unknown PeripheralDevice";
}
uint64_t PeripheralDevice::getSize() {
  return -1;
}
// // ==========================================================
// // GenericPeripheral 实现
// // ==========================================================
// GenericPeripheral::GenericPeripheral(const std::string& name, uint64_t base_addr)
//     : m_name(name), m_base_addr(base_addr) {}
//
// bool GenericPeripheral::handle_write(uc_engine *uc, uint64_t address, int size, int64_t value) {
//   uint64_t offset = address - m_base_addr;
//   std::cout << "[Generic] " << m_name << " Write: Offset=0x" << std::hex << offset
//             << ", Val=0x" << value << std::dec << std::endl;
//   return true;
// }
//
// bool GenericPeripheral::handle_read(uc_engine *uc, uint64_t address, int size, int64_t *read_value) {
//   uint64_t offset = address - m_base_addr;
//   *read_value = 0; // 默认返回0
//   std::cout << "[Generic] " << m_name << " Read: Offset=0x" << std::hex << offset << std::dec << std::endl;
//   return true;
// }
//
// uint64_t GenericPeripheral::getBaseAddress() { return m_base_addr; }
// std::string GenericPeripheral::getName() { return m_name; }