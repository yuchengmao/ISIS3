/**
 * @file
 * $Revision: 1.2 $
 * $Date: 2008/05/14 21:07:12 $
 *
 *   Unless noted otherwise, the portions of Isis written by the USGS are public
 *   domain. See individual third-party library and package descriptions for
 *   intellectual property information,user agreements, and related information.
 *
 *   Although Isis has been used by the USGS, no warranty, expressed or implied,
 *   is made by the USGS as to the accuracy and functioning of such software
 *   and related material nor shall the fact of distribution constitute any such
 *   warranty, and no responsibility is assumed by the USGS in connection
 *   therewith.
 *
 *   For additional information, launch
 *   $ISISROOT/doc//documents/Disclaimers/Disclaimers.html in a browser or see
 *   the Privacy &amp; Disclaimers page on the Isis website,
 *   http://isis.astrogeology.usgs.gov, and the USGS privacy and disclaimers on
 *   http://www.usgs.gov/privacy.html.
 */


#include "ProcessBySample.h"

#include "Buffer.h"
#include "Cube.h"
#include "iException.h"
#include "Process.h"
#include "ProcessByBrick.h"

using namespace std;
namespace Isis {
  /**
   * Opens an input cube specified by the user and verifies requirements are 
   * met. This method is overloaded and adds the requirements of 
   * ic_base::SpatialMatch which requires all input cubes to have the same 
   * number of samples and lines. It also added the requirement 
   * ic_base::BandMatchOrOne which forces 2nd, 3rd, 4th, etc input cubes to 
   * match the number of bands in the 1st input cube or to have exactly one 
   * band. For more information see Process::SetInputCube
   *
   * @return Cube*
   *
   * @param parameter User parameter to obtain file to open. Typically, the 
   *                  value is "FROM". For example, the user can specify on the
   *                  command line FROM=myfile.cub and this method will attempt
   *                  to open the cube "myfile.cub" if the parameter was set to
   *                  "FROM".
   *
   * @param requirements See Process::SetInputCube for more information.
   *                     Defaults to 0
   *
   * @throws Isis::iException::Message
   */
  Isis::Cube *ProcessBySample::SetInputCube(const std::string &parameter,
      int requirements) {
    int allRequirements = Isis::SpatialMatch | Isis::BandMatchOrOne;
    allRequirements |= requirements;
    return Process::SetInputCube(parameter, allRequirements);
  }


  /**
   * Opens an input cube specified by the user with cube attributes and  
   * requirements. For more information see Process::SetInputCube 
   *
   * @return Cube*
   *
   * @param file Name of cube file 
   * @param att Cube attributes 
   * @param requirements See Process::SetInputCube for more information.
   *                     Defaults to 0
   *
   */
  Isis::Cube *ProcessBySample::SetInputCube(const std::string &file,
      Isis::CubeAttributeInput &att,
      int requirements) {
    int allRequirements = Isis::SpatialMatch | Isis::BandMatchOrOne;
    allRequirements |= requirements;
    return Process::SetInputCube(file, att, allRequirements);
  }


  /**
   * This method invokes the process by sample operation over a single input or
   * output cube. It will be an input cube if the method SetInputCube was 
   * invoked exactly one time before calling StartProcess. It will be an output 
   * cube if the SetOutputCube method was invoked exactly one time. Typically 
   * this method can be used to obtain statistics, histograms, or other 
   * information from an input cube. 
   *
   * @deprecated Please use ProcessCubeInPlace()
   * @param funct (Isis::Buffer &b) Name of your processing function
   *
   * @throws Isis::iException::Message
   *
   */
  void ProcessBySample::StartProcess(void funct(Isis::Buffer &inout)) {
    SetBrickSizesForProcessCubeInPlace();
    ProcessByBrick::StartProcess(funct);
  }


  /**
   * This method invokes the process by sample operation over exactly one input and
   * one output cube. Typically, this method is used for simple operations such
   * as stretching a cube or applying various operators to a cube (add constant,
   * multiply by constant, etc).
   *
   * @deprecated Please use ProcessCube()
   * @param funct (Isis::Buffer &in, Isis::Buffer &out) Name of your processing
   *                                                    function
   *
   * @throws Isis::iException::Message
   */
  void ProcessBySample::StartProcess(void
                                     funct(Isis::Buffer &in, Isis::Buffer &out)) {
    SetBrickSizesForProcessCube();
    ProcessByBrick::StartProcess(funct);
  }


  /**
   * This method invokes the process by sample operation over multiple input and
   * output cubes. Typically, this method is used when two input cubes are
   * required for operations like ratios, differences, masking, etc.
   *
   * @deprecated Please use ProcessCubes()
   * @param funct (vector<Isis::Buffer *> &in, vector<Isis::Buffer *> &out) Name
   *                of your processing function
   *
   * @throws Isis::iException::Message
   */
  void ProcessBySample::StartProcess(void funct(std::vector<Isis::Buffer *> &in,
                                     std::vector<Isis::Buffer *> &out)) {
    SetBrickSizesForProcessCubes();
    ProcessByBrick::StartProcess(funct);
  }


  /**
   * This is a helper method for StartProcess() and ProcessCubeInPlace().
   */
  void ProcessBySample::SetBrickSizesForProcessCubeInPlace() {
    // Error checks
    if((InputCubes.size() + OutputCubes.size()) > 1) {
      string m = "You can only specify exactly one input or output cube";
      throw iException::Message(iException::Programmer, m, _FILEINFO_);
    }
    else if((InputCubes.size() + OutputCubes.size()) == 0) {
      string m = "You haven't specified an input or output cube";
      throw iException::Message(iException::Programmer, m, _FILEINFO_);
    }

    // Determine if we have an input or output
    if(InputCubes.size() == 1) SetBrickSize(1, InputCubes[0]->getLineCount(), 1);
    else SetBrickSize(1, OutputCubes[0]->getLineCount(), 1);
  }


  /**
   * This is a helper method for StartProcess() and ProcessCube().
   */
  void ProcessBySample::SetBrickSizesForProcessCube() {
    // Error checks ... there must be one input and output
    if(InputCubes.size() != 1) {
      string m = "You must specify exactly one input cube";
      throw iException::Message(iException::Programmer, m, _FILEINFO_);
    }
    else if(OutputCubes.size() != 1) {
      string m = "You must specify exactly one output cube";
      throw iException::Message(iException::Programmer, m, _FILEINFO_);
    }

    // The samples in the input and output must match
    if(InputCubes[0]->getSampleCount() != OutputCubes[0]->getSampleCount()) {
      string m = "The number of samples in the input and output cubes ";
      m += "must match";
      throw iException::Message(iException::Programmer, m, _FILEINFO_);
    }

    // The bands in the input and output must match
    if(InputCubes[0]->getBandCount() != OutputCubes[0]->getBandCount()) {
      string m = "The number of bands in the input and output cubes ";
      m += "must match";
      throw iException::Message(iException::Programmer, m, _FILEINFO_);
    }

    SetInputBrickSize(1, InputCubes[0]->getLineCount(), 1);
    SetOutputBrickSize(1, OutputCubes[0]->getLineCount(), 1);
  }


  /**
   * This is a helper method for StartProcess() and ProcessCubes().
   */
  void ProcessBySample::SetBrickSizesForProcessCubes() {
    // Make sure we had an image
    if(InputCubes.size() + OutputCubes.size() < 1) {
      string m = "You have not specified any input or output cubes";
      throw iException::Message(iException::Programmer, m, _FILEINFO_);
    }

    // Make sure all the output images have the same number of bands as
    // the first input/output cube
    for(unsigned int i = 0; i < OutputCubes.size(); i++) {
      if(OutputCubes[i]->getSampleCount() != OutputCubes[0]->getSampleCount()) {
        string m = "All output cubes must have the same number of samples ";
        m += "as the first input cube or output cube";
        throw iException::Message(iException::Programmer, m, _FILEINFO_);
      }
      if(OutputCubes[i]->getBandCount() != OutputCubes[0]->getBandCount()) {
        string m = "All output cubes must have the same number of bands ";
        m += "as the first input cube or output cube";
        throw iException::Message(iException::Programmer, m, _FILEINFO_);
      }
    }

    for(unsigned int i = 0; i < InputCubes.size(); i++) {
      SetInputBrickSize(1, InputCubes[i]->getLineCount(), 1, i + 1);
    }
    for(unsigned int i = 0; i < OutputCubes.size(); i++) {
      SetOutputBrickSize(1, OutputCubes[i]->getLineCount(), 1, i + 1);
    }
  }
}
