/***********************************************************************\
 * This software is licensed under the terms of the GNU General Public *
 * License version 3 or later. See G4CMP/LICENSE for the full license. *
\***********************************************************************/

#include "ChargeSteppingAction.hh"

#include "G4CMPUtils.hh"
#include "G4Exception.hh"
#include "G4Step.hh"
#include "G4Track.hh"

#include <cmath>

namespace {
G4bool IsFinite(const G4ThreeVector& value) {
  return std::isfinite(value.x()) && std::isfinite(value.y()) &&
         std::isfinite(value.z());
}
}

void ChargeSteppingAction::UserSteppingAction(const G4Step* step) {
  G4Track* track = step->GetTrack();

  // A stopped carrier should normally be handled by G4CMP's limiter or
  // recombination process.  Guard only a track which would otherwise enter
  // another process-selection cycle with invalid kinematics.  In particular,
  // a NaN velocity can become a NaN intervalley-scattering interaction length
  // and cause Geant4 to abort the complete event with ProcMan201.
  if (track->GetTrackStatus() != fAlive || !G4CMP::IsChargeCarrier(*track)) {
    return;
  }

  const G4double energy = track->GetKineticEnergy();
  const G4ThreeVector momentum = track->GetMomentum();
  const G4ThreeVector direction = track->GetMomentumDirection();

  // Zero energy is a valid end state.  Kill it before another GPIL cycle can
  // ask a scattering process to calculate a mean free path from zero/NaN
  // velocity.  The direction may already be NaN because G4CMP normalized the
  // zero momentum, so do this check first and do not report it as corruption.
  if (std::isfinite(energy) && energy <= 0.) {
    track->SetTrackStatus(fStopAndKill);
    return;
  }

  if (!std::isfinite(energy) || !IsFinite(momentum) ||
      !IsFinite(direction) || momentum.mag2() <= 0.) {
    G4ExceptionDescription message;
    message << "Discarding invalid charge carrier " << track->GetTrackID()
            << " (" << track->GetParticleDefinition()->GetParticleName()
            << ") with kinetic energy " << energy
            << " and momentum " << momentum << ".";
    G4Exception("ChargeSteppingAction::UserSteppingAction", "ChargeKin001",
                JustWarning, message);
    track->SetTrackStatus(fStopAndKill);
  }
}
