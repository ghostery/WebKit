# Plan: Homebrew distribution of validate-dnr-rules

## Goal

Distribute `validate-dnr-rules` as a Homebrew package so Ghostery developers (and any Safari extension developer) can validate DNR rulesets without building WebKit from source.

## Architecture

The tool is a small ObjC++ binary that links against WebKit.framework at runtime. WebKit.framework depends on JavaScriptCore, WebCore, WebKitLegacy, and libwebrtc. All other dependencies are macOS system frameworks.

The distributable package is:
```
validate-dnr-rules/
├── bin/validate-dnr-rules        # compiled binary
├── run.sh                        # wrapper that sets DYLD_FRAMEWORK_PATH
└── Frameworks/
    ├── WebKit.framework/
    ├── WebKitLegacy.framework/
    ├── JavaScriptCore.framework/
    ├── WebCore.framework/
    └── libwebrtc.dylib
```

The wrapper script sets `DYLD_FRAMEWORK_PATH=<prefix>/Frameworks` so the locally-bundled frameworks override the system ones.

## Steps

### 1. Build Release WebKit (one-time, on an arm64 Mac)

```bash
Tools/Scripts/build-webkit --release
```

This produces optimized frameworks without debug symbols. Expected total size ~200-300MB (vs 1.2GB debug).

### 2. Create a packaging script

`Tools/Scripts/package-validate-dnr-rules`:
- Compiles the tool binary against Release frameworks
- Strips debug symbols from frameworks (`strip -S`)
- Removes unnecessary framework contents (Headers, PrivateHeaders, Modules — not needed at runtime)
- Copies only the required frameworks
- Creates the wrapper script
- Tarballs the result

The wrapper script (`run.sh`) should be:
```bash
#!/bin/bash
DIR="$(cd "$(dirname "$0")" && pwd)"
DYLD_FRAMEWORK_PATH="$DIR/Frameworks" exec "$DIR/bin/validate-dnr-rules" "$@"
```

### 3. Measure and optimize size

After stripping, check size. If still too large (>100MB), consider:
- Removing WebInspectorUI.framework resources from WebKit.framework (not needed)
- Removing XPCServices from WebKit.framework (NetworkProcess, WebContent — not needed for translation-only mode)
- If `--compile` is dropped as a feature, we might only need the translator code, which could be extracted into a much smaller library

### 4. Upload to GitHub Releases

On the ghostery/WebKit repo:
```bash
gh release create v1.0.0 validate-dnr-rules-arm64.tar.gz --title "validate-dnr-rules v1.0.0" --notes "..."
```

### 5. Create Homebrew tap

Create repo `ghostery/homebrew-tools` with a formula:

```ruby
class ValidateDnrRules < Formula
  desc "Validate Safari Declarative Net Request rulesets using WebKit's native translator"
  homepage "https://github.com/ghostery/WebKit"
  url "https://github.com/ghostery/WebKit/releases/download/v1.0.0/validate-dnr-rules-arm64.tar.gz"
  sha256 "..."
  license "BSD-2-Clause"

  depends_on :macos
  depends_on arch: :arm64  # Intel users run via Rosetta

  def install
    libexec.install "bin", "Frameworks"
    # Install wrapper as the main binary
    (bin/"validate-dnr-rules").write <<~EOS
      #!/bin/bash
      DYLD_FRAMEWORK_PATH="#{libexec}/Frameworks" exec "#{libexec}/bin/validate-dnr-rules" "$@"
    EOS
  end

  test do
    # Minimal DNR ruleset that should pass
    (testpath/"rules.json").write '[{"id":1,"priority":1,"action":{"type":"block"},"condition":{"urlFilter":"test"}}]'
    assert_match "Rules translated: 1", shell_output("#{bin}/validate-dnr-rules #{testpath}/rules.json")
  end
end
```

Users install with:
```bash
brew tap ghostery/tools
brew install validate-dnr-rules
```

### 6. CI automation (optional, future)

Add a GitHub Action on ghostery/WebKit that:
- Triggers on tags matching `validate-dnr-v*`
- Builds WebKit in Release on macOS arm64 runner
- Runs the packaging script
- Uploads the tarball to GitHub Releases
- Updates the Homebrew formula SHA

## Open questions

- **Size budget**: What's acceptable? Homebrew bottles for large packages (e.g., qt, llvm) can be 100-500MB. WebKit frameworks will likely land in that range.
- **macOS version support**: The built frameworks target a specific macOS SDK. Need to decide minimum supported macOS version (probably 14.0 Sonoma, when DNR support was added).
- **Release cadence**: Rebuild when WebKit trunk changes in ways that affect DNR? Or pin to specific WebKit versions?
- **Compile mode**: Is `--compile` worth the extra framework weight? Translation-only validation catches the most important errors and might need fewer framework components.
