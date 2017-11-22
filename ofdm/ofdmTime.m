clear;
clc;
clf;

pkg load control
pkg load signal
pkg load communications

Fs = 8e8;
Ts = 1/Fs;

Fsym = 1e8;
Tsym = 1/Fsym;

osr = Fs/Fsym;
Nsb = 4;
txBits = randint(Nsb,1);
txNRZs = 2*txBits - 1;

txNRZs_up = repmat(txNRZs,1,osr);

N = 128;
t = 0:Ts:(N-1)*Ts;
sub(1,:) = [txNRZs_up(1,:) zeros(1,length(t)-osr)];
sub(2,:) = [txNRZs_up(2,:) zeros(1,length(t)-osr)];
sub(3,:) = [txNRZs_up(3,:) zeros(1,length(t)-osr)];
sub(4,:) = [txNRZs_up(4,:) zeros(1,length(t)-osr)];

%modOut(1,:) = sub(1,:) .* exp(j*2*pi*1*Fsym*t);
%modOut(2,:) = sub(2,:) .* exp(j*2*pi*2*Fsym*t);
%modOut(3,:) = sub(3,:) .* exp(j*2*pi*3*Fsym*t);

for i = 1:Nsb
    modOut(i,:) = sub(i,:) .* exp(j*2*pi*i*Fsym*t);
end        

txOut = sum(modOut(1:Nsb,:));
rxIn = txOut;

demodOut(1,:) = rxIn .* exp(-j*2*pi*1*Fsym*t);
rxBits(1,:) = sum(demodOut(1,:))/osr;
demodOut(2,:) = rxIn .* exp(-j*2*pi*2*Fsym*t);
rxBits(2,:) = sum(demodOut(2,:))/osr;
demodOut(3,:) = rxIn .* exp(-j*2*pi*3*Fsym*t);
rxBits(3,:) = sum(demodOut(3,:))/osr;
demodOut(4,:) = rxIn .* exp(-j*2*pi*4*Fsym*t);
rxBits(4,:) = sum(demodOut(4,:))/osr;
