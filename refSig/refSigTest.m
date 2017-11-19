clear;
clc;
clf;

useLime = 1;
if useLime == 1
  LoadLimeSuite;
  LimeInitialize();
  LimeLoadConfig("self_test.ini");    
end  



N = 128;

Fc = 800e6;
Fs = 8e6;
Ts = 1/Fs;
osr = 8;


syncword = '+--*';
syncBits = 2*reshape(de2bi(syncword,8,2),1,[]) - 1;
syncBits_up = reshape(repmat(syncBits,osr,1),1,[]);

k = 0:(N-1);
delta_f = 2*pi/N;
n = 1;
phaseIn = 2*pi/N*k*n;

txRefSig_bb = exp(j*phaseIn);
%plot(refSig_bb);

txOfdm_bb = fft(txRefSig_bb)/length(txRefSig_bb);

txPkt_bb = [syncBits txOfdm_bb];
txPkt_bb_up = reshape(repmat(txPkt_bb,osr,1),1,[]);

minPktSize = 16384;

tti = ceil(minPktSize/length(txPkt_bb_up));
txPkt_bb_tti = repmat(txPkt_bb_up,1,tti);

t = 0:Ts:(length(txPkt_bb_tti)-1)*Ts;

if useLime == 1  
  iqDataTx = txPkt_bb_tti;
  iqDataTx_mimo = repmat(iqDataTx,2,1);
  LimeLoopWFMStart(iqDataTx_mimo);
  LimeStartStreaming(length(iqDataTx_mimo),["tx0";"tx1";"rx0";"rx1"]);  
  iqDataRx_left = LimeReceiveSamples(length(iqDataTx_mimo),0);
  iqDataRx_right = LimeReceiveSamples(length(iqDataTx_mimo),1);
  %iqDataRx = iqDataRx_left + iqDataRx_right;
  iqDataRx = iqDataRx_left;
  pause(4);
  LimeLoopWFMStop();
  LimeStopStreaming();
  LimeDestroy();

  rxPkt_bb_tti = iqDataRx;
else	
txPkt_ps = real(txPkt_bb_tti) .* cos(2*pi*Fc*t) - imag(txPkt_bb_tti) .* sin(2*pi*Fc*t);
rxPkt_ps = shift(txPkt_ps,256) + 0.001*noise(1,length(txPkt_ps))';
rxPkt_bb_tti = rxPkt_ps .* cos(2*pi*Fc*t) + j * rxPkt_ps .* sin(2*pi*Fc*t);
end

%search beginb

sync_bb_fft = conj(fft(syncBits));
sync_bb_power = (norm(syncBits))/length(syncBits);
sync_bb_avgeng = (norm(syncBits))*Ts/length(syncBits);
sync_bb_max = max(abs(syncBits));


macfMax = 0;
max_pos = 0;
maxPhase = 0;
maxGain = 0;
maxPowerGain = 0;
maxAvgEng = 0;
for pos = 1:length(txPkt_bb_up)

  rx_sync = zeros(1,length(syncBits));
  for i =  0:osr-1
    rx_sync = rx_sync + rxPkt_bb_tti(pos+i:osr:pos+i+osr*(length(syncBits)-1));
  end
  rx_sync = rx_sync/osr;
  rx_sync(find(abs(rx_sync) < 1e-7)) = 0;

  rx_sync_power = (norm(rx_sync))/length(rx_sync);
  rx_sync_avgeng = (norm(rx_sync))*Ts/length(rx_sync);
  powerGain = sync_bb_power/rx_sync_power;

  rx_sync_max = max(abs(rx_sync));
  gain = sync_bb_max/rx_sync_max;

  rx_sync = rx_sync * gain;

  rx_sync_fft = fft(rx_sync);
  acf = ifft(rx_sync_fft .* sync_bb_fft);
  [macf lacf] = max(abs(acf));
  phaseMax = angle(acf(lacf));
  if (macf > macfMax) && (lacf == 1)
    max_pos = pos;
    macfMax = macf;
    maxPhase = phaseMax;
    maxGain = gain;
    maxPowerGain = powerGain;
    maxAvgEng = rx_sync_avgeng;
    printf("pos %d macf %f lacf %d macfMax %f phaseMax %d  maxGain %f powerGain %f maxAvgEng %f\n",
    				pos, macf, lacf,macfMax,phaseMax,maxGain,maxPowerGain,maxAvgEng);
  end    
end

if (max_pos == 0)
  disp('error');
  pause;
end  

rxPkt_bb_up = rxPkt_bb_tti(max_pos : max_pos + length(txPkt_bb_up)) * maxPowerGain * exp(-j*maxPhase); 

rxPkt_bb = zeros(1,length(txPkt_bb));
for i =  1:osr
    rxPkt_bb = rxPkt_bb + rxPkt_bb_up(i : osr : i + osr*(length(rxPkt_bb)-1));
end

rxPkt_bb = rxPkt_bb/osr;
%plot(real(rxPkt_bb));

rx_sync = rxPkt_bb(1:length(syncBits));


figure(1);plot(real(rx_sync));
figure(2);plot(syncBits,"r");

rxOfdm_bb = rxPkt_bb(length(syncBits)+1 : length(syncBits) + length(txOfdm_bb));
rxRefSig_bb = ifft(rxOfdm_bb) * length(txRefSig_bb);

figure(3);plot(abs(rxRefSig_bb));
figure(4);plot(abs(txRefSig_bb));


%figure(3);plot(real(rxOfdm_bb));
%figure(4);plot(real(txOfdm_bb));

%rxOfdm_bb = ifft(rxPkt_bb(length(syncBits_up)+1 : length(syncBits_up) + 1 + length(txOfdm_bb)));
%plot(rxOfdm_bb);

%rxRefSig_bb = ifft(rxOfdm_bb);max_pos

%figure(1);plot(abs(rxRefSig_bb));
%figure(2);plot(unwrap(angle(rxRefSig_bb)));

%H = rxRefSig_bb .* conj(txRefSig_bb);

%figure(1);plot(abs(H));
%figure(2);plot(unwrap(angle(H)));




