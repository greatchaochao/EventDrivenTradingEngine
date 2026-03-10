#include "engine.h"
#include "strategy.h" // 引入信号计算函数 computeSignal 和 Signal 枚举
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

    // ── 每轮开始时创建（或覆盖）信号日志文件 ──────────────
    // std::ofstream 以写入模式打开文件；若文件已存在则从头覆盖
    // 变量 csv 在本轮 while
    // 循环体结束时自动关闭（RAII：资源随对象生命周期释放）
    std::ofstream csv("data/signals.csv");
    csv << "Date,Price,Signal\n"; // 写入 CSV 表头（第一行）

    // ── 价格历史缓冲区，每轮循环重置 ────────────────────
    // priceHistory 存储截至当前 tick 的所有收盘价（按时间顺序）
    // computeSignal 需要读取这段历史才能计算短期和长期均线
    std::vector<double> priceHistory;

    double prevPrice = ticks[0].price; // 记录上一个 Tick 的价格，用于比较涨跌

    // 内层循环：逐条处理每一笔行情数据
    //
    // for 循环由三个部分组成，用分号隔开：
    //
    //   ① int i = 0
    //      初始化：循环开始前执行一次，创建计数器 i 并设为 0
    //      i 代表当前处理的是第几条数据（从第 0 条开始）
    //
    //   ② i < (int)ticks.size()
    //      条件判断：每次循环开始前检查一次，若条件为假则退出循环
    //      ticks.size() 返回列表的总长度（即数据条数）
    //      (int) 是类型转换：将 size() 返回的无符号整数转为有符号 int，
    //      避免 i 与它比较时编译器产生警告
    //
    //   ③ i++
    //      更新：每次循环结束后执行，将 i 加 1，移动到下一条数据
    //      i++ 等价于 i = i + 1
    //
    // 完整执行顺序：① → 检查② → 执行循环体 → ③ → 检查② → 执行循环体 → ③ → ...
    for (int i = 0; i < (int)ticks.size(); i++) {
      // [] 是「下标运算符」（也叫随机访问运算符）
      //
      // vector 在内存中把所有元素紧挨着排成一排：
      //   ticks[0]  ticks[1]  ticks[2]  ...  ticks[n-1]
      //
      // ticks[i] 的意思是：「直接跳到第 i 个位置，取出那里的元素」
      //   - 不需要从头一个个查找，CPU 直接计算地址：起始地址 + i × 元素大小
      //   - 无论 i 是 0 还是 2000，访问速度完全相同 —— 这就是「随机访问」的含义
      //     （「随机」不是指随机数，而是指「任意位置都可以直接抵达」）
      //
      // 注意：i 必须在合法范围内（0 ≤ i < ticks.size()）
      //   若 i 越界（如 ticks[9999] 而列表只有 100 个元素），程序会崩溃
      //   for 循环的条件 i < (int)ticks.size() 正是为了防止这一点
      double price = ticks[i].price;

      // 将当前价格与上一条行情比较，判断涨跌方向
      std::string direction;
      if (price > prevPrice)
        direction = "UP  "; // 价格上涨
      else if (price < prevPrice)
        direction = "DOWN"; // 价格下跌
      else
        direction = "FLAT"; // 价格不变

      // ── 将本 tick 的价格追加进历史缓冲区 ──────────────
      // 追加后 priceHistory 包含从第 1 天到当前日的所有价格
      priceHistory.push_back(price);

      // ── 计算当前 tick 的交易信号 ───────────────────────
      // computeSignal 内部会检查 priceHistory 的长度：
      //   - 不足 20 条 → 返回 HOLD（长期均线数据不够）
      //   - 满 20 条以上 → 比较短期(5日)与长期(20日)均线是否发生交叉
      Signal sig = computeSignal(priceHistory);

      // ── 将 Signal 枚举转换为可打印的字符串 ─────────────
      // "BUY " 刻意加一个空格，与 "SELL" / "----" 对齐，便于终端阅读
      std::string sigStr;
      if (sig == Signal::BUY)
        sigStr = "BUY ";
      else if (sig == Signal::SELL)
        sigStr = "SELL";
      else
        sigStr = "----";

      std::cout << ticks[i].date << "  " << direction << "  " << sigStr
                << "  price: " << price << std::endl;

      // ── 将本行数据写入 CSV 日志 ───────────────────────
      // 格式：日期,价格,信号   （无空格，符合标准 CSV 规范）
      csv << ticks[i].date << "," << price << "," << sigStr << "\n";

      prevPrice = price; // 更新上一条价格，供下一次循环使用

      // 每条行情之间暂停 50 毫秒，模拟真实行情的播放节奏
      //
      // std::this_thread：指"当前正在运行的线程"（可以理解为程序的执行流程）
      // sleep_for(...)：让这个执行流程暂停一段时间，暂停期间不占用 CPU
      //
      // std::chrono：C++ 的时间库，专门用来表示"时间长度"
      // milliseconds(50)：构造一个"50 毫秒"的时间段
      //   1 秒 = 1000 毫秒，所以 50 毫秒约为 0.05 秒
      //   修改这个数字可以控制行情播放的速度：
      //   数字越小 → 播放越快；数字越大 → 播放越慢
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    std::cout << std::endl;
    // csv 在此离开作用域，ofstream 析构函数自动 flush 并关闭文件
    std::cout << "数据播放完毕。信号日志已写入 data/signals.csv。重新开始..."
              << std::endl;
    std::cout << std::endl;

    // ++ 是「自增运算符」，专门用来把一个整数加 1
    //
    // cycle++ 等价于 cycle = cycle + 1
    //   先读取 cycle 的当前值，然后将其加 1 并写回
    //
    // 为什么不直接写 cycle = cycle + 1？
    //   功能完全相同，但 cycle++ 更简洁，是 C++ 中约定俗成的写法
    //   （C++ 这门语言的名字本身就来自 C++，意为"在 C 的基础上进化了一步"）
    //
    // ++ 有两种写法：
    //   cycle++（后置）：先使用当前值，再加 1 —— 本行只做计数，顺序无影响
    //   ++cycle（前置）：先加 1，再使用新值
    //   在单独一行只做自增时，两者效果完全一致；习惯上用后置 cycle++
    cycle++; // 进入下一轮
  }
}
