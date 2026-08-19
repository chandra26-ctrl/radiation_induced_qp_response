/***********************************************************************\
 * This software is licensed under the terms of the GNU General Public *
 * License version 3 or later. See G4CMP/LICENSE for the full license. *
\***********************************************************************/

#ifndef ChargeRunAction_hh
#define ChargeRunAction_hh 1

#include "G4UserRunAction.hh"

class G4Run;

// Writes a small master-thread manifest so analysis can distinguish a
// completed run from an interrupted or partially written hit file.
class ChargeRunAction final : public G4UserRunAction {
public:
  ChargeRunAction() = default;
  ~ChargeRunAction() override = default;

  void BeginOfRunAction(const G4Run* run) override;
  void EndOfRunAction(const G4Run* run) override;
};

#endif
