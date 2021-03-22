#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "Graph.hh"


bool is_directory(const std::string &path)
{
   struct stat statbuf;
   if (stat(path.c_str(), &statbuf) != 0)
       return 0;
   return S_ISDIR(statbuf.st_mode);
}

int main()
{
  std::string fileList[] = {"data/TestCase1.txt", "data/TestCase2.txt", "data/TestCase3.txt", "data/TestCase4.txt"};
  
  for (auto fileName : fileList) {
    std::cout << "\n\n-----" << "Test " << fileName << "-----\n";
    Peregrine::SmallGraph tempGraph = Peregrine::SmallGraph(fileName);
    tempGraph.to_string_debug();

    Peregrine::AnalyzedPattern analyzedPattern = Peregrine::AnalyzedPattern(tempGraph);
    std::cout << "\n\n";
  }

  return 0;
}
