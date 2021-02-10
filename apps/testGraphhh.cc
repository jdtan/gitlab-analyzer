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

int main(int argc, char *argv[])
{
  const std::string pattern_name(argv[1]);

  Peregrine::SmallGraph tempGraph = Peregrine::SmallGraph (pattern_name);

  tempGraph.to_string(0);

  return 0;
}
