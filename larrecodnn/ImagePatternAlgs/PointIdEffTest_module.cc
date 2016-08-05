////////////////////////////////////////////////////////////////////////
// Class:       PointIdEffTest
// Module Type: analyzer
// File:        PointIdEffTest_module.cc
//
// Author: dorota.stefan@cern.ch
//
// Generated at Fri Apr 29 06:42:27 2016 by Dorota Stefan using artmod
// from cetpkgsupport v1_10_01.
////////////////////////////////////////////////////////////////////////

#include "larsimobj/Simulation/SimChannel.h"
#include "larsim/Simulation/LArG4Parameters.h"
#include "larcore/Geometry/Geometry.h"
#include "larcore/Geometry/GeometryCore.h"
#include "lardataobj/RecoBase/Hit.h"
#include "lardataobj/RecoBase/Cluster.h"
#include "lardataobj/RecoBase/Track.h"
#include "lardataobj/RecoBase/Vertex.h"
#include "lardataobj/RecoBase/Shower.h"
#include "lardata/AnalysisAlg/CalorimetryAlg.h"
#include "nusimdata/SimulationBase/MCParticle.h"
#include "nusimdata/SimulationBase/MCTruth.h"
#include "larcoreobj/SimpleTypesAndConstants/PhysicalConstants.h"
#include "lardata/Utilities/DatabaseUtil.h"

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "canvas/Persistency/Common/FindManyP.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Principal/SubRun.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "art/Framework/Services/Optional/TFileService.h"
#include "canvas/Utilities/InputTag.h"
#include "fhiclcpp/ParameterSet.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

#include "larreco/RecoAlg/ImagePatternAlgs/PointIdAlg/PointIdAlg.h"

#include "TH1.h"
#include "TTree.h"
#include "TLorentzVector.h"
#include "TVector3.h"

#include <iostream>
#include <fstream>

#include <cmath>

namespace nnet
{
	typedef std::map< unsigned int, std::vector< std::vector< art::Ptr<recob::Hit> > > > view_clmap;
	typedef std::map< unsigned int, view_clmap > tpc_view_clmap;
	typedef std::map< unsigned int, tpc_view_clmap > cryo_tpc_view_clmap;

	class PointIdEffTest;
}

class nnet::PointIdEffTest : public art::EDAnalyzer {
public:
  explicit PointIdEffTest(fhicl::ParameterSet const & p);
  // The destructor generated by the compiler is fine for classes
  // without bare pointers or other resource use.

  // Plugins should not be copied or assigned.
  PointIdEffTest(PointIdEffTest const &) = delete;
  PointIdEffTest(PointIdEffTest &&) = delete;
  PointIdEffTest & operator = (PointIdEffTest const &) = delete;
  PointIdEffTest & operator = (PointIdEffTest &&) = delete;

  // Required functions.
	virtual void beginRun(const art::Run& run) override;

	virtual void beginJob() override;
	virtual void endJob() override;

  virtual void analyze(art::Event const & e) override;

	virtual void reconfigure(fhicl::ParameterSet const& parameterSet) override;


private:

  // Declare member data here.

	int GetMCParticle(std::vector< art::Ptr<recob::Hit> > const & hits);

	void GetRecoParticle(std::vector< art::Ptr<recob::Hit> > const & hits, int mctype);

	void TestEffParticle();

	int fRun;
	int fEvent;

	int fShower;
	int fTrack;
	int fMCpid;
	int fClsize;
	int fRecoPid;
	int fPure;
	double fPidValue;
	

	int fTrkOk;
	int fTrkB;
	int fShOk;
	int fShB;
	int fNone;
	int fTotal;

	double fElectronsToGeV;
	int fSimTrackID;
	float fOutTrk;
	float fOutSh;

	TTree *fTree, *fTreecl;

	std::ofstream fHitsOutFile;

	nnet::PointIdAlg fPointIdAlg;
    double fThreshold;
	unsigned int fView;

	geo::GeometryCore const* fGeometry;

	std::map< int, const simb::MCParticle* > fParticleMap;

	art::Handle< std::vector<simb::MCParticle> > fParticleHandle;
	art::Handle< std::vector<sim::SimChannel> > fSimChannelHandle;
	art::Handle< std::vector<recob::Hit> > fHitListHandle;
	art::Handle< std::vector<recob::Cluster> > fClusterListHandle;

	std::vector< art::Ptr<sim::SimChannel> > fChannellist;
	std::vector< art::Ptr<simb::MCParticle> > fSimlist;
	std::vector< art::Ptr<recob::Hit> > fHitlist;
	std::vector< art::Ptr<recob::Cluster> > fClusterlist;

	cryo_tpc_view_clmap fClMap;	

	std::string fSimulationProducerLabel;
	std::string fHitsModuleLabel;
	std::string fClusterModuleLabel;
	bool fSaveHitsFile;
};

void nnet::PointIdEffTest::reconfigure(fhicl::ParameterSet const & p)
{
	fSimulationProducerLabel = p.get< std::string >("SimModuleLabel");
	fHitsModuleLabel = p.get< std::string >("HitsModuleLabel");
	fClusterModuleLabel = p.get< std::string >("ClusterModuleLabel");
	fSaveHitsFile = p.get< bool >("SaveHitsFile");
	fPointIdAlg.reconfigure(p.get< fhicl::ParameterSet >("PointIdAlg"));
	fThreshold = p.get< double >("Threshold");
	fView = p.get< unsigned int >("View");
}


nnet::PointIdEffTest::PointIdEffTest(fhicl::ParameterSet const & p)
  :
  EDAnalyzer(p),
	fShower(0),
	fTrack(1),
	fMCpid(-1),
	fClsize(0),
	fRecoPid(-1),
	fTrkOk(0),
	fTrkB(0),
	fShOk(0),
	fShB(0), fNone(0), fTotal(0),
 	fPointIdAlg(p.get< fhicl::ParameterSet >("PointIdAlg"))
{
	fGeometry = &*(art::ServiceHandle<geo::Geometry>());
	reconfigure(p);
}

void nnet::PointIdEffTest::beginRun(const art::Run&)
{
	art::ServiceHandle< sim::LArG4Parameters > larParameters;
	fElectronsToGeV = 1./larParameters->GeVToElectrons();
}

void nnet::PointIdEffTest::beginJob()
{
	// access art's TFileService, which will handle creating and writing hists
	art::ServiceHandle<art::TFileService> tfs;

	fTree = tfs->make<TTree>("hit","hit tree");

	fTree->Branch("fOutSh", &fOutSh, "fOutSh/F");
	fTree->Branch("fOutTrk", &fOutTrk, "fOutTrk/F");

	fTreecl = tfs->make<TTree>("cluster","cluster tree");
	fTreecl->Branch("fMCpid", &fMCpid, "fMCpid/I");
	fTreecl->Branch("fClsize", &fClsize, "fClsize/I");
	fTreecl->Branch("fRecoPid", &fRecoPid, "fRecoPid/I");
	fTreecl->Branch("fPidValue", &fPidValue, "fPidValue/D");

	if (fSaveHitsFile) fHitsOutFile.open("hits_pid.prn");
}

void nnet::PointIdEffTest::endJob()
{
	if (fSaveHitsFile) fHitsOutFile.close();

	std::cout << " fShOk " << fShOk << " fTrkOk " << fTrkOk << std::endl;
	std::cout << " fShB " << fShB << " fTrkB " << fTrkB << std::endl;
	std::cout << " fNone " << fNone << " Total " << fTotal << std::endl;

	std::cout << " fShErr " << fShB / float(fShB + fShOk) << " fTrkErr " << fTrkB / float(fTrkB + fTrkOk) << std::endl;
}

void nnet::PointIdEffTest::analyze(art::Event const & e)
{
  // Implementation of required member function here.

	fRun = e.run();
	fEvent = e.id().event();
	fSimTrackID = -1; 
	fOutSh = -1; fOutTrk = -1;
	fClMap.clear();

	// access to MC information
	
	// MC Particle

	if (e.getByLabel(fSimulationProducerLabel, fParticleHandle))
		art::fill_ptr_vector(fSimlist, fParticleHandle);
	
	
	for ( auto const& particle : (*fParticleHandle) )
	{
		fSimTrackID = particle.TrackId();
		fParticleMap[fSimTrackID] = &particle;
	}

	// simChannel
	if (e.getByLabel(fSimulationProducerLabel, fSimChannelHandle))
		art::fill_ptr_vector(fChannellist, fSimChannelHandle);

	// output from reconstruction

	// hits
	if (e.getByLabel(fHitsModuleLabel, fHitListHandle))
		art::fill_ptr_vector(fHitlist, fHitListHandle);

	// clusters
	if (e.getByLabel(fClusterModuleLabel, fClusterListHandle))
		art::fill_ptr_vector(fClusterlist, fClusterListHandle);
 

	const art::FindManyP<recob::Hit> findManyHits(fClusterListHandle, e, fClusterModuleLabel);

	for (size_t clid = 0; clid != fClusterListHandle->size(); ++clid)
	{
		auto const& hits = findManyHits.at(clid);
		if (!hits.size()) continue;

		unsigned int cryo = hits.front()->WireID().Cryostat;
		unsigned int tpc = hits.front()->WireID().TPC;
		unsigned int view = hits.front()->WireID().Plane;

		fClMap[cryo][tpc][view].push_back(hits);		 
	}

	for (auto const& c : fClMap)
	{
		unsigned int cryo = c.first;
		for (auto const& t : c.second)
		{
			unsigned int tpc = t.first;
			for (auto const& v : t.second)
			{
				unsigned int view = v.first;
				if (view == fView)
				{
					fPointIdAlg.setWireDriftData(e, view, tpc, cryo);

					for (auto const& h : v.second)
					{  
						int mctype = GetMCParticle(h); // mctype == -1 : problem with simchannel
						if (mctype > -1)
							GetRecoParticle(h, mctype);
					}
				}
			}
		}
	}	
}

/******************************************/

int nnet::PointIdEffTest::GetMCParticle(std::vector< art::Ptr<recob::Hit> > const & hits)
{

	// in argument vector hitow danego klastra

	double ensh = 0.; double entrk = 0.;
	size_t insideFidArea = 0;

	for (auto const& hit: hits)
	{
		if (!fPointIdAlg.isInsideFiducialRegion(hit->WireID().Wire, hit->PeakTime())) continue;
		insideFidArea++;

		// the channel associated with this hit.
		auto hitChannelNumber = hit->Channel();

		for ( auto const& channel : (*fSimChannelHandle) )
		{
			auto simChannelNumber = channel.Channel();

			if ( simChannelNumber != hitChannelNumber ) continue;

			// for every time slice in this channel:
			auto const& timeSlices = channel.TDCIDEMap();
			for ( auto const& timeSlice : timeSlices )
			{
				int time = timeSlice.first;
				if ( std::abs(hit->TimeDistanceAsRMS(time) ) < 1.0 )
				{
					// loop over the energy deposits.
					auto const& energyDeposits = timeSlice.second;
		
					for ( auto const& energyDeposit : energyDeposits )
					{
						int trackID = energyDeposit.trackID;

						double energy = energyDeposit.numElectrons * fElectronsToGeV * 1000;

						if (trackID < 0)
						{
							ensh += energy;
						}
						else if (trackID > 0)
						{
							auto search = fParticleMap.find(trackID);
							bool found = true;
							if (search == fParticleMap.end())
							{
								mf::LogWarning("TrainingDataAlg") << "PARTICLE NOT FOUND";
								found = false;
							}
						
							int pdg = 0;
							if (found)
							{
								const simb::MCParticle& particle = *((*search).second);
								if (!pdg) pdg = particle.PdgCode(); // not EM activity so read what PDG it is
							}

							if ((pdg == 11) || (pdg == -11) || (pdg == 22)) ensh += energy;
							else entrk += energy;
						}
					}
				}
			}
		}
	}

	int result = -1;
	if (insideFidArea > 2 * hits.size() / 3) // 2/3 of the cluster hits inside fiducial area
	{
		if (ensh > 1.5 * entrk) // major energy deposit from EM activity
		{
			result = fShower;
		}
		else if (entrk > 1.5 * ensh)
		{
			result = fTrack;
		}
	}

	return result;
}

/******************************************/

void nnet::PointIdEffTest::GetRecoParticle(std::vector< art::Ptr<recob::Hit> > const & hits, int mctype)
{
	fMCpid = mctype;
	fClsize = hits.size();

	fPidValue = 0;
	if (fPointIdAlg.NClasses() == 1)
	{
		fPidValue = fPointIdAlg.predictIdValue(hits);
	}
	else
	{
		auto vout = fPointIdAlg.predictIdVector(hits);
		double p_trk_or_sh = vout[0] + vout[1];
		if (p_trk_or_sh > 0) fPidValue = vout[0] / p_trk_or_sh;
	}

	if (fPidValue < fThreshold) fRecoPid = fShower;
	else if (fPidValue > fThreshold) fRecoPid = fTrack;
	else fRecoPid = -1;

	if ((fRecoPid == fShower) && (mctype == fShower))
	{
		fShOk++;
	}
	else if ((fRecoPid == fTrack) && (mctype == fTrack))
	{
		fTrkOk++;
	}	
	else if ((fRecoPid == fShower) && (mctype == fTrack))
	{
		fTrkB++;
	}
	else if ((fRecoPid == fTrack) && (mctype == fShower))
	{
		fShB++;
	}
	else
	{
		fNone++;
	}	

	fTotal++;

	if (fSaveHitsFile)
	{
		for (auto const h : hits)
		{
			fHitsOutFile << fRun << " " << fEvent << " "
				<< h->WireID().TPC  << " " << h->WireID().Wire << " " << h->PeakTime() << " "
				<< mctype << " " << fRecoPid << " " << fPidValue << std::endl;
		}
	}
	
	fTreecl->Fill();	
}

/******************************************/

void nnet::PointIdEffTest::TestEffParticle()
{
		// to be filled.
}

/******************************************/

DEFINE_ART_MODULE(nnet::PointIdEffTest)