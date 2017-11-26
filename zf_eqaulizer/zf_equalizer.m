function [h_eq,err,optDelay] = zf_equalizer(h_c,N,delay)

h_c = h_c(:),';
H = toeplitz([h_c zeros(1,N-1)],[h_c(1) zeros(1,N-1)]);

end
