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
    OscPars asimovOscPars;
    int gotSentAsimovOsc;

    bf >> gotSentAsimovOsc;

    if(gotSentAsimovOsc) {
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
  ReceiveLLHOverBifrost(Bifrost& bf) : fBF(bf), fSendAsimovOsc(false)
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
  }

  void SetAsimovOscParameters(OscPars oscpars)
  {
    fAsimovOscPars = oscpars;
    fSendAsimovOsc = true;
  }

  double GetLikelihood()
  {

    fBF << int(fSendAsimovOsc);

    if(fSendAsimovOsc) fBF << fAsimovOscPars;
    int syst_size = fAsimovSysts.size();

    fBF << syst_size;
    for(const std::vector<double>& s: fAsimovSysts) fBF << s;

    fBF << fPars;
    // Bifrost doesn't have explicit support for vector<vector<double>>, just
    // send the size and then loop manually.
    int size = fSysts.size();
    fBF << size;
    for(const std::vector<double>& s: fSysts) fBF << s;
    double llh;
    fBF >> llh;

    fSendAsimovOsc = false;
    fAsimovSysts.clear();
    return llh;
  }

protected:
  Bifrost& fBF;

  std::vector<std::vector<double>> fSysts;
  std::vector<std::vector<double>> fAsimovSysts;
  OscPars fPars;
  OscPars fAsimovOscPars;
  bool fSendAsimovOsc;

};
