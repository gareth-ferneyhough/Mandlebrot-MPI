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
int disp_height = 4000;
int disp_width = 6000;

int real_max = 1;
int real_min = -2;
int imag_max = 1;
int imag_min = -1;

int main()
{
  // Initialize png image and create palette
  png::image< png::index_pixel > image(disp_width, disp_height);
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
  double scale_real = double(real_max - real_min) / disp_width;
  double scale_imag = double(imag_max - imag_min) / disp_height;

  for (int x = 0; x < disp_width; x++){
    for (int y = 0; y < disp_height; y++){

      complex c;
      c.real = real_min + ((double) x * scale_real);
      c.imag = imag_min + ((double) y * scale_imag);

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

