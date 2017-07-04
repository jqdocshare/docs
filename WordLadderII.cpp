class Solution {
 public:
  std::vector<std::vector<std::string>> findLadders(
      const std::string &beginWord,
      const std::string &endWord,
      std::vector<std::string> &wordList) {
    const int beginIndex = wordList.size();
    wordList.emplace_back(beginWord);

    const auto endWordIt = std::find(wordList.begin(), wordList.end(), endWord);
    if (endWordIt == wordList.end()) {
      return {};
    }

    const int endIndex = std::distance(wordList.begin(), endWordIt);
    const int n = wordList.size();
    const int m = beginWord.length();

    std::vector<std::vector<int>> edges(n);
    for (auto &vec : edges) {
      vec.reserve(32);
    }

    // Initialize the graph edges.
    if (n < 65536 && m <= 5) {
      switch (m) {
        case 1: initEdgesCompCodePos<1u>(wordList, edges); break;
        case 2: initEdgesCompCodePos<2u>(wordList, edges); break;
        case 3: initEdgesCompCodePos<3u>(wordList, edges); break;
        case 4: initEdgesCompCodePos<4u>(wordList, edges); break;
        case 5: initEdgesCompCodePos<5u>(wordList, edges); break;
      }
    } else if (m <= 7) {
      switch (m) {
        case 1: initEdgesCompCode<1u, std::uint32_t>(wordList, edges); break;
        case 2: initEdgesCompCode<2u, std::uint32_t>(wordList, edges); break;
        case 3: initEdgesCompCode<3u, std::uint32_t>(wordList, edges); break;
        case 4: initEdgesCompCode<4u, std::uint64_t>(wordList, edges); break;
        case 5: initEdgesCompCode<5u, std::uint64_t>(wordList, edges); break;
        case 6: initEdgesCompCode<6u, std::uint64_t>(wordList, edges); break;
        case 7: initEdgesCompCode<7u, std::uint64_t>(wordList, edges); break;
      }
    } else {
      initEdges(wordList, edges);
    }

    // BFS.
    std::vector<char> visited(n, 0);
    std::vector<std::vector<int>> chain(n);
    for (auto &vec : chain) {
      vec.reserve(16);
    }

    std::vector<int> curr = { endIndex };
    std::vector<int> next;
    curr.reserve(64);
    next.reserve(64);

    visited[endIndex] = 2;
    int pathLen = 1;
    while (!curr.empty() && visited[beginIndex] == 0) {
      next.clear();
      for (const int src : curr) {
        for (const int dst : edges[src]) {
          const char vt = visited[dst];
          if (vt == 0) {
            next.emplace_back(dst);
            visited[dst] = 1;
            chain[dst].emplace_back(src);
          } else if (vt == 1) {
            chain[dst].emplace_back(src);
          }
        }
      }
      for (const int v : next) {
        visited[v] = 2;
      }
      curr.swap(next);
      ++pathLen;
    }

    if (visited[beginIndex] == 0) {
      return {};
    }

    // Stack-based backtracking.
    std::stack<State> stk;
    std::vector<std::vector<std::string>> results;

    std::vector<std::string> ladder;
    ladder.reserve(pathLen);
    ladder.emplace_back(beginWord);

    stk.emplace(std::move(ladder), chain[beginIndex].begin(), chain[beginIndex].end() - 1);

    while (!stk.empty()) {
      auto &state = stk.top();
      int v = *state.iter;

      std::vector<std::string> ladder;
      if (state.iter == state.last) {
        ladder = std::move(state.ladder);
        stk.pop();
      } else {
        ladder = state.ladder;
        ladder.reserve(pathLen);
        ++state.iter;
      }

      while (v != endIndex && chain[v].size() == 1) {
        ladder.emplace_back(wordList[v]);
        v = chain[v].front();
      }
      ladder.emplace_back(wordList[v]);

      if (v == endIndex) {
        results.emplace_back(std::move(ladder));
      } else {
        stk.emplace(std::move(ladder), chain[v].begin(), chain[v].end() - 1);
      }
    }

    return results;
  }

 private:
  struct State {
    inline State(std::vector<std::string> &&ladder_in,
                 const std::vector<int>::iterator &iter_in,
                 const std::vector<int>::iterator &last_in)
        : ladder(std::move(ladder_in)), iter(iter_in), last(last_in) {}

    std::vector<std::string> ladder;
    std::vector<int>::iterator iter;
    std::vector<int>::iterator last;
  };

  void initEdges(
      std::vector<std::string> &wordList,
      std::vector<std::vector<int>> &edges) const {
    const int wordLength = wordList.front().length();
    std::vector<std::unordered_map<std::string, std::vector<int>>> groups(wordLength);

    for (int i = 0; i < wordList.size(); ++i) {
      std::string &word = wordList[i];
      for (int j = 0; j < wordLength; ++j) {
        char c = word[j];
        word[j] = ' ';
        groups[j][word].emplace_back(i);
        word[j] = c;
      }
    }

    for (const auto &group : groups) {
      for (const auto &it : group) {
        const auto &vec = it.second;
        const int n = vec.size();

        if (n > 1) {
          for (int i = 0; i < n; ++i) {
            for (int j = i + 1; j < n; ++j) {
              const int src = vec[i];
              const int dst = vec[j];
              edges[src].emplace_back(dst);
              edges[dst].emplace_back(src);
            }
          }
        }
      }
    }
  }

  template <std::size_t m, typename KeyCode>
  void initEdgesCompCode(
      std::vector<std::string> &wordList,
      std::vector<std::vector<int>> &edges) const {
    constexpr std::uint64_t kByteMask = (1ull << 8) - 1;
    const int n = wordList.size();
    std::vector<std::pair<KeyCode, int>> entries;
    entries.reserve(n * m);

    for (int i = 0; i < wordList.size(); ++i) {
      KeyCode orig_code = 0;
      std::memcpy(&orig_code, wordList[i].c_str(), m);
      for (std::uint64_t j = 0; j < m; ++j) {
        entries.emplace_back(
            ((orig_code | (kByteMask << (j * 8))) << 8) | j, i);
      }
    }

    std::sort(entries.begin(), entries.end(),
              [](const std::pair<KeyCode, int> &lhs,
                 const std::pair<KeyCode, int> &rhs) -> bool {
                  return lhs.first < rhs.first;
              });

    const int numEntries = entries.size();
    int l = 0;
    while (l < numEntries) {
      KeyCode norm_code = entries[l].first;
      int r = l + 1;
      while (r < numEntries && (entries[r].first) == norm_code) {
        ++r;
      }
      if (r - l > 1) {
        for (int i = l; i < r - 1; ++i) {
          for (int j = i + 1; j < r; ++j) {
            const int src = entries[i].second;
            const int dst = entries[j].second;
            edges[src].emplace_back(dst);
            edges[dst].emplace_back(src);
          }
        }
      }
      l = r;
    }
  }

  template <std::size_t m>
  void initEdgesCompCodePos(
      std::vector<std::string> &wordList,
      std::vector<std::vector<int>> &edges) const {
    constexpr std::uint64_t kByteMask = (1ull << 8) - 1;
    const int n = wordList.size();

    std::vector<std::uint64_t> codes;
    codes.reserve(n * m);

    for (std::uint64_t i = 0; i < wordList.size(); ++i) {
      std::uint64_t orig_code = 0;
      std::memcpy(&orig_code, wordList[i].c_str(), m);
      for (std::uint64_t j = 0; j < m; ++j) {
        codes.emplace_back(((orig_code | (kByteMask << (j * 8))) << 24) | (j << 16) | i);
      }
    }

    std::sort(codes.begin(), codes.end());

    const int numCodes = codes.size();
    int l = 0;
    while (l < numCodes) {
      std::uint64_t norm_code = codes[l] >> 16;
      int r = l + 1;
      while (r < numCodes && (codes[r] >> 16) == norm_code) {
        ++r;
      }
      if (r - l > 1) {
        for (int i = l; i < r - 1; ++i) {
          for (int j = i + 1; j < r; ++j) {
            const int src = static_cast<std::uint16_t>(codes[i]);
            const int dst = static_cast<std::uint16_t>(codes[j]);
            edges[src].emplace_back(dst);
            edges[dst].emplace_back(src);
          }
        }
      }
      l = r;
    }
  }
};
