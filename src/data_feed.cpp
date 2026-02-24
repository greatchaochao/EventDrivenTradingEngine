#include "data_feed.h"
#include <fstream>
#include <iostream>
#include <sstream>

std::vector<Tick> loadCSV(const std::string &filename) {
  std::vector<Tick> ticks;
  std::ifstream file(filename);

  if (!file.is_open()) {
    std::cerr << "Error: could not open " << filename << std::endl;
    return ticks;
  }

  std::string line;
  std::getline(file, line); // skip the header row

  while (std::getline(file, line)) {
    std::stringstream ss(line);
    std::string date, priceStr;

    std::getline(ss, date, ',');     // column 0: Date
    std::getline(ss, priceStr, ','); // column 1: AAPL.O

    if (priceStr.empty())
      continue; // row has no Apple price

    Tick t;
    t.date = date;
    t.price = std::stod(priceStr);
    ticks.push_back(t);
  }

  return ticks;
}