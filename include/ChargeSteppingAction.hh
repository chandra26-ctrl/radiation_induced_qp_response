/***********************************************************************\
 * This software is licensed under the terms of the GNU General Public *
 * License version 3 or later. See G4CMP/LICENSE for the full license. *
\***********************************************************************/

#ifndef ChargeSteppingAction_hh
#define ChargeSteppingAction_hh 1

#include "G4UserSteppingAction.hh"

class G4Step;

class ChargeSteppingAction : public G4UserSteppingAction {
public:
  void UserSteppingAction(const G4Step* step) override;
};

#endif
