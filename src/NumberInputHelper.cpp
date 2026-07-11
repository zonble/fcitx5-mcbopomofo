// Copyright (c) 2022 and onwards The McBopomofo Authors.
//
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use,
// copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following
// conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.

#include "NumberInputHelper.h"

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

#include "ChineseNumbers/ChineseNumbers.h"
#include "ChineseNumbers/SuzhouNumbers.h"
#include "RomanNumbers/RomanNumbers.h"

namespace McBopomofo {
namespace NumberInputHelper {

std::vector<std::string> FillCandidatesWithNumber(
    std::string number,
    std::shared_ptr<Formosa::Gramambular2::LanguageModel> languageModel) {
  std::vector<std::string> candidates;
  if (number.empty()) {
    return candidates;
  }

  std::stringstream intStream;
  std::stringstream decStream;
  bool commonFound = false;

  for (char c : number) {
    if (c == '.') {
      commonFound = true;
      continue;
    }
    if (commonFound) {
      decStream << c;
    } else {
      intStream << c;
    }
  }
  std::string intPart = intStream.str();
  std::string decPart = decStream.str();

  candidates.emplace_back(ChineseNumbers::Generate(
      intPart, decPart, ChineseNumbers::ChineseNumberCase::LOWERCASE));
  candidates.emplace_back(ChineseNumbers::Generate(
      intPart, decPart, ChineseNumbers::ChineseNumberCase::UPPERCASE));
  if (decPart.empty()) {
    int value = std::atoi(intPart.c_str());
    if (value > 0 && value <= 3999) {
      candidates.emplace_back(RomanNumbers::ConvertFromInt(
          value, RomanNumbers::RomanNumbersStyle::ALPHABETS));
      candidates.emplace_back(RomanNumbers::ConvertFromInt(
          value, RomanNumbers::RomanNumbersStyle::FULL_WIDTH_UPPER));
      candidates.emplace_back(RomanNumbers::ConvertFromInt(
          value, RomanNumbers::RomanNumbersStyle::FULL_WIDTH_LOWER));
    }
  }

  std::string key = "_number_" + number;
  if (languageModel->hasUnigrams(key)) {
    auto unigrams = languageModel->getUnigrams(key);
    for (const auto& unigram : unigrams) {
      if (std::find(candidates.begin(), candidates.end(), unigram.value()) ==
          candidates.end()) {
        candidates.emplace_back(unigram.value());
      }
    }
  }

  candidates.emplace_back(
      SuzhouNumbers::Generate(intPart, decPart, "單位", true));

  return candidates;
}

}  // namespace NumberInputHelper
}  // namespace McBopomofo
