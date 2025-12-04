#pragma once
#include <iostream>
#include <utility>
#include <vector>
#include <string>
#include <map>
#include <sstream>
#include <iomanip>
#include <memory>
#include <functional>
#include <QDebug>
using namespace std;
// =========================================================================
// 0. 回调函数定义
// =========================================================================
class Register; // 前向声明，确保 BitFieldWriteCallback 可以引用 Register

/**
 * @brief 位域级别的写入回调函数签名。
 *
 * @param reg 寄存器自身的引用。
 * @param new_field_value 写入该位域的新的值。回调函数在此处执行该位域变化引起的副作用。
 */
using BitFieldWriteCallback = std::function<void(Register &reg, uint32_t new_field_value)>;
/**
 * @brief 寄存器级别的读取回调函数签名。
 *
 * @param reg 寄存器自身的引用。
 * @param value_to_read 对即将返回的值的引用。回调函数可以在这里修改返回值，
 * 例如清除状态位或返回特殊的只读值。
 */
using RegisterReadCallback = std::function<void(Register &reg, uint32_t &value_to_read)>;
// =========================================================================
// 1. 位域定义结构
// =========================================================================

struct BitField {
  string name; // 位域名称，如 "HIRCEN"
  int start_bit; // 起始位 (从 0 开始)
  int width; // 宽度，对于单个位为 1

  // 存储位域值到描述的映射，例如 0->"关闭", 1->"打开"
  map<uint32_t, string> value_descriptions;
  //当此位域被修改时执行的回调函数
  BitFieldWriteCallback write_callback;
};

// =========================================================================
// 2. 寄存器抽象基类
// =========================================================================

class Register {
  protected:
    uint32_t m_offset;
    uint32_t m_current_value = 0x00000000;
    string m_name;
    // 寄存器级别的读取回调函数
    RegisterReadCallback m_read_callback = nullptr;
    std::map<std::string, BitField*> m_field_map;
  public:
    // 存储该寄存器所有位域的定义
    vector<BitField> m_fields;
    static bool s_enable_debug_output;
    Register(uint32_t offset, string name) : m_offset(offset), m_name(std::move(name)) {
    }
    /**
     * @brief 构建位域查找映射。必须在所有 BitField 都被添加到 m_fields 后调用。
     */
    void build_field_map() {
      m_field_map.clear();
      for (auto &field : m_fields) {
        m_field_map[field.name] = &field; // 映射名称到 BitField 对象的地址
      }
    }
    /**
     * @brief 按名称查找位域 (O(log N) 性能)。
     * @param field_name 要查找的位域名称。
     * @return 找到的 BitField 引用指针，如果未找到则返回 nullptr。
     */
    BitField* find_field(const std::string& field_name) {
      auto it = m_field_map.find(field_name);
      if (it != m_field_map.end()) {
        return it->second; // 返回映射中存储的 BitField 指针
      }
      return nullptr; // 未找到
    }
    /**
 * @brief 三参数构造函数，用于设置初始复位值
 */
    Register(uint32_t offset, string name, uint32_t reset_value)
      : m_offset(offset), m_current_value(reset_value), m_name(std::move(name)) {
    }
    virtual ~Register();
    // 设置全局调试开关
    static void set_debug_output(bool enable) {
      s_enable_debug_output = enable;
    }
    // 设置寄存器级别的读取回调
    void set_read_callback(RegisterReadCallback callback) {
      m_read_callback = std::move(callback);
    }
    // 获取寄存器偏移地址
     uint32_t get_offset() const { return m_offset; }

    // 模拟读取操作
    uint32_t read() {
      uint32_t simulated_value = m_current_value; // 默认返回当前内部值

      // 1. 执行全局读取回调
      // 回调函数可以修改 simulated_value 或执行状态清除等副作用操作。
      if (m_read_callback) {
        m_read_callback(*this, simulated_value);
      }

      // 2.读取调试输出
      if (s_enable_debug_output) {
        debug_interpret_read(simulated_value);
        std::fflush(stdout);
      }
      return simulated_value;
    }
    string getName() {
      return m_name;
    }

    // 模拟写入操作
    void write(uint32_t new_value) {
      const uint32_t old_value = m_current_value;

      // 1. 更新内部值
      m_current_value = new_value;

      // 2. 遍历位域，检查变更并执行回调
      // 使用 auto& 遍历 m_fields，因为 write_callback 可能会在内部修改寄存器或其他状态
      for (auto &field: m_fields) {
        // 计算掩码
        const uint32_t mask = ((1U << field.width) - 1) << field.start_bit;

        // 提取旧值和新值的位域值
        const uint32_t old_field_value = (old_value & mask) >> field.start_bit;

        // 检查：1. 值是否发生变化？ 2. 是否设置了回调函数？
        const uint32_t new_field_value = (new_value & mask) >> field.start_bit;
        if (old_field_value != new_field_value && field.write_callback) {
          // 执行回调
          field.write_callback(*this, new_field_value);
        }
      }
      if (s_enable_debug_output) {
        debug_interpret_write(new_value);
        std::fflush(stdout);
      }
    }

  private:
    // 位域解析和显示逻辑 (仅用于调试)
void debug_interpret_write(uint32_t value) const {
  // 显示写入头信息
  qDebug().noquote() << "\n--- 写入寄存器: "
                     << m_name.c_str() // 将 std::string 转换为 C 字符串
                     << QString(" (Offset: 0x%1) ---")
                        .arg(m_offset, 2, 16, QChar('0')); // Qt 格式化: 2位, 十六进制, 填充'0'

  qDebug().noquote() << QString("  新值: 0x%1")
                     .arg(value, 8, 16, QChar('0')); // Qt 格式化: 8位, 十六进制, 填充'0'

  for (const auto &field: m_fields) {
    // 计算掩码 (Mask)
    uint32_t mask = ((1U << field.width) - 1) << field.start_bit;

    // 提取该位域的值
    uint32_t field_value = (value & mask) >> field.start_bit;

    // 构建位域范围字符串
    QString bit_range = QString::number(field.start_bit);
    if (field.width > 1) {
        bit_range += "-" + QString::number(field.start_bit + field.width - 1);
    }

    // 基础输出
    QString output_line = QString("  > **位域 %1: %2** ").arg(bit_range, field.name.c_str());

    // 查找描述信息
    if (field.value_descriptions.count(field_value)) {
      output_line += QString(" (写入值: %1) -> %2").arg(field_value)
                                                  .arg(field.value_descriptions.at(field_value).c_str());
    } else {
      // 如果没有特定描述，直接显示值
      output_line += QString(" (写入值: %1)").arg(field_value);
    }

    qDebug().noquote() << output_line;
  }
  // 注意：使用 QDebug 时，不需要 std::fflush(stdout);
}

// 位域解析和显示逻辑 (新增：仅用于读取调试)
void debug_interpret_read(uint32_t value) const {
  // 显示读取头信息
  qDebug().noquote() << "\n--- 读取寄存器: "
                     << m_name.c_str()
                     << QString(" (Offset: 0x%1) ---")
                        .arg(m_offset, 2, 16, QChar('0'));

  qDebug().noquote() << QString("  返回值: 0x%1")
                     .arg(value, 8, 16, QChar('0'));

  for (const auto &field: m_fields) {
    // 计算掩码 (Mask)
    uint32_t mask = ((1U << field.width) - 1) << field.start_bit;

    // 提取该位域的值
    uint32_t field_value = (value & mask) >> field.start_bit;

    // 构建位域范围字符串
    QString bit_range = QString::number(field.start_bit);
    if (field.width > 1) {
        bit_range += "-" + QString::number(field.start_bit + field.width - 1);
    }

    // 基础输出
    QString output_line = QString("  > **位域 %1: %2** ").arg(bit_range, field.name.c_str());

    // 查找描述信息
    if (field.value_descriptions.count(field_value)) {
      output_line += QString(" (当前值: %1) -> %2").arg(field_value)
                                                  .arg(field.value_descriptions.at(field_value).c_str());
    } else {
      // 如果没有特定描述，直接显示值
      output_line += QString(" (当前值: %1)").arg(field_value);
    }

    qDebug().noquote() << output_line;
  }
}
};
