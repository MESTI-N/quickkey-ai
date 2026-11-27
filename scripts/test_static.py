from pathlib import Path
root = Path(__file__).resolve().parents[1]
source = (root / "src" / "QuickKeyAI.cpp").read_text()
worker = (root / "scripts" / "quickkey-worker.sh").read_text()
assert "OfxGetNumberOfPlugins" in source
assert "kOfxActionInstanceChanged" in source
assert "Process Shot with AI" in source
assert "LoadEXR" in source
assert "getChoice" in source
assert "--outputs cutout --format exr" in worker
assert ".quickkey_ready" in worker
assert "torch.load" not in source
assert (root / "Info.plist.in").exists()
print("Source and render pipeline static tests passed. Host/AI integration unverified.")
