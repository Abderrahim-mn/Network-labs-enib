//----------------------------------------------------------------------------

#ifndef IMAGE_HPP
#define IMAGE_HPP 1

#include "crsUtils.hpp"

struct Pixel
{
  uint8_t rgb[3];
  const uint8_t & r()                  const { return rgb[0]; }
        uint8_t & r()                        { return rgb[0]; }
  const uint8_t & g()                  const { return rgb[1]; }
        uint8_t & g()                        { return rgb[1]; }
  const uint8_t & b()                  const { return rgb[2]; }
        uint8_t & b()                        { return rgb[2]; }
  const uint8_t & operator[](size_t n) const { return rgb[n]; }
        uint8_t & operator[](size_t n)       { return rgb[n]; }
};

std::tuple<std::unique_ptr<Pixel[]>, // image data
           int,                      // image width
           int>                      // image height
loadImage(const std::string &fileName);

void
saveImage(const Pixel *image,
          int width,
          int height,
          const std::string &fileName);

class ImageSequence
{
public:

  ImageSequence(const std::vector<std::string> &cmdLine);

  ImageSequence() =delete;
  ImageSequence(const ImageSequence &) =delete;
  ImageSequence & operator=(const ImageSequence &) =delete;
  ImageSequence(ImageSequence &&) =delete;
  ImageSequence & operator=(ImageSequence &&) =delete;

  ~ImageSequence() =default;

  int
  current() const
  {
    return current_;
  }

  std::tuple<Pixel *, // image data
             int,     // image width
             int>     // image height
  next();

private:
  struct Info_
  {
    std::unique_ptr<Pixel[]> image;
    int width, height;
  };
  std::vector<Info_> loaded_;
  int current_;
};

#endif // IMAGE_HPP

//----------------------------------------------------------------------------
