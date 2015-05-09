# 编译器设定和编译选项
CC = gcc
CFLAGS = -Wall -I./include
ASFLAGS = -ggdb -m32 -MD
GITFLAGS = -m "Makefile auto commit" --no-verify --allow-empty

#编译目标
CFILES = $(shell find src/ -name "*.c")
OBJS = $(CFILES:.c=.o)

ser: $(OBJS)
	gcc -Wall -pedantic -std=c99 -g -pthread $(OBJS) -o ser

-include $(patsubst %.o, %.d, $(OBJS))

# 定义伪目标
.PHONY: play clean git
clean:
	rm -f ser $(OBJS) $(OBJS:.o=.d)

play:	ser
	./ser
	-@git add . --ignore-errors &> /dev/null # KEEP IT
	-@git commit $(GITFLAGS) # KEEP IT
	
git:	
	-@git add . --ignore-errors &> /dev/null # KEEP IT
	-@git commit $(GITFLAGS) # KEEP IT


