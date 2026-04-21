#include "config.h"
#include <JavaScriptCore/Yarr.h>
#include <JavaScriptCore/YarrUnicodeProperties.h>
#include <wtf/text/WTFString.h>

// Gigacage/bmalloc stubs — not needed when using system malloc
#include <bmalloc/BPlatform.h>
#include <bmalloc/HeapKind.h>
#include <bmalloc/CompactAllocationMode.h>

namespace Gigacage {
void ensureGigacage() { }
}

extern "C" __attribute__((visibility("default"))) bool disablePrimitiveGigacageRequested = false;

namespace bmalloc::api {
void commitAlignedPhysical(void*, size_t, HeapKind) { }
void decommitAlignedPhysical(void*, size_t, HeapKind) { }
void disableScavenger() { }
void enableMiniMode(bool) { }
void forceEnablePGM(unsigned short) { }
void freeLargeVirtual(void*, size_t, HeapKind) { }
bool isEnabled(HeapKind) { return false; }
void scavenge() { }
void scavengeThisThread() { }
void* tryLargeZeroedMemalignVirtual(size_t, size_t, CompactAllocationMode, HeapKind) { return nullptr; }
}

// YARR Unicode property stubs — only needed for \p{} property
// escapes which DNR regexFilter patterns never use.
namespace JSC::Yarr {

bool characterClassMayContainStrings(BuiltInCharacterClassID)
{
    std::abort();
}

std::optional<BuiltInCharacterClassID> unicodeMatchProperty(WTF::String, CompileMode)
{
    std::abort();
}

std::optional<BuiltInCharacterClassID> unicodeMatchPropertyValue(WTF::String, WTF::String)
{
    std::abort();
}

} // namespace JSC::Yarr
