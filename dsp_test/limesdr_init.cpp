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
#include "threadSafeQueue.h"

typedef std::shared_ptr<uint16_t[]> buf_ptr;
typedef ThreadSafeQueue< buf_ptr > thr_safe_q_t;

//Device structure, should be initialize to NULL
lms_device_t* device = NULL;
int chCount = 1; //number of RX/TX streams

double sample_rate = 20e6;    //sample rate to 5 MHz
double center_freq = 1.2e9;
double rx_gain = 0.664;
double tx_gain = 0.40;

using namespace std;

extern void TX_thread();
extern void RX_thread();

int error()
{
    printf("Error\n");
    if (device != NULL)
        LMS_Close(device);
    exit(-1);
}

int limesdr_init()
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
        
    return 0;
}

void limesdr_close() {

    LMS_Close(device);

}
