clear;
clc;
clf;
pkg load signal
pkg load communications

useLime = 1;
if useLime == 1
  LoadLimeSuite;
  LimeInitialize();
  LimeLoadConfig("self_test_ycyang.ini");    
end  


Fs = 8000000;
osr = 8;
Fsym = Fs/osr;

Fc = 800000000;

Ts= 1/Fs;
Tsym = 1/Fsym;

totalSample = 65535*4;
totalSym = totalSample/osr;

t = 0:Ts:(totalSample-1)*Ts;
t_sym = 0:Tsym:(totalSym-1)*Tsym;

syncword = '+-*/';
syncBits = reshape(de2bi(syncword,8,2),1,[]);

txDat = 'hello world';
txDatBits = reshape(de2bi(txDat,8,2),1,[]);
txPkt = [syncBits txDatBits];
pktSize = 128;
txPkt = [txPkt zeros(1,pktSize-length(txPkt))];
numOfPkt = ceil(totalSym/length(txPkt));

txBits = reshape(repmat(txPkt,1,numOfPkt),1,[]);
tx_bb = 2*txBits-1;
tx_up = reshape(repmat(tx_bb,osr,1),1,[]);

tx_up_packet = tx_up(1:pktSize*osr);
% channel

if useLime == 1  
  %tx_up = reshape(repmat(tx_up,2,1),1,[]);
    
  tx_up = repmat(tx_up,2,1);
    %tx_up = [zeros(1,length(tx_up));tx_up];
    
   
  LimeLoopWFMStart(tx_up);
  %LimeTransmitSamples(tx_up,0);
  LimeStartStreaming(length(tx_up),["tx0";"tx1";"rx0";"rx1"]);  
  rx_up_left = LimeReceiveSamples(length(tx_up),0);
  rx_up_right = LimeReceiveSamples(length(tx_up),1);
  %tx_up = tx_up(1:2:end);  
  
  rx_up = rx_up_left + rx_up_right;

else
tx_signal = tx_up .* cos(2*pi*Fc*t);
rx_signal = tx_signal + awgn(tx_signal,30);
shiftSignal = 30;
rx_signal = [rand(1,shiftSignal) tx_signal(1:end-shiftSignal)];
rx_up = rx_signal .* cos(2*pi*Fc*t);
end

if useLime == 1
  pause(4);
  LimeLoopWFMStop();
  LimeStopStreaming();
  LimeDestroy();
  
end  



sync_bb_fft = conj(fft((2*syncBits-1)));

macfMax = 0;
max_pos = 0;
maxPhase = 0;
for pos = 1:pktSize*osr
  
  rx_sync = zeros(1,length(syncBits));
  for i =  0:osr-1
    rx_sync = rx_sync + rx_up(pos+i:osr:pos+i+osr*(length(syncBits)-1));
  end    
  rx_sync = rx_sync / osr;
  %rx_sync  = rx_up(pos:osr:pos+osr*(length(syncBits)-1));
  rx_sync_fft = fft(rx_sync);
  acf = ifft(rx_sync_fft .* sync_bb_fft);
  [macf lacf] = max(abs(acf));
  phaseMax = angle(acf(lacf));
  if (macf > macfMax) && (lacf == 1)
    max_pos = pos;
    macfMax = macf;
    maxPhase = phaseMax;
    printf("%d %f %d %f %d\n",pos, macf, lacf,macfMax,phaseMax);
  end      
end

rx_up = rx_up * exp(-j*maxPhase);
rx_up = real(rx_up);


% paket extract
syncSize = length(syncBits);
datSize = length(txDatBits);

rx_up_packet = rx_up(max_pos:1:max_pos + pktSize * osr-1);
rx_up_sync = rx_up_packet(1:syncSize*osr);

rx_bb_sync = 0;
for i=1:osr
    rx_bb_sync += rx_up_sync(i:osr:i+(syncSize-1)*osr);
end        
rx_bb_sync = (rx_bb_sync/osr) > 0;

rx_up_dat = rx_up_packet(syncSize*osr+1:end);

rx_bb_dat = 0;
for i=1:osr
    rx_bb_dat += rx_up_dat(i:osr:i+(datSize-1)*osr);
end        
rx_bb_dat = (rx_bb_dat/osr) > 0;
finalOut = char(bi2de(reshape(rx_bb_dat,[],8),2))';
disp(finalOut);
%subplot(2,1,1);plot(txPkt,"b");
%subplot(2,1,2);plot([rx_bb_sync rx_bb_dat],"r")


%subplot(2,1,1);plot(tx_up,"b");
%subplot(2,1,2);plot(rx_up,"r")
figure(1);
subplot(2,1,1);plot(tx_up_packet,"b");
subplot(2,1,2);plot(rx_up_packet,"r");
figure(2);
subplot(2,1,1);plot(real(rx_up_left * exp(-j*maxPhase))); 
subplot(2,1,2);plot(real(rx_up_right * exp(-j*maxPhase)));




