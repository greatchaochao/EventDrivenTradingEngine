#include "strategy.h"
#include <numeric> // std::accumulate — 用于对区间求和

// ─────────────────────────────────────────────
// sma：计算简单移动平均线（Simple Moving Average）
//
// 参数：
//   prices — 完整的历史价格序列
//   len    — 窗口长度（取序列末尾 len 个价格来计算均值）
//
// 返回值：过去 len 天收盘价的算术平均值
//
// 例如：prices = {30.0, 31.0, 29.0, 32.0, 30.5}，len = 3
//   取末尾 3 个 → {29.0, 32.0, 30.5}
//   总和 = 91.5，平均 = 30.5
//
// std::accumulate 的用法：
//   std::accumulate(起始迭代器, 结束迭代器, 初始值)
//   从起始位置到结束位置依次相加，最后加上初始值，返回总和
//   这里用 prices.end() - len 定位到倒数第 len 个元素
// ─────────────────────────────────────────────
static double sma(const std::vector<double> &prices, int len) {
  // prices.end() - len  指向序列倒数第 len 个元素
  // prices.end()        指向序列末尾（最后一个元素的下一位）
  double sum = std::accumulate(prices.end() - len, prices.end(), 0.0);
  return sum / len; // 除以窗口长度得到平均值
}

// ─────────────────────────────────────────────
// computeSignal 实现
// ─────────────────────────────────────────────
Signal computeSignal(const std::vector<double> &prices, int shortLen,
                     int longLen) {

  // ── 条件 1：数据不足 ──────────────────────
  // 如果当前拿到的价格条数少于长期窗口，
  // 根本无法计算 20 日均线，直接返回 HOLD
  if ((int)prices.size() < longLen) {
    return Signal::HOLD;
  }

  // ── 计算"此时此刻"的短期和长期均线 ────────
  double shortNow = sma(prices, shortLen); // 当前 5 日均线
  double longNow = sma(prices, longLen);   // 当前 20 日均线

  // ── 条件 2：数据只够计算一个截面 ──────────
  // 需要对比"上一刻"和"这一刻"，至少要有 longLen+1 条数据
  // 否则上一刻的均线无法计算，同样返回 HOLD
  if ((int)prices.size() < longLen + 1) {
    return Signal::HOLD;
  }

  // ── 计算"上一时刻"的短期和长期均线 ────────
  // 方法：从 prices 中去掉最后一个元素，得到"昨天"的价格序列
  // 用 std::vector 的范围构造：取 prices 的 [0, size-1) 区间
  std::vector<double> prevPrices(prices.begin(), prices.end() - 1);
  double shortPrev = sma(prevPrices, shortLen); // 上一日 5 日均线
  double longPrev = sma(prevPrices, longLen);   // 上一日 20 日均线

  // ── 判断均线交叉 ──────────────────────────
  // 黄金交叉（Golden Cross）：短期均线从下方穿越长期均线 → BUY
  //   上一刻：shortPrev <= longPrev（短线在长线下方或相等）
  //   这一刻：shortNow  >  longNow（短线升到长线上方）
  if (shortPrev <= longPrev && shortNow > longNow) {
    return Signal::BUY;
  }

  // 死亡交叉（Death Cross）：短期均线从上方穿越长期均线 → SELL
  //   上一刻：shortPrev >= longPrev（短线在长线上方或相等）
  //   这一刻：shortNow  <  longNow（短线跌到长线下方）
  if (shortPrev >= longPrev && shortNow < longNow) {
    return Signal::SELL;
  }

  // 没有发生交叉 → HOLD（观望，不做任何操作）
  return Signal::HOLD;
}
