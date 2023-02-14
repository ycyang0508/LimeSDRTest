

loopbackTRX : loopbackTRX.cpp
	g++ loopbackTRX.cpp -o loopbackTRX -lLimeSuite -DUSE_GNU_PLOT
	./loopbackTRX | tee run.log


debug : loopbackTRX.cpp
	g++ loopbackTRX.cpp -o loopbackTRX -lLimeSuite -DUSE_GNU_PLOT -DDUMP_DAT
	./loopbackTRX | tee run.log
	grep ref run.log > ref
	grep test run.log > test

clean :
	rm -f loopbackTRX ref test run.log
