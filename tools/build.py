import sys

sys.dont_write_bytecode = True

import argparse
import importlib.util
import os
import subprocess

try:
    Import("env")
    _PLATFORMIO_ENV = env
except Exception:
    _PLATFORMIO_ENV = None


def _import_from_path(module_name, file_path):
    """Import module from absolute file path."""
    spec = importlib.util.spec_from_file_location(module_name, file_path)
    module = importlib.util.module_from_spec(spec)
    sys.modules[module_name] = module
    spec.loader.exec_module(module)
    return module


def detect_project_dir():
    if "_PROJECT_DIR_OVERRIDE" in globals() and _PROJECT_DIR_OVERRIDE:
        return _PROJECT_DIR_OVERRIDE
    if _PLATFORMIO_ENV:
        return _PLATFORMIO_ENV["PROJECT_DIR"]
    return os.path.abspath(os.getcwd())


def build_assets(targets=None):
    project_dir = detect_project_dir()
    tools_dir = os.path.join(project_dir, "tools")
    
    if not os.path.isdir(project_dir):
        print(f"[build] Error: Project directory not found: {project_dir}")
        return False

    if tools_dir not in sys.path:
        sys.path.insert(0, tools_dir)

    run_fonts = not targets or "fonts" in targets
    run_icons = not targets or "icons" in targets
    
    success = True

    verbose = os.environ.get("PIPGUI_TOOLS_VERBOSE", "0").strip().lower() in ("1", "true", "yes", "on")

    def _ensure_pip_available():
        try:
            p = subprocess.run([sys.executable, "-m", "pip", "--version"], capture_output=True, text=True)
            return p.returncode == 0
        except Exception:
            return False

    def _ensure_pip_packages(packages):
        missing = []
        for p in packages:
            try:
                __import__(p)
            except Exception:
                missing.append(p)

        if not missing:
            return True

        if not _ensure_pip_available():
            try:
                import ensurepip

                if verbose:
                    print("[tools] bootstrapping pip")
                ensurepip.bootstrap(upgrade=True)
            except Exception as e:
                print(f"[tools] pip is not available and ensurepip failed: {e}")
                return False

        print(f"[tools] python deps: installing {', '.join(missing)}")
        try:
            cmd = [sys.executable, "-m", "pip", "install"] + (missing if verbose else (missing + ["-q"]))
            p = subprocess.run(cmd, capture_output=not verbose, text=True)
            if p.returncode != 0:
                if not verbose:
                    err = (p.stderr or "").strip()
                    if err:
                        print(f"[tools] pip failed: {err[:400]}")
                return False
            return True
        except Exception as e:
            print(f"[tools] pip failed: {e}")
            return False

    if run_fonts:
        try:
            fonts_gen_path = os.path.join(tools_dir, "fonts", "bin", "generate.py")
            fonts_mod = _import_from_path("fonts_generate", fonts_gen_path)
            if hasattr(fonts_mod, "is_up_to_date") and fonts_mod.is_up_to_date(project_dir, announce=True):
                pass
            elif not fonts_mod.generate_all(project_dir):
                success = False
        except Exception as e:
            if verbose:
                print(f"[build] Font generation failed: {e}")
            success = False

    if run_icons:
        try:
            icons_gen_path = os.path.join(tools_dir, "icons", "bin", "generate.py")
            icons_mod = _import_from_path("icons_generate", icons_gen_path)
            if hasattr(icons_mod, "is_up_to_date") and icons_mod.is_up_to_date(project_dir, announce=True):
                pass
            else:
                if not _ensure_pip_packages(["svgpathtools"]):
                    success = False
                elif not icons_mod.generate_all(project_dir):
                    success = False
        except Exception as e:
            if verbose:
                print(f"[build] Icon generation failed: {e}")
            success = False

    if success:
        print("[build] ok")
    else:
        print("[build] failed")
    
    return success


def main():
    parser = argparse.ArgumentParser(description="Build PSDF assets (fonts and icons)")
    parser.add_argument("--fonts", "--fonts-only", action="store_true", help="Generate only fonts")
    parser.add_argument("--icons", "--icons-only", action="store_true", help="Generate only icons")
    parser.add_argument("--project-dir", type=str, default=None, help="Override project directory")
    parser.add_argument("-v", "--verbose", action="store_true", help="Verbose output")
    args = parser.parse_args()

    targets = []
    if args.fonts:
        targets.append("fonts")
    if args.icons:
        targets.append("icons")
    
    if args.project_dir:
        global _PROJECT_DIR_OVERRIDE
        _PROJECT_DIR_OVERRIDE = args.project_dir

    if args.verbose:
        os.environ["PIPGUI_TOOLS_VERBOSE"] = "1"

    success = build_assets(targets if targets else None)
    sys.exit(0 if success else 1)


if __name__ == "__main__":
    main()

if _PLATFORMIO_ENV:
    build_assets()
