/**
 * \class L1GlobalTriggerGTL
 * 
 * 
 * Description: Global Trigger Logic board, see header file for details.  
 *
 * Implementation:
 *    <TODO: enter implementation details>
 *   
 * \author: M. Fierro            - HEPHY Vienna - ORCA version 
 * \author: Vasile Mihai Ghete   - HEPHY Vienna - CMSSW version 
 * 
 * $Date$
 * $Revision$
 *
 */

// this class header
#include "L1Trigger/GlobalTrigger/interface/L1GlobalTriggerGTL.h"

// system include files
#include <vector>

// user include files
#include "DataFormats/L1GlobalTrigger/interface/L1GlobalTriggerReadoutSetupFwd.h"
#include "DataFormats/L1GlobalTrigger/interface/L1GlobalTriggerReadoutSetup.h"
#include "DataFormats/L1GlobalTrigger/interface/L1GlobalTriggerReadoutRecord.h"
#include "DataFormats/L1GlobalTrigger/interface/L1GlobalTriggerObjectMapRecord.h"

#include "DataFormats/L1GlobalTrigger/interface/L1GlobalTriggerObjectMap.h"

#include "L1Trigger/GlobalTrigger/interface/L1GlobalTrigger.h"
#include "L1Trigger/GlobalTrigger/interface/L1GlobalTriggerSetup.h"
#include "L1Trigger/GlobalTrigger/interface/L1GlobalTriggerConfig.h"

#include "L1Trigger/GlobalTrigger/interface/L1GlobalTriggerConditions.h"
#include "L1Trigger/GlobalTrigger/interface/L1GlobalTriggerMuonTemplate.h"
#include "L1Trigger/GlobalTrigger/interface/L1GlobalTriggerCaloTemplate.h"
#include "L1Trigger/GlobalTrigger/interface/L1GlobalTriggerEsumsTemplate.h"
#include "L1Trigger/GlobalTrigger/interface/L1GlobalTriggerJetCountsTemplate.h"

#include "L1Trigger/GlobalMuonTrigger/interface/L1MuGlobalMuonTrigger.h"

#include "FWCore/Utilities/interface/Exception.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"

// forward declarations

// constructor
L1GlobalTriggerGTL::L1GlobalTriggerGTL(const L1GlobalTrigger& gt) 
    : 
    m_GT(gt), 
    glt_muonCand( new GMTVector(L1GlobalTriggerReadoutSetup::NumberL1Muons) ) {

    m_gtlAlgorithmOR.reset();
    m_gtlDecisionWord.reset(); 
    
    for ( int i = 0; i < 9; i++) {
        glt_cond[i].reset();
        glt_algos.push_back( particleBlock() );
        glt_particleConditions.push_back( new conditions( L1GlobalTriggerSetup::MaxItem ) );
    }

    glt_muonCand->reserve(L1GlobalTriggerReadoutSetup::NumberL1Muons);
    
}

// destructor
L1GlobalTriggerGTL::~L1GlobalTriggerGTL() { 
    
    reset();
    glt_muonCand->clear();
    delete glt_muonCand;
    
    for (conditionContainer::iterator iter = glt_particleConditions.begin(); 
            iter != glt_particleConditions.end(); iter++ ) {
        (*iter)->clear();
        delete *iter;
    }
}

// operations

// receive input data

void L1GlobalTriggerGTL::receiveData(edm::Event& iEvent, int iBxInEvent) {

    reset(); 
    
    // disabling Global Muon input 

    const L1GlobalTriggerConfig* gtConf = m_GT.gtSetup()->gtConfig();

    if ( gtConf != 0 ) { 
        if ( gtConf->getInputMask()[1] ) {

            LogDebug("L1GlobalTriggerGTL") 
                << "\n**** Global Muon input disabled! \n  inputMask[1] = " 
                << gtConf->getInputMask()[1]
                << "     All candidates empty." << "\n**** \n"
                << std::endl;

            // set all muon candidates empty                
            for ( unsigned int iMuon = 0; iMuon < L1GlobalTriggerReadoutSetup::NumberL1Muons; iMuon++ ) {
        
                MuonDataWord dataword = 0; 
                (*glt_muonCand)[iMuon] = new L1MuGMTCand( dataword );
            }
            return;
            
        }
    } else {
        throw cms::Exception("L1GtConfiguration")
         << "**** No configuration file exists! \n";
    }

    LogDebug("L1GlobalTriggerGTL") 
        << "**** L1GlobalTriggerGTL receiving muon data from input tag " 
        << m_GT.gtSetup()->muGmtInputTag().label()
        << std::endl;
    

    // get data from Global Muon Trigger
    // the GLT receives 4 * 26 bits from the Global Muon Trigger

    edm::Handle<std::vector<L1MuGMTCand> > muonData;
    iEvent.getByLabel(m_GT.gtSetup()->muGmtInputTag().label(), muonData);

    for ( unsigned int iMuon = 0; iMuon < L1GlobalTriggerReadoutSetup::NumberL1Muons; iMuon++ ) {

        MuonDataWord dataword = 0; 
        unsigned int nMuon = 0;

        for ( std::vector<L1MuGMTCand>::const_iterator itMuon = muonData->begin(); 
            itMuon != muonData->end(); itMuon++ ) {
            
            // retrieving info for a given bx only
            if ( (*itMuon).bx() == iBxInEvent ) {
                if ( nMuon == iMuon ) {
                    dataword = (*itMuon).getDataWord();
                    break;
                }
                nMuon++;
            }
        }
        (*glt_muonCand)[iMuon] = new L1MuGMTCand( dataword );
    }
    
    printGmtData(iBxInEvent);    

}

// run GTL
void L1GlobalTriggerGTL::run(int iBxInEvent) {
        
//    LogDebug ("Trace") << "**** L1GlobalTriggerGTL run " << std::endl;

    // try xml conditions
    const L1GlobalTriggerConfig* gtConf = m_GT.gtSetup()->gtConfig();
     
    if (gtConf != 0) {
        unsigned int chipnr;
        LogDebug("L1GlobalTriggerGTL") 
            << "\n***** Result of the XML-conditions \n" 
            << std::endl;

        for (chipnr = 0; chipnr < L1GlobalTriggerConfig::NumberConditionChips; chipnr++) { 
            LogTrace("L1GlobalTriggerGTL") 
                << "\n---------Chip " << chipnr + 1 << " ----------\n" 
                << std::endl; 
    
            for (L1GlobalTriggerConfig::ConditionsMap::const_iterator 
                itxml = gtConf->conditionsmap[chipnr].begin(); 
                itxml != gtConf->conditionsmap[chipnr].end(); itxml++) {

                std::string condName = itxml->first;

                LogTrace("L1GlobalTriggerGTL") 
                    << "\n===============================================\n" 
                    << "Evaluating condition: " << condName 
                    << "\n" 
                    << std::endl;

                bool condResult = itxml->second->blockCondition_sr(); 
                
                LogTrace("L1GlobalTriggerGTL")
                    << condName << " result: " << condResult 
                    << std::endl;
                    
//                CombinationsInCond* combsInCond = itxml->second->getCombinationsInCond();
//                    
//                CombinationsInCond::const_iterator itVV;
//                std::ostringstream myCout1;                 
//        
//                for(itVV  = (*combsInCond).begin(); 
//                    itVV != (*combsInCond).end(); itVV++) {
//                    
//                    myCout1 << "( ";
//                
//                    std::copy((*itVV).begin(), (*itVV).end(), 
//                        std::ostream_iterator<int> (myCout1, " "));
//                    
//                    myCout1 << "); ";
//        
//                }
//        
//        
//                LogTrace("L1GlobalTriggerGTL") 
//                    << "\n  List of combinations passing all requirements for this condition: \n  " 
//                    <<  myCout1.str() 
//                    << " \n" 
//                    << std::endl;
                                         
            
            }
        }
        
        LogTrace("L1GlobalTriggerGTL") 
            << "\n---------- Prealgorithms: evaluation ---------\n" 
            << std::endl;
        for (L1GlobalTriggerConfig::ConditionsMap::const_iterator 
            itxml  = gtConf->prealgosmap.begin(); 
            itxml != gtConf->prealgosmap.end(); itxml++) {
                                
            std::string prealgoName = itxml->first;
            bool prealgoResult = itxml->second->blockCondition_sr(); 
            std::string prealgoLogExpression = itxml->second->getLogicalExpression();
            std::string prealgoNumExpression = itxml->second->getNumericExpression();
            
            // set the pins if VERSION_FINAL (final version uses prealgorithms)
            if (gtConf->getVersion() == L1GlobalTriggerConfig::VERSION_FINAL) {

                // algo( i ) = prealgo( i+1 ), i = 0, MaxNumberAlgorithms
                int prealgoNumber = itxml->second->getAlgoNumber();               
    
                if (itxml->second->getLastResult()) {
                    if (prealgoNumber > 0) {
                        m_gtlAlgorithmOR.set( prealgoNumber-1);
                        
                    }
                }
                
                LogTrace("L1GlobalTriggerGTL")
                    << " Bit " << prealgoNumber-1
                    << " " << prealgoName << " = " << prealgoLogExpression << ": " 
                    << m_gtlAlgorithmOR[ prealgoNumber-1 ]
                    << " = " << prealgoNumExpression
                    << std::endl;
            } else {
            
                LogTrace("L1GlobalTriggerGTL")
                    << " " << prealgoName << " = " << prealgoLogExpression << ": " 
                    << prealgoResult 
                    << " = " << prealgoNumExpression
                    << std::endl;
            }
            
        }
        LogTrace("L1GlobalTriggerGTL") 
            << "\n---------- End of prealgorithm list ---------\n" 
            << std::endl;

        LogTrace("L1GlobalTriggerGTL") 
            << "\n---------- Algorithms: evaluation ----------\n" 
            << std::endl;
        for (L1GlobalTriggerConfig::ConditionsMap::const_iterator 
            itxml  = gtConf->algosmap.begin(); 
            itxml != gtConf->algosmap.end(); itxml++) {
            
            std::string algoName = itxml->first;
            bool algoResult = itxml->second->blockCondition_sr(); 
            std::string algoLogExpression = itxml->second->getLogicalExpression();
            std::string algoNumExpression = itxml->second->getNumericExpression();

            // set the pins if VERSION_PROTOTYPE (prototype version uses algos)
            if (gtConf->getVersion() == L1GlobalTriggerConfig::VERSION_PROTOTYPE) {
                if (itxml->second->getLastResult()) {
                    if (itxml->second->getOutputPin() > 0) {
                        m_gtlAlgorithmOR.set( itxml->second->getOutputPin()-1);
                    }
                }

                LogTrace("L1GlobalTriggerGTL")
                    << " Bit " << itxml->second->getOutputPin()-1
                    << " " << algoName << " = " << algoLogExpression  
                    << " = " << algoNumExpression
                    << std::endl;

            } else {           
                                     
                LogTrace("L1GlobalTriggerGTL")
                    << "  " << algoName << " = " << algoLogExpression 
                    << " = " << algoNumExpression  << " = " << algoResult
                    << std::endl;
            }

        }
        LogTrace("L1GlobalTriggerGTL") 
            << "\n---------- End of algorithm list ----------\n" 
            << std::endl;

    }
    
}

// fill object map record
const std::vector<L1GlobalTriggerObjectMap>* L1GlobalTriggerGTL::objectMap() {

    const L1GlobalTriggerConfig* gtConf = m_GT.gtSetup()->gtConfig();
    
    // empty vector for object maps
    std::vector<L1GlobalTriggerObjectMap>* 
        objMapVec = new std::vector<L1GlobalTriggerObjectMap>();
        
    // do it only if VERSION_FINAL
    if (gtConf->getVersion() == L1GlobalTriggerConfig::VERSION_FINAL) {
    
        L1GlobalTriggerObjectMap* objMap = new L1GlobalTriggerObjectMap();
        
        for (L1GlobalTriggerConfig::ConditionsMap::const_iterator 
            itxml  = gtConf->prealgosmap.begin(); 
            itxml != gtConf->prealgosmap.end(); itxml++) {
                                
            std::string prealgoNameVal = itxml->first;

            // algo( i ) = prealgo( i+1 ), i = 0, MaxNumberAlgorithms
            int prealgoNumberVal = itxml->second->getAlgoNumber(); 
            int algoBitNumberVal = prealgoNumberVal -1;

            bool prealgoResultVal = itxml->second->getLastResult();

            std::string prealgoLogExpressionVal = itxml->second->getLogicalExpression();
            std::string prealgoNumExpressionVal = itxml->second->getNumericExpression();

            std::vector<CombinationsInCond> combVectorVal = 
                itxml->second->getCombinationVector(); 

            std::vector<ObjectTypeInCond> objTypeVal = 
                itxml->second->getObjectTypeVector(); 
            
            // set object map
            
            objMap->reset();
            
            objMap->setAlgoName(prealgoNameVal);
            objMap->setAlgoBitNumber(algoBitNumberVal);
            objMap->setAlgoGtlResult(prealgoResultVal);
            objMap->setAlgoLogicalExpression(prealgoLogExpressionVal);
            objMap->setAlgoNumericalExpression(prealgoNumExpressionVal);
            objMap->setCombinationVector(combVectorVal);
            objMap->setObjectTypeVector(objTypeVal);
            
            std::ostringstream myCout1;                 
            objMap->print(myCout1);

            LogTrace("L1GlobalTriggerGTL") 
                <<  myCout1.str() 
                << std::endl;
                
            objMapVec->push_back(*objMap);
                                                
            // after last usage, objMapVec must be deleted! 
            
        }
        
        delete objMap;
    }

    return objMapVec;
    
}

// clear GTL
void L1GlobalTriggerGTL::reset() {
    
    GMTVector::iterator iter;
    for ( iter = glt_muonCand->begin(); iter < glt_muonCand->end(); iter++ ) {
        if (*iter) {
            delete (*iter);
            *iter = 0;
        }
    }

    m_gtlDecisionWord.reset();
    
    // TODO get rid of 9 hardwired!!!!!
    for (int i = 0; i < 9; i++) {
        glt_cond[i].reset();
    }

    m_gtlAlgorithmOR.reset();

}

// print Global Muon Trigger data

void L1GlobalTriggerGTL::printGmtData(int iBxInEvent) const {
    
    LogTrace("L1GlobalTriggerGTL") 
        << "\nMuon data received by GTL:" << std::endl;
    
    for ( GMTVector::iterator iter = glt_muonCand->begin(); 
            iter < glt_muonCand->end(); iter++ ) {

        LogTrace("L1GlobalTriggerGTL") 
            << "\nIterator value = " << (*iter) << std::endl;

        (*iter)->print();
        
        LogTrace("L1GlobalTriggerGTL")
            << "Indices: pt = " << (*iter)->ptIndex() 
            << " eta = " << (*iter)->etaIndex()  
            << " phi = " << (*iter)->phiIndex()
            << std::endl;  

    }

    LogTrace("L1GlobalTriggerGTL") << std::endl;

}


const std::vector<MuonDataWord> L1GlobalTriggerGTL::getMuons() const { 

    std::vector<MuonDataWord> muon(L1GlobalTriggerReadoutSetup::NumberL1Muons);

    for (unsigned int i = 0; i < L1GlobalTriggerReadoutSetup::NumberL1Muons; i++) {
        muon[i] = (*glt_muonCand)[i]->getDataWord(); 
    }

    return muon;
}
