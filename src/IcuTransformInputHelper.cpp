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

#include "IcuTransformInputHelper.h"

#include <unicode/translit.h>
#include <unicode/unistr.h>

#include <algorithm>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace McBopomofo {
namespace IcuTransformInputHelper {

static icu::Transliterator* getTransliterator(const std::string& id) {
  static std::unordered_map<std::string, std::unique_ptr<icu::Transliterator>>
      cache;
  auto it = cache.find(id);
  if (it == cache.end()) {
    UErrorCode status = U_ZERO_ERROR;
    auto* tr = icu::Transliterator::createInstance(
        icu::UnicodeString::fromUTF8(id), UTRANS_FORWARD, status);
    it = cache.emplace(id, std::unique_ptr<icu::Transliterator>(tr)).first;
  }
  return it->second.get();
}

static std::string transformString(const std::string& id,
                                   const std::string& input) {
  auto* tr = getTransliterator(id);
  icu::UnicodeString uInput = icu::UnicodeString::fromUTF8(input);
  tr->transliterate(uInput);
  std::string output;
  uInput.toUTF8String(output);
  return output;
}

std::vector<std::string> FillCandidatesWithString(const std::string& string) {
  std::vector<std::string> candidates;
  static const std::vector<std::string> transforms = {
      "Latin-Hiragana",    // Hiragana script
      "Latin-Katakana",    // Katakana script
      "Latin-Hangul",      // Hangul script
      "Latin-Thai",        // Thai script
      "Latin-Greek",       // Greek script
      "Latin-Cyrillic",    // Cyrillic script
      "Latin-Arabic",      // Arabic script
      "Latin-Hebrew",      // Hebrew script
      "Latin-Devanagari",  // Devanagari script
  };
  for (const auto& transform : transforms) {
    std::string transformed = transformString(transform, string);
    if (!transformed.empty()) {
      candidates.emplace_back(transformed);
    }
  }
  return candidates;
}
}  // namespace IcuTransformInputHelper
}  // namespace McBopomofo