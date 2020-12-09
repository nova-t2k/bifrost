#include "Bifrost.h"

int main()
{
  // change that to whatever the sif image after doing
  // TMPDIR=/var/tmp singularity pull docker://novaexperiment/nova-sl7-novat2k:latest
  auto bf = Bifrost::Outside("nova-sl7-novat2k_latest.sif");
  std::vector<double> syst (58, 0.);

  // NOTE This is **NOT** the standard way to use bifrost, you should instanciate ReceiveLLHOverBifrost(*bf)
  // and then use the ReceiveLLHOverBifrost::SetParameters(), SetOscParameters() and GetLikelihood() functions.
  // This is for testing only.
  // Sample code:
  //    ReceiveLLHOverBifrost receiver(*bf);
  //    receiver.SetParameters(kNOvAdet, syst);
  //    receiver.SetOscParameter(OscPars);
  //    double llh = receiver.GetLikelihood();

  
  // (*bf) << p.dm32 << p.dm21 << p.sth13 << p.sth12 << p.sth23 << p.dcp;
  (*bf) << 2.41e-3 << 7.53e-5 << 2.18e-2 << .307 << 0.785 << 2.576;
  (*bf) << syst;
  
  double llh;
  (*bf) >> llh;

  std::cout << "Likelihood returned is: " << llh << "\n";
  
  return 0;
}
