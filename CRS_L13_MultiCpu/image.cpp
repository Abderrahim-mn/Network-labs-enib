//----------------------------------------------------------------------------

#include "image.hpp"

std::tuple<std::unique_ptr<Pixel[]>, // image data
           int,                      // image width
           int>                      // image height
loadImage(const std::string &fileName)
{
  int width=-1, height=-1;
  int fd{crs::openR(fileName)};
  for(int state{0}; state<3; )
  {
    auto l{crs::readLine(fd)};
    if(auto pos{crs::find(l, '#')}; pos!=-1)
    {
      l.resize(pos);
    }
    l=crs::strip(l);
    if(empty(l))
    {
      continue;
    }
    switch(state)
    {
      case 0:
      {
        if(l!="P6")
        {
          throw std::runtime_error{"P6 expected"};
        }
        ++state;
        break;
      }
      case 1:
      {
        if(crs::extract(l, width, height)!=2)
        {
          throw std::runtime_error{"width and height expected"};
        }
        ++state;
        break;
      }
      case 2:
      {
        if(l!="255")
        {
          throw std::runtime_error{"255 expected"};
        }
        ++state;
        break;
      }
    }
  }
  const int size{width*height};
  auto image{std::make_unique<Pixel[]>(size)};
  crs::readAll(fd, image.get(), size*int(sizeof(image[0])));
  crs::close(fd);
  return {std::move(image),
          std::move(width),
          std::move(height)};
}

void
saveImage(const Pixel *image,
          int width,
          int height,
          const std::string &fileName)
{
  int fd{crs::openW(fileName)};
  crs::writeAll(fd, crs::txt("P6\n% %\n255\n", width, height));
  crs::writeAll(fd, image, width*height*int(sizeof(*image)));
  crs::close(fd);
}

ImageSequence::ImageSequence(const std::vector<std::string> &cmdLine)
: loaded_{}
, current_{0}
{
  for(int i=1; i<crs::len(cmdLine); ++i)
  {
    const auto &f{cmdLine[i]};
    if(crs::isFile(f)&&crs::access(f, R_OK))
    {
      crs::out("loading image % ", f);
      auto [image, width, height]=loadImage(f);
      crs::out("(% x %)\n", width, height);
      loaded_.emplace_back(Info_{std::move(image), width, height});
    }
  }
  if(empty(loaded_))
  {
    throw std::runtime_error{"no images provided"};
  }
}

std::tuple<Pixel *, // image data
           int,     // image width
           int>     // image height
ImageSequence::next()
{
  auto &info{loaded_[current_++%crs::len(loaded_)]};
  return {info.image.get(),
          info.width,
          info.height};
}

//----------------------------------------------------------------------------
