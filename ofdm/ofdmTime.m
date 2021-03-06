clear;
clc;
clf;

pkg load control
pkg load signal
pkg load communications

Fiq = 3e6;
Tiq = 1/Fiq;

Nsb = 16;
Fsym = 15e3;
Tsym = 1/Fsym;
osr_sym = Fiq/Fsym;

txBits = randint(Nsb,1) + j * randint(Nsb,1);
txNRZs = 2*txBits - (1 + j);

txNRZs_up = repmat(txNRZs,1,osr_sym);

t = 0:Tiq:Tsym-Tiq;
for i = 1:Nsb
    sub(i,:) = txNRZs_up(i,:);
end

Fsc_pos = (1:floor(Nsb/2))*Fsym;
Fsc_neg = (floor(-Nsb/2):1:-1)*Fsym;
Fsc = [Fsc_neg Fsc_pos];

for i = 1:Nsb
    modOut(i,:) = sub(i,:) .* exp(j*2*pi*Fsc(i)*t);
end        

txOut = sum(modOut(1:end,:));
rxIn = txOut;

txOut_fft_db = mag2db(abs(fftshift(fft(txOut)/length(txOut))));

for i = 1:Nsb
    demodOut(i,:) = rxIn .* exp(-j*2*pi*Fsc(i)*t);
    rxNRZs(i,:) = sum(demodOut(i,:))/osr_sym;
end
%disp(txNRZs');
%disp(rxNRZs');
figure(1);subplot(2,1,1);plot(real(txNRZs));subplot(2,1,2);plot(imag(txNRZs))
figure(2);subplot(2,1,1);plot(real(rxNRZs));subplot(2,1,2);plot(imag(rxNRZs))
delta_f = Fiq/length(txOut);
f = -Fiq/2:delta_f:Fiq/2-delta_f;
figure(3);plot(f,txOut_fft_db);


f = -Fiq/2:delta_f:Fiq/2-delta_f;
r = spectral_xdf(rxIn);
figure(4);plot(r(:,1)*Fiq-Fiq/2,fftshift(r(:,2)));
