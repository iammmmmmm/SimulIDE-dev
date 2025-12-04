#pragma once

#include <string>
#include <unicorn/unicorn.h>
#include "peripheral_factory.h"

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

/**
 * @brief FMC 寄存器定义
 * * 内存映射结构体，用于在模拟器中保存 FMC 寄存器的状态
 */
typedef struct {
  volatile uint32_t CTRL1;      /**< 0x00: 控制寄存器1 (复位值: 0x00000030) */
  volatile const uint32_t KEY;  /**< 0x04: 关键字寄存器 (只写, 读返回 0) */
  volatile const uint32_t OBKEY;/**< 0x08: 选项字节关键字寄存器 (只写, 读返回 0) */
  volatile uint32_t STS;        /**< 0x0C: 状态寄存器 (复位值: 0x00000000) */
  volatile uint32_t CTRL2;      /**< 0x10: 控制寄存器2 (复位值: 0x00000080) */
  volatile const uint32_t ADDR; /**< 0x14: 地址寄存器 (只写, 写入要编程/擦除的地址) */
  volatile uint32_t RESERVED1;  /**< 0x18: 保留/填充 */
  volatile uint32_t OBCS;       /**< 0x1C: 选项字节控制/状态寄存器 (复位值: 0x03FFFFFC) */
  volatile const uint32_t WRTPROT;/**< 0x20: 写保护寄存器 (只读, 复位值: 0xFFFFFFFF) */
  // 结构体总大小 0x24 (36 字节)
} FMC_TypeDef;

// --- FMC 关键位定义 (用于 CTRL2 和 STS) ---

#define FMC_STS_BUSYF   ((uint32_t)0x00000001) // 忙碌标志，位 0
#define FMC_STS_PEF     ((uint32_t)0x00000004) // 编程错误标志，位 2
#define FMC_STS_WPEF    ((uint32_t)0x00000010) // 写保护错误标志，位 4
#define FMC_STS_OCF     ((uint32_t)0x00000020) // 操作完成标志，位 5

#define FMC_CTRL2_PG    ((uint32_t)0x00000001) // 编程，位 0
#define FMC_CTRL2_LOCK  ((uint32_t)0x00000080) // 锁定，位 7 (复位值)
#define FMC_KEY1        ((uint32_t)0x45670123) // 假设的解锁序列 Key 1 (实际值需要查阅手册)
#define FMC_KEY2        ((uint32_t)0xCDEF89AB) // 假设的解锁序列 Key 2

class Fmc : public PeripheralDevice {
private:
    FMC_TypeDef m_registers{};
    std::string m_peripheral_name = "FMC";
    uint64_t m_base_addr = FMC_BASE_ADDR;

    // 简化模拟解锁/锁定状态
    bool m_is_locked = true;
    uint32_t m_key_sequence_step = 0; // 解锁步骤

    void initialize_registers();
    //void update_sts_flags(uint32_t clear_mask); // 更新/清除状态标志位

public:
    Fmc();
    bool handle_write(uc_engine *uc, uint64_t address, int size, int64_t value, void *user_data) override;
    bool handle_read(uc_engine *uc, uint64_t address, int size, int64_t *read_value, void *user_data) override;
    uint64_t getBaseAddress() override { return m_base_addr; }
    std::string getName() override { return m_peripheral_name; }
    uint64_t getSize() override { return FMC_REG_SIZE; }
};