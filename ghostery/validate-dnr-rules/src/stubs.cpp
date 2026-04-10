#include "config.h"
#include <cstdio>
#include <cstdlib>
#include <JavaScriptCore/Yarr.h>
#include <JavaScriptCore/YarrUnicodeProperties.h>
#include <wtf/text/WTFString.h>

// WTF assertion stubs — these are normally provided by libWTF
extern "C" {

void WTFCrash()
{
    fprintf(stderr, "WTFCrash\n");
    abort();
}

void WTFCrashWithSecurityImplication()
{
    fprintf(stderr, "WTFCrashWithSecurityImplication\n");
    abort();
}

void WTFReportAssertionFailure(const char* file, int line, const char* function, const char* assertion)
{
    fprintf(stderr, "ASSERTION FAILED: %s (%s:%d %s)\n", assertion, file, line, function);
}

void WTFReportAssertionFailureWithMessage(const char* file, int line, const char* function, const char* assertion, const char* format, ...)
{
    fprintf(stderr, "ASSERTION FAILED: %s (%s:%d %s)\n", assertion, file, line, function);
}

void WTFReportBacktrace()
{
}

} // extern "C"

// YARR Unicode property stubs — these are only needed for \p{} property
// escapes which DNR regexFilter patterns never use.
namespace JSC::Yarr {

bool characterClassMayContainStrings(BuiltInCharacterClassID)
{
    return false;
}

std::optional<BuiltInCharacterClassID> unicodeMatchProperty(WTF::String, CompileMode)
{
    return std::nullopt;
}

std::optional<BuiltInCharacterClassID> unicodeMatchPropertyValue(WTF::String, WTF::String)
{
    return std::nullopt;
}

} // namespace JSC::Yarr
