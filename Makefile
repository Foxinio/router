CC = g++
CXXFLAGS = -std=gnu++17 -g -static -Wall -Wextra

all: router

router: router.o utils.o network_node.o Init.o dgram.o sys_wrappers.o

tester: router.o utils.o network_node.o Init.o dgram.o test/sys_wrappers_fixture.o
	g++ router.o utils.o network_node.o Init.o dgram.o test/sys_wrappers_fixture.o -o tester

sys_wrappers_fixture.o: test/sys_wrappers_fixture.cpp

clean:
	rm -f router *.o szymon_jedras.tar.xz

tar:
	@mkdir szymon_jedras
	@cp *.cpp *.h Makefile szymon_jedras
	@tar cfJ szymon_jedras.tar.xz szymon_jedras
	@rm -rf szymon_jedras
