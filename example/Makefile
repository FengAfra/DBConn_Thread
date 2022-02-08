CC      := g++ -std=c++11
CFLAGS  :=-Wall -O2 -lm

TARGET  := Demo
CLIBS   := -lDBConnThreads -lCSLog -llog4cplus -L/usr/lib64/mysql -lmysqlclient
INCLUDES:=-I ../include/ -I/usr/include/mysql
SRCS    :=$(wildcard *.cpp)
OBJS    :=$(patsubst %cpp, %o, $(SRCS))
all     : $(TARGET)
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(CLIBS)

%.o:%.cpp
	$(CC) -c $(INCLUDES) $(CFLAGS) $^




clean:
	rm -rf *.o $(TARGET) $(OBJS)
