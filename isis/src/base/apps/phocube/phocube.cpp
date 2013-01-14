#include "Isis.h"

#include "Angle.h"
#include "Camera.h"
#include "Cube.h"
#include "Projection.h"
#include "ProjectionFactory.h"
#include "ProcessByBrick.h"
#include "ProcessByLine.h"
#include "SpecialPixel.h"
#include "IException.h"

#include <cmath>

using namespace std;
using namespace Isis;

// Global variables
Camera *cam;
Projection *proj;
int nbands;
bool noCamera;

bool dn;
bool phase; 
bool emission;
bool incidence;
bool localEmission;
bool localIncidence;
bool latitude;
bool longitude;
bool pixelResolution;
bool lineResolution;
bool sampleResolution;
bool detectorResolution;
bool northAzimuth;
bool sunAzimuth;
bool spacecraftAzimuth;
bool offnadirAngle;
bool subSpacecraftGroundAzimuth;
bool subSolarGroundAzimuth;
bool morphology;
bool albedo;

void phocubeDN(Buffer &in, Buffer &out);
void phocube(Buffer &out);


// Function to create a keyword with same values of a specified count
template <typename T> PvlKeyword makeKey(const QString &name, 
                                         const int &nvals, 
                                         const T &value);

// Structure containing new mosaic planes
struct MosData {
  MosData() :  m_morph(Null), m_albedo(Null) {  }
  double m_morph;
  double m_albedo;
};

// Computes the special MORPHOLOGY and ALBEDO planes
MosData *getMosaicIndicies(Camera &camera, MosData &md);
// Updates BandBin keyword
void UpdateBandKey(const QString &keyname, PvlGroup &bb, const int &nvals, 
                   const QString &default_value = "Null");



void IsisMain() {
  UserInterface &ui = Application::GetUserInterface();

  // Get the camera information if this is not a mosaic. Otherwise, get the
  // projection information
  Process p1;
  Cube *icube = p1.SetInputCube("FROM", OneBand);
  if (ui.GetString("SOURCE") == "CAMERA") {
    noCamera = false;
  }
  else {
    noCamera = true;
  }

  if(noCamera) {
    try {
      proj = icube->projection();
    }
    catch(IException &e) {
      QString msg = "Mosaic files must contain mapping labels";
      throw IException(e, IException::User, msg, _FILEINFO_);
    }
  } 
  else {
    try {
      cam = icube->camera();
    }
    catch(IException &e) {
      QString msg = "Input file needs to have spiceinit run on it - if this file ";
      msg += "is a mosaic, then check the MOSAICONLY box";
      throw IException(e, IException::User, msg, _FILEINFO_);
    }
  }

  // We will be processing by brick.
  ProcessByBrick p;

  // Find out which bands are to be created
  nbands = 0;
  phase = false;
  emission = false;
  incidence = false;
  localEmission = false;
  localIncidence = false;
  lineResolution = false;
  sampleResolution = false;
  detectorResolution = false;
  sunAzimuth = false;
  spacecraftAzimuth = false;
  offnadirAngle = false;
  subSpacecraftGroundAzimuth = false;
  subSolarGroundAzimuth = false;
  morphology = false;
  albedo = false;
  northAzimuth = false;
  if (!noCamera) {
    if((phase = ui.GetBoolean("PHASE"))) nbands++;
    if((emission = ui.GetBoolean("EMISSION"))) nbands++;
    if((incidence = ui.GetBoolean("INCIDENCE"))) nbands++;
    if((localEmission = ui.GetBoolean("LOCALEMISSION"))) nbands++;
    if((localIncidence = ui.GetBoolean("LOCALINCIDENCE"))) nbands++;
    if((lineResolution = ui.GetBoolean("LINERESOLUTION"))) nbands++;
    if((sampleResolution = ui.GetBoolean("SAMPLERESOLUTION"))) nbands++;
    if((detectorResolution = ui.GetBoolean("DETECTORRESOLUTION"))) nbands++;
    if((sunAzimuth = ui.GetBoolean("SUNAZIMUTH"))) nbands++;
    if((spacecraftAzimuth = ui.GetBoolean("SPACECRAFTAZIMUTH"))) nbands++;
    if((offnadirAngle = ui.GetBoolean("OFFNADIRANGLE"))) nbands++;
    if((subSpacecraftGroundAzimuth = ui.GetBoolean("SUBSPACECRAFTGROUNDAZIMUTH"))) nbands++;
    if((subSolarGroundAzimuth = ui.GetBoolean("SUBSOLARGROUNDAZIMUTH"))) nbands++;
    if ((morphology = ui.GetBoolean("MORPHOLOGY"))) nbands++; 
    if ((albedo = ui.GetBoolean("ALBEDO"))) nbands++; 
    if ((northAzimuth = ui.GetBoolean("NORTHAZIMUTH"))) nbands++; 
  }
  if((dn = ui.GetBoolean("DN"))) nbands++;
  if((latitude = ui.GetBoolean("LATITUDE"))) nbands++;
  if((longitude = ui.GetBoolean("LONGITUDE"))) nbands++;
  if((pixelResolution = ui.GetBoolean("PIXELRESOLUTION"))) nbands++;

  if(nbands < 1) {
    QString message = "At least one photometry parameter must be entered"
                     "[PHASE, EMISSION, INCIDENCE, LATITUDE, LONGITUDE...]";
    throw IException(IException::User, message, _FILEINFO_);
  }

  // If outputting a a dn band, retrieve the orignal values for the filter name from the input cube,
  // if it exists.  Otherwise, the default will be "DN"
  QString bname = "DN";
  if ( dn && icube->hasGroup("BandBin") ) {
    PvlGroup &mybb = icube->group("BandBin");
    if ( mybb.HasKeyword("Name") ) {
      bname = mybb["Name"][0];
    }
    else if ( mybb.HasKeyword("FilterName") ) {
      bname = mybb["FilterName"][0];
    }
  }

  // Create a bandbin group for the output label
  PvlKeyword name("Name");
  if (dn) name += bname;
  if(phase) name += "Phase Angle";
  if(emission) name += "Emission Angle";
  if(incidence) name += "Incidence Angle";
  if(localEmission) name += "Local Emission Angle";
  if(localIncidence) name += "Local Incidence Angle";
  if(latitude) name += "Latitude";
  if(longitude) name += "Longitude";
  if(pixelResolution) name += "Pixel Resolution";
  if(lineResolution) name += "Line Resolution";
  if(sampleResolution) name += "Sample Resolution";
  if(detectorResolution) name += "Detector Resolution";
  if(northAzimuth) name += "North Azimuth";
  if(sunAzimuth) name += "Sun Azimuth";
  if(spacecraftAzimuth) name += "Spacecraft Azimuth";
  if(offnadirAngle) name += "OffNadir Angle";
  if(subSpacecraftGroundAzimuth) name += "Sub Spacecraft Ground Azimuth";
  if(subSolarGroundAzimuth) name += "Sub Solar Ground Azimuth";
  if (morphology) name += "Morphology";
  if (albedo) name += "Albedo";


  // Create the output cube.  Note we add the input cube to expedite propagation
  // of input cube elements (label, blobs, etc...).  It will be cleared
  // prior to systematic processing only if the DN option is not selected.
  // If DN is chosen by the user, then we propagate the input buffer with a 
  // different function - one that accepts both input and output buffers.
  (void) p.SetInputCube("FROM", OneBand);
  Cube *ocube = p.SetOutputCube("TO", icube->sampleCount(), 
                                icube->lineCount(), nbands);
  p.SetBrickSize(64, 64, nbands);

  if (dn) {
    // Process with input and output buffers
    p.StartProcess(phocubeDN);
  }
  else {
    // Toss the input file as stated above
    p.ClearInputCubes();

    // Start the processing
    p.StartProcess(phocube);
  }

  // Add the bandbin group to the output label.  If a BandBin group already
  // exists, remove all existing keywords and add the keywords for this app.
  // Otherwise, just put the group in.
  PvlObject &cobj = ocube->label()->FindObject("IsisCube");
  if(!cobj.HasGroup("BandBin")) {
    cobj.AddGroup(PvlGroup("BandBin"));
  }

  PvlGroup &bb = cobj.FindGroup("BandBin");
  bb.AddKeyword(name, PvlContainer::Replace);
  int nvals = name.Size();
  UpdateBandKey("Center", bb, nvals, "1.0");

  if ( bb.HasKeyword("OriginalBand") ) {
    UpdateBandKey("OriginalBand", bb, nvals, "1.0");
  }

  if ( bb.HasKeyword("Number") ) {
    UpdateBandKey("Number", bb, nvals, "1.0");
  }

  UpdateBandKey("Width", bb, nvals, "1.0");



  p.EndProcess();
}


//  This propagates the input plane to the output plane, then passes it off to
//  the general routine
void phocubeDN(Buffer &in, Buffer &out) {
  for (int i = 0 ; i < in.size() ; i++) {
    out[i] = in[i];
  }
  phocube(out);
}


//  Computes all the geometric properties for the output buffer.  Certain
//  knowledge of the buffers size is assumed below, so ensure the buffer
//  is still of the expected size.
void phocube(Buffer &out) {


  // If the DN option is selected, it is already added by the phocubeDN
  // function.  We must compute the offset to start at the second band.
  int skipDN = (dn) ? 64 * 64   :  0;

  for(int i = 0; i < 64; i++) {
    for(int j = 0; j < 64; j++) {

      MosData mosd, *p_mosd(0);  // For special mosaic angles

      int index = i * 64 + j + skipDN;
      double samp = out.Sample(index);
      double line = out.Line(index);
      if (noCamera) {
        proj->SetWorld(samp, line);
      }
      else {
        cam->SetImage(samp, line);
      }

      bool isGood=false;
      if (noCamera) {
        if (proj->IsGood()) {
          isGood = true;
        }
      }
      else {
        if (cam->HasSurfaceIntersection()) {
          isGood = true;
        }
      }
      if (isGood) {
        if(phase) {
          out[index] = cam->PhaseAngle();
          index += 64 * 64;
        }
        if(emission) {
          out[index] = cam->EmissionAngle();
          index += 64 * 64;
        }
        if(incidence) {
          out[index] = cam->IncidenceAngle();
          index += 64 * 64;
        }
        if(localEmission || localIncidence) {
          Angle phase;
          Angle incidence;
          Angle emission;
          bool success;
          cam->LocalPhotometricAngles(phase, incidence, emission, success);

          if (localEmission) {
            out[index] = emission.degrees();
            index += 64 * 64;
          }

          if (localIncidence) {
            out[index] = incidence.degrees();
            index += 64 * 64;
          }
        }
        if(latitude) {
          if(noCamera) {
            out[index] = proj->UniversalLatitude();
          }
          else {
            out[index] = cam->UniversalLatitude();
          }
          index += 64 * 64;
        }
        if(longitude) {
          if(noCamera) {
            out[index] = proj->UniversalLongitude();
          }
          else {
            out[index] = cam->UniversalLongitude();
          }
          index += 64 * 64;
        }
        if(pixelResolution) {
          if(noCamera) {
            out[index] = proj->Resolution();
          }
          else {
            out[index] = cam->PixelResolution();
          }
          index += 64 * 64;
        }
        if(lineResolution) {
          out[index] = cam->LineResolution();
          index += 64 * 64;
        }
        if(sampleResolution) {
          out[index] = cam->SampleResolution();
          index += 64 * 64;
        }
        if(detectorResolution) {
          out[index] = cam->DetectorResolution();
          index += 64 * 64;
        }
        if(northAzimuth) {
          out[index] = cam->NorthAzimuth();
          index += 64 * 64;
        }
        if(sunAzimuth) {
          out[index] = cam->SunAzimuth();
          index += 64 * 64;
        }
        if(spacecraftAzimuth) {
          out[index] = cam->SpacecraftAzimuth();
          index += 64 * 64;
        }
        if(offnadirAngle) {
          out[index] = cam->OffNadirAngle();
          index += 64 * 64;
        }
        if(subSpacecraftGroundAzimuth) {
          double ssplat, ssplon;
          ssplat = ssplon = 0.0;
          cam->subSpacecraftPoint(ssplat, ssplon);
          out[index] = cam->GroundAzimuth(cam->UniversalLatitude(),
              cam->UniversalLongitude(), ssplat, ssplon);
          index += 64 * 64;
        }
        if(subSolarGroundAzimuth) {
          double sslat, sslon;
          sslat = sslon = 0.0;
          cam->subSolarPoint(sslat,sslon);
          out[index] = cam->GroundAzimuth(cam->UniversalLatitude(),
              cam->UniversalLongitude(), sslat, sslon);
          index += 64 * 64;
        }

        // Special Mosaic indexes
        if (morphology) {
          if (!p_mosd) { p_mosd = getMosaicIndicies(*cam, mosd); }
          out[index] = mosd.m_morph;
          index += 64 * 64;
        }

        if (albedo) {
          if (!p_mosd) { p_mosd = getMosaicIndicies(*cam, mosd); }
          out[index] = mosd.m_albedo;
          index += 64 * 64;
        }

      }
      // Trim outerspace
      else {
        for(int b = (skipDN) ? 1 : 0; b < nbands; b++) {
          out[index] = Isis::NULL8;
          index += 64 * 64;
        }
      }
    }
  }
}


// Function to create a keyword with same values of a specified count
template <typename T>
  PvlKeyword makeKey(const QString &name, const int &nvals,
                     const T &value) {
    PvlKeyword key(name);
    for (int i = 0 ; i < nvals ; i++) {
      key += value;
    }
    return (key);
  }


// Computes the special MORPHOLOGY and ALBEDO planes
MosData *getMosaicIndicies(Camera &camera, MosData &md) {
  const double Epsilon(1.0E-8);
  Angle myphase;
  Angle myincidence;
  Angle myemission;
  bool mysuccess;
  camera.LocalPhotometricAngles(myphase, myincidence, myemission, mysuccess);
  if (!mysuccess) {
    myemission.setDegrees(camera.EmissionAngle());
    myincidence.setDegrees(camera.IncidenceAngle());
  }
  double res = camera.PixelResolution();
  if (fabs(res) < Epsilon) res = Epsilon;

  md = MosData();  // Nullifies the data
  if (myemission.isValid()) {
    // Compute MORPHOLOGY
    double cose = cos(myemission.radians());
    if (fabs(cose) < Epsilon) cose = Epsilon;
    // Convert resolution to units of KM
    md.m_morph = (res / 1000.0) / cose;

    if (myincidence.isValid()) {
      // Compute ALBEDO
      double cosi = cos(myincidence.radians());
      if (fabs(cosi) < Epsilon) cosi = Epsilon;
      //  Convert resolution to KM
      md.m_albedo = (res / 1000.0 ) * ( (1.0 / cose) + (1.0 / cosi) );
    }
  }

  return (&md);
}


//  Updates existing BandBin keywords with additional values to ensure
//  label compilancy (which should support Camera models).  It checks for the
//  existance of the keyword and uses its (assumed) first value to set nvals
//  values to a constant.  If the keyword doesn't exist, it uses the default
//  value.
void UpdateBandKey(const QString &keyname, PvlGroup &bb, const int &nvals,
                   const QString &default_value) {

  QString defVal(default_value);
  if ( bb.HasKeyword(keyname) ) {
    defVal = bb[keyname][0];
  }

  bb.AddKeyword(makeKey(keyname, nvals, defVal), PvlContainer::Replace);
  return;
}

