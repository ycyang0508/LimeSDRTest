#include <chrono>
#include <thread>
#include <iostream>
#include <stdio.h>
#include <csignal>
#ifdef USE_GNU_PLOT
#include "gnuPlotPipe.h"
#endif
#include <atomic>
#include "liquid.h"


extern int bufersize ;
extern void limesdr_init();
extern void limesdr_close();
extern void TX_dat_thread(cbufferf *cb_in_ptr,std::chrono::seconds op_time);
extern void TX_thread(cbufferf *cb_in_ptr,std::chrono::seconds op_time);

extern void RX_dat_thread(cbufferf *cb_in_ptr,std::chrono::seconds op_time);
extern void RX_thread(cbufferf *cb_in_ptr,std::chrono::seconds op_time);

extern int tx_bufersize ;
extern int rx_bufersize ;

void lime_test() {

    std::chrono::seconds op_time(20);

    cbufferf tx_cb = cbufferf_create(tx_bufersize * 2 * 512);
    cbufferf rx_cb = cbufferf_create(rx_bufersize * 2 * 512);

    limesdr_init();    
    std::thread tx_dt_t(TX_dat_thread,&tx_cb,op_time);
    std::thread tx_t(TX_thread,&tx_cb,op_time);
    std::thread rx_dt_t(RX_dat_thread,&rx_cb,op_time);    
    std::thread rx_t(RX_thread,&rx_cb,op_time);

    tx_dt_t.join();
    tx_t.join();
    rx_dt_t.join();
    rx_t.join();

    cbufferf_destroy(tx_cb);
    cbufferf_destroy(rx_cb);
    limesdr_close();
    std::cout << "lime_test done\n";
}

std::atomic<bool> g_exit_flag(false);
void signal_handler(int sig)
{
    switch(sig) {
        case SIGINT:
            std::cout << "Received SIGINT signal" << std::endl;
            g_exit_flag.store(true);
            break;
        case SIGTERM:
            std::cout << "Terminate signal received.\n";
            g_exit_flag.store(true);
            break;
    }
}

int main() {

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    //cb_test();
    lime_test();
    printf("done\n");
    return 0;
}
