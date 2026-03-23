#include "engine.h"
#include "portfolio.h" // 投资组合模块（Week 5）
#include "risk.h"      // 新增：风控模块（Week 6）
#include "strategy.h"  // 信号计算模块（Week 4）
#include <chrono>
#include <fstream> // std::ofstream — 用于写入 CSV 日志文件
#include <iostream>
#include <thread>

void runEngine(const std::vector<Tick> &ticks) {
  std::cout << "引擎启动，共加载 " << ticks.size() << " 条行情数据。"
            << std::endl;
  // 按 Ctrl+C 可以随时停止程序
  //
  // Ctrl+C 会向程序发送一个"信号"（Signal）
  // 信号是操作系统与程序之间的一种通信机制：
  //   - 操作系统检测到用户按下 Ctrl+C
  //   - 向程序发送 SIGINT 信号（Signal Interrupt，即"中断信号"）
  //   - 程序收到后立即终止，无论当前执行到哪一行
  //
  // 这里不需要写任何额外代码——操作系统自动处理这一切
  // 终端退出时显示 "^C"，这是 Ctrl+C 的标准视觉反馈
  std::cout << "按 Ctrl+C 停止。" << std::endl;
  std::cout << std::endl;

  int cycle = 1; // 记录当前是第几轮循环（仅用于终端显示）

  // 外层循环：数据播完后自动重头开始，模拟持续运行的实时行情
  while (true) {
    std::cout << "--- 第 " << cycle << " 轮 ---" << std::endl;

    // ── 每轮开始时初始化账户与风控 ────────────────────
    // 两个模块使用相同的初始资金，保持数值一致
    Portfolio portfolio = createPortfolio(10000.0);
    RiskManager rm = createRiskManager(10000.0); // Week 6 新增

    // ── 每轮开始时创建（或覆盖）日志文件 ──────────────
    // std::ofstream 以写入模式打开文件；若文件已存在则从头覆盖
    // 两个变量（sigCsv, tradeCsv）在本轮 while 循环体结束时
    // 自动关闭（RAII：资源随对象生命周期释放，无需手动 close）
    std::ofstream sigCsv("data/signals.csv");
    std::ofstream tradeCsv("data/trades.csv");

    // 写入 CSV 表头（第一行）
    // signals.csv 新增 Drawdown 列，供 Week 7 绘制回撤曲线
    sigCsv << "Date,Price,Signal,NetValue,Drawdown\n";
    tradeCsv << "Date,Action,Price,Quantity,Commission,CashAfter\n";

    // ── 价格历史缓冲区，每轮循环重置 ────────────────────
    // priceHistory 存储截至当前 tick 的所有收盘价（按时间顺序）
    // computeSignal 需要读取这段历史才能计算短期和长期均线
    std::vector<double> priceHistory;

    double prevPrice = ticks[0].price; // 记录上一个 Tick 的价格，用于比较涨跌

    // 内层循环：逐条处理每一笔行情数据
    for (int i = 0; i < (int)ticks.size(); i++) {
      double price = ticks[i].price;
      std::string date = ticks[i].date;

      // ── 方向判断（沿用 Week 3 的逻辑）──────────────
      std::string direction;
      if (price > prevPrice)
        direction = "UP  ";
      else if (price < prevPrice)
        direction = "DOWN";
      else
        direction = "FLAT";

      // ── 将本 tick 的价格追加进历史缓冲区 ───────────
      priceHistory.push_back(price);

      // ── 计算当前 tick 的交易信号（Week 4 逻辑）──────
      Signal sig = computeSignal(priceHistory);

      // "BUY " 刻意加一个空格，与 "SELL" / "----" 对齐，便于终端阅读
      std::string sigStr;
      if (sig == Signal::BUY)
        sigStr = "BUY ";
      else if (sig == Signal::SELL)
        sigStr = "SELL";
      else
        sigStr = "----";

      // ── 订单执行（含熔断保护，Week 6 新增）────────────
      //
      // 熔断时的两条规则：
      //   规则 1 — 禁止开新仓（跳过 BUY 信号）
      //   规则 2 — 强制平仓（若有持仓，立即执行卖出）
      //
      // 为什么只拦 BUY 而不拦 SELL？
      //   熔断的目的是"止损"，允许卖出可以及时关闭亏损头寸
      //   禁止买入是为了防止在下跌趋势中继续加仓，扩大损失
      if (rm.halted) {
        // 规则 2：如果熔断时仍有持仓，强制卖出
        if (portfolio.position > 0) {
          bool sold = executeSell(portfolio, date, price);
          if (sold) {
            std::cout << "  [强制平仓] " << date << "  卖出 "
                      << portfolio.tradeLog.back().quantity << " 股 @ " << price
                      << std::endl;
          }
        }
        // 规则 1：熔断状态下不执行任何 BUY 信号，直接跳到净值计算
      } else {
        // 正常状态：按信号执行
        if (sig == Signal::BUY) {
          bool traded = executeBuy(portfolio, date, price);
          if (traded) {
            std::cout << "  [买入成交] " << portfolio.tradeLog.back().quantity
                      << " 股 @ " << price
                      << "  手续费: " << portfolio.tradeLog.back().commission
                      << "  剩余现金: " << portfolio.cash << std::endl;
          }
        } else if (sig == Signal::SELL) {
          bool traded = executeSell(portfolio, date, price);
          if (traded) {
            std::cout << "  [卖出成交] " << portfolio.tradeLog.back().quantity
                      << " 股 @ " << price
                      << "  手续费: " << portfolio.tradeLog.back().commission
                      << "  剩余现金: " << portfolio.cash << std::endl;
          }
        }
      }

      // ── 计算当前账户净值 ─────────────────────────────
      // getNetValue = 现金 + 持仓 × 当前价格
      double netValue = getNetValue(portfolio, price);

      // ── 更新风控状态（Week 6 新增）───────────────────
      // 必须在计算 netValue 之后调用，才能传入最新净值
      // updateRisk 内部会：更新峰值、计算回撤、检查熔断条件
      updateRisk(rm, netValue, date);

      // ── 终端输出 ─────────────────────────────────────
      std::cout << date << "  " << direction << "  " << sigStr
                << "  price: " << price << "  net_value: " << netValue
                << "  drawdown: " << rm.drawdown * 100.0 << "%"
                << (rm.halted ? "  [熔断中]" : "") << std::endl;

      // ── 将本行数据写入信号日志（含新的 Drawdown 列）──
      sigCsv << date << "," << price << "," << sigStr << "," << netValue << ","
             << rm.drawdown << "\n";

      prevPrice = price; // 更新上一条价格

      // 每条行情之间暂停 50 毫秒，模拟真实行情的播放节奏
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // ── 周期结束：将成交记录写入 trades.csv ─────────────
    // 范围 for 循环（Range-based for loop，C++11 起支持）：
    //   for (const Trade& t : portfolio.tradeLog) { ... }
    //   const Trade& t — 只读引用，不复制 Trade 对象，不能修改 t
    //   循环自动从头到尾依次取出 tradeLog 中的每个元素
    for (const Trade &t : portfolio.tradeLog) {
      tradeCsv << t.date << "," << t.action << "," << t.price << ","
               << t.quantity << "," << t.commission << "," << t.cashAfter
               << "\n";
    }

    // ── 打印本轮交易摘要 ─────────────────────────────────
    std::cout << std::endl;
    std::cout << "本轮共成交 " << portfolio.tradeLog.size() << " 笔："
              << std::endl;
    for (const Trade &t : portfolio.tradeLog) {
      std::cout << "  " << t.date << "  " << t.action << "  " << t.quantity
                << " 股"
                << "  @ " << t.price << "  手续费: " << t.commission
                << std::endl;
    }

    // 打印最终净值和盈亏
    // ticks.back() 返回 vector 最后一个元素（最后一天的行情）
    double finalNetValue = getNetValue(portfolio, ticks.back().price);
    std::cout << "最终总价值: " << finalNetValue
              << "  盈亏: " << (finalNetValue - 10000.0) << std::endl;

    // Week 6 新增：打印风控摘要
    std::cout << "历史最大回撤: " << rm.maxDrawdown * 100.0 << "%"
              << (rm.halted ? "  （本轮已触发熔断）" : "  （本轮未触发熔断）")
              << std::endl;

    std::cout << std::endl;
    std::cout << "数据播放完毕。日志已写入 data/signals.csv 和 "
                 "data/trades.csv。重新开始..."
              << std::endl;
    std::cout << std::endl;

    cycle++; // 进入下一轮
  }
}
