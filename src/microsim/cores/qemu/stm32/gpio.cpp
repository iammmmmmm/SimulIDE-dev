#include "gpio.h"

#include <cstdint>
#include <iostream>

/**
 * @brief Gpio 类的构造函数
 * @param base_addr GPIO 端口的基地址
 * @param port_name 端口名称 (如 "Port A")
 */
Gpio::Gpio(const uint64_t base_addr, const std::string& port_name): m_port_name(port_name), m_base_addr(base_addr) {

    // 初始化寄存器到复位值
    initialize_registers();
    std::cout << "[GPIO] Initialized " << Gpio::getName()
              << " at base address 0x" << std::hex << m_base_addr << std::endl;
}

/**
 * @brief 初始化 GPIO 寄存器到复位值
 */
void Gpio::initialize_registers() {
    // GPIOx_CFGLOW (0x00) 复位值: 0x4444 4444
    m_registers.CFGLOW = (uint32_t)0x44444444;
    // GPIOx_CFGHIG (0x04) 复位值: 0x4444 4444
    m_registers.CFGHIG = (uint32_t)0x44444444;
    // GPIOx_IDATA (0x08) 复位值: 0x0000 XXXX (输入数据寄存器，高 16 位保留，低 16 位不确定)
    // 模拟时通常将其初始化为 0 或一个已知状态，但由于是只读，其值由外部输入决定。
    // 在 C++ 结构体初始化中，非 volatile 成员默认初始化，但 IDATA 是 const volatile，这里不显式赋值。
    // 为了模拟，可以将其视为 0，但实际由 handle_read 返回。
    // m_registers.IDATA = (uint32_t)0x00000000;

    // GPIOx_ODATA (0x0C) 复位值: 0x0000 0000
    m_registers.ODATA = (uint32_t)0x00000000;
    // GPIOx_BSC (0x10) 复位值: 0x0000 0000 (只写寄存器)
    m_registers.BSC = (uint32_t)0x00000000;
    // GPIOx_BC (0x14) 复位值: 0x0000 0000 (只写寄存器)
    m_registers.BC = (uint32_t)0x00000000;
    // GPIOx_LOCK (0x18) 复位值: 0x0000 0000
    m_registers.LOCK = (uint32_t)0x00000000;
}
bool Gpio::is_clock_enabled() const {
    return true;
}


// -----------------------------------------------------------------------------
// 读操作处理
// -----------------------------------------------------------------------------

/**
 * @brief 处理对 GPIO 寄存器的读操作
 */
bool Gpio::handle_read(uc_engine *uc, uint64_t address, int size, int64_t *read_value, void *user_data) {
    // 仅支持 32 位读写
    if (size != 4) {
        std::cerr << "[GPIO] Error: Only 32-bit (4 byte) access is supported for " << getName() << std::endl;
        return false;
    }

    uint32_t offset = (uint32_t)(address - m_base_addr);
    uint32_t reg_value = 0;

    switch (offset) {
        case GPIO_CFGLOW_OFFSET:
            reg_value = m_registers.CFGLOW;
            break;
        case GPIO_CFGHIG_OFFSET:
            reg_value = m_registers.CFGHIG;
            break;
        case GPIO_IDATA_OFFSET:
            // 端口输入数据寄存器 (只读, 位 15:0 有效, 31:16 保留)
            // 注意: m_registers.IDATA 应该由外部输入更新
            reg_value = m_registers.IDATA & 0x0000FFFF;
            break;
        case GPIO_ODATA_OFFSET:
            // 端口输出数据寄存器 (可读写, 位 15:0 有效, 31:16 保留)
            reg_value = m_registers.ODATA & 0x0000FFFF;
            break;
        case GPIO_BSC_OFFSET:
        case GPIO_BC_OFFSET:
            // BSC 和 BC 是只写寄存器，读操作返回未定义值，这里返回 0
            reg_value = 0x00000000;
            break;
        case GPIO_LOCK_OFFSET:
            reg_value = m_registers.LOCK;
            break;
        default:
            std::cerr << "[GPIO] Warning: Read from unsupported offset 0x"
                      << std::hex << offset << " in " << getName() << std::endl;
            // 读取保留地址通常返回 0
            reg_value = 0;
            break;
    }

    *read_value = (int64_t)reg_value;
    // std::cout << "[GPIO] Read 0x" << std::hex << reg_value << " from 0x" << address << std::endl;
    return true;
}


// -----------------------------------------------------------------------------
// 写操作处理
// -----------------------------------------------------------------------------

/**
 * @brief 处理对 GPIO 寄存器的写操作
 */
bool Gpio::handle_write(uc_engine *uc, uint64_t address, int size, int64_t value, void *user_data) {
    // 仅支持 32 位读写
    if (size != 4) {
        qWarning("[GPIO] Error: Only 32-bit (4 byte) access is supported for %s", getName().c_str());
        return false;
    }

    uint32_t offset = (uint32_t)(address - m_base_addr);
    uint32_t write_value = (uint32_t)value;

    switch (offset) {
        case GPIO_CFGLOW_OFFSET:
        case GPIO_CFGHIG_OFFSET:
            // 检查 LOCK 状态, 如果 LOCKKEY=1 且对应位被锁，则忽略写操作
            // 简化: 假设只有在 LOCKKEY=0 时才能写
            if ((m_registers.LOCK & (1 << 16)) == 0) { // LOCKKEY=0
                if (offset == GPIO_CFGLOW_OFFSET) {
                    m_registers.CFGLOW = write_value;
                } else {
                    m_registers.CFGHIG = write_value;
                }
            } else {
                // std::cout << "[GPIO] Warning: Configuration is locked. Write ignored." << std::endl;
            }
            break;
        case GPIO_IDATA_OFFSET:
            // IDATA 是只读寄存器，忽略写操作
            std::cerr << "[GPIO] Warning: Attempted write to read-only register IDATA at 0x"
                      << std::hex << address << " in " << getName() << std::endl;
            qWarning("[GPIO] Warning: Attempted write to read-only register IDATA at %x,in %s ", static_cast<unsigned>(address),getName().c_str());
            break;
        case GPIO_ODATA_OFFSET:
            // ODATA (可读写, 只写低 16 位)
            m_registers.ODATA = (m_registers.ODATA & 0xFFFF0000) | (write_value & 0x0000FFFF);
            break;
        case GPIO_BSC_OFFSET: {
            // BSC (只写) 端口位设置/清除寄存器
            // 位 15:0 (BSy): 写 1 置位 ODATA 对应位
            // 位 31:16 (BCy): 写 1 清除 ODATA 对应位
            // 注: 如果同时设置了 BSy 和 BCy，BSy 位起作用

            uint32_t set_mask = write_value & 0x0000FFFF;    // BSy (设置位)
            uint32_t clear_mask = (write_value >> 16) & 0x0000FFFF; // BCy (清除位)

            // 清除操作 (在置位操作之前，因为 BSy 优先)
            m_registers.ODATA &= (~clear_mask);
            // 置位操作
            m_registers.ODATA |= set_mask;

            // BSC 自身不保存值，写后即清零 (在硬件中是这样)
            // m_registers.BSC = 0;
            break;
        }
        case GPIO_BC_OFFSET:
            // BC (只写) 端口位清除寄存器 (只写低 16 位清除，高 16 位保留)
            m_registers.ODATA &= (~(write_value & 0x0000FFFF));
            // m_registers.BC = 0;
            break;
        case GPIO_LOCK_OFFSET:
            // 端口配置锁定寄存器
            // 写入锁键序列时会生效，否则只写入 LOCKy 位。
            // 锁键写入序列: 写 1 -> 写 0 -> 写 1 (写入 0x00010000)

            // 简化锁定逻辑：假设写入 0x00010000 触发 LOCKKEY=1
            // 实际硬件需要状态机来跟踪 4 步写序列。这里仅简单模拟 LOCKKEY 位的设置。
            if (write_value == (1 << 16)) {
                 // 简单模拟 写 1
                m_registers.LOCK = (m_registers.LOCK & 0x0000FFFF) | (write_value & 0x0001FFFF);
            } else if (write_value == 0 && (m_registers.LOCK & (1 << 16))) {
                // 简单模拟 写 0
                 m_registers.LOCK = (m_registers.LOCK & 0x0000FFFF) | (write_value & 0x0001FFFF);
            } else if (write_value == (1 << 16) && (m_registers.LOCK & (1 << 16)) == 0) {
                 // 简单模拟 写 1 (激活)
                m_registers.LOCK |= 0x00010000; // 激活 LOCKKEY
            } else {
                 // 仅写入 LOCKy 位
                 m_registers.LOCK = (m_registers.LOCK & 0xFFFF0000) | (write_value & 0x0000FFFF);
            }
            break;

        default:
            std::cerr << "[GPIO] Warning: Write to unsupported offset 0x"
                      << std::hex << offset << " in " << getName() << ". Value: 0x"
                      << write_value << std::endl;
            return false;
    }

    // std::cout << "[GPIO] Write 0x" << std::hex << write_value << " to 0x" << address << std::endl;
    return true;
}