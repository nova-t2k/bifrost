#pragma once

#include <fstream>
#include <unistd.h>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <sys/types.h>
#include <sys/stat.h>

class Bifrost
{
public:
  // For the host system to start a container
  static std::unique_ptr<Bifrost> Outside(const std::string& img)
  {
    return std::unique_ptr<Bifrost>(new Bifrost(img));
  }

  // For use inside the container
  static Bifrost* Inside()
  {
    return new Bifrost;
  }

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
  Bifrost(const std::string& img);
  Bifrost();

  template<class T> void TypeCheck();

  void WriteEncoded(int x);
  void WriteEncoded(double x);

  template<class T> T ReadEncoded();

  bool fInside;

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

template<class T> std::string TypeTag();
template<> std::string TypeTag<int>() {return "I";}
template<> std::string TypeTag<double>() {return "D";}

template<> std::string TypeTag<std::vector<int>>() {return "VI";}
template<> std::string TypeTag<std::vector<double>>() {return "VD";}

// ----------------------------------------------------------------------------
Bifrost::Bifrost(const std::string& img) : fInside(false)
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
    // Now in the child process
    std::cout << "Bifrost: Outside: starting " << img << std::endl;

    std::vector<const char*> args = {"singularity", "run",
                                     "-B", (fDir+":/bifrost").c_str(),
                                     img.c_str()};
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
Bifrost::Bifrost() : fInside(true), fDir("/bifrost")
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
  fOut << "Q " << std::flush;

  if(!fInside){
    unlink((fDir+"/pipe.in").c_str());
    unlink((fDir+"/pipe.out").c_str());
    rmdir(fDir.c_str());
  }
}

// ----------------------------------------------------------------------------
void Bifrost::WriteEncoded(int x)
{
  fOut << x << " ";
}

// ----------------------------------------------------------------------------
void Bifrost::WriteEncoded(double x)
{
  fOut << TypePun<unsigned long long>(x) << " ";
}

// ----------------------------------------------------------------------------
template<> int Bifrost::ReadEncoded<int>()
{
  int x;
  fIn >> x;
  return x;
}

// ----------------------------------------------------------------------------
template<> double Bifrost::ReadEncoded<double>()
{
  unsigned long long ull;
  fIn >> ull;
  return TypePun<double>(ull);
}

// ----------------------------------------------------------------------------
Bifrost& Bifrost::operator<<(int i)
{
  fOut << TypeTag<int>() << " ";
  WriteEncoded(i);
  fOut << std::flush;
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
  fOut << TypeTag<double>() << " ";
  WriteEncoded(x);
  fOut << std::flush;
  return *this;
}

// ----------------------------------------------------------------------------
Bifrost& Bifrost::operator>>(double& x)
{
  TypeCheck<double>();
  x = ReadEncoded<double>();
  return *this;
}

// ----------------------------------------------------------------------------
template<class T> Bifrost& Bifrost::operator<<(const std::vector<T>& v)
{
  fOut << TypeTag<std::vector<T>>() << " " << v.size() << " ";
  for(const T& x: v) WriteEncoded(x);
  fOut.flush();
  return *this;
}

// ----------------------------------------------------------------------------
template<class T> Bifrost& Bifrost::operator>>(std::vector<T>& v)
{
  TypeCheck<std::vector<T>>();
  size_t N;
  fIn >> N;
  v.clear();
  v.reserve(N);
  for(size_t i = 0; i < N; ++i) v.push_back(ReadEncoded<T>());
  return *this;
}

// ----------------------------------------------------------------------------
template<class T> void Bifrost::TypeCheck()
{
  std::string tag;
  fIn >> tag;

  if(tag == "Q"){
    std::cout << "Bifrost: Inside: received shutdown signal" << std::endl;
    // TODO I guess this could be an exception? But the whole container is
    // going away, so...
    exit(0);
  }

  if(tag != TypeTag<T>()){
    std::cout << "Bifrost: "
              << (fInside ? "Inside: " : "Outside: ")
              << "Type error. Expected '" << TypeTag<T>()
              << "', got '" << tag << "'." << std::endl;
    abort();
  }
}
