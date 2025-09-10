import os
import subprocess
from datetime import datetime
from SCons.Script import Import

Import("env")

def after_build(source, target, env):
    print("📦 Post-build script running")

    # Directories and filenames
    project_dir = env["PROJECT_DIR"]
    build_dir = os.path.join(project_dir, ".pio", "build", env["PIOENV"])
    ota_dir = os.path.join(project_dir, "OTAbuilds")
    os.makedirs(ota_dir, exist_ok=True)

    # Input files
    app_bin = os.path.join(build_dir, "firmware.bin")
    bootloader_bin = os.path.join(project_dir, "bootloader", "bootloader.bin")
    partition_bin = os.path.join(project_dir, "partition_table", "partition-table.bin")

    # Timestamp for filenames
    timestamp = datetime.now().strftime("%Y%m%d-%H%M%S")
    ota_bin_out = os.path.join(ota_dir, f"firmware_{timestamp}.bin")
    merged_bin_out = os.path.join(ota_dir, f"merged_firmware_{timestamp}.bin")

    print(f"📦 Timestamp: {timestamp}")
    print(f"📄 App binary: {app_bin}")
    print(f"📄 Bootloader: {bootloader_bin}")
    print(f"📄 Partition:  {partition_bin}")

    # Verify inputs exist
    if not all(os.path.exists(p) for p in [app_bin, bootloader_bin, partition_bin]):
        print("❌ Required binary not found. Skipping merge.")
        return

    # Copy OTA binary with timestamp
    try:
        import shutil
        shutil.copy2(app_bin, ota_bin_out)
        print(f"✅ OTA firmware copied to {ota_bin_out}")
    except Exception as e:
        print(f"❌ Error copying OTA binary: {e}")
        return

    # Merge binary using esptool.py
    try:
        subprocess.run([
            "esptool.py",
            "--chip", "esp32",
            "merge_bin",
            "-o", merged_bin_out,
            "0x1000", bootloader_bin,
            "0x8000", partition_bin,
            "0x10000", app_bin
        ], check=True)
        print(f"✅ Merged firmware created at {merged_bin_out}")
    except subprocess.CalledProcessError as e:
        print(f"❌ esptool.py failed: {e}")

# Attach to firmware.bin specifically so this always runs after a real build
env.AddPostAction("$BUILD_DIR/firmware.bin", after_build)
