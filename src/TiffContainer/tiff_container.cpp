
#include <iostream>
#include <algorithm>
#include <string>   // memset
#include <cassert>
#include <cmath>

#include "tiff_container.h"
/********************** tiff_image **********************************/
// public

/// Create an empty image
/** @param length length of the image in pixels
    @param width  width of the image in pixels
    @param bits_per_pixel range of a pixel value in bits
    @param dir_numer Number of the image in the tiff stack
**/


tiff_image16_ref::tiff_image16_ref(int length, int width, int bits_per_pixel,  int dir_number)
  : length_(length), width_(width), scanline_size_(width * sizeof(uint16_t)),
    bits_per_pixel_(bits_per_pixel), ref_count_(new int), dir_number_(dir_number)
{ 
  /*
  uint16_t **data = new uint16_t*[length];
  for(int row = 0; row < length; row++) {
    data[row] = new uint16_t[width];
    memset(data[row], '\0', scanline_size_);
  }*/
  
  
  uint16_t **data = new uint16_t*[length];
  uint16_t *data_rows = new uint16_t[length * width];
  
  for(int row = 0; row < (int) length; row++) {
    data[row] =  &data_rows[row * width];
    memset(data[row], '\0', scanline_size_);
  }
  
  assert(scanline_size_ == width * (int) sizeof(uint16_t));
  
  data_ = data;
  *ref_count_ = 1;
}

/// Copy an image reference without copying the image
/** @param image The reference to copy
**/
tiff_image16_ref::tiff_image16_ref(tiff_image16_ref const& image)
{ 
  if(data_ == image.data_) return;

  data_ = image.data_;
  length_ = image.length_;
  width_ = image.width_;
  scanline_size_ = image.scanline_size_;
  bits_per_pixel_ = image.bits_per_pixel_;
  dir_number_ = image.dir_number_;
  ref_count_ = image.ref_count_;

  (*ref_count_)++;
}

/// Image destructor, delete data if last reference to it gets destructed
tiff_image16_ref::~tiff_image16_ref()
{
  (*ref_count_)--;
  
  
  if(*ref_count_ == 0) {
    delete[] data_[0];
    delete[] data_;
    delete ref_count_;
  }
  
  /*
  if(*ref_count_ == 0) {
    for(int row = 0; row < length_; row++) {
      delete[] data_[row];
    }
    delete[] data_;
  }*/

  
}

/// Copy an image by copying its data
tiff_image16_ref tiff_image16_ref::copy()
{ 
  
  uint16_t **data = new uint16_t*[length_];
  uint16_t *data_rows = new uint16_t[length_ * width_];
  for(int row = 0; row < length_; row++) {
    data[row] = &data_rows[row * width_];
    memcpy(data[row], data_[row], scanline_size_);
  }
  
  /*
  uint16_t **data = new uint16_t*[length_];
  for(int row = 0; row < length_; row++) {
    data[row] = new uint16_t[width_];
    memcpy(data[row], data_[row], scanline_size_);
  }*/
  
  assert(scanline_size_ == width_ * (int) sizeof(uint16_t));
  
  return tiff_image16_ref(data, length_, width_, scanline_size_, bits_per_pixel_, dir_number_);
}

/// Return data array of the image
uint16_t *const *tiff_image16_ref::get_data()
{
  return data_;
}

/// Return constant array of the data
uint16_t const* const *tiff_image16_ref::get_data() const
{
  return data_;
}

/// Return the length of the image
/** @return The length of the image in pixels
**/
int tiff_image16_ref::get_length() const
{
  return length_;
}

/// Return the length of the image
/** @return The length of the image in pixels
**/
int tiff_image16_ref::get_width() const
{
  return width_;
}

/// Return the directory number of the image in the tiff stack
/** @return The directory number of the image in the tiff stack
**/
int tiff_image16_ref::get_dir_number() const
{
  return dir_number_;
}

/// Return the size of one line in bytes of the image
/** @return The size of one line in bytes of the image
**/
int tiff_image16_ref::get_scanline_size() const
{
  return scanline_size_;
}

/// Return the number of bytes used to store one pixel
/** @return The number of bytes used to store one pixel
**/
int tiff_image16_ref::get_bytes_per_pixel() const
{
  return scanline_size_ / width_;
}

/// Retunr the range of a pixel value in bits
int tiff_image16_ref::get_bits_per_pixel() const
{
  return bits_per_pixel_;
}

/// Assignment operator
tiff_image16_ref& tiff_image16_ref::operator=(tiff_image16_ref const& image)
{
  if(data_ == image.data_) return *this;
  
  (*ref_count_)--;
  if(*ref_count_ == 0) {
    delete[] data_[0];
    delete[] data_;
    delete ref_count_;
  }
  
  /*
  if(*ref_count_ == 0) {
    for(int row = 0; row < length_; row++) {
      delete[] data_[row];
    }
    delete[] data_;
  }*/

  data_ = image.data_;
  length_ = image.length_;
  width_ = image.width_;
  scanline_size_ = image.scanline_size_;
  bits_per_pixel_ = image.bits_per_pixel_;
  dir_number_ = image.dir_number_;
  ref_count_ = image.ref_count_;

  (*ref_count_)++;
  
  return *this;
}

/// Shift all pixel values to the left
/** @param shift shift amount
**/
tiff_image16_ref& tiff_image16_ref::operator<<=(int shift)
{
  for(int row = 0; row < length_; row++) {
    for(int col = 0; col < width_; col++) {
      data_[row][col] <<= shift;
    }
  }
  
  return *this;
}

/// Shift all pixel values to the right
/** @param shift shift amount
**/
tiff_image16_ref& tiff_image16_ref::operator>>=(int shift)
{
  for(int row = 0; row < length_; row++) {
    for(int col = 0; col < width_; col++) {
      data_[row][col] >>= shift;
    }
  }
  
  return *this;
}


/// Subtract an image from this image
/** @param image The image to subtract
**/
tiff_image16_ref& tiff_image16_ref::operator-=(tiff_image16_ref const& image)
{
  assert(length_ == image.length_ && width_ == image.width_);
  
  for(int row = 0; row < length_; row++) {
    for(int col = 0; col < width_; col++) {
      data_[row][col] = std::max(data_[row][col] - image.data_[row][col],0);
    }
  }
  
  return *this;
}

/// Add an image to this image
/** @param image The image to add
**/
tiff_image16_ref& tiff_image16_ref::operator+=(tiff_image16_ref const& image)
{
  assert(length_ == image.length_ && width_ == image.width_);
  
  for(int row = 0; row < length_; row++) {
    for(int col = 0; col < width_; col++) {
      data_[row][col] = data_[row][col] + image.data_[row][col];
    }
  }
  
  return *this;
}

/// Multiply every pixel value by a factor
/** @param d Factor to multiply each pixel value with
**/

tiff_image16_ref& tiff_image16_ref::operator*=(double d)
{
    
  for(int row = 0; row < length_; row++) {
    for(int col = 0; col < width_; col++) {
      data_[row][col] = (uint16_t) (data_[row][col] * d);
    }
  }
  
  return *this;
}


/// Subtract background from image, update background, and return mean background
/** @param image bg Image of the background
    @param img_weight Defines how much of the current image will form the updated background
    @return Mean value of picture
**/
int tiff_image16_ref::subtr_and_update_bg(tiff_image16_ref const& bg, float img_weight)
{
  assert(length_ == bg.length_ && width_ == bg.width_);
  assert(img_weight >= 0 && img_weight <= 1);
  
  int meanbg = 0;
  
  for(int row = 0; row < length_; row++) {
    for(int col = 0; col < width_; col++) {
      uint16_t img_data = data_[row][col];
      uint16_t bg_data = bg.data_[row][col];
      uint16_t diff_data = std::min<uint16_t>( (img_data - bg_data) , (uint16_t)sqrt((double)img_data) ) ; // subtract background and test if change is bigger than sigma of noise
      data_[row][col] =  std::max( img_data - bg_data , 0 );
             
      bg.data_[row][col] = (uint16_t) (bg_data  + diff_data * img_weight);      // update background
      meanbg += img_data;
    }
  }
  meanbg /= (length_*width_);
  
  return meanbg;
}


/// Subtract a value from all pixel values
/** @param subtrahend The value to subtract from each pixel value
**/
tiff_image16_ref& tiff_image16_ref::operator-=(int subtrahend)
{
  for(int row = 0; row < length_; row++) {
    for(int col = 0; col < width_; col++) {
      data_[row][col] = std::max(data_[row][col] - subtrahend, 0);
    }
  }
  
  return *this;
}


/// Apply a 2D high-pass filter to the image
/** @param fir_radius The radius of the square where the average is calculated from
**/
tiff_image16_ref& tiff_image16_ref::apply_highpass(int fir_radius)
{  
  for(int row = 0; row < length_; row++) {
    for(int col = 0; col < width_; col++) {
      
      int halo_count = 0;
      int halo_acc = sum_halo(row, col, fir_radius, halo_count);
      
      data_[row][col] = data_[row][col] -halo_acc / halo_count;
    }
  }
  
  return *this;
}

/// Print a histogram of all pixel values to stdout
int tiff_image16_ref::histogram()
{
  const int HISTSIZE = 5000;
  int *values = new int[HISTSIZE];
  memset(values, '\0', HISTSIZE );
  int maxvalue=0;


  for(int i=0;i<length_;i++)
  {
    for(int j=0;j<width_;j++)
    {
      if(data_[i][j]>=HISTSIZE)
      {
        maxvalue=HISTSIZE;
        values[HISTSIZE-1]++;
      } else {
        values[data_[i][j]]++;
        if (data_[i][j]>maxvalue) 
          maxvalue=data_[i][j];
      }
    }
  }  
  /* output */
  for(int i=0;i <= maxvalue; i++)
  {
    std::cout << i << "\t" << values[i] << std::endl;
  }
  delete values;
  
  return maxvalue;    
}

/// Calculate and subtract the mean pixel value from the image
/** @return The mean pixel value
**/
int tiff_image16_ref::substract_meanvalue()
{

  int meanbg = 0;
  
  for(int i=0; i<length_; i++)
  {
    for(int j=0; j<width_; j++)
    {
      meanbg += data_[i][j];
    }  
  }
  meanbg = meanbg / (length_ * width_);

  for(int i=0; i<length_; i++)
  {
    for(int j=0; j<width_; j++)
    {
      data_[i][j] = std::max( data_[i][j] - meanbg, 0);
      //data_[i][j] -= value;    
    }  
  }   
  
  return (int) meanbg;
}


/// Calculate the mean pixel value from the image
/** @return The mean pixel value
**/
int tiff_image16_ref::calc_meanvalue()
{
 int value = 0;
  
  for(int i=0; i<length_; i++)
  {
    for(int j=0; j<width_; j++)
    {
      value += data_[i][j];
    }  
  }
  value = value / (length_ * width_);
  
  return value;
}

/// Output the image as CSV
/**  @param out The output stream to print the values to
**/
void tiff_image16_ref::write_as_csv(std::ostream &out)
{
  for(int row = 0; row < length_; row++)
  {
    for(int col = 0; col < width_; col++)
    {
      out << data_[row][col] << "; ";
    }
    out << std::endl;
  }
}


// private
/// Create an image reference from a tiff container
tiff_image16_ref::tiff_image16_ref(uint16_t *const *data, int length, int width,
                           int scanline_size, int bits_per_pixel, int dir_number)
  : data_(data), length_(length), width_(width), scanline_size_(scanline_size),
    bits_per_pixel_(bits_per_pixel), ref_count_(new int), dir_number_(dir_number)
{           
  *ref_count_ = 1;
}

/// Calculate the sum of all pixel values in a halo
int tiff_image16_ref::sum_halo(int row, int col, int fir_radius, int& halo_count)
{
  int halo_acc = 0;
  halo_count = 0;
  
  for(int row_halo = row - fir_radius; row_halo < row + fir_radius + 1; row_halo++) {
    for(int col_halo = col - fir_radius; col_halo < col + fir_radius + 1; col_halo++) {
      if(row_halo > 0 && row_halo < length_ && col_halo > 0 && col_halo < width_) {
        halo_acc += data_[row_halo][col_halo];
        halo_count++;
      }
    }
  }
  
  return halo_acc;
}


/********************** tiff_container **********************************/
// public

/// Create a tiff container object from a tiff file
tiff_container::tiff_container(char *  path,char * mode)
	: path_(path),mode_(mode)
{
  TIFFSetWarningHandler(&TIFFWarningHandler);
  tiff_ = TIFFOpen(path_, mode_);
  if(!tiff_) {
    good_ = false;
  }
  good_ = true;
}

/// Close a tiff container
tiff_container::~tiff_container()
{
  if(good_) {
    TIFFClose(tiff_);
  }
}

/// Indicate wheter the tiff container could be opened
/** @return true iff tiff file could be read and object is valie
**/
bool tiff_container::good()
{
  return good_;
}

/// Return wheter the images in the container can be saved after modifications
/** true iff the images in the container can be saved after modifications
**/
bool tiff_container::writeable()
{
  return mode_[0]=='w';
}

/// Return the opening mode of the tiff file
/** @return the opening mode of the tiff file
**/
const char * const  tiff_container::get_mode()
{
  return mode_;
}

/// Get the path of the tiff file
/** @param the path of the tiff file
**/
const char * const   tiff_container::get_path()
{
  return path_;
}

/// Get the number of images in the tiff container
/** @return the number of images in the tiff container
**/
int tiff_container::get_dir_count()
{
  int count = 0;
  TIFFSetDirectory(tiff_, 0);
  
  do {
    count++;
  } while (TIFFReadDirectory(tiff_));
  
  TIFFSetDirectory(tiff_, 0);
  return count;
}

/// Get an image from the tiff container
/** @param dir The number of the image
    @return a refernce to the requested image
**/
tiff_image16_ref tiff_container::get_image(int dir)
{ 
  TIFFSetDirectory(tiff_, dir);
  
  tsize_t scanline_size = TIFFScanlineSize(tiff_);
  
  uint32 length;
  TIFFGetField(tiff_,  TIFFTAG_IMAGELENGTH, &length);
  
  uint32 width;
  TIFFGetField(tiff_,  TIFFTAG_IMAGEWIDTH, &width);
  
  uint16_t bits_per_pixel;
  TIFFGetField(tiff_, TIFFTAG_BITSPERSAMPLE, &bits_per_pixel);
  
  assert(scanline_size == (tsize_t) (sizeof(uint16_t) * width));
  
  /*
  uint16_t **data = new uint16_t*[length];
  for(int row = 0; row < (int) length; row++) {
    data[row] = new uint16_t[width];
    TIFFReadScanline(tiff_, data[row], row);
  }*/
  
  
  uint16_t **data = new uint16_t*[length];
  uint16_t *data_rows = new uint16_t[length * width];
  
  for(int row = 0; row < (int) length; row++) {
    data[row] =  &data_rows[row * width];
    TIFFReadScanline(tiff_, data[row], row);
  }
  
  // rely on return value optimization here,
  // copy-constructor should not be called
  return tiff_image16_ref(data, length, width, scanline_size, bits_per_pixel, dir);
}

/// Append an image to the end of the tiff container
/** @param image the image to append
**/
void tiff_container::append_image(tiff_image16_ref const& image)
{
  int length = image.get_length();

  TIFFSetField(tiff_, TIFFTAG_IMAGEWIDTH, image.get_width());      // set the width of the image
  TIFFSetField(tiff_, TIFFTAG_IMAGELENGTH, image.get_length());    // set the height of the image
  TIFFSetField(tiff_, TIFFTAG_SAMPLESPERPIXEL, 1);                 // set number of channels per pixel
  TIFFSetField(tiff_, TIFFTAG_BITSPERSAMPLE, 16);                  // set the size of the channels
  TIFFSetField(tiff_, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);   // set the origin of the image
  TIFFSetField(tiff_, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
  TIFFSetField(tiff_, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(tiff_, image.get_scanline_size()));

  for(int row = 0; row < length; row++) {
    TIFFWriteScanline(tiff_, (tdata_t) image.get_data()[row], row);
  }
  
  TIFFWriteDirectory(tiff_);
  
}

void tiff_container::append_new_16bit_image(uint16_t * data, int width, int length)
{

  TIFFSetField(tiff_, TIFFTAG_IMAGEWIDTH, width);      // set the width of the image
  TIFFSetField(tiff_, TIFFTAG_IMAGELENGTH,length);    // set the height of the image
  TIFFSetField(tiff_, TIFFTAG_SAMPLESPERPIXEL, 1);                 // set number of channels per pixel
  TIFFSetField(tiff_, TIFFTAG_BITSPERSAMPLE, 16);                  // set the size of the channels
  TIFFSetField(tiff_, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);   // set the origin of the image
  TIFFSetField(tiff_, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
  TIFFSetField(tiff_, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(tiff_, width*2));

  for(int row = 0; row < length; row++) {
    TIFFWriteScanline(tiff_, (tdata_t) &data[row*width], row);
  }
  
  TIFFWriteDirectory(tiff_);
  
}

void tiff_container::save_stack_as_tif(uint16_t * data_stack, int width, int length, int dir_count ){

	for(int dir=0; dir<dir_count; dir++){
		append_new_16bit_image(&data_stack[dir*width*length], width, length);
	}
}

/// Append an image to the end of the tiff container after converting it to 8 bits
/** @param image the image to append
    @param shift the amount of pixels the image should be shifted to the left before appending
**/
void tiff_container::append_as_8bit_image(tiff_image16_ref const& image, int shift)
{
  int length = image.get_length();
  int width  = image.get_width();
  
  TIFFSetField(tiff_, TIFFTAG_IMAGEWIDTH, image.get_width());  // set the width of the image
  TIFFSetField(tiff_, TIFFTAG_IMAGELENGTH, image.get_length());    // set the height of the image
  TIFFSetField(tiff_, TIFFTAG_SAMPLESPERPIXEL, 1);   // set number of channels per pixel
  TIFFSetField(tiff_, TIFFTAG_BITSPERSAMPLE, 8);    // set the size of the channels
  TIFFSetField(tiff_, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);    // set the origin of the image
  TIFFSetField(tiff_, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
  TIFFSetField(tiff_, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(tiff_, width * sizeof(uint8)));
  
  uint8 *row_8bit = new uint8[width];
  
  for(int row = 0; row < length; row++) {
    for(int col = 0; col < width; col++) {
      row_8bit[col] = image.get_data()[row][col] >> shift;
    }
    TIFFWriteScanline(tiff_, (tdata_t) row_8bit, row);
  }
  
  delete[] row_8bit;
  
  TIFFWriteDirectory(tiff_);
}


void tiff_container::TIFFWarningHandler(const char* /*module*/, const char* /*fmt*/, va_list /*ap*/)
{
 
/* do nothing*/
}
