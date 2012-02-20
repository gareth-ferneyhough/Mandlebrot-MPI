#define BOOST_CHRONO_HEADER_ONLY

#include <mpi/mpi.h>
#include <iostream>
#include <vector>
#include <boost/chrono.hpp>
#include <png++/png.hpp>

using std::cout;
using std::endl;
using std::cerr;

struct complex{
  double real;
  double imag;
};

// struct pixel{
//   complex coord;
//   int color;
// };

// Function prototypes
void generateMandlebrotImage(png::image< png::index_pixel > *image);
int cal_pixel(complex c);
inline void setPixel(int x, int y, int color, png::image< png::index_pixel > *image);
void runMasterProcess(int world_rank, int world_size);
void runSlaveProcess(int world_rank, int world_size);

// Global constants
int IMAGE_HEIGHT = 400;
int IMAGE_WIDTH = 600;

int REAL_MAX = 1;
int REAL_MIN = -2;
int IMAG_MAX = 1;
int IMAG_MIN = -1;

int ROWS_PER_PROCESS = 10;

int main(int argc, char** argv)
{
  // Initialize the MPI environment
  MPI_Init(&argc, &argv);

  // Find out rank, size
  int world_rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
  int world_size;
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);

  // Run master or slave processes
  if (world_rank == 0)
    runMasterProcess(world_rank, world_size);

  else runSlaveProcess(world_rank, world_size);

  // Clean up
  MPI_Finalize();
  return 0;
}

void runMasterProcess(int world_rank, int world_size)
{
  // Initialize png image and create palette
  png::image< png::index_pixel > image(IMAGE_WIDTH, IMAGE_HEIGHT);
  png::palette pal(256);
  for (size_t i = 0; i < pal.size(); ++i){
    pal[i] = png::color(i, i*2.2, i*4.4); // can change these values to create new palette
  }
  image.set_palette(pal);

  // Start timer
  boost::chrono::system_clock::time_point start = boost::chrono::system_clock::now();

  generateMandlebrotImage(&image); // Compute all the things!

  // End timer
  boost::chrono::duration<double> sec = boost::chrono::system_clock::now() - start;
  std::cout << "took " << sec.count() << " seconds\n";

  image.write("mandlebrot.png");
}

void runSlaveProcess(int world_rank, int world_size)
{
  double scale_real = double(REAL_MAX - REAL_MIN) / IMAGE_WIDTH;
  double scale_imag = double(IMAG_MAX - IMAG_MIN) / IMAGE_HEIGHT;

  // Recive row number
  MPI_Status status;
  int row_num = -1;
  MPI_Recv(&row_num,
           1,
           MPI_INT,
           0,
           0,
           MPI_COMM_WORLD,
           &status);

  // Create structure for results.
  // Should really create MPI_STRUCT but will just
  // use array of doubles for now.
  // Each pixel need three values: x, y, and color.

  std::vector<double> results;

  for(int x = 0; x < IMAGE_WIDTH; ++x){
    for(int y = row_num; y < row_num + ROWS_PER_PROCESS; ++y){
      complex c;
      c.real = REAL_MIN + ((double) x * scale_real);
      c.imag = IMAG_MIN + ((double) y * scale_imag);
      int color = cal_pixel(c);

      // Put in results array
      results.push_back(x);
      results.push_back(y);
      results.push_back(static_cast<double> (color));
    }
  }

  // Send results to master
  MPI_Send(&(results[0]),
           results.size(),
           MPI_DOUBLE,
           0,
           0,
           MPI_COMM_WORLD);
}

void generateMandlebrotImage(png::image< png::index_pixel > *image)
{
  // Ensure rows are divisible by ROWS_PER_PROCESS
  assert( IMAGE_HEIGHT % ROWS_PER_PROCESS == 0);

  double scale_real = double(REAL_MAX - REAL_MIN) / IMAGE_WIDTH;
  double scale_imag = double(IMAG_MAX - IMAG_MIN) / IMAGE_HEIGHT;

  int slave_process_id = 1;
  for (int row = 0; row < IMAGE_HEIGHT; row += ROWS_PER_PROCESS){

    // Send row numbers to slaves
    int return_val = MPI_Send(&row,
                              1,
                              MPI_INT,
                              slave_process_id,
                              0,
                              MPI_COMM_WORLD);
    // Check for send errors
    if (return_val != MPI_SUCCESS){
      int str_len;
      char* err_str;
      MPI_Error_string(return_val, err_str, &str_len);
      cerr << "Error sending to slaves: " << err_str << endl;
    }

    slave_process_id ++;
  }

  // Revieve sub-area back from each process
  int sub_area_size = ROWS_PER_PROCESS * IMAGE_WIDTH * 3; // 3 values for each pixel (x, y, color)
  double *sub_area = new double[sub_area_size];

  for (int i = 0; i < IMAGE_HEIGHT / ROWS_PER_PROCESS ; ++i){
    MPI_Status status;
    MPI_Recv(sub_area,
             sub_area_size,
             MPI_DOUBLE,
             MPI_ANY_SOURCE,
             0,
             MPI_COMM_WORLD,
             &status);

    // Set pixel values for sub-area
    for (int j = 0; j < sub_area_size - 3; j+= 3){
      setPixel(sub_area[j], sub_area[j + 1], static_cast<int>(sub_area[j + 2]), image);
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

  // See if pixel is in Mandlebrot set, and if so, compute its intensity
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

