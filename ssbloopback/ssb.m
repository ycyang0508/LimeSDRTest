clear;
clc;
clf;
pkg load signal
pkg load communications

useLime = 0;
if useLime == 1
  LoadLimeSuite;
  LimeInitialize();
  LimeLoadConfig("self_test_ycyang.ini");    
end  

Fc = 800e6;

tx_Fs = 8e6;
tx_Ts = 1/tx_Fs;
rx_Fs = 1;
rx_Ts = 1/rx_Fs;

Fssb = 0.25 * tx_Fs;

TxPts = 8192;
RxPts = 256;

tx_t = 0:tx_Ts:(TxPts-1)*tx_Ts;
rx_t = 0:rx_Ts:(RxPts-1)*rx_Ts;

iqDataTx = exp(i*2*pi*Fssb*tx_t);

if useLime == 1  
  iqDataTx_mimo = repmat(iqDataTx,2,1);     
  LimeLoopWFMStart(iqDataTx_mimo);
  %LimeTransmitSamples(tx_up,0);
  LimeStartStreaming(length(iqDataTx_mimo),["tx0";"tx1";"rx0";"rx1"]);  
  iqDataRx_left = LimeReceiveSamples(length(iqDataTx_mimo),0);
  iqDataRx_right = LimeReceiveSamples(length(iqDataTx_mimo),1);
  %tx_up = tx_up(1:2:end);  
  
  iqDataRx = iqDataRx_left + iqDataRx_right;
  
  pause(4);
  LimeLoopWFMStop();
  LimeStopStreaming();
  LimeDestroy();
  
else


txSig = real(iqDataTx) .* cos(2*pi*Fc*tx_t) - imag(iqDataTx) .* sin(2*pi*Fc*tx_t);
rxSig = txSig + 10*noise(1,length(txSig))';
iqDataRx = txSig .* cos(2*pi*Fc*tx_t) + j * txSig .* sin(2*pi*Fc*tx_t);

end

iqDataRx = iqDataRx(1:RxPts);
N = length(iqDataRx);
delta_f = tx_Fs/N;
f = -tx_Fs/2:delta_f:tx_Fs/2-delta_f;
iqDataRx_fft = fftshift(fft(iqDataRx));
%iqDataRx_fft( find(iqDataRx_fft < 1e-8) ) =0;
fft_mag = abs(iqDataRx_fft);
iqDataRx_fft( find(iqDataRx_fft < 1e-8) ) =0;
fft_phase = angle(iqDataRx_fft);
figure(1);plot(f,10*log10(fft_mag+1e-6));
figure(2);plot(f,fft_phase);
