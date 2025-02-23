#include <algorithm>
#include <cassert>
#include <cctype>
#include <cmath>
#include <fstream>
#include <iostream>
#include <istream>
#include <memory>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

void read_file(std::istream& in, std::vector<std::pair<double, int>>& data,
               int who, int column) {
  std::string scratch;
  double value;
  std::istringstream line;
  line.exceptions(std::ios::badbit);
  while (std::getline(in, scratch)) {
    if (std::all_of(scratch.begin(), scratch.end(),
                    [](unsigned char ch) { return std::isspace(ch); })) {
      continue;
    }
    line.str(scratch);
    for (int i = 1; i != column; ++i) {
      line >> scratch;
    }
    line >> value;
    if (!line) {
      throw std::runtime_error("Column " + std::to_string(column) +
                               " is not a number in: " + line.str());
    }
    data.emplace_back(value, who);
  }
}

struct Args {
  bool help = false;
  int column = 1;  // one-based
  std::vector<std::string> input_files;
};

int parse_args(Args& result, int argc, char* argv[], std::ostream& error) {
  for (int i = 1; i < argc; ++i) {
    const std::string_view arg = argv[i];
    if (arg == "-h" || arg == "--help") {
      result.help = true;
    } else if (arg == "-c" || arg == "--column") {
      if (++i == argc) {
        error << arg << " option missing its integer argument.\n";
        return 1;
      }
      std::istringstream in{argv[i]};
      in >> result.column;
      if (!in) {
        error << "Argument to " << arg
              << " option is not an integer: " << argv[i] << '\n';
        return 2;
      }
    } else {
      result.input_files.emplace_back(arg);
    }
  }

  return 0;
}

void usage(std::ostream& out, const char* argv0) {
  out << "usage: " << argv0
      << " [-h | --help] [(-c | --column) COLUMN] [INPUT_FILE ...]\n";
}

int main(int argc, char* argv[]) try {
  Args args;
  if (int rc = parse_args(args, argc, argv, std::cerr)) {
    usage(std::cerr, argv[0]);
    return rc;
  }
  if (args.help) {
    usage(std::cout, argv[0]);
    return 0;
  }

  std::vector<std::pair<double, int>> data;
  std::vector<std::unique_ptr<std::ostream>> output_files;
  std::vector<unsigned long long> count_in_current_bin;

  for (std::size_t who = 0; who < args.input_files.size(); ++who) {
    std::ifstream in;
    in.exceptions(std::ios::badbit);
    in.open(args.input_files[who]);
    read_file(in, data, who, args.column);
    output_files.push_back(
        std::make_unique<std::ofstream>(args.input_files[who] + ".hist"));
    count_in_current_bin.push_back(0);
  }

  // If there aren't any input files, then read from stdin and write to stdout.
  if (args.input_files.empty()) {
    std::istream in{std::cin.rdbuf()};
    in.exceptions(std::ios::badbit);
    const int who = 0;
    read_file(in, data, who, args.column);
    output_files.push_back(std::make_unique<std::ostream>(std::cout.rdbuf()));
    count_in_current_bin.push_back(0);
  }

  const auto by_first = [](const auto& left, const auto& right) {
    return left.first < right.first;
  };
  std::sort(data.begin(), data.end(), by_first);

  assert(!output_files.empty());
  assert(output_files.size() == count_in_current_bin.size());

  const std::size_t n = data.size();
  const double p25 = data[(25 * n) / 100].first;
  const double p75 = data[(75 * n) / 100].first;
  const double bin_width = 2 * (p75 - p25) / std::cbrt(n);

  double bottom_of_current_bin = data[0].first;
  double bottom_of_next_bin = bottom_of_current_bin + bin_width;
  for (const auto& [value, who] : data) {
    if (value >= bottom_of_next_bin) {
      // Output the current bin.
      for (std::size_t who = 0; who < output_files.size(); ++who) {
        *output_files[who] << bottom_of_current_bin << ' '
                           << count_in_current_bin[who] << '\n';
        count_in_current_bin[who] = 0;
      }
      // On to the next.
      bottom_of_current_bin = bottom_of_next_bin;
      bottom_of_next_bin += bin_width;
    } else {
      ++count_in_current_bin[who];
    }
  }

  // Output the final bin.
  for (std::size_t who = 0; who < output_files.size(); ++who) {
    *output_files[who] << bottom_of_current_bin << ' '
                       << count_in_current_bin[who] << '\n';
  }
} catch (const std::exception& error) {
  std::cerr << error.what() << '\n';
  return 1;
}
