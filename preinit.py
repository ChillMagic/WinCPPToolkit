import sys
import argparse
from pathlib import Path
import shutil
import tempfile
from typing import Optional

sys.path.insert(0, str(Path(__file__).absolute().parent))
from init import download_file, extract


URL_TEMPLATE = {
    '7zip': 'https://www.7-zip.org/a/7z{version}{suffix}.exe',
    'curl': 'https://curl.se/windows/dl-{version}/curl-{version}-{suffix}-mingw.zip',
}

URL_SUFFIX = {
    '7zip': {
        'x86_64': '-x64',
        'x86': '',
        'arm64': '-arm64',
    },
    'curl': {
        'x86_64': 'win64',
        'x86': 'win32',
        'arm64': 'win64a',
    },
}

FILES_MAP = {
    '7zip': {
        '7z.exe': '7z.exe',
        '7z.dll': '7z.dll',
    },
    'curl': {
        'bin/curl.exe': 'curl.exe',
        'bin/curl-ca-bundle.crt': 'curl-ca-bundle.crt',
    },
}


def setup_tool(tool_name: str, version: str, program_home: Path, temp_dir: Path, downloads_dir: Path, external_7z_program: Optional[Path] = None):
    tool_install_dir = program_home / tool_name
    tool_install_dir.mkdir(exist_ok=True)

    tool_temp_dir = temp_dir / tool_name
    if tool_temp_dir.exists():
        shutil.rmtree(tool_temp_dir)
    tool_temp_dir.mkdir(exist_ok=True)

    for arch, suffix in URL_SUFFIX[tool_name].items():
        url = URL_TEMPLATE[tool_name].format(version=version, suffix=suffix)
        file_name = url.split('/')[-1]
        download_file(url, downloads_dir, file_name, use_cache=True)

        with tempfile.TemporaryDirectory(dir=temp_dir) as work_dir:
            work_dir = Path(work_dir)
            dst_dir = tool_temp_dir / arch
            dst_dir.mkdir(exist_ok=True)
            extract(downloads_dir / file_name, work_dir, external_7z_program=external_7z_program)
            for src, dst in FILES_MAP[tool_name].items():
                subdirs = list(work_dir.iterdir())
                if len(subdirs) == 1 and subdirs[0].is_dir():
                    src_dir = subdirs[0]
                else:
                    src_dir = work_dir
                shutil.move(src=src_dir / src, dst=dst_dir / dst)

    with (tool_temp_dir / 'VERSION').open('w') as f:
        f.write(version)

    shutil.rmtree(tool_install_dir)
    shutil.move(tool_temp_dir, tool_install_dir)


def main(args) -> int:
    program_home = Path(__file__).absolute().parent

    downloads_dir = program_home / 'downloads'
    downloads_dir.mkdir(exist_ok=True)

    temp_dir = program_home / 'temp'
    if temp_dir.exists():
        shutil.rmtree(temp_dir)
    temp_dir.mkdir(exist_ok=True)

    if args.seven_zip_version:
        setup_tool(tool_name='7zip', version=args.seven_zip_version, program_home=program_home, temp_dir=temp_dir, downloads_dir=downloads_dir, external_7z_program=args.external_7z_program)

    if args.curl_version:
        setup_tool(tool_name='curl', version=args.curl_version, program_home=program_home, temp_dir=temp_dir, downloads_dir=downloads_dir, external_7z_program=args.external_7z_program)

    shutil.rmtree(temp_dir)

    return 0


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--7z-version', dest='seven_zip_version', type=str)
    parser.add_argument('--curl-version', dest='curl_version', type=str)
    parser.add_argument('--external-7z-program', type=Path)
    sys.exit(main(parser.parse_args(sys.argv[1:])))
