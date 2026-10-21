CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17
LDLIBS = -lreadline
.PHONY: clean
myshell: command.cc tokenizer.cc command.h tokenizer.h
	$(CXX) $(CXXFLAGS) command.cc tokenizer.cc  -o myshell $(LDLIBS)
clean:
	rm -f myshell

