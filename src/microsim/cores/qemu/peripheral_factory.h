#pragma once
#include <map>
#include <string>
#include <memory>
#include <iostream>
#include <qlogging.h>
#include <unicorn/unicorn.h>

#include "e-element.h"

class PeripheralDevice:public eElement{
  public:
    explicit PeripheralDevice(): eElement("") {}
    virtual bool handle_write(uc_engine *uc, uint64_t address, int size, int64_t value, void *user_data);


    virtual bool handle_read(uc_engine *uc, uint64_t address, int size, int64_t *read_value, void *user_data);
    /**
     * @brief Get the starting address of the peripheral for search
     */
    virtual uint64_t getBaseAddress();

    /**
     * @brief getPeripheralName
     */
    virtual std::string getName();
    virtual uint64_t getSize();
};

class PeripheralRegistry {
  public:
    PeripheralRegistry() = default;
    void registerDevice(std::unique_ptr<PeripheralDevice> device) {
      if (device == nullptr) {
        qWarning("device == nullptr");
        return;
      }
      const uint64_t base_addr = device->getBaseAddress();
      if (m_devices.count(base_addr)) {
        std::cerr << "Warning: address 0x" << std::hex << base_addr << " the peripheral on has been registered repeatedly" << std::endl;
        return;
      }
      m_devices[base_addr] = std::move(device);
    }
    //TODO need more code to do more things
     PeripheralDevice *findDevice(uint64_t address) const {
      auto it = m_devices.upper_bound(address);
      if (it != m_devices.begin()) {
        --it;
        PeripheralDevice *device = it->second.get();
        uint64_t base_addr = it->first; // 获取基地址
        if (address < base_addr + device->getSize()) {
          return device;
        }
      }
      return nullptr;
    }

    // 获取所有注册的外设
    const std::map<uint64_t, std::unique_ptr<PeripheralDevice> > &getDevices() const {
      return m_devices;
    }
    void reset() {
     m_devices.clear();
    }

  private:
    // 存储外设：Key是外设的起始地址
    std::map<uint64_t, std::unique_ptr<PeripheralDevice> > m_devices;
};
