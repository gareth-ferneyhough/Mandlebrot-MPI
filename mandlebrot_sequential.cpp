#define BOOST_CHRONO_HEADER_ONLY

#include <iostream>
#include <boost/chrono.hpp>
#include <png++/png.hpp>

using std::cout;
using std::endl;

struct complex{
  double real;
  double imag;
};

// Function prototypes
void generateMandlebrotImage(png::image< png::index_pixel > *image);
int cal_pixel(complex c);
inline void setPixel(int x, int y, int color, png::image< png::index_pixel > *image);

// Global constants
int DISP_HEIGHT = 4000;
int DISP_WIDTH = 6000;

int REAL_MAX = 1;
int REAL_MIN = -2;
int IMAG_MAX = 1;
int IMAG_MIN = -1;

int main()
{
  // Initialize png image and create palette
  png::image< png::index_pixel > image(DISP_WIDTH, DISP_HEIGHT);
  png::palette pal(256);
  for (size_t i = 0; i < pal.size(); ++i){
    pal[i] = png::color(i, i*2.2, i*4.4);
  }
  image.set_palette(pal);

  // Start timer
  boost::chrono::system_clock::time_point start = boost::chrono::system_clock::now();

  generateMandlebrotImage(&image);

  // End timer
  boost::chrono::duration<double> sec = boost::chrono::system_clock::now() - start;
  std::cout << "took " << sec.count() << " seconds\n";

  image.write("mandlebrot.png");

  return 0;
}

void generateMandlebrotImage(png::image< png::index_pixel > *image)
{
  double scale_real = double(REAL_MAX - REAL_MIN) / DISP_WIDTH;
  double scale_imag = double(IMAG_MAX - IMAG_MIN) / DISP_HEIGHT;

  for (int x = 0; x < DISP_WIDTH; x++){
    for (int y = 0; y < DISP_HEIGHT; y++){

      complex c;
      c.real = REAL_MIN + ((double) x * scale_real);
      c.imag = IMAG_MIN + ((double) y * scale_imag);

      int color = cal_pixel(c);
      setPixel(x, y, color, image);
    }
  }
}

int cal_pixel(complex c)
{
  int max_iter = 256;
  int count = 0;

  complex z;
  z.real = 0;
  z.imag = 0;

  double temp, lengthsq;

  do {
    temp = z.real * z.real - z.imag * z.imag + c.real;
    z.imag = 2 * z.real * z.imag + c.imag;
    z.real = temp;
    lengthsq = z.real * z.real + z.imag * z.imag;
    count ++;
  }
  while ((lengthsq < 4.0) && (count < max_iter));

  return count;
}

inline void setPixel(int x, int y, int color, png::image< png::index_pixel > *image)
{
  (*image)[y][x] = png::index_pixel(color);
}

