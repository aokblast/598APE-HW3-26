#include <errno.h>
#include <inttypes.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

static void die(const char *s) {
  fprintf(stderr, "%s\n", s);
  exit(1);
}

int N = 801;
int **grid;
static int *low;
static int *high;

// you can also use this function to debug print your sandpile
static void render(char *filename) {
  int chan = 3;
  unsigned char *pixels = (unsigned char *)malloc(N * N * chan * sizeof(char));
  int stride = N * chan;
  if (!pixels)
    die("malloc failed for PNG pixel buffer");
  for (int i = 0; i < N; ++i) {
    for (int j = 0; j < N; ++j) {
      unsigned char r = 0, g = 0, b = 0;
      switch (grid[i][j]) {
      case 0: // obsidian
        r = 15;
        g = 15;
        b = 25;
        break;
      case 1: // electric cyan
        r = 0;
        g = 210;
        b = 255;
        break;
      case 2: // vibrant magenta
        r = 230;
        g = 30;
        b = 130;
        break;
      case 3: // bright gold
        r = 255;
        g = 200;
        b = 0;
        break;
      default: // white
        r = 0;
        g = 0;
        b = 0;
      }

      int pid = (i * N + j) * chan;
      pixels[pid] = r;
      pixels[pid + 1] = g;
      pixels[pid + 2] = b;
    }
  }
  if (stbi_write_png(filename, N, N, chan, pixels, stride)) {
    printf("Successfully saved sandpile identity to: %s\n", filename);
  } else {
    die("Error: stb_image_write failed to save png file\n");
  }
  free(pixels);
}

static void stabilize() {
  int changed = 0;
  int *restrict rv = calloc(1, (N + 2) * (N + 2) * sizeof(int));
  do {
    changed = 0;
#pragma omp parallel for reduction(| : changed)
    for (int i = 0; i < N; ++i) {
      int l = low[i], r = high[i];
      for (int j = l; j <= r; ++j) {
        int gv = grid[i][j];
        int v = gv / 4;
        rv[(i + 1) * (N + 2) + j + 1] = v;
        changed |= v; // topple it
      }
    }
#pragma omp parallel for
    for (int i = 0; i < N; ++i) {
      int l = low[i], r = high[i];
      if (i > 0) {
        l = MIN(l, low[i - 1]);
        r = MAX(r, high[i - 1]);
      }
      if (i < N - 1) {
        l = MIN(l, low[i + 1]);
        r = MAX(r, high[i + 1]);
      }
      l = MAX(l - 1, 0);
      r = MIN(r + 1, N - 1);
      int *restrict gr = grid[i];
      const int *restrict up = &rv[(i) * (N + 2) + 1];
      const int *restrict mid = &rv[(i + 1) * (N + 2) + 1];
      const int *restrict down = &rv[(i + 2) * (N + 2) + 1];
      for (int j = l; j <= r; ++j) {
        gr[j] += up[j] + down[j] + mid[j - 1] + mid[j + 1] - 4 * mid[j];
      }
    }
  } while (changed);
  free(rv);
}

static void init_bounds() {
  low = malloc(N * sizeof(int));
  high = malloc(N * sizeof(int));
  for (int i = 0; i < N; ++i) {
    low[i] = N;
    high[i] = -1;
    for (int j = 0; j <= MAX(i, N - 1 - i); ++j) {
      if ((i - N / 2) * (i - N / 2) + (j - N / 2) * (j - N / 2) <=
          (N / 2) * (N / 2)) {
        low[i] = MIN(low[i], j);
        high[i] = j;
      }
    }
  }
}

// d = b - Ax
static void cg(const double *b) {
  int NP = N + 2;
  size_t M = NP * NP;
  double *x = calloc(M, sizeof(double));
  double *r = calloc(M, sizeof(double));
  double *d = calloc(M, sizeof(double));
  double *adk = calloc(M, sizeof(double));
#define FOR_EACH(k)                                                            \
  for (int i = 0; i < N; ++i)                                                  \
    for (size_t k = (size_t)(i + 1) * NP + low[i] + 1;                         \
         k <= (size_t)(i + 1) * NP + high[i] + 1; ++k)
  double rr = 0;
  // init: d0 = b
  FOR_EACH(k) {
    r[k] = d[k] = b[k];
    rr += b[k] * b[k];
  }
  double tol = 1e-22 * rr;
  while (rr > tol) {
    double dadk = 0;
    FOR_EACH(k) {
      adk[k] = 4 * d[k] - d[k - 1] - d[k + 1] - d[k - NP] - d[k + NP];
      dadk += d[k] * adk[k];
    }
    double alpha = rr / dadk, rn = 0;
    FOR_EACH(k) {
      x[k] += alpha * d[k];
      r[k] -= alpha * adk[k];
      rn += r[k] * r[k];
    }
    double beta = rn / rr;
    rr = rn;
    FOR_EACH(k) d[k] = r[k] + beta * d[k];
  }
#undef FOR_EACH
  for (int i = 0; i < N; ++i)
    for (int j = low[i]; j <= high[i]; ++j) {
      int t = (int)floor(x[(size_t)(i + 1) * NP + j + 1]) - 1;
      if (t <= 0)
        continue;
      grid[i][j] -= 4 * t;
      grid[i - 1][j] += t;
      grid[i][j - 1] += t;
      grid[i + 1][j] += t;
      grid[i][j + 1] += t;
    }
  free(x);
  free(r);
  free(d);
  free(adk);
}

static void find_identity() {
  for (int i = 0; i < N; ++i) {
    for (int j = 0; j < N; ++j) {
      grid[i][j] = 6;
    }
  }
  init_bounds();
  size_t M = (size_t)(N + 2) * (N + 2);
  double *b = calloc(M, sizeof(double));
  for (int i = 0; i < N; ++i)
    for (int j = low[i]; j <= high[i]; ++j)
      b[(size_t)(i + 1) * (N + 2) + j + 1] = 3;
  cg(b);
  stabilize();
  for (int i = 0; i < N; ++i) {
    for (int j = 0; j < N; ++j) {
      grid[i][j] = 6 - grid[i][j];
    }
  }
  for (int i = 0; i < N; ++i)
    for (int j = low[i]; j <= high[i]; ++j)
      b[(size_t)(i + 1) * (N + 2) + j + 1] = grid[i][j] - 3;
  cg(b);
  free(b);
  stabilize();
}

int main(int argc, char **argv) {
  if (argc < 2 || argc > 2) {
    fprintf(stderr, "Usage: %s N\n", argv[0]);
    return EXIT_FAILURE;
  }
  char *end;
  uintmax_t parsed = strtoumax(argv[1], &end, 10);
  if (errno || argv[1][0] < '0' || argv[1][0] > '9' || *end || !parsed ||
      parsed > INT32_MAX)
    die("grid size must be a positive int");
  N = (int)parsed;

  int NP = N + 2;
  int *mat = calloc(NP * NP, sizeof(int));
  grid = malloc(NP * sizeof(int *));
  for (int i = 0; i < NP; ++i)
    grid[i] = &mat[NP * i + 1];
  ++grid;
  find_identity();

  render("identity.png");
  free(mat);
  free(grid - 1);
  free(low);
  free(high);
  return EXIT_SUCCESS;
}
