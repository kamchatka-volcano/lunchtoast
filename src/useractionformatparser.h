#include <regex>
#include <string>
#include <tuple>
#include <vector>

namespace lunchtoast {

struct UserActionFormat {
    std::string formatRegex;
    std::vector<int> paramsOrder;
};
UserActionFormat makeUserActionFormat(const std::string& format);

}; //namespace lunchtoast