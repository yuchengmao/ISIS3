#include "ScatterPlotData.h"

namespace Isis {

  /**
   * ScatterPlotDataConstructor
   *
   *
   * @param cube1
   * @param band1
   * @param cube2
   * @param band2
   * @param numBins
   */
  ScatterPlotData::ScatterPlotData(CubeViewport *cube1, int band1, int numBins1, CubeViewport *cube2, int band2, int numBins2) : QwtRasterData() {
    p_cube1 = cube1->cube();
    p_cube2 = cube2->cube();

    p_cube1Viewport = cube1;
    p_cube2Viewport = cube2;

    p_band1 = band1;
    p_band2 = band2;
    p_numBins1 = numBins1;
    p_numBins2 = numBins2;

    //initializing the 2D vector
    for(unsigned int i = 0; i < 257; i++) {
      p_counts.push_back(std::vector<int>());
      for(unsigned int j = 0; j < 257; j++) {
        p_counts[i].push_back(0);
      }
    }

    //-------------------------------
    // stretch cube one / band one
    //-------------------------------
    double ssamp, esamp, sline, eline;
    p_cube1Viewport->viewportToCube(0, 0, ssamp, sline);
    p_cube1Viewport->viewportToCube(p_cube1Viewport->viewport()->width() - 1,
                                    p_cube1Viewport->viewport()->height() - 1,
                                    esamp, eline);


    Statistics stats;
    Brick brick(1, 1, 1, p_cube1->getPixelType());

    int bufns1 = (int)esamp - (int)ssamp + 1;
    brick.Resize(bufns1, 1, 1);

    //for (int line=(int)sline; line <= (int)eline; line+=lineRate) {
    for(int line = (int)sline; line <= (int)eline; line++) {
      brick.SetBasePosition((int)ssamp, line, band1);
      p_cube1->read(brick);
      stats.AddData(brick.DoubleBuffer(), bufns1);
    }

    p_min1 = stats.Minimum();
    p_max1 = stats.Maximum();
    p_str1.AddPair(p_min1, 0);
    p_str1.AddPair(p_max1, numBins1 - 1);

    //----------------------------
    // stretch cube two / band two
    //-----------------------------
    p_cube2Viewport->viewportToCube(0, 0, ssamp, sline);
    p_cube2Viewport->viewportToCube(p_cube2Viewport->viewport()->width() - 1,
                                    p_cube2Viewport->viewport()->height() - 1,
                                    esamp, eline);

    stats.Reset();
    int bufns2 = (int)esamp - (int)ssamp + 1;
    brick.Resize(bufns2, 1, 1);

    //for (int line=(int)sline; line <=(int) eline; line+=lineRate) {
    for(int line = (int)sline; line <= (int)eline; line++) {
      brick.SetBasePosition((int)ssamp, line, band2);
      p_cube2->read(brick);
      stats.AddData(brick.DoubleBuffer(), bufns2);
    }

    p_min2 = stats.Minimum();

    p_max2 = stats.Maximum();
    p_str2.AddPair(p_min2, 0);
    p_str2.AddPair(p_max2, numBins2 - 1);

    // -------------------------------------
    // Done getting min max for both cubes!
    // -------------------------------------

    setBoundingRect(QwtDoubleRect(p_min1, p_min2, p_max1, p_max2));

    Brick brick1(1, 1, 1, p_cube1->getPixelType());
    Brick brick2(1, 1, 1, p_cube2->getPixelType());

    // -----------------------------------------------
    // If the actual cube size is smaller than the
    // visible cube viewport area, we don't need
    // to loop through all the access lines/samples.
    // -----------------------------------------------
    if((esamp - ssamp) > p_cube1->getSampleCount()) {
      ssamp = 0;
      esamp = p_cube1->getSampleCount();
    }
    if((eline - sline) > p_cube1->getLineCount()) {
      sline = 0;
      eline = p_cube1->getLineCount();
    }

    for(int s = (int)ssamp; s < (int)esamp; s++) {
      for(int l = (int)sline; l <= (int)eline; l++) {
        brick1.SetBasePosition(s, l, band1);
        p_cube1->read(brick1);

        unsigned int x = (int)(p_str1.Map(brick1[0]));

        brick2.SetBasePosition(s, l, band2);
        p_cube2->read(brick2);

        unsigned int y = (int)(p_str2.Map(brick2[0]));
        if(x < p_counts.size() && y < p_counts.size()) {
          p_counts[x][y] = p_counts[x][y] + 1;
        }
      }
    }
  } //end SpectrogramData contstructor


  /**
   * Destructor
   *
   */
  ScatterPlotData::~ScatterPlotData() {
    p_cube1->close();
    p_cube2->close();
  }


  /**
   * Returns a copy of the ScatterPlotData object.
   *
   */
  QwtRasterData *ScatterPlotData::copy() const {
    return new ScatterPlotData(p_cube1Viewport, p_band1, p_numBins1, p_cube1Viewport,
                               p_band2, p_numBins2);
  }


  /**
   * Returns the range of the counts.
   *
   */
  QwtDoubleInterval ScatterPlotData::range() const {
    int max = 0;
    for(unsigned int i = 0; i < p_counts.size(); i++) {
      for(unsigned int j = 0; j < p_counts.size(); j++) {
        if(p_counts[i][j] > max) max = p_counts[i][j];
      }
    }
    return QwtDoubleInterval(0.0, max);
  }


  /**
   * This gets called every time the scatter plot is re-drawn.
   * It returns the counts for each DN (x), DN (y).
   */
  double ScatterPlotData::value(double x, double y) const {
    unsigned int iX = (int)(p_str1.Map(x));
    unsigned int iY = (int)(p_str2.Map(y));

    if(iX >= p_counts.size() || iY >= p_counts.size()) {
      return 0;
    }
    return p_counts[iX][iY];
  }

}
