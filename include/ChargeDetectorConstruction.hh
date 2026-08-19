/***********************************************************************\
 * This software is licensed under the terms of the GNU General Public *
 * License version 3 or later. See G4CMP/LICENSE for the full license. *
\***********************************************************************/

// $Id: 165a1df2a9364e285f19b48f5c016259faee053b $
//
// 20160904  Add electrode pattern to surface configuration
// 20170721  Surface property owns electrode pattern, deletes at end

#ifndef ChargeDetectorConstruction_h
#define ChargeDetectorConstruction_h 1

#include "G4SystemOfUnits.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4Threading.hh"

class ChargeElectrodeSensitivity;
class G4CMPSurfaceProperty;
class G4LatticeManager;
class G4Material;
class G4VPhysicalVolume;
class G4VUserDetectorConstruction;
class G4ElectricField;

class G4FieldManager;
class G4LogicalVolume;


class ChargeDetectorConstruction : public G4VUserDetectorConstruction {
public:
  ChargeDetectorConstruction();
  virtual ~ChargeDetectorConstruction() override;

  virtual G4VPhysicalVolume* Construct() override;
  void ConstructSDandField() override;
     
private:
  void DefineMaterials();
  void SetupGeometry();
  // void AttachField(G4LogicalVolume* lv);
  void AttachLattice(G4VPhysicalVolume* pv);
  // void AttachSensitivity(G4LogicalVolume* lv);
  void ConstructBacksideCopper(G4LogicalVolume* worldLogical,
                               G4VPhysicalVolume* siliconPhysical);

private:
  G4LogicalVolume* siliconLogical;

  static G4ThreadLocal G4ElectricField* fEMField;
  static G4ThreadLocal G4FieldManager* fFieldManager;
  // ChargeElectrodeSensitivity* sensitivity;
  G4CMPSurfaceProperty* topSurfProp;
  G4CMPSurfaceProperty* botSurfProp;
  G4CMPSurfaceProperty* wallSurfProp;
  G4CMPSurfaceProperty* alSurfProp;
  G4CMPSurfaceProperty* cuSurfProp;
  G4LatticeManager* latManager;
  // G4ElectricField* fEMField;
  G4Material* liquidHelium;
  G4Material* silicon;
  G4Material* aluminum;
  G4Material* tungsten;
  G4Material* niobium;
  G4Material* copper;
  G4VPhysicalVolume* worldPhys;
  const G4double substrateWidth = 8.*mm;
  const G4double substrateThickness = 525.*um;
  // G4double epotScale;
  // G4double voltage;
  // G4bool constructed;
  // G4String epotFileName;
  // G4String outputFileName;
  G4bool constructed;
};

#endif
