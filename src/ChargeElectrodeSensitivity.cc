/***********************************************************************\
 * This software is licensed under the terms of the GNU General Public *
 * License version 3 or later. See G4CMP/LICENSE for the full license. *
\***********************************************************************/

// $Id$
//
// 20170816  Output file name moved to example-specific configuration
// 20170830  Remove FET simulation

#include "ChargeElectrodeSensitivity.hh"
#include "ChargeConfigManager.hh"
#include "G4CMPUtils.hh"
#include "G4Event.hh"
#include "G4RunManager.hh"
#include "G4Run.hh"
#include "G4HCofThisEvent.hh"
#include "G4SDManager.hh"
#include "G4SystemOfUnits.hh"

#include "G4CMPDriftElectron.hh"
#include "G4CMPDriftHole.hh"

#include "G4PhononLong.hh"
#include "G4PhononTransFast.hh"
#include "G4PhononTransSlow.hh"
#include "G4Threading.hh"
#include <cmath>
#include <sstream>
#include <fstream>

namespace {
constexpr G4int kPatchGridHalfIndex = 19;
constexpr G4int kPatchesPerAxis = 2 * kPatchGridHalfIndex + 1;
constexpr G4double kPatchSpacing = 200.*um;
constexpr G4double kAlGapEnergy = 191.e-6*eV;

constexpr G4int PatchCopyNumber(G4int ix, G4int iy) {
  return (ix + kPatchGridHalfIndex) * kPatchesPerAxis
         + (iy + kPatchGridHalfIndex);
}

// Figure 8 patches at (400, 200) um and (3800, 200) um.
constexpr G4int kNearPatchCopyNumber = PatchCopyNumber(2, 1) - 1;
constexpr G4int kFarPatchCopyNumber = PatchCopyNumber(19, 1) - 1;
}

ChargeElectrodeSensitivity::ChargeElectrodeSensitivity(G4String name) :
  G4CMPElectrodeSensitivity(name), 
  baseFileName(""),
  fileName("") {
  SetOutputFile(ChargeConfigManager::GetHitOutput());
}

ChargeElectrodeSensitivity::~ChargeElectrodeSensitivity() {
  if (output.is_open()) output.close();
  if (!output.good()) {
    G4cerr << "Error closing output file, " << fileName << ".\n"
           << "Expect bad things like loss of data.";
  }
}

void ChargeElectrodeSensitivity::EndOfEvent(G4HCofThisEvent* HCE) {
  G4int HCID = G4SDManager::GetSDMpointer()->GetCollectionID(hitsCollection);
  auto* hitCol = static_cast<G4CMPElectrodeHitsCollection*>(HCE->GetHC(HCID));
  std::vector<G4CMPElectrodeHit*>* hitVec = hitCol->GetVector();

  G4RunManager* runMan = G4RunManager::GetRunManager();

  if (output.good()) {
    for (G4CMPElectrodeHit* hit : *hitVec) {
      // The Kaplan film cascade is instantaneous.  Its energy deposition is
      // N_qp*Delta, so the end time of the absorbed incident phonon is the QP
      // creation time for every QP represented by this row.
      const G4ThreeVector& position = hit->GetFinalPosition();
      const G4int patchIX = static_cast<G4int>(
          std::lround(position.x()/kPatchSpacing));
      const G4int patchIY = static_cast<G4int>(
          std::lround(position.y()/kPatchSpacing));
      const G4int patchCopyNumber =
          (patchIX + kPatchGridHalfIndex) * kPatchesPerAxis
          + (patchIY + kPatchGridHalfIndex);
      const G4double qpCount = hit->GetEnergyDeposit()/kAlGapEnergy;

      output << runMan->GetCurrentRun()->GetRunID() << ','
             << runMan->GetCurrentEvent()->GetEventID() << ','
             << hit->GetFinalTime()/ns << ','
             << patchCopyNumber << ','
             << patchIX << ','
             << patchIY << ','
             << patchIX*kPatchSpacing/um << ','
             << patchIY*kPatchSpacing/um << ','
             << qpCount << '\n';
    }
  }
}

// Open the CSV file used by this sensitive detector to record hits.
//
// In a multithreaded run, each worker owns a separate sensitive-detector
// instance.  Giving every worker its own file prevents simultaneous writes to
// one std::ofstream.  For example, a configured name of "charge_hits.txt"
// becomes "charge_hits_0.txt" for worker 0, "charge_hits_1.txt" for worker 1,
// and so on.  A serial run writes to "charge_hits_serial.txt".
//
// Each executable run starts fresh worker files.  This prevents a completed
// run from being mixed with rows left by an earlier interrupted run.
void ChargeElectrodeSensitivity::SetOutputFile(const G4String& fn) {
  // Keep the current stream when it is already open for this configured name.
  if (baseFileName == fn && output.is_open()) return;

  // Close the previous stream before changing the destination file.
  if (output.is_open()) output.close();

  baseFileName = fn;

  // Worker IDs are non-negative.  Geant4 returns a negative ID when this code
  // is running without a worker thread, so use a readable "serial" suffix.
  const G4int threadId = G4Threading::G4GetThreadId();
  std::ostringstream suffix;
  if (threadId >= 0) {
    suffix << "_" << threadId;
  } else {
    suffix << "_serial";
  }

  // Insert the worker suffix before the final filename extension.  If the
  // configured name has no extension, append the suffix to the whole name.
  const std::size_t dot = fn.rfind('.');
  if (dot == G4String::npos) {
    fileName = fn + suffix.str();
  } else {
    fileName = fn.substr(0, dot)
             + suffix.str()
             + fn.substr(dot);
  }

  output.open(fileName, std::ios_base::out | std::ios_base::trunc);

  // Stop immediately if this worker cannot create its output file.
  if (!output.good()) {
    G4ExceptionDescription msg;
    msg << "Error opening output file " << fileName;
    G4Exception("ChargeElectrodeSensitivity::SetOutputFile",
                "Charge003", FatalException, msg);
    return;
  }

  output << "Run ID,"
         << "Event ID,"
         << "QP Creation Time [ns],"
         << "Patch Copy Number,"
         << "Patch IX,"
         << "Patch IY,"
         << "Patch Center X [um],"
         << "Patch Center Y [um],"
         << "QPs Created\n";
}

// G4bool ChargeElectrodeSensitivity::IsHit(const G4Step* step,
//                                          const G4TouchableHistory*) const {
//   // Charge carriers do not deposit energy when they land on an electrode.
//   const G4Track* track = step->GetTrack();
//   const G4StepPoint* postStepPoint = step->GetPostStepPoint();
//   const G4ParticleDefinition* particle = track->GetDefinition();

//   G4bool correctParticle = particle == G4CMPDriftElectron::Definition() ||
//                             particle == G4CMPDriftHole::Definition();

//   G4bool correctStatus = step->GetTrack()->GetTrackStatus() == fStopAndKill &&
//                          postStepPoint->GetStepStatus() == fGeomBoundary;

//   return correctParticle && correctStatus;
// }

// This method checks if a phonon hit the electrode.  If so, it returns true, otherwise false.
// A hit is defined as a phonon that is absorbed by the electrode, which is determined by
// checking if the track status is fStopAndKill, the step status is fGeomBoundary,
// and the non-ionizing energy deposit is greater than 0.
G4bool ChargeElectrodeSensitivity::IsHit(const G4Step* step,
                                         const G4TouchableHistory*) const {
  const G4Track* track = step->GetTrack();
  const G4StepPoint* post = step->GetPostStepPoint();
  const G4ParticleDefinition* particle = track->GetDefinition();

  const G4bool isPhonon =
      particle == G4PhononLong::Definition() ||
      particle == G4PhononTransFast::Definition() ||
      particle == G4PhononTransSlow::Definition();

  const G4VPhysicalVolume* volume = post->GetPhysicalVolume();
  const G4bool hitsTopAluminum =
      volume && volume->GetName() == "alPatchPhysical";
  const G4bool hitsScoredPatch =
      hitsTopAluminum &&
      (volume->GetCopyNo() == kNearPatchCopyNumber ||
       volume->GetCopyNo() == kFarPatchCopyNumber);

  // Positive non-ionizing deposition distinguishes absorption from phonons
  // that are killed while being replaced/reflected at a surface.
  const G4bool isAbsorbed =
      track->GetTrackStatus() == fStopAndKill &&
      post->GetStepStatus() == fGeomBoundary &&
      step->GetNonIonizingEnergyDeposit() > 0.;

  return isPhonon && hitsScoredPatch && isAbsorbed;
}
