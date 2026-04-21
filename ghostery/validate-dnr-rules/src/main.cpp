#include "config.h"

#include <WebCore/CombinedURLFilters.h>
#include <WebCore/URLFilterParser.h>
#include <wtf/JSONValues.h>
#include <wtf/text/MakeString.h>
#include <wtf/text/StringBuilder.h>
#include <wtf/text/WTFString.h>

#include <cstdio>
#include <string>

using namespace WebCore::ContentExtensions;

// Mirrors -[_WKWebExtensionDeclarativeNetRequestRule _regexURLFilterForChromeURLFilter:]:
// strip `||`/`|` anchors, escape regex metachars, expand `*` and `^`, re-apply anchors.
static String regexFromURLFilter(String urlFilter)
{
    bool hasDomainAnchor = urlFilter.startsWith("||"_s);
    if (hasDomainAnchor)
        urlFilter = urlFilter.substring(2);

    bool hasStartAnchor = !hasDomainAnchor && urlFilter.startsWith('|');
    if (hasStartAnchor)
        urlFilter = urlFilter.substring(1);

    bool hasEndAnchor = urlFilter.endsWith('|');
    if (hasEndAnchor)
        urlFilter = urlFilter.left(urlFilter.length() - 1);

    StringBuilder escaped;
    for (unsigned i = 0; i < urlFilter.length(); ++i) {
        char16_t c = urlFilter[i];
        switch (c) {
        case '?': case '+': case '[': case '(': case ')':
        case '{': case '}': case '$': case '|': case '\\': case '.':
            escaped.append('\\');
        }
        escaped.append(c);
    }
    String regex = escaped.toString();
    regex = makeStringByReplacingAll(regex, '*', ".*"_s);
    regex = makeStringByReplacingAll(regex, '^', "[^a-zA-Z0-9_.%-]"_s);

    if (hasDomainAnchor)
        regex = makeString("^[^:]+://+([^:/]+\\.)?"_s, regex);
    if (hasStartAnchor)
        regex = makeString('^', regex);
    if (hasEndAnchor)
        regex = makeString(regex, '$');

    return regex;
}

static int validateFile(const char* path)
{
    FILE* fp = fopen(path, "rb");
    if (!fp) {
        fprintf(stderr, "ERROR: Cannot open file: %s\n", path);
        return 1;
    }

    fseek(fp, 0, SEEK_END);
    long fileSize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    std::string contents(fileSize, '\0');
    fread(contents.data(), 1, fileSize, fp);
    fclose(fp);

    auto jsonString = String::fromUTF8(std::span(reinterpret_cast<const uint8_t*>(contents.data()), contents.size()));
    auto jsonValue = JSON::Value::parseJSON(jsonString);
    if (!jsonValue) {
        fprintf(stderr, "ERROR: Invalid JSON: %s\n", path);
        return 1;
    }

    auto rulesArray = jsonValue->asArray();
    if (!rulesArray) {
        fprintf(stderr, "ERROR: JSON is not an array: %s\n", path);
        return 1;
    }

    printf("=== %s ===\nRules: %zu\n", path, rulesArray->length());

    int errors = 0;
    int valid = 0;
    int total = 0;

    for (size_t i = 0; i < rulesArray->length(); ++i) {
        auto ruleObject = rulesArray->get(i)->asObject();
        if (!ruleObject)
            continue;

        total++;

        auto conditionObject = ruleObject->getObject("condition"_s);
        if (!conditionObject) {
            valid++;
            continue;
        }

        String pattern = conditionObject->getString("regexFilter"_s);
        const char* kind = "regex";
        String original = pattern;
        if (pattern.isEmpty()) {
            original = conditionObject->getString("urlFilter"_s);
            if (original.isEmpty()) {
                valid++;
                continue;
            }
            pattern = regexFromURLFilter(original);
            kind = "url";
        }

        bool caseSensitive = conditionObject->getBoolean("isUrlFilterCaseSensitive"_s).value_or(false);

        CombinedURLFilters combinedFilters;
        URLFilterParser parser(combinedFilters);
        auto status = parser.addPattern(pattern, caseSensitive, 0);

        if (status == URLFilterParser::Ok || status == URLFilterParser::MatchesEverything) {
            valid++;
            continue;
        }

        errors++;
        auto ruleId = ruleObject->getInteger("id"_s).value_or(-1);
        printf("  ERROR: Rule %d [%s]: %s — %s\n", static_cast<int>(ruleId), kind,
            URLFilterParser::statusString(status).characters(),
            original.utf8().data());
    }

    printf("Valid: %d/%d\n\n", valid, total);
    return errors;
}

int main(int argc, const char* argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: validate-dnr-rules <rules.json> [...]\n");
        return 1;
    }

    int totalErrors = 0;
    for (int i = 1; i < argc; i++)
        totalErrors += validateFile(argv[i]);

    if (totalErrors)
        printf("FAILED: %d error(s) found.\n", totalErrors);
    else
        printf("OK: All rules validated successfully.\n");

    return totalErrors ? 1 : 0;
}
