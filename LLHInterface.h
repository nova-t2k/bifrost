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
  OscPars p;
  std::vector<double> systs;

  while(true){
    bf >> p >> systs;
    llh->SetOscParameters(p);
    llh->SetParameters(kNOvAdet, systs);
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
    fSysts = vals; // TODO ignoring iCov
  }

  void SetOscParameters(OscPars oscpars)
  {
    fPars = oscpars;
  }

  double GetLikelihood()
  {
    fBF << fPars << fSysts;
    double llh;
    fBF >> llh;
    return llh;
  }

protected:
  Bifrost& fBF;

  std::vector<double> fSysts;
  OscPars fPars;
};
