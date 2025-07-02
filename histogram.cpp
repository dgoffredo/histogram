#include <algorithm>
#include <cassert>
#include <cctype>
#include <cmath>
#include <fstream>
#include <iostream>
#include <istream>
#include <memory>
#include <optional>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

void read_file(std::istream& in, std::vector<std::pair<double, int>>& data,
               int who, int column, std::optional<double> minimum,
               std::optional<double> maximum) {
  std::string scratch;
  double value;
  std::istringstream line;
  line.exceptions(std::ios::badbit);
  while (std::getline(in, scratch)) {
    if (std::all_of(scratch.begin(), scratch.end(),
                    [](unsigned char ch) { return std::isspace(ch); })) {
      continue;
    }
    line.clear();
    line.str(scratch);
    for (int i = 1; i != column; ++i) {
      line >> scratch;
    }
    line >> value;
    if (!line) {
      throw std::runtime_error("Column " + std::to_string(column) +
                               " is not a number in: " + line.str());
    }
    if ((!minimum || value >= *minimum) && (!maximum || value <= *maximum)) {
      data.emplace_back(value, who);
    }
  }
}

struct Args {
  bool help = false;
  int column = 1;  // one-based
  bool verbose = false;
  std::optional<double> minimum;
  std::optional<double> maximum;
  std::vector<std::string> input_files;
};

int parse_args(Args& result, int argc, char* argv[], std::ostream& error) {
  for (int i = 1; i < argc; ++i) {
    const std::string_view arg = argv[i];
    if (arg == "-h" || arg == "--help") {
      result.help = true;
    } else if (arg == "--column") {
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
    } else if (arg == "--min") {
      if (++i == argc) {
        error << arg << " option missing its argument.\n";
        return 3;
      }
      std::istringstream in{argv[i]};
      in >> result.minimum.emplace();
      if (!in) {
        error << "Argument to " << arg
              << " option is not a real number: " << argv[i] << '\n';
        return 4;
      }
    } else if (arg == "--max") {
      if (++i == argc) {
        error << arg << " option missing its argument.\n";
        return 6;
      }
      std::istringstream in{argv[i]};
      in >> result.maximum.emplace();
      if (!in) {
        error << "Argument to " << arg
              << " option is not a real number: " << argv[i] << '\n';
        return 7;
      }
    } else if (arg == "--verbose") {
      result.verbose = true;
    } else if (!arg.empty() && arg[0] == '-') {
      error << "Argument " << arg
            << " looks like an unknown option.\n"
               "If it's actually the name of a file, then prefix it with "
               "\"./\".\n";
      return 5;
    } else {
      result.input_files.emplace_back(arg);
    }
  }

  return 0;
}

void usage(std::ostream& out, const char* argv0) {
  out << "usage: " << argv0 <<
      R"( [-h | --help] [--column COLUMN] [--min MIN] [--max MAX] [INPUT_FILE ...]

options:

  -h --help
    Print this message to standard output.

  --column COLUMN
    Read values from the one-based COLUMN of each input line.
    COLUMN is 1 (the first column) by default.

  --min MIN
    Ignore input lines whose value is less than MIN.
    By default, no input lines are ignored.

  --max MAX
    Ignore input lines whose value is greater than MAX.
    By default, no input lines are ignored.

  --verbose
    Print statistics to standard error.
)";
}

int main(int argc, char* argv[]) try {
  Args args;
  if (int rc = parse_args(args, argc, argv, std::cerr)) {
    std::cerr << '\n';
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
    read_file(in, data, who, args.column, args.minimum, args.maximum);
    output_files.push_back(
        std::make_unique<std::ofstream>(args.input_files[who] + ".hist"));
    count_in_current_bin.push_back(0);
  }

  // If there aren't any input files, then read from stdin and write to stdout.
  if (args.input_files.empty()) {
    std::istream in{std::cin.rdbuf()};
    in.exceptions(std::ios::badbit);
    const int who = 0;
    read_file(in, data, who, args.column, args.minimum, args.maximum);
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

  if (args.verbose) {
    std::cerr << "n = " << n << "\np25 = " << p25 << "\np75 = " << p75
              << "\nbin_width = " << bin_width
              << "\nmin = " << data.front().first
              << "\nmax = " << data.back().first << "\nnum_bins = "
              << ((data.back().first - data.front().first) / bin_width) << '\n';
  }

  double bottom_of_current_bin = data[0].first;
  double bottom_of_next_bin = bottom_of_current_bin + bin_width;
  for (const auto& [value, who] : data) {
    if (value >= bottom_of_next_bin) {
      // Output the current bin.
      for (std::size_t who = 0; who < output_files.size(); ++who) {
        if (count_in_current_bin[who]) {
          *output_files[who] << bottom_of_current_bin << ' '
                             << count_in_current_bin[who] << '\n';
          count_in_current_bin[who] = 0;
        }
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
    if (count_in_current_bin[who]) {
      *output_files[who] << bottom_of_current_bin << ' '
                         << count_in_current_bin[who] << '\n';
    }
  }
} catch (const std::exception& error) {
  std::cerr << error.what() << '\n';
  return 1;
}
