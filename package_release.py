"""Create an installation ZIP and checksums from the validated BDA."""
import hashlib,json,shutil
from pathlib import Path
from zipfile import ZipFile,ZIP_DEFLATED
ROOT=Path(__file__).resolve().parent
version=(ROOT/'VERSION').read_text().strip()
source=ROOT/'build/Mota24.bda';info=json.loads((ROOT/'build/build-info.json').read_text())
assert info['sha256']==hashlib.sha256(source.read_bytes()).hexdigest(), 'Rebuild before packaging'
out=ROOT/'dist';out.mkdir(exist_ok=True)
bda=out/'Mota24.bda';shutil.copy2(source,bda)
shutil.copy2(ROOT/'build/build-info.json',out/'build-info.json')
archive=out/('BBK9588-mota24-'+version+'.zip')
with ZipFile(archive,'w',ZIP_DEFLATED) as z:
    z.write(bda,'应用/程序/Mota24.bda')
    for name in ['README.md','LICENSE','NOTICE','SOURCES.md','docs/VERIFICATION.md']:z.write(ROOT/name,name)
    z.write(out/'build-info.json','build-info.json')
    for screenshot in sorted((ROOT/'docs/screenshots').glob('*')):z.write(screenshot,screenshot.relative_to(ROOT).as_posix())
files=[bda,archive,out/'build-info.json']
(out/'SHA256SUMS.txt').write_text(''.join(hashlib.sha256(p.read_bytes()).hexdigest()+'  '+p.name+'\n' for p in files),encoding='ascii')
print(archive)
