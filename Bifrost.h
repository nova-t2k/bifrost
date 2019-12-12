#pragma once

#include <fstream>
#include <unistd.h>
#include <iostream>
#include <string>
#include <vector>

#include <sys/types.h>
#include <sys/stat.h>

class Bifrost
{
public:
  // For the host system to start a container
  Bifrost(const std::string& img);
  // For use inside the container
  Bifrost();

  ~Bifrost();

  Bifrost& operator<<(int i);
  Bifrost& operator<<(double x);
  template<class T> Bifrost& operator<<(const std::vector<T>& v);

  Bifrost& operator>>(int& i);
  Bifrost& operator>>(double& x);
  template<class T> Bifrost& operator>>(std::vector<T>& v);

  Bifrost(const Bifrost&) = delete;
  Bifrost& operator=(const Bifrost&) = delete;
protected:
  template<class T> void TypeCheck();

  std::string fDir;
  std::ofstream fOut;
  std::ifstream fIn;
};

template<class T, class U> T TypePun(U x)
{
  static_assert(sizeof(T) == sizeof(U), "size mismatch in cast");
  union{T t; U u;} p = {.u = x};
  return p.t;
}

template<class T> char TypeTag();
template<> char TypeTag<int>() {return 'I';}
template<> char TypeTag<double>() {return 'D';}

// ----------------------------------------------------------------------------
Bifrost::Bifrost(const std::string& img)
{
  fDir = "/tmp/bifrost.XXXXXX";
  mkdtemp(&fDir.front());
  //  std::cout << "Temp dir " << fDir << std::endl;
  int ret = mkfifo((std::string(fDir)+"/pipe.in" ).c_str(), 0777);
  ret    += mkfifo((std::string(fDir)+"/pipe.out").c_str(), 0777);
  if(ret != 0){
    std::cout << "Bifrost: Outside: Failed to make FIFOs" << std::endl;
    abort();
  }

  if(fork() == 0){
    std::cout << "Bifrost: Outside: starting " << img << std::endl;

    std::vector<const char*> args = {"singularity", "exec",
                                     "-B", "/cvmfs:/cvmfs",
                                     "-B", (fDir+":/bifrost").c_str(),
                                     img.c_str(),
                                     "./inside.sh"};
    args.push_back(0); // null terminate

    execvp("singularity", (char**)&args.front());

    std::cout << "Bifrost: Outside: failed to exec()" << std::endl;
    abort();
  }

  // We have to open the streams here rather than in the initialization list
  // because we need control over the order to avoid deadlocks. NB the
  // alternate order to the Inside variant, and the potentially confusing
  // naming.
  fOut.open(fDir+"/pipe.in");
  if(!fOut.good()){
    std::cout << "Bifrost: Outside: Failed to open " << fDir << "/pipe.in" << std::endl;
    abort();
  }

  fIn.open(fDir+"/pipe.out");
  if(!fIn.good()){
    std::cout << "Bifrost: Outside: Failed to open " << fDir << "/pipe.out" << std::endl;
    abort();
  }
}

// ----------------------------------------------------------------------------
Bifrost::Bifrost() : fDir("/bifrost")
{
  // We have to open the streams here rather than in the initialization list
  // because we need control over the order to avoid deadlocks. NB the
  // alternate order to the Outside variant, and the potentially confusing
  // naming.

  fIn.open(fDir+"/pipe.in");
  if(!fIn.good()){
    std::cout << "Bifrost: Inside: Failed to open " << fDir << "/pipe.in" << std::endl;
    abort();
  }

  fOut.open(fDir+"/pipe.out");
  if(!fOut.good()){
    std::cout << "Bifrost: Inside: Failed to open " << fDir << "/pipe.out" << std::endl;
    abort();
  }
}

// ----------------------------------------------------------------------------
Bifrost::~Bifrost()
{
  if(fDir != "/bifrost"){
    unlink((fDir+"/pipe.in").c_str());
    unlink((fDir+"/pipe.out").c_str());
    rmdir(fDir.c_str());
  }
}

// ----------------------------------------------------------------------------
Bifrost& Bifrost::operator<<(int i)
{
  fOut << TypeTag<int>() << i << " " << std::flush;
  return *this;
}

// ----------------------------------------------------------------------------
Bifrost& Bifrost::operator>>(int& i)
{
  TypeCheck<int>();
  fIn >> i;
  return *this;
}

// ----------------------------------------------------------------------------
Bifrost& Bifrost::operator<<(double x)
{
  fOut << TypeTag<double>() << TypePun<unsigned long long>(x) << " " << std::flush;
  return *this;
}

// ----------------------------------------------------------------------------
Bifrost& Bifrost::operator>>(double& x)
{
  TypeCheck<double>();
  unsigned long long ull;
  fIn >> ull;
  x = TypePun<double>(ull);
  return *this;
}

// ----------------------------------------------------------------------------
template<class T> Bifrost& Bifrost::operator<<(const std::vector<T>& v)
{
  fOut << "V" << v.size() << " ";
  for(const T& x: v) *this << x;
  return *this;
}

// ----------------------------------------------------------------------------
template<class T> Bifrost& Bifrost::operator>>(std::vector<T>& v)
{
  TypeCheck<std::vector<T>>();
  size_t N;
  fIn >> N;
  v.reserve(N);
  for(size_t i = 0; i < N; ++i){
    T x;
    fIn >> x;
    v.push_back(x);
  }
  return *this;
}

// ----------------------------------------------------------------------------
template<class T> void Bifrost::TypeCheck()
{
  char tag;
  fIn >> tag;
  if(tag != TypeTag<T>()){
    std::cout << "Bifrost: Type error. Expected '" << TypeTag<T>()
              << "', got '" << tag << "'." << std::endl;
    abort();
  }
}
