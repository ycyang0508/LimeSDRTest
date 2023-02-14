/**
    @file   dualRXTX.cpp
    @author Lime Microsystems (www.limemicro.com)
    @brief  Dual channel RX/TX example
 */
#include "lime/LimeSuite.h"
#include <iostream>
#include <math.h>
#include <chrono>
#ifdef USE_GNU_PLOT
#include "gnuPlotPipe.h"
#endif

using namespace std;

//Device structure, should be initialize to NULL
lms_device_t* device = NULL;

int error()
{
    if (device != NULL)
        LMS_Close(device);
    exit(-1);
}

int main(int argc, char** argv)
{   
    const double sample_rate = 20e6;    //sample rate to 5 MHz
    const double tone_freq = 0.4e5; //tone frequency
    const double center_freq = 1.2e9;
    const double rx_gain = 0.670;
    const double tx_gain = 0.40;

    const double f_ratio = tone_freq/sample_rate;
    
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

    //calibrate Tx, continue on failure
    LMS_Calibrate(device, LMS_CH_TX, 0, sample_rate, 0);
    LMS_Calibrate(device, LMS_CH_RX, 0, sample_rate, 0);



    //Enable test signals generation in RX channels
    //To receive data from RF, remove these lines or change signal to LMS_TESTSIG_NONE
    if (LMS_SetTestSignal(device, LMS_CH_RX, 0, LMS_TESTSIG_NONE , 0, 0) != 0)
        error();

    LMS_Calibrate(device, LMS_CH_TX, 0, sample_rate, 0);
    LMS_Calibrate(device, LMS_CH_RX, 0, sample_rate, 0);

    //Streaming Setup

    const int chCount = 1; //number of RX/TX streams
    lms_stream_t rx_streams[chCount];
    lms_stream_t tx_streams[chCount];
    //Initialize streams
    //All streams setups should be done before starting streams. New streams cannot be set-up if at least stream is running.
    for (int i = 0; i < chCount; ++i)
    {
        rx_streams[i].channel = i; //channel number
        rx_streams[i].fifoSize = 1024 * 1024; //fifo size in samples
        rx_streams[i].throughputVsLatency = 0.5; //some middle ground
        rx_streams[i].isTx = false; //RX channel
        rx_streams[i].dataFmt = lms_stream_t::LMS_FMT_I12; //12-bit integers
        if (LMS_SetupStream(device, &rx_streams[i]) != 0)
            error();
        tx_streams[i].channel = i; //channel number
        tx_streams[i].fifoSize = 1024 * 1024; //fifo size in samples
        tx_streams[i].throughputVsLatency = 0.5; //some middle ground
        tx_streams[i].isTx = true; //TX channel
        tx_streams[i].dataFmt = lms_stream_t::LMS_FMT_I12; //12-bit integers
        if (LMS_SetupStream(device, &tx_streams[i]) != 0)
            error();
    }

    //Initialize data buffers
    const int bufersize = 1024 * 8; //complex samples per buffer
    int16_t * rx_buffers[chCount];
    int16_t * tx_buffers[chCount];
    for (int i = 0; i < chCount; ++i)
    {
        rx_buffers[i] = new int16_t[bufersize * 2]; //buffer to hold complex values (2*samples))
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
        LMS_StartStream(&rx_streams[i]);
        LMS_StartStream(&tx_streams[i]);
    }

    //Streaming

    lms_stream_meta_t rx_metadata; //Use metadata for additional control over sample receive function behavior
    rx_metadata.flushPartialPacket = false; //currently has no effect in RX
    rx_metadata.waitForTimestamp = false; //currently has no effect in RX

    lms_stream_meta_t tx_metadata; //Use metadata for additional control over sample send function behavior
    tx_metadata.flushPartialPacket = false; //do not force sending of incomplete packet
    tx_metadata.waitForTimestamp = false; //Enable synchronization to HW timestamp

#ifdef USE_GNU_PLOT
    GNUPlotPipe gp;
    gp.write("set size square\n set xrange[-4096:4096]\n set yrange[-4096:4096]\n");
#endif
    auto t1 = chrono::high_resolution_clock::now();
    auto t2 = t1;

    while (chrono::high_resolution_clock::now() - t1 < chrono::seconds(10)) //run for 10 seconds
    {
        for (int i = 0; i < chCount; ++i)
        {
            int samplesRead;
            int samplesWrite = bufersize;

            //Send samples with 1024*256 sample delay from RX (waitForTimestamp is enabled)
            LMS_SendStream(&tx_streams[i], tx_buffers[i], samplesWrite, &tx_metadata, 1000);

            //Receive samples
            samplesRead = LMS_RecvStream(&rx_streams[i], rx_buffers[i], bufersize, &rx_metadata, 1000);
        }

        //Print stats every 1s
        if (chrono::high_resolution_clock::now() - t2 > chrono::milliseconds(1000))
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
            cout << "RX rate: " << status.linkRate / 1e6 << " MB/s\n"; //link data rate (both channels))
            cout << "RX 0 FIFO: " << 100 * status.fifoFilledCount / status.fifoSize << "%" << endl; //percentage of RX 0 fifo filled

            LMS_GetStreamStatus(tx_streams, &status); //Obtain TX stream stats
            cout << "TX rate: " << status.linkRate / 1e6 << " MB/s\n"; //link data rate (both channels))
            cout << "TX 0 FIFO: " << 100 * status.fifoFilledCount / status.fifoSize << "%" << endl; //percentage of TX 0 fifo filled
        }
    }

    //Stop streaming
    for (int i = 0; i < chCount; ++i)
    {
        LMS_StopStream(&rx_streams[i]); //stream is stopped but can be started again with LMS_StartStream()
        LMS_StopStream(&tx_streams[i]);
    }
    for (int i = 0; i < chCount; ++i)
    {
        LMS_DestroyStream(device, &rx_streams[i]); //stream is deallocated and can no longer be used
        LMS_DestroyStream(device, &tx_streams[i]);
        delete[] rx_buffers[i];
        delete[] tx_buffers[i];
    }

    //Close device
    LMS_Close(device);

    return 0;
}

