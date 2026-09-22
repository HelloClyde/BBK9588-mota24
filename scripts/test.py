"""Host rules, UI/input and rendering regressions; requires GCC."""
import argparse,subprocess,os
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
ap=argparse.ArgumentParser();ap.add_argument('--cc',default='gcc');args=ap.parse_args()
out=ROOT/'build/tests';out.mkdir(parents=True,exist_ok=True)
for name in ['test_core','test_ui','test_visual']:
    exe=out/(name+('.exe' if os.name=='nt' else ''))
    subprocess.run([args.cc,'-w','-std=c99','-O2','-fno-builtin',str(ROOT/'src'/ (name+'.c')),'-I',str(ROOT/'sdk/sdk/include'),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True,cwd=ROOT)
