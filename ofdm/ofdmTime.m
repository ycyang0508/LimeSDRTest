clear;
clc;
clf;

pkg load control
pkg load signal
pkg load communications

Fs = 32;
Ts = 1/Fs;

Nsb = 16;
Tsym = 1;
Fsym = 1/Tsym;
osr = Fs/Fsym;

txBits = randint(Nsb,1) + j * randint(Nsb,1);
txNRZs = 2*txBits - (1 + j);

txNRZs_up = repmat(txNRZs,1,osr);

t = 0:Ts:Tsym-Ts;
%sub(1,:) = [txNRZs_up(1,:) zeros(1,length(t)-osr)];
%sub(2,:) = [txNRZs_up(2,:) zeros(1,length(t)-osr)];
%sub(3,:) = [txNRZs_up(3,:) zeros(1,length(t)-osr)];
%sub(4,:) = [txNRZs_up(4,:) zeros(1,length(t)-osr)];
for i = 1:Nsb
    sub(i,:) = txNRZs_up(i,:);
end

%modOut(1,:) = sub(1,:) .* exp(j*2*pi*1*Fsym*t);
%modOut(2,:) = sub(2,:) .* exp(j*2*pi*2*Fsym*t);
%modOut(3,:) = sub(3,:) .* exp(j*2*pi*3*Fsym*t);

for i = 1:Nsb
    modOut(i,:) = sub(i,:) .* exp(j*2*pi*i*Fsym*t);
end        

txOut = sum(modOut(1:Nsb,:));
rxIn = txOut;

%demodOut(1,:) = rxIn .* exp(-j*2*pi*1*Fsym*t);
%rxBits(1,:) = sum(demodOut(1,:))/osr;
%demodOut(2,:) = rxIn .* exp(-j*2*pi*2*Fsym*t);
%rxBits(2,:) = sum(demodOut(2,:))/osr;
%demodOut(3,:) = rxIn .* exp(-j*2*pi*3*Fsym*t);
%rxBits(3,:) = sum(demodOut(3,:))/osr;
%demodOut(4,:) = rxIn .* exp(-j*2*pi*4*Fsym*t);
%rxBits(4,:) = sum(demodOut(4,:))/osr;

for i = 1:Nsb
    demodOut(i,:) = rxIn .* exp(-j*2*pi*i*Fsym*t);
    rxNRZs(i,:) = sum(demodOut(i,:))/osr;
end
disp(txNRZs);
disp(rxNRZs);

