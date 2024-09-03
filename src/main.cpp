#include <iostream>
#include <filesystem>
#include <vector>

#define SEARCH_DIR "."
#define CONFIG_FILE "quickbuild"

using namespace std;

template <typename T>
void print_vec(vector<T> vec) {
  for (auto const &i : vec) {
    cout << i << "\n";
  }
}

int main(int argc, char** argv) {
  cout << "= Quickbuild Beta v0.1.0\n";

  // Get a list of all files in this diectory, recursively
  cout << "- Scanning directory...";
  vector<filesystem::directory_entry> file_tree;
  auto dir_iterator = filesystem::recursive_directory_iterator(SEARCH_DIR);
  for (auto &entry : dir_iterator) {
    // Ignore .git and directories
    if (entry.is_directory() || entry.path().string().find(".git") != string::npos)
      continue;
    file_tree.push_back(entry);
  }

  
  return EXIT_SUCCESS;
}
