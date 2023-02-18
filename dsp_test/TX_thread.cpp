#include "lime/LimeSuite.h"
#include <iostream>
#include <math.h>
#include <chrono>
#include <thread>
#include <mutex> 
#include <atomic>
#include <cstdint>
#include <memory>
#include <unistd.h>

#ifdef USE_GNU_PLOT
#include "gnuPlotPipe.h"
#endif
#include "threadSafeQueue.h"
#include "liquid.h"

typedef float rf_tx_dat_t;
typedef std::shared_ptr<int16_t[]> buf_ptr;
typedef ThreadSafeQueue< buf_ptr > thr_safe_q_t;

extern lms_device_t* device;
extern const int chCount;
extern const double sample_rate ;    //sample rate to 5 MHz
extern int error();
extern std::atomic<bool> g_exit_flag;

double tone_freq = 0.4e5; //tone frequency
double f_ratio = tone_freq/sample_rate;
int tx_bufersize = 1024 * 64; //complex samples per buffer
lms_stream_t tx_streams[2];

using namespace std;

void TX_dat_thread(cbufferf *cb_in_ptr,std::chrono::seconds op_time) {

    rf_tx_dat_t * tx_buffers_tmp[2];
    
    for (int i = 0; i < chCount; ++i)
    {
        tx_buffers_tmp[i] = new rf_tx_dat_t[tx_bufersize * 2]; //buffer to hold complex values (2*samples))
    }
    cout << "Tx tone frequency: " << tone_freq/1e6 << " MHz" << endl;

    auto t1 = chrono::high_resolution_clock::now();
    auto t2 = t1;
    auto testCnt = 0u;
    //while (chrono::high_resolution_clock::now() - t1 < chrono::seconds(op_time)) //run for 10 seconds
    while (!g_exit_flag.load())
    {
        auto A = 0.90 ;
        for (int i = 0; i <tx_bufersize; i++) {      //generate TX tone
            const double pi = acos(-1);
            double w = 2*pi*i*f_ratio;
            tx_buffers_tmp[0][2*i] = A*cos(w);
            tx_buffers_tmp[0][2*i+1] = A*sin(w);
#ifdef DUMP_DAT
            printf("ref %d %f %f\n",i,(float)tx_buffers[0][2 * i] ,(float)tx_buffers[0][2 * i + 1] );
#endif        
        } 
        cbufferf_write(*cb_in_ptr, tx_buffers_tmp[0], tx_bufersize * 2);        
        
        while(1) {            
            if (cbufferf_size(*cb_in_ptr) > (cbufferf_max_size(*cb_in_ptr)*0.7))
                std::this_thread::sleep_for(  std::chrono::milliseconds( 100 )  );            
            break;            
        }
        testCnt++;
    }

    for (int i = 0; i < chCount; ++i)
    {
        delete [] tx_buffers_tmp[i];
    }
    std::cout << "TX_dat_thread done\n";
}

void do_init_tx()
{
    for (int i = 0; i < chCount; ++i)
    {
        tx_streams[i].channel = i; //channel number
        tx_streams[i].fifoSize = 1024 * 1024 * 16; //fifo size in samples
        tx_streams[i].throughputVsLatency = 0.5; //some middle ground
        tx_streams[i].isTx = true; //TX channel
        tx_streams[i].dataFmt = lms_stream_t::LMS_FMT_F32; //12-bit integers
        if (LMS_SetupStream(device, &tx_streams[i]) != 0)
            error();
    }    
    //Start streaming
    for (int i = 0; i < chCount; ++i)
    {
        LMS_StartStream(&tx_streams[i]);
    }    
    printf("Tx init done \n");
}

void TX_thread(cbufferf *cb_in_ptr,std::chrono::seconds op_time) {
   
    do_init_tx();

    lms_stream_meta_t tx_metadata; //Use metadata for additional control over sample send function behavior
    tx_metadata.flushPartialPacket = false; //do not force sending of incomplete packet
    tx_metadata.waitForTimestamp = false; //Enable synchronization to HW timestamp

    auto t1 = chrono::high_resolution_clock::now();
    auto t2 = t1;
    float *r;
    int ii = 0;
    auto testCnt = 0;
    auto read_cnt = 0;
    //while (chrono::high_resolution_clock::now() - t1 < chrono::seconds(op_time)) //run for 10 seconds
    while (!g_exit_flag.load())
    {
        while(1) {            
            //std::cout << "wait dat " << cbufferf_size(*cb_in_ptr) << " " << testCnt++ << std::endl;
            if (cbufferf_size(*cb_in_ptr) >= tx_bufersize * 2)
                break;
            std::this_thread::yield();                
        }
        
        uint32_t num_read;
        cbufferf_read(*cb_in_ptr, tx_bufersize*2, &r, &num_read);
        if (num_read < tx_bufersize*2) {
            printf("Error!");
            exit(0);
        }
        
        for (int i = 0; i < chCount; ++i)
        {
            int samplesWrite = tx_bufersize * 2;
            //Send samples with 1024*256 sample delay from RX (waitForTimestamp is enabled)            
            auto tx_rlt = LMS_SendStream(&tx_streams[i], r, samplesWrite, &tx_metadata, 1000);            
            if (tx_rlt > 0) {
            }
            else {
                std::cout << "tx Fail : " << tx_rlt << std::endl;
                exit(0);
            }
            cbufferf_release(*cb_in_ptr,samplesWrite);
            //std::cout << "send dat " << samplesWrite << " " << ii++ << std::endl;
        }
        //Print stats every 1s
        if (chrono::high_resolution_clock::now() - t2 > chrono::milliseconds(100))
        {
            lms_stream_status_t status;
            t2 = chrono::high_resolution_clock::now();
            LMS_GetStreamStatus(tx_streams, &status); //Obtain TX stream stats
            //cout << "TX rate: " << status.linkRate / 1e6 << " MB/s\n"; //link data rate (both channels))
            //cout << "TX 0 FIFO: " << 100 * status.fifoFilledCount / status.fifoSize << "% "  \
                                    <<  status.overrun << " " << status.overrun  \
                                    <<  " drop " << status.droppedPackets \
                                    <<  " timestamp " << status.timestamp \
                                    << endl; //percentage of TX 0 fifo filled
        }
    }

    //Stop streaming
    for (int i = 0; i < chCount; ++i)
    {
        LMS_StopStream(&tx_streams[i]);
    }
    std::cout << "TX_thread done\n";
}
