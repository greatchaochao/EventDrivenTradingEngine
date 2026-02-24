#include "data_feed.h"
#include <iostream>

int main() {
  std::cout << "Event-Driven Trading Engine -- Market Data Feed" << std::endl;
  std::cout << "------------------------------------------------" << std::endl;

  std::vector<Tick> ticks = loadCSV("data/tr_eikon_eod_data.csv");

  if (ticks.empty()) {
    std::cout << "No data loaded. Check that data/tr_eikon_eod_data.csv exists."
              << std::endl;
    return 1;
  }

  std::cout << "Loaded " << ticks.size() << " AAPL.O ticks." << std::endl
            << std::endl;

  int limit = 10;
  for (int i = 0; i < limit && i < (int)ticks.size(); i++) {
    std::cout << ticks[i].date << "  AAPL.O  price: " << ticks[i].price
              << std::endl;
  }

  std::cout << "...\n(showing first " << limit << " of " << ticks.size()
            << " rows)" << std::endl;
  return 0;
}