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

typedef std::shared_ptr<uint16_t[]> buf_ptr;
typedef ThreadSafeQueue< buf_ptr > thr_safe_q_t;


using namespace std;
extern lms_device_t* device;
extern const int chCount ; //number of RX/TX streams
extern lms_stream_t rx_streams[2];
extern int error();
extern const double sample_rate ;    //sample rate to 5 MHz
extern const double tone_freq ; //tone frequency
extern const double center_freq ;
extern const double rx_gain ;
extern const double tx_gain ;
extern const double f_ratio ;
extern int bufersize ;

int16_t * rx_buffers[2];



void RX_thread() {
    
    //Initialize data buffers    
    for (int i = 0; i < chCount; ++i)
    {
        rx_buffers[i] = new int16_t[bufersize * 2]; //buffer to hold complex values (2*samples))
    }


    lms_stream_meta_t rx_metadata; //Use metadata for additional control over sample receive function behavior
    rx_metadata.flushPartialPacket = false; //currently has no effect in RX
    rx_metadata.waitForTimestamp = false; //currently has no effect in RX

    auto t1 = chrono::high_resolution_clock::now();
    auto t2 = t1;

#ifdef USE_GNU_PLOT
    GNUPlotPipe gp;
    gp.write("set size square\n set xrange[-4096:4096]\n set yrange[-4096:4096]\n");
#endif
    
    while (chrono::high_resolution_clock::now() - t1 < chrono::seconds(10)) //run for 10 seconds
    {
        for (int i = 0; i < chCount; ++i)
        {
            int samplesRead;
            //Receive samples
            samplesRead = LMS_RecvStream(&rx_streams[i], rx_buffers[i], bufersize, &rx_metadata, 1000);
        }

        //Print stats every 1s
        if (chrono::high_resolution_clock::now() - t2 > chrono::milliseconds(10))
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
                for (uint32_t j = 0; j < bufersize ; ++j) {
                    gp.writef("%i %i\n", rx_buffers[i][2 * j] , rx_buffers[i][2 * j + 1] );
#ifdef DUMP_DAT                    
                    printf("test i %d %f %f\n",j,(float)rx_buffers[i][2 * j] /sf,(float)rx_buffers[i][2 * j + 1] /sf);
#endif                    
                }
                gp.write("e\n");
                gp.flush();
            }
#endif
            //Print stats
            lms_stream_status_t status;
            LMS_GetStreamStatus(rx_streams, &status); //Obtain RX stream stats
            //cout << "RX rate: " << status.linkRate / 1e6 << " MB/s\n"; //link data rate (both channels))
            //cout << "RX 0 FIFO: " << 100 * status.fifoFilledCount / status.fifoSize << "%" << endl; //percentage of RX 0 fifo filled
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


}
