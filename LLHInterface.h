#pragma once

// Depends on having a suitable definition of TemplateLLHGetter #included
// beforehand

#include "Bifrost.h"

Bifrost& operator>>(Bifrost& bf, OscPars& p)
{
  bf >> p.dm32 >> p.dm21 >> p.sth13 >> p.sth12 >> p.sth23 >> p.dcp;
  return bf;
}

Bifrost& operator<<(Bifrost& bf, OscPars& p)
{
  bf << p.dm32 << p.dm21 << p.sth13 << p.sth12 << p.sth23 << p.dcp;
  return bf;
}

/// For use from inside the container
void SendLLHOverBifrost(Bifrost& bf, TemplateLLHGetter* llh)
{
  while(true){

    int gotSentAsimov;
    bf >> gotSentAsimov;

    int gotSentPoisson;
    bf >> gotSentPoisson;

    int gotSentResetToDataFile;
    bf >> gotSentResetToDataFile;

    int gotSentSkipLLH;
    bf >> gotSentSkipLLH;

    OscPars p;
    bf >> p;
    llh->SetOscParameters(p);

    int nsystvecs;
    bf >> nsystvecs;

    for(int i = 0; i < nsystvecs; ++i){
      std::vector<double> systs;
      bf >> systs;
      CovTypes iCov = static_cast<CovTypes>(i);
      llh->SetParameters(iCov, systs);
    }

    if(gotSentAsimov) {
      llh->SetDataToAsimov();
    }
    if(gotSentPoisson) {
      llh->SetDataToPoissonStatFluctuations();
    }
    if(gotSentResetToDataFile) {
      llh->ResetToDataFile();
    }

    if(!gotSentSkipLLH) bf << llh->GetLikelihood();
  }
}

/// For use from outside the container
class ReceiveLLHOverBifrost : public TemplateLLHGetter
{
public:
  ReceiveLLHOverBifrost(Bifrost& bf) : fBF(bf), 
    fSetDataToAsimov(false),
    fSetDataToPoissonStatFluctuations(false),
    fResetToDataFile(false),
    fSkipLLHCalculation(false)
  {
  }

  void init(){}

  void SetParameters(CovTypes iCov, std::vector<double> vals)
  {
    if(fSysts.size() <= iCov) fSysts.resize(iCov+1);
    fSysts[iCov] = vals;
  }

  void SetOscParameters(OscPars oscpars)
  {
    fPars = oscpars;
  }

  void SetDataToAsimov()
  {
    fSetDataToAsimov = true;
    fResetToDataFile = false;
    fSetDataToPoissonStatFluctuations = false;
  }

  void SetDataToPoissonStatFluctuations()
  {
    fSetDataToPoissonStatFluctuations = true;
    fSetDataToAsimov = false;
    fResetToDataFile = false;
  }

  void ForceDummyLikelihood()
  {
    fSkipLLHCalculation = true;
    this->GetLikelihood();
  }

  void ResetToDataFile()
  {
    fResetToDataFile = true;
    fSetDataToAsimov = false;
    fSetDataToPoissonStatFluctuations = false;
  }

  double GetLikelihood()
  {

    fBF << int(fSetDataToAsimov);
    fBF << int(fSetDataToPoissonStatFluctuations);
    fBF << int(fResetToDataFile);
    fBF << int(fSkipLLHCalculation);

    fBF << fPars;

    // Bifrost doesn't have explicit support for vector<vector<double>>, just
    // send the size and then loop manually.
    int size = fSysts.size();
    fBF << size;
    for(const std::vector<double>& s: fSysts) fBF << s;

    double llh;

    if(!fSkipLLHCalculation) fBF >> llh;

    fSetDataToAsimov = false;
    fSetDataToPoissonStatFluctuations = false; 
    fResetToDataFile = false;
    fSkipLLHCalculation = false;

    return llh;
  }
  
protected:
  Bifrost& fBF;

  std::vector<std::vector<double>> fSysts;
  OscPars fPars;

  bool fSetDataToAsimov;
  bool fSetDataToPoissonStatFluctuations;
  bool fResetToDataFile;
  bool fSkipLLHCalculation;

};
