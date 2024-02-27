/* There's is an input.txt provided, the recommended (but surely not required) path to test with it is
 * 2 1 2 3 3
 * That's it for the comments, hope, the code is self-explanatory.
 */

#include <iostream>
#include <fstream>
#include <string_view>
#include <vector>
#include <unordered_set>


const std::string DEFAULT_SEPS = " ,.!?&()[]:;\n\r\\";

class DictCorrector {
    std::unordered_set<std::string> dictionary_;

    static std::string read_all(const std::string& filename, const char sep) {
        std::ifstream input_file(filename);
        std::string ret;
        std::string line;
        while (std::getline(input_file, line, sep)) {
            ret += line;
            ret += sep;
        }
        return ret;
    }

    // tokenize, basically, performs this manipulation:
    // word1 sep1 word2 sep2 ... -> {{word1, sep1}, {word2, sep2}}
    static std::vector<std::pair<std::string_view, std::string_view>>
    tokenize(std::string_view view, const std::string_view seps) {
        std::vector<std::pair<std::string_view, std::string_view>> ret;

        if (const size_t non_sep_pos = view.find_first_not_of(seps); non_sep_pos != 0) {
            if (non_sep_pos == std::string_view::npos) {
                return {{{}, {view}}};
            }
            ret.push_back({{}, {view.substr(0, non_sep_pos)}});
            view = view.substr(non_sep_pos, std::string_view::npos);
        }

        while (!view.empty()) {
            const auto sep_pos = view.find_first_of(seps);
            const auto word = view.substr(0, sep_pos);
            auto sep = std::string_view{};
            if (sep_pos != std::string_view::npos) {
                view = view.substr(sep_pos, std::string_view::npos);
                const auto next_word_pos = view.find_first_not_of(seps);
                sep = view.substr(0, next_word_pos);
                if (next_word_pos != std::string_view::npos) {
                    view = view.substr(next_word_pos, std::string_view::npos);
                } else {
                    view = {};
                }
            }

            ret.emplace_back(word, sep);
        }
        return ret;
    }

    static std::size_t levenshtein_dist(const std::string_view lhs, const std::string_view rhs) {
        if (lhs.empty())
            return rhs.size();
        if (rhs.empty())
            return lhs.size();

        auto ret = std::vector(lhs.size() + 1, std::vector<size_t>(rhs.size() + 1, 0));

        for (size_t i = 0; i <= lhs.size(); i++)
            ret[i][0] = i;
        for (size_t j = 0; j <= rhs.size(); j++)
            ret[0][j] = j;

        for (size_t i = 1; i <= lhs.size(); i++) {
            for (size_t j = 1; j <= rhs.size(); j++) {
                const size_t mod = (rhs[j - 1] != lhs[i - 1]);
                ret[i][j] = std::min(
                    std::min(
                        ret[i - 1][j] + 1,
                        ret[i][j - 1] + 1
                    ),
                    ret[i - 1][j - 1] + mod
                );
            }
        }

        return ret[lhs.size()][rhs.size()];
    }

    std::vector<std::string_view> get_closest(const std::string_view target) const {
        std::vector<std::string_view> ret;
        for (const auto& word : this->dictionary_) {
            if (levenshtein_dist(word, target) <= 1) {
                ret.push_back(word);
            }
        }
        return ret;
    }

    std::string on_missing_word_internal(std::string word) {
        const auto closest = this->get_closest(word);
        std::cout << "MISSING WORD '" << word << "' WHAT DO WE DO ABOUT IT?\n"
            "1. Keep it, add to the dictionary\n"
            "2. Keep it, DO NOT add to the dictionary\n"
            "3. Replace with a word from dictionary (" << closest.size() << " variants)\n";
        std::cout << "ENTER THE NUMBER : " << std::flush;
        size_t choise;
        std::cin >> choise;
        std::string ret;

        switch (choise) {
        case 1: {
            this->dictionary_.insert(word);
            return word;
        }
        case 2: {
            return word;
        }
        case 3: {
            if (closest.empty()) {
                throw std::invalid_argument("THERES NO SIMILAR WORDS, CANT REPLACE");
            }

            std::cout << "NOW CHOOSE THE WORD:\n";
            for (size_t i = 0; i < closest.size(); i++) {
                std::cout << (i + 1) << ". '" << closest[i] << "'\n";
            }
            std::cout << "ENTER THE NUMBER : " << std::flush;
            size_t sub_choise;
            std::cin >> sub_choise;

            if (sub_choise > closest.size() || sub_choise == 0) {
                throw std::invalid_argument("THE CHOSEN NUMBER IS NOT THE LIST");
            }

            return std::string{closest[sub_choise - 1]};
        }
        default:
            throw std::invalid_argument("THE CHOSEN NUMBER IS NOT THE LIST");
        }
    }

    std::string on_missing_word(const std::string& word) {
        std::string ret;
        try {
            ret = on_missing_word_internal(word);
        } catch (std::invalid_argument& e) {
            std::cout << "\nINVALID ARGRUMENT: " << e.what() << "\nRETRY! (pls, sorry for shouting)\n" << std::endl;
            // recursion is ok, 100+ user fails wouldnt crash the programm
            ret = on_missing_word(word);
        } catch (std::exception& e) {
            std::cout << "\nUNEXPECTED ERROR: " << e.what() << std::endl;
        }
        return ret;
    }

public:
    static DictCorrector from_file(const std::string& filename, const std::string& seps = DEFAULT_SEPS) {
        DictCorrector ret;
        const auto data = read_all(filename, '\n');
        for (auto [word, _] : tokenize(data, seps)) {
            ret.dictionary_.insert(std::string{word});
        }
        return ret;
    }

    void correct_file(const std::string& src, const std::string& dst, const std::string& seps = DEFAULT_SEPS) {
        std::ofstream fout(dst);
        std::vector<std::string> corrected;
        const auto src_data = read_all(src, '\n');
        for (auto [word, sep] : tokenize(src_data, seps)) {
            auto token = std::string{word};
            if (!this->dictionary_.contains(token)) {
                token = on_missing_word(token);
            }
            fout << token << sep;
        }
    }
};


int main() {
    auto dict = DictCorrector::from_file("dict.txt");

    dict.correct_file("input.txt", "output.txt");

    return 0;
}
