#pragma once

#include <string>
#include <unicorn/unicorn.h>
#include "peripheral_factory.h"
#include "Register.h"

namespace stm32Fmc {
// --- FMC 外设基地址宏定义 ---

#define FMC_BASE_ADDR            ((uint32_t)0x40022000) // FMC 基地址 (AHB 总线)
#define FMC_REG_SIZE             ((uint32_t)0x24)       // 寄存器地址范围 0x00 到 0x20，总大小为 0x24 (36字节)

// --- FMC 寄存器偏移地址宏定义 (Offset) ---

#define FMC_CTRL1_OFFSET         ((uint32_t)0x00) // 控制寄存器1
#define FMC_KEY_OFFSET           ((uint32_t)0x04) // 关键字寄存器
#define FMC_OBKEY_OFFSET         ((uint32_t)0x08) // 选项字节关键字寄存器
#define FMC_STS_OFFSET           ((uint32_t)0x0C) // 状态寄存器
#define FMC_CTRL2_OFFSET         ((uint32_t)0x10) // 控制寄存器2
#define FMC_ADDR_OFFSET          ((uint32_t)0x14) // 闪存地址寄存器
#define FMC_OBCS_OFFSET          ((uint32_t)0x1C) // 选项字节控制/状态寄存器
#define FMC_WRTPROT_OFFSET       ((uint32_t)0x20) // 写保护寄存器

// --- 结构体定义 ---

// --- FMC 关键位定义 (用于 CTRL2 和 STS) ---

class Fmc: public PeripheralDevice {
  private:
    std::map<uint32_t, std::unique_ptr<Register> > m_registers;
    std::string m_peripheral_name = "FMC";
    uint64_t m_base_addr = FMC_BASE_ADDR;

    // 简化模拟解锁/锁定状态
    bool m_is_locked = true;
    uint32_t m_key_sequence_step = 0; // 解锁步骤

    void initialize_registers();
    //void update_sts_flags(uint32_t clear_mask); // 更新/清除状态标志位

  public:
    Fmc();
    bool handle_write(uc_engine *uc,
                      uint64_t address,
                      int size,
                      int64_t value,
                      void *user_data) override;
    bool handle_read(uc_engine *uc,
                     uint64_t address,
                     int size,
                     int64_t *read_value,
                     void *user_data) override;
    uint64_t getBaseAddress() override { return m_base_addr; }
    std::string getName() override { return m_peripheral_name; }
    uint64_t getSize() override { return FMC_REG_SIZE; }
};
}
