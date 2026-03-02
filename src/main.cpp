#include "data_feed.h"
#include "engine.h"
#include <iostream>

int main() {
  std::cout << "事件驱动交易引擎" << std::endl;
  std::cout << "----------------" << std::endl;

  // 从 CSV 文件中读取苹果公司的历史行情数据，存入 ticks 列表
  std::vector<Tick> ticks = loadCSV("data/tr_eikon_eod_data.csv");

  // 若数据为空（文件不存在或路径错误），打印提示并退出
  if (ticks.empty()) {
    std::cout << "未能加载数据，请确认 data/tr_eikon_eod_data.csv 文件存在。"
              << std::endl;
    return 1; // 返回非零值表示程序异常退出
  }

  // 将行情数据交给引擎，启动逐笔驱动的模拟循环
  runEngine(ticks);
  return 0; // 返回 0 表示程序正常退出（实际上不会到达这里，因为引擎会持续运行）
}