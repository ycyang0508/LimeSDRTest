

dsp_test :  dsp_test.cpp limesdr_init.cpp TX_thread.cpp RX_thread.cpp
	g++ limesdr_init.cpp dsp_test.cpp TX_thread.cpp RX_thread.cpp \
	-lliquid -lpthread  -lLimeSuite \
	--std=c++17 -w -DUSE_GNU_PLOT \
	-o  dsp_test 
	#./dsp_test


debug :  dsp_test.cpp limesdr_init.cpp TX_thread.cpp RX_thread.cpp
	g++ limesdr_init.cpp dsp_test.cpp TX_thread.cpp RX_thread.cpp \
	-lliquid -lpthread  -lLimeSuite \
	--std=c++17 -w \
	-o  dsp_test 
	#./dsp_test


clean : 
	rm -f dsp_test
