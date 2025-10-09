import os
import pathlib
import shutil
import sys
import zipfile

# args
# output file
ZIP = sys.argv[1]
# build dir
BUILD_DIR = os.path.normpath(sys.argv[2])
# game js
JS = sys.argv[3]
# game wasm
WASM = sys.argv[4]
# game wasm
INDEX = sys.argv[5]

dir_name, _ = os.path.splitext(os.path.basename(ZIP))

# create dir structure
root_dir = pathlib.Path(BUILD_DIR) / "ZIP" / dir_name  # TODO: use temporary dir?

shutil.rmtree(root_dir, ignore_errors=True)

os.makedirs(str(root_dir), exist_ok=True)

shutil.copy(JS, root_dir)
shutil.copy(WASM, root_dir)
shutil.copy(INDEX, root_dir)

with zipfile.ZipFile(ZIP, "w", zipfile.ZIP_DEFLATED) as zipf:
    for root, dirs, files in os.walk(root_dir):
        for file in files:
            # Create the full file path
            file_path = os.path.join(root, file)
            # Add file to the ZIP archive, maintaining the directory structure
            zipf.write(file_path, os.path.relpath(file_path, root_dir))

# remove tmp dir
shutil.rmtree(root_dir)
