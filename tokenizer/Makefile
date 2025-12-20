CXX = g++
CXXFLAGS = -std=c++20 -Wall
LIBS = -lpq
TARGET = app
SRC = main.cpp Tokenizer.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC) $(LIBS)

clean:
	rm -f $(TARGET)

bench:
	g++ -O2 -std=c++17 tokenize_bench.cpp -o tokenize_bench
	./tokenize_bench

test:
	g++ -std=c++17 -Wall test_tokenizer.cpp -lgtest -lgtest_main -pthread -o test_tokenizer
	./test_tokenizer