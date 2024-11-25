#!/usr/bin/env python3

import os
import sys
import subprocess
import argparse
import json
from pathlib import Path

# Required nRF libraries and their versions
REQUIRED_LIBS = {
    'nrf_modem_lib': '2.4.0',
    'at_monitor': '2.4.0',
    'at_cmd_parser': '2.4.0',
    'lte_link_control': '2.4.0',
    'modem_info': '2.4.0',
    'modem_key_mgmt': '2.4.0'
}

def check_environment():
    """Check if required environment variables are set"""
    required_vars = ['ZEPHYR_BASE', 'GNUARMEMB_TOOLCHAIN_PATH']
    missing = [var for var in required_vars if not os.getenv(var)]
    if missing:
        print(f"Error: Missing environment variables: {', '.join(missing)}")
        sys.exit(1)

def verify_nrf_sdk():
    """Verify nRF Connect SDK installation"""
    nrf_path = Path(os.getenv('ZEPHYR_BASE')).parent / 'nrf'
    if not nrf_path.exists():
        print("Error: nRF Connect SDK not found")
        sys.exit(1)
    
    print(f"Found nRF Connect SDK at: {nrf_path}")
    return nrf_path

def check_library_versions(nrf_path):
    """Check versions of required nRF libraries"""
    print("\nChecking library versions:")
    all_ok = True
    
    for lib, required_version in REQUIRED_LIBS.items():
        lib_path = nrf_path / 'lib' / lib
        if not lib_path.exists():
            print(f"❌ {lib}: Not found")
            all_ok = False
            continue
            
        # Try to get version from CMakeLists.txt
        cmake_path = lib_path / 'CMakeLists.txt'
        if cmake_path.exists():
            with open(cmake_path) as f:
                content = f.read()
                if f'VERSION {required_version}' in content:
                    print(f"✓ {lib}: {required_version}")
                else:
                    print(f"❌ {lib}: Version mismatch")
                    all_ok = False
        else:
            print(f"⚠ {lib}: Cannot determine version")
            all_ok = False
    
    return all_ok

def verify_modem_firmware():
    """Verify modem firmware version and status"""
    print("\nChecking modem firmware:")
    
    try:
        # Use nrfjprog to check modem firmware
        result = subprocess.run(['nrfjprog', '--com', '--memrd', '0x00', '0x100'],
                              capture_output=True, text=True)
        if result.returncode == 0:
            print("✓ Modem firmware accessible")
            # Parse version information
            if 'mfw_nrf9160' in result.stdout:
                print("✓ Modem firmware version verified")
            else:
                print("⚠ Unexpected modem firmware version")
        else:
            print("❌ Cannot access modem firmware")
            return False
    except FileNotFoundError:
        print("❌ nrfjprog not found")
        return False
    
    return True

def check_build_dependencies():
    """Verify build dependencies"""
    print("\nChecking build dependencies:")
    
    dependencies = {
        'cmake': 'cmake --version',
        'ninja': 'ninja --version',
        'dtc': 'dtc --version',
        'gperf': 'gperf --version',
        'python3': 'python3 --version'
    }
    
    all_ok = True
    for dep, cmd in dependencies.items():
        try:
            subprocess.run(cmd.split(), capture_output=True, check=True)
            print(f"✓ {dep}")
        except (subprocess.CalledProcessError, FileNotFoundError):
            print(f"❌ {dep} not found")
            all_ok = False
    
    return all_ok

def verify_project_config():
    """Verify project configuration files"""
    print("\nChecking project configuration:")
    
    files_to_check = {
        'prj.conf': ['CONFIG_NRF_MODEM_LIB=y', 'CONFIG_LTE_LINK_CONTROL=y'],
        'CMakeLists.txt': ['nrf_modem_lib', 'lte_link_control'],
        'west.yml': ['nrf', 'nrfxlib']
    }
    
    all_ok = True
    for file, required_content in files_to_check.items():
        if not Path(file).exists():
            print(f"❌ {file} not found")
            all_ok = False
            continue
            
        with open(file) as f:
            content = f.read()
            missing = [req for req in required_content if req not in content]
            if missing:
                print(f"❌ {file}: Missing required content: {', '.join(missing)}")
                all_ok = False
            else:
                print(f"✓ {file}")
    
    return all_ok

def update_modem_firmware(firmware_path=None):
    """Update modem firmware if needed"""
    if not firmware_path:
        print("No firmware path provided, skipping update")
        return True
    
    print("\nUpdating modem firmware:")
    try:
        cmd = ['nrfjprog', '--program', firmware_path, '--sectorerase']
        result = subprocess.run(cmd, capture_output=True, text=True)
        if result.returncode == 0:
            print("✓ Modem firmware updated successfully")
            return True
        else:
            print(f"❌ Firmware update failed: {result.stderr}")
            return False
    except FileNotFoundError:
        print("❌ nrfjprog not found")
        return False

def main():
    parser = argparse.ArgumentParser(description='Verify nRF library setup')
    parser.add_argument('--update-firmware', help='Path to modem firmware file')
    parser.add_argument('--verbose', action='store_true', help='Enable verbose output')
    args = parser.parse_args()

    print("nRF Library Verification Tool")
    print("============================")

    # Check environment
    check_environment()

    # Verify nRF SDK
    nrf_path = verify_nrf_sdk()

    # Check components
    libs_ok = check_library_versions(nrf_path)
    modem_ok = verify_modem_firmware()
    deps_ok = check_build_dependencies()
    config_ok = verify_project_config()

    # Update firmware if requested
    if args.update_firmware:
        firmware_ok = update_modem_firmware(args.update_firmware)
    else:
        firmware_ok = True

    # Summary
    print("\nVerification Summary:")
    print("--------------------")
    print(f"nRF Libraries: {'✓' if libs_ok else '❌'}")
    print(f"Modem Firmware: {'✓' if modem_ok else '❌'}")
    print(f"Build Dependencies: {'✓' if deps_ok else '❌'}")
    print(f"Project Configuration: {'✓' if config_ok else '❌'}")
    print(f"Firmware Update: {'✓' if firmware_ok else '❌'}")

    if all([libs_ok, modem_ok, deps_ok, config_ok, firmware_ok]):
        print("\n✓ All checks passed")
        sys.exit(0)
    else:
        print("\n❌ Some checks failed")
        sys.exit(1)

if __name__ == '__main__':
    main()
