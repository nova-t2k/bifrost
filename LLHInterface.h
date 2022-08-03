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
    //send asimov osc pars
    int gotSentAsimovOsc;
    bf >> gotSentAsimovOsc;

    if(gotSentAsimovOsc) {
      OscPars asimovOscPars;
      bf >> asimovOscPars;
      llh->SetAsimovOscParameters(asimovOscPars);
    }

    int nasimovsystvecs;
    bf >> nasimovsystvecs;

    for(int i = 0; i < nasimovsystvecs; ++i){
       std::vector<double> asimovsysts;
       bf >> asimovsysts;
       CovTypes iCov = static_cast<CovTypes>(i);
       llh->SetAsimovSystParameters(iCov, asimovsysts);
    }

    //send poisson data parameters
    int gotSentPoissonFakeDataOsc;
    bf >> gotSentPoissonFakeDataOsc;

    if(gotSentPoissonFakeDataOsc) {
      OscPars poissonFakeDataOscPars;
      bf >> poissonFakeDataOscPars;
      llh->SetPoissonFakeDataOscParameters(poissonFakeDataOscPars);
    }

    int npoissonFakeDatasystvecs;
    bf >> npoissonFakeDatasystvecs;

    for(int i = 0; i < npoissonFakeDatasystvecs; ++i){
       std::vector<double> poissonFakeDatasysts;
       bf >> poissonFakeDatasysts;
       CovTypes iCov = static_cast<CovTypes>(i);
       llh->SetPoissonFakeDataSystParameters(iCov, poissonFakeDatasysts);
    }

    //instruction to reset to the original data file
    int gotSentResetToDataFile;
    bf >> gotSentResetToDataFile;
    if(gotSentResetToDataFile) {
      llh->ResetToDataFile();
    }

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

    bf << llh->GetLikelihood();
  }
}

/// For use from outside the container
class ReceiveLLHOverBifrost : public TemplateLLHGetter
{
public:
  ReceiveLLHOverBifrost(Bifrost& bf) : fBF(bf), fSendAsimovOsc(false), fSendAsimovSysts(false),
    fSendPoissonFakeDataOsc(false), fSendPoissonFakeDataSysts(false), fResetToDataFile(false)
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

  //Setting Asimov parameters
  void SetAsimovSystParameters(CovTypes iCov, std::vector<double> vals)
  {
    if(fAsimovSysts.size() <= iCov) fAsimovSysts.resize(iCov+1);
    fAsimovSysts[iCov] = vals;
    fSendAsimovSysts = true;
  }

  void SetAsimovOscParameters(OscPars oscpars)
  {
    fAsimovOscPars = oscpars;
    fSendAsimovOsc = true;
  }

  //Setting parameters for Poisson fake data
  void SetPoissonFakeDataSystParameters(CovTypes iCov, std::vector<double> vals)
  {
    if(fPoissonFakeDataSysts.size() <= iCov) fPoissonFakeDataSysts.resize(iCov+1);
    fPoissonFakeDataSysts[iCov] = vals;
    fSendPoissonFakeDataSysts = true;
  }

  void SetPoissonFakeDataOscParameters(OscPars oscpars)
  {
    fPoissonFakeDataOscPars = oscpars;
    fSendPoissonFakeDataOsc = true;
  }

  void ResetToDataFile()
  {
    fResetToDataFile = true;
  }

  double GetLikelihood()
  {

    fBF << int(fSendAsimovOsc);
    if(fSendAsimovOsc) fBF << fAsimovOscPars;

    if(fSendAsimovSysts) {
      fBF << int(fAsimovSysts.size());
      for(const std::vector<double>& s: fAsimovSysts) fBF << s;
    }
    else fBF << 0; //equivalent to an empty fAsimovSysts.size()


    fBF << int(fSendPoissonFakeDataOsc);
    if(fSendPoissonFakeDataOsc) fBF << fPoissonFakeDataOscPars;

    if(fSendPoissonFakeDataSysts) {
      fBF << int(fPoissonFakeDataSysts.size());
      for(const std::vector<double>& s: fPoissonFakeDataSysts) fBF << s;
    }
    else fBF << 0;

    fBF << int(fResetToDataFile);

    fBF << fPars;
    // Bifrost doesn't have explicit support for vector<vector<double>>, just
    // send the size and then loop manually.
    int size = fSysts.size();
    fBF << size;
    for(const std::vector<double>& s: fSysts) fBF << s;
    double llh;
    fBF >> llh;

    fSendAsimovOsc = fSendAsimovSysts = false;
    fSendPoissonFakeDataOsc = fSendPoissonFakeDataSysts = false;
    fResetToDataFile = false;

    return llh;
  }

protected:
  Bifrost& fBF;

  std::vector<std::vector<double>> fSysts;
  OscPars fPars;

  std::vector<std::vector<double>> fAsimovSysts;
  OscPars fAsimovOscPars;
  bool fSendAsimovOsc;
  bool fSendAsimovSysts;

  std::vector<std::vector<double>> fPoissonFakeDataSysts;
  OscPars fPoissonFakeDataOscPars;
  bool fSendPoissonFakeDataOsc;
  bool fSendPoissonFakeDataSysts;

  bool fResetToDataFile;
};
