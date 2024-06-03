import sys
import argparse
from pathlib import Path
import shutil
import tempfile

sys.path.insert(0, str(Path(__file__).absolute().parent))
from init import download_file, extract


SEVENZIP_URL_TEMPLATE = 'https://www.7-zip.org/a/7z%s%s.exe'

SEVENZIP_URL_SUFFIX = {
    'x86_64': '-x64',
    'x86': '',
    'arm64': '-arm64',
}


def main(args) -> int:
    program_home = Path(__file__).absolute().parent

    sevenzip_dir = program_home / '7zip'
    sevenzip_dir.mkdir(exist_ok=True)

    downloads_dir = program_home / 'downloads'
    downloads_dir.mkdir(exist_ok=True)

    temp_dir = program_home / 'temp'
    if temp_dir.exists():
        shutil.rmtree(temp_dir)
    temp_dir.mkdir(exist_ok=True)

    new_7z_dir = temp_dir / '7zip'
    if new_7z_dir.exists():
        shutil.rmtree(new_7z_dir)
    new_7z_dir.mkdir(exist_ok=True)

    for arch, suffix in SEVENZIP_URL_SUFFIX.items():
        url = SEVENZIP_URL_TEMPLATE % (args.seven_zip_version, suffix)
        file_name = url.split('/')[-1]
        download_file(url, downloads_dir, file_name, use_cache=True)

        with tempfile.TemporaryDirectory(dir=temp_dir) as work_dir:
            work_dir = Path(work_dir)
            dst_dir = new_7z_dir / arch
            dst_dir.mkdir(exist_ok=True)
            extract(downloads_dir / file_name, work_dir, external_7z_program=args.external_7z_program)
            shutil.move(src=work_dir / '7z.exe', dst=dst_dir / f'7z.exe')
            shutil.move(src=work_dir / '7z.dll', dst=dst_dir / f'7z.dll')

    with (new_7z_dir / 'VERSION').open('w') as f:
        f.write(args.seven_zip_version)

    shutil.rmtree(sevenzip_dir)
    shutil.move(new_7z_dir, sevenzip_dir)
    shutil.rmtree(temp_dir)

    return 0


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--7z-version', dest='seven_zip_version', type=str, required=True)
    parser.add_argument('--external-7z-program', type=Path)
    sys.exit(main(parser.parse_args(sys.argv[1:])))
