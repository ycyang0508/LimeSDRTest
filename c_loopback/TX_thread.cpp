#include "lime/LimeSuite.h"
#include <iostream>
#include <math.h>
#include <chrono>
#include <thread>
#include <mutex> 
#include <atomic>
#include <cstdint>
#include <chrono>
#ifdef USE_GNU_PLOT
#include "gnuPlotPipe.h"
#endif

using namespace std;
extern lms_device_t* device;
extern const int chCount;
lms_stream_t tx_streams[2];
extern int error();

extern const double sample_rate ;    //sample rate to 5 MHz
extern const double tone_freq ; //tone frequency
extern const double center_freq ;
extern const double rx_gain ;
extern const double tx_gain ;
extern const double f_ratio ;

extern std::mutex mtx;   
extern std::atomic<int> counter;

void TX_thread() {
    std::unique_lock<std::mutex> lck (mtx,std::defer_lock);
    lck.lock();
    for (int i = 0; i < chCount; ++i)
    {
        tx_streams[i].channel = i; //channel number
        tx_streams[i].fifoSize = 1024 * 1024 * 16; //fifo size in samples
        tx_streams[i].throughputVsLatency = 0.5; //some middle ground
        tx_streams[i].isTx = true; //TX channel
        tx_streams[i].dataFmt = lms_stream_t::LMS_FMT_I12; //12-bit integers
        if (LMS_SetupStream(device, &tx_streams[i]) != 0)
            error();
    }
    const int bufersize = 1024 * 64; //complex samples per buffer
    int16_t * tx_buffers[chCount];
    for (int i = 0; i < chCount; ++i)
    {
        tx_buffers[i] = new int16_t[bufersize * 2]; //buffer to hold complex values (2*samples))
    }
    double sf = 2047;
    double A = 0.90;
    for (int i = 0; i <bufersize; i++) {      //generate TX tone
        const double pi = acos(-1);
        double w = 2*pi*i*f_ratio;
        tx_buffers[0][2*i] = (int16_t)(A*cos(w) * sf);
        tx_buffers[0][2*i+1] = (int16_t)(A*sin(w) * sf);
#ifdef DUMP_DAT
        printf("ref %d %f %f\n",i,(float)tx_buffers[0][2 * i]/sf ,(float)tx_buffers[0][2 * i + 1] / sf);
#endif        
    }   
    cout << "Tx tone frequency: " << tone_freq/1e6 << " MHz" << endl;

    //Start streaming
    for (int i = 0; i < chCount; ++i)
    {
        LMS_StartStream(&tx_streams[i]);
    }
    lms_stream_meta_t tx_metadata; //Use metadata for additional control over sample send function behavior
    tx_metadata.flushPartialPacket = false; //do not force sending of incomplete packet
    tx_metadata.waitForTimestamp = false; //Enable synchronization to HW timestamp

    auto t1 = chrono::high_resolution_clock::now();
    auto t2 = t1;
    printf("Tx1\n");
    
    lck.unlock();
    
    counter--;
    while(counter > 0)
    {
        std::this_thread::yield();
    }
    
    //std::this_thread::yield();

    while (chrono::high_resolution_clock::now() - t1 < chrono::seconds(1000)) //run for 10 seconds
    {
        for (int i = 0; i < chCount; ++i)
        {
            int samplesWrite = bufersize;

            //Send samples with 1024*256 sample delay from RX (waitForTimestamp is enabled)
            auto tx_rlt = LMS_SendStream(&tx_streams[i], tx_buffers[i], samplesWrite, &tx_metadata, 1000);
            if (tx_rlt > 0) {
            }
            else {
                std::cout << "tx Fail : " << tx_rlt << std::endl;
                exit(0);
            }
        }

        //Print stats every 1s
        if (chrono::high_resolution_clock::now() - t2 > chrono::milliseconds(10))
        {
            lms_stream_status_t status;
            t2 = chrono::high_resolution_clock::now();
            LMS_GetStreamStatus(tx_streams, &status); //Obtain TX stream stats
            //cout << "TX rate: " << status.linkRate / 1e6 << " MB/s\n"; //link data rate (both channels))
            cout << "TX 0 FIFO: " << 100 * status.fifoFilledCount / status.fifoSize << "% "  \
                                    <<  status.overrun << " " << status.overrun  \
                                    <<  " drop " << status.droppedPackets \
                                    <<  " timestamp " << status.timestamp \
                                    << endl; //percentage of TX 0 fifo filled
        }
        std::this_thread::yield();
    }

    //Stop streaming
    for (int i = 0; i < chCount; ++i)
    {
        LMS_StopStream(&tx_streams[i]);
    }
    for (int i = 0; i < chCount; ++i)
    {
        LMS_DestroyStream(device, &tx_streams[i]);
        delete[] tx_buffers[i];
    }

}
