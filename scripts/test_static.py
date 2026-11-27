from pathlib import Path
root = Path(__file__).resolve().parents[1]
source = (root / "src" / "QuickKeyAI.cpp").read_text()
assert "OfxGetNumberOfPlugins" in source
assert "OfxGetPlugin" in source
assert "kOfxImageEffectActionRender" in source
assert "dst[3] = 0" in source
assert "QuickKey AI - Foundation Test" in source
assert "torch.load" not in source
assert (root / "Info.plist.in").exists()
print("Source/package sanity checks passed (no Resolve/Mac runtime executed).")
