//----------------------------------------------------------------------------

#include "image.hpp"

void
histogramEqualisation(Pixel *result,
                      const Pixel *image,
                      int width,
                      int height,
                      int histoR[256],
                      int histoG[256],
                      int histoB[256])
{
  //---- compute histogram ----
  ::memset(histoR, 0, 256*sizeof(*histoR));
  ::memset(histoG, 0, 256*sizeof(*histoG));
  ::memset(histoB, 0, 256*sizeof(*histoB));
  const int size=width*height;
  for(int i=0; i<size; ++i)
  {
    const Pixel p=image[i];
    ++histoR[p.r()];
    ++histoG[p.g()];
    ++histoB[p.b()];
  }

  //---- accumulate and equalise histogram ----
  const double norm=255.0/size;
  for(int prevR=0, prevG=0, prevB=0, i=0; i<256; ++i)
  {
    histoR[i]=int(norm*(prevR+=histoR[i]));
    histoG[i]=int(norm*(prevG+=histoG[i]));
    histoB[i]=int(norm*(prevB+=histoB[i]));
  }

  //---- adjust intensities ----
  for(int i=0; i<size; ++i)
  {
    const Pixel p=image[i];
    result[i].r()=uint8_t(histoR[p.r()]);
    result[i].g()=uint8_t(histoG[p.g()]);
    result[i].b()=uint8_t(histoB[p.b()]);
  }
}

int
main(int argc,
     char **argv)
{
  std::vector<std::string> args{argv, argv+argc};

  //---- prepare images and storage for result ----
  ImageSequence seq{args};
  std::vector<Pixel> storage;
  Pixel *result=nullptr;
  int resultWidth=-1, resultHeight=-1;
  const double t0=crs::gettimeofday();
  const double e0=crs::cpuEnergy();

  //---- process every image in the sequence ----
  for(int img=0; img<3000; ++img)
  {
    const auto [image, width, height]=seq.next();

    //---- ensure storage is large enough for result ----
    const int size=width*height;
    if(size>crs::len(storage))
    {
      storage.resize(size);
    }
    result=data(storage);
    resultWidth=width;
    resultHeight=height;

    //---- perform histogram normalisation ----
    int histoR[256];
    int histoG[256];
    int histoB[256];
    histogramEqualisation(result, image, width, height,
                          histoR, histoG, histoB);
  }

  //---- display performances ----
  const double duration=crs::gettimeofday()-t0;
  const double energy=crs::cpuEnergy()-e0;
  crs::out("% images in % seconds (% per second, % Joules)\n",
           seq.current(), duration, seq.current()/duration, energy);

  //---- save last result (to check correctness) ----
  if(result)
  {
    saveImage(result, resultWidth, resultHeight, "output_last.ppm");
  }

return 0;
}

//----------------------------------------------------------------------------
