histogram: histogram.cpp
	$(CXX) -Wall -Wextra -pedantic -Werror -o $@ -O2 -DNDEBUG $^

.PHONY: format
format:
	clang-format -i --style='{BasedOnStyle: Google, Language: Cpp, ColumnLimit: 80}' histogram.cpp
