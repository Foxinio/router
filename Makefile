CC = g++
CXXFLAGS = -std=gnu++17 -g -static -Wall -Wextra

all: router

router: router.o utils.o network_node.o Init.o dgram.o sys_wrappers.o

clean:
	rm -f router *.o szymon_jedras.tar.xz

tar:
	@mkdir szymon_jedras
	@cp *.cpp *.h Makefile szymon_jedras
	@tar cfJ szymon_jedras.tar.xz szymon_jedras
	@rm -rf szymon_jedras
