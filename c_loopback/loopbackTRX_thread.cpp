/**
    @file   dualRXTX.cpp
    @author Lime Microsystems (www.limemicro.com)
    @brief  Dual channel RX/TX example
 */
#include "lime/LimeSuite.h"
#include <iostream>
#include <math.h>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>
#include <cstdint>
#include <memory>

#ifdef USE_GNU_PLOT
#include "gnuPlotPipe.h"
#endif
#include "threadSafeQueue.h"

typedef std::shared_ptr<uint16_t[]> buf_ptr;
typedef ThreadSafeQueue< buf_ptr > thr_safe_q_t;

using namespace std;

//Device structure, should be initialize to NULL
lms_device_t* device = NULL;
int chCount = 1; //number of RX/TX streams

double sample_rate = 20e6;    //sample rate to 5 MHz
double tone_freq = 0.4e5; //tone frequency
double center_freq = 1.2e9;
double rx_gain = 0.661;
double tx_gain = 0.40;
double f_ratio = tone_freq/sample_rate;
int bufersize = 1024 * 64; //complex samples per buffer

lms_stream_t tx_streams[2];
lms_stream_t rx_streams[2];


extern void TX_thread();
extern void RX_thread();


int error()
{
    printf("Error\n");
    if (device != NULL)
        LMS_Close(device);
    exit(-1);
}

void do_init_tx()
{
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
    
    //Start streaming
    for (int i = 0; i < chCount; ++i)
    {
        LMS_StartStream(&tx_streams[i]);
    }
    
    printf("Tx init done \n");
}

void do_init_rx()
{
    for (int i = 0; i < chCount; ++i)
    {
        rx_streams[i].channel = i; //channel number
        rx_streams[i].fifoSize = 1024 * 1024 * 4; //fifo size in samples
        rx_streams[i].throughputVsLatency = 0.5; //some middle ground
        rx_streams[i].isTx = false; //RX channel
        rx_streams[i].dataFmt = lms_stream_t::LMS_FMT_I12; //12-bit integers
        if (LMS_SetupStream(device, &rx_streams[i]) != 0)
            error();
    }

    //Start streaming
    for (int i = 0; i < chCount; ++i)
    {        
        LMS_StartStream(&rx_streams[i]);
    }
}

int main(int argc, char** argv)
{   
    //Find devices
    int n;
    lms_info_str_t list[8]; //should be large enough to hold all detected devices
    if ((n = LMS_GetDeviceList(list)) < 0) //NULL can be passed to only get number of devices
        error();

    cout << "Devices found: " << n << endl; //print number of devices
    if (n < 1)
        return -1;

    //open the first device
    if (LMS_Open(&device, list[0], NULL))
        error();

    //Initialize device with default configuration
    //Do not use if you want to keep existing configuration
    //Use LMS_LoadConfig(device, "/path/to/file.ini") to load config from INI
    if (LMS_Init(device) != 0)
        error();

    //Get number of channels
    if ((n = LMS_GetNumChannels(device, LMS_CH_RX)) < 0)
        error();
    cout << "Number of RX channels: " << n << endl;
    if ((n = LMS_GetNumChannels(device, LMS_CH_TX)) < 0)
        error();
    cout << "Number of TX channels: " << n << endl;

    //Enable RX channel
    //Channels are numbered starting at 0
    if (LMS_EnableChannel(device, LMS_CH_RX, 0, true) != 0)
        error();
    //Enable TX channels
    if (LMS_EnableChannel(device, LMS_CH_TX, 0, true) != 0)
        error();

    //Set RX center frequency to 1 GHz
    if (LMS_SetLOFrequency(device, LMS_CH_RX, 0, center_freq) != 0)
        error();
    //Set TX center frequency to 1.2 GHz
    //Automatically selects antenna port
    if (LMS_SetLOFrequency(device, LMS_CH_TX, 0, center_freq) != 0)
        error();

    //Set sample rate to 10 MHz, preferred oversampling in RF 4x
    //This set sampling rate for all channels
    if (LMS_SetSampleRate(device, sample_rate, 4) != 0)
        error();



    //Set RX gain
    if (LMS_SetNormalizedGain(device, LMS_CH_RX, 0, rx_gain) != 0)
        error();
    //Set TX gain
    if (LMS_SetNormalizedGain(device, LMS_CH_TX, 0, tx_gain) != 0)
        error();

    if (LMS_SetLPFBW(device, LMS_CH_RX, 0, sample_rate) != 0)
        error();


    //calibrate Tx, continue on failure
    //if (LMS_Calibrate(device, LMS_CH_TX, 0, sample_rate, 0) != 0)
    //    error();

    if (LMS_Calibrate(device, LMS_CH_RX, 0, sample_rate, 0) != 0)
        error();

    //Enable test signals generation in RX channels
    //To receive data from RF, remove these lines or change signal to LMS_TESTSIG_NONE
    if (LMS_SetTestSignal(device, LMS_CH_RX, 0, LMS_TESTSIG_NONE , 0, 0) != 0)
        error();

    do_init_tx();
    do_init_rx();    

    std::thread tx_t(TX_thread);
    std::thread rx_t(RX_thread);

    tx_t.join();
    rx_t.join();

    printf("done\n");

    //Close device
    LMS_Close(device);

    return 0;
}

