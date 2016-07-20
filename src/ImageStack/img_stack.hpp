#ifndef IMAGE_HPP
#define IMAGE_HPP

#include <string>
#include <tiffio.h>
#include <ostream>

#include <stdint.h>
#include <iostream>
#include <fstream>
#include <string>


class image16_ref
{
  friend class tiff_file_accessor;

  public:
    image16_ref(int length, int width, int bits_per_pixel, int img);
    image16_ref(image16_ref const& image);
    image16_ref();
    ~image16_ref();

    image16_ref copy();
    uint16_t *const *get_data();
    uint16_t const *const *get_data() const;
    
    int get_length() const;
    int get_width() const;
    int get_scanline_size() const;
    int get_bytes_per_pixel() const;
    int get_bits_per_pixel() const;
    int get_dir_number() const;
    
    image16_ref& operator=(image16_ref const& image);
    
    image16_ref& operator<<=(int shift); 
    image16_ref& operator>>=(int shift);
    image16_ref& operator*=(double d);
    image16_ref& operator-=(image16_ref const& image);
    image16_ref& operator+=(image16_ref const& image);

    image16_ref& operator-=(int subtrahend);
    
    image16_ref& apply_highpass(int fir_radius);
    image16_ref& apply_highpass(int fir_radius, image16_ref *fir_img);

    int subtr_and_update_bg(image16_ref const& bg, float img_weight = 1.0/16);
    
    int substract_meanvalue();
    int calc_meanvalue();
    
    int histogram();
    
    void write_as_csv(std::ostream &out);
    void print(int size);
    
  private:
    image16_ref(uint16_t *const *data, int length, int width,
                 int scanline_size, int bits_per_pixel, int dir_number);
    
    int sum_halo(int row, int col, int fir_radius);
                 
    uint16_t *const *data_;        ///< data array of image
    int length_;                ///< length of the image
    int width_;                 ///< width of the image
    int scanline_size_;         ///< size of a line in bytes
    int bits_per_pixel_;        ///< bits used to store a pixel value
    int *ref_count_;            ///< nr of references that point to same data
    int dir_number_;            ///< number of the image in its tiff container
};


class img_stack
{
  public:
    img_stack(std::string path, std::string mode = "r");
    ~img_stack();
    
    bool good();
    bool writeable();
    std::string const& get_mode();
    std::string const& get_path();
    
    int img_count();

    image16_ref get_image(int img);
    void append_image(image16_ref const& image);
    void append_as_8bit_image(image16_ref const& image, int shift = 0);
	void append_new_16bit_image(uint16_t * data, int width, int length);

	image16_ref average_img(int begin, int end);
    image16_ref overview_img(int begin, int end);
    
  private:
    class img_file_accessor *accessor_;     ///< The file accessor for the underlying image container
};




#endif

