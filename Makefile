CC = g++
CXXFLAGS = -std=gnu++17 -g -static -Wall -Wextra

all: router

router: router.o utils.o network_node.o Init.o dgram.o sys_wrappers.o \
				 utils.h network_node.h Init.h dgram.h sys_wrappers.h
	g++ router.o utils.o network_node.o Init.o dgram.o sys_wrappers.o -o router

tester: router.o utils.o network_node.o Init.o dgram.o sys_wrappers_fixture.o \
				 utils.h network_node.h Init.h dgram.h sys_wrappers.h
	g++ router.o utils.o network_node.o Init.o dgram.o sys_wrappers_fixture.o -o tester

clean:
	rm -f router tester *.o szymon_jedras.tar.xz

tar:
	@mkdir szymon_jedras
	@cp *.cpp *.h Makefile szymon_jedras
	@tar cfJ szymon_jedras.tar.xz szymon_jedras
	@rm -rf szymon_jedras
