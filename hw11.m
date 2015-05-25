clc
clear all
close all

x=dlmread('zaccels.txt');
y=fft(x);
N = length(x);
mag(1)=abs(y(1))/N;
mag(N/2+1) = abs(y(N/2+1))/N;      
mag(2:N/2) = 2*abs(y(2:N/2))/N;
freqs = linspace(0,1,N/2+1);
figure
plot(x);
title('unfiltered');
figure
stem(freqs,mag);
figure
plot(freqs,mag);
title('fft of unfiltered');

P = 5; 
fifo = [0 0 0 0 0]; 
accelsMAF = [];
for i=1:length(x)
    fifo = [fifo(2:end) x(i)];
    accelsMAF(i) = 1/(P)*sum(fifo); 
end

figure
plot(accelsMAF)
title('MAF');


Nmaf=length(accelsMAF);
ymaf=fft(accelsMAF);

magmaf(1)=abs(ymaf(1))/Nmaf;
magmaf(Nmaf/2+1) = abs(ymaf(Nmaf/2+1))/Nmaf;      
magmaf(2:Nmaf/2) = 2*abs(ymaf(2:Nmaf/2))/Nmaf;
freqsmaf = linspace(0,1,Nmaf/2+1);
figure
plot(freqsmaf,magmaf);
title('fft of MAF');



b=fir1(10,0.1)
figure
freqz(b)

accelsFIR=conv(b,x);
figure
plot(accelsFIR);
title('FIR filter');



Nfir=length(accelsFIR);
yfir=fft(accelsFIR);

magfir(1)=abs(yfir(1))/Nfir;
magfir(Nfir/2+1) = abs(yfir(Nfir/2+1))/Nfir;      
magfir(2:Nfir/2) = 2*abs(yfir(2:Nfir/2))/Nfir;
freqsfir = linspace(0,1,Nfir/2+1);
figure
plot(freqsfir,magfir);
title('fft of FIR');


