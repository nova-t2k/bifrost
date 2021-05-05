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
    OscPars p;
    bf >> p;
    llh->SetOscParameters(p);

    unsigned int nsystvecs;
    bf >> nsystvecs;

    for(unsigned int i = 0; i < nsystvecs; ++i){
      std::vector<double> systs;
      bf >> systs;
      llh->SetParameters(i, systs);
    }

    bf << llh->GetLikelihood();
  }
}

/// For use from outside the container
class ReceiveLLHOverBifrost : public TemplateLLHGetter
{
public:
  ReceiveLLHOverBifrost(Bifrost& bf) : fBF(bf)
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

  double GetLikelihood()
  {
    fBF << fPars;
    // Bifrost doesn't have explicit support for vector<vector<double>>, just
    // send the size and then loop manually.
    fBF << fSysts.size();
    for(const std::vector<double>& s: fSysts) fBF << s;
    double llh;
    fBF >> llh;
    return llh;
  }

protected:
  Bifrost& fBF;

  std::vector<std::vector<double>> fSysts;
  OscPars fPars;
};
