clc;
close all;
clear all;

%Señal de entrada
x=randi([-512 511],1,20);
%Respuesta al impulso del filtro
h=[ 3 4  -4 -3 ];

%Tamaño del bloque de datos
Nx=4;
%Tamaño de la respuesa al impulso
Nh=length(h);
%Próxima longitud potencia de 2 que es mayor o igual a Nx+Nh-1
Nfft=2^nextpow2(Nx+Nh-1);

%Cantidad de bloques
Nbloques=floor(length(x)/Nx)

%Respuesta en frecuencia del filtro
H=fft([h zeros(1,Nfft-Nh)]);

%Convolución lineal.
y1=conv(x,h)

%Condiciones iniciales
xi=zeros(1,Nfft-Nx);

%Se segmenta la señal
for k=1:Nbloques
    
    %Señal de entrada ampliada a la derecha con las condiciones iniciales
    xk=[xi x((k-1)*Nx+1:k*Nx)];
    
    
    %Convolución rápida
    y2=round(real(ifft(fft(xk ).*H)));
    
    %Se actualizan las condiciones iniciales.
    xi=xk(Nx+1:Nfft);
    
    
    %Se muestra el bloque de salida
    y2(Nfft-Nx+1:end)   
    
    
end;


