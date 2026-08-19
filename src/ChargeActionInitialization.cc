/***********************************************************************\
 * This software is licensed under the terms of the GNU General Public *
 * License version 3 or later. See G4CMP/LICENSE for the full license. *
\***********************************************************************/

#include "ChargeActionInitialization.hh"
#include "ChargePrimaryGeneratorAction.hh"
#include "ChargeRunAction.hh"
#include "ChargeSteppingAction.hh"
#include "G4CMPStackingAction.hh"

void ChargeActionInitialization::BuildForMaster() const {
  SetUserAction(new ChargeRunAction);
}

void ChargeActionInitialization::Build() const {
  SetUserAction(new ChargeRunAction);
  SetUserAction(new ChargePrimaryGeneratorAction);
  SetUserAction(new G4CMPStackingAction);
  SetUserAction(new ChargeSteppingAction);
}
