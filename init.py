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
import os


ProgramHome = Path(__file__).absolute().parent
PythonRoot = ProgramHome / 'python'
ToolsDir = ProgramHome / 'tools'
DownloadsDir = ProgramHome / 'downloads'
TempDir = ProgramHome / 'temp'
BinDir = ProgramHome / 'bin'
BinAuxDir = ProgramHome / 'bin-aux'

PythonExecute = PythonRoot / 'python.exe'
ClangExecute = ToolsDir / 'clang_llvm' / 'bin' / 'clang.exe'


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


def init_dirs():
    ToolsDir.mkdir(exist_ok=True)
    DownloadsDir.mkdir(exist_ok=True)

    if TempDir.exists():
        shutil.rmtree(TempDir)
    TempDir.mkdir(exist_ok=True)

    if BinDir.exists():
        shutil.rmtree(BinDir)
    BinDir.mkdir(exist_ok=True)

    if BinAuxDir.exists():
        shutil.rmtree(BinAuxDir)
    BinAuxDir.mkdir(exist_ok=True)


def do_enable_pip():
    if (PythonRoot / 'Scripts' / 'pip.exe').exists():
        return
    pth_file = list(filter(lambda p: p.suffix == '._pth', PythonRoot.iterdir()))[0]
    with pth_file.open('r') as f:
        content = f.read()
    content = content.replace('#import site', 'import site')
    with pth_file.open('w') as f:
        f.write(content)
    download_file('https://bootstrap.pypa.io/get-pip.py', DownloadsDir, 'get-pip.py', use_cache=True)
    subprocess.check_call([PythonRoot / 'python.exe', DownloadsDir / 'get-pip.py'], stdout=subprocess.DEVNULL)


def main(args):
    if args.verbose:
        logging.getLogger().setLevel(logging.INFO)

    init_dirs()

    if not PythonExecute.exists():
        subprocess.check_call([ProgramHome / 'get-python.bat'], stdout=subprocess.DEVNULL)

    do_enable_pip()

    with Path(ProgramHome / 'tools.json').open() as f:
        tools = json.load(f)

    # Setup `tools` directory
    for tool, tool_info in tools[get_platform_machine()].items():
        if (ToolsDir / tool).exists():
            continue

        url = tool_info.get('url')
        if url is None:
            continue

        tool_download_name = url.split('/')[-1]
        sha256 = tool_info['sha256']
        download_file(url, DownloadsDir, tool_download_name, use_cache=True, sha256=sha256)
        extract_dir = TempDir / f'{tool}.temp'
        extract(DownloadsDir / tool_download_name, extract_dir)
        if tool_info.get('nested_tar'):
            tar_file = extract_dir / (DownloadsDir / tool_download_name).with_suffix('').name
            extract(tar_file, extract_dir)
            tar_file.unlink()
        if tool_info.get('use_subdir'):
            shutil.move(src=extract_dir / list(extract_dir.iterdir())[0], dst=ToolsDir / tool)
        else:
            shutil.move(src=extract_dir, dst=ToolsDir / tool)

    # Run `install`
    for tool, tool_info in tools[get_platform_machine()].items():
        install = tool_info.get('install')
        if install:
            install[0] = install[0].replace('$[python]', str(PythonExecute))
            subprocess.check_call(install, stdout=subprocess.DEVNULL)

    # Setup `bin-aux` directory
    for tool, tool_info in tools[get_platform_machine()].items():
        for command, config in tool_info['command'].items():
            if isinstance(config, dict):
                program = config['program']
                script = '\n'.join(config['script']).replace('$[program]', program)
                with (BinAuxDir / f'{command}.bat').open('w+') as f:
                    f.write(script + "\n")
            else:
                with (BinAuxDir / f'{command}.command').open('w+') as f:
                    assert isinstance(config, str)
                    program = config
                    program = program.replace('$[python]', str(PythonExecute.relative_to(ProgramHome)))
                    if program.startswith('^'):
                        run_command = os.path.normpath(program[1:]).replace('\\', '/')
                    else:
                        run_command = f'tools/{tool}/{program}'
                    f.write(f'{run_command}')

    # Setup `bin` directory
    subprocess.check_call([ClangExecute, ProgramHome / 'scripts' / 'run-aux.c', '-DBATCH_MODE', '-o', TempDir / 'run-bat.exe', '-O3'])
    subprocess.check_call([ClangExecute, ProgramHome / 'scripts' / 'run-aux.c', '-DCOMMAND_MODE', '-o', TempDir / 'run-command.exe', '-O3'])
    for bat in BinAuxDir.iterdir():
        if bat.suffix == '.command':
            command = bat.with_suffix('').name
            shutil.copyfile(src=TempDir / 'run-command.exe', dst=BinDir / f'{command}.exe')
        elif bat.suffix == '.bat':
            command = bat.with_suffix('').name
            shutil.copyfile(src=TempDir / 'run-bat.exe', dst=BinDir / f'{command}.exe')

    shutil.rmtree(TempDir)

    return 0


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--verbose', '-v', action='store_true')
    args = parser.parse_args()
    sys.exit(main(args))
