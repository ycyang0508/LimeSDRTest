clear;
clc;
clf;


useLime = 1;
if useLime == 1
  LoadLimeSuite;
  LimeInitialize();
  LimeLoadConfig("self_test_ycyang.ini");    
end  


fc = 800e6;
Fs = 8e6;
Ts = 1/Fs;
totalSample = 8192;
fftSize = 512;
t = 0:Ts:(totalSample-1)*Ts;

fmid = 2e4;
fm = 0.5*1e4;
bw = Fs/2;


%m = [-2 -1 0 1 2];
%m = [-2 -1 0 1 2];
m = [-100 -50 0 50 100];

rep = fftSize;
tti = ceil(totalSample/(length(m)*rep));

m_rep = repmat(reshape(repmat(m,rep,1),1,[]),1,tti);
m_rep = m_rep(1:length(t));

ph = 2*pi*fmid*t + 2*pi .* m_rep * fm .* t;
%ph =  2*pi .* m_rep * fm .* t;
%ph = 2*pi*fmid*t ;
iqDataTx = exp(j*ph);

if useLime == 1  
  iqDataTx_mimo = repmat(iqDataTx,2,1);     
  LimeLoopWFMStart(iqDataTx_mimo);
  LimeStartStreaming(length(iqDataTx_mimo),["tx0";"tx1";"rx0";"rx1"]);  
  iqDataRx_left = LimeReceiveSamples(length(iqDataTx_mimo),0);
  iqDataRx_right = LimeReceiveSamples(length(iqDataTx_mimo),1);
  iqDataRx = iqDataRx_left + iqDataRx_right;
  pause(4);
  LimeLoopWFMStop();
  LimeStopStreaming();
  LimeDestroy();

else
pbTx = real(iqDataTx) .* cos(2*pi*fc*t) - imag(iqDataTx) .* sin(2*pi*fc*t);
%pbRx = pbTx + noise(1,length(pbTx))';
pbRx = pbTx ;
iqDataRx = pbRx .* cos(2*pi*fc*t) + j * pbRx .* sin(2*pi*fc*t);
end


iqDatRxLen = length(t);
iqDataRx = (iqDataRx(1:iqDatRxLen));

iqDataRx_phase = (angle(iqDataRx));
iqDataRx_freq = diff([0 iqDataRx_phase])/pi*180;
%figure(1);plot(t(1:iqDatRxLen),iqDataRx_freq/max(abs(iqDataRx_freq)));
%hold on;
figure(1);plot(t(1:iqDatRxLen),real(iqDataRx));
hold off;
delta_f = Fs/fftSize;
f = -(fftSize/2):1:(fftSize/2-1);
f = f*delta_f;

iqDataRx_fftIn = reshape(iqDataRx,fftSize,[]);
iqDataRx_fftout = fftshift(fft(iqDataRx_fftIn .* hanning(length(fftSize))'),1);
wtfld = mag2db(abs(iqDataRx_fftout)' + 1e-5);
%figure(2);img = imagesc(f,[],wtfld);
figure(2);img = imagesc(f,[],wtfld);
figure(3);plot(f,wtfld(1,:));
hold on
plot(f,wtfld(2,:),"r");
plot(f,wtfld(3,:),"s");
plot(f,wtfld(4,:),"b");
plot(f,wtfld(5,:),"y");

