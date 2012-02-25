#define BOOST_CHRONO_HEADER_ONLY

#include <stdlib.h>
#include <mpi.h>
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

// Function prototypes
void generateMandlebrotImage(png::image< png::index_pixel > *image, int world_size);
int cal_pixel(complex c);
inline void setPixel(int x, int y, int color, png::image< png::index_pixel > *image);
void runMasterProcess(int world_rank, int world_size);
void runSlaveProcess(int world_rank, int world_size);

// Global constants

// Some MPI Message tag defines
static const int MANDLEBROT_NORMAL_TAG = 0;
static const int MANDLEBROT_FINISH_TAG = 1;

// Default image size.
// Can set new image size with args 2 and 3 (x and y)
int IMAGE_HEIGHT = 800;
int IMAGE_WIDTH = 1200;

static const int REAL_MAX = 1;
static const int REAL_MIN = -2;
static const int IMAG_MAX = 1;
static const int IMAG_MIN = -1;

static const int ROWS_PER_PROCESS = 1;

int main(int argc, char** argv)
{
  if (argc == 3){
    IMAGE_WIDTH  = static_cast<int>(strtod(argv[1], NULL));
    IMAGE_HEIGHT = static_cast<int>(strtod(argv[2], NULL));
  }

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

  generateMandlebrotImage(&image, world_size); // Compute all the things!

  // End timer
  boost::chrono::duration<double> sec = boost::chrono::system_clock::now() - start;
  std::cout << world_size << " " << IMAGE_WIDTH << "x" <<IMAGE_HEIGHT << " " << sec.count() << " seconds\n";

  image.write("mandlebrot_p.png");
}

void runSlaveProcess(int world_rank, int world_size)
{
  while (1){
    double scale_real = double(REAL_MAX - REAL_MIN) / IMAGE_WIDTH;
    double scale_imag = double(IMAG_MAX - IMAG_MIN) / IMAGE_HEIGHT;

    // Recive row number
    MPI_Status status;
    int row_num = -1;

    MPI_Recv(&row_num,
             1,
             MPI_INT,
             0,
             MPI_ANY_TAG,
             MPI_COMM_WORLD,
             &status);

    if (status.MPI_TAG == MANDLEBROT_FINISH_TAG)
      return;

    // Create structure for results.
    // Should really create MPI_STRUCT but will just
    // use array of doubles for now.
    // Each pixel need three values: x, y, and color.

    std::vector<int> results;

    for(int x = 0; x < IMAGE_WIDTH; ++x){
      for(int y = row_num; y < row_num + ROWS_PER_PROCESS; ++y){
        complex c;
        c.real = REAL_MIN + ((double) x * scale_real);
        c.imag = IMAG_MIN + ((double) y * scale_imag);
        int color = cal_pixel(c);

        // Put in results array
        results.push_back(x);
        results.push_back(y);
        results.push_back(color);
      }
    }

    // Send results to master
    MPI_Send(&(results[0]),
             results.size(),
             MPI_INT,
             0,
             MANDLEBROT_NORMAL_TAG,
             MPI_COMM_WORLD);
  }
}

void generateMandlebrotImage(png::image< png::index_pixel > *image, int world_size)
{
  // Ensure rows are divisible by ROWS_PER_PROCESS
  assert( IMAGE_HEIGHT % ROWS_PER_PROCESS == 0);
  //assert( ROWS_PER_PROCESS == 1);

  double scale_real = double(REAL_MAX - REAL_MIN) / IMAGE_WIDTH;
  double scale_imag = double(IMAG_MAX - IMAG_MIN) / IMAGE_HEIGHT;

  // First, send initial row(s) to each slave.
  int row_index = 0;
  int slave_process_id = 1;
  while (slave_process_id < world_size){

    int return_val = MPI_Send(&row_index,
                              1,
                              MPI_INT,
                              slave_process_id,
                              MANDLEBROT_NORMAL_TAG,
                              MPI_COMM_WORLD);
    // Check for send errors
    if (return_val != MPI_SUCCESS){
      int str_len;
      char* err_str;
      MPI_Error_string(return_val, err_str, &str_len);
      cerr << "Error sending to slaves: " << err_str << endl;
    }

    row_index += ROWS_PER_PROCESS;
    slave_process_id ++;
  }

  // Then revieve sub-area back from each process.
  // If there are rows remaining, send them to the process we
  // just recieved from.

  int sub_area_size = ROWS_PER_PROCESS * IMAGE_WIDTH * 3; // 3 values for each pixel (x, y, color)
  int *sub_area = new int[sub_area_size];
  int rows_recieved = 0;

  while (rows_recieved != IMAGE_HEIGHT){

    // Recieve sub-area
    MPI_Status status;
    MPI_Recv(sub_area,
             sub_area_size,
             MPI_INT,
             MPI_ANY_SOURCE,
             MPI_ANY_TAG,
             MPI_COMM_WORLD,
             &status);

    rows_recieved += ROWS_PER_PROCESS; // This must be 1 currently.

    // Set pixel values for sub-area
    for (int j = 0; j <= sub_area_size - 3; j+= 3){
      setPixel(sub_area[j], sub_area[j + 1], sub_area[j + 2], image);
    }

    // If there are rows remaining to process, send them
    if (row_index != IMAGE_HEIGHT){

      MPI_Send(&row_index,
               1,
               MPI_INT,
               status.MPI_SOURCE, // Send back to slave that we just recieved from
               MANDLEBROT_NORMAL_TAG,
               MPI_COMM_WORLD);

      row_index += ROWS_PER_PROCESS;
    }

    // else tell slave to quit.
    else{
      MPI_Send(&row_index, // This is dummy data now; we are just telling slave to quit.
               1,
               MPI_INT,
               status.MPI_SOURCE,     // Send back to slave that we just recieved from and
               MANDLEBROT_FINISH_TAG, // tell the slave to quit.
               MPI_COMM_WORLD);
    }
  } // done creating image.

  for (int slave = 1; slave < world_size; slave++){
      MPI_Send(&slave, // This is dummy data now; we are just telling slave to quit.
               1,
               MPI_INT,
               slave,     // Send back to slave that we just recieved from and
               MANDLEBROT_FINISH_TAG, // tell the slave to quit.
               MPI_COMM_WORLD);
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
