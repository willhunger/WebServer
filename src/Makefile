CXX = g++
CFLAGS = -std=c++14 -O2 -Wall -g 

TARGET = webserver
OBJS = ./pool/*.cc ./timer/*.cc \
       ./http/*.cc ./server/*.cc \
       ./buffer/*.cc ./main.cc

all: $(OBJS)
	$(CXX) $(CFLAGS) $(OBJS) -o ./$(TARGET)  -pthread -lmysqlclient

clean:
	rm -rf ./$(OBJS) $(TARGET)
