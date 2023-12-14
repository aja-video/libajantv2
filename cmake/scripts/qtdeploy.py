"""
Calls the Qt deploy tool and parses the list of deployed runtime DLLs from the
JSON output of the command, then copies the DLLs to the CMake install directory
for the specified target executable.

Usage:
python3 qtdeploy.py <deployqt path> <target exe path> <cmake install path>

Optional: --debug
    - Deploy Qt debug DLLs instead of Release DLLs
"""
import json
import os
# import platform
import shutil
import subprocess
import sys
if __name__ == "__main__":
    if len(sys.argv) < 4:
        print("Usage: python3 qtdeploy.py <deployqt path> <target exe path> <cmake install path>")
        sys.exit(1)

    if len(sys.argv) == 5 and sys.argv[4] == "--debug":
        is_debug = True
    else:
        is_debug = False

    deployqt_path = os.path.normpath(sys.argv[1])
    if not os.path.exists(deployqt_path):
        print(f"Qt deploy tool not found: {deployqt_path}")
        sys.exit(1)
    exe_path = os.path.abspath(os.path.normpath(sys.argv[2]))
    if not os.path.exists(exe_path):
        print(f"Target exe path not found: {exe_path}")
        sys.exit(1)
    exe_dirname = os.path.dirname(exe_path)
    install_path = os.path.abspath(os.path.normpath(sys.argv[3]))
    if not os.path.exists(install_path):
        os.makedirs(install_path)

    qtdeploy_args = [deployqt_path, "--no-compiler-runtime", "--json", exe_path]
    if is_debug:
        qtdeploy_args.insert(1, "--debug")
    else:
        qtdeploy_args.insert(1, "--release")

    res = subprocess.run(qtdeploy_args, stdout=subprocess.PIPE, stderr=subprocess.PIPE, encoding="utf-8")
    if res.returncode == 0:
        blob = json.loads(res.stdout)
        if not blob or not "files" in blob:
            print("Invalid json parsed from Qt deploy command!")
            sys.exit(1)

        num_copied_files = 0
        num_deploy_files = len(blob["files"])
        if num_deploy_files == 0:
            print("No Qt deploy files specified!")
            sys.exit(1)

        for elem in blob["files"]:
            if not "source" in elem or not "target" in elem:
                print("Error parsing Qt deploy command JSON output!")
                sys.exit(1)
            src_path = os.path.abspath(elem["source"])
            if not os.path.exists(src_path):
                print(f"Qt deploy source path not found: {src_path}")
                sys.exit(1)
            tgt_path = os.path.abspath(elem["target"])
            qt_tgt_dir = tgt_path.replace(exe_dirname, "")
            install_tgt_dir = os.path.join(install_path, qt_tgt_dir[1:])
            src_file = os.path.basename(elem["source"])
            install_tgt_path = os.path.join(install_tgt_dir, src_file)
            print(f"src: {src_path}\ntgt: {install_tgt_path}\n")
            if not os.path.exists(install_tgt_dir):
                os.makedirs(install_tgt_dir)
            shutil.copyfile(src_path, install_tgt_path)
            if not os.path.exists(install_tgt_path):
                print(f"Error copying {src_path} to {install_tgt_path}")
                sys.exit(1)
            num_copied_files += 1

        if num_copied_files == num_deploy_files:
            print(f"Successfully copied all Qt deploy sources to {install_path} ({num_copied_files}/{num_deploy_files})")
        else:
            print(f"Error copying all Qt deploy sources ({num_copied_files}/{num_deploy_files})")
            sys.exit(1)
    else:
        print(f"Error calling Qt deploy tools! Exit code: {res.returncode}")

    sys.exit(res.returncode)
