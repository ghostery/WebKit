#include "config.h"
#include <JavaScriptCore/Yarr.h>
#include <JavaScriptCore/YarrUnicodeProperties.h>
#include <wtf/text/WTFString.h>

// Gigacage stub — not needed when using system malloc
namespace Gigacage {
void ensureGigacage() { }
}

// YARR Unicode property stubs — only needed for \p{} property
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
