/*
 * Copyright (c) 2019 Opticks Team. All Rights Reserved.
 *
 * This file is part of Opticks
 * (see https://bitbucket.org/simoncblyth/opticks).
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); 
 * you may not use this file except in compliance with the License.  
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software 
 * distributed under the License is distributed on an "AS IS" BASIS, 
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  
 * See the License for the specific language governing permissions and 
 * limitations under the License.
 */

//
// Project Headers
//
#include "G4.hh"
#include "Ctx.hh"
#include "DetectorConstruction.hh"
#include "PrimaryGeneratorAction.hh"
#include "RunAction.hh"
#include "EventAction.hh"
#include "TrackingAction.hh"
#include "SteppingAction.hh"
#include "ConfigurationManager.hh"
//
// Geant4 headers
//
#include "G4RunManager.hh"
#include "G4GeometryManager.hh"
#include "G4VUserPhysicsList.hh"
#include "G4Version.hh"
#include "G4OpAbsorption.hh"
#include "G4OpRayleigh.hh"
#include "G4OpBoundaryProcess.hh"
#include "G4PhysListFactoryAlt.hh" 
#include "G4PhysicsConstructorRegistry.hh"
#include "G4PhysListRegistry.hh"
#include "G4OpticalPhysics.hh"
#include "G4NeutronTrackingCut.hh"
#include "G4VModularPhysicsList.hh"
#include "G4StepLimiter.hh"
#include "G4StepLimiterPhysics.hh"
#include "G4SystemOfUnits.hh"

G4::G4(G4String fname)
:
ctx(new Ctx),
rm(new G4RunManager),

sdn("SD0"),
//sd(new SensitiveDetector(sdn)),
dc(new DetectorConstruction(fname)),


pl(NULL),
ga(NULL),
ra(NULL),
ea(NULL),
ta(NULL),
sa(NULL) {
    //
    // Access to registries and factories
    //
    G4PhysicsConstructorRegistry* g4pcr = G4PhysicsConstructorRegistry::Instance();
    G4PhysListRegistry* g4plr = G4PhysListRegistry::Instance();

    bool verbose= ConfigurationManager::getInstance()->isEnable_verbose();
    if (verbose) {
        G4cout << "Available Physics Constructors:  " << g4pcr->AvailablePhysicsConstructors().size() << G4endl;
        G4cout << "Available Physics Lists:         " << g4plr->AvailablePhysLists().size() << G4endl;
        G4cout << "Available Physics Extensions:    " << g4plr->AvailablePhysicsExtensions().size() << G4endl;
        G4cout << "Available Physics Lists Em:      " << g4plr->AvailablePhysListsEM().size() << G4endl;
        g4plr->SetVerbose(1);
    } else {
        g4plr->SetVerbose(0);
    }

    g4plr->AddPhysicsExtension("OPTICAL", "G4OpticalPhysics");
    g4plr->AddPhysicsExtension("STEPLIMIT", "G4StepLimiterPhysics");
    g4plr->AddPhysicsExtension("NEUTRONLIMIT", "G4NeutronTrackingCut");
    if (verbose) {
        g4pcr->PrintAvailablePhysicsConstructors();
        g4plr->PrintAvailablePhysLists();
    }
    g4alt::G4PhysListFactory factory;
    G4VModularPhysicsList* phys = nullptr;
    G4String physName = "FTFP_BERT+OPTICAL+STEPLIMIT+NEUTRONLIMIT";
    //
    // currently using the Constructor names doesn't work otherwise it would be:
    // G4String physName = "FTFP_BERT+G4OpticalPhysics+G4StepLimiterPhysics";
    // using the name doesn't work either
    //G4String physName = "FTFP_BERT+Optical+stepLimiter";
    // reference PhysicsList via its name
    if (factory.IsReferencePhysList(physName)) {
        phys = factory.GetReferencePhysList(physName);
    }
    /*
        // now add optical physics constructor:
        G4OpticalPhysics* opticalPhysics = new G4OpticalPhysics();
        phys->RegisterPhysics(opticalPhysics);
        // Cerenkov off by default
     */
    if (verbose) {
        G4cout << phys->GetPhysicsTableDirectory() << G4endl;
    }
    G4OpticalPhysics* opticalPhysics = (G4OpticalPhysics*) phys->GetPhysics("Optical");
    opticalPhysics->Configure(kCerenkov, true);
    opticalPhysics->SetCerenkovStackPhotons(false);
    opticalPhysics->Configure(kWLS, false);
    opticalPhysics->Configure(kScintillation, true);
    opticalPhysics->Configure(kRayleigh, true);
    opticalPhysics->Configure(kBoundary, true);
    opticalPhysics->Configure(kAbsorption, true);
    opticalPhysics->SetScintillationStackPhotons(false);
    opticalPhysics->SetTrackSecondariesFirst(kCerenkov, true); // only relevant if we actually stack and trace the optical photons
    opticalPhysics->SetTrackSecondariesFirst(kScintillation, false); // only relevant if we actually stack and trace the optical photons
    opticalPhysics->SetMaxNumPhotonsPerStep(100);
    opticalPhysics->SetMaxBetaChangePerStep(10.0);
    if (verbose) {
        phys->DumpList();
    }
    rm->SetUserInitialization(dc);
    rm->SetUserInitialization(phys);
    ga = new PrimaryGeneratorAction();
    ra = new RunAction();
    ea = new EventAction(ctx);
    ta = new TrackingAction(ctx);
    sa = new SteppingAction(ctx);

    rm->SetUserAction(ga);
    rm->SetUserAction(ra);
    rm->SetUserAction(ea);
    rm->SetUserAction(ta);
    rm->SetUserAction(sa);
    rm->Initialize();
}

G4::~G4() {
    G4GeometryManager::GetInstance()->OpenGeometry();
}




