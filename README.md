# Práctica 3:  Descomposición de dominio:  Mandelbrot
## Conjunto de Maldelbrot
- El conjunto de Mandelbrot es un fractal que se define como el
conjunto de puntos c en el plano complejo para los cuales la
secuencia Z<sub>*n*+1</sub> = Z<sub>*n*</sub><sup>2</sup> + c con Z<sub>0</sub> = 0
no tiende a infinito.
- El objetivo de esta práctica es la paralización de un código
que computa una imagen del conjunto de Mandelbrot.

## Código secuencial
Código secuencial: `mandel.c`
```
/*The Mandelbrot set is a fractal that is defined as the set of points c
in the complex plane for which the sequence z_{n+1} = z_n^2 + c
with z_0 = 0 does not tend to infinity.*/

/*This code computes an image of the Mandelbrot set.*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#define DEBUG 1

#define          X_RESN  1024  /* x resolution */
#define          Y_RESN  1024  /* y resolution */

/* Boundaries of the mandelbrot set */
#define           X_MIN  -2.0
#define           X_MAX   2.0
#define           Y_MIN  -2.0
#define           Y_MAX   2.0

/* More iterations -> more detailed image & higher computational cost */
#define   maxIterations  1000

typedef struct complextype
{
  float real, imag;
} Compl;

static inline double get_seconds(struct timeval t_ini, struct timeval t_end)
{
  return (t_end.tv_usec - t_ini.tv_usec) / 1E6 +
         (t_end.tv_sec - t_ini.tv_sec);
}

int main ( )
{

  /* Mandelbrot variables */
  int i, j, k;
  Compl   z, c;
  float   lengthsq, temp;
  int *vres, *res[Y_RESN];

  /* Timestamp variables */
  struct timeval  ti, tf;

  /* Allocate result matrix of Y_RESN x X_RESN */
  vres = (int *) malloc(Y_RESN * X_RESN * sizeof(int));
  if (!vres)
  {
    fprintf(stderr, "Error allocating memory\n");
    return 1;
  }
  for (i=0; i<Y_RESN; i++)
    res[i] = vres + i*X_RESN;

  /* Start measuring time */
  gettimeofday(&ti, NULL);

  /* Calculate and draw points */
  for(i=0; i < Y_RESN; i++)
  {
    for(j=0; j < X_RESN; j++)
    {
      z.real = z.imag = 0.0;
      c.real = X_MIN + j * (X_MAX - X_MIN)/X_RESN;
      c.imag = Y_MAX - i * (Y_MAX - Y_MIN)/Y_RESN;
      k = 0;

      do
      {    /* iterate for pixel color */
        temp = z.real*z.real - z.imag*z.imag + c.real;
        z.imag = 2.0*z.real*z.imag + c.imag;
        z.real = temp;
        lengthsq = z.real*z.real+z.imag*z.imag;
        k++;
      } while (lengthsq < 4.0 && k < maxIterations);

      if (k >= maxIterations) res[i][j] = 0;
      else res[i][j] = k;
    }
  }

  /* End measuring time */
  gettimeofday(&tf, NULL);
  fprintf (stderr, "(PERF) Time (seconds) = %lf\n", get_seconds(ti,tf));

  /* Print result out */
  if( DEBUG ) {
    for(i=0;i<Y_RESN;i++) {
      for(j=0;j<X_RESN;j++)
              printf("%3d ", res[i][j]);
      printf("\n");
    }
  }

  free(vres);

  return 0;
}
```

## Descomposición de dominio
- Dividir las *N* filas de la matriz imagen entre *p* procesos.
- Cada proceso se encargará de calcular *N*/*p* filas consecutivas de la imagen.
- Por simplicidad, considerad inicialmente que *N* mod *p* = 0.

Posteriormente, modificad la práctica para considerar el caso general.

## Paralelización
- Implementación SPMD.
- Cada proceso computa su trozo de imagen.
- El proceso 0 recoge los resultados locales para construir la imagen global
- Recolección del resultado con operaciones colectivas.
- La E/S (printf) la hace el proceso 0.

## Análisis de rendimiento y balanceo de la carga
- Analizar el balanceo de la carga a través del número de
operaciones en punto flotante que realiza cada proceso.
  - Posible métrica: *b<sub>p</sub>* = *flops<sub>total</sub>* / *flops<sub>p</sub> * nprocs*
  
  *b<sub>p</sub>* = 1 ⇒ el proceso *p* ha realizado exactamente 1 / *nprocs* del trabajo.
- Medir, de forma separada, el tiempo dedicado por cada
proceso a la computación y a las comunicaciones.
