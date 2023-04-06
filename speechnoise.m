clc;
close all;
clear all;

[x,fs]=audioread('message.wav','native');
x=double(x);

figure;
plot(x);


X=fft(x);
figure;
plot(abs(X));

SNR=0;
xn=awgn(x,SNR,'measured');


figure;
plot(xn);


Xn=fft(xn);
figure;
plot(abs(Xn));

soundsc(x,fs);
pause;
soundsc(xn,fs);