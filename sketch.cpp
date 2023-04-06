//Cancelación de ruido usando solapamiento y suma con FFT

#include <string.h>
#include <stdlib.h>
#include <simdsp.h>
#include <math.h>
#include <random>

//Tamaño del bloque de datos
#define Nx 512

//Tamaño de la FFT, doble del tamaño del bloque de datos
#define Nfft (Nx<<1)

//Umbral de potencia de ruido
#define THRESHOLD 10000

//Frecuencia de muestreo
#define FS 22050

//Buffer de salida
short BufferSalida[Nx];


//Buffers para hacer el procesamiento interno de la convolución rápida
double X_r[Nfft];
double X_i[Nfft];
double Y_r[Nfft];
double Y_i[Nfft];
double Yadd[Nx];

//Función para generar ruido Gaussiano
double getNoise(double var){

static std::default_random_engine generator;
static  std::normal_distribution<double> distribution(0,1);

return  distribution(generator)*var;

}


//Función para calcular FFT
void myFFT(short int dir,long n,double *x,double *y)
{
   long m,nn,i,i1,j,k,i2,l,l1,l2;
   double c1,c2,tx,ty,t1,t2,u1,u2,z;

 /* Calculate log2(n) */
   m = 0;
   nn=n;
   while((nn=nn>>1)) 
      m++; 

   /* Do the bit reversal */
   i2 = n >> 1;
   j = 0;
   for (i=0;i<n-1;i++) {
      if (i < j) {
         tx = x[i];
         ty = y[i];
         x[i] = x[j];
         y[i] = y[j];
         x[j] = tx;
         y[j] = ty;
      }
      k = i2;
      while (k <= j) {
         j -= k;
         k >>= 1;
      }
      j += k;
   }

   /* Compute the FFT */
   c1 = -1.0; 
   c2 = 0.0;
   l2 = 1;
   for (l=0;l<m;l++) {
      l1 = l2;
      l2 <<= 1;
      u1 = 1.0; 
      u2 = 0.0;
      for (j=0;j<l1;j++) {
         for (i=j;i<n;i+=l2) {
            i1 = i + l1;
            t1 = u1 * x[i1] - u2 * y[i1];
            t2 = u1 * y[i1] + u2 * x[i1];
            x[i1] = x[i] - t1; 
            y[i1] = y[i] - t2;
            x[i] += t1;
            y[i] += t2;
         }
         z =  u1 * c1 - u2 * c2;
         u2 = u1 * c2 + u2 * c1;
         u1 = z;
      }
      c2 = sqrt((1.0 - c1) / 2.0);
      if (dir == 1) 
         c2 = -c2;
      c1 = sqrt((1.0 + c1) / 2.0);
   }

   /* Scaling for forward transform */
   if (dir == 1) {
      for (i=0;i<n;i++) {
         x[i] /= n;
         y[i] /= n;
      }
   }
   
  return;
}

//Rutina que simulará la ISR que se invoca una vez ha finalizado
//la transferencia por DMA
void dma_callbackfnc(short *audio){
	int n,k;
	float buffer_x;
	
	//Inicia una nueva captura por DMA para garantizar la ejecución en
	//tiempo real. 
	captureBlock( dma_callbackfnc );

	//Completa el bloque de entrada con ceros, fija la parte imaginaria a 0.0
	for(n=0;n<Nx;n++) {
		X_r[n]  = 0.0;
		X_i[n]  = 0.0;
		buffer_x=(double)audio[Nx+n]+getNoise(1000);
	}	
	for(;n<Nfft;n++) {
		X_r[n] = (double)audio[n-Nx]+getNoise(1000);
		X_i[n] = 0.0;
	}
	
	
	//Calcula la FFT	
	myFFT(1,Nfft,X_r,X_i);

	
	
	//Aplica el denoising
	for(k=0;k<Nfft;k++) {
		//Magnitud para cada frecuencia
		double mag = X_r[k] * X_r[k] + X_i[k] * X_i[k];
		
		//Si no es ruido
		if (mag > THRESHOLD) {
			Y_r[k] = X_r[k]; //Parte real
			Y_i[k] = X_i[k]; //Parte imaginaria
		} else {
			//Es ruido, entonces hace cero esa frecuencia
			Y_r[k]   = 0.0; //Parte real
			Y_i[k] = 0.0; //Parte imaginaria
		}
	}

	
	//Calcula la IFFT 
	myFFT(-1,Nfft,Y_r,Y_i);


	
	//Escribe al buffer de salida sumando las condiciones iniciales
	for(n=0;n<Nx;n++) {
		BufferSalida[n] = Y_r[n+Nx];
	}
	
	//Prepara el buffer de adición para el siguiente traslape
	for(n=0;n<Nx;n++) {
		Yadd[n] = Y_r[Nx+n];
	}
	
		
	//Envia el buffer calculado al DAC a través de DMA
	playBlock(BufferSalida);
}


void dsp_setup(){
	int n;

	//Condiciones iniciales en cero	
	for(n=0; n<Nx; n++) {
		Yadd[n] = 0.0;
	}
	
	//Habilita entrada de micrófono
	enableAudio(Nx,FS);



	//Produce un llamado a la captura de datos por DMA. Esta función invoca
	//la función callbackfnc una vez la captura ha finalizado, generando así
	//múltiples capturas de bloques.
	captureBlock(dma_callbackfnc );
}


void dsp_loop(){
	//El programa principal en este caso no hace nada, pues todo el
	//procesamiento se hace vía DMA
}
