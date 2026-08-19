/***********************************************************************\
 * This software is licensed under the terms of the GNU General Public *
 * License version 3 or later. See G4CMP/LICENSE for the full license. *
\***********************************************************************/

// $Id: be4e879b33241dd90f04560177057fb1aecebf27 $
//
// 20160904  Add electrode pattern to surface configuration
// 20170721  Surface property owns electrode pattern, deletes at end
// 20170816  Field configuration parameters moved to local configuration
// 20211207  Replace G4Logical*Surface with G4CMP-specific versions.
// 20251117 G4CMP-541 -- For G4 v11, replace ::Invisible w/::GetInvisible()

#include "ChargeDetectorConstruction.hh"
#include "ChargeConfigManager.hh"
#include "ChargeElectrodeSensitivity.hh"
#include "G4CMPSurfaceProperty.hh"
#include "G4CMPLogicalBorderSurface.hh"
#include "G4CMPFieldManager.hh"
#include "G4CMPMeshElectricField.hh"
#include "G4Box.hh"
#include "G4Colour.hh"
#include "G4LatticeManager.hh"
#include "G4LatticePhysical.hh"
#include "G4LogicalVolume.hh"
#include "G4Material.hh"
#include "G4NistManager.hh"
#include "G4PVPlacement.hh"
#include "G4RunManager.hh"
#include "G4SDManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4VisAttributes.hh"
#include "G4GeometryManager.hh"
#include "G4PhysicalVolumeStore.hh"
#include "G4LogicalVolumeStore.hh"
#include "G4SolidStore.hh"
#include "G4UniformElectricField.hh"
#include "G4CMPPhononElectrode.hh"
#include "G4CMPUtils.hh"
#include "G4MultiUnion.hh"
#include "G4SubtractionSolid.hh"
#include "G4Transform3D.hh"
#include "G4RotationMatrix.hh"

G4ThreadLocal G4ElectricField*
ChargeDetectorConstruction::fEMField = nullptr;

G4ThreadLocal G4FieldManager*
ChargeDetectorConstruction::fFieldManager = nullptr;

// ChargeDetectorConstruction::ChargeDetectorConstruction() :

//   sensitivity(nullptr),  topSurfProp(nullptr),
//   botSurfProp(nullptr), wallSurfProp(nullptr),
//   latManager(G4LatticeManager::GetLatticeManager()),
//   fEMField(nullptr), liquidHelium(nullptr), silicon(nullptr),
//   aluminum(nullptr), tungsten(nullptr), worldPhys(nullptr),
//   epotScale(0.), voltage(0.), constructed(false),
//   epotFileName(""), outputFileName("")
// {
//   /* Default initialization does not leave object in usable state.
//    * Doesn't matter because run initialization will call Construct() and all
//    * will be well.
//    */
// }
ChargeDetectorConstruction::ChargeDetectorConstruction()
  : siliconLogical(nullptr),
    topSurfProp(nullptr),
    botSurfProp(nullptr),
    wallSurfProp(nullptr),
    alSurfProp(nullptr),
    cuSurfProp(nullptr),
    latManager(G4LatticeManager::GetLatticeManager()),
    liquidHelium(nullptr),
    silicon(nullptr),
    aluminum(nullptr),
    tungsten(nullptr),
    niobium(nullptr),
    copper(nullptr),
    worldPhys(nullptr),
    constructed(false)
{}

ChargeDetectorConstruction::~ChargeDetectorConstruction()
{
  // delete fEMField;
  delete topSurfProp;
  delete botSurfProp;
  delete wallSurfProp;
  delete alSurfProp;
  delete cuSurfProp;
}

G4VPhysicalVolume* ChargeDetectorConstruction::Construct()
{
  if (constructed) {
    if (!G4RunManager::IfGeometryHasBeenDestroyed()) {
      // Run manager hasn't cleaned volume stores. This code shouldn't execute
      G4GeometryManager::GetInstance()->OpenGeometry();
      G4PhysicalVolumeStore::GetInstance()->Clean();
      G4LogicalVolumeStore::GetInstance()->Clean();
      G4SolidStore::GetInstance()->Clean();
    }

    // Only regenerate E field if it has changed since last construction.
    // if (epotFileName != ChargeConfigManager::GetEPotFile() ||
    //     epotScale != ChargeConfigManager::GetEPotScale() ||
    //     voltage != ChargeConfigManager::GetVoltage()) {
    //    delete fEMField; fEMField = nullptr;
    // }

    // Sensitivity doesn't need to ever be deleted, just updated.
    // if (outputFileName != ChargeConfigManager::GetHitOutput()) {
    //   outputFileName = ChargeConfigManager::GetHitOutput();
    //   if (sensitivity) sensitivity->SetOutputFile(outputFileName);
    // }

    // Have to completely remove all lattices to avoid warning on reconstruction
    latManager->Reset();
    // Clear all LogicalSurfaces; no need to redfine SurfaceProperty
    G4CMPLogicalBorderSurface::CleanSurfaceTable();
  }


  // Store current values in order to identify changes above
  // voltage = ChargeConfigManager::GetVoltage();
  // epotScale = ChargeConfigManager::GetEPotScale();
  // epotFileName = ChargeConfigManager::GetEPotFile();
  // outputFileName = ChargeConfigManager::GetHitOutput();

  DefineMaterials();
  SetupGeometry();

  constructed = true;
  return worldPhys;
}

void ChargeDetectorConstruction::DefineMaterials() { 
  G4NistManager* nistManager = G4NistManager::Instance();

  liquidHelium = nistManager->FindOrBuildMaterial("G4_AIR"); //FIXME
  silicon = nistManager->FindOrBuildMaterial("G4_Si");
  aluminum = nistManager->FindOrBuildMaterial("G4_Al");
  tungsten = nistManager->FindOrBuildMaterial("G4_W");
  niobium = nistManager->FindOrBuildMaterial("G4_Nb");
  copper = nistManager->FindOrBuildMaterial("G4_Cu");

  // Attach lattice information for silicon
  latManager->LoadLattice(silicon, "Si");
}

void ChargeDetectorConstruction::SetupGeometry()
{
  // World
  G4VSolid* worldSolid = new G4Box("World", 16.*cm, 16.*cm, 16.*cm);
  G4LogicalVolume* worldLogical = new G4LogicalVolume(worldSolid,
                                                      liquidHelium,
                                                      "World");
  worldPhys = new G4PVPlacement(0,
                                G4ThreeVector(),
                                worldLogical,
                                "World",
                                0,
                                false,
                                0);

  // 8 mm x 8 mm x 525 um silicon substrate. G4Box takes half-lengths.
  G4VSolid* siliconSolid = new G4Box("siliconSolid",
                                     substrateWidth/2.,
                                     substrateWidth/2.,
                                     substrateThickness/2.);
  // G4LogicalVolume* siliconLogical = new G4LogicalVolume(siliconSolid, silicon,
  //                                                       "siliconLogical");
  siliconLogical =
    new G4LogicalVolume(siliconSolid, silicon, "siliconLogical");
  G4VPhysicalVolume* siliconPhysical = new G4PVPlacement(0, G4ThreeVector(),
                                                         siliconLogical,
                                                         "siliconPhysical",
                                                         worldLogical,
                                                         false, 0);

  // G4CMP border surfaces are selected by the pair of physical volumes, not
  // by an individual face of a volume.  Put four thin, non-transporting tag
  // volumes against the vertical Si faces so their loss model can be kept
  // separate from the bare backside.  The tag thickness has no physical role;
  // the Si-side surface property below always absorbs or reflects the phonon.
  // Keep the tags and top film slightly apart at edges and corners.  Exact
  // three-volume junctions are ambiguous to the Geant4 navigator and can make
  // a reflected Si phonon appear to step directly from the film into a tag.
  const G4double edgeClearance = 10.*nm;
  const G4double wallTagThickness = 1.*mm;
  G4VSolid* xWallTagSolid = new G4Box("xWallTagSolid",
                                      wallTagThickness/2.,
                                      substrateWidth/2. - edgeClearance,
                                      substrateThickness/2. - edgeClearance);
  G4LogicalVolume* xWallTagLogical = new G4LogicalVolume(
      xWallTagSolid, liquidHelium, "xWallTagLogical");

  G4VSolid* yWallTagSolid = new G4Box("yWallTagSolid",
                                      substrateWidth/2. - edgeClearance,
                                      wallTagThickness/2.,
                                      substrateThickness/2. - edgeClearance);
  G4LogicalVolume* yWallTagLogical = new G4LogicalVolume(
      yWallTagSolid, liquidHelium, "yWallTagLogical");

  const G4double wallTagOffset = substrateWidth/2. + wallTagThickness/2.;
  G4VPhysicalVolume* xPlusWallPhysical = new G4PVPlacement(
      nullptr, G4ThreeVector(wallTagOffset, 0., 0.), xWallTagLogical,
      "xPlusWallPhysical", worldLogical, false, 0, true);
  G4VPhysicalVolume* xMinusWallPhysical = new G4PVPlacement(
      nullptr, G4ThreeVector(-wallTagOffset, 0., 0.), xWallTagLogical,
      "xMinusWallPhysical", worldLogical, false, 1, true);
  G4VPhysicalVolume* yPlusWallPhysical = new G4PVPlacement(
      nullptr, G4ThreeVector(0., wallTagOffset, 0.), yWallTagLogical,
      "yPlusWallPhysical", worldLogical, false, 0, true);
  G4VPhysicalVolume* yMinusWallPhysical = new G4PVPlacement(
      nullptr, G4ThreeVector(0., -wallTagOffset, 0.), yWallTagLogical,
      "yMinusWallPhysical", worldLogical, false, 1, true);

  // Attach E field to silicon (logical volume, so all placements)
  // AttachField(siliconLogical);

  // Physical lattice for each placed detector
  AttachLattice(siliconPhysical);

  // Aluminum
  // const G4double electrodeHalfThickness = 0.01*cm;
  // const G4double electrodeZ = substrateThickness/2. + electrodeHalfThickness;
  // G4VSolid* aluminumSolid = new G4Box("aluminumSolid",
  //                                     substrateWidth/2.,
  //                                     substrateWidth/2.,
  //                                     electrodeHalfThickness);
  // G4LogicalVolume* aluminumLogical = new G4LogicalVolume(aluminumSolid,
  //                                                        aluminum,
  //                                                        "aluminumLogical");

  // G4VPhysicalVolume* aluminumTopPhys = new G4PVPlacement(
  //                                              0,
  //                                              G4ThreeVector(0.,0.,electrodeZ),
  //                                              aluminumLogical,
  //                                              "topAluminumPhysical",
  //                                               worldLogical,
  //                                              false,
  //                                              0);

  // geometry top niobium film
  const G4double nbThickness = 120*nm;
  const G4double nbHalfThickness = nbThickness/2.;
  const G4double nbZ = substrateThickness/2. + nbHalfThickness;

  // geometry of the aluminum patches
  const G4double alPatchWidth = 10*um;
  const G4double alPatchThickness = nbThickness;
  const G4double patchSpacing = 200*um;

  // create an unperforated niobium plate
  G4VSolid* niobiumPlate = new G4Box("niobiumPlate",
                                      substrateWidth/2. - edgeClearance,
                                      substrateWidth/2. - edgeClearance,
                                      nbHalfThickness);

  // create a single opening in the niobium plate for the aluminum patch
  G4VSolid* patchOpening = new G4Box(
    "alPatchOpening",
    alPatchWidth / 2.,
    alPatchWidth / 2.,
    nbThickness);

  // create a multi-union of all the openings in the niobium plate
  G4MultiUnion* allPatchOpenings = new G4MultiUnion("allPatchOpenings");

  // add all the openings to the multi-union
  for (G4int ix = -19; ix <= 19; ++ix) {
    for (G4int iy = -19; iy <= 19; ++iy) {
      const G4double x = ix * patchSpacing;
      const G4double y = iy * patchSpacing;

      if (x == 0. && y == 0.) continue; // Skip the center patch, which is covered by Nb

      allPatchOpenings -> AddNode(
        *patchOpening,
        G4Transform3D(
          G4RotationMatrix(),
          G4ThreeVector(x, y, 0.))
      );
    }
  }

  // voxelize the multi-union to create a single solid representing all the openings
  allPatchOpenings->Voxelize();

  // subtract the multi-union of openings from the niobium plate to create the final niobium solid
  G4VSolid* niobiumSolid = new G4SubtractionSolid(
    "niobiumSolid",
    niobiumPlate,
    allPatchOpenings);
  
  G4LogicalVolume* niobiumLogical = new G4LogicalVolume(niobiumSolid,
                                                         niobium,
                                                         "niobiumLogical");
  G4PVPlacement* niobiumTopPhys = new G4PVPlacement(
                                               0,
                                               G4ThreeVector(0.,0.,nbZ),
                                               niobiumLogical,
                                               "topNiobiumPhysical",
                                                worldLogical,
                                               false,
                                               0,
                                               true);

  G4VSolid* alPatchSolid = new G4Box("alPatchSolid",
                                     alPatchWidth/2.,
                                     alPatchWidth/2.,
                                     alPatchThickness/2.);
  G4LogicalVolume* alPatchLogical = new G4LogicalVolume(alPatchSolid,
                                                        aluminum,
                                                        "alPatchLogical");
  const G4double alPatchZ = substrateThickness/2. + alPatchThickness/2.;
  G4int copyNumber = 0;

  // Define surface properties. Only should be done once
  if (!constructed) {
    topSurfProp = new G4CMPSurfaceProperty("SiNbProp",
                                           1., 0, 0., 0.,
                                           0, 1, 0., 0.);

    auto* nbFilmProperties =
      topSurfProp->GetPhononMaterialPropertiesTablePointer();

    // Probability that an incident Si phonon transmits into the Nb film.
    G4CMP::UpdateMPT(
        nbFilmProperties, "filmAbsorption", 0.745);

    // Nb film parameters used by the Kaplan cascade.
    G4CMP::UpdateMPT(
        nbFilmProperties, "filmThickness", nbThickness);

    // Table I: ΔNb = 1538 µeV
    G4CMP::UpdateMPT(
        nbFilmProperties, "gapEnergy", 1538.e-6*eV);

    // Table I: τ0ph = 0.00417 ns
    G4CMP::UpdateMPT(
        nbFilmProperties, "phononLifetime", 0.00417*ns);
    // Equivalent to 4.17*ps

    // Table I: νs = 2.44 µm/ns
    G4CMP::UpdateMPT(
        nbFilmProperties, "vSound", 2.44*um/ns);

    // Appendix A pair-breaking-rate expression
    G4CMP::UpdateMPT(
        nbFilmProperties, "phononLifetimeSlope", 0.29);

    // Appendix A: stop explicitly evolving QPs below 3Δ
    G4CMP::UpdateMPT(
        nbFilmProperties, "lowQPLimit", 3.0);

    alSurfProp = new G4CMPSurfaceProperty(
    "SiAlProp",
    1., 0., 0., 0.,     // charge absorption
    0., 1., 0., 0.);    // phonons handled by the electrode

    // obtain the phonon material properties table for the aluminum surface property
    auto* alFilmProperties =
        alSurfProp->GetPhononMaterialPropertiesTablePointer();

    // Table I
    G4CMP::UpdateMPT(
        alFilmProperties, "filmAbsorption", 0.795);

    // Sec II
    G4CMP::UpdateMPT(
        alFilmProperties, "filmThickness", alPatchThickness);

    // Sec II
    G4CMP::UpdateMPT(
        alFilmProperties, "gapEnergy", 191.e-6*eV);

    // Appendix B, Table I
    G4CMP::UpdateMPT(
        alFilmProperties, "phononLifetime", 0.242*ns);

    // Appendix A
    G4CMP::UpdateMPT(
        alFilmProperties, "phononLifetimeSlope", 0.29);

    // Appendix B, Table I  
    G4CMP::UpdateMPT(
        alFilmProperties, "vSound", 3.58*um/ns);

    // Appendix A: "Film boundaries"
    G4CMP::UpdateMPT(
        alFilmProperties, "lowQPLimit", 3.0);

    // attach electrode behaviour to the aluminum surface property. This will handle phonons that hit the aluminum patches.
    alSurfProp->SetPhononElectrode(
        new G4CMPPhononElectrode);

    // attach electrode behaviour to the top surface property. This will handle phonons that hit the niobium film.
    topSurfProp->SetPhononElectrode(
      new G4CMPPhononElectrode);

    // Paper model: a phonon is lost with probability 2.5% whenever it
    // encounters one of the four vertical Si walls.  reflProb is conditional
    // on absorption having failed, so 1.0 makes the remaining 97.5% reflect.
    // pSpec=0 gives the diffuse reflection used in the paper.
    wallSurfProp = new G4CMPSurfaceProperty("verticalWallSurfProp",
                                            1., 0., 0., 0.,
                                            0.025, 1., 0., 0.);

    // The bare backside is diffuse and lossless in the no-Cu configuration.
    botSurfProp = new G4CMPSurfaceProperty("backsideSurfProp",
                                           1., 0., 0., 0.,
                                           0., 1., 0., 0.);

    
  }

  // Figure 8 backside: bare Si (lossless) perforated by 10-um Cu islands.
  // Must run after botSurfProp (above) is defined.
  ConstructBacksideCopper(worldLogical, siliconPhysical);

    for (G4int ix = -19; ix <= 19; ++ix) {
    for (G4int iy = -19; iy <= 19; ++iy) {
      const G4double x = ix * patchSpacing;
      const G4double y = iy * patchSpacing;

      if (x == 0. && y == 0.) continue; // Skip the center patch, which is covered by Nb

      G4PVPlacement* alPatchPhys = new G4PVPlacement(
        0,
        G4ThreeVector(x, y, alPatchZ),
        alPatchLogical,
        "alPatchPhysical",
        worldLogical,
        false,
        copyNumber,
        true);
        
      G4String surfaceName = "SiToAl_";
      surfaceName += std::to_string(copyNumber);

      new G4CMPLogicalBorderSurface(surfaceName,
                                   siliconPhysical,
                                   alPatchPhys,
                                   alSurfProp);
      ++copyNumber;
    }
  }

  // Add the top-film, vertical-wall, and bare-backside surfaces.
  new G4CMPLogicalBorderSurface("SiToNb", siliconPhysical, niobiumTopPhys,
                             topSurfProp);

  new G4CMPLogicalBorderSurface("SiToXPlusWall", siliconPhysical,
                                xPlusWallPhysical, wallSurfProp);
  new G4CMPLogicalBorderSurface("SiToXMinusWall", siliconPhysical,
                                xMinusWallPhysical, wallSurfProp);
  new G4CMPLogicalBorderSurface("SiToYPlusWall", siliconPhysical,
                                yPlusWallPhysical, wallSurfProp);
  new G4CMPLogicalBorderSurface("SiToYMinusWall", siliconPhysical,
                                yMinusWallPhysical, wallSurfProp);

  new G4CMPLogicalBorderSurface("SiToWorldFallback", siliconPhysical,
                                worldPhys, botSurfProp);

  // Detector -- aluminum electrode sensitivity is attached to silicon.
  // AttachSensitivity(siliconLogical);

  // Visualization attributes
  worldLogical->SetVisAttributes(G4VisAttributes::GetInvisible());
  xWallTagLogical->SetVisAttributes(G4VisAttributes::GetInvisible());
  yWallTagLogical->SetVisAttributes(G4VisAttributes::GetInvisible());
  // Silicon: white
  G4VisAttributes* siliconVisAtt =
    new G4VisAttributes(G4Colour(1.0, 1.0, 1.0));

  siliconVisAtt->SetVisibility(true);
  siliconLogical->SetVisAttributes(siliconVisAtt);

  // niobium: purple
  G4VisAttributes* niobiumVisAtt =
    new G4VisAttributes(G4Colour(0.5, 0.0, 0.5));

  niobiumVisAtt->SetVisibility(true);
  niobiumLogical->SetVisAttributes(niobiumVisAtt);

  // aluminum: blue
  G4VisAttributes* alPatchVisAtt =
    new G4VisAttributes(G4Colour(0.0, 0.0, 1.0));

  alPatchVisAtt->SetVisibility(true);
  alPatchLogical->SetVisAttributes(alPatchVisAtt);
}

void ChargeDetectorConstruction::ConstructSDandField()
{
  // This method runs once for each worker thread.

  // Create a worker-local sensitive detector.
  auto* sensitivity =
      new ChargeElectrodeSensitivity("ChargeElectrode");

  G4SDManager::GetSDMpointer()->AddNewDetector(sensitivity);
  SetSensitiveDetector(siliconLogical, sensitivity);

  // Create a worker-local electric field.
  const G4String& epotFile = ChargeConfigManager::GetEPotFile();

  // A zero-valued field still makes Geant4 invoke G4ChordFinder.  Charge
  // carriers which come to rest can then enter the field equation with zero
  // momentum, producing NaNs before the recombination process can kill the
  // track.  There is no trajectory to integrate when neither a field map nor
  // a bias voltage was configured, so leave the volume field-free.
  if (epotFile.empty() && ChargeConfigManager::GetVoltage() == 0.) {
    fEMField = nullptr;
    fFieldManager = nullptr;
    siliconLogical->SetFieldManager(nullptr, true);
    return;
  }

  if (!epotFile.empty()) {
    fEMField = new G4CMPMeshElectricField(
        epotFile,
        ChargeConfigManager::GetEPotScale());
  } else {
    const G4double fieldMagnitude =
        -ChargeConfigManager::GetVoltage() / substrateThickness;

    fEMField = new G4UniformElectricField(
        fieldMagnitude * G4ThreeVector(0., 0., 1.));
  }

  // Create a worker-local G4CMP field manager.
  fFieldManager = new G4CMPFieldManager(fEMField);

  constexpr G4bool forceToAllDaughters = true;
  siliconLogical->SetFieldManager(
      fFieldManager,
      forceToAllDaughters);
}

// void ChargeDetectorConstruction::AttachField(G4LogicalVolume* lv)
// {
//   if (!fEMField) { // Only create field if one doesn't exist.
//     if (!epotFileName.empty()) {
//       fEMField = new G4CMPMeshElectricField(epotFileName, epotScale);
//     } else {
//       G4double fieldMag = -voltage/substrateThickness;
//       fEMField = new G4UniformElectricField(fieldMag*G4ThreeVector(0., 0., 1.));
//     }
//   }

//   // Ensure that logical volume has a field manager attached
//   if (!lv->GetFieldManager()) { // Should always run
//     G4FieldManager* fFieldMgr = new G4CMPFieldManager(fEMField);
//     lv->SetFieldManager(fFieldMgr, true);
//   }

//   lv->GetFieldManager()->SetDetectorField(fEMField);
// }

void ChargeDetectorConstruction::AttachLattice(G4VPhysicalVolume* pv)
{
  G4LatticePhysical* detLattice =
    new G4LatticePhysical(latManager->GetLattice(silicon));
  detLattice->SetMillerOrientation(1,0,0,45.*deg);	// Flats at [110]
  latManager->RegisterLattice(pv, detLattice);
}

// Private helper method used to construct the backside copper island pattern on the detector.
// This method works by creating a multi-union of copper islands and subtracting it from a solid
// representing the backside of the silicon detector. It then creates logical volumes for both
// the bare backside and the copper islands, and places them in the world volume. Additionally,
// it sets up surface properties for the copper islands to handle phonon interactions appropriately.
// Parameters for the method:
// - worldLogical: the logical volume representing the world in which the detector resides.
// - siliconPhysical: the physical volume of the silicon detector to which the backside copper
//   pattern will be attached.
void ChargeDetectorConstruction::ConstructBacksideCopper(
    G4LogicalVolume* worldLogical, G4VPhysicalVolume* siliconPhysical)
{
  const G4double wallTagThickness = 1.*mm;   // placeholder for "bare backside" thickness
  const G4double cuIslandWidth = 200.*um;   // Width of each copper island
  const G4double cuIslandPitch = 250.*um;    // Center-to-center distance between islands
  const G4double cuThickness = 10.*um;       // Thickness of each copper island
  const G4int islandsPerSide = 31;           // Number of islands along one side of the square pattern
  const G4int half = (islandsPerSide - 1)/2; // Half the number of islands along one side, used for positioning

  // create the footprint which will be subtracted from the backside wall tag
  // to create the bare backside
  G4VSolid* islandFootprint = new G4Box("cuIslandFootprint",
                                        cuIslandWidth/2., cuIslandWidth/2.,
                                        wallTagThickness);
  // create the copper island solid which will be placed on the backside of the silicon
  G4VSolid* cuIslandSolid = new G4Box("cuIslandSolid",
                                      cuIslandWidth/2., cuIslandWidth/2.,
                                      cuThickness/2.);

  // Multi union of the island footprint which is used to create the bare backside pattern
  G4MultiUnion* islandFootprintField = new G4MultiUnion("cuIslandFootprintField");

  // Multi union of the copper islands which will be placed on the backside of the silicon
  G4MultiUnion* cuIslandField = new G4MultiUnion("cuIslandField");

  // This for-loop creates the grid of copper island and their 
  // corresponding footprint for the bare backside.
  for (G4int ix = -half; ix <= half; ++ix) {
    for (G4int iy = -half; iy <= half; ++iy) {
      const G4double x = ix * cuIslandPitch; // x-coordinates of current island
      const G4double y = iy * cuIslandPitch; // y-coordinates of current island

      // Create a placement for the current island footprint and copper island
      const G4Transform3D placement(
        G4RotationMatrix(), // identity rotation, no rotation applied
        G4ThreeVector(x, y, 0.)); // position of the island in the x-y plane, no z-offset applied (handled later)

      // Add the current island footprint and copper island to their respective multi-union fields
      islandFootprintField->AddNode(*islandFootprint, placement);
      cuIslandField->AddNode(*cuIslandSolid, placement);
    }
  }
  // voxelize builds a voxel grid so Geant4 can quickly
  // trace intersections and optimize the geometry for faster navigation.
  islandFootprintField->Voxelize();
  cuIslandField->Voxelize();

  // plane box spanning the backside of the substrate, with wall tag thickness applied
  G4VSolid* backWallTagSolid = new G4Box("backWallTagSolid",
                                         substrateWidth/2., substrateWidth/2.,
                                         wallTagThickness/2.);
  // Takes the backWallTagSolid and subtracks islandFootprintField from it
  // So now this slab has holes where the copper islands will be placed
  G4VSolid* bareBacksideSolid = new G4SubtractionSolid(
      "bareBacksideSolid", backWallTagSolid, islandFootprintField);

  // Create logical volumes for the bare backside and copper islands which
  // is used to define the physical properties and placement of these components in the simulation.
  G4LogicalVolume* bareBacksideLogical = new G4LogicalVolume(
      bareBacksideSolid, liquidHelium, "bareBacksideLogical");
  G4LogicalVolume* cuIslandLogical = new G4LogicalVolume(
      cuIslandField, copper, "cuIslandLogical");

  // Calculate the z-positions for the backside wall tag 
  //and copper islands relative to the substrate center.
  const G4double backWallTagZ = -substrateThickness/2. - wallTagThickness/2.;
  const G4double cuZ = -substrateThickness/2. - cuThickness/2.;

  // Place the bare backside and copper islands in the world volume at the calculated z-positions.
  G4VPhysicalVolume* bareBacksidePhys = new G4PVPlacement(
      nullptr, G4ThreeVector(0., 0., backWallTagZ), bareBacksideLogical,
      "bareBacksidePhysical", worldLogical, false, 0, true);
  G4VPhysicalVolume* cuIslandsPhys = new G4PVPlacement(
      nullptr, G4ThreeVector(0., 0., cuZ), cuIslandLogical,
      "backsideCuPhysical", worldLogical, false, 0, true);

  if (!cuSurfProp) {
    // Table I: p_abs = 0.736 for Cu.  No electrode is attached here, so an
    // "absorbed" phonon is simply dropped -- a stand-in for the
    // electron-phonon down-conversion described in Sec. II / Appendix B2,
    // rather than the full Kaplan cascade used for the Nb/Al QP electrodes.
    cuSurfProp = new G4CMPSurfaceProperty("backsideCuSurfProp",
                                          1., 0., 0., 0.,
                                          0.736, 1., 0., 0.);
  }

  // Create logical border surfaces to define the interface properties between
  // the silicon substrate and the backside components.
  new G4CMPLogicalBorderSurface("SiToBackside", siliconPhysical,
                                bareBacksidePhys, botSurfProp);
  new G4CMPLogicalBorderSurface("SiToBacksideCu", siliconPhysical,
                                cuIslandsPhys, cuSurfProp);

  // Set the visual attributes for the backside components. The bare backside is invisible.
  bareBacksideLogical->SetVisAttributes(G4VisAttributes::GetInvisible());

  // Make the copper island brownish color, enable its visibility in the simulation.
  G4VisAttributes* cuVisAtt = new G4VisAttributes(G4Colour(0.85, 0.45, 0.2));
  cuVisAtt->SetVisibility(true);
  cuIslandLogical->SetVisAttributes(cuVisAtt);
}

// void ChargeDetectorConstruction::AttachSensitivity(G4LogicalVolume *lv)
// {
//   if (!sensitivity) { // Only create detector if one doesn't exist.
//     // NOTE: ChargeElectrodeSensitivity's ctor will call SetOutputFile()
//     sensitivity = new ChargeElectrodeSensitivity("ChargeElectrode");
//   }
//   G4SDManager* SDman = G4SDManager::GetSDMpointer();
//   SDman->AddNewDetector(sensitivity);
//   lv->SetSensitiveDetector(sensitivity);
// }
