#pragma once
// strategy.h — 策略模块的声明文件
// 本文件定义了策略所需的数据类型，并声明了信号计算函数
// 任何想要使用策略功能的文件，只需 #include "strategy.h" 即可

#include <vector>

// ─────────────────────────────────────────────
// Signal：枚举类，表示策略产生的三种信号
//   BUY  — 买入信号（短期均线从下方穿越长期均线）
//   SELL — 卖出信号（短期均线从上方穿越长期均线）
//   HOLD — 无操作（均线未发生交叉，持仓不变）
//
// 使用 "enum class" 而非普通 "enum" 的原因：
//   enum class 会把枚举值限定在自己的命名空间里
//   例如必须写 Signal::BUY，而不能直接写 BUY
//   这样可以避免与其他模块的同名变量发生冲突
// ─────────────────────────────────────────────
enum class Signal {
  BUY,  // 买入
  SELL, // 卖出
  HOLD  // 观望
};

// ─────────────────────────────────────────────
// computeSignal：根据历史价格序列，计算当前时刻的交易信号
//
// 参数：
//   prices   — 截至当前时刻的全部收盘价（最新价格在末尾）
//   shortLen — 短期均线的窗口长度，默认 5 天
//   longLen  — 长期均线的窗口长度，默认 20 天
//
// 返回值：Signal::BUY / Signal::SELL / Signal::HOLD
//
// 策略逻辑（SMA 均线交叉）：
//   当 prices 数量不足 longLen 时，返回 HOLD（数据不够，无法判断）
//   当 shortSMA 从上一刻的"低于 longSMA"变为"高于 longSMA" → BUY
//   当 shortSMA 从上一刻的"高于 longSMA"变为"低于 longSMA" → SELL
//   其余情况 → HOLD
// ─────────────────────────────────────────────
Signal computeSignal(const std::vector<double> &prices, int shortLen = 5,
                     int longLen = 20);
