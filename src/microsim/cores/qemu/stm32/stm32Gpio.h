#pragma once

#include <string>
#include <unicorn/unicorn.h>
#include "peripheral_factory.h"
#include "Register.h"

namespace stm32Gpio {
// GPIO 外设基地址宏定义
#define GPIOA_BASE            ((uint32_t)0x40010800) // Port A 基地址 (APB2 总线)
#define GPIOB_BASE            ((uint32_t)0x40010C00) // Port B 基地址 (APB2 总线)
#define GPIOC_BASE            ((uint32_t)0x40011000) // Port C 基地址 (APB2 总线)
#define GPIOD_BASE            ((uint32_t)0x40011400) // Port D 基地址 (APB2 总线)
#define GPIOE_BASE            ((uint32_t)0x40011800) // Port E 基地址 (APB2 总线)

// 单个 GPIO 端口的地址范围 (0x1C + 0x4 = 0x20)
#define GPIO_REG_SIZE         ((uint32_t)0x20)

// --- GPIO 寄存器偏移地址宏定义 (Offset) ---

#define CFGLOW_OFFSET    ((uint32_t)0x00) // 端口配置低寄存器 (GPIOx_CFGLOW)
#define CFGHIG_OFFSET    ((uint32_t)0x04) // 端口配置高寄存器 (GPIOx_CFGHIG)
#define IDATA_OFFSET     ((uint32_t)0x08) // 端口输入数据寄存器 (GPIOx_IDATA)
#define ODATA_OFFSET     ((uint32_t)0x0C) // 端口输出数据寄存器 (GPIOx_ODATA)
#define BSC_OFFSET       ((uint32_t)0x10) // 端口位设置/清除寄存器 (GPIOx_BSC)
#define BC_OFFSET        ((uint32_t)0x14) // 端口位清除寄存器 (GPIOx_BC)
#define LOCK_OFFSET      ((uint32_t)0x18) // 端口配置锁定寄存器 (GPIOx_LOCK)

// --- 结构体定义 ---
/**
 * @brief GPIO 外设模拟类定义
 */
class Gpio : public PeripheralDevice {
  std::map<uint32_t, std::unique_ptr<Register> > m_registers;
  std::string m_port_name; // 例如 "Port A", "Port B"
  uint64_t m_base_addr;    // 端口基地址
  uint8_t m_port;
  void initialize_registers(const std::string& portName);
  // 检查 RCM 中对应 GPIO 端口的时钟是否开启
  bool is_clock_enabled(void *user_data) const;

  public:
    /**
     * @brief 构造函数
     * @param base_addr GPIO 端口的基地址 (如 GPIOA_BASE)
     * @param port_name 端口名称 (如 "Port A")
     */
    Gpio(uint64_t base_addr, std::string  port_name);

    // PeripheralDevice 接口实现
    bool handle_write(uc_engine *uc, uint64_t address, int size, int64_t value, void *user_data) override;
    bool handle_read(uc_engine *uc, uint64_t address, int size, int64_t *read_value, void *user_data) override;
    uint64_t getBaseAddress() override { return m_base_addr; }
    std::string getName() override { return "GPIO (" + m_port_name + ")"; }
    uint64_t getSize() override { return GPIO_REG_SIZE; }

};
}
