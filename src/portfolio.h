#pragma once
// portfolio.h — 投资组合模块的声明文件
// 本文件定义账户所需的数据类型，并声明三个操作函数
// 任何需要管理账户的文件，只需 #include "portfolio.h" 即可

#include <string>
#include <vector>

// ─────────────────────────────────────────────
// Trade：记录一次完整的成交记录
//
// 每当 BUY 或 SELL 信号触发真实成交时，就创建一个 Trade 对象并保存下来
// 这些记录最终写入 data/trades.csv，供 Week 7 的 Python 脚本绘图使用
// ─────────────────────────────────────────────
struct Trade {
  std::string date;   // 成交日期，例如 "2010-02-17"
  std::string action; // 操作方向："BUY" 或 "SELL"
  double price;       // 成交价格（单位：美元/股）
  int quantity;       // 成交数量（股数）
  double commission;  // 手续费 = 成交金额 × 0.1%
  double cashAfter;   // 成交后账户剩余现金
};

// ─────────────────────────────────────────────
// Portfolio：实时记录账户的资产状态
//
//   cash      — 账户中当前持有的现金
//   position  — 当前持有的股票数量（本系统只持有一种资产：AAPL）
//   tradeLog  — 历史成交记录，用 vector 存储所有 Trade 对象
//
// 设计原则：Portfolio 是一个纯数据容器（Plain Data Struct）
//   它自己不做任何计算，只存储状态
//   所有修改都通过下方的三个函数完成
// ─────────────────────────────────────────────
struct Portfolio {
  double cash;                 // 账户现金
  int position;                // 持仓数量（股数）
  std::vector<Trade> tradeLog; // 成交日志
};

// ─────────────────────────────────────────────
// createPortfolio：初始化一个空账户
//
// 参数：
//   initialCash — 初始资金（例如 10000.0）
//
// 返回：一个现金充足、持仓为零的 Portfolio 对象
// ─────────────────────────────────────────────
Portfolio createPortfolio(double initialCash);

// ─────────────────────────────────────────────
// executeBuy：以当前价格买入尽可能多的股票
//
// 参数：
//   p     — Portfolio 的引用（& 代表引用，函数内修改会影响外部变量）
//   date  — 本次成交的日期字符串
//   price — 本次成交的价格
//
// 返回：
//   true  — 成交成功（已买入股票）
//   false — 成交失败（原因：已有持仓，或现金不足）
//
// 注意：& 与 const & 的区别
//   const Portfolio& p  — 只读引用，函数不能修改 p（Week 3/4 中 runEngine
//   的参数）
//         Portfolio& p  — 可写引用，函数可以修改 p（本函数需要扣现金、增持仓）
// ─────────────────────────────────────────────
bool executeBuy(Portfolio &p, const std::string &date, double price);

// ─────────────────────────────────────────────
// executeSell：卖出全部持仓
//
// 参数：同 executeBuy
//
// 返回：
//   true  — 成交成功（已卖出全部股票）
//   false — 成交失败（原因：当前没有持仓）
// ─────────────────────────────────────────────
bool executeSell(Portfolio &p, const std::string &date, double price);

// ─────────────────────────────────────────────
// getNetValue：计算账户当前总价值
//
// 公式：总价值 = 现金 + 持仓股数 × 当前价格
//
// 参数：
//   p            — 只读引用（不修改账户，仅读取数据）
//   currentPrice — 本 tick 的最新市场价格
//
// 返回：以浮点数表示的账户总价值（美元）
// ─────────────────────────────────────────────
double getNetValue(const Portfolio &p, double currentPrice);
