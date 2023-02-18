#include "lime/LimeSuite.h"
#include <iostream>
#include <math.h>
#include <chrono>
#include <thread>
#include <mutex> 
#include <atomic>
#include <chrono>
#include <memory>


#ifdef USE_GNU_PLOT
#include "gnuPlotPipe.h"
#endif
#include "threadSafeQueue.h"
#include "liquid.h"

typedef std::shared_ptr<uint16_t[]> buf_ptr;
typedef ThreadSafeQueue< buf_ptr > thr_safe_q_t;
typedef float rf_rx_dat_t;

extern lms_device_t* device;
extern const int chCount ; //number of RX/TX streams
extern int error();
int rx_bufersize = 1024 * 64; //complex samples per buffer

extern std::atomic<bool> g_exit_flag;

lms_stream_t rx_streams[2];

using namespace std;

void do_init_rx()
{
    for (int i = 0; i < chCount; ++i)
    {
        rx_streams[i].channel = i; //channel number
        rx_streams[i].fifoSize = 1024 * 1024 * 16; //fifo size in samples
        rx_streams[i].throughputVsLatency = 0.5; //some middle ground
        rx_streams[i].isTx = false; //RX channel
        rx_streams[i].dataFmt = lms_stream_t::LMS_FMT_F32; //12-bit integers
        if (LMS_SetupStream(device, &rx_streams[i]) != 0)
            error();
    }
    //Start streaming
    for (int i = 0; i < chCount; ++i)
    {        
        LMS_StartStream(&rx_streams[i]);
    }
}

void RX_thread(cbufferf *cb_in_ptr,std::chrono::seconds op_time) {

    rf_rx_dat_t * rx_buffers[2];

    do_init_rx();

    
    //Initialize data buffers    
    for (int i = 0; i < chCount; ++i)
    {
        rx_buffers[i] = new rf_rx_dat_t[rx_bufersize * 2]; //buffer to hold complex values (2*samples))
    }


    lms_stream_meta_t rx_metadata; //Use metadata for additional control over sample receive function behavior
    rx_metadata.flushPartialPacket = false; //currently has no effect in RX
    rx_metadata.waitForTimestamp = false; //currently has no effect in RX

    auto t1 = chrono::high_resolution_clock::now();
    auto t2 = t1;

    
    //while (chrono::high_resolution_clock::now() - t1 < chrono::seconds(op_time)) //run for 10 seconds
    while (!g_exit_flag.load())
    {
        int samplesRead;

        for (int i = 0; i < chCount; ++i)
        {            
            //Receive samples
            samplesRead = LMS_RecvStream(&rx_streams[i], rx_buffers[i], rx_bufersize, &rx_metadata, 1000);
        }

        cbufferf_write(*cb_in_ptr, rx_buffers[0], samplesRead * 2);

        //Print stats every 1s
        if (chrono::high_resolution_clock::now() - t2 > chrono::milliseconds(100))
        {
            t2 = chrono::high_resolution_clock::now();
            //Print stats
            lms_stream_status_t status;
            LMS_GetStreamStatus(rx_streams, &status); //Obtain RX stream stats
            //cout << "RX rate: " << status.linkRate / 1e6 << " MB/s\n"; //link data rate (both channels))
            //cout << "RX 0 FIFO: " << 100 * status.fifoFilledCount / status.fifoSize << "% " \
                                  <<  status.overrun << " " << status.overrun  \
                                  <<  " drop " << status.droppedPackets \
                                  <<  " timestamp " << status.timestamp \
                                  << endl; //percentage of RX 0 fifo filled
                                   
        }
        std::this_thread::yield();
    }


    //Stop streaming
    for (int i = 0; i < chCount; ++i)
    {
        LMS_StopStream(&rx_streams[i]); //stream is stopped but can be started again with LMS_StartStream()
    }
    for (int i = 0; i < chCount; ++i)
    {
        LMS_DestroyStream(device, &rx_streams[i]); //stream is deallocated and can no longer be used
        delete[] rx_buffers[i];
    }

    std::cout << "RX_thread done\n";
}

void RX_dat_thread(cbufferf *cb_in_ptr,std::chrono::seconds op_time) {

    float *recv_buf_ptr;
#ifdef USE_GNU_PLOT
    GNUPlotPipe gp;
    gp.write("set size square\n set xrange[-2:2]\n set yrange[-2:2]\n");
#endif

    auto t1 = chrono::high_resolution_clock::now();
    auto t2 = t1;

    //while (chrono::high_resolution_clock::now() - t1 < chrono::seconds(op_time)) //run for 10 seconds
    while (!g_exit_flag.load())
    {
        while(1) {            
            //std::cout << "wait dat " << cbufferf_size(*cb_in_ptr) << " " << testCnt++ << std::endl;
            if (cbufferf_size(*cb_in_ptr) >= rx_bufersize * 2)
                break;
            std::this_thread::yield();                
        }

        uint32_t num_read;
        cbufferf_read(*cb_in_ptr, rx_bufersize*2, &recv_buf_ptr, &num_read);
        if (num_read < rx_bufersize*2) {
            printf("Error!");
            exit(0);
        }

        if (chrono::high_resolution_clock::now() - t2 > chrono::milliseconds(100))
        {
            t2 = chrono::high_resolution_clock::now();


#ifdef USE_GNU_PLOT
            //Plot samples
            gp.write("plot '-' with points title 'ch 0'");            
            for (int i = 1; i < chCount; ++i)
                gp.write(", '-' with points title 'ch 1'");
            gp.write("\n");
            for (int i = 0; i < chCount; ++i)
            {
                for (uint32_t j = 0; j < 1024 ; ++j) {
                    gp.writef("%f %f\n", recv_buf_ptr[2 * j] , recv_buf_ptr[2 * j + 1] );                    
#ifdef DUMP_DAT                    
                    printf("test i %d %f %f\n",j,(float)recv_buf_ptr[2 * j] ,(float)recv_buf_ptr[2 * j + 1] );
#endif                    
                }
                gp.write("e\n");
                gp.flush();
            }
#endif
        }
        cbufferf_release(*cb_in_ptr,num_read);
    }
#ifdef USE_GNU_PLOT    
    gp.write("exit\n");
    gp.flush();
#endif

    std::cout << "RX_dat_thread done\n";

}

