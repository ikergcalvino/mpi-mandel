/*The Mandelbrot set is a fractal that is defined as the set of points c
in the complex plane for which the sequence z_{n+1} = z_n^2 + c
with z_0 = 0 does not tend to infinity.*/

/*This code computes an image of the Mandelbrot set.*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <mpi.h>

#define DEBUG 1
#define STATS 1

#define          X_RESN  1024  /* x resolution */
#define          Y_RESN  1024  /* y resolution */

/* Boundaries of the mandelbrot set */
#define           X_MIN  -2.0
#define           X_MAX   2.0
#define           Y_MIN  -2.0
#define           Y_MAX   2.0

/* Incrementos de X e Y */
#define PASO_X ((X_MAX - X_MIN)/X_RESN)
#define PASO_Y ((Y_MAX - Y_MIN)/Y_RESN)

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

int mandelbrot(int i, int j, int *flops)
{
  int     k;
  Compl   z, c;
  float   lengthsq, temp;

  z.real = z.imag = 0.0;
  c.real = X_MIN + j * PASO_X;
  c.imag = Y_MAX - i * PASO_Y;
	*flops += 4;
  k = 0;

  do
  {    /* iterate for pixel color */
    temp = z.real*z.real - z.imag*z.imag + c.real;
    z.imag = 2.0*z.real*z.imag + c.imag;
    z.real = temp;
    lengthsq = z.real*z.real+z.imag*z.imag;
    k++;
    *flops += 10;
  } while (lengthsq < 4.0 && k < maxIterations);

  return k;
}

int main (int argc, char *argv[])
{

  int flops = 0;
  int total_flops = 0;

  /* MPI variables */
  int numprocs, rank;

  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  /* Mandelbrot variables */
  int i, j, k;
	int res_parcial[(Y_RESN/numprocs) + 1][X_RESN];
  int *vres, *res[Y_RESN];

  int flopsxproc[numprocs];

  int rows[numprocs];
  int startrow[numprocs];
	int count[numprocs];
	int displ[numprocs];

  for(i = 0; i < numprocs; i++) {
		if( i < Y_RESN%numprocs ) {
			rows[i] = Y_RESN/numprocs + 1;
			startrow[i] = i*rows[i];
		} else {
			rows[i] = Y_RESN/numprocs;
			startrow[i] = startrow[i-1] + rows[i-1];
		}
		count[i] = rows[i] * X_RESN;
		displ[i] = startrow[i] * X_RESN;
	}

  /* Timestamp variables */
  struct timeval  ti, tf;

  if (rank == 0)
  {
    /* Allocate result matrix of Y_RESN x X_RESN */
    vres = (int *) malloc(Y_RESN * X_RESN * sizeof(int));
    if (!vres)
    {
      fprintf(stderr, "Error allocating memory\n");
      return 1;
    }
    for (i=0; i<Y_RESN; i++)
      res[i] = vres + i*X_RESN;
  }

  /* Start measuring time */
  gettimeofday(&ti, NULL);

  /* Calculate and draw points */
  for(i=startrow[rank]; i < startrow[rank]+rows[rank]; i++)
  {
    for(j=0; j < X_RESN; j++)
    {
      k = mandelbrot(i, j, &flops);

      if (k >= maxIterations) res_parcial[i-startrow[rank]][j] = 0;
      else res_parcial[i-startrow[rank]][j] = k;
    }
  }

  /* End measuring time */
  gettimeofday(&tf, NULL);
  fprintf (stderr, "(PERF) Computing Time (seconds) = %lf\n", get_seconds(ti,tf));

	MPI_Barrier(MPI_COMM_WORLD);

  gettimeofday(&ti, NULL);

	MPI_Gatherv(res_parcial, count[rank], MPI_INT, vres, count, displ, MPI_INT, 0, MPI_COMM_WORLD);

  gettimeofday(&tf, NULL);
  fprintf (stderr, "(PERF) Communication Time (seconds) = %lf\n", get_seconds(ti,tf));

	MPI_Gather(&flops, 1, MPI_INT, flopsxproc, 1, MPI_INT, 0, MPI_COMM_WORLD);

  /* Print result out */
  if( DEBUG && rank == 0 ) {
    for(i=0;i<Y_RESN;i++) {
      for(j=0;j<X_RESN;j++)
              printf("%3d ", res[i][j]);
      printf("\n");
    }
  }

  free(vres);

  MPI_Finalize();

  return 0;
}
