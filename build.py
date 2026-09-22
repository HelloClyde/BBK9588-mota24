"""Build from checked-in generated assets with the pinned SDK."""
import argparse, hashlib, json, os, subprocess, sys
from pathlib import Path
ROOT = Path(__file__).resolve().parent
SDK = ROOT / 'sdk'
def run(args, **kwargs):
    subprocess.run(args, check=True, cwd=ROOT, **kwargs)
def main():
    ap=argparse.ArgumentParser();ap.add_argument('--prefix');ap.add_argument('--regenerate',action='store_true');args=ap.parse_args()
    if not (SDK/'bda_packer').is_dir(): raise SystemExit('Run git submodule update --init sdk')
    if args.regenerate: run([sys.executable,str(ROOT/'src/generate.py')])
    output=ROOT/'build/Mota24.bda';output.parent.mkdir(exist_ok=True)
    env=os.environ.copy();env['PYTHONPATH']=str(SDK);env['PYTHONUTF8']='1'
    env['BDA_SDK_INCLUDE']=str(SDK/'sdk/include')
    cmd=[sys.executable,'-m','bda_packer',str(ROOT/'src/mota24.c'),'--title','魔塔24层','--category','4','-o',str(output)]
    if args.prefix: cmd+=['--prefix',args.prefix]
    run(cmd,env=env)
    run([sys.executable,'-m','bda_packer.validate',str(output)],env=env)
    revision=subprocess.check_output(['git','-C',str(SDK),'rev-parse','HEAD'],text=True).strip()
    (ROOT/'build/build-info.json').write_text(json.dumps({'version':(ROOT/'VERSION').read_text().strip(),'sdk_commit':revision,'sha256':hashlib.sha256(output.read_bytes()).hexdigest()},indent=2)+'\n',encoding='utf8')
if __name__=='__main__': main()
