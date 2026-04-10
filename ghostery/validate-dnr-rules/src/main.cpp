#include "config.h"

#include <WebCore/CombinedURLFilters.h>
#include <WebCore/URLFilterParser.h>
#include <wtf/JSONValues.h>
#include <wtf/text/WTFString.h>

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

using namespace WebCore::ContentExtensions;

struct ValidationResult {
    int rulesTotal { 0 };
    int rulesValid { 0 };
    int errors { 0 };
};

static ValidationResult validateFile(const char* path)
{
    ValidationResult result;

    FILE* fp = fopen(path, "rb");
    if (!fp) {
        fprintf(stderr, "ERROR: Cannot open file: %s\n", path);
        result.errors = 1;
        return result;
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
        result.errors = 1;
        return result;
    }

    auto rulesArray = jsonValue->asArray();
    if (!rulesArray) {
        fprintf(stderr, "ERROR: JSON is not an array: %s\n", path);
        result.errors = 1;
        return result;
    }

    printf("=== %s ===\n", path);
    printf("Rules: %u\n", rulesArray->length());

    for (unsigned i = 0; i < rulesArray->length(); ++i) {
        auto ruleObject = rulesArray->get(i)->asObject();
        if (!ruleObject)
            continue;

        result.rulesTotal++;

        auto conditionObject = ruleObject->getObject("condition"_s);
        if (!conditionObject) {
            result.rulesValid++;
            continue;
        }

        auto ruleId = ruleObject->getInteger("id"_s).value_or(-1);
        bool caseSensitive = conditionObject->getBoolean("isUrlFilterCaseSensitive"_s).value_or(false);

        String regexFilter = conditionObject->getString("regexFilter"_s);

        if (regexFilter.isEmpty()) {
            result.rulesValid++;
            continue;
        }

        CombinedURLFilters combinedFilters;
        URLFilterParser parser(combinedFilters);
        auto status = parser.addPattern(regexFilter, caseSensitive, 0);

        if (status == URLFilterParser::Ok || status == URLFilterParser::MatchesEverything) {
            result.rulesValid++;
        } else {
            result.errors++;
            printf("  ERROR: Rule %d: %s — %s\n", static_cast<int>(ruleId),
                URLFilterParser::statusString(status).characters(),
                regexFilter.utf8().data());
        }
    }

    printf("Valid: %d/%d\n\n", result.rulesValid, result.rulesTotal);
    return result;
}

int main(int argc, const char* argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: validate-dnr-rules <rules.json> [...]\n");
        return 1;
    }

    int totalErrors = 0;
    for (int i = 1; i < argc; i++) {
        auto result = validateFile(argv[i]);
        totalErrors += result.errors;
    }

    if (totalErrors == 0)
        printf("OK: All rules validated successfully.\n");
    else
        printf("FAILED: %d error(s) found.\n", totalErrors);

    return totalErrors > 0 ? 1 : 0;
}
