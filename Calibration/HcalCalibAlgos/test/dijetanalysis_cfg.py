import FWCore.ParameterSet.Config as cms

process = cms.Process("DIJETSANALYSIS")

process.load("Configuration.StandardSequences.Geometry_cff")

process.load("Configuration.StandardSequences.MagneticField_cff")

process.load("Configuration.StandardSequences.Reconstruction_cff")

process.load("RecoJets.Configuration.CaloTowersRec_cff")                                                           
process.towerMaker.ecalInputs = ['DiJProd:DiJetsEcalRecHitCollection']
process.towerMaker.hbheInput = 'HitsReCalibration:DiJetsHBHEReRecHitCollection'
process.towerMaker.hoInput = 'HitsReCalibration:DiJetsHOReRecHitCollection'
process.towerMaker.hfInput = 'HitsReCalibration:DiJetsHFReRecHitCollection'
process.towerMakerWithHO.ecalInputs = ['DiJProd:DiJetsEcalRecHitCollection']
process.towerMakerWithHO.hbheInput = 'HitsReCalibration:DiJetsHBHEReRecHitCollection'
process.towerMakerWithHO.hoInput = 'HitsReCalibration:DiJetsHOReRecHitCollection'
process.towerMakerWithHO.hfInput = 'HitsReCalibration:DiJetsHFReRecHitCollection'

process.iterativeCone5CaloJets.correctInputToSignalVertex = cms.bool(False)

process.load("RecoJets.JetProducers.iterativeCone5CaloJets_cff")



process.maxEvents = cms.untracked.PSet(
    input = cms.untracked.int32(-1)
)
process.source = cms.Source("PoolSource",
    fileNames = 
cms.untracked.vstring('/store/relval/CMSSW_3_1_0_pre3/RelValQCD_Pt_80_120/ALCARECO/STARTUP_30X_StreamALCARECOHcalCalDijets_v1/0001/D43D0349-800A-DE11-8751-000423D6006E.root')
)

process.es_ascii2 = cms.ESSource("HcalTextCalibrations",
    input = cms.VPSet(cms.PSet(
        object = cms.string('RespCorrs'),
        file = cms.FileInPath('Calibration/HcalCalibAlgos/test/corrections.txt')
    )),
    appendToDataLabel = cms.string('recalibrate')
)

process.prefer("es_ascii2")
process.HitsReCalibration = cms.EDFilter("HitReCalibrator",
    hbheInput = cms.InputTag("DiJProd","DiJetsHBHERecHitCollection"),
    hfInput = cms.InputTag("DiJProd","DiJetsHFRecHitCollection"),
    hoInput = cms.InputTag("DiJProd","DiJetsHORecHitCollection")
)

process.DiJetAnalysis = cms.EDAnalyzer("DiJetAnalyzer",
    hbheInput = cms.InputTag("HitsReCalibration","DiJetsHBHEReRecHitCollection"),
    HistOutFile = cms.untracked.string('hi.root'),
    hfInput = cms.InputTag("HitsReCalibration","DiJetsHFReRecHitCollection"),
    hoInput = cms.InputTag("HitsReCalibration","DiJetsHOReRecHitCollection"),
    ecInput = cms.InputTag("DiJProd","DiJetsEcalRecHitCollection"),
    jetsInput = cms.InputTag("iterativeCone5CaloJets")
)

process.DiJetsRecoPool = cms.OutputModule("PoolOutputModule",
    outputCommands = cms.untracked.vstring('drop *', 
        'keep *_DiJetsReco_*_*'),
    fileName = cms.untracked.string('/tmp/krohotin/di.root')
)

process.p = cms.Path(process.HitsReCalibration*process.caloTowersRec*process.iterativeCone5CaloJets*process.DiJetAnalysis)

