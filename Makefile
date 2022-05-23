TARGET_C = mainc
TARGET_CPP = maincpp
TARGET_TEST = test
COMMON_OBJECTS = arithmetic.o
C_OBJECTS = mainc.o
CPP_OBJECTS = maincpp.o
HEADERS = arithmetic.h
CFLAGS = -Wall -O2

%.o: %.c $(HEADERS)
	gcc $(CFLAGS) $< -c

%.o: %.cpp $(HEADERS)
	g++ $(CFLAGS) $< -c

$(TARGET_C): $(COMMON_OBJECTS) $(C_OBJECTS)
	gcc $(C_OBJECTS) $(COMMON_OBJECTS) -o $(TARGET_C)

$(TARGET_CPP): $(COMMON_OBJECTS) $(CPP_OBJECTS)
	g++ $(CPP_OBJECTS) $(COMMON_OBJECTS) -o $(TARGET_CPP)

$(TARGET_TEST): arithmetic.c test.c
	gcc $(CFLAGS) -o $(TARGET_TEST) test.c

all: $(TARGET_C) $(TARGET_CPP) $(TARGET_TEST)

clean:
	rm -f $(COMMON_OBJECTS) $(C_OBJECTS) $(CPP_OBJECTS) $(TARGET_C) $(TARGET_CPP) $(TARGET_TEST)

.PHONY: all clean
