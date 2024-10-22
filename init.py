import sys
import shutil
import subprocess
import logging
from pathlib import Path
import platform
import urllib.request
import json
from typing import Optional, List
import hashlib
import argparse
import os
import enum


ProgramHome = Path(__file__).absolute().parent
PythonRoot = ProgramHome / 'python'
ToolsDir = ProgramHome / 'tools'
DownloadsDir = ProgramHome / 'downloads'
TempDir = ProgramHome / 'temp'
BinDir = ProgramHome / 'bin'
BinAuxDir = ProgramHome / 'bin-aux'

PythonExecute = PythonRoot / 'python.exe'
ClangExecute = ToolsDir / 'clang_llvm' / 'bin' / 'clang.exe'
AuxSource = ProgramHome / 'scripts' / 'run-aux.c'


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


SEVENZIP = Path(__file__).absolute().parent / '7zip' / get_platform_machine() / '7z.exe'
CURL = Path(__file__).absolute().parent / 'curl' / get_platform_machine() / 'curl.exe'
curl_environ = os.environ.copy()


def download_file(url: str, download_path: Path, target_name: str, use_cache: bool = False, sha256: Optional[str] = None) -> bool:
    target_file = download_path / target_name
    if use_cache and target_file.exists() and (get_sha256(target_file) == sha256 if sha256 else True):
        logging.info(f'Skip download `{url}`, using cache `{target_file}`.')
    else:
        logging.info(f'Downloading `{url}` to `{target_file}`...')
        temp_download_path = download_path / (target_name + '.download')
        if args.use_request:
            urllib.request.urlretrieve(url, temp_download_path)
        else:
            subprocess.check_call([CURL, '-L', url, '-o', temp_download_path, '--silent'], env=curl_environ, stdout=subprocess.DEVNULL)
        if sha256:
            temp_sha256 = get_sha256(temp_download_path)
            if temp_sha256 != sha256:
                logging.error(f'Download `{url}` failed, sha256 is `{temp_sha256}`, excepted `{sha256}`.')
                return False
        shutil.move(src=temp_download_path, dst=target_file)
    return True


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


def do_enable_pip(proxy: Optional[str]):
    if (PythonRoot / 'Scripts' / 'pip.exe').exists():
        return
    pth_file = list(filter(lambda p: p.suffix == '._pth', PythonRoot.iterdir()))[0]
    with pth_file.open('r') as f:
        content = f.read()
    content = content.replace('#import site', 'import site')
    with pth_file.open('w') as f:
        f.write(content)
    success = download_file('https://bootstrap.pypa.io/get-pip.py', DownloadsDir, 'get-pip.py', use_cache=True)
    if success:
        subprocess.check_call([PythonRoot / 'python.exe', DownloadsDir / 'get-pip.py'] + (['--proxy', proxy] if proxy else []), stdout=subprocess.DEVNULL)


class ToolsConfig:
    def __init__(self, data: dict) -> None:
        self.data = data

    def tool_itmes(self):
        for tool, tool_info in self.data.items():
            select_tool_info = tool_info.copy()
            if 'source' in tool_info:
                url = tool_info['source'][get_platform_machine()]['url']
                version = select_tool_info.get('version')
                if version:
                    url = url.format(version=version)
                select_tool_info['url'] = url
                select_tool_info['sha256'] = tool_info['source'][get_platform_machine()]['sha256']
            yield tool, select_tool_info


def main(args):
    if args.verbose:
        logging.getLogger().setLevel(logging.INFO)

    # Setup proxy
    if args.proxy:
        logging.info(f'Setup proxy `{args.proxy}`')
        if args.use_request:
            proxy = urllib.request.ProxyHandler({'http': args.proxy, 'https': args.proxy})
            opener = urllib.request.build_opener(proxy)
            urllib.request.install_opener(opener)
        else:
            curl_environ['ALL_PROXY'] = args.proxy

    # Init
    init_dirs()

    if not PythonExecute.exists():
        subprocess.check_call([ProgramHome / 'get-python.bat'], stdout=subprocess.DEVNULL)

    do_enable_pip(proxy=args.proxy)

    with Path(ProgramHome / 'tools.json').open() as f:
        tools = ToolsConfig(json.load(f))

    # Setup `tools` directory
    for tool, tool_info in tools.tool_itmes():
        if (ToolsDir / tool).exists():
            continue

        url = tool_info.get('url')
        if url is None:
            continue

        tool_download_name = url.split('/')[-1]
        sha256 = tool_info['sha256']
        success = download_file(url, DownloadsDir, tool_download_name, use_cache=True, sha256=sha256)
        if not success:
            continue
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
    for tool, tool_info in tools.tool_itmes():
        install = tool_info.get('install')
        if install:
            install[0] = install[0].replace('$[python]', str(PythonExecute))
            subprocess.check_call(install, stdout=subprocess.DEVNULL)

    def setup_command_file(command: str, program: str):
        with (BinAuxDir / f'{command}.command').open('w+') as f:
            program = program.replace('$[python]', str(PythonExecute.relative_to(ProgramHome)))
            if program.startswith('^'):
                run_command = os.path.normpath(program[1:]).replace('\\', '/')
            else:
                run_command = f'tools/{tool}/{program}'
            f.write(f'{run_command}')

    def compile_file(source_file: Path, output_file: Path, object_files: Optional[List[Path]] = None, extra_flags: Optional[list] = None):
        subprocess.check_call([ClangExecute, source_file, '-Os', '-o', output_file] + (object_files if object_files else []) + (extra_flags if extra_flags else []))

    # Get objects
    object_files = []
    for source in ('run-aux-lib.c', 'arg2cmdline.c'):
        source_file = ProgramHome / 'scripts' / source
        object_file = TempDir / source_file.with_suffix('.o').name
        compile_file(source_file, object_file, extra_flags=['-c'])
        object_files.append(object_file)

    # Setup `bin-aux` directory
    class CommandKind(enum.Enum):
        RunCommandDirectly = 1
        RunBatch = 2
        CompileAndRunC = 3
    setup_commands = {}
    for tool, tool_info in tools.tool_itmes():
        for command, config in tool_info['command'].items():
            if isinstance(config, dict):
                program = config['program']
                if isinstance(config['script'], list):
                    script = '\n'.join(config['script']).replace('$[program]', program)
                    with (BinAuxDir / f'{command}.bat').open('w+') as f:
                        f.write(script + "\n")
                    setup_commands[command] = (CommandKind.RunBatch, None)
                elif isinstance(config['script'], str):
                    suffix = Path(config['script']).suffix.lower()
                    command_kind = {
                        '.c': CommandKind.CompileAndRunC,
                        '.bat': CommandKind.RunBatch,
                    }.get(suffix)
                    script_file = ProgramHome / config['script']
                    if command_kind is CommandKind.RunBatch:
                        shutil.copyfile(src=script_file, dst=BinAuxDir / f'{command}.bat')
                        setup_commands[command] = (CommandKind.RunBatch, None)
                    elif command_kind is CommandKind.CompileAndRunC:
                        setup_command_file(command=command, program=program)
                        output_file = TempDir / script_file.with_suffix('.exe').name
                        compile_file(script_file, output_file, object_files=object_files)
                        setup_commands[command] = (CommandKind.CompileAndRunC, output_file)
                    else:
                        assert False
                else:
                    assert False
            else:
                assert isinstance(config, str)
                setup_command_file(command=command, program=config)
                setup_commands[command] = (CommandKind.RunCommandDirectly, None)

    # Setup `bin` directory
    compile_file(AuxSource, TempDir / 'run-bat.exe',     extra_flags=['-DBATCH_MODE'],  object_files=object_files)
    compile_file(AuxSource, TempDir / 'run-command.exe', extra_flags=['-DCOMMAND_MODE'], object_files=object_files)
    for command, (kind, info) in setup_commands.items():
        if kind is CommandKind.RunCommandDirectly:
            shutil.copyfile(src=TempDir / 'run-command.exe', dst=BinDir / f'{command}.exe')
        elif kind is CommandKind.RunBatch:
            shutil.copyfile(src=TempDir / 'run-bat.exe', dst=BinDir / f'{command}.exe')
        elif kind is CommandKind.CompileAndRunC:
            shutil.copyfile(src=info, dst=BinDir / f'{command}.exe')

    shutil.rmtree(TempDir)

    return 0


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--verbose', '-v', action='store_true')
    parser.add_argument('--proxy', type=str)
    parser.add_argument('--use-request', action='store_true')
    args = parser.parse_args()
    sys.exit(main(args))
