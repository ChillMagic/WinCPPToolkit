import sys
import shutil
import subprocess
import logging
from pathlib import Path
import platform
import urllib.request
import json
from typing import Optional
import hashlib
import argparse


def get_platform_machine() -> str:
    machine = platform.machine().lower()
    result = {
        'aarch64': 'arm64',
        'arm64': 'arm64',
        'amd64': 'x86_64',
        'x86_64': 'x86_64',
        'i386': 'x86',
        'i686': 'x86',
        'x86': 'x86',
    }.get(machine)
    if result is None:
        logging.error(f'Unknown machine type `{machine}`.')
        return 'unknown'
    return result


SEVENZIP = Path(__file__).absolute().parent / '7zip' / get_platform_machine() / f'7z.exe'


def download_file(url: str, download_path: Path, target_name: str, use_cache: bool = False, sha256: Optional[str] = None):
    target_file = download_path / target_name
    if use_cache and target_file.exists() and (get_sha256(target_file) == sha256 if sha256 else True):
        logging.info(f'Skip download `{url}`, using cache `{target_file}`.')
    else:
        logging.info(f'Downloading `{url}` to `{target_file}`...')
        temp_download_path = download_path / (target_name + '.download')
        urllib.request.urlretrieve(url, temp_download_path)
        if sha256:
            temp_sha256 = get_sha256(temp_download_path)
            if temp_sha256 != sha256:
                logging.error(f'Download `{url}` failed, sha256 is `{temp_sha256}`, excepted `{sha256}`.')
                return 1
        shutil.move(src=temp_download_path, dst=target_file)


def extract(archive_file: Path, output_path: Path, external_7z_program: Optional[Path] = None):
    logging.info(f'Extracting `{archive_file}` to `{output_path}`...')
    subprocess.check_call([
        external_7z_program if external_7z_program else SEVENZIP, 'x', archive_file, f'-o{output_path}', '-y'], stdout=subprocess.DEVNULL)


def get_sha256(file: Path) -> str:
    with file.open('rb') as f:
        return hashlib.sha256(f.read()).hexdigest()


def main(args):
    if args.verbose:
        logging.getLogger().setLevel(logging.INFO)

    program_home = Path(__file__).absolute().parent

    tools_dir = program_home / 'tools'
    tools_dir.mkdir(exist_ok=True)

    downloads_dir = program_home / 'downloads'
    downloads_dir.mkdir(exist_ok=True)

    temp_dir = program_home / 'temp'
    if temp_dir.exists():
        shutil.rmtree(temp_dir)
    temp_dir.mkdir(exist_ok=True)

    bin_dir = program_home / 'bin'
    bin_dir.mkdir(exist_ok=True)

    with Path('tools.json').open() as f:
        tools = json.load(f)

    # Setup `tools` directory
    for tool, tool_info in tools[get_platform_machine()].items():
        if (tools_dir / tool).exists():
            continue

        url = tool_info['url']
        tool_download_name = url.split('/')[-1]
        sha256 = tool_info['sha256']
        download_file(url, downloads_dir, tool_download_name, use_cache=True, sha256=sha256)
        extract_dir = temp_dir / f'{tool}.temp'
        extract(downloads_dir / tool_download_name, extract_dir)
        if tool_info.get('nested_tar'):
            tar_file = extract_dir / (downloads_dir / tool_download_name).with_suffix('').name
            extract(tar_file, extract_dir)
            tar_file.unlink()
        if tool_info.get('use_subdir'):
            shutil.move(src=extract_dir / list(extract_dir.iterdir())[0], dst=tools_dir / tool)
        else:
            shutil.move(src=extract_dir, dst=tools_dir / tool)

    shutil.rmtree(temp_dir)

    # Setup `bin` directory
    for tool, tool_info in tools[get_platform_machine()].items():
        for command, path in tool_info['command'].items():
            with (bin_dir / f'{command}.bat').open('w+') as f:
                f.write(f'@echo off\n"%~dp0../tools/{tool}/{path}" %*\n')

    return 0


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--verbose', '-v', action='store_true')
    args = parser.parse_args()
    sys.exit(main(args))
