assembler: simulator.o
	g++ simulator.o -o riscv_sim

assembler.o: simulator.cpp
	g++ -c simulator.cpp

clean: rm *.o riscv_sim