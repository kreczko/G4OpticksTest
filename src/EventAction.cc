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

#include <string>
#include "G4Event.hh"
#include "G4HCofThisEvent.hh"
#include "G4SDManager.hh"
#include "G4HCtable.hh"
#include "G4ThreeVector.hh"
#include "EventAction.hh"
#include "RootIO.hh"
#include "Event.hh"
#include <vector> 
#include "PhotonHit.hh"
#ifdef WITH_OPTICKS
#include "OpticksFlags.hh"
#include "G4Opticks.hh"
#include "NPho.hpp"
#include "NPY.hpp"
#endif

#include "Ctx.hh"
#include "ConfigurationManager.hh"

std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    return split(s, delim, elems);
}

EventAction::EventAction(Ctx* ctx_)
:
ctx(ctx_) {
    CaTSEvt = new Event();
}

void EventAction::BeginOfEventAction(const G4Event* anEvent) {
    enable_IO = ConfigurationManager::getInstance()->isWriteHits();
    ctx->setEvent(anEvent);
    CaTSEvt->SetEventNr(anEvent->GetEventID());
}

void EventAction::EndOfEventAction(const G4Event* event) {
    G4cout << "Event:   " << event->GetEventID() << G4endl;
    bool verbose = ConfigurationManager::getInstance()->isEnable_verbose();
    G4HCofThisEvent* HCE = event->GetHCofThisEvent();
    assert(HCE);
    std::vector<G4VHit*> hitsVector;
    std::map<G4String, std::vector<G4VHit* > >* hcmap = CaTSEvt->GetHCMap();
    if (ConfigurationManager::getInstance()->isEnable_opticks()) {

#ifdef WITH_OPTICKS
        G4Opticks* ok = G4Opticks::Get();
        G4int eventid = event->GetEventID();
        int num_hits = ok->propagateOpticalPhotons(eventid);

        NPY<float>* hits = ok->getHits();
        NPho* m_hits = new NPho(hits);
        unsigned m_num_hits = m_hits->getNumPhotons();
        assert(hits == NULL || hits->getNumItems() == unsigned(num_hits));
        if (verbose) {
            G4cout << "EventAction::EndOfEventAction"
                    << " num_hits " << num_hits
                    << "   m_num_hits: " << m_num_hits
                    << " hits " << hits
                    << G4endl;
        }
        G4ThreeVector position;
        G4ThreeVector direction;
        G4ThreeVector polarization;
        for (unsigned i = 0; i < m_num_hits; i++) {
            glm::vec4 post = m_hits->getPositionTime(i);
            //        G4ThreeVector * position = new G4ThreeVector(double(post.x), double(post.y), double(post.z));
            position.setX(double(post.x));
            position.setY(double(post.y));
            position.setZ(double(post.z));
            //position->set(double(post.x), double(post.y), double(post.z));
            G4double time = double(post.w);
            glm::vec4 dirw = m_hits->getDirectionWeight(i);
            direction.setX(double(dirw.x));
            direction.setY(double(dirw.y));
            direction.setZ(double(dirw.z));
            G4double weight = double(dirw.w);
            glm::vec4 polw = m_hits->getPolarizationWavelength(i);
            polarization.setX(double(polw.x));
            polarization.setY(double(polw.y));
            polarization.setZ(double(polw.z));
            G4double wavelength = double(polw.w);
            glm::uvec4 flags = m_hits->getFlags(i);
            G4int flags_x = flags.x;
            G4int flags_y = flags.y;
            G4int flags_z = flags.z;
            G4int flags_w = flags.w;
            G4bool is_cerenkov = (flags.w & CERENKOV) != 0;
            G4bool is_reemission = (flags.w & BULK_REEMIT) != 0;
            //        G4cout << "weight:  " << flags_w
            //                << "  cerenkov:  " << is_cerenkov
            //                << "  reemit:  " << is_reemission
            //                << G4endl;
            hitsVector.push_back(new PhotonHit(i,
                    0,
                    wavelength,
                    time,
                    position,
                    direction,
                    polarization));
        }

        hcmap->insert(std::make_pair("PhotonDetector", hitsVector));
        ok->reset();
#endif 
    }
    //
    // Now we deal with The geant4 Hit collections. 
    //
    // G4cout << "Number of collections:  " << HCE->GetNumberOfCollections() << G4endl;

    for (int i = 0; i < HCE->GetNumberOfCollections(); i++) {
        hitsVector.clear();
        G4VHitsCollection* hc = HCE->GetHC(i);
        G4String hcname = hc->GetName();
        std::vector<std::string> y = split(hcname, '_');
        std::string Classname = y[1];
        if (verbose) {
            G4cout << "Classname: " << Classname << G4endl;
        }
        if (Classname == "lArTPC") {
            G4int NbHits = hc->GetSize();
            for (G4int ii = 0; ii < NbHits; ii++) {
                G4VHit* hit = hc->GetHit(ii);
                lArTPCHit* Hit = dynamic_cast<lArTPCHit*> (hit);
                hitsVector.push_back(Hit);
            }
            hcmap->insert(std::make_pair(hcname, hitsVector));
        } else if (Classname == "Photondetector") {
            G4int NbHits = hc->GetSize();
            if (verbose) G4cout << "Photondetector size: " << hc->GetSize() << G4endl;
            for (G4int ii = 0; ii < NbHits; ii++) {
                G4VHit* hit = hc->GetHit(ii);
                PhotonHit* Hit = dynamic_cast<PhotonHit*> (hit);
                hitsVector.push_back(Hit);
            }
            hcmap->insert(std::make_pair(hcname, hitsVector));
        } else {
            G4cout << "SD type: " << Classname << " unknown" << G4endl;
        }
    }
    if (enable_IO) {
        RootIO::GetInstance()->Write(CaTSEvt);
    }
    CaTSEvt->Reset();
}

